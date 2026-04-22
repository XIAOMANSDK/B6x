"""
Source File Mapper - Declaration to Implementation Mapping
==========================================================

Maps C header files (.h) to their implementation files (.c)
and tracks which APIs are declared/implemented in each file.

This enables:
- Finding the .c file for any API declaration
- Tracing API usage across multiple files
- Building complete dependency graphs

Author: B6x MCP Server Team
Version: 1.0.0
"""

import logging
import re
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple
from dataclasses import dataclass, field

# Tree-sitter is REQUIRED dependency
import tree_sitter
from tree_sitter import Language, Parser

logger = logging.getLogger(__name__)


@dataclass
class SourceFileMapping:
    """
    Mapping between a header file and its implementation file.

    Attributes:
        header_file: Path to .h file (relative to SDK root)
        source_file: Path to .c file (relative to SDK root)
        declared_apis: List of API function names declared in header
        implemented_apis: List of API function names implemented in source
        include_guards: List of include guards in header
        includes: List of files included in source
    """
    header_file: str
    source_file: str
    declared_apis: List[str] = field(default_factory=list)
    implemented_apis: List[str] = field(default_factory=list)
    include_guards: List[str] = field(default_factory=list)
    includes: List[str] = field(default_factory=list)

    def get_coverage(self) -> float:
        """Get implementation coverage ratio."""
        if not self.declared_apis:
            return 1.0
        return len(self.implemented_apis) / len(self.declared_apis)

    def to_dict(self) -> Dict:
        return {
            "header_file": self.header_file,
            "source_file": self.source_file,
            "declared_apis": self.declared_apis,
            "implemented_apis": self.implemented_apis,
            "include_guards": self.include_guards,
            "includes": self.includes,
            "coverage": self.get_coverage()
        }


@dataclass
class APISourceLocation:
    """
    Source location for an API function.

    Attributes:
        api_name: Function name
        header_file: Where it's declared
        source_file: Where it's implemented (None if inline)
        line_number: Declaration line in header
        is_static: Whether implementation is static
        is_inline: Whether declared as inline
    """
    api_name: str
    header_file: str
    source_file: Optional[str]
    line_number: int
    is_static: bool = False
    is_inline: bool = False


class SourceFileMapper:
    """
    Maps C header files to implementation files using TreeSitter.
    """

    # Common naming patterns for .h/.c correspondence
    NAMING_PATTERNS = [
        # Exact name match: gpio.h -> gpio.c
        lambda h, s: Path(h).stem == Path(s).stem,
        # Header with _api suffix: uart_api.h -> uart.c
        lambda h, s: Path(h).stem.replace('_api', '') == Path(s).stem,
        # Header with _reg suffix: timer_reg.h -> timer.c
        lambda h, s: Path(h).stem.replace('_reg', '') == Path(s).stem,
        # Source with _impl suffix: i2c.h -> i2c_impl.c
        lambda h, s: Path(h).stem == Path(s).stem.replace('_impl', ''),
    ]

    def __init__(self, sdk_path: str):
        """
        Initialize source file mapper.

        Args:
            sdk_path: Root path to SDK directory
        """
        self.sdk_path = Path(sdk_path)
        self.mappings: Dict[str, SourceFileMapping] = {}  # header -> mapping
        self.api_locations: Dict[str, APISourceLocation] = {}  # api_name -> location

        # TreeSitter parser (lazy init)
        self._parser: Optional[Parser] = None
        self._c_language: Optional[Language] = None

    # ========================================================================
    # Main Mapping Methods
    # ========================================================================

    def build_mappings(
        self,
        api_dir: str = "drivers/api",
        src_dir: str = "drivers/src",
        ble_api_dir: str = "ble/api",
        ble_prf_dir: str = "ble/prf"
    ) -> Dict[str, SourceFileMapping]:
        """
        Build all header → source mappings.

        Args:
            api_dir: Directory containing .h files
            src_dir: Directory containing .c files
            ble_api_dir: BLE API directory
            ble_prf_dir: BLE Profile directory

        Returns:
            Dictionary mapping header file paths to SourceFileMapping objects
        """
        logger.info("Building source file mappings...")

        # Collect header and source files
        header_files = self._find_headers(api_dir)
        header_files.extend(self._find_headers(ble_api_dir))

        source_files = self._find_sources(src_dir)
        source_files.extend(self._find_sources(ble_prf_dir))

        logger.info(f"Found {len(header_files)} header files, {len(source_files)} source files")

        # Build mappings using TreeSitter (required)
        self._build_mappings_with_treesitter(header_files, source_files)

        logger.info(f"Built {len(self.mappings)} header→source mappings")
        logger.info(f"Indexed {len(self.api_locations)} API locations")

        return self.mappings

    def _build_mappings_with_treesitter(
        self,
        header_files: List[Path],
        source_files: List[Path]
    ) -> None:
        """Build mappings using TreeSitter AST parsing."""
        parser = self._get_parser()
        if not parser:
            logger.warning("TreeSitter parser not available, falling back to pattern matching")
            self._build_mappings_by_pattern(header_files, source_files)
            return

        # Parse headers to extract declared APIs
        header_apis: Dict[Path, List[str]] = {}
        for header_path in header_files:
            try:
                apis = self._extract_declarations(header_path, parser)
                header_apis[header_path] = apis
            except Exception as e:
                logger.debug(f"Failed to parse {header_path}: {e}")

        # Parse sources to extract implemented APIs
        source_apis: Dict[Path, List[str]] = {}
        for source_path in source_files:
            try:
                apis = self._extract_implementations(source_path, parser)
                source_apis[source_path] = apis
            except Exception as e:
                logger.debug(f"Failed to parse {source_path}: {e}")

        # Match headers to sources
        for header_path, declared in header_apis.items():
            matching_source = self._find_matching_source(header_path, source_files)

            if matching_source:
                implemented = source_apis.get(matching_source, [])

                mapping = SourceFileMapping(
                    header_file=str(header_path.relative_to(self.sdk_path)),
                    source_file=str(matching_source.relative_to(self.sdk_path)),
                    declared_apis=declared,
                    implemented_apis=implemented
                )

                self.mappings[str(header_path.relative_to(self.sdk_path))] = mapping

                # Index API locations
                for api_name in declared:
                    self.api_locations[api_name] = APISourceLocation(
                        api_name=api_name,
                        header_file=mapping.header_file,
                        source_file=mapping.source_file if api_name in implemented else None,
                        line_number=0  # TODO: Extract line numbers
                    )

    def _build_mappings_by_pattern(
        self,
        header_files: List[Path],
        source_files: List[Path]
    ) -> None:
        """Build mappings using filename patterns (fallback)."""
        for header_path in header_files:
            matching_source = self._find_matching_source(header_path, source_files)

            if matching_source:
                mapping = SourceFileMapping(
                    header_file=str(header_path.relative_to(self.sdk_path)),
                    source_file=str(matching_source.relative_to(self.sdk_path)),
                    declared_apis=[],  # Empty without TreeSitter
                    implemented_apis=[]
                )

                self.mappings[str(header_path.relative_to(self.sdk_path))] = mapping

    # ========================================================================
    # TreeSitter Parsing
    # ========================================================================

    def _get_parser(self) -> Optional[Parser]:
        """Get or create TreeSitter parser for C."""
        if self._parser is not None:
            return self._parser

        try:
            # Try tree_sitter_c_language (most common)
            try:
                from tree_sitter_c_language import get_language
                self._c_language = get_language()
                self._parser = Parser(self._c_language)
                logger.debug("Using tree_sitter_c_language")
                return self._parser
            except ImportError:
                pass

            # Try tree_sitter_language_pack (alternative)
            try:
                from tree_sitter_language_pack import get_language
                self._c_language = get_language('c')
                self._parser = Parser(self._c_language)
                logger.debug("Using tree_sitter_language_pack")
                return self._parser
            except ImportError:
                pass

            # Try tree_sitter_c (another alternative)
            try:
                from tree_sitter_c import language
                self._c_language = tree_sitter.Language(language())
                self._parser = Parser(self._c_language)
                logger.debug("Using tree_sitter_c")
                return self._parser
            except ImportError:
                pass

            # Try tree_sitter_languages (legacy)
            try:
                from tree_sitter_languages import get_language
                self._c_language = get_language('c')
                self._parser = Parser(self._c_language)
                logger.debug("Using tree_sitter_languages")
                return self._parser
            except ImportError:
                pass

            logger.warning("No tree-sitter C language module found")
            logger.warning("Please install one of: pip install tree-sitter-c")
            return None

        except Exception as e:
            logger.warning(f"Failed to initialize TreeSitter: {e}")
            return None

    def _extract_declarations(self, header_path: Path, parser: Parser) -> List[str]:
        """Extract function declarations from header file."""
        with open(header_path, 'r', encoding='utf-8', errors='ignore') as f:
            source_code = f.read()

        tree = parser.parse(bytes(source_code, 'utf8'))

        declarations = []
        root = tree.root_node

        # Find all function declarations
        for node in root.children:
            if node.type == 'declaration':
                for child in node.children:
                    if child.type == 'function_declarator':
                        # Extract function name
                        func_name = self._extract_function_name(child)
                        if func_name:
                            declarations.append(func_name)

        return declarations

    def _extract_implementations(self, source_path: Path, parser: Parser) -> List[str]:
        """Extract function implementations from source file."""
        with open(source_path, 'r', encoding='utf-8', errors='ignore') as f:
            source_code = f.read()

        tree = parser.parse(bytes(source_code, 'utf8'))

        implementations = []
        root = tree.root_node

        # Find all function definitions
        for node in root.children:
            if node.type == 'function_definition':
                for child in node.children:
                    if child.type == 'function_declarator':
                        func_name = self._extract_function_name(child)
                        if func_name:
                            implementations.append(func_name)

        return implementations

    def _extract_function_name(self, node) -> Optional[str]:
        """Extract function name from AST node."""
        for child in node.children:
            if child.type == 'identifier':
                return child.text.decode('utf-8') if isinstance(child.text, bytes) else child.text
        return None

    # ========================================================================
    # Helper Methods
    # ========================================================================

    def _find_matching_source(self, header_path: Path, source_files: List[Path]) -> Optional[Path]:
        """Find the .c file that corresponds to a .h file."""
        header_name = header_path.name

        for source_path in source_files:
            for pattern in self.NAMING_PATTERNS:
                if pattern(header_name, source_path.name):
                    return source_path

        return None

    def _find_headers(self, directory: str) -> List[Path]:
        """Find all .h files in a directory."""
        dir_path = self.sdk_path / directory
        if not dir_path.exists():
            return []

        return list(dir_path.rglob('*.h'))

    def _find_sources(self, directory: str) -> List[Path]:
        """Find all .c files in a directory."""
        dir_path = self.sdk_path / directory
        if not dir_path.exists():
            return []

        return list(dir_path.rglob('*.c'))

    # ========================================================================
    # Query Methods
    # ========================================================================

    def get_source_file(self, header_path: str) -> Optional[str]:
        """Get the source file for a given header file."""
        mapping = self.mappings.get(header_path)
        return mapping.source_file if mapping else None

    def get_api_location(self, api_name: str) -> Optional[APISourceLocation]:
        """Get source location for an API function."""
        return self.api_locations.get(api_name)

    def get_apis_in_file(self, file_path: str) -> List[str]:
        """Get all APIs declared/implemented in a file."""
        # Check as header
        if file_path in self.mappings:
            return self.mappings[file_path].declared_apis

        # Check as source
        apis = []
        for mapping in self.mappings.values():
            if mapping.source_file == file_path:
                apis.extend(mapping.implemented_apis)

        return apis

    def get_all_mappings(self) -> List[SourceFileMapping]:
        """Get all header→source mappings."""
        return list(self.mappings.values())

    def export_to_json(self, output_path: str) -> None:
        """Export mappings to JSON file."""
        import json

        data = {
            "mappings": {
                header: mapping.to_dict()
                for header, mapping in self.mappings.items()
            },
            "api_locations": {
                api: {
                    "api_name": loc.api_name,
                    "header_file": loc.header_file,
                    "source_file": loc.source_file,
                    "line_number": loc.line_number
                }
                for api, loc in self.api_locations.items()
            },
            "statistics": {
                "total_mappings": len(self.mappings),
                "total_apis": len(self.api_locations),
                "average_coverage": sum(m.get_coverage() for m in self.mappings.values()) / len(self.mappings) if self.mappings else 0
            }
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)


# ============================================================================
# Convenience Functions
# ============================================================================

def build_source_mappings(sdk_path: str) -> SourceFileMapper:
    """Build source file mappings for SDK."""
    mapper = SourceFileMapper(sdk_path)
    mapper.build_mappings()
    return mapper


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python source_file_mapper.py <sdk_path>")
        sys.exit(1)

    sdk_path = sys.argv[1]
    mapper = SourceFileMapper(sdk_path)

    print(f"\n{'='*70}")
    print(f"Source File Mapper")
    print(f"{'='*70}\n")

    mapper.build_mappings()

    # Show sample mappings
    mappings = mapper.get_all_mappings()
    print(f"Found {len(mappings)} header→source mappings\n")

    for i, mapping in enumerate(mappings[:10], 1):
        print(f"{i}. {mapping.header_file}")
        print(f"   -> {mapping.source_file}")
        print(f"   Declared: {len(mapping.declared_apis)}, Implemented: {len(mapping.implemented_apis)}")
        if mapping.declared_apis:
            print(f"   Sample APIs: {', '.join(mapping.declared_apis[:3])}")
        print()
