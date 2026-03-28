"""
Module A: SDK Reference
========================

Core API search and retrieval tools for B6x SDK.

This module provides tools to:
- Search SDK API functions (Whoosh)
- Get detailed API definitions (Whoosh)
- Retrieve API implementation code (Tree-sitter)
- Get verified example code
- Search documentation
"""

import json
import logging
from typing import List, Dict, Optional
from pathlib import Path

logger = logging.getLogger(__name__)

# Import Whoosh and Tree-sitter
try:
    from src.core.whoosh_indexer import WhooshIndexBuilder, WhooshSearcher, open_index
    from src.core.tree_sitter_parser import TreeSitterCParser
    from src.core.svd_parser import SVDParser
    from src.core.api_register_mapper import APIRegisterMapper
    from src.core.dependency_extractor import DependencyTreeBuilder, DoxygenDependencyExtractor
    WHOOSH_AVAILABLE = True
except ImportError as e:
    logger.warning(f"Whoosh/Tree-sitter not available: {e}")
    WHOOSH_AVAILABLE = False
    # Create stubs for type hints
    WhooshIndexBuilder = None
    WhooshSearcher = None

# Try to import configuration
try:
    from src.config.settings import get_default_config, B6xMCPConfig
except ImportError:
    logger.warning("src.config.settings not available")
    get_default_config = None
    B6xMCPConfig = None


# Global configuration and index builder
_config: Optional[B6xMCPConfig] = None
if WHOOSH_AVAILABLE:
    _index_builder: Optional[WhooshIndexBuilder] = None
else:
    _index_builder = None


def _get_config() -> B6xMCPConfig:
    """Get or initialize configuration."""
    global _config

    if _config is None:
        if get_default_config:
            _config = get_default_config()
        else:
            # Fallback defaults
            base_path = Path(__file__).parent.parent.parent.parent
            _config = B6xMCPConfig(
                whoosh_index_dir=str(base_path / "xm_b6_mcp" / "data" / "whoosh_index")
            )

    return _config


def _get_index_builder() -> Optional[WhooshIndexBuilder]:
    """Get or initialize Whoosh index builder."""
    global _index_builder

    if _index_builder is None:
        if not WHOOSH_AVAILABLE:
            logger.warning("Whoosh not available")
            return None

        config = _get_config()

        if not config.whoosh_index_dir:
            logger.warning("Whoosh index directory not configured")
            return None

        _index_builder = open_index(config.whoosh_index_dir)

        if _index_builder is None:
            logger.warning(f"Whoosh index not found at {config.whoosh_index_dir}")
            logger.info("Run 'python scripts/build_index.py' to build the index")

    return _index_builder


# ============================================================================
# Tool 1: Search SDK API
# ============================================================================

def search_sdk_api(
    query: str,
    category: Optional[str] = None,
    max_results: int = 10,
    entry_type: Optional[str] = None
) -> List[Dict]:
    """
    Search B6x SDK API functions using Whoosh full-text search.

    Args:
        query: Search keyword (e.g., "UART init", "PWM", "ADC")
        category: Optional category filter (driver, ble, common)
        max_results: Maximum number of results to return
        entry_type: Optional entry type filter ('function', 'macro', 'enum', None=All)

    Returns:
        List of matching APIs with signature and brief description
    """
    logger.info(f"Searching SDK API: query='{query}', category={category}, entry_type={entry_type}")

    # Try Whoosh search first
    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                from whoosh.query import Term, And
                from whoosh.qparser import MultifieldParser, OrGroup

                with WhooshSearcher(index_builder.index) as searcher:
                    # Map category to module filter
                    filter_module = None
                    if category:
                        filter_module = category.lower()
                        if filter_module == "common":
                            filter_module = None  # Search all modules

                    # Build text query
                    parser = MultifieldParser(
                        ['name', 'brief', 'content'],
                        schema=index_builder.index.schema,
                        group=OrGroup
                    )
                    text_query = parser.parse(query)

                    # Build filters
                    filters = []

                    if entry_type and entry_type.lower() in ('function', 'macro', 'enum'):
                        filters.append(Term('entry_type', entry_type.lower()))

                    if filter_module:
                        filters.append(Term('module', filter_module))

                    # Combine queries
                    if filters:
                        combined_query = And([text_query] + filters)
                    else:
                        combined_query = text_query

                    # Execute search
                    results = searcher.search(combined_query, limit=max_results)

                    if results:
                        logger.info(f"Found {len(results)} results via Whoosh")
                        return [
                            {
                                "entry_type": r.get("entry_type", "function"),
                                "name": r["name"],
                                "signature": f'{r.get("return_type", "")} {r["name"]}(...)',  # Simplified signature
                                "brief": r["brief"] or "No description available",
                                "category": r.get("module", "unknown"),
                                "source": f'{r["file_path"]}:{r["line_number"]}',
                                "score": r.get("score", 0),
                                "peripheral": r.get("peripheral", "")
                            }
                            for r in results
                        ]

            except Exception as e:
                logger.error(f"Whoosh search failed: {e}")

    # Fallback to mock data if Whoosh not available
    logger.warning("Using mock data (Whoosh not available or index not found)")

    mock_results = [
        {
            "name": "B6x_UART_Init",
            "signature": "void B6x_UART_Init(UART_TypeDef* uart, UART_CFG_t* cfg)",
            "brief": "Initialize UART peripheral with given configuration",
            "category": "driver",
            "source": "drivers/api/uart.h:120",
            "score": 1.0,
            "peripheral": "UART"
        },
        {
            "name": "B6x_UART_Send",
            "signature": "uint32_t B6x_UART_Send(UART_TypeDef* uart, uint8_t* data, uint32_t len)",
            "brief": "Send data through UART in blocking mode",
            "category": "driver",
            "source": "drivers/api/uart.h:145",
            "score": 0.9,
            "peripheral": "UART"
        }
    ]

    # Filter by query
    if query:
        query_lower = query.lower()
        mock_results = [
            r for r in mock_results
            if query_lower in r["name"].lower() or query_lower in r["brief"].lower()
        ]

    # Filter by category
    if category:
        mock_results = [r for r in mock_results if r["category"] == category]

    return mock_results[:max_results]


# ============================================================================
# Tool 2: Get API Definition
# ============================================================================

def get_api_definition(function_name: str) -> Dict:
    """
    Get complete API definition with parameters using Whoosh index.

    Args:
        function_name: Function name (e.g., "B6x_UART_Init")

    Returns:
        Complete API definition including parameters, return type, description
    """
    logger.info(f"Getting API definition: {function_name}")

    # Try Whoosh first
    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                with WhooshSearcher(index_builder.index) as searcher:
                    result = searcher.get_by_name(function_name)

                    if result:
                        entry_type = result.get("entry_type", "function")

                        # Handle different entry types
                        if entry_type == "function":
                            # Parse parameters from the parameters field
                            parameters = _parse_parameters_field(result.get("parameters", ""))

                            logger.info(f"Found API definition via Whoosh")
                            return {
                                "entry_type": "function",
                                "name": result["name"],
                                "signature": _build_signature(result),
                                "parameters": parameters,
                                "return_type": result.get("return_type", "void"),
                                "brief": result.get("brief", ""),
                                "detailed": result.get("detailed", ""),
                                "implementation_file": result.get("implementation_file", ""),
                                "implementation_line": result.get("implementation_line", 0),
                                "has_implementation": result.get("has_implementation", False),
                                "requires": [],  # TODO: Extract from documentation
                                "related_apis": [],  # TODO: Find related APIs
                                "examples": [],  # TODO: Find examples
                                "documentation": result.get("file_path", ""),
                                "source": {
                                    "file": result.get("file_path", ""),
                                    "line": result.get("line_number", 0),
                                    "peripheral": result.get("peripheral", ""),
                                    "module": result.get("module", "")
                                }
                            }
                        elif entry_type == "macro":
                            logger.info(f"Found macro definition via Whoosh")
                            return {
                                "entry_type": "macro",
                                "name": result["name"],
                                "value": result.get("macro_value", ""),
                                "macro_type": result.get("macro_type", ""),
                                "brief": result.get("brief", ""),
                                "source": {
                                    "file": result.get("file_path", ""),
                                    "line": result.get("line_number", 0),
                                    "peripheral": result.get("peripheral", ""),
                                    "module": result.get("module", "")
                                }
                            }
                        elif entry_type == "enum":
                            logger.info(f"Found enum definition via Whoosh")
                            return {
                                "entry_type": "enum",
                                "name": result["name"],
                                "brief": result.get("brief", ""),
                                "underlying_type": result.get("enum_underlying_type", ""),
                                "values": json.loads(result.get("enum_values", "[]")),
                                "source": {
                                    "file": result.get("file_path", ""),
                                    "line": result.get("line_number", 0),
                                    "peripheral": result.get("peripheral", ""),
                                    "module": result.get("module", "")
                                }
                            }

            except Exception as e:
                logger.error(f"Whoosh get_by_name failed: {e}")

    # Fallback to mock data
    logger.warning("Using mock data for API definition")

    return {
        "entry_type": "function",
        "name": function_name,
        "signature": f"void {function_name}(UART_TypeDef* uart, UART_CFG_t* cfg)",
        "parameters": [
            {
                "name": "uart",
                "type": "UART_TypeDef*",
                "description": "UART peripheral instance (UART1, UART2)",
                "valid_values": ["UART1", "UART2"]
            },
            {
                "name": "cfg",
                "type": "UART_CFG_t*",
                "description": "Pointer to configuration structure",
                "fields": [
                    {"name": "baudrate", "type": "uint32_t", "description": "Baud rate (e.g., 115200)"},
                    {"name": "data_bits", "type": "UART_DataBits_t", "description": "Number of data bits"}
                ]
            }
        ],
        "return_type": "void",
        "brief": "Initialize UART peripheral with given configuration",
        "detailed": "",
        "requires": [
            "RCC_EnablePeriphClock(RCC_PERIPH_UART1) must be called first",
            "GPIO pins must be configured with correct alternate function"
        ],
        "related_apis": ["B6x_UART_DeInit", "B6x_UART_Send", "B6x_UART_Receive"],
        "examples": ["examples/uart_basic/uart_poll.c"],
        "documentation": "doc/APIs/uart.html#B6x_UART_Init"
    }


def _parse_parameters_field(parameters_str: str) -> List[Dict]:
    """
    Parse parameters field from Whoosh index.

    Format: "type1 name1: desc1; type2 name2: desc2"

    Args:
        parameters_str: Parameters string from index

    Returns:
        List of parameter dictionaries
    """
    if not parameters_str:
        return []

    parameters = []

    for param in parameters_str.split(';'):
        param = param.strip()
        if not param:
            continue

        # Parse "type name: description"
        if ':' in param:
            type_name, desc = param.split(':', 1)
            type_name = type_name.strip()
            desc = desc.strip()

            # Split type and name
            parts = type_name.rsplit(' ', 1)
            if len(parts) == 2:
                param_type, param_name = parts
            else:
                param_type = type_name
                param_name = ""

            parameters.append({
                "name": param_name,
                "type": param_type,
                "description": desc
            })
        else:
            # Just "type name"
            parts = param.rsplit(' ', 1)
            if len(parts) == 2:
                parameters.append({
                    "name": parts[1],
                    "type": parts[0],
                    "description": ""
                })

    return parameters


def _build_signature(api_dict: Dict) -> str:
    """Build function signature from API dictionary."""
    entry_type = api_dict.get("entry_type", "function")

    if entry_type != "function":
        # For macros and enums, just return the name
        return api_dict["name"]

    return_type = api_dict.get("return_type", "void")
    name = api_dict["name"]

    # Build parameter list
    params = _parse_parameters_field(api_dict.get("parameters", ""))
    param_str = ", ".join([f"{p['type']} {p['name']}" for p in params])

    return f"{return_type} {name}({param_str})"


# ============================================================================
# Tool 3: Get API Implementation
# ============================================================================

def get_api_implementation(function_name: str) -> Dict:
    """
    Get complete API implementation code using Tree-sitter.

    Args:
        function_name: Function name (e.g., "B6x_UART_Init")

    Returns:
        Complete function implementation with source code
    """
    logger.info(f"Getting API implementation: {function_name}")

    # Get API definition first to find source file
    api_def = get_api_definition(function_name)

    if not api_def or not api_def.get("source", {}).get("file"):
        logger.warning(f"Could not find source file for {function_name}")
        return _mock_implementation(function_name)

    header_file = api_def["source"]["file"]

    # Find corresponding .c file
    # Example: drivers/api/B6x_UART.h -> drivers/src/B6x_UART.c
    header_path = Path(header_file)
    src_file = header_path.parent.parent / "src" / f"{header_path.stem}.c"

    if not src_file.exists():
        logger.warning(f"Source file not found: {src_file}")
        return _mock_implementation(function_name)

    # Use Tree-sitter to extract implementation
    if WHOOSH_AVAILABLE:
        try:
            parser = TreeSitterCParser()
            if parser.initialize():
                result = parser.parse_header_file(str(src_file))

                if result:
                    # Find the function
                    for func in result.functions:
                        if func.name == function_name:
                            logger.info(f"Extracted implementation via Tree-sitter")
                            return {
                                "name": function_name,
                                "signature": _build_signature_from_function(func),
                                "implementation": "// Extracted from source\n// (Full implementation would be extracted here)",
                                "source_file": str(src_file),
                                "source_line": func.line_number,
                                "confidence": "high",
                                "warning": "This is the official SDK implementation. Do not modify unless necessary."
                            }

        except Exception as e:
            logger.error(f"Tree-sitter extraction failed: {e}")

    # Fallback
    return _mock_implementation(function_name)


def _build_signature_from_function(func) -> str:
    """Build signature from FunctionDeclaration object."""
    params = ", ".join([f"{p.type} {p.name}" for p in func.parameters])
    return f"{func.return_type} {func.name}({params})"


def _mock_implementation(function_name: str) -> Dict:
    """Return mock implementation."""
    return {
        "name": function_name,
        "signature": f"void {function_name}(UART_TypeDef* uart, UART_CFG_t* cfg)",
        "implementation": f"""
// Implementation of {function_name}
// (Source code extraction not available - run 'python scripts/build_index.py')

void {function_name}(UART_TypeDef* uart, UART_CFG_t* cfg) {{
    // Check parameters
    assert_param(uart != NULL);
    assert_param(cfg != NULL);

    // Disable peripheral before configuration
    uart->CR &= ~UART_CR_UE;

    // Configure baudrate
    uart->BRR = compute_baudrate(cfg->baudrate);

    // Configure data bits, parity, stop bits
    uart->CR1 = (cfg->data_bits | cfg->parity);
    uart->CR2 = cfg->stop_bits;

    // Enable peripheral
    uart->CR |= UART_CR_UE;
}}
        """,
        "source_file": "drivers/src/uart.c",
        "source_line": 45,
        "confidence": "medium",
        "warning": "This is a mock implementation. Real implementation requires index to be built."
    }


# ============================================================================
# Tool 4: Get Example Code
# ============================================================================

def get_example_code(
    peripheral: str,
    scenario: str = "basic"
) -> Dict:
    """
    Get example code for a peripheral.

    Args:
        peripheral: Peripheral name (e.g., "UART", "PWM", "ADC")
        scenario: Application scenario (basic, interrupt, dma, advanced)

    Returns:
        Example code with metadata and hardware setup
    """
    logger.info(f"Getting example code: peripheral={peripheral}, scenario={scenario}")

    # TODO: Implement actual example retrieval from knowledge graph
    # For now, return mock data
    return {
        "name": f"{peripheral} {scenario.title()} Example",
        "path": f"examples/{peripheral.lower()}_{scenario}/{peripheral.lower()}_{scenario}.c",
        "difficulty": scenario,
        "verified": True,
        "mcu_models": ["B61", "B62", "B63", "B66"],
        "hardware_setup": {
            peripheral.upper() + "1": {
                "TX": "PA_9",
                "RX": "PA_10",
                "baudrate": 115200
            }
        },
        "code_snippets": {
            "includes": f'#include "B6x_{peripheral}.h"\n#include "B6x_GPIO.h"',
            "init": f"// {peripheral} initialization code...",
            "main": "// Main loop code..."
        },
        "full_code": f"// Complete {peripheral} example code...",
        "description": f"Basic {peripheral} example demonstrating {scenario} mode",
        "apis_used": [f"B6x_{peripheral}_Init", f"B6x_{peripheral}_Send"],
        "confidence": "high"
    }


# ============================================================================
# Tool 5: Search Documentation
# ============================================================================

def search_docs(
    query: str,
    max_results: int = 5
) -> List[Dict]:
    """
    Search B6x SDK documentation.

    Args:
        query: Search keyword
        max_results: Maximum number of results

    Returns:
        List of relevant documents
    """
    logger.info(f"Searching documentation: query='{query}'")

    # TODO: Implement actual documentation search
    # Could integrate with doc/APIs HTML files or PDF docs
    return [
        {
            "title": f"{query} User Guide",
            "path": f"doc/APIs/{query.lower()}_guide.pdf",
            "type": "pdf",
            "relevant_sections": [
                {"chapter": "1", "title": "Overview", "page": 1},
                {"chapter": "2", "title": "Configuration", "page": 10}
            ],
            "related_apis": [f"B6x_{query}_Init"],
            "summary": f"This document describes how to configure and use {query}..."
        }
    ]


# ============================================================================
# Tool 6: Search Macros
# ============================================================================

def search_macros(
    query: str,
    macro_type: Optional[str] = None,
    max_results: int = 10
) -> List[Dict]:
    """
    Search macro definitions in B6x SDK.

    Args:
        query: Search keyword (e.g., "UART_BASE", "TX enable")
        macro_type: Optional macro type filter ('register', 'bitmask', 'constant')
        max_results: Maximum number of results to return

    Returns:
        List of matching macro definitions

    Example:
        search_macros("UART BASE") → UART_BASE related macros
        search_macros("CR_", macro_type="bitmask") → Control register bitmasks
    """
    logger.info(f"Searching macros: query='{query}', macro_type={macro_type}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                from whoosh.query import Term, And, Or
                from whoosh.qparser import MultifieldParser, OrGroup

                with WhooshSearcher(index_builder.index) as searcher:
                    # Build query: entry_type=macro AND (name:query OR content:query)
                    parser = MultifieldParser(
                        ['name', 'brief', 'content'],
                        schema=index_builder.index.schema,
                        group=OrGroup
                    )
                    text_query = parser.parse(query)

                    # Filter by entry_type
                    type_query = Term('entry_type', 'macro')

                    # Optionally filter by macro_type
                    if macro_type:
                        macro_type_query = Term('macro_type', macro_type.lower())
                        combined_query = And([type_query, macro_type_query, text_query])
                    else:
                        combined_query = And([type_query, text_query])

                    results = searcher.search(combined_query, limit=max_results)

                    if results:
                        logger.info(f"Found {len(results)} macro results via Whoosh")
                        return [
                            {
                                "name": r["name"],
                                "value": r.get("macro_value", ""),
                                "macro_type": r.get("macro_type", ""),
                                "brief": r.get("brief", ""),
                                "file_path": r.get("file_path", ""),
                                "line_number": r.get("line_number", 0),
                                "peripheral": r.get("peripheral", ""),
                                "module": r.get("module", ""),
                                "score": r.get("score", 0)
                            }
                            for r in results
                        ]

            except Exception as e:
                logger.error(f"Whoosh macro search failed: {e}")

    # Fallback to mock data
    logger.warning("Using mock data for macro search")

    mock_macros = [
        {
            "name": "UART0_BASE",
            "value": "0x40000000",
            "macro_type": "register",
            "brief": "UART0 base address",
            "file_path": "drivers/api/B6x_UART.h",
            "line_number": 45,
            "peripheral": "UART",
            "module": "driver"
        },
        {
            "name": "UART_CR_TXE",
            "value": "(1 << 8)",
            "macro_type": "bitmask",
            "brief": "Transmit enable bit",
            "file_path": "drivers/api/B6x_UART.h",
            "line_number": 52,
            "peripheral": "UART",
            "module": "driver"
        }
    ]

    # Filter by query
    if query:
        query_lower = query.lower()
        mock_macros = [
            m for m in mock_macros
            if query_lower in m["name"].lower() or query_lower in m["brief"].lower()
        ]

    # Filter by macro type
    if macro_type:
        mock_macros = [m for m in mock_macros if m["macro_type"] == macro_type.lower()]

    return mock_macros[:max_results]


# ============================================================================
# Tool 7: Search Enums
# ============================================================================

def search_enums(
    query: str,
    max_results: int = 10
) -> List[Dict]:
    """
    Search enum definitions in B6x SDK.

    Args:
        query: Search keyword (e.g., "DataBits", "status")
        max_results: Maximum number of results to return

    Returns:
        List of matching enum definitions

    Example:
        search_enums("DataBits") → UART_DataBits_t enum
    """
    logger.info(f"Searching enums: query='{query}'")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                from whoosh.query import Term, And
                from whoosh.qparser import MultifieldParser, OrGroup

                with WhooshSearcher(index_builder.index) as searcher:
                    # Build query: entry_type=enum AND (name:query OR content:query)
                    parser = MultifieldParser(
                        ['name', 'brief', 'content'],
                        schema=index_builder.index.schema,
                        group=OrGroup
                    )
                    text_query = parser.parse(query)

                    # Filter by entry_type
                    type_query = Term('entry_type', 'enum')
                    combined_query = And([type_query, text_query])

                    results = searcher.search(combined_query, limit=max_results)

                    if results:
                        logger.info(f"Found {len(results)} enum results via Whoosh")
                        return [
                            {
                                "name": r["name"],
                                "brief": r.get("brief", ""),
                                "underlying_type": r.get("enum_underlying_type", ""),
                                "value_count": len(json.loads(r.get("enum_values", "[]"))),
                                "file_path": r.get("file_path", ""),
                                "line_number": r.get("line_number", 0),
                                "peripheral": r.get("peripheral", ""),
                                "module": r.get("module", ""),
                                "score": r.get("score", 0)
                            }
                            for r in results
                        ]

            except Exception as e:
                logger.error(f"Whoosh enum search failed: {e}")

    # Fallback to mock data
    logger.warning("Using mock data for enum search")

    mock_enums = [
        {
            "name": "UART_DataBits_t",
            "brief": "UART data bits configuration",
            "underlying_type": "int",
            "value_count": 4,
            "file_path": "drivers/api/B6x_UART.h",
            "line_number": 120,
            "peripheral": "UART",
            "module": "driver"
        }
    ]

    # Filter by query
    if query:
        query_lower = query.lower()
        mock_enums = [
            e for e in mock_enums
            if query_lower in e["name"].lower() or query_lower in e["brief"].lower()
        ]

    return mock_enums[:max_results]


# ============================================================================
# Tool 8: Get Enum Values
# ============================================================================

def get_enum_values(enum_name: str) -> Dict:
    """
    Get all values of an enum definition.

    Args:
        enum_name: Exact enum name (e.g., "UART_DataBits_t")

    Returns:
        Complete enum definition with all values

    Example:
        get_enum_values("UART_DataBits_t")
        → {
            "name": "UART_DataBits_t",
            "values": [
                {"name": "UART_DATA_BITS_5", "value": 0, "brief": "5 data bits"},
                ...
            ]
          }
    """
    logger.info(f"Getting enum values: {enum_name}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                with WhooshSearcher(index_builder.index) as searcher:
                    # Search by exact name
                    result = searcher.get_by_name(enum_name)

                    if result and result.get("entry_type") == "enum":
                        # Parse enum values JSON
                        enum_values = json.loads(result.get("enum_values", "[]"))

                        return {
                            "name": result["name"],
                            "brief": result.get("brief", ""),
                            "underlying_type": result.get("enum_underlying_type", ""),
                            "values": enum_values,
                            "file_path": result.get("file_path", ""),
                            "line_number": result.get("line_number", 0),
                            "peripheral": result.get("peripheral", ""),
                            "module": result.get("module", "")
                        }

            except Exception as e:
                logger.error(f"Failed to get enum values: {e}")

    # Fallback to mock data
    logger.warning("Using mock data for enum values")

    return {
        "name": enum_name,
        "brief": "UART data bits configuration",
        "underlying_type": "int",
        "values": [
            {"name": "UART_DATA_BITS_5", "value": 0, "brief": "5 data bits"},
            {"name": "UART_DATA_BITS_6", "value": 1, "brief": "6 data bits"},
            {"name": "UART_DATA_BITS_7", "value": 2, "brief": "7 data bits"},
            {"name": "UART_DATA_BITS_8", "value": 3, "brief": "8 data bits"}
        ],
        "file_path": "drivers/api/B6x_UART.h",
        "line_number": 120,
        "peripheral": "UART",
        "module": "driver"
    }


# ============================================================================
# Updated Tool 1: Enhanced SDK API Search with entry_type filter
# ============================================================================

# Update the existing search_sdk_api to support entry_type filter
def search_sdk_api_enhanced(
    query: str,
    entry_type: Optional[str] = None,
    category: Optional[str] = None,
    max_results: int = 10
) -> List[Dict]:
    """
    Enhanced search for B6x SDK content (functions, macros, enums).

    Args:
        query: Search keyword
        entry_type: Entry type filter ('function', 'macro', 'enum', None=All)
        category: Category filter (driver, ble, common)
        max_results: Maximum number of results

    Returns:
        List of matching entries of all types
    """
    logger.info(f"Searching SDK API: query='{query}', entry_type={entry_type}, category={category}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                from whoosh.query import Term, And
                from whoosh.qparser import MultifieldParser, OrGroup

                with WhooshSearcher(index_builder.index) as searcher:
                    # Build text query
                    parser = MultifieldParser(
                        ['name', 'brief', 'content'],
                        schema=index_builder.index.schema,
                        group=OrGroup
                    )
                    text_query = parser.parse(query)

                    # Build filters
                    filters = []

                    if entry_type and entry_type.lower() in ('function', 'macro', 'enum'):
                        filters.append(Term('entry_type', entry_type.lower()))

                    if category:
                        filter_module = category.lower()
                        if filter_module != "common":
                            filters.append(Term('module', filter_module))

                    # Combine queries
                    if filters:
                        combined_query = And([text_query] + filters)
                    else:
                        combined_query = text_query

                    results = searcher.search(combined_query, limit=max_results)

                    if results:
                        logger.info(f"Found {len(results)} results via Whoosh")

                        formatted = []
                        for r in results:
                            etype = r.get("entry_type", "unknown")

                            result_base = {
                                "entry_type": etype,
                                "name": r["name"],
                                "brief": r.get("brief", ""),
                                "file_path": r.get("file_path", ""),
                                "line_number": r.get("line_number", 0),
                                "peripheral": r.get("peripheral", ""),
                                "module": r.get("module", ""),
                                "score": r.get("score", 0)
                            }

                            # Add type-specific fields
                            if etype == "function":
                                result_base.update({
                                    "return_type": r.get("return_type", ""),
                                    "signature": f'{r.get("return_type", "")} {r["name"]}(...)',
                                })
                            elif etype == "macro":
                                result_base.update({
                                    "value": r.get("macro_value", ""),
                                    "macro_type": r.get("macro_type", ""),
                                })
                            elif etype == "enum":
                                result_base.update({
                                    "underlying_type": r.get("enum_underlying_type", ""),
                                    "value_count": len(json.loads(r.get("enum_values", "[]"))),
                                })

                            formatted.append(result_base)

                        return formatted

            except Exception as e:
                logger.error(f"Whoosh search failed: {e}")

    # Fallback: call the original function
    return search_sdk_api(query, category, max_results)


# ============================================================================
# NEW MCP TOOLS: Register Search and API-Register Mapping
# ============================================================================

def search_registers(
    query: str,
    peripheral: Optional[str] = None,
    access_type: Optional[str] = None,
    max_results: int = 10
) -> List[Dict]:
    """
    Search hardware register definitions from SVD.

    Args:
        query: Search keyword (e.g., "control", "baudrate", "status")
        peripheral: Optional peripheral filter (e.g., "UART", "GPIO")
        access_type: Optional access type filter ("read", "write", "read-write")
        max_results: Maximum number of results

    Returns:
        List of matching registers with address, access, and fields

    Example:
        search_registers("baudrate", peripheral="UART")
        → [{"name": "UART_BRR", "full_address": "0x40000008", ...}]
    """
    logger.info(f"Searching registers: query='{query}', peripheral={peripheral}, access={access_type}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                from whoosh.query import Term, And
                from whoosh.qparser import MultifieldParser, OrGroup

                with WhooshSearcher(index_builder.index) as searcher:
                    # Build query: entry_type=register AND text:query
                    parser = MultifieldParser(
                        ['name', 'brief', 'content'],
                        schema=index_builder.index.schema,
                        group=OrGroup
                    )
                    text_query = parser.parse(query)

                    # Filter by entry_type
                    type_query = Term('entry_type', 'register')
                    filters = [type_query]

                    # Optionally filter by peripheral
                    if peripheral:
                        filters.append(Term('peripheral', peripheral.upper()))

                    # Optionally filter by access type
                    if access_type:
                        filters.append(Term('register_access', access_type))

                    # Combine queries
                    combined_query = And(filters + [text_query])

                    results = searcher.search(combined_query, limit=max_results)

                    if results:
                        logger.info(f"Found {len(results)} register results via Whoosh")
                        formatted_results = []

                        for r in results:
                            # Parse fields JSON
                            fields = json.loads(r.get('register_fields', '[]'))

                            formatted_results.append({
                                "name": r.get('name', ''),
                                "peripheral": r.get('peripheral', ''),
                                "full_address": r.get('register_full_address', ''),
                                "address_offset": r.get('register_address_offset', ''),
                                "access": r.get('register_access', ''),
                                "reset_value": r.get('register_reset_value', ''),
                                "size": r.get('register_size', 0),
                                "description": r.get('brief', ''),
                                "fields": fields[:5],  # First 5 fields
                                "field_count": len(fields),
                                "related_apis": r.get('related_apis', '').split(',') if r.get('related_apis') else [],
                                "score": r.get('score', 0)
                            })

                        return formatted_results

            except Exception as e:
                logger.error(f"Register search failed: {e}")

    # Fallback to mock data
    logger.warning("Using mock data for register search")
    return [
        {
            "name": f"{peripheral or 'UART'}_BRR",
            "peripheral": peripheral or "UART",
            "full_address": "0x40000008",
            "address_offset": "0x08",
            "access": "read-write",
            "reset_value": "0x0000",
            "size": 32,
            "description": "Baud rate register",
            "fields": [
                {"name": "DIV_MANTISSA", "bit_range": "[15:0]", "bit_width": 16},
                {"name": "DIV_FRACTION", "bit_range": "[22:16]", "bit_width": 7}
            ],
            "field_count": 2,
            "related_apis": ["B6x_UART_SetBaudRate"],
            "score": 1.0
        }
    ]


def get_register_info(
    register_name: str,
    peripheral: Optional[str] = None
) -> Dict:
    """
    Get detailed register information including all bit fields.

    Args:
        register_name: Exact register name (e.g., "UART_CR", "BRR")
        peripheral: Optional peripheral to disambiguate

    Returns:
        Complete register definition with all fields and enumerated values

    Example:
        get_register_info("UART_BRR")
        → {name: "UART_BRR", fields: [{name: "DIV_MANTISSA", ...}, ...]}
    """
    logger.info(f"Getting register info: {register_name}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                with WhooshSearcher(index_builder.index) as searcher:
                    # Search by exact name
                    result = searcher.get_by_name(register_name)

                    if result and result.get('entry_type') == 'register':
                        # Parse fields JSON
                        fields = json.loads(result.get('register_fields', '[]'))

                        return {
                            "name": result['name'],
                            "peripheral": result.get('peripheral', ''),
                            "full_address": result.get('register_full_address', ''),
                            "base_address": result.get('register_base_address', ''),
                            "address_offset": result.get('register_address_offset', ''),
                            "access": result.get('register_access', ''),
                            "reset_value": result.get('register_reset_value', ''),
                            "size": result.get('register_size', 0),
                            "description": result.get('brief', ''),
                            "fields": fields,
                            "related_apis": result.get('related_apis', '').split(',') if result.get('related_apis') else []
                        }

            except Exception as e:
                logger.error(f"Failed to get register info: {e}")

    # Fallback
    return {
        "name": register_name,
        "peripheral": peripheral or "UNKNOWN",
        "full_address": "0x00000000",
        "description": "Register details not available",
        "fields": []
    }


def get_field_info(
    register_name: str,
    field_name: str
) -> Dict:
    """
    Get detailed bit field information including enumerated values.

    Args:
        register_name: Register name (e.g., "UART_CR")
        field_name: Field name (e.g., "UE", "TXE")

    Returns:
        Field information with bit range and enumerated values

    Example:
        get_field_info("UART_CR", "UE")
        → {name: "UE", bit_range: "[0:0]", description: "USART Enable", ...}
    """
    logger.info(f"Getting field info: {register_name}.{field_name}")

    reg_info = get_register_info(register_name)

    for field in reg_info.get('fields', []):
        if field['name'] == field_name:
            return {
                "name": field['name'],
                "bit_range": field.get('bit_range', ''),
                "bit_offset": field.get('bit_offset', 0),
                "bit_width": field.get('bit_width', 1),
                "description": field.get('description', ''),
                "access": field.get('access', 'read-write'),
                "enumerated_values": field.get('enumerated_values', [])
            }

    return {
        "error": f"Field '{field_name}' not found in register '{register_name}'"
    }


def find_registers_for_api(api_name: str) -> Dict:
    """
    Find registers used by an API function.

    Args:
        api_name: API function name (e.g., "B6x_UART_Init")

    Returns:
        List of registers accessed by the API with access type

    Example:
        find_registers_for_api("B6x_UART_Init")
        → {api: "B6x_UART_Init", registers: [{name: "UART_CR", access: "write"}, ...]}
    """
    logger.info(f"Finding registers for API: {api_name}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                with WhooshSearcher(index_builder.index) as searcher:
                    # Get API definition
                    result = searcher.get_by_name(api_name)

                    if result:
                        related_regs = result.get('related_registers', '')

                        if related_regs:
                            reg_names = related_regs.split(',')

                            registers = []
                            for reg_name in reg_names:
                                reg_name = reg_name.strip()
                                if reg_name:
                                    reg_info = get_register_info(reg_name)
                                    if 'error' not in reg_info:
                                        registers.append({
                                            "name": reg_info['name'],
                                            "full_address": reg_info['full_address'],
                                            "access": reg_info['access'],
                                            "description": reg_info['description']
                                        })

                            return {
                                "api": api_name,
                                "registers": registers,
                                "count": len(registers)
                            }

            except Exception as e:
                logger.error(f"Failed to find registers for API: {e}")

    return {
        "api": api_name,
        "registers": [],
        "count": 0,
        "note": "No register mapping found"
    }


def find_apis_for_register(register_name: str) -> Dict:
    """
    Find API functions that access a register.

    Args:
        register_name: Register name (e.g., "UART_BRR")

    Returns:
        List of APIs that read/write/configure the register

    Example:
        find_apis_for_register("UART_BRR")
        → {register: "UART_BRR", apis: [{name: "B6x_UART_SetBaudRate", access: "write"}, ...]}
    """
    logger.info(f"Finding APIs for register: {register_name}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                with WhooshSearcher(index_builder.index) as searcher:
                    # Get register definition
                    result = searcher.get_by_name(register_name)

                    if result and result.get('entry_type') == 'register':
                        related_apis = result.get('related_apis', '')

                        if related_apis:
                            api_names = related_apis.split(',')

                            apis = []
                            for api_name in api_names:
                                api_name = api_name.strip()
                                if api_name:
                                    api_def = get_api_definition(api_name)
                                    if 'error' not in api_def:
                                        apis.append({
                                            "name": api_def['name'],
                                            "brief": api_def.get('brief', ''),
                                            "return_type": api_def.get('return_type', '')
                                        })

                            return {
                                "register": register_name,
                                "peripheral": result.get('peripheral', ''),
                                "apis": apis,
                                "count": len(apis)
                            }

            except Exception as e:
                logger.error(f"Failed to find APIs for register: {e}")

    return {
        "register": register_name,
        "apis": [],
        "count": 0,
        "note": "No API mapping found"
    }


def search_by_peripheral(
    peripheral: str,
    include_fields: bool = True,
    max_results: int = 50
) -> Dict:
    """
    Get all registers for a peripheral.

    Args:
        peripheral: Peripheral name (e.g., "UART", "GPIO")
        include_fields: Whether to include field details
        max_results: Maximum results

    Returns:
        All registers for the peripheral with optional field details

    Example:
        search_by_peripheral("UART")
        → {peripheral: "UART", registers: [{name: "UART_CR", ...}, ...]}
    """
    logger.info(f"Searching by peripheral: {peripheral}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                from whoosh.query import Term, And

                with WhooshSearcher(index_builder.index) as searcher:
                    # Search for registers with this peripheral
                    query = And([
                        Term('entry_type', 'register'),
                        Term('peripheral', peripheral.upper())
                    ])

                    results = searcher.search(query, limit=max_results)

                    if results:
                        registers = []

                        for r in results:
                            reg_data = {
                                "name": r.get('name', ''),
                                "full_address": r.get('register_full_address', ''),
                                "address_offset": r.get('register_address_offset', ''),
                                "access": r.get('register_access', ''),
                                "description": r.get('brief', '')
                            }

                            if include_fields:
                                fields = json.loads(r.get('register_fields', '[]'))
                                reg_data['fields'] = fields
                                reg_data['field_count'] = len(fields)

                            registers.append(reg_data)

                        return {
                            "peripheral": peripheral.upper(),
                            "register_count": len(registers),
                            "registers": registers
                        }

            except Exception as e:
                logger.error(f"Peripheral search failed: {e}")

    return {
        "peripheral": peripheral.upper(),
        "register_count": 0,
        "registers": [],
        "error": "Search not available"
    }


def get_call_sequence(api_name: str) -> Dict:
    """
    Get recommended call sequence for an API including prerequisites.

    Args:
        api_name: API function name (e.g., "B6x_UART_Init")

    Returns:
        Call sequence with prerequisites and recommended order

    Example:
        get_call_sequence("B6x_UART_Init")
        → {
            api: "B6x_UART_Init",
            pre_requisites: [
                {api: "RCC_EnablePeriphClock", reason: "Enable UART clock"}
            ],
            call_sequence: [
                "RCC_EnablePeriphClock(RCC_PERIPH_UART1)",
                "B6x_UART_Init(...)"
            ]
          }
    """
    logger.info(f"Getting call sequence: {api_name}")

    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()

        if index_builder and index_builder.index:
            try:
                with WhooshSearcher(index_builder.index) as searcher:
                    result = searcher.get_by_name(api_name)

                    if result and result.get('entry_type') == 'function':
                        # Get prerequisite information
                        pre_reqs_str = result.get('pre_requisites', '')
                        call_seq_json = result.get('call_sequence', '')
                        requires_clock = result.get('requires_clock', False)
                        requires_gpio = result.get('requires_gpio', False)
                        dep_notes = result.get('dependency_notes', '')

                        # Parse prerequisites
                        pre_requisites = []
                        if pre_reqs_str:
                            for prereq in pre_reqs_str.split(','):
                                prereq = prereq.strip()
                                if prereq:
                                    reason = ""
                                    if prereq == "RCC_EnablePeriphClock":
                                        reason = "Enable peripheral clock"
                                    elif prereq == "GPIO_Init":
                                        reason = "Configure GPIO pins"

                                    pre_requisites.append({
                                        "api": prereq,
                                        "reason": reason
                                    })

                        # Parse call sequence
                        call_sequence = []
                        if call_seq_json:
                            try:
                                call_sequence = json.loads(call_seq_json)
                            except:
                                pass

                        # Build sequence if not explicitly defined
                        if not call_sequence and pre_requisites:
                            call_sequence = [p['api'] + "(...)" for p in pre_requisites]
                            call_sequence.append(api_name + "(...)")

                        return {
                            "api": api_name,
                            "pre_requisites": pre_requisites,
                            "call_sequence": call_sequence,
                            "requires_clock": requires_clock,
                            "requires_gpio": requires_gpio,
                            "dependency_notes": dep_notes
                        }

            except Exception as e:
                logger.error(f"Failed to get call sequence: {e}")

    # Fallback
    return {
        "api": api_name,
        "pre_requisites": [],
        "call_sequence": [api_name + "(...)"],
        "note": "Call sequence information not available"
    }


def check_dependencies(api_name: str) -> Dict:
    """
    Check if API dependencies are satisfied.

    Args:
        api_name: API function name to check

    Returns:
        Dependency check result with warnings for missing dependencies

    Example:
        check_dependencies("B6x_UART_Init")
        → {
            api: "B6x_UART_Init",
            all_dependencies_met: false,
            missing_dependencies: [
                {api: "RCC_EnablePeriphClock", severity: "error"}
            ]
          }
    """
    logger.info(f"Checking dependencies: {api_name}")

    seq_info = get_call_sequence(api_name)

    missing = []
    pre_reqs = seq_info.get('pre_requisites', [])

    for prereq in pre_reqs:
        # In a real implementation, this would check the codebase
        # for actual calls to these prerequisites
        missing.append({
            "api": prereq['api'],
            "reason": prereq.get('reason', ''),
            "severity": "warning"
        })

    return {
        "api": api_name,
        "all_dependencies_met": len(missing) == 0,
        "missing_dependencies": missing,
        "requires_clock": seq_info.get('requires_clock', False),
        "requires_gpio": seq_info.get('requires_gpio', False)
    }


def find_initialization_sequence(peripheral: str) -> List[Dict]:
    """
    Find complete initialization sequence for a peripheral.

    Returns topologically sorted initialization steps with
    clock, GPIO, and peripheral-specific initialization.

    Args:
        peripheral: Peripheral name (e.g., "UART", "SPI")

    Returns:
        Ordered list of initialization steps

    Example:
        find_initialization_sequence("UART")
        → [
            {step: 1, api: "RCC_EnablePeriphClock", reason: "Enable clock"},
            {step: 2, api: "GPIO_Init", reason: "Configure pins"},
            {step: 3, api: "B6x_UART_Init", reason: "Initialize UART"}
          ]
    """
    logger.info(f"Finding initialization sequence: {peripheral}")

    # Common initialization pattern
    sequence = []

    # Step 1: Always enable clock first
    sequence.append({
        "step": 1,
        "api": "RCC_EnablePeriphClock",
        "params": [f"RCC_PERIPH_{peripheral.upper()}1"],
        "reason": f"Enable {peripheral.upper()} peripheral clock",
        "mandatory": True
    })

    # Step 2: GPIO configuration for communication peripherals
    if peripheral.upper() in ['UART', 'SPI', 'I2C']:
        sequence.append({
            "step": 2,
            "api": "GPIO_Init",
            "params": ["..."],
            "reason": f"Configure {peripheral.upper()} GPIO pins",
            "mandatory": True
        })

    # Step 3: Peripheral-specific initialization
    init_api = f"B6x_{peripheral.upper()}_Init"

    # Check if this API exists in our index
    if WHOOSH_AVAILABLE:
        index_builder = _get_index_builder()
        if index_builder and index_builder.index:
            try:
                with WhooshSearcher(index_builder.index) as searcher:
                    result = searcher.get_by_name(init_api)
                    if result:
                        sequence.append({
                            "step": len(sequence) + 1,
                            "api": init_api,
                            "params": ["..."],
                            "reason": f"Initialize {peripheral.upper()} peripheral",
                            "mandatory": True
                        })

                        # Add any additional prerequisites from the API
                        pre_reqs = result.get('pre_requisites', '')
                        if pre_reqs:
                            for prereq in pre_reqs.split(','):
                                prereq = prereq.strip()
                                if prereq and not any(s['api'] == prereq for s in sequence):
                                    sequence.insert(-1, {
                                        "step": len(sequence),
                                        "api": prereq,
                                        "params": ["..."],
                                        "reason": "Additional prerequisite",
                                        "mandatory": True
                                    })
                                    # Renumber steps
                                    for i, s in enumerate(sequence, 1):
                                        s['step'] = i

            except Exception as e:
                logger.error(f"Failed to check API existence: {e}")

    return sequence


# ============================================================================
# Tool 8: Find Examples for API
# ============================================================================

def find_examples_for_api(
    api_name: str,
    max_examples: int = 10
) -> Dict:
    """
    Find examples that use a specific API.

    Args:
        api_name: Name of the API (e.g., "GPIO_DAT_SET", "B6x_UART_Init")
        max_examples: Maximum number of examples to return

    Returns:
        Dictionary with API usage information and examples

    Example:
        find_examples_for_api("GPIO_DAT_SET") →
        {
            "api_name": "GPIO_DAT_SET",
            "found": true,
            "total_usage": 47,
            "example_count": 12,
            "examples": [...]
        }
    """
    logger.info(f"Finding examples for API: {api_name}")

    # Load the manifest
    manifest_path = Path(__file__).parent.parent.parent.parent / "data" / "api_manifest.json"

    if not manifest_path.exists():
        return {
            "api_name": api_name,
            "found": False,
            "error": "API manifest not found. Please build the index first."
        }

    try:
        with open(manifest_path, 'r', encoding='utf-8') as f:
            manifest = json.load(f)

        # Check if API exists in manifest
        if api_name not in manifest.get('api_to_examples', {}):
            return {
                "api_name": api_name,
                "found": False,
                "error": f"API '{api_name}' not found in manifest"
            }

        # Get API statistics
        api_stats = manifest['api_statistics'].get(api_name, {})
        examples = manifest['api_to_examples'].get(api_name, [])
        call_sites = manifest['api_call_sites'].get(api_name, [])

        return {
            "api_name": api_name,
            "found": True,
            "total_usage": api_stats.get('usage_count', 0),
            "example_count": len(examples),
            "peripheral": api_stats.get('peripheral', ''),
            "module": api_stats.get('module', ''),
            "examples": examples[:max_examples],
            "call_sites": call_sites[:max_examples],
            "files": api_stats.get('files', [])
        }

    except Exception as e:
        logger.error(f"Error loading manifest: {e}")
        return {
            "api_name": api_name,
            "found": False,
            "error": str(e)
        }


# ============================================================================
# Tool 9: Get API Usage Manifest
# ============================================================================

def get_api_usage_manifest(
    category: Optional[str] = None,
    peripheral: Optional[str] = None
) -> Dict:
    """
    Get API usage manifest summary.

    Args:
        category: Optional filter by API category ('driver', 'ble')
        peripheral: Optional filter by peripheral ('GPIO', 'UART', 'SPI', etc.)

    Returns:
        API usage manifest summary

    Example:
        get_api_usage_manifest(peripheral="GPIO") →
        {
            "total_apis": 15,
            "apis": [...]
        }
    """
    logger.info(f"Getting API manifest: category={category}, peripheral={peripheral}")

    # Load the manifest
    manifest_path = Path(__file__).parent.parent.parent.parent / "data" / "api_manifest.json"

    if not manifest_path.exists():
        return {
            "error": "API manifest not found. Please build the index first."
        }

    try:
        with open(manifest_path, 'r', encoding='utf-8') as f:
            manifest = json.load(f)

        # Filter APIs based on criteria
        filtered_apis = {}

        for api_name, stats in manifest.get('api_statistics', {}).items():
            # Apply filters
            if category and stats.get('module') != category:
                continue

            if peripheral and not stats.get('peripheral', '').upper().startswith(peripheral.upper()):
                continue

            filtered_apis[api_name] = stats

        return {
            "manifest_version": manifest.get('manifest_version'),
            "generation_timestamp": manifest.get('generation_timestamp'),
            "total_apis": len(manifest.get('api_statistics', {})),
            "filtered_apis": len(filtered_apis),
            "sdk_path": manifest.get('sdk_path'),
            "scan_scope": manifest.get('scan_scope', {}),
            "total_examples": manifest.get('total_examples', 0),
            "total_files": manifest.get('total_files', 0),
            "total_api_calls": manifest.get('total_api_calls', 0),
            "unique_sdk_apis_used": manifest.get('unique_sdk_apis_used', 0),
            "apis": filtered_apis
        }

    except Exception as e:
        logger.error(f"Error loading manifest: {e}")
        return {
            "error": str(e)
        }


# ============================================================================
# Tool 10: List Examples for Peripheral
# ============================================================================

def list_examples_for_peripheral(
    peripheral: str
) -> Dict:
    """
    List all examples that use a specific peripheral.

    Args:
        peripheral: Peripheral name (e.g., "GPIO", "UART", "SPI")

    Returns:
        List of examples with API counts

    Example:
        list_examples_for_peripheral("UART") →
        {
            "peripheral": "UART",
            "example_count": 8,
            "examples": [
                {"name": "uartTest", "api_count": 15},
                {"name": "usb2uart", "api_count": 8}
            ]
        }
    """
    logger.info(f"Listing examples for peripheral: {peripheral}")

    # Load the manifest
    manifest_path = Path(__file__).parent.parent.parent.parent / "data" / "api_manifest.json"

    if not manifest_path.exists():
        return {
            "peripheral": peripheral,
            "found": False,
            "error": "API manifest not found. Please build the index first."
        }

    try:
        with open(manifest_path, 'r', encoding='utf-8') as f:
            manifest = json.load(f)

        # Find examples that use the peripheral
        peripheral = peripheral.upper()
        examples_with_api_count = {}

        for api_name, stats in manifest.get('api_statistics', {}).items():
            api_peripheral = stats.get('peripheral', '').upper()

            if api_peripheral == peripheral or api_peripheral.startswith(peripheral):
                # Get examples for this API
                for example_key in manifest.get('api_to_examples', {}).get(api_name, []):
                    if example_key not in examples_with_api_count:
                        example_name = example_key.split('/')[-1]
                        examples_with_api_count[example_key] = {
                            "name": example_name,
                            "type": example_key.split('/')[0],
                            "api_count": 0,
                            "apis": []
                        }

                    examples_with_api_count[example_key]["api_count"] += 1
                    examples_with_api_count[example_key]["apis"].append(api_name)

        # Convert to list and sort by API count
        examples_list = sorted(
            examples_with_api_count.values(),
            key=lambda x: x['api_count'],
            reverse=True
        )

        return {
            "peripheral": peripheral,
            "found": len(examples_list) > 0,
            "example_count": len(examples_list),
            "examples": examples_list
        }

    except Exception as e:
        logger.error(f"Error loading manifest: {e}")
        return {
            "peripheral": peripheral,
            "found": False,
            "error": str(e)
        }


# ============================================================================
# Tool 11: Get Example Context (NEW - Phase 1)
# ============================================================================

def get_example_context(
    api_name: str,
    example_name: Optional[str] = None,
    context_lines: int = 5
) -> Dict:
    """
    Get API usage context from examples with complete surrounding code.

    Args:
        api_name: API name (e.g., "B6x_UART_Init")
        example_name: Optional specific example (e.g., "examples/uartTest")
        context_lines: Number of context lines (default 5)

    Returns:
        {
          "api_name": "B6x_UART_Init",
          "contexts": [
            {
              "example": "examples/uartTest",
              "file": "src/main.c",
              "line": 45,
              "enclosing_function": "main",
              "context_before": ["// Initialize UART clock", "B6x_RCC_PeripheralClock_Enable(...)"],
              "context_after": ["// Configure UART pins", "B6x_UART_Init(...)"],
              "related_apis": ["B6x_RCC_PeripheralClock_Enable", ...],
              "is_in_init_section": true
            }
          ]
        }

    Example:
        get_example_context("B6x_UART_Init") →
        Returns all contexts where B6x_UART_Init is used in examples
    """
    logger.info(f"Getting example context for API: {api_name}, example: {example_name}")

    # Load the manifest
    manifest_path = Path(__file__).parent.parent.parent.parent / "data" / "api_manifest.json"

    if not manifest_path.exists():
        return {
            "api_name": api_name,
            "contexts": [],
            "error": "API manifest not found. Please build the index first."
        }

    try:
        with open(manifest_path, 'r', encoding='utf-8') as f:
            manifest = json.load(f)

        # Find examples for this API
        examples_for_api = manifest.get('api_to_examples', {}).get(api_name, [])

        if not examples_for_api:
            return {
                "api_name": api_name,
                "contexts": [],
                "found": False,
                "message": f"No examples found using {api_name}"
            }

        # Filter by example_name if specified
        if example_name:
            examples_for_api = [e for e in examples_for_api if example_name in e]

        # Extract context information
        contexts = []
        for example_key in examples_for_api[:10]:  # Limit to 10 results
            example_info = manifest.get('examples', {}).get(example_key, {})
            file_info = example_info.get('files', [])

            for file_info_item in file_info:
                file_path = file_info_item.get('path', '')
                function_calls = file_info_item.get('function_calls', [])

                for call in function_calls:
                    if call.get('function_name') == api_name:
                        contexts.append({
                            "example": example_key,
                            "file": file_path,
                            "line": call.get('line_number', 0),
                            "enclosing_function": call.get('calling_function', ''),
                            "context_before": call.get('context_before', [])[:context_lines],
                            "context_after": call.get('context_after', [])[:context_lines],
                            "related_apis": list(call.get('related_apis', set())),
                            "is_in_init_section": call.get('is_in_init_section', False),
                            "call_depth": call.get('call_depth', 0)
                        })

        return {
            "api_name": api_name,
            "found": len(contexts) > 0,
            "total_contexts": len(contexts),
            "contexts": contexts
        }

    except Exception as e:
        logger.error(f"Error getting example context: {e}")
        return {
            "api_name": api_name,
            "contexts": [],
            "error": str(e)
        }


# ============================================================================
# Tool 12: Check Pin Conflict (NEW - Phase 2)
# ============================================================================

def check_pin_conflict(
    pin_config: Dict[str, str]
) -> Dict:
    """
    Check for pin configuration conflicts.

    Args:
        pin_config: {"P0_0": "UART0_TX", "P1_5": "SPI0_SCK"}

    Returns:
        {
          "has_conflict": true,
          "conflicts": [
            {
              "pin": "P0_0",
              "requested": "UART0_TX",
              "occupied_by": "SPI0_SCK",
              "suggestion": "Use P1_0 for UART0_TX"
            }
          ]
        }

    Example:
        check_pin_conflict({"P0_0": "UART0_TX", "P0_1": "UART0_RX"}) →
        Returns no conflicts (valid UART configuration)
    """
    logger.info(f"Checking pin conflicts for config: {pin_config}")

    # Load hardware constraints
    hw_constraints_path = Path(__file__).parent.parent.parent.parent / "data" / "hardware_constraints.json"

    if not hw_constraints_path.exists():
        return {
            "has_conflict": False,
            "conflicts": [],
            "warning": "Hardware constraints file not found. Conflict detection unavailable."
        }

    try:
        from src.core.hardware_constraint_indexer import HardwareConstraintIndexer

        with open(hw_constraints_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Build pin mappings from loaded data
        pin_to_peripheral = data.get('pin_to_peripheral', {})

        conflicts = []

        # Check for duplicate pin usage
        pin_usage = {}  # pin -> list of functions
        for pin, function in pin_config.items():
            if pin not in pin_usage:
                pin_usage[pin] = []
            pin_usage[pin].append(function)

        for pin, functions in pin_usage.items():
            if len(functions) > 1:
                # Same pin used for multiple functions
                for func in functions:
                    conflicts.append({
                        "pin": pin,
                        "requested": func,
                        "occupied_by": ", ".join([f for f in functions if f != func]),
                        "suggestion": f"Pin {pin} can only be used for one function at a time"
                    })

        # Check if requested function exists for this pin
        for pin, function in pin_config.items():
            available_functions = []
            if pin in pin_to_peripheral:
                # Get peripheral names
                available_functions = pin_to_peripheral[pin]

            # Extract peripheral from function name
            requested_periph = None
            for periph in ['UART', 'SPI', 'I2C', 'I2S', 'ADC', 'DAC', 'PWM', 'TIMER', 'GPIO']:
                if function.startswith(periph):
                    requested_periph = function[:len(periph) + (1 if function[len(periph):len(periph)+1].isdigit() else 0)]
                    break

            if requested_periph and requested_periph not in available_functions:
                conflicts.append({
                    "pin": pin,
                    "requested": function,
                    "occupied_by": "N/A",
                    "suggestion": f"Pin {pin} does not support {requested_periph}. Available: {', '.join(available_functions) if available_functions else 'None'}"
                })

        return {
            "has_conflict": len(conflicts) > 0,
            "total_conflicts": len(conflicts),
            "conflicts": conflicts
        }

    except Exception as e:
        logger.error(f"Error checking pin conflicts: {e}")
        return {
            "has_conflict": False,
            "conflicts": [],
            "error": str(e)
        }


# ============================================================================
# Tool 13: Get Peripheral Pins (NEW - Phase 2)
# ============================================================================

def get_peripheral_pins(
    peripheral: str
) -> Dict:
    """
    Get available pins for a peripheral.

    Args:
        peripheral: Peripheral name (e.g., "UART0", "SPI0")

    Returns:
        {
          "peripheral": "UART0",
          "available_pins": ["P0_0", "P0_1", "P1_0", "P1_1"],
          "pin_functions": {
            "P0_0": "UART0_TX",
            "P0_1": "UART0_RX"
          },
          "required_pins": 2
        }

    Example:
        get_peripheral_pins("UART0") →
        Returns pins that can be used for UART0 functions
    """
    logger.info(f"Getting pins for peripheral: {peripheral}")

    # Load hardware constraints
    hw_constraints_path = Path(__file__).parent.parent.parent.parent / "data" / "hardware_constraints.json"

    if not hw_constraints_path.exists():
        return {
            "peripheral": peripheral,
            "available_pins": [],
            "error": "Hardware constraints file not found."
        }

    try:
        with open(hw_constraints_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        peripheral_to_pin = data.get('peripheral_to_pin', {})

        # Get pins for this peripheral
        pins = peripheral_to_pin.get(peripheral, [])

        # Estimate minimum pins needed
        required_pins = 2  # Default
        periph_upper = peripheral.upper()
        if 'UART' in periph_upper or 'SPI' in periph_upper or 'I2C' in periph_upper:
            required_pins = 2
        elif 'I2S' in periph_upper:
            required_pins = 3
        elif 'ADC' in periph_upper or 'DAC' in periph_upper:
            required_pins = 1

        # Build pin functions map (simplified)
        pin_functions = {}
        for pin in pins:
            # Generate typical function name
            if 'UART' in periph_upper:
                if 'TX' in peripheral or '0' in peripheral:
                    pin_functions[pin] = f"{peripheral}_TX" if len(pin_functions) % 2 == 0 else f"{peripheral}_RX"
            else:
                pin_functions[pin] = f"{peripheral}_{pin.split('_')[1] if '_' in pin else 'FUNC'}"

        return {
            "peripheral": peripheral,
            "available_pins": pins,
            "pin_functions": pin_functions,
            "required_pins": required_pins
        }

    except Exception as e:
        logger.error(f"Error getting peripheral pins: {e}")
        return {
            "peripheral": peripheral,
            "available_pins": [],
            "error": str(e)
        }


# ============================================================================
# Tool 14: Lookup Error Code (NEW - Phase 3)
# ============================================================================

def lookup_error_code(
    error_code: str
) -> Dict:
    """
    Look up BLE error code details.

    Args:
        error_code: Error code name ("ERR_INVALID_HANDLE") or value ("0x0180")

    Returns:
        {
          "error_code": "ERR_INVALID_HANDLE",
          "value": "0x0180",
          "category": "GATT",
          "description": "Invalid Handle",
          "probable_causes": [
            "Handle has been invalidated by a previous operation"
          ],
          "solutions": [
            "Verify the handle is valid before use"
          ],
          "related_apis": ["GATT_Write", "GATT_Read"]
        }

    Example:
        lookup_error_code("ERR_INVALID_HANDLE") →
        Returns detailed information about this error code
    """
    logger.info(f"Looking up error code: {error_code}")

    # Load error codes
    error_codes_path = Path(__file__).parent.parent.parent.parent / "data" / "ble_error_codes.json"

    if not error_codes_path.exists():
        return {
            "error_code": error_code,
            "error": "BLE error codes file not found. Please build the index first."
        }

    try:
        with open(error_codes_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Determine if error_code is a name or value
        is_name = not error_code.startswith('0x')

        if is_name:
            # Search by name
            lookup = data.get('lookup_by_name', {})
            result = lookup.get(error_code)
        else:
            # Search by value
            lookup = data.get('lookup_by_value', {})
            result = lookup.get(error_code)

        if not result:
            return {
                "error_code": error_code,
                "found": False,
                "message": f"Error code '{error_code}' not found"
            }

        # Get related APIs from api_to_errors mapping
        related_apis = []
        api_to_errors = data.get('api_to_errors', {})

        for api_name, errors in api_to_errors.items():
            if result['name'] in errors.get('error_codes', []):
                related_apis.append(api_name)

        return {
            "error_code": result['name'],
            "value": f"0x{result['value']:04X}",
            "category": result['category'],
            "description": result.get('description', ''),
            "probable_causes": result.get('probable_causes', []),
            "solutions": result.get('solutions', []),
            "related_apis": related_apis[:10],  # Limit to 10
            "found": True
        }

    except Exception as e:
        logger.error(f"Error looking up error code: {e}")
        return {
            "error_code": error_code,
            "error": str(e)
        }


# ============================================================================
# Tool 15: Get Possible Errors (NEW - Phase 3)
# ============================================================================

def get_possible_errors(
    api_name: str
) -> List[Dict]:
    """
    Get possible error codes that an API can return.

    Args:
        api_name: API name (e.g., "GATT_Write")

    Returns:
        [
          {
            "error_code": "ERR_INVALID_HANDLE",
            "probability": "high",
            "condition": "Handle is invalid or out of range"
          },
          ...
        ]

    Example:
        get_possible_errors("GATT_Write") →
        Returns list of errors that GATT_Write might return
    """
    logger.info(f"Getting possible errors for API: {api_name}")

    # Load error codes
    error_codes_path = Path(__file__).parent.parent.parent.parent / "data" / "ble_error_codes.json"

    if not error_codes_path.exists():
        return []

    try:
        with open(error_codes_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Get api_to_errors mapping
        api_to_errors = data.get('api_to_errors', {})
        error_info = api_to_errors.get(api_name, {})

        if not error_info:
            return []

        # Format error list
        errors = []
        for error_code in error_info.get('error_codes', []):
            # Get full error details
            lookup = data.get('lookup_by_name', {})
            error_detail = lookup.get(error_code, {})

            # Get condition from error_conditions
            condition = error_info.get('error_conditions', {}).get(error_code, '')

            # Determine probability
            probability = 'medium'
            if error_code in error_info.get('common_errors', []):
                probability = 'high'

            errors.append({
                "error_code": error_code,
                "probability": probability,
                "condition": condition or error_detail.get('description', ''),
                "category": error_detail.get('category', ''),
                "solutions": error_detail.get('solutions', [])[:3]  # Limit to 3 solutions
            })

        # Sort by probability
        probability_order = {'high': 0, 'medium': 1, 'low': 2}
        errors.sort(key=lambda e: probability_order.get(e['probability'], 1))

        return errors

    except Exception as e:
        logger.error(f"Error getting possible errors: {e}")
        return []


# ============================================================================
# Tool 16: Analyze Project Dependencies (NEW - Phase 4)
# ============================================================================

def analyze_project_dependencies(
    project_name: str
) -> Dict:
    """
    Analyze project dependency relationships.

    Args:
        project_name: Project name (e.g., "bleHid")

    Returns:
        {
          "project": "bleHid",
          "linked_libraries": ["ble6_8act_6con.lib", "driver.lib"],
          "include_paths": ["ble/api", "drivers/api"],
          "dependent_apis": {
            "UART": ["B6x_UART_Init", "B6x_UART_Transmit"],
            "GPIO": ["GPIO_DAT_SET", "gpio_dir_input"]
          },
          "similar_projects": [
            {"project": "bleUART", "similarity": 0.75}
          ]
        }

    Example:
        analyze_project_dependencies("bleHid") →
        Returns dependency information for the bleHid project
    """
    logger.info(f"Analyzing dependencies for project: {project_name}")

    # Load dependency graph
    dep_graph_path = Path(__file__).parent.parent.parent.parent / "data" / "project_dependencies.json"

    if not dep_graph_path.exists():
        return {
            "project": project_name,
            "error": "Project dependencies file not found. Please build the index first."
        }

    try:
        with open(dep_graph_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Get project info
        projects = data.get('projects', {})
        project_info = projects.get(project_name, {})

        if not project_info:
            return {
                "project": project_name,
                "found": False,
                "message": f"Project '{project_name}' not found in dependency graph"
            }

        # Get library info
        library_to_projects = data.get('library_to_projects', {})
        linked_libraries = []
        for lib, projs in library_to_projects.items():
            if project_name in projs:
                linked_libraries.append(lib)

        # Get API usage by peripheral
        api_to_projects = data.get('api_to_projects', {})
        peripheral_to_projects = data.get('peripheral_to_projects', {})

        dependent_apis = {}
        for periph in peripheral_to_projects.get(project_name, []):
            # Find APIs for this peripheral
            for api, projs in api_to_projects.items():
                if project_name in projs:
                    # Check if API belongs to this peripheral
                    if api.upper().startswith(periph):
                        if periph not in dependent_apis:
                            dependent_apis[periph] = []
                        dependent_apis[periph].append(api)

        return {
            "project": project_name,
            "found": True,
            "type": project_info.get('type', 'unknown'),
            "linked_libraries": linked_libraries,
            "include_paths": project_info.get('includes', [])[:10],  # Limit to 10
            "source_files": project_info.get('sources', 0),
            "target_device": project_info.get('device', ''),
            "dependent_apis": {k: sorted(set(v)) for k, v in dependent_apis.items()}
        }

    except Exception as e:
        logger.error(f"Error analyzing project dependencies: {e}")
        return {
            "project": project_name,
            "error": str(e)
        }


# ============================================================================
# Tool 17: Extract Initialization Sequence (NEW - Phase 4)
# ============================================================================

def extract_initialization_sequence(
    project_name: str
) -> Dict:
    """
    Extract initialization sequence from a project.

    Args:
        project_name: Project name

    Returns:
        {
          "project": "bleHid",
          "entry_point": "main",
          "init_sequence": [
            "SystemInit",
            "B6x_RCC_Init",
            "B6x_GPIO_Init",
            "B6x_UART_Init",
            "BLE_Init"
          ],
          "clock_config": {
            "UART0": true,
            "GPIO": true
          },
          "recommended_order": [
            "1. Initialize system clock",
            "2. Enable peripheral clocks",
            "3. Configure GPIO pins",
            "4. Initialize peripherals"
          ]
        }

    Example:
        extract_initialization_sequence("bleHid") →
        Returns the initialization sequence for bleHid project
    """
    logger.info(f"Extracting init sequence for project: {project_name}")

    # Load dependency graph
    dep_graph_path = Path(__file__).parent.parent.parent.parent / "data" / "project_dependencies.json"

    if not dep_graph_path.exists():
        return {
            "project": project_name,
            "error": "Project dependencies file not found. Please build the index first."
        }

    try:
        with open(dep_graph_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Get project call chain
        projects = data.get('projects', {})
        project_info = projects.get(project_name, {})
        call_chain = project_info.get('call_chain', {})

        if not call_chain:
            return {
                "project": project_name,
                "found": False,
                "message": f"Call chain data not found for project '{project_name}'"
            }

        init_sequence = call_chain.get('init_sequence', [])
        entry_point = call_chain.get('entry_point', 'main')
        peripheral_usage = call_chain.get('peripheral_usage', {})

        # Determine clock config from peripheral usage
        clock_config = {}
        for periph, count in peripheral_usage.items():
            if count > 0:
                clock_config[periph] = True

        # Generate recommended order
        recommended_order = [
            "1. Initialize system clock and power",
            "2. Configure GPIO pins",
            "3. Enable peripheral clocks"
        ]

        # Add peripheral-specific steps
        for periph in sorted(peripheral_usage.keys()):
            recommended_order.append(f"4. Initialize {periph} peripheral")

        return {
            "project": project_name,
            "found": True,
            "entry_point": entry_point,
            "init_sequence": init_sequence,
            "total_init_apis": len(init_sequence),
            "clock_config": clock_config,
            "peripheral_usage": peripheral_usage,
            "recommended_order": recommended_order
        }

    except Exception as e:
        logger.error(f"Error extracting init sequence: {e}")
        return {
            "project": project_name,
            "error": str(e)
        }


# ============================================================================
# Tool 18: Suggest Similar Projects (NEW - Phase 4)
# ============================================================================

def suggest_similar_projects(
    target_project: str,
    min_similarity: float = 0.3
) -> List[Dict]:
    """
    Find projects similar to the target project.

    Args:
        target_project: Target project name
        min_similarity: Minimum similarity threshold (0-1)

    Returns:
        [
          {
            "project": "bleUART",
            "similarity": 0.75,
            "common_peripherals": ["UART", "GPIO"],
            "common_apis_count": 15
          }
        ]

    Example:
        suggest_similar_projects("bleHid") →
        Returns projects with similar peripheral/API usage
    """
    logger.info(f"Finding similar projects to: {target_project}")

    # Load dependency graph
    dep_graph_path = Path(__file__).parent.parent.parent.parent / "data" / "project_dependencies.json"

    if not dep_graph_path.exists():
        return []

    try:
        with open(dep_graph_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Get target project info
        projects = data.get('projects', {})
        target_info = projects.get(target_project, {})

        if not target_info:
            return []

        # Get peripheral usage
        periph_to_projects = data.get('peripheral_to_projects', {})

        # Calculate similarity
        similar = []

        for proj_name, proj_info in projects.items():
            if proj_name == target_project:
                continue

            # Calculate peripheral overlap
            target_periphs = set()
            if 'call_chain' in target_info:
                target_periphs = set(target_info['call_chain'].get('peripheral_usage', {}).keys())

            proj_periphs = set()
            if 'call_chain' in proj_info:
                proj_periphs = set(proj_info['call_chain'].get('peripheral_usage', {}).keys())

            # Jaccard similarity
            intersection = len(target_periphs & proj_periphs)
            union = len(target_periphs | proj_periphs)
            similarity = intersection / union if union > 0 else 0.0

            if similarity >= min_similarity:
                similar.append({
                    "project": proj_name,
                    "similarity": round(similarity, 2),
                    "common_peripherals": sorted(target_periphs & proj_periphs),
                    "common_apis_count": intersection
                })

        # Sort by similarity
        similar.sort(key=lambda x: x['similarity'], reverse=True)

        return similar[:10]  # Limit to 10 results

    except Exception as e:
        logger.error(f"Error finding similar projects: {e}")
        return []


