"""
Standard Result Types with Error Status
========================================

Provides unified result types with error status across all layers.
This enables users to distinguish between "no results" and "search failed".

Author: B6x MCP Server Team
Version: 1.0.0
"""

from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional, Generic, TypeVar
from enum import Enum


T = TypeVar('T')


class ResultStatus(Enum):
    """Result status enumeration"""
    SUCCESS = "success"
    PARTIAL = "partial"  # Partially successful (some domains failed)
    FAILURE = "failure"


@dataclass
class SearchResult(Generic[T]):
    """
    Unified search result with status.

    Enables callers to distinguish between:
    - Success with results
    - Success but no matching data
    - Failure due to error
    """
    status: ResultStatus
    data: List[T] = field(default_factory=list)
    error_message: Optional[str] = None
    error_code: Optional[str] = None
    metadata: Dict[str, Any] = field(default_factory=dict)

    @property
    def is_success(self) -> bool:
        """Check if result is successful"""
        return self.status == ResultStatus.SUCCESS

    @property
    def has_data(self) -> bool:
        """Check if result has data"""
        return len(self.data) > 0

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for JSON serialization"""
        result = {
            "status": self.status.value,
            "success": self.is_success,
            "data": self.data,
        }
        if self.error_message:
            result["error"] = {
                "message": self.error_message,
                "code": self.error_code
            }
        if self.metadata:
            result["metadata"] = self.metadata
        return result


@dataclass
class APIResult:
    """
    Result from API operations.

    Standard result type for API calls across all layers.
    """
    success: bool
    data: Any = None
    error: Optional[Dict[str, str]] = None
    metadata: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for JSON serialization"""
        result = {
            "success": self.success,
        }
        if self.data is not None:
            result["data"] = self.data
        if self.error:
            result["error"] = self.error
        if self.metadata:
            result["metadata"] = self.metadata
        return result


def success_result(data: Any = None, metadata: Dict = None) -> Dict[str, Any]:
    """
    Create a success result dictionary.

    Args:
        data: Result data
        metadata: Optional metadata

    Returns:
        Standardized success result dict
    """
    result = {
        "status": "success",
        "success": True,
    }
    if data is not None:
        result["data"] = data
    if metadata:
        result["metadata"] = metadata
    return result


def error_result(
    message: str,
    code: str = "UNKNOWN_ERROR",
    layer: str = "",
    suggestion: str = None,
    engine: str = None
) -> Dict[str, Any]:
    """
    Create an error result dictionary.

    Args:
        message: Error message
        code: Error code (e.g., "SEARCH_FAILED", "INDEX_NOT_FOUND")
        layer: Layer where error occurred (layer1, layer2, layer3)
        suggestion: Optional suggestion for fixing the error
        engine: Engine that produced the error (for layer1)

    Returns:
        Standardized error result dict
    """
    error_info = {
        "message": message,
        "code": code,
    }
    if layer:
        error_info["layer"] = layer
    if engine:
        error_info["engine"] = engine
    if suggestion:
        error_info["suggestion"] = suggestion

    return {
        "status": "failure",
        "success": False,
        "error": error_info,
        "data": [],
    }


def partial_result(
    data: List[Any],
    failed_domains: List[str] = None,
    errors: List[Dict] = None,
    metadata: Dict = None
) -> Dict[str, Any]:
    """
    Create a partial success result (some domains succeeded, some failed).

    Args:
        data: Results from successful domains
        failed_domains: List of domains that failed
        errors: List of errors from failed domains
        metadata: Optional metadata

    Returns:
        Standardized partial result dict
    """
    result = {
        "status": "partial",
        "success": True,  # Still successful overall
        "data": data,
    }
    if failed_domains:
        result["failed_domains"] = failed_domains
    if errors:
        result["errors"] = errors
    if metadata:
        result["metadata"] = metadata
    return result


# Export symbols
__all__ = [
    "ResultStatus",
    "SearchResult",
    "APIResult",
    "success_result",
    "error_result",
    "partial_result",
]
