"""
MCP Tools: BLE Error Code Analyzer
==================================

Provides AI with ability to:
1. Translate BLE error codes to human-readable descriptions
2. Provide troubleshooting guidance
3. Analyze log files for error patterns

This is Module B - Domain-Specific AI Tools
"""

import json
import logging
from pathlib import Path
from typing import Dict, List, Optional

# Add parent directory to path
import sys
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from core.ble_error_parser import BleErrorKnowledgeBase, BleErrorAssistant

logger = logging.getLogger(__name__)

# Global knowledge base instance
_kb_instance = None


def get_kb():
    """Get or create knowledge base instance."""
    global _kb_instance
    if _kb_instance is None:
        # Try to load from JSON if exists, otherwise create new
        kb_path = Path(__file__).parent.parent.parent.parent / "data" / "ble_error_codes.json"
        if kb_path.exists():
            logger.info(f"Loading BLE error codes from {kb_path}")
            _kb_instance = BleErrorKnowledgeBase()
        else:
            logger.info("Creating new BLE error knowledge base")
            _kb_instance = BleErrorKnowledgeBase()
            # Export for future use
            kb_path.parent.mkdir(parents=True, exist_ok=True)
            _kb_instance.export_to_json(str(kb_path))
    return _kb_instance


# ========================================================================
# MCP Tool Definitions
# ========================================================================

def translate_ble_error(error_code: int) -> Dict:
    """
    Translate a BLE error code to human-readable description.

    Args:
        error_code: Error code as integer (e.g., 0x40 or 64)

    Returns:
        Dictionary with error information including:
        - code: Error code in hex
        - name: Error name (e.g., GAP_ERR_INVALID_PARAM)
        - category: Protocol layer (GAP, ATT, L2C, etc.)
        - description: Human-readable description
        - common_causes: List of common causes
        - troubleshooting: List of troubleshooting steps
        - related_apis: List of APIs that may return this error (NEW)
        - api_checklist: API-level troubleshooting checklist (NEW)

    Example:
        >>> translate_ble_error(0x40)
        {
            "code": "0x40",
            "name": "GAP_ERR_INVALID_PARAM",
            "category": "GAP",
            "description": "Invalid parameters set",
            "common_causes": ["Check all parameters in API call", ...],
            "troubleshooting": ["Verify parameter ranges", ...],
            "related_apis": [{"name": "gapc_connect_req", "condition": "...", "probability": "high"}, ...],
            "api_checklist": ["[ ] Check gapc_connect_req call: ...", ...]
        }
    """
    kb = get_kb()
    error = kb.lookup_error(error_code)

    if not error:
        return {
            "code": f"0x{error_code:02X}",
            "name": "UNKNOWN_ERROR",
            "category": "Unknown",
            "description": f"Error code 0x{error_code:02X} not found in knowledge base",
            "common_causes": [
                "Custom application error (check your code)",
                "New error code in updated SDK",
                "Invalid error code"
            ],
            "troubleshooting": [
                "Search SDK documentation for this error code",
                "Check ble/api/le_err.h for new error definitions",
                "Review application code for custom errors"
            ],
            "related_apis": [],
            "api_checklist": []
        }

    # Get related APIs from error object
    related_apis = error.related_apis if hasattr(error, 'related_apis') else []

    # Generate API-level troubleshooting checklist
    api_checklist = _generate_api_checklist(related_apis)

    return {
        "code": f"0x{error.code:02X}",
        "name": error.name,
        "category": error.category,
        "description": error.description,
        "common_causes": error.common_causes,
        "troubleshooting": error.troubleshooting if error.troubleshooting else error.common_causes,
        "related_apis": related_apis,
        "api_checklist": api_checklist
    }


def _generate_api_checklist(related_apis: List[Dict]) -> List[str]:
    """
    Generate an API-level troubleshooting checklist.

    Args:
        related_apis: List of related API information

    Returns:
        List of checklist items for API investigation
    """
    checklist = []

    if not related_apis:
        return checklist

    # Sort by probability (high first)
    prob_order = {"high": 0, "medium": 1, "low": 2}
    sorted_apis = sorted(
        related_apis,
        key=lambda x: prob_order.get(x.get("probability", "medium"), 1)
    )

    for api in sorted_apis:
        api_name = api.get("name", "unknown_api")
        condition = api.get("condition", "")
        probability = api.get("probability", "medium")

        # Format checklist item
        checklist_item = f"[ ] Check {api_name}()"

        if condition:
            checklist_item += f": {condition}"

        checklist.append(checklist_item)

    return checklist


def analyze_ble_log(log_text: str) -> Dict:
    """
    Analyze a BLE log and extract error codes with analysis.

    Args:
        log_text: Log text containing error codes

    Returns:
        Dictionary with:
        - error_count: Number of unique errors found
        - errors: List of error analyses
        - summary: Brief summary of issues

    Example:
        >>> analyze_ble_log("[ERROR] GAP: 0x45\\n[ERROR] ATT: 0x0A")
        {
            "error_count": 2,
            "errors": [...],
            "summary": "Found 2 distinct errors: GAP_ERR_TIMEOUT..."
        }
    """
    kb = get_kb()
    assistant = BleErrorAssistant(kb)

    # Parse log for error codes
    error_codes = set()
    import re
    hex_pattern = r'0[xX]?([0-9A-Fa-f]{2})'
    matches = re.findall(hex_pattern, log_text)

    for match in matches:
        code = int(match, 16)
        # Filter to valid BLE error ranges
        if 0x01 <= code <= 0xFF or 0x91 <= code <= 0xCE:
            error_codes.add(code)

    if not error_codes:
        return {
            "error_count": 0,
            "errors": [],
            "summary": "No BLE error codes found in log"
        }

    # Analyze each error
    errors = []
    for code in sorted(error_codes):
        error_info = translate_ble_error(code)
        errors.append(error_info)

    # Generate summary
    error_names = [e["name"] for e in errors]
    summary = f"Found {len(error_codes)} distinct error(s): {', '.join(error_names[:3])}"
    if len(error_names) > 3:
        summary += f" and {len(error_names) - 3} more"

    return {
        "error_count": len(error_codes),
        "errors": errors,
        "summary": summary
    }


def search_ble_errors(keyword: str) -> Dict:
    """
    Search BLE error codes by keyword.

    Args:
        keyword: Search term (matches against name and description)

    Returns:
        Dictionary with search results

    Example:
        >>> search_ble_errors("timeout")
        {
            "count": 5,
            "errors": [...]
        }
    """
    kb = get_kb()
    errors = kb.search_errors(keyword)

    return {
        "keyword": keyword,
        "count": len(errors),
        "errors": [
            {
                "code": f"0x{e.code:02X}",
                "name": e.name,
                "category": e.category,
                "description": e.description
            }
            for e in errors
        ]
    }


def list_ble_errors_by_category(category: str) -> Dict:
    """
    List all error codes in a specific category.

    Args:
        category: Protocol layer (GAP, ATT, L2C, GATT, SMP, PRF, LL)

    Returns:
        Dictionary with category information and errors

    Example:
        >>> list_ble_errors_by_category("GAP")
        {
            "category": "GAP",
            "count": 14,
            "errors": [...]
        }
    """
    kb = get_kb()
    category = category.upper()
    errors = kb.get_errors_by_category(category)

    return {
        "category": category,
        "count": len(errors),
        "errors": [
            {
                "code": f"0x{e.code:02X}",
                "name": e.name,
                "description": e.description
            }
            for e in errors
        ]
    }


def get_common_ble_errors(top_n: int = 10) -> Dict:
    """
    Get the most common BLE errors with quick fixes.

    Args:
        top_n: Number of top errors to return (default: 10)

    Returns:
        Dictionary with common errors and quick fixes

    Example:
        >>> get_common_ble_errors(5)
        {
            "count": 5,
            "errors": [...]
        }
    """
    # Most common errors based on industry experience
    common_errors = [
        {"code": 0x40, "name": "GAP_ERR_INVALID_PARAM", "quick_fix": "Check API call parameters"},
        {"code": 0x45, "name": "GAP_ERR_TIMEOUT", "quick_fix": "Increase timeout or check peer device"},
        {"code": 0x46, "name": "GAP_ERR_DISCONNECTED", "quick_fix": "Check signal strength and connection"},
        {"code": 0x02, "name": "ATT_ERR_READ_NOT_PERMITTED", "quick_fix": "Check CCCD and enable notifications"},
        {"code": 0x03, "name": "ATT_ERR_WRITE_NOT_PERMITTED", "quick_fix": "Verify write permissions"},
        {"code": 0x05, "name": "ATT_ERR_INSUFF_AUTHEN", "quick_fix": "Authenticate connection"},
        {"code": 0x0A, "name": "ATT_ERR_ATTRIBUTE_NOT_FOUND", "quick_fix": "Run service discovery"},
        {"code": 0x66, "name": "SMP_ERR_LOC_ENC_KEY_SIZE", "quick_fix": "Adjust encryption key size"},
        {"code": 0x95, "name": "LL_ERR_AUTH_FAILURE", "quick_fix": "Verify authentication credentials"},
        {"code": 0x98, "name": "LL_ERR_CON_TIMEOUT", "quick_fix": "Increase supervision timeout"},
    ]

    kb = get_kb()
    results = []

    for err in common_errors[:top_n]:
        error = kb.lookup_error(err["code"])
        if error:
            results.append({
                "code": f"0x{error.code:02X}",
                "name": error.name,
                "category": error.category,
                "description": error.description,
                "quick_fix": err["quick_fix"]
            })

    return {
        "count": len(results),
        "errors": results
    }


def get_ble_error_statistics() -> Dict:
    """
    Get statistics about the BLE error code knowledge base.

    Returns:
        Dictionary with statistics

    Example:
        >>> get_ble_error_statistics()
        {
            "total_errors": 160,
            "categories": {...}
        }
    """
    kb = get_kb()
    categories = ["GAP", "ATT", "L2C", "GATT", "SMP", "PRF", "LL"]

    stats = {
        "total_errors": len(kb.errors),
        "categories": {}
    }

    for cat in categories:
        errors = kb.get_errors_by_category(cat)
        stats["categories"][cat] = {
            "count": len(errors),
            "description": _get_category_description(cat)
        }

    return stats


def _get_category_description(category: str) -> str:
    """Get description for a protocol layer category."""
    descriptions = {
        "GAP": "Generic Access Profile - Connection and advertising",
        "ATT": "Attribute Protocol - Data access and permissions",
        "L2C": "Logical Link Control - Connection-oriented channels",
        "GATT": "Generic Attribute Profile - Service framework",
        "SMP": "Security Manager Protocol - Pairing and encryption",
        "PRF": "Profiles - High-level services (Battery, HID, etc.)",
        "LL": "Link Layer - Radio and hardware control"
    }
    return descriptions.get(category, "Unknown category")


# ========================================================================
# Tool Registration
# ========================================================================

def register_mcp_tools(server):
    """
    Register BLE error analyzer tools with MCP server.

    Args:
        server: MCP server instance

    Returns:
        List of registered tool names
    """
    tools = [
        {
            "name": "translate_ble_error",
            "description": "Translate a BLE error code to human-readable description with troubleshooting steps",
            "parameters": {
                "error_code": {
                    "type": "integer",
                    "description": "Error code (e.g., 0x40 or 64)"
                }
            }
        },
        {
            "name": "analyze_ble_log",
            "description": "Analyze a BLE log and extract error codes with detailed analysis",
            "parameters": {
                "log_text": {
                    "type": "string",
                    "description": "Log text containing error codes"
                }
            }
        },
        {
            "name": "search_ble_errors",
            "description": "Search BLE error codes by keyword",
            "parameters": {
                "keyword": {
                    "type": "string",
                    "description": "Search term (e.g., 'timeout', 'authentication')"
                }
            }
        },
        {
            "name": "list_ble_errors_by_category",
            "description": "List all error codes in a specific protocol layer",
            "parameters": {
                "category": {
                    "type": "string",
                    "description": "Protocol layer: GAP, ATT, L2C, GATT, SMP, PRF, LL"
                }
            }
        },
        {
            "name": "get_common_ble_errors",
            "description": "Get the most common BLE errors with quick fixes",
            "parameters": {
                "top_n": {
                    "type": "integer",
                    "description": "Number of top errors to return (default: 10)",
                    "default": 10
                }
            }
        },
        {
            "name": "get_ble_error_statistics",
            "description": "Get statistics about the BLE error code knowledge base",
            "parameters": {}
        }
    ]

    # Register tools with server
    for tool in tools:
        server.add_tool(tool)

    logger.info(f"Registered {len(tools)} BLE error analyzer tools")

    return [t["name"] for t in tools]


# ========================================================================
# Standalone Testing
# ========================================================================

if __name__ == "__main__":
    import pprint

    print("BLE Error Code Analyzer - MCP Tools")
    print("=" * 70)

    # Test 1: Translate error
    print("\n1. translate_ble_error(0x40):")
    result = translate_ble_error(0x40)
    pprint.pprint(result)

    # Test 2: Analyze log
    print("\n2. analyze_ble_log():")
    log = "[ERROR] GAP: 0x45 timeout\\n[ERROR] ATT: 0x0A not found"
    result = analyze_ble_log(log)
    pprint.pprint(result)

    # Test 3: Search errors
    print("\n3. search_ble_errors('timeout'):")
    result = search_ble_errors("timeout")
    print(f"Found {result['count']} timeout-related errors")

    # Test 4: Statistics
    print("\n4. get_ble_error_statistics():")
    result = get_ble_error_statistics()
    print(f"Total errors: {result['total_errors']}")
    for cat, info in result['categories'].items():
        print(f"  {cat}: {info['count']} errors - {info['description']}")
