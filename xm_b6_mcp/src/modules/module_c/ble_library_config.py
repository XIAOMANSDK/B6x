"""
BLE Library Configuration Tools for Project Template Generation (Module C)
==========================================================================

Provides AI with tools to select and configure the appropriate BLE library
when generating project templates.

Key Features:
1. BLE library selection based on requirements
2. Automatic cfg.h macro generation
3. Memory layout recommendations
4. Makefile/project file configuration
"""

from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from enum import Enum


class BLELibraryType(Enum):
    """BLE Library variants"""
    LITE = "ble6_lite.lib"           # Lightweight, 1 connection, ~6KB RAM
    STANDARD = "ble6.lib"            # Standard, 3 connections, ~14.6KB RAM
    FULL = "ble6_8act_6con.lib"      # Full featured, 6 connections, ~16.2KB RAM


@dataclass
class BLELibraryConfig:
    """BLE library configuration details"""
    lib_type: BLELibraryType
    lib_file: str
    connections_max: int
    activities_max: int
    exch_size: int
    heap_env_size: int
    heap_msg_size: int
    total_ram_kb: float
    roles: List[str]
    use_cases: List[str]
    lite_macro: int
    large_macro: int


# BLE Library Configurations
BLE_LIBRARIES: Dict[BLELibraryType, BLELibraryConfig] = {
    BLELibraryType.LITE: BLELibraryConfig(
        lib_type=BLELibraryType.LITE,
        lib_file="ble6_lite.lib",
        connections_max=1,
        activities_max=2,
        exch_size=0x0BAC,        # 3KB
        heap_env_size=0x600,     # 1.5KB
        heap_msg_size=0x600,     # 1.5KB
        total_ram_kb=6.0,
        roles=["Peripheral"],
        use_cases=[
            "HID devices (mouse, keyboard)",
            "Battery-powered sensors",
            "Single connection peripherals",
            "Ultra-low power applications",
            "Cost-sensitive products"
        ],
        lite_macro=1,
        large_macro=0
    ),

    BLELibraryType.STANDARD: BLELibraryConfig(
        lib_type=BLELibraryType.STANDARD,
        lib_file="ble6.lib",
        connections_max=3,
        activities_max=4,
        exch_size=0x0FD8,        # 3.6KB
        heap_env_size=0xC00,     # 3KB
        heap_msg_size=0x2000,    # 8KB
        total_ram_kb=14.6,
        roles=["Central", "Peripheral"],
        use_cases=[
            "General BLE applications",
            "Multi-connection peripherals (2-3)",
            "Central + Peripheral dual role",
            "Standard BLE services"
        ],
        lite_macro=0,
        large_macro=0
    ),

    BLELibraryType.FULL: BLELibraryConfig(
        lib_type=BLELibraryType.FULL,
        lib_file="ble6_8act_6con.lib",
        connections_max=6,
        activities_max=8,
        exch_size=0x1590,        # 5.2KB
        heap_env_size=0xC00,     # 3KB
        heap_msg_size=0x2000,    # 8KB
        total_ram_kb=16.2,
        roles=["Multi-role Central", "Multi-role Peripheral"],
        use_cases=[
            "Connection concentrators",
            "Gateway devices",
            "Multi-device managers (6 connections)",
            "Mesh network coordinators",
            "Complex scenarios with 8 activities"
        ],
        lite_macro=0,
        large_macro=1
    )
}


def recommend_ble_library(
    num_connections: int = 1,
    ram_optimization: bool = False,
    application_type: str = "",
    max_activities: int = 0
) -> Dict:
    """
    Recommend the appropriate BLE library based on requirements

    Args:
        num_connections: Required number of simultaneous connections (1-6)
        ram_optimization: If True, prioritize RAM usage
        application_type: Application type (e.g., "mouse", "gateway", "sensor")
        max_activities: Maximum concurrent BLE activities needed

    Returns:
        Dictionary with recommendation and reasoning
    """
    # Determine library based on requirements
    if ram_optimization and num_connections <= 1:
        recommended = BLELibraryType.LITE
        reason = f"RAM optimization requested, {num_connections} connection(s) sufficient for lite library"

    elif num_connections <= 1:
        # Check application type for single connection
        app_lower = application_type.lower()
        if any(keyword in app_lower for keyword in ["mouse", "keyboard", "hid", "sensor", "beacon"]):
            recommended = BLELibraryType.LITE
            reason = f"Application type '{application_type}' is well-suited for lite library (single connection, ultra-low power)"
        else:
            recommended = BLELibraryType.STANDARD
            reason = f"Standard library recommended for general application with {num_connections} connection(s)"

    elif num_connections <= 3:
        if max_activities > 4:
            recommended = BLELibraryType.FULL
            reason = f"Activities requirement ({max_activities}) exceeds standard library limit (4), full library required"
        else:
            recommended = BLELibraryType.STANDARD
            reason = f"{num_connections} connection(s) within standard library limit (3)"

    elif num_connections <= 6:
        recommended = BLELibraryType.FULL
        reason = f"{num_connections} connection(s) require full library (supports up to 6)"

    else:
        recommended = BLELibraryType.FULL
        reason = f"Requested {num_connections} connections exceeds maximum (6), full library configured for max capacity"

    config = BLE_LIBRARIES[recommended]

    return {
        "recommended_library": config.lib_file,
        "library_type": config.lib_type.value,
        "reasoning": reason,
        "configuration": {
            "connections_max": config.connections_max,
            "activities_max": config.activities_max,
            "total_ram_kb": config.total_ram_kb,
            "roles": config.roles
        },
        "use_cases": config.use_cases
    }


def generate_cfg_macros(
    lib_type: BLELibraryType = BLELibraryType.STANDARD,
    nb_slave: int = 0,
    nb_master: int = 0,
    custom_heap: bool = False,
    heap_base: int = 0,
    heap_env_size: int = 0,
    heap_msg_size: int = 0
) -> Dict:
    """
    Generate cfg.h macro configuration for selected BLE library

    Args:
        lib_type: BLE library type
        nb_slave: Number of slave connections to support
        nb_master: Number of master connections to support
        custom_heap: Whether to use custom heap configuration
        heap_base: Custom heap base address (if custom_heap=True)
        heap_env_size: Custom heap environment size (if custom_heap=True)
        heap_msg_size: Custom heap message size (if custom_heap=True)

    Returns:
        Dictionary with cfg.h macro definitions
    """
    config = BLE_LIBRARIES[lib_type]

    # Validate connection counts
    total_connections = nb_slave + nb_master
    if total_connections > config.connections_max:
        raise ValueError(
            f"Requested {total_connections} connections (slave={nb_slave}, master={nb_master}) "
            f"exceeds {config.lib_file} limit ({config.connections_max})"
        )

    macros = {
        "BLE_LITELIB": config.lite_macro,
        "BLE_LARGELIB": config.large_macro,
        "BLE_NB_SLAVE": nb_slave,
        "BLE_NB_MASTER": nb_master,
        "auto_defined_by_blelib_h": {
            "BLE_CONNECTION_MAX": config.connections_max,
            "BLE_ACTIVITY_MAX": config.activities_max,
            "BLE_EXCH_BASE": "0x20008000",
            "BLE_EXCH_SIZE": f"0x{config.exch_size:X}",
            "BLE_EXCH_END": f"0x{0x20008000 + config.exch_size:X}"
        }
    }

    # Custom heap configuration
    if custom_heap:
        macros["custom_heap_config"] = {
            "BLE_HEAP_BASE": f"0x{heap_base:X}",
            "BLE_HEAP_ENV_SIZE": f"0x{heap_env_size:X}",
            "BLE_HEAP_MSG_SIZE": f"0x{heap_msg_size:X}",
            "BLE_HEAP_SIZE": f"0x{heap_env_size + heap_msg_size:X}"
        }
    else:
        # Default heap configuration
        macros["default_heap_config"] = {
            "BLE_HEAP_ENV_SIZE": f"0x{config.heap_env_size:X}",
            "BLE_HEAP_MSG_SIZE": f"0x{config.heap_msg_size:X}",
            "BLE_HEAP_SIZE": f"0x{config.heap_env_size + config.heap_msg_size:X}"
        }

        # Add heap base for lite library (in retention RAM)
        if lib_type == BLELibraryType.LITE:
            exch_end = 0x20008000 + config.exch_size
            macros["default_heap_config"]["BLE_HEAP_BASE"] = f"0x{exch_end:X}"
        else:
            # Standard and Full libraries use default base
            macros["default_heap_config"]["BLE_HEAP_BASE"] = "0x20004E00"

    return macros


def generate_makefile_config(
    lib_type: BLELibraryType,
    sdk_root: str = "$(SDK_ROOT)"
) -> Dict:
    """
    Generate Makefile configuration for BLE library

    Args:
        lib_type: BLE library type
        sdk_root: SDK root path variable

    Returns:
        Dictionary with Makefile configuration
    """
    config = BLE_LIBRARIES[lib_type]

    return {
        "ble_library": {
            "path": f"{sdk_root}/ble/lib/{config.lib_file}",
            "linker_flags": f'"{sdk_root}/ble/lib/{config.lib_file}"',
            "linker_script": f"link_xip_ble{'lite' if lib_type == BLELibraryType.LITE else ''}.sct"
        },
        "memory_regions": {
            "ble_exch": {
                "origin": "0x20008000",
                "length": f"0x{config.exch_size:X}"
            },
            "ble_heap": {
                "origin": "0x20004E00" if lib_type != BLELibraryType.LITE else f"0x{0x20008000 + config.exch_size:X}",
                "length": f"0x{config.heap_env_size + config.heap_msg_size:X}"
            },
            "ble_rwzi": {
                "origin": f"0x{0x20008000 + config.exch_size:X}",
                "end": "0x2000A000"
            }
        }
    }


def generate_keil_project_config(
    lib_type: BLELibraryType,
    project_dir: str = "..\\..\\ble\\lib"
) -> Dict:
    """
    Generate Keil .uvprojx configuration for BLE library

    Args:
        lib_type: BLE library type
        project_dir: Relative path to ble/lib directory

    Returns:
        Dictionary with Keil project configuration
    """
    config = BLE_LIBRARIES[lib_type]

    return {
        "file_group": {
            "Files": [
                {
                    "FileName": f"{project_dir}\\{config.lib_file}",
                    "FileType": 4,  # Library file type in Keil
                    "FilePath": f"{project_dir}\\{config.lib_file}"
                }
            ]
        },
        "linker_script": f"link_xip_ble{'lite' if lib_type == BLELibraryType.LITE else ''}.sct",
        "memory_regions": {
            "IRAM": {
                "start": "0x20000000",
                "size": f"0x{0x20000:X}",
                "attributes": "RWX"
            }
        },
        "preprocessor_definitions": [
            f"BLE_LITELIB={config.lite_macro}",
            f"BLE_LARGELIB={config.large_macro}"
        ]
    }


def compare_all_libraries() -> Dict:
    """
    Compare all BLE library variants

    Returns:
        Dictionary with comparison table
    """
    comparison = {
        "libraries": []
    }

    for lib_type, config in BLE_LIBRARIES.items():
        comparison["libraries"].append({
            "name": config.lib_file,
            "type": config.lib_type.value,
            "connections_max": config.connections_max,
            "activities_max": config.activities_max,
            "ram_kb": config.total_ram_kb,
            "roles": config.roles,
            "use_cases": config.use_cases,
            "cfg_macros": {
                "BLE_LITELIB": config.lite_macro,
                "BLE_LARGELIB": config.large_macro
            }
        })

    return comparison


def get_library_info(lib_type: BLELibraryType) -> Dict:
    """
    Get detailed information about a specific BLE library

    Args:
        lib_type: BLE library type

    Returns:
        Dictionary with library information
    """
    config = BLE_LIBRARIES[lib_type]

    return {
        "library_file": config.lib_file,
        "library_type": config.lib_type.value,
        "capabilities": {
            "max_connections": config.connections_max,
            "max_activities": config.activities_max,
            "supported_roles": config.roles
        },
        "memory_usage": {
            "exch_size_hex": f"0x{config.exch_size:X}",
            "exch_size_kb": config.exch_size / 1024,
            "heap_env_size_hex": f"0x{config.heap_env_size:X}",
            "heap_env_size_kb": config.heap_env_size / 1024,
            "heap_msg_size_hex": f"0x{config.heap_msg_size:X}",
            "heap_msg_size_kb": config.heap_msg_size / 1024,
            "total_ram_kb": config.total_ram_kb
        },
        "cfg_configuration": {
            "BLE_LITELIB": config.lite_macro,
            "BLE_LARGELIB": config.large_macro,
            "BLE_CONNECTION_MAX": config.connections_max,
            "BLE_ACTIVITY_MAX": config.activities_max
        },
        "ideal_use_cases": config.use_cases,
        "example_projects": _get_example_projects(lib_type)
    }


def _get_example_projects(lib_type: BLELibraryType) -> List[str]:
    """Get example projects that use this library type"""
    examples = {
        BLELibraryType.LITE: [
            "projects/QKIE_Mouse/",
            "projects/bleHid_Mouse/",
            "projects/bleHid_Keybd/",
            "projects/bleRemoteControl/Slave/"
        ],
        BLELibraryType.STANDARD: [
            "projects/bleUart/",
            "projects/bleOTA/",
            "projects/bleMasterSlave/",
            "projects/bleUartAT/"
        ],
        BLELibraryType.FULL: [
            "projects/bleMesh/",
            "projects/bleMaster/",
            "projects/FindMy_Dongle/"
        ]
    }
    return examples.get(lib_type, [])


def validate_configuration(
    lib_type: BLELibraryType,
    nb_slave: int,
    nb_master: int,
    heap_env_size: Optional[int] = None,
    heap_msg_size: Optional[int] = None
) -> Tuple[bool, List[str]]:
    """
    Validate BLE library configuration

    Args:
        lib_type: Selected BLE library type
        nb_slave: Number of slave connections
        nb_master: Number of master connections
        heap_env_size: Heap environment size (optional)
        heap_msg_size: Heap message size (optional)

    Returns:
        Tuple of (is_valid, list of warnings/errors)
    """
    config = BLE_LIBRARIES[lib_type]
    errors = []
    warnings = []

    # Check connection limits
    total_connections = nb_slave + nb_master
    if total_connections > config.connections_max:
        errors.append(
            f"Connection limit exceeded: requested {total_connections}, "
            f"max {config.connections_max} for {config.lib_file}"
        )

    # Check heap sizes
    if heap_env_size is not None and heap_env_size < config.heap_env_size:
        errors.append(
            f"HEAP_ENV_SIZE too small: 0x{heap_env_size:X} < minimum 0x{config.heap_env_size:X}"
        )

    if heap_msg_size is not None and heap_msg_size < config.heap_msg_size:
        errors.append(
            f"HEAP_MSG_SIZE too small: 0x{heap_msg_size:X} < minimum 0x{config.heap_msg_size:X}"
        )

    # Warnings
    if total_connections < config.connections_max:
        warnings.append(
            f"Using only {total_connections} of {config.connections_max} available connections. "
            f"Consider {BLELibraryType.LITE.lib_file if total_connections <= 1 else BLELibraryType.STANDARD.lib_file} for better RAM optimization."
        )

    is_valid = len(errors) == 0
    return is_valid, errors + warnings


# Convenience function for AI to call
def analyze_requirements_and_recommend(
    user_description: str,
    num_connections: Optional[int] = None,
    priority_ram: bool = False,
    priority_features: bool = False
) -> Dict:
    """
    AI-friendly function: Analyze user description and recommend BLE library

    Args:
        user_description: Natural language description of requirements
        num_connections: Optional explicit connection count
        priority_ram: Prioritize RAM usage
        priority_features: Prioritize maximum features

    Returns:
        Complete recommendation with configuration
    """
    description_lower = user_description.lower()

    # Extract hints from description
    if num_connections is None:
        # Try to infer from description
        if "multi" in description_lower or "multiple" in description_lower:
            num_connections = 3
        elif "gateway" in description_lower or "hub" in description_lower:
            num_connections = 6
        elif "mesh" in description_lower:
            num_connections = 6
        elif "mouse" in description_lower or "keyboard" in description_lower or "hid" in description_lower:
            num_connections = 1
        elif "sensor" in description_lower or "beacon" in description_lower:
            num_connections = 1
        else:
            num_connections = 1

    # Determine priority
    ram_optimization = priority_ram or any(keyword in description_lower for keyword in
                                          ["minimal ram", "low memory", "ram optimization", "cost sensitive"])

    max_features = priority_features or any(keyword in description_lower for keyword in
                                           ["max connections", "multi-device", "gateway", "hub", "mesh"])

    # Get recommendation
    if max_features and num_connections > 3:
        lib_type = BLELibraryType.FULL
    elif ram_optimization and num_connections <= 1:
        lib_type = BLELibraryType.LITE
    elif num_connections <= 1:
        # Check if application type suggests lite library
        if any(keyword in description_lower for keyword in ["mouse", "keyboard", "hid", "sensor", "beacon"]):
            lib_type = BLELibraryType.LITE
        else:
            lib_type = BLELibraryType.STANDARD
    elif num_connections <= 3:
        lib_type = BLELibraryType.STANDARD
    else:
        lib_type = BLELibraryType.FULL

    # Get full configuration
    recommendation = recommend_ble_library(
        num_connections=num_connections,
        ram_optimization=ram_optimization,
        application_type=user_description
    )

    # Add configuration details
    config = BLE_LIBRARIES[lib_type]
    recommendation["configuration_details"] = {
        "cfg_h_macros": generate_cfg_macros(lib_type),
        "makefile_config": generate_makefile_config(lib_type),
        "keil_config": generate_keil_project_config(lib_type)
    }

    recommendation["validation"] = {
        "is_valid": True,
        "warnings": [] if num_connections <= config.connections_max else
                   [f"Requested {num_connections} connections, library supports {config.connections_max}"]
    }

    return recommendation
