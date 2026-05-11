"""
Common modules for B6x MCP Server.

This package contains shared utilities and data structures used across
all three layers of the MCP architecture.
"""

from .node_id import NodeId, NodeType
from .config_schema import ConfigSchema, ValidationMode

# Import new modules with fallback
try:
    from .result_types import (
        ResultStatus,
        SearchResult,
        APIResult,
        success_result,
        error_result,
        partial_result,
    )
except ImportError:
    pass

try:
    from .errors import (
        ErrorSeverity,
        MCPError,
        MCPException,
        SearchError,
        InspectionError,
        ValidationError,
        ErrorCodes,
        create_error_response,
    )
except ImportError:
    pass

try:
    from .peripheral_utils import (
        get_api_for_peripheral,
        get_apis_from_peripherals,
        get_peripheral_type_from_name,
        is_valid_peripheral_type,
        normalize_peripheral_name,
        get_peripheral_category,
    )
except ImportError:
    pass

__all__ = [
    # Original exports
    "NodeId",
    "NodeType",
    "ConfigSchema",
    "ValidationMode",
    # New result types
    "ResultStatus",
    "SearchResult",
    "APIResult",
    "success_result",
    "error_result",
    "partial_result",
    # New error types
    "ErrorSeverity",
    "MCPError",
    "MCPException",
    "SearchError",
    "InspectionError",
    "ValidationError",
    "ErrorCodes",
    "create_error_response",
    # New peripheral utilities
    "get_api_for_peripheral",
    "get_apis_from_peripherals",
    "get_peripheral_type_from_name",
    "is_valid_peripheral_type",
    "normalize_peripheral_name",
    "get_peripheral_category",
]
