#!/usr/bin/env python3
"""
B6x MCP Server - Main Entry Point
=================================

A Model Context Protocol (MCP) server for B6x SDK that provides AI assistants
with deep knowledge of the B6x BLE SoC ecosystem.

Version: 3.5.0 (Clang Integration)
License: Proprietary
MCP SDK: >=0.9.0

Architecture:
    Three-Layer + Clang Integration
    ├── Layer 1: Discovery (search_sdk)
    ├── Layer 2: Detail (inspect_node)
    ├── Layer 3: Validation (validate_config)
    └── Clang: Semantic Analysis (clang_analyze)

MCP Compliance:
    - Server: Created with Server(name)
    - Transport: stdio (stdio_server)
    - Handlers: @server.list_tools(), @server.call_tool()
    - Response: list[TextContent] with JSON payload

Tools Provided:
    1. search_sdk(query, scope, max_results, merge_mode)
       - Discover SDK content across APIs, docs, registers, examples
       - Supports register patterns: "UART1_BRR", "BRR", "UART1"

    2. inspect_node(node_id, view_type, include_context)
       - Get detailed node information
       - Node ID format: "api:B6x_UART_Init", "reg:UART1_BRR"

    3. validate_config(config_json, stop_on_first_error, include_suggestions)
       - Validate hardware configuration against chip constraints
       - Sections: pins, clock, dma, interrupts, memory

    4. clang_analyze(code, type, files, sdk_context) [optional]
       - Real-time C code semantic analysis
       - Types: refs (find references), type (deduce type), check (diagnostics)

    5. get_server_info()
       - Server statistics and feature status

Usage:
    # Start server
    python run.py

    # Or directly
    python -m src.main

Environment Variables:
    B6X_SDK_PATH          - SDK root directory (default: parent of xm_b6_mcp)
    B6X_SERVER_NAME       - Server name (default: b6x-mcp-server)
    B6X_SERVER_VERSION    - Server version (default: 3.5.0)
    B6X_LOG_LEVEL         - Log level (default: INFO)

See Also:
    docs/MCP_INTERFACE_GUIDE.md - Complete API reference
"""

import sys
import json
import logging
import asyncio
import atexit
from pathlib import Path
from typing import Optional
from datetime import datetime

# Try to import mcp server library
try:
    from mcp.server.models import InitializationOptions
    from mcp.server import NotificationOptions, Server
    from mcp.types import (
        Resource,
        Tool,
        TextContent,
        ImageContent,
        EmbeddedResource,
    )
    MCP_AVAILABLE = True
except ImportError:
    print("Warning: MCP library not installed. Install with: pip install mcp")
    print("Falling back to stdio mode for development...")
    MCP_AVAILABLE = False

# Import local modules
try:
    from src.core.knowledge_graph import load_knowledge_graph, SDKKnowledgeGraph

    # Import three-layer architecture (ONLY)
    from src.layer1_discovery import get_search_composer, SDKSearchComposer
    from src.layer2_detail import get_node_inspector, NodeInspector
    from src.layer3_validation import get_config_validator, ConfigValidator
except ImportError as e:
    print(f"Error: Required module import failed: {e}")
    print("Please ensure three-layer architecture modules are available.")
    sys.exit(1)

# Import Clang tools (optional - gracefully degrade if not available)
try:
    from src.clang_tools import (
        CLANG_ANALYZE_TOOL_DEFINITION,
        handle_clang_analyze,
        validate_analysis_request,
    )
    CLANG_TOOLS_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Clang tools not available: {e}")
    print("Clang analysis features will be disabled.")
    CLANG_TOOLS_AVAILABLE = False
    CLANG_ANALYZE_TOOL_DEFINITION = None
    handle_clang_analyze = None
    validate_analysis_request = None

# Import Clang batch processor (optional - gracefully degrade if not available)
try:
    from src.core.clang_batch import (
        start_batch_worker,
        stop_batch_worker,
        get_batcher,
        BatchConfig,
    )
    CLANG_BATCH_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Clang batch processor not available: {e}")
    print("Batch processing will be disabled.")
    CLANG_BATCH_AVAILABLE = False
    start_batch_worker = None
    stop_batch_worker = None
    get_batcher = None
    BatchConfig = None


# ============================================================================
# Configuration
# ============================================================================

import os
from dataclasses import dataclass


@dataclass
class B6xServerConfig:
    """B6x MCP Server Configuration (v3.4 - Fixed Tool Registration)"""

    # SDK Paths
    sdk_path: str = ""
    knowledge_graph_path: str = ""
    whoosh_index_dir: str = ""

    # Server Settings
    server_name: str = "b6x-mcp-server"
    server_version: str = "3.5.0"

    # Feature flags
    clang_enabled: bool = True  # Enable Clang integration

    # Logging
    log_level: str = "INFO"

    @classmethod
    def from_env(cls) -> "B6xServerConfig":
        """Load configuration from environment variables"""
        config = cls()

        # Base path is SDK root (parent of xm_b6_mcp)
        base_path = Path(__file__).parent.parent.parent

        # SDK paths from env or defaults
        config.sdk_path = os.getenv("B6X_SDK_PATH", str(base_path))
        config.knowledge_graph_path = os.getenv(
            "B6X_KNOWLEDGE_GRAPH_PATH",
            str(base_path / "xm_b6_mcp" / "data" / "knowledge_graph.json")
        )
        config.whoosh_index_dir = os.getenv(
            "B6X_WHOOSH_INDEX_DIR",
            str(base_path / "xm_b6_mcp" / "data" / "whoosh_index")
        )

        # Server settings from env
        config.server_name = os.getenv("B6X_SERVER_NAME", "b6x-mcp-server")
        config.server_version = os.getenv("B6X_SERVER_VERSION", "3.5.0")
        config.log_level = os.getenv("B6X_LOG_LEVEL", "INFO")

        # Convert to Path objects for easier use
        config.sdk_path = Path(config.sdk_path)
        config.knowledge_graph_path = Path(config.knowledge_graph_path)
        config.whoosh_index_dir = Path(config.whoosh_index_dir)

        return config


# ============================================================================
# Global State
# ============================================================================

class B6xMCPServerState:
    """Global server state"""

    def __init__(self, config: B6xServerConfig):
        self.config = config
        self.knowledge_graph: Optional[SDKKnowledgeGraph] = None
        self.start_time = datetime.now()

        # Statistics
        self.stats = {
            "total_requests": 0,
            "successful_requests": 0,
            "failed_requests": 0,
            "tools_called": {},
        }

    def load_knowledge_graph(self) -> bool:
        """Load the knowledge graph"""
        try:
            if self.config.knowledge_graph_path.exists():
                self.knowledge_graph = load_knowledge_graph(str(self.config.knowledge_graph_path))
                logging.info(f"Knowledge graph loaded from {self.config.knowledge_graph_path}")
                return True
            else:
                logging.warning(f"Knowledge graph not found at {self.config.knowledge_graph_path}")
                return False
        except Exception as e:
            logging.error(f"Failed to load knowledge graph: {e}")
            return False

    def record_tool_call(self, tool_name: str, success: bool):
        """Record a tool call for statistics"""
        self.stats["total_requests"] += 1
        if success:
            self.stats["successful_requests"] += 1
        else:
            self.stats["failed_requests"] += 1

        if tool_name not in self.stats["tools_called"]:
            self.stats["tools_called"][tool_name] = 0
        self.stats["tools_called"][tool_name] += 1


# Global state instance
server_state: Optional[B6xMCPServerState] = None


# ============================================================================
# Tool Definitions (for list_tools)
# ============================================================================

def get_tool_definitions() -> list[Tool]:
    """Return tool definitions for list_tools handler"""
    tools = [
        Tool(
            name="search_sdk",
            description="""Search B6x SDK across multiple domains (APIs, docs, registers, examples)

This is the entry point for discovering information in the SDK.
Use this when you want to find "where information is located".

Args:
    query: Your search query (e.g., "UART initialization", "low power", "DMA")
    scope: Domains to search - "api", "docs", "macros", "registers", "examples", or "all"
    max_results: Maximum results per domain (default: 10)
    merge_mode: How to combine results - "interleave", "group", or "rank"

Register Search Patterns (scope="registers"):
    - Full name: "UART1_BRR" -> exact match for UART1's BRR register
    - Short name: "BRR" -> matches BRR register in all peripherals
    - Peripheral prefix: "UART1" -> matches all UART1 registers
    - Fuzzy: "baud" -> searches in register names and descriptions

Next Steps:
    After search, use inspect_node(item_id) to get detailed information.
    item_id format: "api:name", "reg:name", "ex:path", "macro:name", "doc:id"
    All item_id values from search results can be passed directly to inspect_node.""",
            inputSchema={
                "type": "object",
                "properties": {
                    "query": {"type": "string", "description": "Search query"},
                    "scope": {"type": "string", "default": "all",
                              "enum": ["all", "api", "docs", "macros", "registers", "examples"]},
                    "max_results": {"type": "integer", "default": 10},
                    "merge_mode": {"type": "string", "default": "interleave",
                                   "enum": ["interleave", "group", "rank"]}
                },
                "required": ["query"]
            }
        ),
        Tool(
            name="inspect_node",
            description="""Inspect a specific SDK node (API, register, doc, example, macro) with detailed view

Use this after search_sdk() to get detailed information about a specific item.

Args:
    node_id: Unique node identifier in "type:identifier" format. MUST use the item_id from search_sdk results directly.
        Examples: "api:B6x_UART_Init", "reg:UART1_CTRL", "ex:examples/spiMaster/src/main.c", "macro:SPIM_CR_DFLT", "doc:doc_xxx"
    view_type: Type of view (default: "auto")
        - All types: "auto", "summary"
        - API: "definition", "implementation", "dependencies", "call_chain", "usage_examples", "full"
        - Register: "register_info", "bit_fields", "memory_map", "full"
        - Document: "doc_content", "doc_summary", "full"
        - Example: "full"
        - Macro: "definition"
    include_context: Include related nodes and context (default: true)""",
            inputSchema={
                "type": "object",
                "properties": {
                    "node_id": {"type": "string", "description": 'Node identifier in "type:id" format (e.g. "api:B6x_UART_Init"). Use item_id from search_sdk results.'},
                    "view_type": {"type": "string", "default": "auto",
                                  "enum": ["auto", "summary", "full", "definition", "implementation", "dependencies",
                                           "call_chain", "usage_examples", "register_info", "bit_fields", "memory_map",
                                           "doc_content", "doc_summary"]},
                    "include_context": {"type": "boolean", "default": True}
                },
                "required": ["node_id"]
            }
        ),
        Tool(
            name="validate_config",
            description="""Validate hardware configuration against constraints

Use this to check if your configuration is valid and get fixing suggestions.

Args:
    config_json: Configuration as JSON string with sections:
        - pins: {"PA0": "UART1_TX", "PA1": "UART1_RX"}
        - clock: {"system_clock": 32, "enabled_peripherals": ["UART", "GPIO"]}
        - dma: [{"channel": 0, "peripheral": "UART1_TX"}]
        - interrupts: [{"irq_name": "UART1_IRQn", "priority": 5}]
        - memory: {"flash_used_kb": 128, "sram_used_kb": 16}
    stop_on_first_error: Stop validation at first error (default: false)
    include_suggestions: Include fixing suggestions (default: true)""",
            inputSchema={
                "type": "object",
                "properties": {
                    "config_json": {"type": "string", "description": "Configuration JSON"},
                    "stop_on_first_error": {"type": "boolean", "default": False},
                    "include_suggestions": {"type": "boolean", "default": True}
                },
                "required": ["config_json"]
            }
        ),
        Tool(
            name="get_server_info",
            description="Get B6x MCP server information and statistics",
            inputSchema={
                "type": "object",
                "properties": {}
            }
        )
    ]

    # Add clang_analyze tool if available
    if CLANG_TOOLS_AVAILABLE and CLANG_ANALYZE_TOOL_DEFINITION:
        tools.append(Tool(
            name="clang_analyze",
            description=CLANG_ANALYZE_TOOL_DEFINITION["description"],
            inputSchema=CLANG_ANALYZE_TOOL_DEFINITION["inputSchema"]
        ))

    return tools


# ============================================================================
# Tool Handlers
# ============================================================================

async def handle_search_sdk(arguments: dict) -> list[TextContent]:
    """Handle search_sdk tool call"""
    query = arguments.get("query", "")
    scope = arguments.get("scope", "all")
    max_results = arguments.get("max_results", 10)
    merge_mode = arguments.get("merge_mode", "interleave")

    result = await get_search_composer().search(
        query=query,
        scope=scope,
        max_results=max_results,
        merge_mode=merge_mode
    )

    if server_state:
        server_state.record_tool_call("search_sdk", "error" not in result)

    return [TextContent(type="text", text=json.dumps(result, indent=2, ensure_ascii=False))]


async def handle_inspect_node(arguments: dict) -> list[TextContent]:
    """Handle inspect_node tool call"""
    node_id = arguments.get("node_id", "")
    view_type = arguments.get("view_type", "auto")
    include_context = arguments.get("include_context", True)

    result = await get_node_inspector().inspect(
        node_id=node_id,
        view_type=view_type,
        include_context=include_context
    )

    if server_state:
        server_state.record_tool_call("inspect_node", "error" not in result)

    return [TextContent(type="text", text=json.dumps(result, indent=2, ensure_ascii=False))]


async def handle_validate_config(arguments: dict) -> list[TextContent]:
    """Handle validate_config tool call"""
    config_json = arguments.get("config_json", "{}")
    stop_on_first_error = arguments.get("stop_on_first_error", False)
    include_suggestions = arguments.get("include_suggestions", True)

    try:
        config_data = json.loads(config_json)
    except json.JSONDecodeError as e:
        result = {
            "is_valid": False,
            "error": f"Invalid JSON: {str(e)}"
        }
        return [TextContent(type="text", text=json.dumps(result, indent=2, ensure_ascii=False))]

    result = await get_config_validator().validate(
        config_data=config_data,
        stop_on_first_error=stop_on_first_error,
        include_suggestions=include_suggestions
    )

    if server_state:
        server_state.record_tool_call("validate_config", result.get("is_valid", False))

    return [TextContent(type="text", text=json.dumps(result, indent=2, ensure_ascii=False))]


async def handle_get_server_info(arguments: dict) -> list[TextContent]:
    """Handle get_server_info tool call"""
    tool_list = ["search_sdk", "inspect_node", "validate_config", "get_server_info"]
    if CLANG_TOOLS_AVAILABLE:
        tool_list.append("clang_analyze")

    info = {
        "name": server_state.config.server_name if server_state else "b6x-mcp-server",
        "version": server_state.config.server_version if server_state else "3.5.0",
        "architecture": "Three-Layer v3.5 + Clang Integration",
        "total_tools": len(tool_list),
        "tool_list": tool_list,
        "features": {
            "search": True,
            "inspect": True,
            "validate": True,
            "clang_analysis": CLANG_TOOLS_AVAILABLE,
        },
        "uptime_seconds": (datetime.now() - server_state.start_time).total_seconds() if server_state else 0,
        "stats": server_state.stats if server_state else {}
    }
    return [TextContent(type="text", text=json.dumps(info, indent=2, ensure_ascii=False))]


# Tool dispatcher
TOOL_HANDLERS = {
    "search_sdk": handle_search_sdk,
    "inspect_node": handle_inspect_node,
    "validate_config": handle_validate_config,
    "get_server_info": handle_get_server_info,
}

# Add clang_analyze handler if available
if CLANG_TOOLS_AVAILABLE and handle_clang_analyze:
    TOOL_HANDLERS["clang_analyze"] = handle_clang_analyze


# ============================================================================
# Main Entry Point
# ============================================================================

async def main():
    """Main entry point"""
    # Setup logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=[
            logging.StreamHandler(sys.stderr),
        ]
    )

    logger = logging.getLogger(__name__)
    # Load configuration first to get correct version
    config = B6xServerConfig.from_env()
    logger.info(f"Starting B6x MCP Server v{config.server_version} (Three-Layer Architecture)...")
    logger.info(f"SDK Path: {config.sdk_path}")
    logger.info(f"Server Version: {config.server_version}")

    # Initialize server state
    global server_state
    server_state = B6xMCPServerState(config)

    # Load knowledge graph
    if not server_state.load_knowledge_graph():
        logger.warning("Knowledge graph not loaded. Some features may be limited.")

    # Register shutdown hook for ClangParser (P1-2 fix)
    if CLANG_TOOLS_AVAILABLE:
        try:
            from src.core.clang_parser import get_clang_parser
            parser = get_clang_parser()
            atexit.register(parser.shutdown)
            if parser.is_available():
                logger.info(f"Clang integration enabled: {parser.libclang_version or 'unknown version'}")
            else:
                error = parser.get_load_error()
                logger.warning(f"Clang integration disabled: {error.message if error else 'unknown error'}")
        except Exception as e:
            logger.warning(f"Failed to initialize Clang shutdown hook: {e}")

    # Start Clang batch worker for request processing (P2-1)
    if CLANG_TOOLS_AVAILABLE and CLANG_BATCH_AVAILABLE:
        try:
            # Create batch configuration
            batch_config = BatchConfig(
                max_queue_size=50,
                max_waiters_per_request=10,
                batch_window_ms=50,
                queue_timeout_ms=30000,
            )
            # Start the worker
            worker = await start_batch_worker(config=batch_config)
            logger.info("Clang batch worker started")

            # Register synchronous shutdown handler
            def shutdown_batch_worker_sync():
                """Synchronous wrapper for async shutdown"""
                try:
                    # Try to get running loop
                    try:
                        loop = asyncio.get_running_loop()
                        # If we have a running loop, schedule the shutdown
                        loop.call_soon_threadsafe(
                            lambda: asyncio.ensure_future(stop_batch_worker())
                        )
                    except RuntimeError:
                        # No running loop, create new one
                        asyncio.run(stop_batch_worker())
                    logger.info("Clang batch worker stopped")
                except Exception as e:
                    logger.error(f"Error stopping batch worker: {e}")

            atexit.register(shutdown_batch_worker_sync)
        except Exception as e:
            logger.warning(f"Failed to start Clang batch worker: {e}")
            logger.warning("Falling back to direct processing mode")

    # Check if MCP is available
    if not MCP_AVAILABLE:
        logger.info("MCP library not available, running in stdio mode")
        await run_stdio_mode()
        return

    # Create MCP server
    server = Server(config.server_name)

    # ========================================================================
    # Register list_tools handler (CRITICAL for MCP protocol)
    # ========================================================================
    @server.list_tools()
    async def list_tools_handler() -> list[Tool]:
        """Return list of available tools"""
        logger.info("list_tools called - returning tool definitions")
        return get_tool_definitions()

    # ========================================================================
    # Register call_tool handler (CRITICAL for MCP protocol)
    # ========================================================================
    @server.call_tool()
    async def call_tool_handler(name: str, arguments: dict) -> list[TextContent]:
        """Handle tool calls"""
        logger.info(f"call_tool called: {name} with args: {list(arguments.keys())}")

        handler = TOOL_HANDLERS.get(name)
        if handler:
            try:
                return await handler(arguments)
            except Exception as e:
                logger.error(f"Tool {name} failed: {e}", exc_info=True)
                if server_state:
                    server_state.record_tool_call(name, False)
                return [TextContent(type="text", text=json.dumps({
                    "error": str(e),
                    "tool": name,
                    "suggestion": "Check the logs for more details"
                }, indent=2, ensure_ascii=False))]
        else:
            logger.warning(f"Unknown tool: {name}")
            return [TextContent(type="text", text=json.dumps({
                "error": f"Unknown tool: {name}",
                "available_tools": list(TOOL_HANDLERS.keys())
            }, indent=2, ensure_ascii=False))]

    # Log registration
    logger.info("=" * 60)
    logger.info(f"B6x MCP Server v{config.server_version} - Three-Layer Architecture")
    logger.info("=" * 60)
    logger.info("Registered MCP handlers:")
    logger.info("  - list_tools() -> returns tool definitions")
    logger.info("  - call_tool() -> executes tool calls")
    logger.info("=" * 60)
    logger.info("Available tools:")
    for tool_def in get_tool_definitions():
        logger.info(f"  - {tool_def.name}")
    logger.info("=" * 60)

    # Run server
    logger.info(f"Starting MCP server: {config.server_name} v{config.server_version}")

    # Use stdio transport
    from mcp.server.stdio import stdio_server

    async with stdio_server() as (read_stream, write_stream):
        await server.run(
            read_stream,
            write_stream,
            InitializationOptions(
                server_name=config.server_name,
                server_version=config.server_version,
                capabilities=server.get_capabilities(
                    notification_options=NotificationOptions(),
                    experimental_capabilities={},
                )
            )
        )


async def run_stdio_mode():
    """Run in stdio mode for testing"""
    logger = logging.getLogger(__name__)
    logger.info("Starting stdio interaction mode...")
    logger.info("Type 'help' for available commands, 'quit' to exit")

    while True:
        try:
            cmd = input("b6x-mcp> ").strip()

            if not cmd:
                continue

            if cmd.lower() in ['quit', 'exit', 'q']:
                logger.info("Goodbye!")
                break

            if cmd.lower() == 'help':
                clang_status = "enabled" if CLANG_TOOLS_AVAILABLE else "disabled"
                print(f"""
Available commands (Three-Layer Architecture + Clang):
  search <query>       - Search SDK (Layer 1)
  inspect <node_id>    - Inspect node (Layer 2)
  validate <config>    - Validate config (Layer 3)
  clang <code> <type>  - Clang analysis (Clang integration) [{clang_status}]
  info                 - Server info
  help                 - Show this help
  quit/exit            - Exit

Clang analysis types: refs, type, check
                """)
                continue

            # Add more stdio commands as needed
            print(f"Command '{cmd}' not implemented in stdio mode")

        except KeyboardInterrupt:
            print("\nGoodbye!")
            break
        except EOFError:
            break


if __name__ == "__main__":
    import asyncio
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nShutting down gracefully...")
