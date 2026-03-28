"""
Profile Dependency Parser - BLE Profile to API Mapping
======================================================

Parses BLE Profile .c files to extract actual GATT API calls
using TreeSitter AST analysis.

This replaces simple keyword matching with accurate dependency
tracking based on function call analysis.

Author: B6x MCP Server Team
Version: 1.0.0
"""

import logging
import re
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple
from dataclasses import dataclass, field
from collections import defaultdict

# Tree-sitter is REQUIRED dependency
import tree_sitter
from tree_sitter import Language, Parser

logger = logging.getLogger(__name__)


@dataclass
class FunctionCall:
    """
    Represents a function call found in source code.

    Attributes:
        function_name: Name of the function being called
        line_number: Line where call occurs
        context: Surrounding function name (who calls this)
        call_type: Type of call ('gatt_api', 'driver_api', 'ke_msg', 'other')
    """
    function_name: str
    line_number: int
    context: str = ""  # Function containing this call
    call_type: str = "other"  # gatt_api, driver_api, ke_msg, other


@dataclass
class ProfileDependency:
    """
    Dependency information for a BLE Profile.

    Attributes:
        profile_name: Profile name (e.g., "hids", "battery")
        source_file: Path to profile .c file
        gatt_api_calls: List of GATT API function calls
        driver_api_calls: List of driver API calls
        ke_msg_calls: List of kernel message API calls
        all_function_calls: All function calls found
        include_files: Files included by this profile
    """
    profile_name: str
    source_file: str
    gatt_api_calls: List[FunctionCall] = field(default_factory=list)
    driver_api_calls: List[FunctionCall] = field(default_factory=list)
    ke_msg_calls: List[FunctionCall] = field(default_factory=list)
    all_function_calls: List[FunctionCall] = field(default_factory=list)
    include_files: List[str] = field(default_factory=list)

    def get_all_apis(self) -> Set[str]:
        """Get set of all unique API names called."""
        apis = set()
        for call in self.gatt_api_calls:
            apis.add(call.function_name)
        for call in self.driver_api_calls:
            apis.add(call.function_name)
        for call in self.ke_msg_calls:
            apis.add(call.function_name)
        return apis

    def to_dict(self) -> Dict:
        return {
            "profile_name": self.profile_name,
            "source_file": self.source_file,
            "gatt_api_calls": [fc.function_name for fc in self.gatt_api_calls],
            "driver_api_calls": [fc.function_name for fc in self.driver_api_calls],
            "ke_msg_calls": [fc.function_name for fc in self.ke_msg_calls],
            "all_apis": list(self.get_all_apis()),
            "include_files": self.include_files
        }


class ProfileDependencyParser:
    """
    Parses BLE Profile files to extract API dependencies.
    """

    # GATT API patterns (common function prefixes)
    GATT_PATTERNS = [
        r'Gatt[A-Z][a-z]+',  # GattAddService, GattSetDescValue
        r'gap_[a-z_]+',      # gapm_start_advertise, gapc_connect
        r'ke_[a-z_]+',       # ke_msg_alloc, ke_timer_set
        r'prf_[a-z_]+',      # prf_add_task, prf_get_task_from_id
    ]

    # Driver API patterns
    DRIVER_PATTERNS = [
        r'uart_[a-z_]+',
        r'gpio_[a-z_]+',
        r'i2c_[a-z_]+',
        r'spi_[a-z_]+',
        r'timer_[a-z_]+',
        r'dma_[a-z_]+',
    ]

    def __init__(self, sdk_path: str):
        """
        Initialize profile dependency parser.

        Args:
            sdk_path: Root path to SDK directory
        """
        self.sdk_path = Path(sdk_path)
        self.dependencies: Dict[str, ProfileDependency] = {}  # profile_name -> dependency

        # TreeSitter parser (lazy init)
        self._parser: Optional[Parser] = None
        self._c_language: Optional[Language] = None

    # ========================================================================
    # Main Parsing Methods
    # ========================================================================

    def parse_profile_directory(self, profile_dir: str = "ble/prf") -> Dict[str, ProfileDependency]:
        """
        Parse all profile .c files in a directory.

        Args:
            profile_dir: Directory containing profile source files

        Returns:
            Dictionary mapping profile names to ProfileDependency objects
        """
        logger.info(f"Parsing profiles in {profile_dir}...")

        prf_path = self.sdk_path / profile_dir
        if not prf_path.exists():
            logger.warning(f"Profile directory not found: {prf_path}")
            return {}

        # Find all .c files in profile directories
        profile_files = list(prf_path.rglob('*.c'))
        logger.info(f"Found {len(profile_files)} profile source files")

        # Parse each profile
        for profile_file in profile_files:
            try:
                dep = self._parse_profile_file(profile_file)
                if dep:
                    self.dependencies[dep.profile_name] = dep
            except Exception as e:
                logger.debug(f"Failed to parse {profile_file}: {e}")

        logger.info(f"Parsed {len(self.dependencies)} profiles")
        return self.dependencies

    def _parse_profile_file(self, profile_file: Path) -> Optional[ProfileDependency]:
        """Parse a single profile .c file."""
        parser = self._get_parser()
        if not parser:
            raise RuntimeError("Tree-sitter parser not available. This is a required dependency.")

        # Extract profile name from file path
        profile_name = self._extract_profile_name(profile_file)

        with open(profile_file, 'r', encoding='utf-8', errors='ignore') as f:
            source_code = f.read()

        # Parse with TreeSitter
        tree = parser.parse(bytes(source_code, 'utf8'))
        root = tree.root_node

        # Extract function calls
        function_calls = self._extract_function_calls_from_ast(root, source_code)

        # Categorize calls
        gatt_calls, driver_calls, ke_msg_calls = self._categorize_calls(function_calls)

        # Extract includes
        includes = self._extract_includes(source_code)

        return ProfileDependency(
            profile_name=profile_name,
            source_file=str(profile_file.relative_to(self.sdk_path)),
            gatt_api_calls=gatt_calls,
            driver_api_calls=driver_calls,
            ke_msg_calls=ke_msg_calls,
            all_function_calls=function_calls,
            include_files=includes
        )

    def _parse_profile_fallback(self, profile_file: Path) -> Optional[ProfileDependency]:
        """Fallback parsing without TreeSitter (regex-based)."""
        profile_name = self._extract_profile_name(profile_file)

        with open(profile_file, 'r', encoding='utf-8', errors='ignore') as f:
            source_code = f.read()

        # Extract function calls using regex
        function_calls = self._extract_function_calls_regex(source_code)
        gatt_calls, driver_calls, ke_msg_calls = self._categorize_calls(function_calls)
        includes = self._extract_includes(source_code)

        return ProfileDependency(
            profile_name=profile_name,
            source_file=str(profile_file.relative_to(self.sdk_path)),
            gatt_api_calls=gatt_calls,
            driver_api_calls=driver_calls,
            ke_msg_calls=ke_msg_calls,
            all_function_calls=function_calls,
            include_files=includes
        )

    # ========================================================================
    # TreeSitter AST Analysis
    # ========================================================================

    def _get_parser(self) -> Optional[Parser]:
        """Get or create TreeSitter parser for C."""
        if self._parser is not None:
            return self._parser

        try:
            # Try to use tree-sitter-languages package (easier installation)
            try:
                from tree_sitter_languages import get_language
                self._c_language = get_language('c')
                self._parser = Parser(self._c_language)
                return self._parser
            except ImportError:
                # Fallback: build from source if tree-sitter-c is available
                vendor_path = Path(__file__).parent.parent.parent / 'vendor' / 'tree-sitter-c'

                if vendor_path.exists():
                    # Build library
                    build_path = Path(__file__).parent.parent / 'build'
                    build_path.mkdir(exist_ok=True)

                    # Compile the language
                    Language.build_library(
                        str(build_path / 'languages.so'),
                        [str(vendor_path)]
                    )
                    self._c_language = Language(str(build_path / 'languages.so'), 'c')
                    self._parser = Parser(self._c_language)
                    return self._parser

        except Exception as e:
            logger.warning(f"Failed to initialize TreeSitter: {e}")
            return None

    def _extract_function_calls_from_ast(self, root_node, source_code: bytes) -> List[FunctionCall]:
        """Extract all function calls from AST."""
        calls = []

        # Find all call expressions
        def find_calls(node, context_func=""):
            if node.type == 'call_expression':
                # Extract function name
                func_node = node.children[0]
                if func_node.type == 'identifier':
                    func_name = func_node.text.decode('utf-8') if isinstance(func_node.text, bytes) else func_node.text

                    # Get line number
                    line_num = source_code[:node.start_byte].count(b'\n') + 1

                    call = FunctionCall(
                        function_name=func_name,
                        line_number=line_num,
                        context=context_func
                    )
                    calls.append(call)

            # Recurse into children
            for child in node.children:
                # Track if we're in a function definition
                if child.type == 'function_definition':
                    # Extract function name
                    func_name = ""
                    for subchild in child.children:
                        if subchild.type == 'function_declarator':
                            func_name = self._extract_function_name_from_declarator(subchild)
                            break
                    find_calls(child, func_name)
                else:
                    find_calls(child, context_func)

        find_calls(root_node)
        return calls

    def _extract_function_name_from_declarator(self, node) -> str:
        """Extract function name from declarator node."""
        for child in node.children:
            if child.type == 'identifier':
                name = child.text.decode('utf-8') if isinstance(child.text, bytes) else child.text
                return name
        return ""

    # ========================================================================
    # Regex Fallback Methods
    # ========================================================================

    def _extract_function_calls_regex(self, source_code: str) -> List[FunctionCall]:
        """Extract function calls using regex (fallback)."""
        calls = []

        # Match function call patterns: name followed by (
        pattern = r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\('

        for match in re.finditer(pattern, source_code):
            func_name = match.group(1)
            line_num = source_code[:match.start()].count('\n') + 1

            calls.append(FunctionCall(
                function_name=func_name,
                line_number=line_num,
                context="",
                call_type="other"
            ))

        return calls

    # ========================================================================
    # Categorization Methods
    # ========================================================================

    def _categorize_calls(self, calls: List[FunctionCall]) -> Tuple[List[FunctionCall], List[FunctionCall], List[FunctionCall]]:
        """Categorize function calls into GATT, driver, and kernel message APIs."""
        gatt_calls = []
        driver_calls = []
        ke_msg_calls = []

        for call in calls:
            func_name = call.function_name.lower()

            # Check for GATT APIs
            if self._is_gatt_api(call.function_name):
                call.call_type = "gatt_api"
                gatt_calls.append(call)

            # Check for driver APIs
            elif self._is_driver_api(call.function_name):
                call.call_type = "driver_api"
                driver_calls.append(call)

            # Check for kernel message APIs
            elif self._is_ke_msg_api(call.function_name):
                call.call_type = "ke_msg"
                ke_msg_calls.append(call)

        return gatt_calls, driver_calls, ke_msg_calls

    def _is_gatt_api(self, func_name: str) -> bool:
        """Check if function is a GATT API."""
        func_lower = func_name.lower()

        # GATT API patterns
        gatt_prefixes = ['gatt', 'gap', 'prf_']
        for prefix in gatt_prefixes:
            if func_lower.startswith(prefix):
                return True

        return False

    def _is_driver_api(self, func_name: str) -> bool:
        """Check if function is a driver API."""
        func_lower = func_name.lower()

        driver_prefixes = ['gpio', 'uart', 'i2c', 'spi', 'timer', 'dma', 'adc', 'dac']
        for prefix in driver_prefixes:
            if func_lower.startswith(prefix):
                return True

        return False

    def _is_ke_msg_api(self, func_name: str) -> bool:
        """Check if function is a kernel message API."""
        return func_name.lower().startswith('ke_')

    # ========================================================================
    # Helper Methods
    # ========================================================================

    def _extract_profile_name(self, profile_file: Path) -> str:
        """Extract profile name from file path."""
        # Try to extract from parent directory or filename
        parent_name = profile_file.parent.name.lower()
        stem_name = profile_file.stem.lower()

        # Common profile patterns
        if parent_name.startswith('prf_'):
            return parent_name.replace('prf_', '')
        elif stem_name.startswith('prf_'):
            return stem_name.replace('prf_', '')
        else:
            return stem_name

    def _extract_includes(self, source_code: str) -> List[str]:
        """Extract all #include statements."""
        includes = []

        # Match #include "..." or #include <...>
        pattern = r'#\s*include\s*["<]([^">]+)[">]'

        for match in re.finditer(pattern, source_code):
            includes.append(match.group(1))

        return includes

    # ========================================================================
    # Query Methods
    # ========================================================================

    def get_profile_dependency(self, profile_name: str) -> Optional[ProfileDependency]:
        """Get dependency info for a specific profile."""
        return self.dependencies.get(profile_name)

    def get_all_gatt_apis(self) -> Set[str]:
        """Get all unique GATT APIs used across all profiles."""
        apis = set()
        for dep in self.dependencies.values():
            for call in dep.gatt_api_calls:
                apis.add(call.function_name)
        return apis

    def get_profiles_using_api(self, api_name: str) -> List[str]:
        """Get list of profiles that use a specific API."""
        profiles = []
        for profile_name, dep in self.dependencies.items():
            if any(call.function_name == api_name for call in dep.all_function_calls):
                profiles.append(profile_name)
        return profiles

    def export_to_json(self, output_path: str) -> None:
        """Export dependencies to JSON file."""
        import json

        data = {
            "dependencies": {
                name: dep.to_dict()
                for name, dep in self.dependencies.items()
            },
            "statistics": {
                "total_profiles": len(self.dependencies),
                "unique_gatt_apis": len(self.get_all_gatt_apis()),
                "total_function_calls": sum(len(dep.all_function_calls) for dep in self.dependencies.values())
            }
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)


# ============================================================================
# Convenience Functions
# ============================================================================

def parse_profile_dependencies(sdk_path: str) -> ProfileDependencyParser:
    """Parse all BLE profile dependencies."""
    parser = ProfileDependencyParser(sdk_path)
    parser.parse_profile_directory()
    return parser


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python profile_dependency_parser.py <sdk_path>")
        sys.exit(1)

    sdk_path = sys.argv[1]
    parser = ProfileDependencyParser(sdk_path)

    print(f"\n{'='*70}")
    print(f"Profile Dependency Parser")
    print(f"{'='*70}\n")

    parser.parse_profile_directory()

    # Show results
    print(f"Parsed {len(parser.dependencies)} profiles\n")

    for profile_name, dep in parser.dependencies.items():
        print(f"Profile: {profile_name}")
        print(f"  File: {dep.source_file}")
        print(f"  GATT APIs: {len(dep.gatt_api_calls)}")
        print(f"  Driver APIs: {len(dep.driver_api_calls)}")
        print(f"  KE_MSG APIs: {len(dep.ke_msg_calls)}")

        if dep.gatt_api_calls:
            print(f"  Sample GATT calls: {', '.join([c.function_name for c in dep.gatt_api_calls[:3]])}")
        print()
