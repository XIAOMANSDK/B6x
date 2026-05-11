"""
Unified Error Handling for B6x MCP Server
==========================================

Provides standardized error types and handling across all layers.
This ensures consistent error reporting and enables better debugging.

Author: B6x MCP Server Team
Version: 1.0.0
"""

from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any
from enum import Enum


class ErrorSeverity(Enum):
    """Error severity levels"""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class MCPError:
    """
    Standardized error type across all layers.

    Provides consistent error information with:
    - Unique error code
    - Human-readable message
    - Severity level
    - Layer information
    - Optional suggestion for fixing
    - Context data for debugging
    """
    code: str
    message: str
    severity: ErrorSeverity = ErrorSeverity.ERROR
    layer: str = ""  # "layer1", "layer2", "layer3"
    suggestion: Optional[str] = None
    context: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for JSON serialization"""
        result = {
            "code": self.code,
            "message": self.message,
            "severity": self.severity.value,
        }
        if self.layer:
            result["layer"] = self.layer
        if self.suggestion:
            result["suggestion"] = self.suggestion
        if self.context:
            result["context"] = self.context
        return result


class MCPException(Exception):
    """
    Base exception for MCP server.

    Wraps MCPError to provide both programmatic access
    and human-readable string representation.
    """

    def __init__(self, error: MCPError):
        self.error = error
        super().__init__(error.message)

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for JSON serialization"""
        return {
            "exception": "MCPException",
            "error": self.error.to_dict()
        }


class SearchError(MCPException):
    """Exception for search-related errors (Layer 1)"""

    def __init__(self, message: str, code: str = "SEARCH_ERROR", suggestion: str = None):
        super().__init__(MCPError(
            code=code,
            message=message,
            severity=ErrorSeverity.ERROR,
            layer="layer1",
            suggestion=suggestion
        ))


class InspectionError(MCPException):
    """Exception for inspection-related errors (Layer 2)"""

    def __init__(self, message: str, code: str = "INSPECTION_ERROR", suggestion: str = None):
        super().__init__(MCPError(
            code=code,
            message=message,
            severity=ErrorSeverity.ERROR,
            layer="layer2",
            suggestion=suggestion
        ))


class ValidationError(MCPException):
    """Exception for validation-related errors (Layer 3)"""

    def __init__(self, message: str, code: str = "VALIDATION_ERROR", suggestion: str = None):
        super().__init__(MCPError(
            code=code,
            message=message,
            severity=ErrorSeverity.ERROR,
            layer="layer3",
            suggestion=suggestion
        ))


class ErrorCodes:
    """
    Predefined error codes for common error scenarios.

    Format: E{layer}{category}{number}
    - Layer: 1=discovery, 2=detail, 3=validation
    - Category: 01=not_found, 02=invalid_input, 03=internal, 04=config
    """

    # Layer 1 (Discovery) errors - E1xx
    SEARCH_FAILED = "E101"
    INDEX_NOT_FOUND = "E102"
    QUERY_TOO_SHORT = "E103"
    INVALID_SCOPE = "E104"
    TOKEN_LIMIT_EXCEEDED = "E105"

    # Layer 2 (Detail) errors - E2xx
    NODE_NOT_FOUND = "E201"
    INVALID_NODE_TYPE = "E202"
    INVALID_VIEW_TYPE = "E203"
    PARSER_NOT_AVAILABLE = "E204"
    SOURCE_FILE_NOT_FOUND = "E205"

    # Layer 3 (Validation) errors - E3xx
    VALIDATION_FAILED = "E301"
    CONFIG_LOAD_FAILED = "E302"
    CONSTRAINT_VIOLATION = "E303"
    PIN_CONFLICT = "E304"
    CLOCK_CONFIG_INVALID = "E305"
    DMA_CHANNEL_CONFLICT = "E306"

    # Common errors - E0xx
    INTERNAL_ERROR = "E001"
    INVALID_JSON = "E002"
    TIMEOUT = "E003"


# Error code descriptions for user-friendly messages
ERROR_DESCRIPTIONS = {
    ErrorCodes.SEARCH_FAILED: "Search operation failed",
    ErrorCodes.INDEX_NOT_FOUND: "Search index not found - run build scripts",
    ErrorCodes.QUERY_TOO_SHORT: "Search query is too short",
    ErrorCodes.INVALID_SCOPE: "Invalid search scope specified",
    ErrorCodes.TOKEN_LIMIT_EXCEEDED: "Result token limit exceeded",
    ErrorCodes.NODE_NOT_FOUND: "Requested node not found",
    ErrorCodes.INVALID_NODE_TYPE: "Invalid node type specified",
    ErrorCodes.INVALID_VIEW_TYPE: "Invalid view type for this node",
    ErrorCodes.PARSER_NOT_AVAILABLE: "Required parser not available",
    ErrorCodes.SOURCE_FILE_NOT_FOUND: "Source file not found",
    ErrorCodes.VALIDATION_FAILED: "Configuration validation failed",
    ErrorCodes.CONFIG_LOAD_FAILED: "Failed to load configuration",
    ErrorCodes.CONSTRAINT_VIOLATION: "Hardware constraint violated",
    ErrorCodes.PIN_CONFLICT: "Pin assignment conflict detected",
    ErrorCodes.CLOCK_CONFIG_INVALID: "Invalid clock configuration",
    ErrorCodes.DMA_CHANNEL_CONFLICT: "DMA channel conflict detected",
    ErrorCodes.INTERNAL_ERROR: "Internal server error",
    ErrorCodes.INVALID_JSON: "Invalid JSON input",
    ErrorCodes.TIMEOUT: "Operation timed out",
}


def create_error_response(
    code: str,
    message: str = None,
    layer: str = "",
    suggestion: str = None,
    **context
) -> Dict[str, Any]:
    """
    Create a standardized error response dictionary.

    Args:
        code: Error code from ErrorCodes
        message: Optional custom message (default: use predefined description)
        layer: Layer where error occurred
        suggestion: Optional suggestion for fixing
        **context: Additional context data

    Returns:
        Standardized error response dict
    """
    if message is None:
        message = ERROR_DESCRIPTIONS.get(code, "Unknown error")

    error_info = {
        "code": code,
        "message": message,
    }
    if layer:
        error_info["layer"] = layer
    if suggestion:
        error_info["suggestion"] = suggestion
    if context:
        error_info["context"] = context

    return {
        "success": False,
        "status": "failure",
        "error": error_info,
    }


# Export symbols
__all__ = [
    "ErrorSeverity",
    "MCPError",
    "MCPException",
    "SearchError",
    "InspectionError",
    "ValidationError",
    "ErrorCodes",
    "ERROR_DESCRIPTIONS",
    "create_error_response",
]
