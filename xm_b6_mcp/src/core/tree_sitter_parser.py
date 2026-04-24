"""
Tree-sitter C Parser
====================

AST-based C code parser for extracting function declarations,
parameters, and Doxygen comments from B6x SDK headers.

Author: B6x MCP Server Team
Version: 0.1.0
"""

import json
import logging
from pathlib import Path
from typing import List, Dict, Optional, Tuple, Set, Any
from dataclasses import dataclass, field, asdict

from common.path_utils import make_relative, get_sdk_root

logger = logging.getLogger(__name__)


@dataclass
class FunctionParameter:
    """Function parameter information."""
    name: str
    type: str
    description: str = ""  # From Doxygen


@dataclass
class FunctionDeclaration:
    """Function declaration extracted from header file."""
    name: str
    return_type: str
    parameters: List[FunctionParameter] = field(default_factory=list)
    brief_description: str = ""  # @brief from Doxygen
    detailed_description: str = ""  # Full Doxygen comment
    file_path: str = ""
    line_number: int = 0
    is_exported: bool = False  # Check for B6x_API_EXPORT or similar
    implementation_file: str = ""  # Path to .c implementation
    implementation_line: int = 0  # Line in .c file
    has_implementation: bool = False


@dataclass
class MacroDefinition:
    """Macro definition extracted from header file."""
    name: str
    value: str  # Macro value (could be expression)
    macro_type: str  # 'register', 'bitmask', 'constant', 'other'
    brief: str = ""  # Doxygen comment
    file_path: str = ""
    line_number: int = 0


@dataclass
class EnumValue:
    """Single enum value."""
    name: str
    value: int  # Integer value
    brief: str = ""  # Doxygen comment for this value


@dataclass
class EnumDefinition:
    """Enum definition extracted from header file."""
    name: str
    values: List[EnumValue] = field(default_factory=list)
    underlying_type: str = ""  # int, uint32_t, etc.
    brief: str = ""  # Doxygen comment
    file_path: str = ""
    line_number: int = 0


@dataclass
class HeaderParseResult:
    """Result of parsing a header file (legacy - kept for compatibility)."""
    file_path: str
    functions: List[FunctionDeclaration] = field(default_factory=list)
    macros: List[Dict] = field(default_factory=list)
    typedefs: List[Dict] = field(default_factory=list)
    parse_errors: List[str] = field(default_factory=list)


@dataclass
class FullParseResult:
    """Complete result of parsing a header file with all content types."""
    file_path: str
    functions: List[FunctionDeclaration] = field(default_factory=list)
    macros: List[MacroDefinition] = field(default_factory=list)
    enums: List[EnumDefinition] = field(default_factory=list)
    typedefs: List[Dict] = field(default_factory=list)
    parse_errors: List[str] = field(default_factory=list)


@dataclass
class FunctionDefinition:
    """Function definition extracted from .c source file."""
    name: str = ""
    return_type: str = ""
    parameters: List[FunctionParameter] = field(default_factory=list)
    function_body: str = ""  # Source code of function body
    file_path: str = ""
    line_number: int = 0
    is_static: bool = False


@dataclass
class SourceParseResult:
    """Result of parsing a .c source file."""
    file_path: str
    functions: List[FunctionDefinition] = field(default_factory=list)
    macros: List[MacroDefinition] = field(default_factory=list)
    parse_errors: List[str] = field(default_factory=list)


@dataclass
class FunctionCall:
    """Function call extracted from example code."""
    function_name: str              # Name of the function being called
    file_path: str                  # Example file path
    line_number: int                # Call line number
    column_number: int              # Call column number
    calling_function: str           # Parent function name
    arguments: List[str] = field(default_factory=list)  # Argument text snippets
    is_sdk_api: bool = False        # Whether this is an SDK API
    module: str = ""                # 'driver', 'ble', 'unknown'
    peripheral: str = ""            # 'GPIO', 'UART', etc.
    context_snippet: str = ""       # Code context (2 lines before/after)
    example_name: str = ""          # Example directory name


@dataclass
class FunctionCallWithContext:
    """
    Enhanced function call with complete context information.

    Provides extended context around API calls in examples:
    - Lines before and after the call
    - Enclosing function name
    - Call depth and related APIs
    - Initialization sequence detection

    Note: This class proxies all FunctionCall attributes via __getattr__,
    so you can access properties like 'calling_function' directly.
    """
    base: FunctionCall                      # Original function call
    context_before: List[str] = field(default_factory=list)    # Lines before call
    context_after: List[str] = field(default_factory=list)     # Lines after call
    enclosing_function: str = ""            # Function containing this call
    call_depth: int = 0                     # Nesting depth
    is_in_init_section: bool = False        # Is in initialization code
    related_apis: Set[str] = field(default_factory=set)        # Nearby SDK API calls
    call_sequence_index: int = -1           # Position in init sequence

    # Proxy properties to access base FunctionCall attributes
    @property
    def function_name(self) -> str:
        """Proxy to base.function_name."""
        return self.base.function_name

    @property
    def file_path(self) -> str:
        """Proxy to base.file_path."""
        return self.base.file_path

    @property
    def line_number(self) -> int:
        """Proxy to base.line_number."""
        return self.base.line_number

    @property
    def column_number(self) -> int:
        """Proxy to base.column_number."""
        return self.base.column_number

    @property
    def calling_function(self) -> str:
        """Proxy to base.calling_function (or use enclosing_function if not set)."""
        return self.base.calling_function or self.enclosing_function

    @property
    def arguments(self) -> List[str]:
        """Proxy to base.arguments."""
        return self.base.arguments

    @property
    def is_sdk_api(self) -> bool:
        """Proxy to base.is_sdk_api."""
        return self.base.is_sdk_api

    @property
    def module(self) -> str:
        """Proxy to base.module."""
        return self.base.module

    @property
    def peripheral(self) -> str:
        """Proxy to base.peripheral."""
        return self.base.peripheral

    @property
    def context_snippet(self) -> str:
        """Proxy to base.context_snippet."""
        return self.base.context_snippet

    @property
    def example_name(self) -> str:
        """Proxy to base.example_name."""
        return self.base.example_name

    def __getattr__(self, name: str):
        """Fallback: delegate any unknown attribute to base FunctionCall."""
        try:
            return getattr(self.base, name)
        except AttributeError:
            raise AttributeError(
                f"'{type(self).__name__}' object has no attribute '{name}'"
            )


@dataclass
class ExampleParseResult:
    """Result of parsing an example file."""
    file_path: str
    example_name: str
    example_type: str               # 'examples' or 'projects'
    function_calls: List[FunctionCall] = field(default_factory=list)
    functions_defined: List[str] = field(default_factory=list)
    included_headers: List[str] = field(default_factory=list)


# ============================================================================
# JSON Serialization Functions for Parse Result Caching
# ============================================================================

def serialize_full_parse_result(result: FullParseResult) -> dict:
    """
    Convert FullParseResult to JSON-serializable dict.

    Args:
        result: FullParseResult object to serialize

    Returns:
        JSON-serializable dictionary
    """
    data = asdict(result)
    # Convert dataclass objects in lists
    data['functions'] = [_serialize_function_decl(f) for f in result.functions]
    data['macros'] = [asdict(m) for m in result.macros]
    data['enums'] = [_serialize_enum_def(e) for e in result.enums]
    return data


def deserialize_full_parse_result(data: dict) -> FullParseResult:
    """
    Convert dict back to FullParseResult.

    Args:
        data: Dictionary from JSON

    Returns:
        FullParseResult object
    """
    # Reconstruct dataclass objects
    data['functions'] = [_deserialize_function_decl(f) for f in data['functions']]
    data['macros'] = [MacroDefinition(**m) for m in data['macros']]
    data['enums'] = [_deserialize_enum_def(e) for e in data['enums']]
    return FullParseResult(**data)


def _serialize_function_decl(func: FunctionDeclaration) -> dict:
    """Serialize FunctionDeclaration including parameters."""
    data = asdict(func)
    data['parameters'] = [asdict(p) for p in func.parameters]
    return data


def _deserialize_function_decl(data: dict) -> FunctionDeclaration:
    """Deserialize FunctionDeclaration including parameters."""
    data['parameters'] = [FunctionParameter(**p) for p in data['parameters']]
    return FunctionDeclaration(**data)


def _serialize_enum_def(enum: EnumDefinition) -> dict:
    """Serialize EnumDefinition including values."""
    data = asdict(enum)
    data['values'] = [asdict(v) for v in enum.values]
    return data


def _deserialize_enum_def(data: dict) -> EnumDefinition:
    """Deserialize EnumDefinition including values."""
    data['values'] = [EnumValue(**v) for v in data['values']]
    return EnumDefinition(**data)


def serialize_source_parse_result(result: 'SourceParseResult') -> dict:
    """
    Convert SourceParseResult to JSON-serializable dict.

    Args:
        result: SourceParseResult object to serialize

    Returns:
        JSON-serializable dictionary
    """
    data = asdict(result)
    # Convert dataclass objects in lists
    data['functions'] = [_serialize_function_def(f) for f in result.functions]
    data['macros'] = [asdict(m) for m in result.macros]
    return data


def deserialize_source_parse_result(data: dict) -> 'SourceParseResult':
    """
    Convert dict back to SourceParseResult.

    Args:
        data: Dictionary from JSON

    Returns:
        SourceParseResult object
    """
    # Reconstruct dataclass objects
    data['functions'] = [_deserialize_function_def(f) for f in data['functions']]
    data['macros'] = [MacroDefinition(**m) for m in data['macros']]
    return SourceParseResult(**data)


def _serialize_function_def(func: 'FunctionDefinition') -> dict:
    """Serialize FunctionDefinition including parameters."""
    data = asdict(func)
    data['parameters'] = [asdict(p) for p in func.parameters]
    return data


def _deserialize_function_def(data: dict) -> 'FunctionDefinition':
    """Deserialize FunctionDefinition including parameters."""
    data['parameters'] = [FunctionParameter(**p) for p in data['parameters']]
    return FunctionDefinition(**data)


class TreeSitterCParser:
    """
    Tree-sitter based C code parser.

    Uses AST (Abstract Syntax Tree) for accurate extraction of:
    - Function declarations
    - Function parameters
    - Doxygen comments
    - Macros and typedefs
    """

    def __init__(self):
        """Initialize the parser."""
        self.parser = None
        self.language = None
        self._initialized = False
        self._source_bytes = None  # Store raw bytes for correct text extraction

    def _extract_node_text(self, node, source_bytes: bytes) -> str:
        """
        Extract text from a Tree-sitter node using raw bytes.

        This is necessary because Tree-sitter returns byte offsets, but when
        the source is decoded to a string, multi-byte UTF-8 characters cause
        character indices to not match byte offsets.

        Args:
            node: Tree-sitter node
            source_bytes: Raw source bytes

        Returns:
            Extracted text as string
        """
        return source_bytes[node.start_byte:node.end_byte].decode('utf-8', errors='ignore')

    def initialize(self) -> bool:
        """
        Initialize Tree-sitter parser and C language.

        Returns:
            True if initialization successful, False otherwise.
        """
        try:
            import tree_sitter

            # Try multiple language module options
            self.language = None

            # Try tree_sitter_c_language (most common)
            try:
                from tree_sitter_c_language import get_language
                self.language = get_language()
                logger.info("Using tree_sitter_c_language")
            except ImportError:
                pass

            # Try tree_sitter_language_pack (alternative)
            if not self.language:
                try:
                    from tree_sitter_language_pack import get_language
                    self.language = get_language('c')
                    logger.info("Using tree_sitter_language_pack")
                except ImportError:
                    pass

            # Try tree_sitter_c (another alternative)
            if not self.language:
                try:
                    from tree_sitter_c import language
                    self.language = tree_sitter.Language(language())
                    logger.info("Using tree_sitter_c")
                except ImportError:
                    pass

            if not self.language:
                logger.error("No tree-sitter C language module found")
                logger.error("Please install one of:")
                logger.error("  pip install tree-sitter-c")
                return False

            self.parser = tree_sitter.Parser(self.language)
            self._initialized = True

            logger.info("Tree-sitter C parser initialized successfully")
            return True

        except ImportError as e:
            logger.error(f"Failed to import tree-sitter: {e}")
            logger.error("Please install: pip install tree-sitter")
            logger.error("And one of: tree-sitter-c-language, tree-sitter-language-pack, or tree-sitter-c")
            return False
        except Exception as e:
            logger.error(f"Failed to initialize parser: {e}")
            return False

    def _preprocess_for_parsing(self, source_code: str) -> str:
        """
        Preprocess source code to handle non-standard compiler extensions.

        Tree-sitter's C parser doesn't understand compiler-specific keywords like:
        - __forceinline (MSVC)
        - __INLINE__ (macro for inline)
        - __attribute__ (GCC)
        - __asm (inline assembly)

        This method replaces these with standard C keywords to allow parsing.

        Args:
            source_code: Original source code

        Returns:
            Preprocessed source code that Tree-sitter can parse
        """
        import re

        result = source_code

        # Replace __forceinline followed by static with static inline
        result = re.sub(r'\b__forceinline\s+static\b', 'static inline', result)

        # Replace __forceinline alone with inline
        result = re.sub(r'\b__forceinline\b', 'inline', result)

        # Replace __INLINE__ macro (commonly defined as __forceinline static)
        result = re.sub(r'\b__INLINE__\b', 'static inline', result)

        # Remove GCC __attribute__((...)) - handle nested parens carefully
        # Use a function to handle nested parentheses
        def remove_attribute(match):
            return ''

        # Simple pattern for __attribute__ with single-level parens
        result = re.sub(r'\b__attribute__\s*\(\([^()]+\)\)', '', result)

        # Handle _Bool (C99 boolean) - replace with int
        result = re.sub(r'\b_Bool\b', 'int', result)

        return result

    def parse_header_file(self, file_path: str) -> Optional[HeaderParseResult]:
        """
        Parse a C header file and extract function declarations.

        Args:
            file_path: Path to the header file (.h)

        Returns:
            HeaderParseResult with extracted information, or None if parsing failed.
        """
        if not self._initialized:
            if not self.initialize():
                return None

        header_path = Path(file_path)
        if not header_path.exists():
            logger.error(f"File not found: {file_path}")
            return None

        try:
            # Read file content as binary to preserve byte offsets
            with open(header_path, 'rb') as f:
                source_bytes = f.read()

            # Decode to string for text extraction (preserving original line endings)
            source_code = source_bytes.decode('utf-8', errors='ignore')

            # Preprocess source code to handle non-standard compiler extensions
            # Replace compiler-specific keywords with standard C keywords
            preprocessed_code = self._preprocess_for_parsing(source_code)
            preprocessed_bytes = preprocessed_code.encode('utf-8')

            # Store original bytes for correct text extraction
            self._source_bytes = source_bytes

            # Parse with Tree-sitter using preprocessed bytes
            tree = self.parser.parse(preprocessed_bytes)
            root_node = tree.root_node

            # Extract declarations
            result = HeaderParseResult(file_path=str(header_path))
            result.functions = self._extract_functions(root_node, preprocessed_bytes, source_code, str(header_path))
            result.macros = self._extract_macros(root_node, preprocessed_bytes, source_code)
            result.typedefs = self._extract_typedefs(root_node, preprocessed_bytes, source_code)

            # Check for parse errors
            if root_node.has_error:
                result.parse_errors = self._collect_parse_errors(root_node, source_code)
                if result.parse_errors:
                    logger.debug(f"Parse errors found in {file_path}: {len(result.parse_errors)} errors")

            logger.info(f"Parsed {file_path}: {len(result.functions)} functions found")
            return result

        except Exception as e:
            logger.error(f"Failed to parse {file_path}: {e}")
            return None

    def parse_header_file_full(self, file_path: str) -> Optional[FullParseResult]:
        """
        Parse a C header file and extract ALL content (functions, macros, enums).

        Args:
            file_path: Path to the header file (.h)

        Returns:
            FullParseResult with extracted information, or None if parsing failed.
        """
        if not self._initialized:
            if not self.initialize():
                return None

        header_path = Path(file_path)
        if not header_path.exists():
            logger.error(f"File not found: {file_path}")
            return None

        try:
            # Read file content as binary to preserve byte offsets
            with open(header_path, 'rb') as f:
                source_bytes = f.read()

            # Decode to string for text extraction (preserving original line endings)
            source_code = source_bytes.decode('utf-8', errors='ignore')

            # Preprocess source code to handle non-standard compiler extensions
            preprocessed_code = self._preprocess_for_parsing(source_code)
            preprocessed_bytes = preprocessed_code.encode('utf-8')

            # Store original bytes for correct text extraction
            self._source_bytes = source_bytes

            # Parse with Tree-sitter using preprocessed bytes
            tree = self.parser.parse(preprocessed_bytes)
            root_node = tree.root_node

            # Extract all content
            result = FullParseResult(file_path=str(header_path))
            result.functions = self._extract_functions(root_node, preprocessed_bytes, source_code, str(header_path))
            result.macros = self._extract_macros_full(root_node, source_bytes, source_code)
            result.enums = self._extract_enums_full(root_node, source_bytes, source_code)
            result.typedefs = self._extract_typedefs(root_node, source_bytes, source_code)

            # Set file paths and line numbers for macros and enums
            for macro in result.macros:
                if not macro.file_path:
                    macro.file_path = str(header_path)
            for enum in result.enums:
                if not enum.file_path:
                    enum.file_path = str(header_path)

            # Check for parse errors
            if root_node.has_error:
                result.parse_errors = self._collect_parse_errors(root_node, source_code)
                logger.debug(f"Parse errors found in {file_path}: {len(result.parse_errors)} errors")

            logger.info(f"Parsed {file_path}: {len(result.functions)} functions, "
                       f"{len(result.macros)} macros, {len(result.enums)} enums")
            return result

        except Exception as e:
            logger.error(f"Failed to parse {file_path}: {e}")
            return None

    def parse_source_file(self, file_path: str) -> Optional[SourceParseResult]:
        """
        Parse a C source file and extract function implementations.

        Args:
            file_path: Path to the source file (.c)

        Returns:
            SourceParseResult with extracted information, or None if parsing failed.
        """
        if not self._initialized:
            if not self.initialize():
                return None

        source_path = Path(file_path)
        if not source_path.exists():
            logger.error(f"File not found: {file_path}")
            return None

        try:
            # Read file content as binary to preserve byte offsets
            with open(source_path, 'rb') as f:
                source_bytes = f.read()

            # Decode to string for text extraction (preserving original line endings)
            source_code = source_bytes.decode('utf-8', errors='ignore')

            # Preprocess source code to handle non-standard compiler extensions
            preprocessed_code = self._preprocess_for_parsing(source_code)
            preprocessed_bytes = preprocessed_code.encode('utf-8')

            # Store original bytes for correct text extraction
            self._source_bytes = source_bytes

            # Parse with Tree-sitter using preprocessed bytes
            tree = self.parser.parse(preprocessed_bytes)
            root_node = tree.root_node

            # Extract content
            result = SourceParseResult(file_path=str(source_path))
            result.functions = self._extract_function_definitions(root_node, preprocessed_bytes, source_code, str(source_path))
            result.macros = self._extract_macros_full(root_node, preprocessed_bytes, source_code)

            # Set file paths for macros
            for macro in result.macros:
                if not macro.file_path:
                    macro.file_path = str(source_path)

            # Check for parse errors
            if root_node.has_error:
                result.parse_errors = self._collect_parse_errors(root_node, source_code)
                logger.debug(f"Parse errors found in {file_path}: {len(result.parse_errors)} errors")

            logger.info(f"Parsed {file_path}: {len(result.functions)} function definitions found")
            return result

        except Exception as e:
            logger.error(f"Failed to parse {file_path}: {e}")
            return None

    def _extract_function_definitions(
        self,
        root_node,
        source_bytes: bytes,
        source_code: str,
        file_path: str
    ) -> List[FunctionDefinition]:
        """
        Extract function definitions from AST (.c files).

        Args:
            root_node: Tree-sitter root node
            source_bytes: Raw source bytes for correct text extraction
            source_code: Original source code as string
            file_path: File path for error reporting

        Returns:
            List of FunctionDefinition objects
        """
        functions = []

        def find_function_definitions(node):
            """Recursively find all function_definition nodes."""
            if node.type == 'function_definition':
                func_def = self._extract_single_function_definition(node, source_bytes, source_code, file_path)
                if func_def:
                    functions.append(func_def)

            for child in node.children:
                find_function_definitions(child)

        find_function_definitions(root_node)
        return functions

    def _extract_single_function_definition(
        self,
        func_node,
        source_bytes: bytes,
        source_code: str,
        file_path: str
    ) -> Optional[FunctionDefinition]:
        """
        Extract a single function definition from a function_definition node.

        Args:
            func_node: Tree-sitter function_definition node
            source_bytes: Raw source bytes for correct text extraction
            source_code: Original source code as string
            file_path: File path

        Returns:
            FunctionDefinition or None
        """
        try:
            # Get preceding Doxygen comment (needs source_bytes for correct extraction)
            doxygen_comment = self._extract_doxygen_comment(func_node, source_code)
            brief, detailed = self._parse_doxygen_comment(doxygen_comment) if doxygen_comment else ("", "")

            # Extract function information
            func = FunctionDefinition()

            # Check for static
            func.is_static = False
            for child in func_node.children:
                if child.type == 'storage_class_specifier':
                    text = self._extract_node_text(child, source_bytes)
                    if text == 'static':
                        func.is_static = True

            # Parse return type and name
            func_type = func_node.child_by_field_name('type')
            func_declarator = func_node.child_by_field_name('declarator')

            if func_type:
                func.return_type = self._extract_node_text(func_type, source_bytes).strip()

            if func_declarator:
                func.name, func.parameters = self._extract_parameters(func_declarator, source_bytes)

            # Extract function body
            body_node = func_node.child_by_field_name('body')
            if body_node:
                func.function_body = self._extract_node_text(body_node, source_bytes).strip()

            func.file_path = file_path
            func.line_number = func_node.start_point[0] + 1

            return func

        except Exception as e:
            logger.debug(f"Failed to extract function definition from node: {e}")
            return None

    def _extract_functions(
        self,
        root_node,
        source_bytes: bytes,
        source_code: str,
        file_path: str
    ) -> List[FunctionDeclaration]:
        """
        Extract function declarations from AST.

        Args:
            root_node: Tree-sitter root node
            source_bytes: Raw source bytes for correct text extraction
            source_code: Original source code as string
            file_path: File path for error reporting

        Returns:
            List of FunctionDeclaration objects
        """
        functions = []

        def find_declarations_recursive(node):
            """Recursively find all declaration nodes in the AST."""
            # Check if this node is a declaration
            if node.type == 'declaration':
                func = self._extract_function_declaration(node, source_bytes, source_code)
                if func:
                    if func.name:  # Only add if function has a name
                        func.file_path = file_path
                        functions.append(func)
                    else:
                        # Debug: why was name not extracted?
                        logger.debug(f"Function extracted but name is empty. Return type: '{func.return_type}'")

            # Recursively search children
            for child in node.children:
                find_declarations_recursive(child)

        # Start recursive search from root
        find_declarations_recursive(root_node)

        return functions

    def _extract_function_declaration(
        self,
        node,
        source_bytes: bytes,
        source_code: str
    ) -> Optional[FunctionDeclaration]:
        """
        Extract single function declaration from AST node.

        Args:
            node: Tree-sitter declaration node
            source_bytes: Raw source bytes for correct text extraction
            source_code: Original source code as string (for Doxygen parsing)

        Returns:
            FunctionDeclaration or None
        """
        try:
            # Get preceding Doxygen comment
            doxygen_comment = self._extract_doxygen_comment(node, source_code)

            # Extract return type and declarator
            func_type = node.child_by_field_name('type')
            func_declarator = node.child_by_field_name('declarator')

            # Extract return type
            return_type = ""
            if func_type:
                return_type = self._extract_node_text(func_type, source_bytes).strip()

            # Extract function name and parameters
            function_name = ""
            parameters = []
            if func_declarator:
                function_name, parameters = self._extract_parameters(func_declarator, source_bytes)

            # Parse Doxygen descriptions
            brief_description = ""
            detailed_description = ""
            is_exported = False
            if doxygen_comment:
                brief_description, detailed_description = self._parse_doxygen_comment(doxygen_comment)
                is_exported = 'B6x_API_EXPORT' in doxygen_comment or 'API_EXPORT' in doxygen_comment

            line_number = node.start_point[0] + 1  # Convert to 1-indexed

            # Create FunctionDeclaration object with required parameters
            func = FunctionDeclaration(
                name=function_name,
                return_type=return_type,
                parameters=parameters,
                brief_description=brief_description,
                detailed_description=detailed_description,
                line_number=line_number,
                is_exported=is_exported
            )

            return func

        except Exception as e:
            logger.debug(f"Failed to extract function from node: {e}")
            return None

    def _extract_parameters(
        self,
        declarator_node,
        source_bytes: bytes
    ) -> Tuple[str, List[FunctionParameter]]:
        """
        Extract function name and parameters from declarator.

        Args:
            declarator_node: Tree-sitter function_declarator node
            source_bytes: Raw source bytes for correct text extraction

        Returns:
            Tuple of (function_name, List[FunctionParameter])
        """
        function_name = ""
        parameters = []

        # Get function name (direct child of function_declarator)
        for child in declarator_node.children:
            if child.type == 'identifier':
                function_name = self._extract_node_text(child, source_bytes)
            elif child.type == 'parameter_list':
                parameters = self._extract_parameter_list(child, source_bytes)

        return function_name, parameters

    def _extract_parameter_list(
        self,
        parameter_list_node,
        source_bytes: bytes
    ) -> List[FunctionParameter]:
        """
        Extract parameters from parameter list node.

        Args:
            parameter_list_node: Tree-sitter parameter_list node
            source_bytes: Raw source bytes for correct text extraction

        Returns:
            List of FunctionParameter objects
        """
        parameters = []

        for child in parameter_list_node.children:
            if child.type == 'parameter_declaration':
                param = self._extract_single_parameter(child, source_bytes)
                if param:
                    parameters.append(param)

        return parameters

    def _extract_single_parameter(
        self,
        param_node,
        source_bytes: bytes
    ) -> Optional[FunctionParameter]:
        """
        Extract single parameter from parameter_declaration node.

        Args:
            param_node: Tree-sitter parameter_declaration node
            source_bytes: Raw source bytes for correct text extraction

        Returns:
            FunctionParameter or None
        """
        try:
            param_type_node = param_node.child_by_field_name('type')
            param_declarator = param_node.child_by_field_name('declarator')

            param_type = ""
            param_name = ""

            if param_type_node:
                param_type = self._extract_node_text(param_type_node, source_bytes).strip()

            if param_declarator:
                # Get the identifier from declarator
                for child in param_declarator.children:
                    if child.type == 'identifier':
                        param_name = self._extract_node_text(child, source_bytes)
                        break

            return FunctionParameter(
                name=param_name or "",
                type=param_type or "",
                description=""  # Will be filled from Doxygen @param tags
            )

        except Exception as e:
            logger.debug(f"Failed to extract parameter: {e}")
            return None

    def _extract_doxygen_comment(self, node, source_code: str) -> Optional[str]:
        """
        Extract Doxygen comment preceding a node.

        Args:
            node: Tree-sitter node
            source_code: Original source code

        Returns:
            Doxygen comment string or None
        """
        # Look for comment before the function declaration
        # Tree-sitter stores comments as siblings in the tree
        current_node = node.prev_sibling

        comments = []
        while current_node:
            if current_node.type in ('comment', 'block_comment'):
                comment_text = source_code[current_node.start_byte:current_node.end_byte]
                comments.append(comment_text)

                # Stop if we hit a non-comment (we only want comments immediately preceding)
                if current_node.prev_sibling and current_node.prev_sibling.type not in ('comment', 'block_comment'):
                    break
            else:
                # Stop if we hit a non-comment statement
                if current_node.type != 'comment':
                    break

            current_node = current_node.prev_sibling

        if comments:
            # Reverse to get correct order (top to bottom)
            comments.reverse()
            return '\n'.join(comments)

        return None

    def _parse_doxygen_comment(self, comment: str) -> Tuple[str, str]:
        """
        Parse Doxygen comment to extract brief and detailed descriptions.

        Args:
            comment: Doxygen comment string

        Returns:
            Tuple of (brief_description, detailed_description)
        """
        # Remove comment markers (/**, */, *, //)
        lines = []
        for line in comment.split('\n'):
            line = line.strip()
            line = line.removeprefix('/**')
            line = line.removeprefix('/*')
            line = line.removeprefix('*')
            line = line.removeprefix('//')
            line = line.removesuffix('*/')
            line = line.strip()
            lines.append(line)

        comment_text = '\n'.join(lines)

        # Extract @brief
        brief = ""
        detailed = []

        in_brief = False
        in_detailed = False

        for line in lines:
            line = line.strip()
            if line.startswith('@brief'):
                brief = line.removeprefix('@brief').strip()
                in_brief = True
            elif line.startswith('@'):
                # Other Doxygen commands
                in_brief = False
                in_detailed = True
                detailed.append(line)
            elif in_brief and brief:
                # Continuation of brief
                brief += ' ' + line
            else:
                if line:
                    in_detailed = True
                    detailed.append(line)

        return brief, '\n'.join(detailed)

    def _extract_macros(self, root_node, source_bytes: bytes, source_code: str) -> List[Dict]:
        """
        Extract macro definitions from AST (legacy - kept for compatibility).

        Args:
            root_node: Tree-sitter root node
            source_code: Original source code

        Returns:
            List of macro definitions
        """
        macros = []

        # Tree-sitter doesn't parse preprocessor directives by default
        # This is a placeholder for future enhancement
        # For now, we'll return empty list

        return macros

    def _extract_macros_full(self, root_node, source_bytes: bytes, source_code: str) -> List[MacroDefinition]:
        """
        Extract macro definitions using regex preprocessor parsing.

        Tree-sitter C parser doesn't handle preprocessor directives by default.
        We use regex to find #define directives.

        Args:
            root_node: Tree-sitter root node (not used, kept for interface consistency)
            source_bytes: Raw source bytes for correct text extraction
            source_code: Original source code as string (for regex matching)

        Returns:
            List of MacroDefinition objects
        """
        import re

        macros = []

        # Pattern for #define directives
        # Matches: #define NAME value
        # Handles both simple macros and macros with continuation lines
        define_pattern = re.compile(
            r'^\s*#define\s+(\w+)\s+(.+?)(?=\s*/\*|\s*$|\\)',
            re.MULTILINE
        )

        # Also find preproc_def nodes from tree-sitter (if available)
        # Some tree-sitter C versions include these
        for node in root_node.children:
            if node.type == 'preproc_def':
                try:
                    text = self._extract_node_text(node, source_bytes)
                    # Extract name and value
                    match = re.match(r'#\s*define\s+(\w+)\s+(.*)', text, re.DOTALL)
                    if match:
                        name = match.group(1)
                        value = match.group(2).strip()

                        # Clean up value
                        value = re.sub(r'\s*/\*.*?\*/', '', value)  # Remove comments
                        value = value.rstrip('\\').strip()
                        value = re.sub(r'\s+', ' ', value)

                        # Get Doxygen comment
                        doxygen = self._extract_doxygen_comment(node, source_code)
                        brief, _ = self._parse_doxygen_comment(doxygen) if doxygen else ("", "")

                        # Classify macro
                        macro_type = self.classify_macro(name, value)

                        macro = MacroDefinition(
                            name=name,
                            value=value,
                            macro_type=macro_type,
                            brief=brief,
                            file_path="",  # Will be set by caller
                            line_number=node.start_point[0] + 1
                        )
                        macros.append(macro)

                except Exception as e:
                    logger.debug(f"Failed to extract macro from node: {e}")

        # If tree-sitter didn't find preproc_def nodes, use regex
        if not macros:
            lines = source_code.split('\n')
            i = 0
            while i < len(lines):
                line = lines[i]
                match = define_pattern.match(line)
                if match:
                    name = match.group(1)
                    value = match.group(2).strip()

                    # Handle continuation lines
                    while value.endswith('\\'):
                        i += 1
                        if i < len(lines):
                            next_line = lines[i].strip()
                            # Remove comments from continuation line
                            next_line = re.sub(r'/\*.*?\*/', '', next_line)
                            value = value[:-1].strip() + ' ' + next_line
                        else:
                            break

                    # Clean up value
                    value = re.sub(r'/\*.*?\*/', '', value)  # Remove inline comments
                    value = value.strip()

                    # Classify macro
                    macro_type = self.classify_macro(name, value)

                    macro = MacroDefinition(
                        name=name,
                        value=value,
                        macro_type=macro_type,
                        brief="",
                        file_path="",  # Will be set by caller
                        line_number=i + 1
                    )
                    macros.append(macro)

                i += 1

        return macros

    def classify_macro(self, name: str, value: str) -> str:
        """
        Classify macro into type category.

        Args:
            name: Macro name
            value: Macro value

        Returns:
            Macro type: 'register', 'bitmask', 'constant', 'other'
        """
        name_upper = name.upper()
        value_stripped = value.strip()

        # Check for register addresses
        if any(keyword in name_upper for keyword in ['BASE', 'ADDR', 'OFFSET']):
            return 'register'

        # Check for bitmasks (bit operations)
        if '<<' in value_stripped or '>>' in value_stripped:
            return 'bitmask'

        if '|' in value_stripped or '&' in value_stripped or '^' in value_stripped:
            # Could be bitmask if the value looks like (1 << N)
            if '(' in value_stripped or any(c.isdigit() for c in value_stripped):
                return 'bitmask'

        # Check for hexadecimal values (likely constants)
        if value_stripped.startswith('0x') or value_stripped.startswith('0X'):
            return 'constant'

        # Check for simple numeric values
        if value_stripped.isdigit() or (value_stripped.startswith('-') and value_stripped[1:].isdigit()):
            return 'constant'

        # Check for function-like macros (has parentheses)
        if '(' in value_stripped and ')' in value_stripped:
            return 'function_macro'

        # Default: other
        return 'other'

    def _extract_enums_full(self, root_node, source_bytes: bytes, source_code: str) -> List[EnumDefinition]:
        """
        Extract enum definitions from AST.

        Args:
            root_node: Tree-sitter root node
            source_code: Original source code

        Returns:
            List of EnumDefinition objects
        """
        enums = []

        def find_enum_specifiers(node):
            """Recursively find all enum_specifier nodes."""
            if node.type == 'enum_specifier':
                enum_def = self._extract_single_enum(node, source_code)
                if enum_def:
                    enums.append(enum_def)

            for child in node.children:
                find_enum_specifiers(child)

        find_enum_specifiers(root_node)
        return enums

    def _extract_single_enum(self, enum_node, source_code: str) -> Optional[EnumDefinition]:
        """
        Extract a single enum definition from an enum_specifier node.

        Args:
            enum_node: Tree-sitter enum_specifier node
            source_code: Original source code

        Returns:
            EnumDefinition or None
        """
        try:
            # Get Doxygen comment
            doxygen = self._extract_doxygen_comment(enum_node, source_code)
            brief, _ = self._parse_doxygen_comment(doxygen) if doxygen else ("", "")

            # Extract enum name (from typedef if present)
            enum_name = ""

            # Check if this is a typedef enum
            # Parent might be a declaration with type_specifier
            parent = enum_node.parent
            if parent and parent.type == 'type_qualifier':
                grandparent = parent.parent
                if grandparent and grandparent.type == 'declaration':
                    # Look for typedef declarator
                    for child in grandparent.children:
                        if child.type == 'type_qualifier' and source_code[child.start_byte:child.end_byte] == 'typedef':
                            # Find the declarator with the name
                            for gc in grandparent.children:
                                if gc.type == 'declarator':
                                    for gcc in gc.children:
                                        if gcc.type == 'identifier':
                                            enum_name = source_code[gcc.start_byte:gcc.end_byte]
                                            break

            # Extract enum values
            enum_values = self._extract_enum_values(enum_node, source_code)

            # Try to get underlying type (C++ feature, not common in C)
            underlying_type = ""

            enum_def = EnumDefinition(
                name=enum_name,
                values=enum_values,
                underlying_type=underlying_type,
                brief=brief,
                file_path="",  # Will be set by caller
                line_number=enum_node.start_point[0] + 1
            )

            return enum_def

        except Exception as e:
            logger.debug(f"Failed to extract enum: {e}")
            return None

    def _extract_enum_values(self, enum_node, source_code: str) -> List[EnumValue]:
        """
        Extract enum values from an enum_specifier node.

        Args:
            enum_node: Tree-sitter enum_specifier node
            source_code: Original source code

        Returns:
            List of EnumValue objects
        """
        enum_values = []
        current_value = 0

        # Find the enumerator_list
        for child in enum_node.children:
            if child.type == 'enumerator_list':
                for enumerator in child.children:
                    if enumerator.type == 'enumerator':
                        # Get enumerator name
                        name_node = enumerator.child_by_field_name('name')
                        if name_node and name_node.type == 'identifier':
                            name = source_code[name_node.start_byte:name_node.end_byte]

                            # Get enumerator value
                            value_node = enumerator.child_by_field_name('value')
                            if value_node:
                                value_text = source_code[value_node.start_byte:value_node.end_byte].strip()

                                # Try to evaluate the value
                                try:
                                    # Handle simple expressions
                                    if value_text.isdigit():
                                        current_value = int(value_text)
                                    elif value_text.startswith('0x') or value_text.startswith('0X'):
                                        current_value = int(value_text, 16)
                                    elif value_text.startswith('0b') or value_text.startswith('0B'):
                                        current_value = int(value_text, 2)
                                    else:
                                        # Try to evaluate simple arithmetic
                                        # This is limited and might fail for complex expressions
                                        current_value = eval(value_text, {"__builtins__": {}}, {})
                                except:
                                    # If evaluation fails, keep the current auto-increment value
                                    pass
                            else:
                                # No explicit value, use auto-increment
                                value_text = str(current_value)

                            # Get Doxygen comment for this value
                            doxygen = self._extract_doxygen_comment(enumerator, source_code)
                            brief, _ = self._parse_doxygen_comment(doxygen) if doxygen else ("", "")

                            enum_value = EnumValue(
                                name=name,
                                value=current_value,
                                brief=brief
                            )
                            enum_values.append(enum_value)

                            # Auto-increment for next value
                            current_value += 1

                break

        return enum_values

    def _extract_typedefs(self, root_node, source_bytes: bytes, source_code: str) -> List[Dict]:
        """
        Extract typedef declarations from AST.

        Args:
            root_node: Tree-sitter root node
            source_code: Original source code

        Returns:
            List of typedef declarations
        """
        typedefs = []

        for node in root_node.children:
            if node.type == 'declaration':
                # Check if this is a typedef
                for child in node.children:
                    if child.type == 'type_specifier' and source_code[child.start_byte:child.end_byte] == 'typedef':
                        typedefs.append({
                            'text': source_code[node.start_byte:node.end_byte],
                            'line': node.start_point[0] + 1
                        })
                        break

        return typedefs

    def _collect_parse_errors(self, root_node, source_code: str) -> List[str]:
        """
        Collect parse error information.

        Args:
            root_node: Tree-sitter root node
            source_code: Original source code

        Returns:
            List of error messages
        """
        errors = []

        def find_errors(node):
            if node.has_error:
                if node.type == 'ERROR':
                    line = node.start_point[0] + 1
                    col = node.start_point[1] + 1
                    error_text = source_code[node.start_byte:node.end_byte]
                    errors.append(f"Line {line}, Col {col}: '{error_text}'")

                for child in node.children:
                    find_errors(child)

        find_errors(root_node)
        return errors

    def match_declarations_to_implementations(
        self,
        headers: List[FullParseResult],
        sources: List[SourceParseResult]
    ) -> List[FunctionDeclaration]:
        """
        Match function declarations (.h) with implementations (.c).

        Args:
            headers: List of FullParseResult from .h files
            sources: List of SourceParseResult from .c files

        Returns:
            List of FunctionDeclaration with implementation info populated
        """
        # Build a map of function name -> implementation
        impl_map = {}

        for source in sources:
            for func_def in source.functions:
                impl_map[func_def.name] = func_def

        # Match declarations with implementations
        matched_functions = []

        for header in headers:
            for func_decl in header.functions:
                # Copy the declaration
                matched_func = FunctionDeclaration(
                    name=func_decl.name,
                    return_type=func_decl.return_type,
                    parameters=func_decl.parameters,
                    brief_description=func_decl.brief_description,
                    detailed_description=func_decl.detailed_description,
                    file_path=func_decl.file_path,
                    line_number=func_decl.line_number,
                    is_exported=func_decl.is_exported
                )

                # Check if we have an implementation
                if func_decl.name in impl_map:
                    impl = impl_map[func_decl.name]
                    matched_func.implementation_file = impl.file_path
                    matched_func.implementation_line = impl.line_number
                    matched_func.has_implementation = True

                matched_functions.append(matched_func)

        logger.info(f"Matched {len(matched_functions)} functions "
                   f"({sum(1 for f in matched_functions if f.has_implementation)} with implementations)")
        return matched_functions

    def parse_example_file(
        self,
        file_path: str,
        known_sdk_apis: set,
        example_type: str = 'examples',
        extract_context: bool = False,
        context_lines: int = 5
    ) -> Optional[ExampleParseResult]:
        """
        Parse an example file to extract function calls.

        Args:
            file_path: Path to the example .c file
            known_sdk_apis: Set of known SDK API names
            example_type: 'examples' or 'projects'
            extract_context: Whether to extract extended context (default: False)
            context_lines: Number of context lines before/after (default: 5)

        Returns:
            ExampleParseResult or None
        """
        if not self._initialized:
            logger.error("Parser not initialized")
            return None

        file_path_obj = Path(file_path)
        if not file_path_obj.exists():
            logger.warning(f"File not found: {file_path}")
            return None

        try:
            # Read file content as binary to preserve byte offsets
            with open(file_path, 'rb') as f:
                source_bytes = f.read()

            # Decode to string for text extraction (preserving original line endings)
            source_code = source_bytes.decode('utf-8', errors='ignore')

            lines = source_code.split('\n')

            # Parse the file using original bytes
            tree = self.parser.parse(source_bytes)
            root_node = tree.root_node

            # Extract example name from path
            example_name = self._extract_example_name(file_path, example_type)

            # Extract function calls (with or without extended context)
            function_calls = self._extract_function_calls(
                root_node, source_code, lines, known_sdk_apis, example_name,
                extract_context=extract_context, context_lines=context_lines
            )

            # Extract defined functions
            functions_defined = self._extract_function_names(root_node)

            # Extract included headers
            included_headers = self._extract_included_headers(root_node)

            result = ExampleParseResult(
                file_path=file_path,
                example_name=example_name,
                example_type=example_type,
                function_calls=function_calls,
                functions_defined=functions_defined,
                included_headers=included_headers
            )

            logger.debug(f"Parsed example file: {file_path} - {len(function_calls)} SDK API calls")
            return result

        except RecursionError as e:
            # Skip files with deeply nested structures (e.g., auto-generated code)
            logger.debug(f"Skipping file due to deep nesting: {file_path}")
            return None
        except Exception as e:
            logger.error(f"Error parsing example file {file_path}: {e}")
            return None

    def _extract_example_name(self, file_path: str, example_type: str) -> str:
        """Extract example name from file path."""
        path = Path(file_path)
        parts = path.parts

        # Find 'examples' or 'projects' in path
        for i, part in enumerate(parts):
            if part == example_type and i + 1 < len(parts):
                return parts[i + 1]

        return path.parent.name

    def _extract_function_calls(
        self,
        root_node,
        source_code: str,
        lines: List[str],
        known_sdk_apis: set,
        example_name: str,
        extract_context: bool = False,
        context_lines: int = 5
    ) -> List[FunctionCall]:
        """
        Extract all function call expressions from the AST.

        Args:
            root_node: Tree-sitter root node
            source_code: Source code as string
            lines: Source code split into lines
            known_sdk_apis: Set of known SDK API names
            example_name: Example directory name
            extract_context: Whether to extract extended context
            context_lines: Number of context lines to extract

        Returns:
            List of FunctionCall objects (or FunctionCallWithContext if extract_context=True)
        """
        calls = []
        all_sdk_calls = []  # Track all SDK calls for related APIs

        def find_call_expressions(node, parent_function: str = "", call_depth: int = 0):
            if node.type == 'function_definition':
                # Extract function name for context
                for child in node.children:
                    if child.type == 'function_declarator':
                        for subchild in child.children:
                            if subchild.type == 'identifier':
                                parent_function = subchild.text.decode('utf-8')
                                break

            if node.type == 'call_expression':
                call = self._parse_call_expression(
                    node, source_code, lines, known_sdk_apis, example_name, parent_function,
                    extract_context, context_lines
                )
                if call:
                    calls.append(call)
                    # Track SDK calls for context relationship
                    if isinstance(call, FunctionCall):
                        if call.is_sdk_api:
                            all_sdk_calls.append((call, node.start_point[0]))
                    elif isinstance(call, FunctionCallWithContext):
                        if call.base.is_sdk_api:
                            all_sdk_calls.append((call, node.start_point[0]))

            for child in node.children:
                find_call_expressions(child, parent_function, call_depth)

        find_call_expressions(root_node)

        # If context extraction is enabled, populate related APIs
        if extract_context:
            for i, (call, line_num) in enumerate(all_sdk_calls):
                if isinstance(call, FunctionCallWithContext):
                    # Find related APIs (within 20 lines)
                    for other_call, other_line in all_sdk_calls:
                        if other_call is not call and abs(other_line - line_num) <= 20:
                            api_name = other_call.base.function_name if isinstance(other_call, FunctionCallWithContext) else other_call.function_name
                            call.related_apis.add(api_name)

        return calls

    def _parse_call_expression(
        self,
        call_node,
        source_code: str,
        lines: List[str],
        known_sdk_apis: set,
        example_name: str,
        parent_function: str,
        extract_context: bool = False,
        context_lines: int = 5
    ) -> Optional[FunctionCall]:
        """
        Parse a single call_expression node.

        Args:
            call_node: Tree-sitter call_expression node
            source_code: Source code as string
            lines: Source code split into lines
            known_sdk_apis: Set of known SDK API names
            example_name: Example directory name
            parent_function: Name of containing function
            extract_context: Whether to extract extended context
            context_lines: Number of context lines to extract

        Returns:
            FunctionCall or FunctionCallWithContext
        """
        # Find the function being called
        function_node = None
        for child in call_node.children:
            if child.type == 'identifier':
                function_node = child
                break

        if not function_node:
            return None

        function_name = function_node.text.decode('utf-8')

        # Check if this is an SDK API call
        is_sdk_api = function_name in known_sdk_apis

        # Get line and column numbers
        line_number = call_node.start_point[0] + 1
        column_number = call_node.start_point[1] + 1

        # Extract arguments
        arguments = []
        for child in call_node.children:
            if child.type == 'argument_list':
                for arg in child.children:
                    if arg.type in ('identifier', 'number_literal', 'string_literal', 'call_expression'):
                        arg_text = arg.text.decode('utf-8').strip()
                        if arg_text and arg_text != ',':
                            arguments.append(arg_text[:50])  # Limit argument length

        # Classify the API call
        module, peripheral = self._classify_api_call(function_name)

        # Create base FunctionCall
        base_call = FunctionCall(
            function_name=function_name,
            file_path="",  # Will be set by caller
            line_number=line_number,
            column_number=column_number,
            calling_function=parent_function,
            arguments=arguments,
            is_sdk_api=is_sdk_api,
            module=module,
            peripheral=peripheral,
            context_snippet="",  # Will be filled if needed
            example_name=example_name
        )

        # If extended context is requested, create FunctionCallWithContext
        if extract_context:
            context_before, context_after = self._extract_extended_context(
                lines, line_number, context_lines
            )

            # Detect if in initialization section
            is_in_init = self._is_in_init_section(parent_function, lines, line_number)

            # Calculate call depth
            call_depth = self._calculate_call_depth(call_node)

            return FunctionCallWithContext(
                base=base_call,
                context_before=context_before,
                context_after=context_after,
                enclosing_function=parent_function,
                call_depth=call_depth,
                is_in_init_section=is_in_init,
                related_apis=set()  # Will be populated by caller
            )
        else:
            # Legacy behavior: simple context snippet
            context_snippet = self._extract_context_snippet(lines, line_number, context_lines=2)
            base_call.context_snippet = context_snippet
            return base_call

    def _extract_context_snippet(self, lines: List[str], line_number: int, context_lines: int = 2) -> str:
        """Extract context code around a line."""
        start = max(0, line_number - context_lines - 1)
        end = min(len(lines), line_number + context_lines)

        snippet_lines = []
        for i in range(start, end):
            prefix = ">>> " if i == line_number - 1 else "    "
            snippet_lines.append(f"{prefix}{lines[i]}")

        return '\n'.join(snippet_lines)

    def _extract_extended_context(
        self, lines: List[str], line_number: int, context_lines: int = 5
    ) -> Tuple[List[str], List[str]]:
        """
        Extract extended context before and after a line.

        Args:
            lines: Source code lines
            line_number: Line number (1-indexed)
            context_lines: Number of lines to extract

        Returns:
            Tuple of (context_before, context_after) as lists of lines
        """
        start_before = max(0, line_number - context_lines - 1)
        end_after = min(len(lines), line_number + context_lines)

        context_before = []
        for i in range(start_before, line_number - 1):
            context_before.append(lines[i].rstrip())

        context_after = []
        for i in range(line_number, end_after):
            context_after.append(lines[i].rstrip())

        return context_before, context_after

    def _is_in_init_section(
        self, enclosing_function: str, lines: List[str], line_number: int
    ) -> bool:
        """
        Detect if a call is in an initialization section.

        Checks:
        - Function name contains 'init', 'setup', 'config'
        - Position in main() early in the file
        - Before main loop markers (while(1), for(;;))

        Args:
            enclosing_function: Name of containing function
            lines: Source code lines
            line_number: Line number of the call

        Returns:
            True if likely in initialization code
        """
        # Check function name
        init_keywords = ['init', 'setup', 'config', 'open', 'start']
        func_lower = enclosing_function.lower()
        if any(kw in func_lower for kw in init_keywords):
            return True

        # Check if in main() and early in file
        if enclosing_function == 'main':
            # Look for main loop markers after this line
            for i in range(line_number - 1, min(line_number + 20, len(lines))):
                line = lines[i].strip()
                if 'while(1)' in line or 'while (1)' in line or 'for(;;)' in line:
                    return True  # Before the main loop
                if 'ble_' in line.lower() and 'main' in line.lower():
                    return False  # In BLE main loop

        return False

    def _calculate_call_depth(self, call_node) -> int:
        """
        Calculate the nesting depth of a function call.

        Args:
            call_node: Tree-sitter call_expression node

        Returns:
            Nesting depth (0 = top level in function)
        """
        depth = 0
        node = call_node.parent

        while node:
            if node.type == 'call_expression':
                depth += 1
            elif node.type == 'function_definition':
                break
            node = node.parent

        return depth

    def _classify_api_call(self, function_name: str) -> Tuple[str, str]:
        """Classify an API call by module and peripheral."""
        module = "unknown"
        peripheral = ""

        # Driver APIs usually start with peripheral name
        driver_peripherals = [
            'GPIO', 'UART', 'SPI', 'I2C', 'ADC', 'DAC', 'TIMER', 'PWM',
            'DMA', 'EXTI', 'RTC', 'WDT', 'FLASH', 'USB', 'IWDG', 'LDO'
        ]

        for p in driver_peripherals:
            if function_name.startswith(p):
                module = 'driver'
                peripheral = p
                break

        # BLE APIs
        if function_name.startswith(('ble_', 'BLE', 'ke_', 'KE')):
            module = 'ble'
            peripheral = 'BLE'

        # Common SDK functions
        if function_name.startswith(('B6x_', 'b6x_', 'cpu')):
            module = 'driver'

        return module, peripheral

    def _extract_function_names(self, root_node) -> List[str]:
        """Extract all function names defined in the file."""
        names = []

        def find_functions(node):
            if node.type == 'function_definition':
                for child in node.children:
                    if child.type == 'function_declarator':
                        for subchild in child.children:
                            if subchild.type == 'identifier':
                                names.append(subchild.text.decode('utf-8'))
                                break

            for child in node.children:
                find_functions(child)

        find_functions(root_node)
        return names

    def _extract_included_headers(self, root_node) -> List[str]:
        """Extract all included header names."""
        headers = []

        def find_includes(node):
            if node.type == 'preproc_include':
                for child in node.children:
                    if child.type in ('string_literal', 'system_lib_string'):
                        header = child.text.decode('utf-8').strip('"<>')
                        headers.append(header)

            for child in node.children:
                find_includes(child)

        find_includes(root_node)
        return headers

    def extract_dependencies(self, api_name: str) -> Optional[Dict[str, Any]]:
        """
        Extract dependency information for a given API function.

        This method analyzes function calls, parameter types, and comments
        to infer hardware/software dependencies.

        Args:
            api_name: Name of the API function to analyze

        Returns:
            Dictionary with dependency information:
            {
                "hardware": {
                    "peripherals": [...],
                    "clocks": [...],
                    "gpio": [...],
                    "dma": [...],
                    "interrupts": [...]
                },
                "software": {
                    "required_apis": [...],
                    "optional_apis": [...],
                    "config_structures": [...]
                },
                "resources": {
                    "memory": {...},
                    "timers": [...]
                }
            }
        """
        # Initialize result structure
        result = {
            "hardware": {
                "peripherals": [],
                "clocks": [],
                "gpio": [],
                "dma": [],
                "interrupts": []
            },
            "software": {
                "required_apis": [],
                "optional_apis": [],
                "config_structures": []
            },
            "resources": {
                "memory": {},
                "timers": []
            }
        }

        # Infer dependencies from API name patterns
        api_upper = api_name.upper()

        # Hardware peripheral detection
        peripheral_patterns = {
            "UART": ["UART", "USART"],
            "SPI": ["SPI"],
            "I2C": ["I2C"],
            "GPIO": ["GPIO"],
            "TIMER": ["TIM", "TIMER"],
            "ADC": ["ADC"],
            "DAC": ["DAC"],
            "DMA": ["DMA"],
            "RTC": ["RTC"],
            "WDT": ["WDT", "WATCHDOG"],
            "PWM": ["PWM"],
            "FLASH": ["FLASH"],
            "RCC": ["RCC", "CLOCK"]
        }

        for periph_type, patterns in peripheral_patterns.items():
            for pattern in patterns:
                if pattern in api_upper:
                    result["hardware"]["peripherals"].append(periph_type)
                    if periph_type not in ["GPIO", "RCC"]:
                        result["hardware"]["clocks"].append(f"{periph_type}_CLK")
                    break

        # Clock requirement inference
        if any(p in api_upper for p in ["Init", "DeInit", "Config", "Enable", "Disable"]):
            if result["hardware"]["peripherals"]:
                result["hardware"]["clocks"].append("Peripheral_Clock")

        # GPIO requirement inference
        if "GPIO" in api_upper or any(
            pin in api_upper for pin in ["PIN", "PAD", "IO"]
        ):
            result["hardware"]["gpio"].append("GPIO_Config_Required")

        # DMA requirement inference
        if "DMA" in api_upper or "TRANSFER" in api_upper:
            result["hardware"]["dma"].append("DMA_Channel_Required")

        # Interrupt requirement inference
        if any(int_word in api_upper for int_word in ["INT", "IRQ", "INTERRUPT", "NVIC"]):
            result["hardware"]["interrupts"].append("Interrupt_Config_Required")

        # Software dependencies based on common patterns
        if "Init" in api_name:
            result["software"]["required_apis"].append("Clock_Enable")
            result["software"]["config_structures"].append(f"{api_name.replace('Init', '')}_Config")

        # Resource requirements
        if "BUFFER" in api_upper or "DMA" in api_upper:
            result["resources"]["memory"]["buffer_required"] = True

        if "TIMER" in api_upper or "TIM" in api_upper:
            result["resources"]["timers"].append("Timer_Channel")

        # Deduplicate lists
        for category in ["hardware", "software"]:
            for key in result[category]:
                if isinstance(result[category][key], list):
                    result[category][key] = list(set(result[category][key]))

        return result

    def find_function_definition(self, function_name: str, search_paths: List[str] = None) -> Optional[Dict]:
        """
        Find a function definition by searching SDK source files.

        This method searches through C source files to find a specific
        function definition by name.

        Args:
            function_name: Function name to find (e.g., "B6x_UART_Init")
            search_paths: Optional list of directories to search. If not provided,
                          defaults to common SDK source directories.

        Returns:
            Dictionary with function information:
            {
                "file": str,          # Source file path
                "line": int,          # Line number
                "code": str,          # Function body
                "complexity": str     # Complexity indicator
            }
            or None if not found
        """
        if not self._initialized:
            if not self.initialize():
                logger.warning("Tree-sitter parser not initialized")
                return None

        # Default search paths - try to locate SDK source directories
        if search_paths is None:
            try:
                base_path = Path(get_sdk_root())
            except RuntimeError:
                base_path = Path(__file__).parent.parent.parent
            search_paths = [
                str(base_path / "drivers" / "src"),
                str(base_path / "ble" / "app"),
                str(base_path / "examples"),
                str(base_path / "projects"),
            ]

        for search_path in search_paths:
            path = Path(search_path)
            if not path.exists():
                continue

            for c_file in path.rglob("*.c"):
                try:
                    result = self.parse_source_file(str(c_file))
                    if result and result.functions:
                        for func in result.functions:
                            if func.name == function_name:
                                return {
                                    "file": str(c_file),
                                    "line": func.line_number,
                                    "code": func.function_body or "",
                                    "complexity": getattr(func, 'complexity', 'unknown'),
                                }
                except Exception as e:
                    logger.debug(f"Error parsing {c_file}: {e}")
                    continue

        logger.debug(f"Function definition not found: {function_name}")
        return None


# Convenience functions

def parse_header_file(file_path: str) -> Optional[HeaderParseResult]:
    """
    Convenience function to parse a single header file.

    Args:
        file_path: Path to the header file

    Returns:
        HeaderParseResult or None
    """
    parser = TreeSitterCParser()
    return parser.parse_header_file(file_path)


def parse_directory(directory: str, pattern: str = "*.h") -> List[HeaderParseResult]:
    """
    Parse all header files in a directory.

    Args:
        directory: Directory path
        pattern: File pattern (default: "*.h")

    Returns:
        List of HeaderParseResult objects
    """
    parser = TreeSitterCParser()
    if not parser.initialize():
        return []

    results = []
    dir_path = Path(directory)

    for header_file in dir_path.rglob(pattern):
        result = parser.parse_header_file(str(header_file))
        if result:
            results.append(result)

    logger.info(f"Parsed {len(results)} header files from {directory}")
    return results


def parse_directory_full(
    directory: str,
    pattern: str = "*.h",
    use_cache: bool = True,
    cache_dir: str = "data/parse_cache"
) -> List[FullParseResult]:
    """
    Parse all header files in a directory with caching support.

    Args:
        directory: Directory path to parse
        pattern: File pattern (default: "*.h")
        use_cache: Enable/disable cache (default: True)
        cache_dir: Cache directory (default: "data/parse_cache")

    Returns:
        List of FullParseResult objects
    """
    parser = TreeSitterCParser()
    if not parser.initialize():
        return []

    results = []
    dir_path = Path(directory)
    cache_path = Path(cache_dir)

    # Get SDK root for relative path conversion
    try:
        sdk_root = get_sdk_root()
    except RuntimeError:
        sdk_root = None

    # Determine cache subdirectory from directory name
    # "drivers/api" -> "drivers"
    # "ble/api" -> "ble"
    cache_subdir = dir_path.parts[-2] if len(dir_path.parts) >= 2 else dir_path.name
    cache_domain_path = cache_path / cache_subdir
    cache_domain_path.mkdir(parents=True, exist_ok=True)

    for header_file in dir_path.rglob(pattern):
        cache_file = cache_domain_path / f"{header_file.name}.json"

        result = None
        if use_cache and cache_file.exists():
            # Load from cache
            try:
                with open(cache_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                result = deserialize_full_parse_result(data)
                logger.debug(f"Loaded from cache: {header_file.name}")
            except Exception as e:
                logger.warning(f"Cache load failed for {header_file.name}: {e}, re-parsing...")
                result = None

        if result is None:
            # Parse with Tree-sitter
            result = parser.parse_header_file_full(str(header_file))
            if result and use_cache:
                # Convert file paths to relative before caching
                if sdk_root and result.file_path:
                    result.file_path = make_relative(result.file_path, sdk_root)
                    # Also convert paths in functions, macros, enums
                    for func in result.functions:
                        if func.file_path:
                            func.file_path = make_relative(func.file_path, sdk_root)
                    for macro in result.macros:
                        if macro.file_path:
                            macro.file_path = make_relative(macro.file_path, sdk_root)
                    for enum in result.enums:
                        if enum.file_path:
                            enum.file_path = make_relative(enum.file_path, sdk_root)

                # Save to cache
                try:
                    data = serialize_full_parse_result(result)
                    with open(cache_file, 'w', encoding='utf-8') as f:
                        json.dump(data, f, indent=2, ensure_ascii=False)
                    logger.debug(f"Cached: {header_file.name}")
                except Exception as e:
                    logger.warning(f"Cache save failed for {header_file.name}: {e}")

        if result:
            results.append(result)

    logger.info(f"Parsed {len(results)} header files from {directory} (cache={use_cache})")
    return results


def parse_directory_source(
    directory: str,
    pattern: str = "*.c",
    use_cache: bool = True,
    cache_dir: str = "data/parse_cache"
) -> List[SourceParseResult]:
    """
    Parse all source files in a directory with caching support.

    Args:
        directory: Directory path to parse
        pattern: File pattern (default: "*.c")
        use_cache: Enable/disable cache (default: True)
        cache_dir: Cache directory (default: "data/parse_cache")

    Returns:
        List of SourceParseResult objects
    """
    parser = TreeSitterCParser()
    if not parser.initialize():
        return []

    results = []
    dir_path = Path(directory)
    cache_path = Path(cache_dir)

    # Get SDK root for relative path conversion
    try:
        sdk_root = get_sdk_root()
    except RuntimeError:
        sdk_root = None

    # Determine cache subdirectory from directory name
    # "drivers/src" -> "drivers"
    # "ble/app" -> "ble"
    cache_subdir = dir_path.parts[-2] if len(dir_path.parts) >= 2 else dir_path.name
    cache_domain_path = cache_path / cache_subdir
    cache_domain_path.mkdir(parents=True, exist_ok=True)

    for source_file in dir_path.rglob(pattern):
        cache_file = cache_domain_path / f"{source_file.name}.json"

        result = None
        if use_cache and cache_file.exists():
            # Load from cache
            try:
                with open(cache_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                result = deserialize_source_parse_result(data)
                logger.debug(f"Loaded from cache: {source_file.name}")
            except Exception as e:
                logger.warning(f"Cache load failed for {source_file.name}: {e}, re-parsing...")
                result = None

        if result is None:
            # Parse with Tree-sitter
            result = parser.parse_source_file(str(source_file))
            if result and use_cache:
                # Convert file paths to relative before caching
                if sdk_root and result.file_path:
                    result.file_path = make_relative(result.file_path, sdk_root)
                    # Also convert paths in function definitions
                    for func_def in result.functions:
                        if func_def.file_path:
                            func_def.file_path = make_relative(func_def.file_path, sdk_root)

                # Save to cache
                try:
                    data = serialize_source_parse_result(result)
                    with open(cache_file, 'w', encoding='utf-8') as f:
                        json.dump(data, f, indent=2, ensure_ascii=False)
                    logger.debug(f"Cached: {source_file.name}")
                except Exception as e:
                    logger.warning(f"Cache save failed for {source_file.name}: {e}")

        if result:
            results.append(result)

    logger.info(f"Parsed {len(results)} source files from {directory} (cache={use_cache})")
    return results


if __name__ == "__main__":
    # Test the parser
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) > 1:
        # Parse specific file
        result = parse_header_file(sys.argv[1])
        if result:
            print(f"\nFile: {result.file_path}")
            print(f"Functions found: {len(result.functions)}")
            for func in result.functions[:5]:  # Show first 5
                print(f"\n  {func.return_type} {func.name}({', '.join([p.type + ' ' + p.name for p in func.parameters])})")
                print(f"    Brief: {func.brief_description}")
    else:
        print("Usage: python tree_sitter_parser.py <header_file>")
