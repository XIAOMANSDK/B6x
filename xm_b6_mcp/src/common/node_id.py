"""
Standard node ID format for cross-layer communication.

This module defines the standardized NodeId format used across
all three layers of the MCP architecture. It ensures consistent
node identification and parsing between layers.

Format: "type:identifier" (e.g., "api:B6x_UART_Init", "reg:UART1_BRR")

Author: B6x MCP Server Team
Version: 1.0.0
"""

import re
import logging
from enum import Enum
from typing import NamedTuple, Optional


logger = logging.getLogger(__name__)


class NodeType(str, Enum):
    """
    Standard node type prefixes.

    These prefixes are used to identify the type of SDK node
    when passing node_id between layers.
    """
    API = "api"
    REGISTER = "reg"
    DOCS = "doc"
    MACRO = "macro"
    MACROS = "macro"  # Alias for MACRO (plural form)
    EXAMPLE = "ex"


class NodeId(NamedTuple):
    """
    Standardized node ID.

    This class provides a consistent format for node identifiers
    across all layers of the MCP architecture.

    Attributes:
        node_type: The type of node (api, reg, doc, macro, ex)
        identifier: The unique identifier for the node

    Example:
        >>> node_id = NodeId(NodeType.API, "B6x_UART_Init")
        >>> node_id.to_string()
        'api:B6x_UART_Init'

        >>> parsed = NodeId.from_string("reg:UART1_BRR")
        >>> parsed.node_type
        <NodeType.REGISTER: 'reg'>
    """

    node_type: NodeType
    identifier: str

    def to_string(self) -> str:
        """
        Convert to standard string format: type:id

        Returns:
            Standardized node ID string (e.g., "api:B6x_UART_Init")
        """
        return f"{self.node_type.value}:{self.identifier}"

    @classmethod
    def from_string(cls, node_id_str: str) -> "NodeId":
        """
        Parse node_id string with strict validation.

        This method parses a node_id string and validates its format.
        It supports backward compatibility for legacy node IDs without
        type prefixes by auto-inferring the node type.

        Args:
            node_id_str: Node ID string (e.g., "api:B6x_UART_Init")

        Returns:
            NodeId instance with normalized type prefix

        Raises:
            ValueError: If format is invalid and cannot be inferred

        Examples:
            >>> NodeId.from_string("api:B6x_UART_Init")
            NodeId(node_type=<NodeType.API: 'api'>, identifier='B6x_UART_Init')

            >>> NodeId.from_string("API:B6x_UART_Init")  # Case normalized
            NodeId(node_type=<NodeType.API: 'api'>, identifier='B6x_UART_Init')

            >>> NodeId.from_string("  api: B6x_UART_Init  ")  # Whitespace trimmed
            NodeId(node_type=<NodeType.API: 'api'>, identifier='B6x_UART_Init')

            >>> NodeId.from_string("B6x_UART_Init")  # Auto-inferred
            NodeId(node_type=<NodeType.API: 'api'>, identifier='B6x_UART_Init')
        """
        # Clean input: trim whitespace
        cleaned = node_id_str.strip()

        # Parse format: type:id
        match = re.match(r"^([a-z]+):(.+)$", cleaned, re.IGNORECASE)

        if match:
            type_prefix, identifier = match.groups()

            # Normalize type prefix to lowercase
            try:
                node_type = NodeType(type_prefix.lower())
            except ValueError:
                raise ValueError(
                    f"Invalid node type prefix: '{type_prefix}'. "
                    f"Valid types: {[t.value for t in NodeType]}"
                )

            # Validate identifier is not empty
            identifier = identifier.strip()
            if not identifier:
                raise ValueError("Node identifier cannot be empty")

            return cls(node_type=node_type, identifier=identifier)

        # No type prefix found - try auto-inference for backward compatibility
        return cls._infer_from_string(cleaned)

    @classmethod
    def _infer_from_string(cls, identifier: str) -> "NodeId":
        """
        Infer node type from identifier (backward compatibility).

        This method is called when a node_id string doesn't have an
        explicit type prefix. It uses pattern matching to infer the
        most likely node type.

        Args:
            identifier: Node identifier without type prefix

        Returns:
            NodeId with inferred node type
        """
        identifier = identifier.strip()

        # API functions: B6x_* prefix
        if identifier.startswith("B6x_"):
            return cls(NodeType.API, identifier)

        # Registers: _CTRL, _REG suffixes, or specific register names
        if "_CTRL" in identifier or "_REG" in identifier or identifier.endswith("BRR"):
            return cls(NodeType.REGISTER, identifier)

        # Documentation: _guide, _primer suffixes
        if identifier.endswith("_guide") or identifier.endswith("_primer"):
            return cls(NodeType.DOCS, identifier)

        # Examples: _example suffix
        if identifier.endswith("_example") or identifier.endswith("_demo"):
            return cls(NodeType.EXAMPLE, identifier)

        # Default to API with warning
        logger.debug(f"Could not infer node type for '{identifier}', defaulting to API")
        return cls(NodeType.API, identifier)

    def is_valid(self) -> bool:
        """
        Check if this NodeId is valid.

        Returns:
            True if node_type is valid and identifier is non-empty
        """
        return (
            isinstance(self.node_type, NodeType) and
            bool(self.identifier)
        )

    def to_layer1_format(self) -> str:
        """
        Convert to Layer 1 search result format.

        Returns the node_id in the format expected by Layer 1
        search results.

        Returns:
            Node ID string in Layer 1 format
        """
        return self.to_string()

    def to_layer2_input(self) -> str:
        """
        Convert to Layer 2 inspect input format.

        Returns the node_id in the format expected by Layer 2
        inspect_node tool.

        Returns:
            Node ID string for Layer 2 inspection
        """
        return self.to_string()


def create_node_id(item_type: str, identifier: str) -> NodeId:
    """
    Helper function to create NodeId from Layer 1 item types.

    Maps Layer 1 item_type strings to NodeType enums.

    Args:
        item_type: Layer 1 item type (function, register, documentation, etc.)
        identifier: The unique identifier

    Returns:
        NodeId instance

    Examples:
        >>> create_node_id("function", "B6x_UART_Init")
        NodeId(node_type=<NodeType.API: 'api'>, identifier='B6x_UART_Init')

        >>> create_node_id("register", "UART1_BRR")
        NodeId(node_type=<NodeType.REGISTER: 'reg'>, identifier='UART1_BRR')
    """
    type_mapping = {
        "function": NodeType.API,
        "register": NodeType.REGISTER,
        "documentation": NodeType.DOCS,
        "macro": NodeType.MACRO,
        "example": NodeType.EXAMPLE,
    }

    node_type = type_mapping.get(item_type.lower(), NodeType.API)

    return NodeId(node_type=node_type, identifier=identifier)


# ---------------------------------------------------------------------------
# 2-part <-> 3-part ID conversion (bridge between MCP API and relations)
# ---------------------------------------------------------------------------

# NodeType -> candidate domains for 3-part relation IDs
_NODE_TYPE_DOMAINS: dict[NodeType, list[str]] = {
    NodeType.API: ["drivers", "ble"],
    NodeType.REGISTER: ["hardware"],
    NodeType.MACRO: ["drivers"],
    NodeType.EXAMPLE: ["applications"],
    NodeType.DOCS: [],
}

# Relation entry type -> NodeType (reverse mapping)
_RELATION_TYPE_TO_NODE_TYPE: dict[str, NodeType] = {
    "api": NodeType.API,
    "function": NodeType.API,       # legacy EntryType in drivers domain
    "ble_api": NodeType.API,        # BLE domain variant
    "register": NodeType.REGISTER,
    "macro": NodeType.MACRO,
    "macros": NodeType.MACRO,
    "example": NodeType.EXAMPLE,
}

# NodeType -> relation entry type (first match wins)
_NODE_TYPE_TO_RELATION_TYPE: dict[NodeType, str] = {
    NodeType.API: "api",
    NodeType.REGISTER: "register",
    NodeType.MACRO: "macro",
    NodeType.EXAMPLE: "example",
}


def node_id_to_relation_ids(node_id_str: str) -> list[str]:
    """
    Convert a 2-part node ID to candidate 3-part relation IDs.

    Args:
        node_id_str: 2-part node ID (e.g., "api:rtc_conf")

    Returns:
        List of 3-part candidates (e.g., ["drivers:api:rtc_conf", "ble:api:rtc_conf"])
    """
    parsed = NodeId.from_string(node_id_str)
    domains = _NODE_TYPE_DOMAINS.get(parsed.node_type, [])
    rel_type = _NODE_TYPE_TO_RELATION_TYPE.get(parsed.node_type, parsed.node_type.value)
    return [f"{domain}:{rel_type}:{parsed.identifier}" for domain in domains]


def relation_id_to_node_id(relation_id: str) -> str | None:
    """
    Convert a 3-part relation ID to a 2-part node ID.

    Args:
        relation_id: 3-part relation ID (e.g., "drivers:api:rtc_conf")

    Returns:
        2-part node ID (e.g., "api:rtc_conf"), or None if type has no NodeType mapping
    """
    parts = relation_id.split(":", 2)
    if len(parts) != 3:
        return None
    _, entry_type, name = parts
    node_type = _RELATION_TYPE_TO_NODE_TYPE.get(entry_type)
    if node_type is None:
        return None
    return f"{node_type.value}:{name}"


def normalize_node_id(node_id: str) -> str:
    """
    Normalize a node_id string to standard format.

    This is a convenience function that parses a node_id string
    and returns it in the standardized format.

    Args:
        node_id: Node ID string (may be non-standard format)

    Returns:
        Normalized node ID string

    Examples:
        >>> normalize_node_id("API:B6x_UART_Init")
        'api:B6x_UART_Init'

        >>> normalize_node_id("  api: B6x_UART_Init  ")
        'api:B6x_UART_Init'

        >>> normalize_node_id("B6x_UART_Init")
        'api:B6x_UART_Init'
    """
    parsed = NodeId.from_string(node_id)
    return parsed.to_string()


# Export for convenience
__all__ = [
    "NodeId",
    "NodeType",
    "create_node_id",
    "normalize_node_id",
    "node_id_to_relation_ids",
    "relation_id_to_node_id",
]
