"""
MCP Tools for 4D Knowledge Graph
===================================

Expose KnowledgeGraphQuery as MCP tools for AI agents.
Provides semantic, domain-specific queries for the B6x SDK.

Domain 1: Hardware - Pin mux, memory regions, interrupts
Domain 2: Drivers - Init APIs, config structs, dependencies
Domain 3: BLE - GATT services, error codes, profiles
Domain 4: Applications - Examples, call chains, init sequences
"""

import json
import logging
from typing import Optional, List, Any, TYPE_CHECKING
from pathlib import Path

logger = logging.getLogger(__name__)

# Try to import query interface
try:
    from src.core.knowledge_graph_query import KnowledgeGraphQuery, QueryResult
    QUERY_AVAILABLE = True
except ImportError as e:
    logger.warning(f"KnowledgeGraphQuery not available: {e}")
    QUERY_AVAILABLE = False
    # Create a dummy QueryResult for type hints when import fails
    if TYPE_CHECKING:
        from src.core.knowledge_graph_query import QueryResult
    else:
        QueryResult = Any  # type: ignore


# ============================================================================
# Helper Functions
# ============================================================================

def handle_query_result(result: Any) -> str:
    """Convert QueryResult to JSON string for MCP response."""
    if QUERY_AVAILABLE and hasattr(result, 'success'):
        output = {
            "success": result.success,
            "data": result.data if hasattr(result, 'data') else result,
            "message": result.message if hasattr(result, 'message') else "",
            "source_domain": result.source_domain if hasattr(result, 'source_domain') else "",
            "confidence": result.confidence if hasattr(result, 'confidence') else 0.0
        }
    else:
        output = {
            "success": True,
            "data": result,
            "message": "",
            "source_domain": "",
            "confidence": 1.0
        }
    return json.dumps(output, indent=2, ensure_ascii=False)


# ============================================================================
# Domain 1: Hardware Tools
# ============================================================================

async def tool_get_pin_mux(pin: str) -> str:
    """
    Get pin multiplexing options for a pin.

    Args:
        pin: Pin name (e.g., "PA00", "PB15")

    Returns:
        List of available functions for the pin
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_pin_mux(pin)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_pin_mux failed: {e}")
        return json.dumps({"error": str(e), "pin": pin})


async def tool_check_pin_conflict(pins: List[str], functions: List[str]) -> str:
    """
    Check if pin assignments have conflicts.

    Args:
        pins: List of pin names
        functions: List of desired functions

    Returns:
        Conflict analysis results
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.check_pin_conflict(pins, functions)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"check_pin_conflict failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_get_memory_region(address: int) -> str:
    """
    Get memory region for a given address.

    Args:
        address: Memory address (integer)

    Returns:
        Memory region information
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_memory_region(address)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_memory_region failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_check_memory_overlap(address: int, size: int) -> str:
    """
    Check if memory range overlaps with defined regions.

    Args:
        address: Start address
        size: Size in bytes

    Returns:
        Overlap analysis
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.check_memory_overlap(address, size)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"check_memory_overlap failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_get_interrupt_handler(irq_name: str) -> str:
    """
    Get interrupt handler for an IRQ.

    Args:
        irq_name: Interrupt name (e.g., "UART1_IRQHandler")

    Returns:
        Handler information
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_interrupt_handler(irq_name)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_interrupt_handler failed: {e}")
        return json.dumps({"error": str(e)})


# ============================================================================
# Domain 2: Driver Tools
# ============================================================================

async def tool_get_peripheral_init_api(peripheral: str) -> str:
    """
    Get initialization API for a peripheral.

    Args:
        peripheral: Peripheral name (e.g., "UART", "SPI", "GPIO")

    Returns:
        API information
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_peripheral_init_api(peripheral)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_peripheral_init_api failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_get_config_struct(peripheral: str) -> str:
    """
    Get configuration structure for a peripheral.

    Args:
        peripheral: Peripheral name

    Returns:
        Structure definition
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_config_struct(peripheral)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_config_struct failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_get_api_dependencies(api_name: str) -> str:
    """
    Get dependencies for an API.

    Args:
        api_name: API function name

    Returns:
        Dependency information
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_api_dependencies(api_name)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_api_dependencies failed: {e}")
        return json.dumps({"error": str(e)})


# ============================================================================
# Domain 3: BLE Tools
# ============================================================================

async def tool_get_gatt_service(uuid: str) -> str:
    """
    Get GATT service by UUID.

    Args:
        uuid: Service UUID (e.g., "0x180F")

    Returns:
        Service definition
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_gatt_service(uuid)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_gatt_service failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_get_characteristic_properties(char_uuid: str) -> str:
    """
    Get properties of a GATT characteristic.

    Args:
        char_uuid: Characteristic UUID

    Returns:
        Characteristic properties
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_characteristic_properties(char_uuid)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_characteristic_properties failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_get_ble_error_description(error_code: int) -> str:
    """
    Get description of a BLE error code.

    Args:
        error_code: Error code value

    Returns:
        Error description
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_ble_error_description(error_code)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_ble_error_description failed: {e}")
        return json.dumps({"error": str(e)})


# ============================================================================
# Domain 4: Application Tools
# ============================================================================

async def tool_get_example_for_api(api_name: str) -> str:
    """
    Find examples that use an API.

    Args:
        api_name: API function name

    Returns:
        List of examples
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_example_for_api(api_name)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_example_for_api failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_get_init_sequence(example_name: str) -> str:
    """
    Get initialization sequence from an example.

    Args:
        example_name: Example or project name

    Returns:
        Initialization sequence
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_init_sequence(example_name)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_init_sequence failed: {e}")
        return json.dumps({"error": str(e)})


# ============================================================================
# Cross-Domain Tools
# ============================================================================

async def tool_get_full_call_chain(api_name: str, domain: str = "drivers") -> str:
    """
    Get complete call chain from API to register level.

    Args:
        api_name: API function name
        domain: Domain of the API (default: "drivers")

    Returns:
        Complete call chain
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.get_full_call_chain(api_name, domain)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"get_full_call_chain failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_resolve_dependencies(api_name: str) -> str:
    """
    Resolve all dependencies for an API.

    Args:
        api_name: API function name

    Returns:
        Complete dependency tree
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query = KnowledgeGraphQuery()
        result = query.resolve_dependencies(api_name)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"resolve_dependencies failed: {e}")
        return json.dumps({"error": str(e)})


async def tool_search(query: str, domain: Optional[str] = None) -> str:
    """
    Semantic search across all domains.

    Args:
        query: Search query string
        domain: Optional domain filter

    Returns:
        Search results
    """
    if not QUERY_AVAILABLE:
        return json.dumps({"error": "KnowledgeGraphQuery not available"})

    try:
        query_interface = KnowledgeGraphQuery()
        result = query_interface.search(query, domain)
        return handle_query_result(result)
    except Exception as e:
        logger.error(f"search failed: {e}")
        return json.dumps({"error": str(e)})


# ============================================================================
# Tool Registration Function
# ============================================================================

def register_4d_knowledge_graph_tools(server) -> None:
    """
    Register all 4D Knowledge Graph tools with the MCP server.

    Args:
        server: MCP Server instance
    """
    logger.info("Registering 4D Knowledge Graph tools...")

    # ========================================================================
    # Domain 1: Hardware Tools
    # ========================================================================

    @server.call_tool()
    async def get_pin_mux(
        pin: str
    ) -> list[str]:
        """
        Get pin multiplexing options

        Domain: Hardware
        Args:
            pin: Pin name (e.g., "PA00", "PB15")

        Returns:
            List of available functions for the pin
        """
        result = await tool_get_pin_mux(pin)
        return [result]

    @server.call_tool()
    async def check_pin_conflict(
        pins: list[str],
        functions: list[str]
    ) -> list[str]:
        """
        Check pin assignment conflicts

        Domain: Hardware
        Args:
            pins: List of pin names
            functions: List of desired functions

        Returns:
            Conflict analysis
        """
        result = await tool_check_pin_conflict(pins, functions)
        return [result]

    @server.call_tool()
    async def get_memory_region(
        address: int
    ) -> list[str]:
        """
        Get memory region for address

        Domain: Hardware
        Args:
            address: Memory address

        Returns:
            Memory region information
        """
        result = await tool_get_memory_region(address)
        return [result]

    @server.call_tool()
    async def check_memory_overlap(
        address: int,
        size: int
    ) -> list[str]:
        """
        Check memory overlap

        Domain: Hardware
        Args:
            address: Start address
            size: Size in bytes

        Returns:
            Overlap analysis
        """
        result = await tool_check_memory_overlap(address, size)
        return [result]

    @server.call_tool()
    async def get_interrupt_handler(
        irq_name: str
    ) -> list[str]:
        """
        Get interrupt handler

        Domain: Hardware
        Args:
            irq_name: Interrupt handler name

        Returns:
            Handler information
        """
        result = await tool_get_interrupt_handler(irq_name)
        return [result]

    # ========================================================================
    # Domain 2: Driver Tools
    # ========================================================================

    @server.call_tool()
    async def get_peripheral_init_api(
        peripheral: str
    ) -> list[str]:
        """
        Get peripheral initialization API

        Domain: Drivers
        Args:
            peripheral: Peripheral name

        Returns:
            API information
        """
        result = await tool_get_peripheral_init_api(peripheral)
        return [result]

    @server.call_tool()
    async def get_config_struct(
        peripheral: str
    ) -> list[str]:
        """
        Get peripheral configuration structure

        Domain: Drivers
        Args:
            peripheral: Peripheral name

        Returns:
            Structure definition
        """
        result = await tool_get_config_struct(peripheral)
        return [result]

    @server.call_tool()
    async def get_api_dependencies(
        api_name: str
    ) -> list[str]:
        """
        Get API dependencies

        Domain: Drivers
        Args:
            api_name: API function name

        Returns:
            Dependency information
        """
        result = await tool_get_api_dependencies(api_name)
        return [result]

    # ========================================================================
    # Domain 3: BLE Tools
    # ========================================================================

    @server.call_tool()
    async def get_gatt_service(
        uuid: str
    ) -> list[str]:
        """
        Get GATT service definition

        Domain: BLE
        Args:
            uuid: Service UUID (e.g., "0x180F" for Battery Service)

        Returns:
            Service definition with characteristics
        """
        result = await tool_get_gatt_service(uuid)
        return [result]

    @server.call_tool()
    async def get_characteristic_properties(
        char_uuid: str
    ) -> list[str]:
        """
        Get characteristic properties

        Domain: BLE
        Args:
            char_uuid: Characteristic UUID (e.g., "0x2A19")

        Returns:
            Properties and capabilities
        """
        result = await tool_get_characteristic_properties(char_uuid)
        return [result]

    @server.call_tool()
    async def get_ble_error_description(
        error_code: int
    ) -> list[str]:
        """
        Get BLE error code description

        Domain: BLE
        Args:
            error_code: Error code value

        Returns:
            Error description and suggested fixes
        """
        result = await tool_get_ble_error_description(error_code)
        return [result]

    # ========================================================================
    # Domain 4: Application Tools
    # ========================================================================

    @server.call_tool()
    async def get_example_for_api(
        api_name: str
    ) -> list[str]:
        """
        Find examples using an API

        Domain: Applications
        Args:
            api_name: API function name

        Returns:
            List of examples with file paths
        """
        result = await tool_get_example_for_api(api_name)
        return [result]

    @server.call_tool()
    async def get_init_sequence(
        example_name: str
    ) -> list[str]:
        """
        Get initialization sequence

        Domain: Applications
        Args:
            example_name: Example or project name

        Returns:
            Initialization sequence with API order
        """
        result = await tool_get_init_sequence(example_name)
        return [result]

    # ========================================================================
    # Cross-Domain Tools
    # ========================================================================

    @server.call_tool()
    async def get_full_call_chain(
        api_name: str,
        domain: str = "drivers"
    ) -> list[str]:
        """
        Trace API call chain to register level

        Cross-Domain
        Args:
            api_name: API function name
            domain: Source domain (default: "drivers")

        Returns:
            Complete call chain including registers
        """
        result = await tool_get_full_call_chain(api_name, domain)
        return [result]

    @server.call_tool()
    async def resolve_dependencies(
        api_name: str
    ) -> list[str]:
        """
        Resolve complete API dependencies

        Cross-Domain
        Args:
            api_name: API function name

        Returns:
            All dependencies (clocks, GPIO, registers, etc.)
        """
        result = await tool_resolve_dependencies(api_name)
        return [result]

    @server.call_tool()
    async def search_knowledge_graph(
        query: str,
        domain: str = None
    ) -> list[str]:
        """
        Semantic search across knowledge graph

        Cross-Domain
        Args:
            query: Search query
            domain: Optional domain filter (hardware, drivers, ble, applications)

        Returns:
            Matching entries from all domains
        """
        result = await tool_search(query, domain)
        return [result]

    logger.info("Registered 20 4D Knowledge Graph tools")
