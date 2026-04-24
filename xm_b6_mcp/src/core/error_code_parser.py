"""
BLE Error Code Parser
=====================

Parse ble/api/le_err.h to build error code dictionary.

Extracted data:
- Error code definitions (ERR_xxx)
- Error descriptions
- Error categories
- API -> Error mappings

Author: B6x MCP Server Team
Version: 0.1.0
"""

import re
import logging
from pathlib import Path
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, asdict

logger = logging.getLogger(__name__)


@dataclass
class ErrorCode:
    """
    BLE error code definition.

    Attributes:
        name: Error code name (e.g., "ERR_INVALID_HANDLE")
        value: Hex value (e.g., 0x0180)
        category: Error category (GATT, GAP, L2CAP, COMMON)
        description: Brief description from Doxygen
        probable_causes: List of possible causes
        solutions: List of suggested solutions
        related_apis: APIs that might return this error
    """
    name: str
    value: int
    category: str
    description: str = ""
    probable_causes: List[str] = None
    solutions: List[str] = None
    related_apis: List[str] = None

    def __post_init__(self):
        if self.probable_causes is None:
            self.probable_causes = []
        if self.solutions is None:
            self.solutions = []
        if self.related_apis is None:
            self.related_apis = []

    def __hash__(self):
        return hash(self.name)


@dataclass
class ErrorCategoryInfo:
    """Information about an error category."""
    name: str
    description: str
    prefix: str
    error_codes: List[str]  # List of error code names


class BLEErrorCodeParser:
    """
    Parse BLE error codes from le_err.h header file.

    Extracts:
    - Error code definitions (#define ERR_XXXX)
    - Doxygen comments for descriptions
    - Error categorization
    - Value mappings
    """

    def __init__(self, le_err_path: Optional[str] = None):
        """
        Initialize the error code parser.

        Args:
            le_err_path: Path to le_err.h file (optional, can be set later)
        """
        self.le_err_path = Path(le_err_path) if le_err_path else None
        self._error_codes: List[ErrorCode] = []
        self._error_lookup: Dict[int, ErrorCode] = {}
        self._name_lookup: Dict[str, ErrorCode] = {}

        # Known error categories
        self._categories = {
            'GATT': ErrorCategoryInfo(
                'GATT', 'Generic Attribute Profile errors', 'GATT', []
            ),
            'GAP': ErrorCategoryInfo(
                'GAP', 'Generic Access Profile errors', 'GAP', []
            ),
            'L2CAP': ErrorCategoryInfo(
                'L2CAP', 'Logical Link Control and Adaptation Protocol errors', 'L2CAP', []
            ),
            'SMP': ErrorCategoryInfo(
                'SMP', 'Security Manager Protocol errors', 'SMP', []
            ),
            'ATT': ErrorCategoryInfo(
                'ATT', 'Attribute Protocol errors', 'ATT', []
            ),
            'COMMON': ErrorCategoryInfo(
                'COMMON', 'Common BLE errors', 'ERR', []
            ),
        }

    def parse_error_codes(self, le_err_path: Optional[str] = None) -> List[ErrorCode]:
        """
        Parse le_err.h to extract all error codes.

        Args:
            le_err_path: Path to le_err.h (overrides constructor path)

        Returns:
            List of ErrorCode objects
        """
        if le_err_path:
            self.le_err_path = Path(le_err_path)

        if not self.le_err_path or not self.le_err_path.exists():
            logger.warning(f"le_err.h not found at {self.le_err_path}")
            return []

        try:
            with open(self.le_err_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            self._error_codes = self._extract_error_codes(content)

            # Build lookup dictionaries
            self._error_lookup = {ec.value: ec for ec in self._error_codes}
            self._name_lookup = {ec.name: ec for ec in self._error_codes}

            # Categorize error codes
            self._categorize_errors()

            logger.info(f"Parsed {len(self._error_codes)} error codes from {self.le_err_path.name}")
            return self._error_codes

        except Exception as e:
            logger.error(f"Failed to parse error codes: {e}")
            return []

    def _extract_error_codes(self, content: str) -> List[ErrorCode]:
        """Extract error codes from header file content.

        Supports two formats:
        1. #define ERR_XXXX 0xYYYY
        2. enum { XXXX_ERR_YYYY = 0xZZ, } (B6x BLE style)
        """
        error_codes = []

        # Split content into lines for processing
        lines = content.split('\n')

        # First, extract from enum blocks (B6x BLE style)
        error_codes.extend(self._extract_enum_error_codes(content))

        # Also extract from #define statements
        error_codes.extend(self._extract_define_error_codes(lines))

        return error_codes

    def _extract_enum_error_codes(self, content: str) -> List[ErrorCode]:
        """Extract error codes from enum definitions (B6x BLE style)."""
        error_codes = []

        # Pattern to match enum blocks
        # enum gap_err { ... };
        enum_pattern = r'enum\s+(\w+)_err\s*\{([^}]+)\}'
        enum_matches = re.finditer(enum_pattern, content, re.DOTALL)

        for enum_match in enum_matches:
            enum_name = enum_match.group(1).upper()  # GAP, ATT, L2C, GATT, SMP, etc.
            enum_body = enum_match.group(2)

            # Parse each entry in the enum
            # Format: /// description
            #         NAME = 0xXX,
            lines = enum_body.split('\n')
            current_comment = ""

            for line in lines:
                line_stripped = line.strip()

                # Extract Doxygen comment
                if line_stripped.startswith('///'):
                    current_comment = line_stripped[3:].strip()
                    continue

                # Parse enum entry: NAME = VALUE
                entry_match = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*,?', line_stripped)
                if entry_match:
                    name = entry_match.group(1)
                    value_str = entry_match.group(2)

                    try:
                        value = int(value_str, 16)
                    except ValueError:
                        continue

                    # Map enum name to category
                    category_map = {
                        'GAP': 'GAP',
                        'ATT': 'ATT',
                        'L2C': 'L2CAP',
                        'GATT': 'GATT',
                        'SMP': 'SMP',
                        'PRF': 'PRF',
                        'LL': 'LL',
                    }
                    category = category_map.get(enum_name, enum_name)

                    # Use comment as description
                    description = current_comment if current_comment else name.replace('_', ' ').title()

                    error_code = ErrorCode(
                        name=name,
                        value=value,
                        category=category,
                        description=description,
                        probable_causes=[],
                        solutions=[],
                        related_apis=[]
                    )
                    error_codes.append(error_code)

                    # Reset comment after use
                    current_comment = ""

        return error_codes

    def _extract_define_error_codes(self, lines: List[str]) -> List[ErrorCode]:
        """Extract error codes from #define statements."""
        error_codes = []

        i = 0
        while i < len(lines):
            line = lines[i]

            # Look for #define ERR_XXXX or #define LE_SUCCESS etc.
            if '#define' in line and ('ERR_' in line or '_ERR_' in line or 'LE_SUCCESS' in line or 'LE_ERR' in line):
                # Extract comment before define (if any)
                comment = self._extract_preceding_comment(lines, i)

                # Parse the define line - support multiple patterns
                match = re.search(r'#define\s+(\w+)\s+\(?([0-9A-Fa-fx]+)\)?', line)
                if match:
                    name = match.group(1)
                    value_str = match.group(2)

                    # Parse value
                    try:
                        if value_str.startswith('0x') or value_str.startswith('0X'):
                            value = int(value_str, 16)
                        else:
                            value = int(value_str)
                    except ValueError:
                        logger.debug(f"Could not parse error code value: {value_str}")
                        i += 1
                        continue

                    # Extract description from comment
                    description = self._parse_description_from_comment(comment, name)

                    # Classify error
                    category = self._classify_error(name)

                    error_code = ErrorCode(
                        name=name,
                        value=value,
                        category=category,
                        description=description,
                        probable_causes=[],
                        solutions=[],
                        related_apis=[]
                    )

                    error_codes.append(error_code)

            i += 1

        return error_codes

    def _extract_preceding_comment(self, lines: List[str], line_idx: int) -> str:
        """Extract comment block preceding a line."""
        comment_lines = []

        # Look backwards from current line
        i = line_idx - 1
        while i >= 0:
            line = lines[i].strip()

            # Stop if we hit a non-comment line (blank lines are OK)
            if line and not line.startswith('/'):
                # Allow a few blank lines before comment
                if not line:
                    blank_count = 0
                    j = i
                    while j >= 0 and not lines[j].strip():
                        blank_count += 1
                        j -= 1
                        if blank_count > 2:
                            break
                    if blank_count > 2:
                        break
                    i -= 1
                    continue
                else:
                    break

            # Collect comment lines
            if line.startswith('/'):
                comment_lines.insert(0, line)
            elif line:  # Non-blank, non-comment line
                break

            i -= 1

        return '\n'.join(comment_lines)

    def _parse_description_from_comment(self, comment: str, error_name: str) -> str:
        """Parse error description from Doxygen comment."""
        if not comment:
            return ""

        # Look for @brief tag
        brief_match = re.search(r'@brief\s+([^\n\r*]+)', comment)
        if brief_match:
            return brief_match.group(1).strip()

        # Look for description after /** or /*!
        desc_match = re.search(r'/\*\*?\s*([^\n\r@*]+)', comment)
        if desc_match:
            return desc_match.group(1).strip()

        # Return error name as fallback
        return error_name.replace('ERR_', '').replace('_', ' ').title()

    def _classify_error(self, error_name: str) -> str:
        """Classify error code into category."""
        name_upper = error_name.upper()

        # Check for category prefixes/suffixes
        for category, info in self._categories.items():
            if category != 'COMMON':
                if info.prefix in name_upper:
                    return category

        # Check for specific patterns
        if any(kw in name_upper for kw in ['ATT_', 'ATTRIBUTE']):
            return 'ATT'
        elif any(kw in name_upper for kw in ['GATT_']):
            return 'GATT'
        elif any(kw in name_upper for kw in ['GAP_']):
            return 'GAP'
        elif any(kw in name_upper for kw in ['L2CAP_']):
            return 'L2CAP'
        elif any(kw in name_upper for kw in ['SMP_', 'SECURITY']):
            return 'SMP'

        return 'COMMON'

    def _categorize_errors(self):
        """Categorize error codes and update category info."""
        for error_code in self._error_codes:
            if error_code.category in self._categories:
                self._categories[error_code.category].error_codes.append(error_code.name)

    def get_error_by_value(self, value: int) -> Optional[ErrorCode]:
        """Look up error code by numeric value."""
        return self._error_lookup.get(value)

    def get_error_by_name(self, name: str) -> Optional[ErrorCode]:
        """Look up error code by name."""
        return self._name_lookup.get(name)

    def get_errors_by_category(self, category: str) -> List[ErrorCode]:
        """Get all error codes in a category."""
        return [ec for ec in self._error_codes if ec.category == category]

    def get_categories(self) -> List[ErrorCategoryInfo]:
        """Get all error categories."""
        return list(self._categories.values())

    def export_to_json(self, output_path: str):
        """
        Export error codes to JSON file.

        Args:
            output_path: Path to output JSON file
        """
        data = {
            "version": "1.0.0",
            "source_file": str(self.le_err_path) if self.le_err_path else "",
            "total_error_codes": len(self._error_codes),
            "error_codes": [asdict(ec) for ec in self._error_codes],
            "lookup_by_value": {k: asdict(v) for k, v in self._error_lookup.items()},
            "lookup_by_name": {k: asdict(v) for k, v in self._name_lookup.items()},
            "categories": [asdict(cat) for cat in self._categories.values()]
        }

        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)

        logger.info(f"Error codes exported to: {output_path}")

    def load_from_json(self, json_path: str):
        """
        Load error codes from JSON file.

        Args:
            json_path: Path to JSON file
        """
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Restore error codes
        self._error_codes = [
            ErrorCode(**ec) for ec in data.get("error_codes", [])
        ]

        # Rebuild lookups
        self._error_lookup = {ec.value: ec for ec in self._error_codes}
        self._name_lookup = {ec.name: ec for ec in self._error_codes}

        logger.info(f"Loaded {len(self._error_codes)} error codes from: {json_path}")


def parse_error_codes(le_err_path: str) -> List[ErrorCode]:
    """Convenience function to parse error codes."""
    parser = BLEErrorCodeParser()
    return parser.parse_error_codes(le_err_path)


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python error_code_parser.py <le_err.h>")
        sys.exit(1)

    parser = BLEErrorCodeParser()
    error_codes = parser.parse_error_codes(sys.argv[1])

    print(f"\nParsed {len(error_codes)} error codes")

    # Print by category
    for category in parser.get_categories():
        if category.error_codes:
            print(f"\n{category.name} ({category.description}): {len(category.error_codes)} errors")
            for error_name in category.error_codes[:5]:
                error = parser.get_error_by_name(error_name)
                print(f"  {error.name}: 0x{error.value:04X} - {error.description}")
            if len(category.error_codes) > 5:
                print(f"  ... and {len(category.error_codes) - 5} more")

    # Export to JSON
    output_path = Path(__file__).parent.parent.parent / "data" / "domain" / "ble" / "error_codes.json"
    parser.export_to_json(str(output_path))
    print(f"\nExported to: {output_path}")
