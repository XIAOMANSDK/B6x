"""
Clang Analysis Tools for MCP Server
====================================

Provides clang_analyze() MCP tool for real-time C code analysis.
This module integrates with the Clang parser to offer semantic analysis
capabilities through the MCP protocol.

Author: B6x MCP Server Team
Version: 1.0.0
"""

import json
import logging
from typing import Dict, List, Any, Optional
from dataclasses import dataclass

logger = logging.getLogger(__name__)


# ============================================================================
# Tool Definition
# ============================================================================

CLANG_ANALYZE_TOOL_DEFINITION = {
    "name": "clang_analyze",
    "description": """Analyze C code using Clang for semantic understanding.

This tool provides real-time code analysis including:
- Reference finding: Find all uses of a symbol across SDK
- Type deduction: Determine the type of an expression
- Diagnostics: Check code for errors and warnings

IMPORTANT: This is a real-time analysis tool, not a search tool.
Use search_sdk() for searching indexed SDK content.

Args:
    code: C code snippet or symbol name to analyze
    type: Analysis type - "refs" (find references), "type" (deduce type), "check" (diagnostics)
    files: Optional context files for analysis (max 3 files)
    sdk_context: Optional SDK context override (include_paths, defines, target)
    brief: Return brief output (default: true)

Returns:
    Analysis results with success status, results, and metadata

Examples:
    # Find all references to a symbol
    clang_analyze(code="B6x_UART_Init", type="refs")

    # Deduce type of an expression
    clang_analyze(code="cfg->baudrate", type="type")

    # Check code for errors
    clang_analyze(code="B6x_UART_Init(UART1, &cfg);", type="check")

    # With context files
    clang_analyze(
        code="uart_init(&config)",
        type="refs",
        files=["projects/my_app/src/uart_config.c"]
    )
""",
    "inputSchema": {
        "type": "object",
        "properties": {
            "code": {
                "type": "string",
                "description": "C code snippet or symbol name to analyze"
            },
            "type": {
                "type": "string",
                "enum": ["refs", "type", "check"],
                "description": "Analysis type: refs=find references, type=deduce type, check=diagnostics"
            },
            "files": {
                "type": "array",
                "items": {"type": "string"},
                "maxItems": 3,
                "description": "Optional context files for analysis (max 3)"
            },
            "sdk_context": {
                "type": "object",
                "properties": {
                    "include_paths": {
                        "type": "array",
                        "items": {"type": "string"},
                        "description": "Additional include paths"
                    },
                    "defines": {
                        "type": "object",
                        "additionalProperties": {"type": "string"},
                        "description": "Additional preprocessor defines"
                    },
                    "target": {
                        "type": "string",
                        "default": "arm-none-eabi",
                        "description": "Target platform"
                    }
                },
                "description": "Optional SDK context override"
            },
            "brief": {
                "type": "boolean",
                "default": True,
                "description": "Return brief output"
            }
        },
        "required": ["code", "type"]
    }
}


# ============================================================================
# Analysis Type Descriptions
# ============================================================================

ANALYSIS_TYPE_DESCRIPTIONS = {
    "refs": """Find all references to a symbol.

Searches for all uses of a symbol (function, variable, type, macro) across
the SDK source files. Returns file locations with line numbers and context.

Use cases:
- Find where a function is called
- Find all uses of a configuration macro
- Trace variable usage across files
""",
    "type": """Deduce the type of an expression.

Analyzes a C expression and determines its type, including:
- Builtin types (int, uint32_t, etc.)
- Pointer types
- Struct/union types
- Function pointer types

Use cases:
- Understand configuration structure fields
- Verify function parameter types
- Debug type-related errors
""",
    "check": """Check code for compilation diagnostics.

Analyzes code for errors, warnings, and informational messages that
would be reported by a compiler.

Use cases:
- Validate code syntax
- Check for potential bugs
- Get suggestions for code improvements
"""
}


# ============================================================================
# Tool Handler
# ============================================================================

async def handle_clang_analyze(arguments: dict) -> list:
    """
    Handle clang_analyze tool call.

    Uses batch processing for request deduplication and queue management.

    Args:
        arguments: Tool arguments from MCP client

    Returns:
        List containing TextContent with JSON result
    """
    from mcp.types import TextContent

    # Validate arguments first (P0-1 fix)
    is_valid, error_msg = validate_analysis_request(arguments)
    if not is_valid:
        return [TextContent(
            type="text",
            text=json.dumps({
                "success": False,
                "error": {
                    "code": "INVALID_INPUT",
                    "message": error_msg,
                    "solution": "Check parameter format and values"
                }
            }, indent=2, ensure_ascii=False)
        )]

    # Extract arguments
    code = arguments.get("code", "")
    analysis_type = arguments.get("type", "refs")
    files = arguments.get("files", [])
    sdk_context = arguments.get("sdk_context")
    brief = arguments.get("brief", True)

    # Check if batch processing is enabled (default: use batcher)
    use_batch = arguments.get("_use_batch", True)

    if use_batch:
        # Use batch processor for request deduplication and queue management
        try:
            from src.core.clang_batch import get_batcher, get_worker

            # Check if worker is running
            worker = get_worker()
            if worker is None or not worker.is_running:
                # Fall back to direct processing
                logger.warning("Batch worker not running, falling back to direct processing")
                use_batch = False
            else:
                # Submit to batcher
                batcher = get_batcher()
                result = await batcher.submit_request(
                    code=code,
                    analysis_type=analysis_type,
                    files=files,
                    sdk_context=sdk_context,
                    brief=brief
                )
        except ImportError as e:
            logger.warning(f"Batch processor not available: {e}, using direct processing")
            use_batch = False

    if not use_batch:
        # Direct processing (fallback or disabled)
        try:
            from src.core.clang_parser import get_clang_parser
            parser = get_clang_parser()
        except ImportError as e:
            logger.error(f"Failed to import Clang parser: {e}")
            return [TextContent(
                type="text",
                text=json.dumps({
                    "success": False,
                    "error": {
                        "code": "MODULE_IMPORT_ERROR",
                        "message": f"Failed to import clang parser: {str(e)}",
                        "solution": "Check that src/core/clang_parser.py exists"
                    }
                }, indent=2, ensure_ascii=False)
            )]

        # Check if Clang is available
        if not parser.is_available():
            error = parser.get_load_error()
            return [TextContent(
                type="text",
                text=json.dumps({
                    "success": False,
                    "error": error.to_dict() if error else {"message": "Clang not available"}
                }, indent=2, ensure_ascii=False)
            )]

        # Perform analysis directly
        result = await parser.analyze(
            code=code,
            analysis_type=analysis_type,
            files=files,
            sdk_context_override=sdk_context,
            brief=brief
        )

    # Format output (for both batch and direct)
    if brief and result.get("success"):
        # Brief output: omit verbose fields
        result = _format_brief_output(result, analysis_type)

    return [TextContent(
        type="text",
        text=json.dumps(result, indent=2, ensure_ascii=False)
    )]


def _format_brief_output(result: dict, analysis_type: str) -> dict:
    """Format brief output by omitting verbose fields"""
    brief_result = {
        "success": result["success"],
        "type": result.get("type", analysis_type),
    }

    if analysis_type == "refs":
        brief_result["symbol"] = result.get("symbol", "")
        brief_result["count"] = result.get("count", 0)

        # Include only first 5 results in brief mode
        results = result.get("results", [])
        brief_result["results"] = results[:5]
        # Always return truncated field as per design spec
        brief_result["truncated"] = len(results) > 5
        if brief_result["truncated"]:
            brief_result["total"] = len(results)

    elif analysis_type == "type":
        brief_result["result"] = result.get("result", {})

    elif analysis_type == "check":
        brief_result["summary"] = result.get("summary", {})
        # Only include errors and warnings in brief mode
        diagnostics = result.get("diagnostics", [])
        brief_result["diagnostics"] = [
            d for d in diagnostics
            if d.get("severity") in ("error", "warning")
        ]

    # Include metadata
    if "metadata" in result:
        brief_result["metadata"] = result["metadata"]

    return brief_result


# ============================================================================
# Helper Functions
# ============================================================================

def get_analysis_type_help() -> Dict[str, str]:
    """Get help text for each analysis type"""
    return ANALYSIS_TYPE_DESCRIPTIONS.copy()


def validate_analysis_request(arguments: dict) -> tuple:
    """
    Validate analysis request before processing.

    Returns:
        (is_valid, error_message) tuple
    """
    code = arguments.get("code", "")
    if not code:
        return False, "Missing required parameter: code"

    if not isinstance(code, str):
        return False, "Parameter 'code' must be a string"

    MAX_CODE_LENGTH = 2000
    if len(code) > MAX_CODE_LENGTH:
        return False, f"Code exceeds maximum length of {MAX_CODE_LENGTH} characters (INPUT_TOO_LARGE)"

    analysis_type = arguments.get("type", "refs")
    valid_types = ["refs", "type", "check"]
    if analysis_type not in valid_types:
        return False, f"Invalid analysis type '{analysis_type}'. Valid types: {valid_types}"

    files = arguments.get("files", [])
    if not isinstance(files, list):
        return False, "Parameter 'files' must be an array"

    if len(files) > 3:
        return False, "Maximum 3 context files allowed"

    return True, None


# ============================================================================
# ClangToolsHandler Class (for testing)
# ============================================================================

class ClangToolsHandler:
    """
    Handler class for clang analysis tools.

    This class provides a simplified interface for testing purposes,
    wrapping the ClangParser functionality.
    """

    def __init__(self):
        """Initialize handler with clang parser."""
        from src.core.clang_parser import get_clang_parser
        self._parser = get_clang_parser()

    def is_available(self) -> bool:
        """Check if clang is available."""
        return self._parser.is_available()

    def get_load_error(self):
        """Get load error if clang is not available."""
        return self._parser.get_load_error()

    async def analyze(
        self,
        code: str,
        analysis_type: str,
        files: Optional[List[str]] = None,
        sdk_context: Optional[Dict[str, Any]] = None,
        brief: bool = True
    ) -> Dict[str, Any]:
        """
        Perform clang analysis.

        Args:
            code: C code snippet or symbol name to analyze
            analysis_type: Analysis type - "refs", "type", or "check"
            files: Optional context files (max 3)
            sdk_context: Optional SDK context override
            brief: Return brief output (default: True)

        Returns:
            Analysis result dictionary
        """
        return await self._parser.analyze(
            code=code,
            analysis_type=analysis_type,
            files=files,
            sdk_context_override=sdk_context,
            brief=brief
        )


# ============================================================================
# Exports
# ============================================================================

__all__ = [
    "CLANG_ANALYZE_TOOL_DEFINITION",
    "handle_clang_analyze",
    "get_analysis_type_help",
    "validate_analysis_request",
    "ANALYSIS_TYPE_DESCRIPTIONS",
    "ClangToolsHandler",
]
