"""
SDK Error Return Analyzer
=========================

Analyzes SDK C source code to extract API-to-error-code mappings.

This module:
1. Parses ble/app/*.c and ble/prf/*.c files
2. Identifies return statements that return error codes
3. Builds a reverse index: error_code -> [apis that may return it]

Author: B6x MCP Server Team
Version: 1.0.0
"""

import re
import json
import logging
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional
from dataclasses import dataclass, field, asdict

logger = logging.getLogger(__name__)


@dataclass
class ErrorApiMapping:
    """Mapping between an error code and an API that may return it."""
    error_name: str
    api_name: str
    condition: str = ""
    probability: str = "medium"  # high, medium, low
    source: str = "sdk_analysis"  # sdk_analysis, config_file


class ErrorReturnAnalyzer:
    """
    Analyzes SDK source code to extract error code to API mappings.

    The analyzer:
    1. Parses C source files in ble/app/ and ble/prf/
    2. Looks for return statements with error code constants
    3. Builds a reverse mapping (error -> apis)
    """

    # Common BLE error code patterns to search for
    ERROR_PATTERNS = [
        # GAP errors
        r'GAP_ERR_\w+',
        # ATT errors
        r'ATT_ERR_\w+',
        # L2C errors
        r'L2C_ERR_\w+',
        # GATT errors
        r'GATT_ERR_\w+',
        # SMP errors
        r'SMP_ERR_\w+',
        # PRF errors
        r'PRF_ERR_\w+',
        # PRF status
        r'PRF_\w+',
        # LL errors
        r'LL_ERR_\w+',
        # Common error constants
        r'BLE_ERR_\w+',
        r'COMMON_ERR_\w+',
        # Status codes
        r'BLE_STATUS_\w+',
        r'KE_STATUS_\w+',
    ]

    # API function patterns
    API_PATTERNS = [
        r'^(gapc|gapm|gattc|gatts|l2cc|smpc|smps|prf|ble)_\w+$',
    ]

    def __init__(self, sdk_path: str):
        """
        Initialize the analyzer.

        Args:
            sdk_path: Path to the SDK root directory
        """
        self.sdk_path = Path(sdk_path)
        self.error_to_apis: Dict[str, List[ErrorApiMapping]] = {}
        self.api_to_errors: Dict[str, Set[str]] = {}

    def analyze_all(self) -> Dict[str, List[Dict]]:
        """
        Analyze all BLE source files and build error-to-API mappings.

        Returns:
            Dictionary mapping error names to list of API info:
            {
                "GAP_ERR_TIMEOUT": [
                    {"api": "gapc_connect_req", "condition": "", "probability": "high"},
                    ...
                ],
                ...
            }
        """
        logger.info("Starting SDK error return analysis...")

        # Analyze BLE application layer
        app_path = self.sdk_path / "ble" / "app"
        if app_path.exists():
            self._analyze_directory(app_path, "ble/app")

        # Analyze BLE profiles
        prf_path = self.sdk_path / "ble" / "prf"
        if prf_path.exists():
            self._analyze_directory(prf_path, "ble/prf")

        # Analyze BLE API layer (for message handlers)
        api_path = self.sdk_path / "ble" / "api"
        if api_path.exists():
            self._analyze_directory(api_path, "ble/api")

        logger.info(f"Analysis complete. Found {len(self.error_to_apis)} error codes.")

        return self._build_result()

    def _analyze_directory(self, directory: Path, module_name: str):
        """
        Analyze all C files in a directory.

        Args:
            directory: Directory path to analyze
            module_name: Module name for logging
        """
        logger.info(f"Analyzing {module_name}...")

        c_files = list(directory.glob("**/*.c"))
        logger.info(f"Found {len(c_files)} C source files")

        for c_file in c_files:
            try:
                self._analyze_file(c_file)
            except Exception as e:
                logger.warning(f"Failed to analyze {c_file}: {e}")

    def _analyze_file(self, file_path: Path):
        """
        Analyze a single C source file for error returns.

        Args:
            file_path: Path to the C source file
        """
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            logger.warning(f"Could not read {file_path}: {e}")
            return

        # Extract the current function context
        functions = self._extract_functions(content)

        for func_name, func_body in functions.items():
            # Skip if not an API function
            if not self._is_api_function(func_name):
                continue

            # Find error returns in this function
            error_returns = self._find_error_returns(func_body)

            for error_name in error_returns:
                self._add_mapping(error_name, func_name, file_path)

    def _extract_functions(self, content: str) -> Dict[str, str]:
        """
        Extract function definitions from C code.

        Args:
            content: C source code

        Returns:
            Dictionary mapping function names to their body content
        """
        functions = {}

        # Match function definitions
        # Pattern: return_type func_name(params) { body }
        # This is a simplified pattern; real C parsing is complex
        pattern = r'(?:static\s+)?(?:inline\s+)?\w+(?:\s*\*)?\s+(\w+)\s*\([^)]*\)\s*\{'

        matches = re.finditer(pattern, content)

        for match in matches:
            func_name = match.group(1)
            start_pos = match.end() - 1  # Position of opening brace

            # Find matching closing brace
            brace_count = 1
            pos = start_pos + 1

            while pos < len(content) and brace_count > 0:
                if content[pos] == '{':
                    brace_count += 1
                elif content[pos] == '}':
                    brace_count -= 1
                pos += 1

            if brace_count == 0:
                func_body = content[start_pos:pos]
                functions[func_name] = func_body

        return functions

    def _is_api_function(self, func_name: str) -> bool:
        """
        Check if a function is a BLE API function.

        Args:
            func_name: Function name

        Returns:
            True if it's a BLE API function
        """
        for pattern in self.API_PATTERNS:
            if re.match(pattern, func_name):
                return True

        # Also check for message handlers
        if func_name.endswith('_handler') or func_name.endswith('_msg_handler'):
            return True

        return False

    def _find_error_returns(self, func_body: str) -> Set[str]:
        """
        Find all error code names in return statements.

        Args:
            func_body: Function body content

        Returns:
            Set of error code names found
        """
        errors = set()

        # Match return statements with error constants
        return_pattern = r'return\s+(\w+(?:_\w+)*);'

        for match in re.finditer(return_pattern, func_body):
            return_value = match.group(1)

            # Check if it's an error constant
            for error_pattern in self.ERROR_PATTERNS:
                if re.match(error_pattern, return_value):
                    errors.add(return_value)
                    break

        # Also check for direct error code assignments to status variables
        status_pattern = r'status\s*=\s*(\w+(?:_\w+)*)'
        for match in re.finditer(status_pattern, func_body):
            status_value = match.group(1)
            for error_pattern in self.ERROR_PATTERNS:
                if re.match(error_pattern, status_value):
                    errors.add(status_value)
                    break

        return errors

    def _add_mapping(self, error_name: str, api_name: str, source_file: Path):
        """
        Add an error-to-API mapping.

        Args:
            error_name: Error code name
            api_name: API function name
            source_file: Source file where found
        """
        # Determine probability based on context
        probability = self._estimate_probability(error_name, api_name, source_file)

        mapping = ErrorApiMapping(
            error_name=error_name,
            api_name=api_name,
            condition="",  # Will be filled by config or manual review
            probability=probability,
            source="sdk_analysis"
        )

        if error_name not in self.error_to_apis:
            self.error_to_apis[error_name] = []

        # Check if this mapping already exists
        existing = [m for m in self.error_to_apis[error_name] if m.api_name == api_name]
        if not existing:
            self.error_to_apis[error_name].append(mapping)

        # Also update reverse mapping
        if api_name not in self.api_to_errors:
            self.api_to_errors[api_name] = set()
        self.api_to_errors[api_name].add(error_name)

    def _estimate_probability(self, error_name: str, api_name: str, source_file: Path) -> str:
        """
        Estimate the probability of an error occurring from an API.

        Args:
            error_name: Error code name
            api_name: API function name
            source_file: Source file

        Returns:
            Probability: "high", "medium", or "low"
        """
        # High probability patterns
        high_patterns = [
            # Parameter errors
            (r'INVALID_PARAM', r'param|config|init|set|start'),
            # Timeout errors
            (r'TIMEOUT', r'connect|wait|pair|bond'),
            # Not found errors
            (r'NOT_FOUND', r'disc|search|find|scan'),
            # Disconnection errors
            (r'DISCONNECT', r'connect|write|read'),
            # Auth errors
            (r'AUTH|SECURITY|ENC', r'bond|encrypt|pair'),
        ]

        for error_pat, api_pat in high_patterns:
            if re.search(error_pat, error_name, re.IGNORECASE):
                if re.search(api_pat, api_name, re.IGNORECASE):
                    return "high"

        # Low probability: errors that are rare
        low_patterns = [
            (r'HARDWARE_FAILURE', r'.*'),
            (r'UNSPECIFIED', r'.*'),
            (r'UNEXPECTED', r'.*'),
        ]

        for error_pat, api_pat in low_patterns:
            if re.search(error_pat, error_name, re.IGNORECASE):
                return "low"

        return "medium"

    def _build_result(self) -> Dict[str, List[Dict]]:
        """
        Build the final result dictionary.

        Returns:
            Dictionary mapping error names to API lists
        """
        result = {}

        for error_name, mappings in self.error_to_apis.items():
            # Sort by probability (high first)
            sorted_mappings = sorted(
                mappings,
                key=lambda m: {"high": 0, "medium": 1, "low": 2}.get(m.probability, 1)
            )

            result[error_name] = [
                {
                    "name": m.api_name,
                    "condition": m.condition,
                    "probability": m.probability,
                    "source": m.source
                }
                for m in sorted_mappings
            ]

        return result

    def export_to_json(self, output_path: str):
        """
        Export the mappings to a JSON file.

        Args:
            output_path: Output file path
        """
        result = self._build_result()

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(result, f, indent=2, ensure_ascii=False)

        logger.info(f"Exported error-to-API mappings to {output_path}")

    def merge_with_config(self, config_path: str) -> Dict[str, List[Dict]]:
        """
        Merge SDK analysis results with manual configuration.

        Args:
            config_path: Path to the YAML config file

        Returns:
            Merged dictionary
        """
        import yaml

        # Load config file
        config_file = Path(config_path)
        if not config_file.exists():
            logger.warning(f"Config file not found: {config_path}")
            return self._build_result()

        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                config = yaml.safe_load(f)
        except Exception as e:
            logger.warning(f"Failed to load config: {e}")
            return self._build_result()

        result = self._build_result()

        # Merge config into result
        if config and 'error_to_apis' in config:
            for error_name, apis in config['error_to_apis'].items():
                if error_name not in result:
                    result[error_name] = []

                for api_info in apis:
                    # Mark as coming from config
                    api_info['source'] = 'config_file'

                    # Check if this API already exists
                    existing = [a for a in result[error_name] if a['name'] == api_info['api']]
                    if existing:
                        # Update existing with config data (higher priority)
                        existing[0].update({
                            'condition': api_info.get('condition', ''),
                            'probability': api_info.get('probability', 'medium'),
                            'source': 'config_file'
                        })
                    else:
                        # Add new entry
                        result[error_name].append({
                            'name': api_info['api'],
                            'condition': api_info.get('condition', ''),
                            'probability': api_info.get('probability', 'medium'),
                            'source': 'config_file'
                        })

        # Re-sort all entries
        for error_name in result:
            result[error_name].sort(
                key=lambda m: {"high": 0, "medium": 1, "low": 2}.get(m.get('probability', 'medium'), 1)
            )

        return result


def analyze_sdk_errors(sdk_path: str, output_path: str = None) -> Dict[str, List[Dict]]:
    """
    Convenience function to analyze SDK error returns.

    Args:
        sdk_path: Path to SDK root
        output_path: Optional output file path for JSON

    Returns:
        Dictionary mapping error names to API lists
    """
    analyzer = ErrorReturnAnalyzer(sdk_path)
    result = analyzer.analyze_all()

    if output_path:
        analyzer.export_to_json(output_path)

    return result


# CLI interface
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="SDK Error Return Analyzer")
    parser.add_argument("--sdk", required=True, help="Path to SDK root")
    parser.add_argument("--output", help="Output JSON file path")
    parser.add_argument("--config", help="Optional YAML config file to merge")

    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    analyzer = ErrorReturnAnalyzer(args.sdk)
    result = analyzer.analyze_all()

    if args.config:
        result = analyzer.merge_with_config(args.config)

    if args.output:
        analyzer.export_to_json(args.output)

    # Print summary
    print(f"\nAnalysis Summary:")
    print(f"  Total error codes: {len(result)}")

    # Top errors by API count
    sorted_errors = sorted(result.items(), key=lambda x: len(x[1]), reverse=True)
    print(f"\nTop errors by API count:")
    for error, apis in sorted_errors[:10]:
        print(f"  {error}: {len(apis)} APIs")
        for api in apis[:3]:
            print(f"    - {api['name']} ({api['probability']})")
