"""
Domain Configuration for Four-Dimensional Knowledge Graph
==========================================================

Defines the four domains and their build configurations:
1. Hardware  - registers, pin mux, memory map, interrupts
2. Drivers   - APIs, structs, dependencies
3. BLE       - APIs, profiles, error codes
4. Apps      - examples, call chains, configs
"""

from dataclasses import dataclass
from typing import Dict, List, Optional
from pathlib import Path
from enum import Enum


class DomainType(Enum):
    """Four domain types"""
    HARDWARE = "hardware"
    DRIVERS = "drivers"
    BLE = "ble"
    APPLICATIONS = "applications"


@dataclass
class DomainConfig:
    """Configuration for a single domain"""
    name: str
    domain_type: DomainType
    source_dirs: List[str]
    parser_types: List[str]
    output_files: List[str]
    dependencies: List[str] = None

    def __post_init__(self):
        if self.dependencies is None:
            self.dependencies = []


# Domain Registry
DOMAIN_REGISTRY: Dict[str, DomainConfig] = {
    "hardware": DomainConfig(
        name="Hardware & Registers",
        domain_type=DomainType.HARDWARE,
        source_dirs=[
            "core/B6x.SVD",           # SVD file for registers
            "doc/SW_Spec",             # Excel specs for pin mux, memory
        ],
        parser_types=["svd", "excel", "linker", "interrupt"],
        output_files=[
            "data/domain/hardware/pin_mux.json",
            "data/domain/hardware/pin_mux_metadata.json",
            "data/domain/hardware/interrupts.json",
            "data/domain/hardware/memory_boundaries.json",
            "data/domain/hardware/memory_boundaries_metadata.json",
            "data/domain/hardware/memory_regions.json",
            "data/domain/hardware/memory_regions_metadata.json",
            "data/domain/hardware/sram_regions.json",
            "data/domain/hardware/sram_regions_metadata.json",
            "data/domain/hardware/power_consumption.json",
            "data/domain/hardware/power_consumption_metadata.json",
            "data/domain/hardware/registers.json",
        ],
        dependencies=[]
    ),
    "drivers": DomainConfig(
        name="SOC Drivers",
        domain_type=DomainType.DRIVERS,
        source_dirs=[
            "drivers/api",            # Driver API headers
            "drivers/src",            # Driver implementations
        ],
        parser_types=["tree_sitter", "struct", "dependency"],
        output_files=[
            "data/domain/drivers/apis.json",
            "data/domain/drivers/structs.json",
            "data/domain/drivers/enums.json",
            "data/domain/drivers/macros.json",
            "data/domain/drivers/dependencies.json",
            "data/domain/drivers/call_graph.json",
            "data/domain/drivers/dependency_tree.json",
        ],
        dependencies=["hardware"]
    ),
    "ble": DomainConfig(
        name="BLE Stack",
        domain_type=DomainType.BLE,
        source_dirs=[
            "ble/api",                # BLE API headers
            "ble/prf",                # GATT profiles
            "ble/lib",                # BLE libraries
        ],
        parser_types=["tree_sitter", "profile", "error_code", "mesh"],
        output_files=[
            "data/domain/ble/apis.json",
            "data/domain/ble/profiles.json",
            "data/domain/ble/error_codes.json",
            "data/domain/ble/mesh_apis.json",
            "data/domain/ble/mesh_models.json",
            "data/domain/ble/mesh_error_codes.json",
            "data/domain/ble/phone_compatibility_issues.json",
            "data/domain/ble/phone_compatibility_metadata.json",
        ],
        dependencies=["hardware", "drivers"]
    ),
    "applications": DomainConfig(
        name="Applications",
        domain_type=DomainType.APPLICATIONS,
        source_dirs=[
            "examples",               # Example projects
            "projects",               # Sample projects
        ],
        parser_types=["example_scanner", "call_chain", "config"],
        output_files=[
            "data/domain/applications/examples.json",
            "data/domain/applications/call_chains.json",
            "data/domain/applications/init_sequences.json",
            "data/domain/applications/config_summary.json",
            "data/domain/applications/known_init_sequences.json",
            "data/domain/applications/project_configs.json",
        ],
        dependencies=["drivers", "ble"]
    ),
}


def get_all_domains() -> List[DomainConfig]:
    """Get all domain configurations in dependency order"""
    domains = []
    for key in ["hardware", "drivers", "ble", "applications"]:
        if key in DOMAIN_REGISTRY:
            domains.append(DOMAIN_REGISTRY[key])
    return domains


def get_domain(name: str) -> Optional[DomainConfig]:
    """Get a specific domain configuration by name"""
    return DOMAIN_REGISTRY.get(name)


def get_domain_by_type(domain_type: DomainType) -> Optional[DomainConfig]:
    """Get a domain configuration by type"""
    for config in DOMAIN_REGISTRY.values():
        if config.domain_type == domain_type:
            return config
    return None
