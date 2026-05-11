"""
MCP Tools: Hardware Constraint Validator
=========================================

Provides AI with ability to validate hardware configurations against Excel-derived constraints.

Tools:
1. validate_pin_config - Validate pin mux configuration
2. check_flash_usage - Check Flash memory usage
3. check_sram_allocation - Check SRAM memory allocation
4. estimate_battery_life - Estimate battery life from power consumption
5. check_phone_compatibility - Check phone BLE compatibility issues
6. get_ble_feature_compatibility - Get BLE feature compatibility
7. check_memory_boundaries - Check memory boundary constraints

This is Module B - Domain-Specific AI Tools
Author: B6x MCP Server Team
Version: 1.0.0
"""

import json
import logging
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

# Add parent directory to path
import sys
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

logger = logging.getLogger(__name__)

# Global cache for constraint data
_constraint_cache: Dict[str, dict] = {}


def load_constraint(constraint_name: str) -> Optional[dict]:
    """
    Load constraint JSON file with caching.

    Args:
        constraint_name: Name of constraint file (e.g., "io_map.json")

    Returns:
        Parsed JSON data or None if file not found
    """
    if constraint_name in _constraint_cache:
        return _constraint_cache[constraint_name]

    constraint_path = Path(__file__).parent.parent.parent.parent / "data" / "constraints" / constraint_name

    if not constraint_path.exists():
        logger.warning(f"Constraint file not found: {constraint_path}")
        return None

    try:
        with open(constraint_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
            _constraint_cache[constraint_name] = data
            logger.info(f"Loaded constraint: {constraint_name}")
            return data
    except Exception as e:
        logger.error(f"Error loading constraint {constraint_name}: {e}")
        return None


# ============================================================================
# Tool 1: Validate Pin Configuration
# ============================================================================

def validate_pin_config(pin_config: Dict[str, str]) -> Dict:
    """
    Validate pin configuration against hardware constraints.

    Args:
        pin_config: Dictionary mapping pin names to requested functions
                   e.g., {"PA0": "UART1_TX", "PA1": "UART1_RX"}

    Returns:
        {
            "valid": false,
            "total_pins": 2,
            "valid_pins": 1,
            "invalid_pins": 1,
            "conflicts": [
                {
                    "pin": "PA1",
                    "requested": "UART1_RX",
                    "available_functions": ["GPIO", "SWDIO"],
                    "suggestion": "Use PA3 for UART1_RX"
                }
            ],
            "warnings": [
                {
                    "pin": "PA0",
                    "type": "default_override",
                    "message": "PA0 default function is SWCLK, will be overridden"
                }
            ]
        }

    Example:
        >>> validate_pin_config({"PA0": "UART1_TX", "PA1": "UART1_RX"})
        Validates if PA0 and PA1 support UART1 functions
    """
    logger.info(f"Validating pin config: {pin_config}")

    io_map = load_constraint("io_map.json")
    if not io_map:
        return {
            "valid": False,
            "error": "IO map constraints not loaded. Run build_excel_constraints.py first."
        }

    conflicts = []
    warnings = []
    valid_pins = 0
    invalid_pins = 0

    # Build pin lookup
    pins_dict = {pin["pin_name"]: pin for pin in io_map.get("pins", [])}

    for pin_name, requested_function in pin_config.items():
        if pin_name not in pins_dict:
            conflicts.append({
                "pin": pin_name,
                "requested": requested_function,
                "available_functions": [],
                "suggestion": f"Pin {pin_name} not found in IO map"
            })
            invalid_pins += 1
            continue

        pin_data = pins_dict[pin_name]
        available_functions = [f["name"] for f in pin_data.get("functions", [])]

        # Check if requested function exists
        function_exists = False
        for func in pin_data.get("functions", []):
            if func["name"] == requested_function or func["peripheral"].upper() in requested_function.upper():
                function_exists = True

                # Check if overriding default
                if func.get("is_default") == False and pin_data.get("default_function"):
                    warnings.append({
                        "pin": pin_name,
                        "type": "default_override",
                        "message": f"{pin_name} default function is {pin_data['default_function']}, will be overridden to {requested_function}"
                    })
                break

        if not function_exists:
            conflicts.append({
                "pin": pin_name,
                "requested": requested_function,
                "available_functions": available_functions,
                "suggestion": f"Pin {pin_name} does not support {requested_function}. Available: {', '.join(available_functions[:5])}"
            })
            invalid_pins += 1
        else:
            valid_pins += 1

    return {
        "valid": len(conflicts) == 0,
        "total_pins": len(pin_config),
        "valid_pins": valid_pins,
        "invalid_pins": invalid_pins,
        "conflicts": conflicts,
        "warnings": warnings
    }


def get_available_pins_for_function(function_pattern: str) -> Dict:
    """
    Get all pins that support a specific function.

    Args:
        function_pattern: Function name or pattern (e.g., "UART1", "SPI0_SCK")

    Returns:
        {
            "function": "UART1",
            "total_pins": 4,
            "pins": [
                {"pin": "PA0", "function": "UART1_TX"},
                {"pin": "PA1", "function": "UART1_RX"},
                {"pin": "PA3", "function": "UART1_TX"},
                {"pin": "PA4", "function": "UART1_RX"}
            ]
        }
    """
    logger.info(f"Getting pins for function: {function_pattern}")

    io_map = load_constraint("io_map.json")
    if not io_map:
        return {"error": "IO map constraints not loaded"}

    matching_pins = []
    pattern_upper = function_pattern.upper()

    for pin_data in io_map.get("pins", []):
        for func in pin_data.get("functions", []):
            func_name = func["name"]
            func_periph = func["peripheral"].upper()

            # Match by function name or peripheral
            if (pattern_upper in func_name or
                pattern_upper in func_periph or
                func_name.startswith(pattern_upper)):

                matching_pins.append({
                    "pin": pin_data["pin_name"],
                    "function": func_name,
                    "peripheral": func_periph,
                    "direction": func.get("direction", ""),
                    "is_default": func.get("is_default", False)
                })
                break

    return {
        "function": function_pattern,
        "total_pins": len(matching_pins),
        "pins": matching_pins
    }


# ============================================================================
# Tool 2: Check Flash Usage
# ============================================================================

def check_flash_usage(code_size_kb: float, data_size_kb: float = 0) -> Dict:
    """
    Check if Flash usage exceeds available Flash memory.

    Args:
        code_size_kb: Code size in KB
        data_size_kb: Data size in KB (optional, defaults to 0)

    Returns:
        {
            "total_used_kb": 128.5,
            "code_size_kb": 128.0,
            "data_size_kb": 0.5,
            "total_flash_kb": 256,
            "usage_percentage": 50.2,
            "remaining_kb": 127.5,
            "exceeded": false,
            "regions": [
                {
                    "name": "FLASH_CODE",
                    "size_kb": 256,
                    "is_xip": true,
                    "suitable_for_code": true
                }
            ]
        }

    Example:
        >>> check_flash_usage(128, 0.5)
        Checks if 128.5 KB fits in available Flash
    """
    logger.info(f"Checking flash usage: code={code_size_kb}KB, data={data_size_kb}KB")

    flash_map = load_constraint("flash_map.json")
    if not flash_map:
        return {
            "error": "Flash map constraints not loaded. Run build_excel_constraints.py first."
        }

    total_used = code_size_kb + data_size_kb
    total_flash = sum(r["size_kb"] for r in flash_map.get("regions", []))
    usage_pct = (total_used / total_flash * 100) if total_flash > 0 else 0

    return {
        "total_used_kb": round(total_used, 2),
        "code_size_kb": code_size_kb,
        "data_size_kb": data_size_kb,
        "total_flash_kb": total_flash,
        "usage_percentage": round(usage_pct, 1),
        "remaining_kb": round(total_flash - total_used, 2),
        "exceeded": total_used > total_flash,
        "regions": [
            {
                "name": r["name"],
                "size_kb": r["size_kb"],
                "is_xip": r.get("is_xip", False),
                "suitable_for_code": r.get("access_type") in ["READ", "XIP", "READ_ONLY"]
            }
            for r in flash_map.get("regions", [])
        ]
    }


# ============================================================================
# Tool 3: Check SRAM Allocation
# ============================================================================

def check_sram_allocation(library_type: str, requested_regions: List[Dict[str, any]]) -> Dict:
    """
    Check if SRAM allocation is valid for a BLE library type.

    Args:
        library_type: BLE library type ("lite", "standard", "full")
        requested_regions: List of requested memory regions
                         e.g., [{"name": "RAM0", "size_kb": 8}]

    Returns:
        {
            "library_type": "lite",
            "total_requested_kb": 8,
            "total_available_kb": 32,
            "usage_percentage": 25.0,
            "regions_valid": true,
            "region_details": [
                {
                    "name": "RAM0",
                    "requested_kb": 8,
                    "available_kb": 8,
                    "start_address": "0x20000000",
                    "valid": true
                }
            ],
            "warnings": [],
            "errors": []
        }

    Example:
        >>> check_sram_allocation("lite", [{"name": "RAM0", "size_kb": 6}])
        Validates SRAM allocation for BLE lite library
    """
    logger.info(f"Checking SRAM allocation for {library_type} library")

    sram_map = load_constraint("sram_map.json")
    if not sram_map:
        return {
            "error": "SRAM map constraints not loaded. Run build_excel_constraints.py first."
        }

    # Build region lookup
    regions_dict = {r["name"]: r for r in sram_map.get("regions", [])}

    total_requested = sum(r.get("size_kb", 0) for r in requested_regions)
    total_available = sum(r["size_kb"] for r in regions_dict.values())

    warnings = []
    errors = []
    region_details = []

    for req_region in requested_regions:
        region_name = req_region.get("name")
        requested_kb = req_region.get("size_kb", 0)

        if region_name not in regions_dict:
            errors.append(f"Region {region_name} not found in SRAM map")
            region_details.append({
                "name": region_name,
                "requested_kb": requested_kb,
                "valid": False,
                "error": "Region not found"
            })
            continue

        avail_region = regions_dict[region_name]
        available_kb = avail_region["size_kb"]

        # Check if region matches library type
        region_lib_type = avail_region.get("library_type", "")
        if region_lib_type and library_type.lower() not in region_lib_type.lower():
            warnings.append(
                f"Region {region_name} is optimized for {region_lib_type} library, "
                f"not {library_type}"
            )

        region_details.append({
            "name": region_name,
            "requested_kb": requested_kb,
            "available_kb": available_kb,
            "start_address": avail_region.get("start_address", ""),
            "end_address": avail_region.get("end_address", ""),
            "valid": requested_kb <= available_kb,
            "description": avail_region.get("description", "")
        })

        if requested_kb > available_kb:
            errors.append(
                f"Region {region_name}: requested {requested_kb}KB exceeds available {available_kb}KB"
            )

    return {
        "library_type": library_type,
        "total_requested_kb": total_requested,
        "total_available_kb": total_available,
        "usage_percentage": round((total_requested / total_available * 100) if total_available > 0 else 0, 1),
        "regions_valid": len(errors) == 0,
        "region_details": region_details,
        "warnings": warnings,
        "errors": errors
    }


# ============================================================================
# Tool 4: Estimate Battery Life
# ============================================================================

def estimate_battery_life(
    mode: str,
    peripherals: List[str],
    battery_capacity_mah: float = 200,
    duty_cycle_percent: float = 100.0
) -> Dict:
    """
    Estimate battery life based on power consumption constraints.

    Args:
        mode: Power mode ("SLEEP", "IDLE", "ACTIVE", "TX", "RX")
        peripherals: List of active peripherals (e.g., ["BLE", "UART"])
        battery_capacity_mah: Battery capacity in mAh (default: 200)
        duty_cycle_percent: Duty cycle percentage (default: 100 = always on)

    Returns:
        {
            "mode": "SLEEP",
            "peripherals": ["BLE"],
            "battery_capacity_mah": 200,
            "total_current_ua": 2.01,
            "average_power_mw": 6.03,
            "duty_cycle_percent": 100,
            "estimated_battery_hours": 99502.5,
            "estimated_battery_days": 4145.9,
            "current_breakdown": [
                {
                    "peripheral": "BLE",
                    "current_ua": 2.01,
                    "conditions": "interval: 1000ms; note: 未断开连接"
                }
            ]
        }

    Example:
        >>> estimate_battery_life("SLEEP", ["BLE"], 200, 100)
        Estimates battery life for BLE sleep mode
    """
    logger.info(f"Estimating battery life: mode={mode}, peripherals={peripherals}")

    power_data = load_constraint("power_consumption.json")
    if not power_data:
        return {
            "error": "Power consumption constraints not loaded. Run build_excel_constraints.py first."
        }

    # Find mode data
    mode_data = None
    for m in power_data.get("modes", []):
        if m["mode"].upper() == mode.upper():
            mode_data = m
            break

    if not mode_data:
        return {
            "error": f"Power mode '{mode}' not found. Available: {[m['mode'] for m in power_data['modes']]}"
        }

    # Calculate total current for requested peripherals
    total_current_ua = 0
    current_breakdown = []

    for periph in peripherals:
        # Find measurements for this peripheral
        for meas in mode_data.get("measurements", []):
            if meas["peripheral"].upper() == periph.upper():
                current_ua = meas.get("current_ua", 0)
                total_current_ua += current_ua
                current_breakdown.append({
                    "peripheral": periph,
                    "current_ua": current_ua,
                    "voltage_mv": meas.get("voltage_mv", 3000),
                    "power_mw": meas.get("power_mw", 0),
                    "conditions": meas.get("conditions", "")
                })

    if total_current_ua == 0:
        return {
            "warning": f"No power data found for peripherals: {peripherals}",
            "available_peripherals": list(set(m["peripheral"] for m in mode_data["measurements"]))
        }

    # Apply duty cycle
    avg_current_ua = total_current_ua * (duty_cycle_percent / 100.0)

    # Calculate battery life
    # Battery life (hours) = Capacity (mAh) / Average current (mA)
    avg_current_ma = avg_current_ua / 1000.0
    battery_hours = battery_capacity_mah / avg_current_ma if avg_current_ma > 0 else 0

    return {
        "mode": mode,
        "peripherals": peripherals,
        "battery_capacity_mah": battery_capacity_mah,
        "total_current_ua": round(total_current_ua, 2),
        "average_current_ua": round(avg_current_ua, 2),
        "average_power_mw": round(avg_current_ua * 3.0 / 1000, 2),  # Assuming 3V
        "duty_cycle_percent": duty_cycle_percent,
        "estimated_battery_hours": round(battery_hours, 1),
        "estimated_battery_days": round(battery_hours / 24, 1),
        "current_breakdown": current_breakdown
    }


# ============================================================================
# Tool 5: Check Phone Compatibility
# ============================================================================

def check_phone_compatibility(phone_brand: str, phone_model: Optional[str] = None) -> Dict:
    """
    Check for known BLE compatibility issues with specific phone brands/models.

    Args:
        phone_brand: Phone brand name (e.g., "苹果", "华为", "小米")
        phone_model: Optional phone model (e.g., "iPhone 15")

    Returns:
        {
            "phone_brand": "苹果",
            "phone_model": "iPhone 15",
            "has_known_issues": true,
            "total_issues": 5,
            "issues": [
                {
                    "phone_model": "iPhone 15",
                    "os_version": "IOS17.1",
                    "issue_type": "兼容性问题",
                    "description": "IOS17.1",
                    "severity": "medium",
                    "workaround": "",
                    "solution": ""
                }
            ],
            "severity_breakdown": {
                "critical": 0,
                "high": 0,
                "medium": 5,
                "low": 0
            }
        }

    Example:
        >>> check_phone_compatibility("苹果", "iPhone 15")
        Returns known BLE issues for iPhone 15
    """
    logger.info(f"Checking phone compatibility: {phone_brand} - {phone_model}")

    phone_data = load_constraint("phone_compatibility_issues.json")
    if not phone_data:
        return {
            "error": "Phone compatibility constraints not loaded. Run build_excel_constraints.py first."
        }

    # Find issues for this brand
    brand_issues = phone_data.get("brands", {}).get(phone_brand, [])

    # Filter by model if specified
    if phone_model:
        filtered_issues = [
            issue for issue in brand_issues
            if phone_model.lower() in issue.get("phone_model", "").lower() or
               phone_model.lower() in issue.get("os_version", "").lower()
        ]
    else:
        filtered_issues = brand_issues

    # Count by severity
    severity_counts = {"critical": 0, "high": 0, "medium": 0, "low": 0}
    for issue in filtered_issues:
        severity = issue.get("severity", "low")
        severity_counts[severity] = severity_counts.get(severity, 0) + 1

    return {
        "phone_brand": phone_brand,
        "phone_model": phone_model or "All models",
        "has_known_issues": len(filtered_issues) > 0,
        "total_issues": len(filtered_issues),
        "issues": filtered_issues,
        "severity_breakdown": severity_counts,
        "total_brands_in_db": phone_data.get("total_brands", 0),
        "total_issues_in_db": phone_data.get("total_issues", 0)
    }


def list_phone_brands() -> Dict:
    """
    List all phone brands with known compatibility issues.

    Returns:
        {
            "total_brands": 11,
            "total_issues": 147,
            "brands": [
                {
                    "name": "苹果",
                    "issue_count": 5,
                    "severity_breakdown": {"critical": 0, "high": 0, "medium": 5, "low": 0}
                },
                ...
            ]
        }
    """
    logger.info("Listing all phone brands with compatibility issues")

    phone_data = load_constraint("phone_compatibility_issues.json")
    if not phone_data:
        return {"error": "Phone compatibility constraints not loaded"}

    brands_summary = []
    for brand, issues in phone_data.get("brands", {}).items():
        severity_counts = {"critical": 0, "high": 0, "medium": 0, "low": 0}
        for issue in issues:
            severity = issue.get("severity", "low")
            severity_counts[severity] = severity_counts.get(severity, 0) + 1

        brands_summary.append({
            "name": brand,
            "issue_count": len(issues),
            "severity_breakdown": severity_counts
        })

    return {
        "total_brands": phone_data.get("total_brands", 0),
        "total_issues": phone_data.get("total_issues", 0),
        "brands": sorted(brands_summary, key=lambda x: x["issue_count"], reverse=True)
    }


# ============================================================================
# Tool 6: Get BLE Feature Compatibility
# ============================================================================

def get_ble_feature_compatibility(feature_name: Optional[str] = None) -> Dict:
    """
    Get BLE phone compatibility information.

    Args:
        feature_name: Optional brand name to filter (e.g., "苹果", "华为", "Samsung")

    Returns:
        {
            "total_brands": 11,
            "total_issues": 147,
            "matched_brands": {
                "brand_name": [
                    {"phone_model": "...", "os_version": "...", "severity": "...", ...}
                ]
            }
        }
    """
    logger.info(f"Getting BLE feature compatibility: {feature_name}")

    compat_data = load_constraint("phone_compatibility_issues.json")
    if not compat_data:
        return {
            "error": "Phone compatibility constraints not loaded. Run build_excel_constraints.py first."
        }

    brands = compat_data.get("brands", {})
    total_issues = compat_data.get("total_issues", 0)

    if feature_name:
        filtered_brands = {
            brand: issues
            for brand, issues in brands.items()
            if feature_name.lower() in brand.lower() or
               brand.lower() in feature_name.lower()
        }
    else:
        filtered_brands = brands

    return {
        "total_brands": len(brands),
        "total_issues": total_issues,
        "matched_brands": filtered_brands,
        "matched_brand_count": len(filtered_brands)
    }


# ============================================================================
# Tool 7: Check Memory Boundaries
# ============================================================================

def check_memory_boundaries(library_type: str) -> Dict:
    """
    Get memory boundary constraints for a BLE library type.

    Args:
        library_type: BLE library type ("lite", "standard", "full")

    Returns:
        {
            "library_type": "lite",
            "total_boundaries": 8,
            "total_size_kb": 32,
            "boundaries": [
                {
                    "region_name": "RAM0",
                    "start_address": "0x20000000",
                    "end_address": "0x20002000",
                    "size_kb": 8,
                    "boundary_type": "sram",
                    "library_type": "lite",
                    "access_restrictions": "BLE stack only",
                    "alignment_requirement": "4-byte aligned"
                }
            ],
            "warnings": []
        }

    Example:
        >>> check_memory_boundaries("lite")
        Returns memory boundaries for BLE lite library
    """
    logger.info(f"Checking memory boundaries for {library_type} library")

    boundary_data = load_constraint("memory_boundaries.json")
    if not boundary_data:
        return {
            "error": "Memory boundary constraints not loaded. Run build_excel_constraints.py first."
        }

    # Filter by library type
    boundaries = [
        b for b in boundary_data.get("boundaries", [])
        if b.get("library_type", "").lower() == library_type.lower()
    ]

    # Collect warnings
    warnings = []
    for b in boundaries:
        if b.get("overlap_warnings"):
            warnings.append({
                "region": b["region_name"],
                "warnings": b["overlap_warnings"]
            })

    return {
        "library_type": library_type,
        "total_boundaries": len(boundaries),
        "total_size_kb": sum(b["size_kb"] for b in boundaries),
        "boundaries": boundaries,
        "warnings": warnings
    }


# ============================================================================
# Tool Registry
# ============================================================================

# Export all tools for MCP registration
ALL_TOOLS = [
    # Pin configuration tools
    ("validate-pin-config", validate_pin_config),
    ("get-available-pins-for-function", get_available_pins_for_function),

    # Memory tools
    ("check-flash-usage", check_flash_usage),
    ("check-sram-allocation", check_sram_allocation),
    ("check-memory-boundaries", check_memory_boundaries),

    # Power tools
    ("estimate-battery-life", estimate_battery_life),

    # Compatibility tools
    ("check-phone-compatibility", check_phone_compatibility),
    ("list-phone-brands", list_phone_brands),
    ("get-ble-feature-compatibility", get_ble_feature_compatibility),
]


if __name__ == "__main__":
    # Test tools
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    print("="*70)
    print("Hardware Constraint Tools - Test")
    print("="*70)

    # Test 1: Validate pin config
    print("\n[Test 1] Validate Pin Configuration")
    result = validate_pin_config({"PA0": "UART1_TX"})
    print(f"Valid: {result.get('valid')}")
    print(f"Conflicts: {len(result.get('conflicts', []))}")

    # Test 2: Check flash usage
    print("\n[Test 2] Check Flash Usage")
    result = check_flash_usage(128, 0.5)
    print(f"Usage: {result.get('usage_percentage')}%")
    print(f"Exceeded: {result.get('exceeded')}")

    # Test 3: Estimate battery life
    print("\n[Test 3] Estimate Battery Life")
    result = estimate_battery_life("SLEEP", ["BLE"], 200, 100)
    print(f"Estimated battery life: {result.get('estimated_battery_days')} days")

    # Test 4: Check phone compatibility
    print("\n[Test 4] Check Phone Compatibility")
    result = check_phone_compatibility("苹果")
    print(f"Brand: {result.get('phone_brand')}")
    print(f"Total issues: {result.get('total_issues')}")
