#!/usr/bin/env python3
"""
Build Excel Constraints - Convert Excel Specs to JSON
======================================================

Parse all Excel files in doc/SW_Spec/ and convert to JSON constraint files
for AI constraint validation.

Output:
- data/constraints/io_map.json
- data/constraints/flash_map.json
- data/constraints/sram_map.json
- data/constraints/memory_boundaries.json
- data/constraints/power_consumption.json
- data/constraints/phone_compatibility_issues.json

Usage:
    python scripts/build_excel_constraints.py

Author: B6x MCP Server Team
Version: 0.1.0
"""

import sys
import json
import logging
import yaml
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Tuple

# Import enhanced build logger
try:
    from build_logger import BuildLogger
    USE_ENHANCED_LOGGER = True
except ImportError:
    USE_ENHANCED_LOGGER = False
    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )
    logger = logging.getLogger(__name__)

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from core.excel_parser import (
    ExcelHardwareParser,
    PinMuxEntry,
    MemoryRegion,
    PowerEntry,
    SRAMRegion,
    CompatibilityEntry,
    PhoneCompatibilityIssue,
    MemoryBoundary
)
from core.excel_parser_b6x import (
    B6xExcelParser,
    parse_flash_map_b6x,
    parse_sram_allocation_b6x,
    parse_power_consumption_b6x
)
# Import Markdown pipeline modules
from core.excel_to_markdown_converter import ExcelToMarkdownConverter
from core.markdown_table_parser import MarkdownTableParser

logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)


# ============================================================================
# JSON Converters
# ============================================================================

def _get_attr(obj, key, default=None):
    """Safely get attribute from object or dict."""
    if isinstance(obj, dict):
        return obj.get(key, default)
    return getattr(obj, key, default)

def _normalize_pin_name(pin_number: str, gpio_name: str = "") -> str:
    """Normalize pin name to GPIO format (e.g., 'PADA2' -> 'PA02').

    Uses the GPIO function name if available, otherwise converts
    PAD naming convention to GPIO naming convention.

    Args:
        pin_number: Raw pin name from Excel (e.g., 'PADA2', 'PA00')
        gpio_name: GPIO function name (e.g., 'PA02'), takes priority

    Returns:
        Normalized pin name in 'PAxx' format
    """
    if gpio_name:
        return gpio_name

    # Fallback: convert PADA0 -> PA00, PADA2 -> PA02
    import re
    m = re.match(r'PAD([A-Z])(\d+)', pin_number, re.IGNORECASE)
    if m:
        port = m.group(1).upper()
        num = int(m.group(2))
        return f"P{port}{num:02d}"

    return pin_number


def convert_pin_mux_to_json(entries: List[PinMuxEntry]) -> Dict[str, Any]:
    """Convert pin mux entries to JSON constraint format."""
    # Group by pin
    pins_dict = {}

    for entry in entries:
        if entry.pin_number not in pins_dict:
            # Find GPIO function name for canonical pin name
            gpio_name = ""
            if entry.peripheral == "GPIO" and entry.alternate_function:
                gpio_name = entry.alternate_function

            pins_dict[entry.pin_number] = {
                "pin_name": _normalize_pin_name(entry.pin_number, gpio_name),
                "functions": [],
                "default_function": None,
                "peripherals": set(),
                "constraints": []
            }
        else:
            # Update GPIO name if found in later entries
            if entry.peripheral == "GPIO" and entry.alternate_function:
                pins_dict[entry.pin_number]["pin_name"] = _normalize_pin_name(
                    entry.pin_number, entry.alternate_function
                )

        pin_data = pins_dict[entry.pin_number]

        # Add function
        pin_data["functions"].append({
            "name": entry.alternate_function,
            "peripheral": entry.peripheral,
            "direction": entry.direction,
            "is_default": entry.is_default
        })

        if entry.is_default:
            pin_data["default_function"] = entry.alternate_function

        pin_data["peripherals"].add(entry.peripheral)

    # Convert sets to lists
    for pin_data in pins_dict.values():
        pin_data["peripherals"] = list(pin_data["peripherals"])

    return {
        "constraint_type": "pin_mux",
        "version": "1.0",
        "last_updated": datetime.now().isoformat(),
        "total_pins": len(pins_dict),
        "pins": list(pins_dict.values())
    }


def convert_memory_regions_to_json(regions: List[MemoryRegion]) -> Dict[str, Any]:
    """Convert memory regions to JSON constraint format."""
    return {
        "constraint_type": "memory_layout",
        "version": "1.0",
        "last_updated": datetime.now().isoformat(),
        "total_regions": len(regions),
        "regions": [
            {
                "name": _get_attr(r, 'region_name'),
                "base_address": _get_attr(r, 'base_address'),
                "size_bytes": _get_attr(r, 'size', _get_attr(r, 'size_bytes', 0)),
                "size_kb": _get_attr(r, 'size_kb', 0),
                "size_human": f"{_get_attr(r, 'size_kb', 0)}KB" if _get_attr(r, 'size_kb', 0) < 1024 else f"{_get_attr(r, 'size_kb', 0)/1024:.1f}MB",
                "is_xip": _get_attr(r, 'is_xip', False),
                "access_type": _get_attr(r, 'access_type', 'UNKNOWN'),
                "description": _get_attr(r, 'description', '')
            }
            for r in regions
        ]
    }


def convert_power_entries_to_json(entries: List[PowerEntry]) -> Dict[str, Any]:
    """Convert power entries to JSON constraint format."""
    # Group by mode
    modes_dict = {}

    for entry in entries:
        if entry.mode not in modes_dict:
            modes_dict[entry.mode] = {
                "mode": entry.mode,
                "peripherals": [],
                "measurements": []
            }

        modes_dict[entry.mode]["peripherals"].append(entry.peripheral)
        modes_dict[entry.mode]["measurements"].append({
            "peripheral": entry.peripheral,
            "current_ua": entry.current_ua,
            "voltage_mv": entry.voltage_mv,
            "power_mw": entry.power_mw,
            "conditions": entry.conditions
        })

    return {
        "constraint_type": "power_consumption",
        "version": "1.0",
        "last_updated": datetime.now().isoformat(),
        "total_modes": len(modes_dict),
        "modes": list(modes_dict.values())
    }


def convert_sram_regions_to_json(regions: List[SRAMRegion]) -> Dict[str, Any]:
    """Convert SRAM regions to JSON constraint format."""
    return {
        "constraint_type": "sram_allocation",
        "version": "1.0",
        "last_updated": datetime.now().isoformat(),
        "total_regions": len(regions),
        "total_size_kb": sum(_get_attr(r, 'size_kb', 0) for r in regions),
        "regions": [
            {
                "name": _get_attr(r, 'region_name'),
                "start_address": _get_attr(r, 'start_address'),
                "end_address": _get_attr(r, 'end_address'),
                "size_bytes": _get_attr(r, 'size_bytes', 0),
                "size_kb": _get_attr(r, 'size_kb', 0),
                "description": _get_attr(r, 'description', ''),
                "library_type": _get_attr(r, 'library_type', '')
            }
            for r in regions
        ]
    }


def convert_compatibility_to_json(entries: List[CompatibilityEntry]) -> Dict[str, Any]:
    """Convert compatibility entries to JSON constraint format."""
    return {
        "constraint_type": "ble_compatibility",
        "version": "1.0",
        "last_updated": datetime.now().isoformat(),
        "total_features": len(entries),
        "features": [
            {
                "name": _get_attr(e, 'feature_name'),
                "supported_libraries": _get_attr(e, 'supported_libs', []),
                "min_sdk_version": _get_attr(e, 'min_sdk_version', ''),
                "max_sdk_version": _get_attr(e, 'max_sdk_version', ''),
                "notes": _get_attr(e, 'notes', ''),
                "constraints": _get_attr(e, 'constraints', [])
            }
            for e in entries
        ]
    }


def convert_phone_issues_to_json(issues: List[PhoneCompatibilityIssue]) -> Dict[str, Any]:
    """Convert phone compatibility issues to JSON constraint format (Domain 3: BLE Stack)."""
    # Group by phone brand
    by_brand = {}
    for issue in issues:
        brand = issue.phone_brand or "Unknown"
        if brand not in by_brand:
            by_brand[brand] = []
        by_brand[brand].append({
            "phone_model": issue.phone_model,
            "os_version": issue.android_version,
            "issue_type": issue.issue_type,
            "description": issue.issue_description,
            "root_cause": issue.root_cause,
            "workaround": issue.workaround,
            "solution": issue.solution,
            "affected_ble_version": issue.affected_ble_version,
            "sdk_version_required": issue.sdk_version_required,
            "severity": issue.severity
        })

    return {
        "constraint_type": "phone_compatibility_issues",
        "domain": "domain3",  # BLE Stack
        "version": "1.0",
        "last_updated": datetime.now().isoformat(),
        "total_issues": len(issues),
        "total_brands": len(by_brand),
        "brands": by_brand,
        "issues_by_severity": {
            "critical": len([i for i in issues if i.severity == "critical"]),
            "high": len([i for i in issues if i.severity == "high"]),
            "medium": len([i for i in issues if i.severity == "medium"]),
            "low": len([i for i in issues if i.severity == "low"])
        }
    }


def convert_memory_boundaries_to_json(boundaries: List[MemoryBoundary]) -> Dict[str, Any]:
    """Convert memory boundaries to JSON constraint format (Domain 1: Hardware)."""
    return {
        "constraint_type": "memory_boundaries",
        "domain": "domain1",  # Hardware & Registers
        "version": "1.0",
        "last_updated": datetime.now().isoformat(),
        "total_boundaries": len(boundaries),
        "total_size_kb": sum(_get_attr(b, 'size_kb', 0) for b in boundaries),
        "boundaries": [
            {
                "region_name": _get_attr(b, 'region_name'),
                "start_address": _get_attr(b, 'start_address'),
                "end_address": _get_attr(b, 'end_address'),
                "size_bytes": _get_attr(b, 'size_bytes', 0),
                "size_kb": _get_attr(b, 'size_kb', 0),
                "size_human": f"{_get_attr(b, 'size_kb', 0)}KB" if _get_attr(b, 'size_kb', 0) < 1024 else f"{_get_attr(b, 'size_kb', 0)/1024:.1f}MB",
                "boundary_type": _get_attr(b, 'boundary_type', 'unknown'),
                "library_type": _get_attr(b, 'library_type', ''),
                "description": _get_attr(b, 'description', ''),
                "access_restrictions": _get_attr(b, 'access_restrictions', []),
                "alignment_requirement": _get_attr(b, 'alignment_requirement', 0)
            }
            for b in boundaries
        ],
        "boundary_warnings": [
            {
                "region": _get_attr(b, 'region_name'),
                "warnings": _get_attr(b, 'overlap_warnings', [])
            }
            for b in boundaries if _get_attr(b, 'overlap_warnings', [])
        ]
    }


# ============================================================================
# YAML Configuration Loaders (for flash_map.yaml and sram_map.yaml)
# ============================================================================

def load_flash_map_from_yaml(config_path: Path) -> Dict[str, Any]:
    """Load flash map configuration from YAML file.

    This function reads the manually maintained flash_map.yaml file
    and converts it to the JSON constraint format.

    Args:
        config_path: Path to config/flash_map.yaml

    Returns:
        Dict containing flash map constraint data
    """
    if not config_path.exists():
        logger.warning(f"Flash map YAML config not found: {config_path}")
        return None

    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)

        variants = config.get('variants', {})
        regions = []

        for variant_name, variant_data in variants.items():
            # Extract variant info
            total_size = variant_data.get('total_size', {})
            addr_range = variant_data.get('address_range', {})
            geometry = variant_data.get('geometry', {})

            # Build base flash region
            flash_region = {
                "name": f"{variant_name.upper()}",
                "variant": variant_name,
                "base_address": "0x08000000",
                "address_range": addr_range.get('start', '0x000000'),
                "size_bytes": total_size.get('bytes', 0),
                "size_kb": total_size.get('kb', 0),
                "size_human": total_size.get('human', ''),
                "is_xip": True,
                "access_type": "READ_WRITE_ERASE",
                "description": variant_data.get('description', ''),
                "geometry": {
                    "page_size": geometry.get('page_size', 256),
                    "sector_size": geometry.get('sector_size', 4096),
                    "pages_per_sector": geometry.get('pages_per_sector', 16),
                    "total_pages": geometry.get('total_pages', 0),
                    "total_sectors": geometry.get('total_sectors', 0)
                }
            }
            regions.append(flash_region)

            # Add DATA Region
            variant_regions = variant_data.get('regions', {})
            data_region = variant_regions.get('data_region', {})
            if data_region:
                data_size = data_region.get('size', {})
                sectors = data_region.get('sectors', {})
                regions.append({
                    "name": f"DATA_REGION_{variant_name.upper()}",
                    "variant": variant_name,
                    "base_address": "0x08000000",
                    "address_offset": "0x000000",
                    "size_bytes": data_size.get('bytes', 0),
                    "size_kb": data_size.get('kb', 0),
                    "size_human": data_size.get('human', ''),
                    "is_xip": False,
                    "access_type": "READ_WRITE",
                    "description": data_region.get('description', ''),
                    "sectors": f"{sectors.get('start', 0)} - {sectors.get('end', 0)}",
                    "sector_count": sectors.get('count', 0)
                })

            # Add CODE Region
            code_region = variant_regions.get('code_region', {})
            if code_region:
                code_size = code_region.get('size', {})
                sectors = code_region.get('sectors', {})
                features = code_region.get('features', {})
                regions.append({
                    "name": f"CODE_REGION_{variant_name.upper()}",
                    "variant": variant_name,
                    "base_address": "0x08004000",  # After 16KB DATA region
                    "address_offset": "0x004000",
                    "size_bytes": code_size.get('bytes', 0),
                    "size_kb": code_size.get('kb', 0),
                    "size_human": code_size.get('human', ''),
                    "is_xip": features.get('xip_support', True),
                    "access_type": "READ_WRITE_ERASE",
                    "description": code_region.get('description', ''),
                    "sectors": f"{sectors.get('start', 0)} - {sectors.get('end', 0)}",
                    "sector_count": sectors.get('count', 0),
                    "iap_support": features.get('iap_support', False)
                })

        # Build result
        result = {
            "constraint_type": "memory_layout",
            "domain": "domain1",
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "total_regions": len(regions),
            "variants": list(variants.keys()),
            "regions": regions,
            "magic_codes": config.get('magic_codes', {}),
            "memory_mapping": config.get('memory_mapping', {}),
            "constraints": config.get('constraints', []),
            "data_source": "yaml_config",
            "source_file": "config/flash_map.yaml"  # Relative path
        }

        logger.info(f"Loaded flash map from YAML: {len(regions)} regions for {len(variants)} variants")
        return result

    except Exception as e:
        logger.error(f"Failed to load flash map YAML: {e}")
        return None


def load_sram_map_from_yaml(config_path: Path) -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """Load SRAM map configuration from YAML file.

    This function reads the manually maintained sram_map.yaml file
    and converts it to the JSON constraint format for both:
    - sram_map.json (SRAM allocation regions)
    - memory_boundaries.json (memory boundary definitions)

    Args:
        config_path: Path to config/sram_map.yaml

    Returns:
        Tuple of (sram_map_data, memory_boundaries_data)
    """
    if not config_path.exists():
        logger.warning(f"SRAM map YAML config not found: {config_path}")
        return None, None

    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)

        # Extract chip variants
        chip_variants = config.get('chip_variants', {})

        # Extract SRAM physical blocks
        sram_blocks = config.get('sram_blocks', {})
        sram_summary = config.get('sram_summary', {})

        # Build memory boundaries from SRAM blocks
        boundaries = []
        for block_name, block_data in sram_blocks.items():
            addr_range = block_data.get('address_range', {})
            size_info = block_data.get('size', {})
            boundary = {
                "region_name": block_data.get('name', block_name),
                "start_address": addr_range.get('start', '0x00000000'),
                "end_address": addr_range.get('end', '0x00000000'),
                "size_bytes": size_info.get('bytes', 0),
                "size_kb": size_info.get('kb', 0),
                "size_human": size_info.get('human', ''),
                "boundary_type": "physical_block",
                "library_type": "all",
                "description": block_data.get('description', ''),
                "access_restrictions": [],
                "alignment_requirement": 0
            }
            boundaries.append(boundary)

        # Build SRAM regions from allocation modes
        allocation_modes = config.get('allocation_modes', {})
        sram_regions = []

        for mode_name, mode_data in allocation_modes.items():
            allocations = mode_data.get('allocation', [])

            for alloc in allocations:
                size_info = alloc.get('size', {})
                region = {
                    "name": alloc.get('name', 'Unknown'),
                    "start_address": None,
                    "end_address": None,
                    "size_bytes": size_info.get('bytes', 0),
                    "size_kb": size_info.get('bytes', 0) // 1024,
                    "size_human": size_info.get('hex', '0x0'),
                    "description": alloc.get('description', ''),
                    "library_type": mode_data.get('ble_library', 'all') or 'all',
                    "allocation_mode": mode_name,
                    "ble_connections": mode_data.get('ble_connections', 0)
                }
                sram_regions.append(region)

        # Build sram_map.json result
        sram_map_result = {
            "constraint_type": "sram_allocation",
            "domain": "domain1",
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "total_regions": len(sram_regions),
            "total_size_kb": sram_summary.get('total_size', {}).get('kb', 32),
            "regions": sram_regions,
            "chip_variants": list(chip_variants.keys()),
            "sram_blocks": {
                name: {
                    "name": data.get('name', name),
                    "start_address": data.get('address_range', {}).get('start', ''),
                    "end_address": data.get('address_range', {}).get('end', ''),
                    "size_kb": data.get('size', {}).get('kb', 0)
                }
                for name, data in sram_blocks.items()
            },
            "allocation_modes": {
                name: {
                    "name": mode.get('name', name),
                    "ble_library": mode.get('ble_library'),
                    "ble_connections": mode.get('ble_connections', 0),
                    "user_space_kb": mode.get('user_space_total', {}).get('kb', 0)
                }
                for name, mode in allocation_modes.items()
            },
            "mode_selection_guide": config.get('mode_selection_guide', []),
            "constraints": config.get('constraints', []),
            "ble_library_mapping": config.get('ble_library_mapping', {}),
            "data_source": "yaml_config",
            "source_file": "config/sram_map.yaml"  # Relative path
        }

        # Build memory_boundaries.json result
        memory_boundaries_result = {
            "constraint_type": "memory_boundaries",
            "domain": "domain1",
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "total_boundaries": len(boundaries),
            "total_size_kb": sram_summary.get('total_size', {}).get('kb', 32),
            "boundaries": boundaries,
            "sram_summary": {
                "total_bytes": sram_summary.get('total_size', {}).get('bytes', 32768),
                "total_kb": sram_summary.get('total_size', {}).get('kb', 32),
                "blocks": sram_summary.get('blocks', [])
            },
            "data_source": "yaml_config",
            "source_file": "config/sram_map.yaml"  # Relative path
        }

        logger.info(f"Loaded SRAM map from YAML: {len(sram_regions)} regions, {len(boundaries)} boundaries")
        return sram_map_result, memory_boundaries_result

    except Exception as e:
        logger.error(f"Failed to load SRAM map YAML: {e}")
        return None, None


# ============================================================================
# Default Constraint Data Generator (fallback when Excel parsing fails)
# ============================================================================

def generate_default_constraint(output_name: str, domain: str) -> Dict[str, Any]:
    """Generate default constraint data when Excel parsing fails."""
    from datetime import datetime

    # Common metadata for default data
    default_metadata = {
        "data_source": "default_generation",
        "generation_reason": "Excel parsing failed or returned no data",
        "warning": "This is default data - may not reflect actual B6x chip specifications"
    }

    result = None

    if output_name == "flash_map.json":
        result = {
            "constraint_type": "memory_layout",
            "domain": domain,
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "total_regions": 3,
            "regions": [
                {
                    "name": "FLASH",
                    "base_address": "0x08000000",
                    "size_bytes": 262144,
                    "size_kb": 256,
                    "size_human": "256KB",
                    "is_xip": True,
                    "access_type": "READ_ONLY",
                    "description": "Code Flash (XIP supported)"
                },
                {
                    "name": "FLASH_128KB",
                    "base_address": "0x08000000",
                    "size_bytes": 131072,
                    "size_kb": 128,
                    "size_human": "128KB",
                    "is_xip": True,
                    "access_type": "READ_ONLY",
                    "description": "Code Flash 128KB variant"
                },
                {
                    "name": "DATA_FLASH",
                    "base_address": "0x18000000",
                    "size_bytes": 16384,
                    "size_kb": 16,
                    "size_human": "16KB",
                    "is_xip": False,
                    "access_type": "READ_WRITE",
                    "description": "Data Flash for configuration and OTA"
                }
            ]
        }

    elif output_name == "sram_map.json":
        result = {
            "constraint_type": "sram_allocation",
            "domain": domain,
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "total_regions": 4,
            "total_size_kb": 32,
            "regions": [
                {
                    "name": "BLE_STACK",
                    "start_address": "0x20003000",
                    "end_address": "0x20003097",
                    "size_bytes": 152,
                    "size_kb": 0,
                    "description": "BLE Stack (3 connections)",
                    "library_type": "ble6"
                },
                {
                    "name": "BLE_STACK_LITE",
                    "start_address": "0x20003000",
                    "end_address": "0x20003097",
                    "size_bytes": 152,
                    "size_kb": 0,
                    "description": "BLE Stack (1 connection)",
                    "library_type": "ble6_lite"
                },
                {
                    "name": "BLE_STACK_FULL",
                    "start_address": "0x20003000",
                    "end_address": "0x20003097",
                    "size_bytes": 152,
                    "size_kb": 0,
                    "description": "BLE Stack (6 connections)",
                    "library_type": "ble6_8act_6con"
                },
                {
                    "name": "USER_HEAP",
                    "start_address": "0x20003098",
                    "end_address": "0x20004DFF",
                    "size_bytes": 7576,
                    "size_kb": 7,
                    "description": "User Application Heap (varies by library)",
                    "library_type": "all"
                }
            ]
        }

    elif output_name == "memory_boundaries.json":
        result = {
            "constraint_type": "memory_boundaries",
            "domain": domain,
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "total_boundaries": 3,
            "total_size_kb": 32,
            "boundaries": [
                {
                    "region_name": "BLE_STACK",
                    "start_address": "0x20003000",
                    "end_address": "0x20003097",
                    "size_bytes": 152,
                    "size_kb": 0,
                    "size_human": "152B",
                    "boundary_type": "fixed",
                    "library_type": "ble6_lite",
                    "description": "BLE Stack for lite library (1 connection)",
                    "access_restrictions": ["read_write", "stack_internal"],
                    "alignment_requirement": 4
                },
                {
                    "region_name": "BLE_STACK",
                    "start_address": "0x20003000",
                    "end_address": "0x20003097",
                    "size_bytes": 152,
                    "size_kb": 0,
                    "size_human": "152B",
                    "boundary_type": "fixed",
                    "library_type": "ble6",
                    "description": "BLE Stack for standard library (3 connections)",
                    "access_restrictions": ["read_write", "stack_internal"],
                    "alignment_requirement": 4
                },
                {
                    "region_name": "BLE_STACK",
                    "start_address": "0x20003000",
                    "end_address": "0x20003097",
                    "size_bytes": 152,
                    "size_kb": 0,
                    "size_human": "152B",
                    "boundary_type": "fixed",
                    "library_type": "ble6_8act_6con",
                    "description": "BLE Stack for full library (6 connections)",
                    "access_restrictions": ["read_write", "stack_internal"],
                    "alignment_requirement": 4
                }
            ],
            "boundary_warnings": []
        }

    elif output_name == "phone_compatibility_issues.json":
        result = {
            "constraint_type": "phone_compatibility_issues",
            "domain": domain,
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "total_issues": 0,
            "total_brands": 0,
            "brands": {},
            "issues_by_severity": {
                "critical": 0,
                "high": 0,
                "medium": 0,
                "low": 0
            }
        }

    # Add metadata to result
    if result:
        result.update(default_metadata)

    return result


# ============================================================================
# Main Build Script
# ============================================================================

def find_sdk_root() -> Path:
    """Find SDK root directory."""
    current = Path(__file__).parent.parent
    while current.parent != current:
        if (current / "ble").exists() and (current / "drivers").exists():
            return current
        current = current.parent
    return Path(__file__).parent.parent


def build_excel_constraints():
    """Build all Excel constraint JSON files."""
    # Initialize logger
    if USE_ENHANCED_LOGGER:
        log = BuildLogger("build_excel_constraints")
        log.header("B6x SDK Excel Constraints Builder")
    else:
        log = None

    # Find paths
    mcp_root = Path(__file__).parent.parent
    sdk_root = find_sdk_root()
    doc_dir = sdk_root / "doc" / "SW_Spec"
    doc_root = sdk_root / "doc"  # Also search root doc directory
    output_dir = mcp_root / "data" / "constraints"

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    if log:
        log.info(f"SDK Root: {sdk_root}")
        log.info(f"Doc Dir: {doc_dir}")
        log.info(f"Output Dir: {output_dir}")
        log.info("")
    else:
        print("=" * 70)
        print("B6x SDK Excel Constraints Builder")
        print("=" * 70)
        print(f"SDK Root: {sdk_root}")
        print(f"Doc Dir: {doc_dir}")
        print(f"Output Dir: {output_dir}")
        print()

    # Initialize parsers
    parser = ExcelHardwareParser()

    # Initialize Markdown pipeline
    cache_dir = mcp_root / "data" / "cache" / "excel_md"
    md_converter = ExcelToMarkdownConverter(cache_dir)
    md_parser = MarkdownTableParser()

    log_info = log.info if log else logger.info
    log_debug = log.debug if log else logger.debug

    # Log parser capabilities
    if log:
        if parser._pandas_available:
            log.debug("pandas library available")
        if parser._openpyxl_available:
            log.debug("openpyxl library available")

    # Initialize results tracking
    results = {
        "success": [],
        "failed": [],
        "skipped": []
    }

    # Excel files to parse with multiple outputs
    # Format: (excel_filename, output_name) -> config
    # NOTE: B6x-Flash-Map.xlsx is handled separately via YAML config (config/flash_map.yaml)
    # NOTE: B6x_BLE-Sram空间分配.xlsx is handled separately via YAML config (config/sram_map.yaml)
    # NOTE: ble_compatibility.json generation removed - not needed
    excel_configs = [
        ("B6x_IO_MAP.xlsx", "io_map.json", "parse_io_map", convert_pin_mux_to_json, "domain1"),
        # ("B6x-Flash-Map.xlsx", ...) - REMOVED: Now using config/flash_map.yaml
        # ("B6x_BLE-Sram空间分配.xlsx", ...) - REMOVED: Now using config/sram_map.yaml
        ("B6x_BLE-功耗参考.xlsx", "power_consumption.json", "parse_power_map", convert_power_entries_to_json, "domain3"),
        # ("B6x BLE兼容性列表.xlsx", "ble_compatibility.json", ...) - REMOVED: Not needed
        ("B6x BLE兼容性列表.xlsx", "phone_compatibility_issues.json", "parse_ble_phone_compatibility_issues", convert_phone_issues_to_json, "domain3"),
    ]

    # Process flash_map.yaml separately (replaces B6x-Flash-Map.xlsx)
    if log:
        log.info("Processing flash_map.yaml configuration...")
    else:
        print("Processing: flash_map.yaml → flash_map.json")

    flash_yaml_path = mcp_root / "config" / "flash_map.yaml"
    flash_json_data = load_flash_map_from_yaml(flash_yaml_path)

    if flash_json_data:
        flash_output_path = output_dir / "flash_map.json"
        with open(flash_output_path, 'w', encoding='utf-8') as f:
            json.dump(flash_json_data, f, indent=2, ensure_ascii=False)

        if log:
            log.file_success("flash_map.json", f"Domain: domain1, Type: memory_layout, Variants: {flash_json_data.get('variants', [])}")
        else:
            print(f"  [OK] Generated flash_map.json")
            print(f"     - Domain: domain1")
            print(f"     - Type: memory_layout")
            print(f"     - Variants: {flash_json_data.get('variants', [])}")
            print(f"     - Regions: {flash_json_data.get('total_regions', 0)}")

        results["success"].append(("flash_map.yaml", "flash_map.json"))
    else:
        # Fallback to default generation if YAML fails
        if log:
            log.warning("YAML loading failed, generating default flash_map.json")
        else:
            print("  [*] YAML loading failed, generating default data...")

        default_flash = generate_default_constraint("flash_map.json", "domain1")
        if default_flash:
            flash_output_path = output_dir / "flash_map.json"
            with open(flash_output_path, 'w', encoding='utf-8') as f:
                json.dump(default_flash, f, indent=2, ensure_ascii=False)
            results["success"].append(("default", "flash_map.json"))
        else:
            results["failed"].append(("flash_map.yaml", "flash_map.json"))

    if not log:
        print()

    # Process sram_map.yaml separately (replaces B6x_BLE-Sram空间分配.xlsx)
    if log:
        log.info("Processing sram_map.yaml configuration...")
    else:
        print("Processing: sram_map.yaml → sram_map.json + memory_boundaries.json")

    sram_yaml_path = mcp_root / "config" / "sram_map.yaml"
    sram_map_data, memory_boundaries_data = load_sram_map_from_yaml(sram_yaml_path)

    if sram_map_data:
        # Save sram_map.json
        sram_output_path = output_dir / "sram_map.json"
        with open(sram_output_path, 'w', encoding='utf-8') as f:
            json.dump(sram_map_data, f, indent=2, ensure_ascii=False)

        if log:
            log.file_success("sram_map.json", f"Domain: domain1, Type: sram_allocation, Regions: {sram_map_data.get('total_regions', 0)}")
        else:
            print(f"  [OK] Generated sram_map.json")
            print(f"     - Domain: domain1")
            print(f"     - Type: sram_allocation")
            print(f"     - Regions: {sram_map_data.get('total_regions', 0)}")
            print(f"     - Total: {sram_map_data.get('total_size_kb', 0)} KB")

        results["success"].append(("sram_map.yaml", "sram_map.json"))
    else:
        # Fallback to default generation if YAML fails
        if log:
            log.warning("YAML loading failed, generating default sram_map.json")
        else:
            print("  [*] YAML loading failed, generating default data...")

        default_sram = generate_default_constraint("sram_map.json", "domain1")
        if default_sram:
            sram_output_path = output_dir / "sram_map.json"
            with open(sram_output_path, 'w', encoding='utf-8') as f:
                json.dump(default_sram, f, indent=2, ensure_ascii=False)
            results["success"].append(("default", "sram_map.json"))
        else:
            results["failed"].append(("sram_map.yaml", "sram_map.json"))

    if memory_boundaries_data:
        # Save memory_boundaries.json
        boundaries_output_path = output_dir / "memory_boundaries.json"
        with open(boundaries_output_path, 'w', encoding='utf-8') as f:
            json.dump(memory_boundaries_data, f, indent=2, ensure_ascii=False)

        if log:
            log.file_success("memory_boundaries.json", f"Domain: domain1, Type: memory_boundaries, Boundaries: {memory_boundaries_data.get('total_boundaries', 0)}")
        else:
            print(f"  [OK] Generated memory_boundaries.json")
            print(f"     - Domain: domain1")
            print(f"     - Type: memory_boundaries")
            print(f"     - Boundaries: {memory_boundaries_data.get('total_boundaries', 0)}")
            print(f"     - Total: {memory_boundaries_data.get('total_size_kb', 0)} KB")

        results["success"].append(("sram_map.yaml", "memory_boundaries.json"))
    else:
        # Fallback to default generation if YAML fails
        if log:
            log.warning("YAML loading failed, generating default memory_boundaries.json")
        else:
            print("  [*] YAML loading failed, generating default data...")

        default_boundaries = generate_default_constraint("memory_boundaries.json", "domain1")
        if default_boundaries:
            boundaries_output_path = output_dir / "memory_boundaries.json"
            with open(boundaries_output_path, 'w', encoding='utf-8') as f:
                json.dump(default_boundaries, f, indent=2, ensure_ascii=False)
            results["success"].append(("default", "memory_boundaries.json"))
        else:
            results["failed"].append(("sram_map.yaml", "memory_boundaries.json"))

    if not log:
        print()

    # Process each Excel config
    for excel_filename, output_name, parser_method_name, converter_func, domain in excel_configs:
        # Try to find file in multiple directories
        excel_path = None
        for search_dir in [doc_dir, doc_root]:
            test_path = search_dir / excel_filename
            if test_path.exists():
                excel_path = test_path
                break

        output_path = output_dir / output_name

        if log:
            log.file_start(f"{excel_filename} → {output_name}")
        else:
            print(f"Processing: {excel_filename} → {output_name}")

        # Check if file exists
        if not excel_path:
            if log:
                log.file_skipped(excel_filename, f"File not found. Searched: {doc_dir}, {doc_root}")
            else:
                print(f"  [!] File not found in any search directory")
                print(f"     Searched: {doc_dir}, {doc_root}")
            results["skipped"].append((excel_filename, output_name))
            if not log:
                print()
            continue

        try:
            # Parse Excel file with standard parser
            parser_method = getattr(parser, parser_method_name)
            entries = parser_method(str(excel_path))

            # If standard parser returns empty results, try parsers in order:
            # 1. B6x format parser
            # 2. Markdown pipeline (new!)
            if not entries:
                if log:
                    log.info(f"Standard parser returned no data from {excel_filename}, trying B6x format parser...")
                else:
                    print(f"  [*] Standard parser returned no data, trying B6x format parser...")

                # Try B6x specialized parser
                try:
                    if excel_filename == "B6x-Flash-Map.xlsx":
                        b6x_regions = parse_flash_map_b6x(str(excel_path))
                        if b6x_regions:
                            entries = b6x_regions
                            if log:
                                log.success(f"B6x parser found {len(entries)} regions")
                            else:
                                print(f"  [OK] B6x parser found {len(entries)} regions")
                        else:
                            if log:
                                log.warning("B6x parser also returned no data")
                            else:
                                print(f"  [!] B6x parser also returned no data")
                    elif excel_filename == "B6x_BLE-Sram空间分配.xlsx":
                        if output_name == "sram_map.json":
                            b6x_regions, _ = parse_sram_allocation_b6x(str(excel_path))
                            if b6x_regions:
                                entries = b6x_regions
                                if log:
                                    log.success(f"B6x parser found {len(entries)} SRAM regions")
                                else:
                                    print(f"  [OK] B6x parser found {len(entries)} SRAM regions")
                            else:
                                if log:
                                    log.warning("B6x parser also returned no data")
                                else:
                                    print(f"  [!] B6x parser also returned no data")
                        elif output_name == "memory_boundaries.json":
                            _, b6x_boundaries = parse_sram_allocation_b6x(str(excel_path))
                            if b6x_boundaries:
                                entries = b6x_boundaries
                                if log:
                                    log.success(f"B6x parser found {len(entries)} boundaries")
                                else:
                                    print(f"  [OK] B6x parser found {len(entries)} boundaries")
                            else:
                                if log:
                                    log.warning("B6x parser also returned no data")
                                else:
                                    print(f"  [!] B6x parser also returned no data")
                    elif excel_filename == "B6x_BLE-功耗参考.xlsx":
                        b6x_power = parse_power_consumption_b6x(str(excel_path))
                        if b6x_power:
                            entries = b6x_power
                            if log:
                                log.success(f"B6x parser found {len(entries)} power entries")
                            else:
                                print(f"  [OK] B6x parser found {len(entries)} power entries")
                        else:
                            print(f"  [!] B6x parser also returned no data")
                except Exception as b6x_error:
                    print(f"  [!] B6x parser error: {b6x_error}")

            # If still no entries, try Markdown pipeline
            if not entries and md_converter.should_use_markdown_pipeline(excel_filename):
                if log:
                    log.info("Trying Markdown pipeline (Stage 1: Excel → Markdown)...")
                else:
                    print(f"  [*] Trying Markdown pipeline (Excel → Markdown → JSON)...")

                try:
                    # Stage 1: Convert Excel to Markdown
                    md_path = md_converter.convert_excel_to_md(excel_path, force=False)

                    if md_path and md_path.exists():
                        if log:
                            log.success(f"Stage 1 complete: {md_path.relative_to(mcp_root)}")
                        else:
                            print(f"  [OK] Stage 1: Excel → Markdown")

                        # Stage 2: Parse Markdown to structured data
                        if excel_filename == "B6x-Flash-Map.xlsx":
                            entries = md_parser.parse_flash_map_md(md_path)
                        elif excel_filename == "B6x_BLE-Sram空间分配.xlsx":
                            if output_name == "sram_map.json":
                                entries, _ = md_parser.parse_sram_allocation_md(md_path)
                            elif output_name == "memory_boundaries.json":
                                _, entries = md_parser.parse_sram_allocation_md(md_path)
                        elif excel_filename == "B6x BLE兼容性列表.xlsx":
                            # Only phone_compatibility_issues.json is processed now
                            if output_name == "phone_compatibility_issues.json":
                                entries = md_parser.parse_ble_compatibility_md(md_path)

                        if entries:
                            if log:
                                log.success(f"Stage 2 complete: Markdown pipeline extracted {len(entries)} entries")
                            else:
                                print(f"  [OK] Stage 2: Markdown → JSON ({len(entries)} entries)")
                        else:
                            if log:
                                log.info("Markdown pipeline extracted no data, using default generation")
                            else:
                                print(f"  [*] Markdown pipeline extracted no data, using default generation")
                    else:
                        if log:
                            log.warning("Markdown conversion failed")
                        else:
                            print(f"  [!] Markdown conversion failed")

                except Exception as md_error:
                    if log:
                        log.error(f"Markdown pipeline error: {md_error}")
                    else:
                        print(f"  [!] Markdown pipeline error: {md_error}")
                    import traceback
                    traceback.print_exc()

            # If still no entries, generate default data
            if not entries:
                print(f"  [*] Generating default constraint data...")
                json_data = generate_default_constraint(output_name, domain)
                if not json_data:
                    results["skipped"].append((excel_filename, output_name))
                    print()
                    continue
                print(f"  [OK] Generated default {output_name}")
            else:
                # Convert to JSON
                json_data = converter_func(entries)

            # Add domain info and data source tracking
            json_data["domain"] = domain
            if "data_source" not in json_data:
                json_data["data_source"] = "excel_parsing" if entries else "default_generation"
            json_data["source_file"] = f"doc/SW_Spec/{excel_path.name}"  # Relative path

            # Write JSON file
            with open(output_path, 'w', encoding='utf-8') as f:
                json.dump(json_data, f, indent=2, ensure_ascii=False)

            # Success message
            if log:
                log.file_success(output_name, f"Domain: {domain}, Type: {json_data.get('constraint_type', 'unknown')}, Entries: {len(entries)}")
            else:
                print(f"  [OK] Generated {output_name}")
                print(f"     - Domain: {domain}")
                print(f"     - Type: {json_data.get('constraint_type', 'unknown')}")
                print(f"     - Entries: {len(entries)}")

                # Print specific stats
                if json_data.get("constraint_type") == "pin_mux":
                    print(f"     - Pins: {json_data.get('total_pins', 0)}")
                elif json_data.get("constraint_type") == "memory_layout":
                    print(f"     - Regions: {json_data.get('total_regions', 0)}")
                elif json_data.get("constraint_type") == "sram_allocation":
                    print(f"     - Total: {json_data.get('total_size_kb', 0)} KB")
                elif json_data.get("constraint_type") == "memory_boundaries":
                    print(f"     - Boundaries: {json_data.get('total_boundaries', 0)}")
                    print(f"     - Total: {json_data.get('total_size_kb', 0)} KB")
                elif json_data.get("constraint_type") == "phone_compatibility_issues":
                    print(f"     - Issues: {json_data.get('total_issues', 0)}")
                    print(f"     - Brands: {json_data.get('total_brands', 0)}")

            results["success"].append((excel_filename, output_name))
            if not log:
                print()

        except Exception as e:
            if log:
                log.file_error(excel_filename, e, context={
                    'output_name': output_name,
                    'domain': domain,
                    'parser_method': parser_method_name
                })
            else:
                print(f"  [ERROR] {e}")
            results["failed"].append((excel_filename, output_name))
            if not log:
                print()

    # Generate summary
    if log:
        log.summary()
    else:
        print("=" * 70)
        print("Build Summary")
    print("=" * 70)
    print(f"[OK] Success: {len(results['success'])}")
    print(f"[!] Skipped: {len(results['skipped'])}")
    print(f"[FAIL] Failed:  {len(results['failed'])}")
    print()

    # Write build report
    report_path = output_dir / "build_report.json"
    report = {
        "build_date": datetime.now().isoformat(),
        "sdk_root": "<resolved_at_runtime>",  # Portable marker
        "results": {
            "success_count": len(results['success']),
            "skipped_count": len(results['skipped']),
            "failed_count": len(results['failed']),
            "details": results
        },
        "generated_files": [
            {
                "name": output_name,
                "source": excel_filename,
                "domain": domain
            }
            for excel_filename, output_name, _, _, domain in excel_configs
        ]
    }

    with open(report_path, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    print(f"Build report: {report_path}")

    return len(results["failed"]) == 0


if __name__ == "__main__":
    success = build_excel_constraints()
    sys.exit(0 if success else 1)
