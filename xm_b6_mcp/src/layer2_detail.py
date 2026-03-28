"""
Layer 2: Detail Layer - Node Inspector
======================================

解决"这个东西长什么样"的问题

This layer provides detailed views of SDK nodes:
- API functions (definition, implementation, dependencies, call chain)
- Registers (info, bit fields, memory map)
- Documentation (content, summary)

Pattern: Strategy Table Pattern (映射视图类型到处理方法)

Author: B6x MCP Server Team
Version: 1.0.0
"""

import json
import logging
from typing import Dict, List, Any, Optional, Literal
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from types import SimpleNamespace

# Import existing parsers
try:
    from src.core.whoosh_indexer import WhooshSearcher, open_index
    from src.core.svd_parser import SVDParser
    from src.core.tree_sitter_parser import TreeSitterCParser
    from src.core.dependency_extractor import DoxygenDependencyExtractor
    from src.core.call_chain_extractor import CallChainExtractor
    from src.core.config_loader import get_config_loader, DependencyOverride
except ImportError as e:
    logging.warning(f"Layer 2: Import failed: {e}")

# Import NodeId standardization
try:
    from src.common.node_id import NodeId, NodeType
except ImportError:
    # Define fallback if common module not available
    class NodeType(str, Enum):
        API = "api"
        REGISTER = "register"
        DOCS = "docs"
        MACRO = "macro"
        MACROS = "macro"  # Alias for MACRO (plural form)
        EXAMPLE = "ex"

    NodeId = None
    logging.warning("Layer 2: NodeId module not available, using fallback")


logger = logging.getLogger(__name__)


# ============================================================================
# Enums and Data Classes
# ============================================================================

class ViewType(Enum):
    """View type enumeration"""
    # Universal views
    AUTO = "auto"
    SUMMARY = "summary"
    FULL = "full"

    # API specific views
    DEFINITION = "definition"
    IMPLEMENTATION = "implementation"
    DEPENDENCIES = "dependencies"
    CALL_CHAIN = "call_chain"
    USAGE_EXAMPLES = "usage_examples"

    # Register specific views
    REGISTER_INFO = "register_info"
    BIT_FIELDS = "bit_fields"
    MEMORY_MAP = "memory_map"

    # Document specific views
    DOC_CONTENT = "doc_content"
    DOC_SUMMARY = "doc_summary"


@dataclass
class ViewResponse:
    """Standard view response"""
    success: bool
    node_id: str
    node_type: str
    view_type: str
    data: Dict[str, Any]
    context: Optional[Dict[str, Any]] = None
    related_nodes: Optional[List[Dict[str, str]]] = None
    available_views: Optional[List[str]] = None
    error: Optional[str] = None


# ============================================================================
# Main Node Inspector
# ============================================================================

class NodeInspector:
    """
    Node inspector using Strategy Table Pattern.

    Maps (node_type, view_type) pairs to handler methods,
    allowing flexible multi-view inspection of SDK nodes.
    """

    # Strategy table: maps (node_type, view_type) to (method_name, dependencies)
    STRATEGIES = {
        # API strategies
        (NodeType.API, ViewType.AUTO): ("_get_api_summary", None),
        (NodeType.API, ViewType.SUMMARY): ("_get_api_summary", None),
        (NodeType.API, ViewType.FULL): ("_get_api_full", None),
        (NodeType.API, ViewType.DEFINITION): ("_get_api_definition", None),
        (NodeType.API, ViewType.IMPLEMENTATION): ("_get_api_implementation", None),
        (NodeType.API, ViewType.DEPENDENCIES): ("_get_api_dependencies", None),
        (NodeType.API, ViewType.CALL_CHAIN): ("_get_api_call_chain", ["_get_api_dependencies"]),
        (NodeType.API, ViewType.USAGE_EXAMPLES): ("_get_api_examples", None),

        # Register strategies
        (NodeType.REGISTER, ViewType.AUTO): ("_get_register_summary", None),
        (NodeType.REGISTER, ViewType.SUMMARY): ("_get_register_summary", None),
        (NodeType.REGISTER, ViewType.FULL): ("_get_register_full", None),
        (NodeType.REGISTER, ViewType.REGISTER_INFO): ("_get_register_info", None),
        (NodeType.REGISTER, ViewType.BIT_FIELDS): ("_get_bit_fields", None),
        (NodeType.REGISTER, ViewType.MEMORY_MAP): ("_get_memory_map", None),

        # Document strategies
        (NodeType.DOCS, ViewType.AUTO): ("_get_doc_summary", None),
        (NodeType.DOCS, ViewType.FULL): ("_get_doc_content", None),
        (NodeType.DOCS, ViewType.DOC_CONTENT): ("_get_doc_content", None),
        (NodeType.DOCS, ViewType.DOC_SUMMARY): ("_get_doc_summary", None),
    }

    def __init__(self):
        from pathlib import Path

        self.parsers: Dict[str, Any] = {}
        # Initialize config for parser paths
        self.config = SimpleNamespace(
            sdk_path=Path(__file__).parent.parent.parent,
            whoosh_index_dir=Path(__file__).parent.parent.parent / "xm_b6_mcp" / "data" / "whoosh_index"
        )
        self._init_parsers()

        # Load dependency overrides configuration
        try:
            self.config_loader = get_config_loader()
            self.dependencies: Dict[str, DependencyOverride] = self.config_loader.load_dependency_overrides()
            logger.info(f"NodeInspector: Loaded {len(self.dependencies)} dependency overrides")
        except Exception as e:
            logger.warning(f"NodeInspector: Failed to load dependency overrides: {e}")
            self.config_loader = None
            self.dependencies = {}

    def _init_parsers(self):
        """Initialize all parsers"""
        try:
            # Whoosh for API search
            if self.config.whoosh_index_dir and Path(self.config.whoosh_index_dir).exists():
                self.parsers["whoosh"] = open_index(self.config.whoosh_index_dir)
                logger.info("NodeInspector: Whoosh parser loaded")
        except Exception as e:
            logger.warning(f"NodeInspector: Failed to load Whoosh: {e}")

        try:
            # Tree-sitter for code parsing
            self.parsers["tree_sitter"] = TreeSitterCParser()
            logger.info("NodeInspector: Tree-sitter parser loaded")
        except Exception as e:
            logger.warning(f"NodeInspector: Failed to load Tree-sitter: {e}")

        try:
            # SVD for registers (new path: sdk6/core/B6x.svd)
            svd_path = self.config.sdk_path / "core" / "B6x.svd"

            if not svd_path.exists():
                # Fallback to previous path
                svd_path = self.config.sdk_path / "xm_b6_mcp" / "SVD" / "DragonC.svd"

            if not svd_path.exists():
                # Fallback to legacy path
                svd_path = self.config.sdk_path / "doc" / "DataSheet" / "B6x.svd"

            if svd_path.exists():
                self.parsers["svd"] = SVDParser(str(svd_path))
                logger.info(f"NodeInspector: SVD parser loaded from {svd_path}")
        except Exception as e:
            logger.warning(f"NodeInspector: Failed to load SVD: {e}")

    async def inspect(
        self,
        node_id: str,
        view_type: str = "auto",
        include_context: bool = True
    ) -> Dict[str, Any]:
        """
        Inspect a node with specified view.

        Args:
            node_id: Node identifier (e.g., "B6x_UART_Init", "UART1_CTRL")
            view_type: Type of view to display
            include_context: Include related nodes and context

        Returns:
            Detailed node information
        """
        # Parse node_id - catch exceptions and return error response
        try:
            node_type, actual_id = self._parse_node_id(node_id)
        except ValueError as e:
            return self._error_response(
                str(e),
                node_id,
                available_views=self._get_all_views()
            )

        # Auto-select view
        if view_type == "auto":
            view_type = self._auto_select_view(node_type)

        # Get strategy
        try:
            vt = ViewType(view_type)
        except ValueError:
            return self._error_response(
                f"Unknown view_type: {view_type}",
                node_id,
                available_views=self._get_all_views()
            )

        strategy = self.STRATEGIES.get((node_type, vt))

        if not strategy:
            return self._error_response(
                f"View '{view_type}' not available for node type '{node_type.value}'",
                node_id,
                available_views=self._get_available_views(node_type)
            )

        method_name, dependencies = strategy

        # Execute dependencies
        context = {}
        if dependencies:
            for dep_method in dependencies:
                try:
                    dep_result = await getattr(self, dep_method)(actual_id)
                    context.update(dep_result)
                except Exception as e:
                    logger.warning(f"Dependency {dep_method} failed: {e}")

        # Execute main strategy
        try:
            result = await getattr(self, method_name)(actual_id, context)
        except Exception as e:
            error_msg = str(e) if str(e) else f"Unknown error in {method_name}"
            logger.error(f"Strategy {method_name} failed: {error_msg}")
            return self._error_response(
                f"Failed to get view '{view_type}': {error_msg}",
                node_id
            )

        # Build response
        response = {
            "success": True,
            "node_id": node_id,
            "node_type": node_type.value,
            "view_type": view_type,
            "data": result,
        }

        # Add warning if result contains mock data
        if isinstance(result, dict) and result.get("_mock"):
            response["_warning"] = result.get("_warning", "Mock data returned")
            response["_suggestion"] = result.get("_suggestion", "Check index availability")

        if include_context:
            response["context"] = await self._get_context(node_type, actual_id)
            response["related_nodes"] = await self._get_related_nodes(node_type, actual_id)
            response["available_views"] = self._get_available_views(node_type)

        return response

    def _parse_node_id(self, node_id: str) -> tuple[NodeType, str]:
        """
        Parse node_id using strict validation.

        Args:
            node_id: Node ID string (e.g., "api:B6x_UART_Init")

        Returns:
            (NodeType, identifier) tuple

        Raises:
            ValueError: If format is invalid
        """
        # Validate empty identifier when colon is present
        if ":" in node_id:
            type_prefix, actual_id = node_id.split(":", 1)

            # Check for empty identifier
            if not actual_id or not actual_id.strip():
                raise ValueError(
                    f"Empty identifier in node_id: '{node_id}'. "
                    f"Identifier part cannot be empty."
                )

            # Check for valid type prefix
            try:
                node_type = NodeType(type_prefix.lower())
                return node_type, actual_id
            except ValueError:
                # Invalid type prefix - return error instead of auto-inferring
                raise ValueError(
                    f"Invalid type prefix '{type_prefix}' in node_id: '{node_id}'. "
                    f"Valid types: {[t.value for t in NodeType]}"
                )

        # Use NodeId parser if available (for auto-inference)
        if NodeId is not None:
            try:
                parsed = NodeId.from_string(node_id)
                node_type = NodeType(parsed.node_type.value)
                return node_type, parsed.identifier
            except ValueError:
                pass  # Fall through to legacy auto-infer

        # Auto-infer node type (legacy compatibility)
        return self._infer_node_type(node_id)

    def _infer_node_type(self, node_id: str) -> tuple[NodeType, str]:
        """Infer node type from node_id (backward compatibility)"""
        if node_id.startswith("B6x_"):
            return NodeType.API, node_id
        elif "_CTRL" in node_id or "_REG" in node_id or node_id.endswith("BRR"):
            return NodeType.REGISTER, node_id
        elif node_id.endswith("_guide") or node_id.endswith("_primer"):
            return NodeType.DOCS, node_id
        else:
            # Default to API
            return NodeType.API, node_id

    def _auto_select_view(self, node_type: NodeType) -> str:
        """Auto-select best view for node type"""
        auto_views = {
            NodeType.API: "summary",
            NodeType.REGISTER: "register_info",
            NodeType.DOCS: "doc_summary",
            NodeType.MACRO: "summary",
            NodeType.EXAMPLE: "summary"
        }
        return auto_views.get(node_type, "summary")

    def _get_available_views(self, node_type: NodeType) -> List[str]:
        """Get all available views for a node type"""
        return [
            view_type.value
            for (nt, view_type) in self.STRATEGIES.keys()
            if nt == node_type
        ]

    def _get_all_views(self) -> List[str]:
        """Get all possible view types"""
        return list(set(v.value for (_, v) in self.STRATEGIES.keys()))

    # ========================================================================
    # API Strategy Methods
    # ========================================================================

    async def _get_api_summary(self, api_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get API summary (brief overview)"""
        definition = await self._get_api_definition(api_name)
        return {
            "name": api_name,
            "signature": definition.get("signature", ""),
            "brief": definition.get("brief", ""),
            "parameters": definition.get("parameters", [])[:3],
            "return_type": definition.get("return_type", "void"),
            "header_file": definition.get("header_file", ""),
        }

    async def _get_api_definition(self, api_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get complete API definition"""
        # Try to get from Whoosh index
        if "whoosh" in self.parsers:
            try:
                results = self.parsers["whoosh"].search(api_name, limit=1)
                if results and len(results) > 0:
                    result = results[0]
                    return {
                        "name": api_name,
                        "signature": result.get("signature", ""),
                        "brief": result.get("brief", ""),
                        "description": result.get("description", ""),
                        "parameters": result.get("parameters", []),
                        "return_type": result.get("return_type", "void"),
                        "header_file": result.get("header_file", ""),
                        "line_number": result.get("line_number", 0),
                    }
            except Exception as e:
                logger.warning(f"Whoosh search failed: {e}")

        # Fallback: return mock data
        return self._mock_api_definition(api_name)

    async def _get_api_implementation(self, api_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get API implementation (source code)"""
        # Try Tree-sitter parser
        if "tree_sitter" in self.parsers:
            try:
                impl = self.parsers["tree_sitter"].find_function_definition(api_name)
                if impl:
                    return {
                        "name": api_name,
                        "source_file": impl.get("file", ""),
                        "line_number": impl.get("line", 0),
                        "implementation": impl.get("code", ""),
                        "complexity": impl.get("complexity", "unknown"),
                    }
            except Exception as e:
                logger.warning(f"Tree-sitter parsing failed: {e}")

        # Fallback: return mock implementation
        return self._mock_api_implementation(api_name)

    async def _get_api_dependencies(self, api_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get API dependencies (hardware and software)"""
        # First, try to get from dependency_overrides.yaml
        if api_name in self.dependencies:
            dep = self.dependencies[api_name]

            result = {
                "api_name": dep.api,
                "source": "dependency_overrides.yaml",
                "pre_requisites": dep.pre_requisites,
                "call_sequence": dep.call_sequence,
                "notes": dep.notes,
                "requirements": {
                    "clock": dep.requires_clock,
                    "gpio": dep.requires_gpio,
                    "dma": dep.requires_dma,
                    "interrupt": dep.requires_interrupt,
                },
                "see_also": dep.see_also,
                "parameters": dep.parameters,
                "source_reference": {
                    "file": dep.source_file,
                    "line": dep.source_line,
                },
            }

            # Add register mapping information from api_register_mapping.yaml
            if self.config_loader:
                try:
                    registers = self.config_loader.get_registers_for_api(api_name)
                    if registers:
                        result["registers_accessed"] = registers
                        result["register_count"] = len(registers)
                except Exception as e:
                    logger.warning(f"Failed to get register mapping for {api_name}: {e}")

            return result

        # Fallback: Try to extract from source using tree-sitter
        if "tree_sitter" in self.parsers:
            try:
                deps = self.parsers["tree_sitter"].extract_dependencies(api_name)
                if deps:
                    return {
                        "api_name": api_name,
                        "source": "tree_sitter_parser",
                        "hardware_dependencies": deps.get("hardware", {}),
                        "software_dependencies": deps.get("software", {}),
                        "resource_requirements": deps.get("resources", {}),
                    }
            except Exception as e:
                logger.warning(f"Dependency extraction failed: {e}")

        # Final fallback: return mock dependencies
        return self._mock_api_dependencies(api_name)

    async def _get_api_call_chain(self, api_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get API call chain"""
        # First, try to get from dependency_overrides.yaml
        if api_name in self.dependencies and self.config_loader:
            call_sequence = self.config_loader.get_call_sequence(api_name)

            # Build call tree from sequence
            call_tree = []
            for i, api in enumerate(call_sequence):
                call_tree.append({
                    "api": api,
                    "level": i,
                    "calls": []
                })

            return {
                "api_name": api_name,
                "source": "dependency_overrides.yaml",
                "call_sequence": call_sequence,
                "call_tree": call_tree,
                "max_depth": len(call_sequence),
                "total_calls": len(call_sequence),
            }

        # Fallback: Try tree-sitter extraction
        deps = context if context else await self._get_api_dependencies(api_name)

        return {
            "call_tree": [
                {
                    "api": api_name,
                    "level": 0,
                    "calls": [
                        {
                            "api": "B6x_RCC_EnablePeriphClock",
                            "level": 1,
                            "calls": [
                                {"api": "__raw_write_reg", "level": 2}
                            ]
                        },
                        {
                            "api": "B6x_GPIO_Init",
                            "level": 1,
                            "calls": []
                        }
                    ]
                }
            ],
            "max_depth": 2,
            "total_calls": 3,
        }

    async def _get_api_full(self, api_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get full API information (all views combined)"""
        return {
            "definition": await self._get_api_definition(api_name),
            "implementation": await self._get_api_implementation(api_name),
            "dependencies": await self._get_api_dependencies(api_name),
        }

    async def _get_api_examples(self, api_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get API usage examples"""
        # This would search the examples directory
        return {
            "examples": [
                {
                    "title": "Basic UART Initialization",
                    "path": "examples/peripheral/uart/uart_basic/main.c",
                    "snippet": '''
// Initialize UART
UART_Config uart_config = {
    .baudrate = 115200,
    .word_length = UART_WORD_LENGTH_8B,
    .stop_bits = UART_STOP_BITS_1
};
B6x_UART_Init(UART1, &uart_config);
'''
                }
            ]
        }

    # ========================================================================
    # Register Strategy Methods
    # ========================================================================

    async def _get_register_info(self, reg_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get register information"""
        # Get base register info from SVD
        svd_info = {}
        if "svd" in self.parsers:
            try:
                svd_info = self.parsers["svd"].get_register_info(reg_name)
            except Exception as e:
                error_msg = str(e) if str(e) else "Unknown SVD parsing error"
                logger.warning(f"SVD parsing failed for {reg_name}: {error_msg}")

        # Enhance with API mapping information
        if self.config_loader:
            try:
                # Get APIs that access this register
                api_access = self.config_loader.get_apis_for_register(reg_name)

                if api_access and (api_access['read'] or api_access['write'] or api_access['configure']):
                    # Build API list with access types
                    apis_with_access = []

                    for access_type, apis in api_access.items():
                        for api in apis:
                            apis_with_access.append({
                                "api": api,
                                "access_type": access_type
                            })

                    # Add to SVD info
                    if svd_info:
                        svd_info["accessing_apis"] = apis_with_access
                    else:
                        # No SVD info, create minimal response
                        svd_info = {
                            "name": reg_name,
                            "accessing_apis": apis_with_access,
                            "source": "api_register_mapping.yaml"
                        }
            except Exception as e:
                error_msg = str(e) if str(e) else "Unknown config loader error"
                logger.warning(f"Failed to get API mapping for register {reg_name}: {error_msg}")

        # Return SVD info (with or without API mapping)
        if svd_info:
            return svd_info

        # Fallback: mock register info
        return self._mock_register_info(reg_name)

    async def _get_bit_fields(self, reg_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get register bit fields"""
        if "svd" in self.parsers:
            try:
                fields = self.parsers["svd"].get_register_fields(reg_name)
                if fields:
                    return {
                        "register": reg_name,
                        "fields": fields,
                        "total_fields": len(fields),
                    }
            except Exception as e:
                logger.warning(f"SVD field parsing failed: {e}")

        # Fallback: mock bit fields
        return self._mock_bit_fields(reg_name)

    async def _get_register_summary(self, reg_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get register summary"""
        info = await self._get_register_info(reg_name)
        return {
            "name": info.get("name", reg_name),
            "peripheral": info.get("peripheral", ""),
            "description": info.get("description", ""),
            "address": info.get("address", ""),
            "access": info.get("access", ""),
        }

    async def _get_register_full(self, reg_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get full register information"""
        return {
            "info": await self._get_register_info(reg_name),
            "bit_fields": await self._get_bit_fields(reg_name),
        }

    async def _get_memory_map(self, reg_name: str, context: Dict = None) -> Dict[str, Any]:
        """Get memory map for register"""
        return {
            "register": reg_name,
            "memory_map": {
                "base_address": "0x40000000",
                "offset": "0x00",
                "size": "4 bytes",
                "peripheral_base": "0x40011000",
            }
        }

    # ========================================================================
    # Document Strategy Methods
    # ========================================================================

    async def _get_doc_summary(self, doc_id: str, context: Dict = None) -> Dict[str, Any]:
        """Get document summary"""
        return {
            "id": doc_id,
            "title": doc_id.replace("_", " ").title(),
            "type": "PDF",
            "pages": 45,
            "summary": f"Documentation for {doc_id}",
        }

    async def _get_doc_content(self, doc_id: str, context: Dict = None) -> Dict[str, Any]:
        """Get document content"""
        return {
            "id": doc_id,
            "content": "Document content would be here...",
            "sections": [
                {"title": "Introduction", "page": 1},
                {"title": "Configuration", "page": 5},
                {"title": "API Reference", "page": 15},
            ]
        }

    # ========================================================================
    # Context and Related Nodes
    # ========================================================================

    async def _get_context(self, node_type: NodeType, node_id: str) -> Dict[str, Any]:
        """Get node context information"""
        if node_type == NodeType.API and "UART" in node_id:
            return {
                "module": "UART Driver",
                "category": "Peripheral Drivers",
                "related_peripherals": ["UART1", "UART2"],
                "related_docs": ["docs/uart_guide.pdf"],
            }
        return {}

    async def _get_related_nodes(self, node_type: NodeType, node_id: str) -> List[Dict[str, str]]:
        """Get related nodes"""
        if node_type == NodeType.API and "UART" in node_id:
            return [
                {"node_id": "UART1_CTRL", "relation": "uses_register", "node_type": "register"},
                {"node_id": "B6x_UART_Transmit", "relation": "related_api", "node_type": "api"},
                {"node_id": "uart_example", "relation": "example", "node_type": "examples"},
            ]
        return []

    def _error_response(
        self,
        message: str,
        node_id: str,
        available_views: List[str] = None
    ) -> Dict[str, Any]:
        """Generate error response"""
        return {
            "success": False,
            "error": message,
            "node_id": node_id,
            "available_views": available_views or []
        }

    # ========================================================================
    # Mock Data Methods (for fallback)
    # ========================================================================

    # Warning message for mock data
    MOCK_DATA_WARNING = "MOCK DATA - Index not available"
    MOCK_DATA_SUGGESTION = "Run build scripts to generate Whoosh index"

    def _mock_api_definition(self, api_name: str) -> Dict[str, Any]:
        """Generate mock API definition with clear warning."""
        return {
            "_mock": True,
            "_warning": self.MOCK_DATA_WARNING,
            "_suggestion": self.MOCK_DATA_SUGGESTION,
            "name": api_name,
            "signature": f"void {api_name}(void *instance, const {api_name}_Config *config)",
            "brief": f"[MOCK] Initialize and configure {api_name.replace('B6x_', '')}",
            "description": "This is mock data. Real data not available - index not loaded.",
            "parameters": [
                {"name": "instance", "type": "void*", "description": "Peripheral instance pointer"},
                {"name": "config", "type": "const Config*", "description": "Configuration structure"}
            ],
            "return_type": "void",
            "header_file": f"drivers/api/{api_name.lower()}.h",
            "line_number": 125,
        }

    def _mock_api_implementation(self, api_name: str) -> Dict[str, Any]:
        """Generate mock API implementation with clear warning."""
        return {
            "_mock": True,
            "_warning": self.MOCK_DATA_WARNING,
            "_suggestion": self.MOCK_DATA_SUGGESTION,
            "name": api_name,
            "source_file": f"drivers/src/{api_name.lower()}.c",
            "line_number": 256,
            "implementation": f"// MOCK DATA - Implementation not available. Index not loaded.",
            "complexity": "unknown",
            "cyclomatic_complexity": 0,
        }

    def _mock_api_dependencies(self, api_name: str) -> Dict[str, Any]:
        """Generate mock API dependencies with clear warning."""
        return {
            "_mock": True,
            "_warning": self.MOCK_DATA_WARNING,
            "_suggestion": self.MOCK_DATA_SUGGESTION,
            "hardware_dependencies": {
                "peripherals": [f"{api_name.split('_')[1] if '_' in api_name else 'UNKNOWN'}"],
                "clocks": ["PCLK"],
                "dma_channels": [],
                "interrupts": [f"{api_name.split('_')[1] if '_' in api_name else 'UNKNOWN'}_IRQn"],
            },
            "software_dependencies": {
                "apis_called": ["B6x_RCC_EnablePeriphClock", "B6x_GPIO_Init"],
                "macros_used": [f"{api_name.split('_')[1] if '_' in api_name else 'UNKNOWN'}_ENABLE"],
                "types_used": [f"{api_name}_Config", f"{api_name}_TypeDef"],
            },
            "resource_requirements": {
                "flash": "N/A",
                "sram": "N/A",
                "execution_time_us": 0,
            }
        }

    def _mock_register_info(self, reg_name: str) -> Dict[str, Any]:
        """Generate mock register info with clear warning."""
        peripheral = reg_name.split('_')[0] if '_' in reg_name else "UNKNOWN"
        return {
            "_mock": True,
            "_warning": self.MOCK_DATA_WARNING,
            "_suggestion": self.MOCK_DATA_SUGGESTION,
            "name": reg_name,
            "peripheral": peripheral,
            "description": f"[MOCK] {peripheral} register - SVD not loaded",
            "address": "0x40000000",
            "offset": "0x00",
            "access": "read-write",
            "reset_value": "0x00000000",
            "size_bits": 32,
        }

    def _mock_bit_fields(self, reg_name: str) -> Dict[str, Any]:
        """Generate mock bit fields with clear warning."""
        return {
            "_mock": True,
            "_warning": self.MOCK_DATA_WARNING,
            "_suggestion": self.MOCK_DATA_SUGGESTION,
            "register": reg_name,
            "fields": [
                {
                    "name": "ENABLE",
                    "bit_range": "[0]",
                    "description": "[MOCK] Enable bit",
                    "access": "read-write",
                    "reset_value": "0"
                },
                {
                    "name": "MODE",
                    "bit_range": "[2:1]",
                    "description": "[MOCK] Mode selection",
                    "access": "read-write",
                    "reset_value": "0x00"
                }
            ],
            "total_fields": 2,
            "reserved_bits": "[31:3]",
        }


# ============================================================================
# Global Instance and Tool Registration
# ============================================================================

_inspector_instance: Optional[NodeInspector] = None


def get_node_inspector() -> NodeInspector:
    """Get or create node inspector singleton"""
    global _inspector_instance
    if _inspector_instance is None:
        try:
            # config removed
            _inspector_instance = NodeInspector()
        except Exception as e:
            logger.error(f"Failed to create NodeInspector: {e}")
            _inspector_instance = NodeInspector()
    return _inspector_instance


# Export for use in main.py
__all__ = [
    "NodeInspector",
    "ViewType",
    "NodeType",
    "ViewResponse",
    "get_node_inspector"
]
