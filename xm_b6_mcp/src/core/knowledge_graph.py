"""
SDK Knowledge Graph Loader
==========================

Loads and provides access to the B6x SDK knowledge graph.
"""

import json
import logging
from pathlib import Path
from typing import Optional, Dict, List
from dataclasses import dataclass, field

logger = logging.getLogger(__name__)


# ============================================================================
# Data Models
# ============================================================================

@dataclass
class HardwareDependency:
    """Hardware dependency information"""
    required_peripherals: List[str] = field(default_factory=list)
    gpio_pins: Dict[str, Dict] = field(default_factory=dict)
    clocks: Dict[str, str] = field(default_factory=dict)
    interrupts: Dict[str, str] = field(default_factory=dict)
    dma: Dict[str, str] = field(default_factory=dict)


@dataclass
class SoftwareDependency:
    """Software dependency information"""
    required_headers: List[str] = field(default_factory=list)
    dependent_peripherals: List[str] = field(default_factory=list)
    middlewares: List[str] = field(default_factory=list)
    libraries: List[str] = field(default_factory=list)


@dataclass
class ExampleCode:
    """Example code metadata"""
    id: str
    name: str
    path: str
    difficulty: str
    features: List[str] = field(default_factory=list)
    verified: bool = False
    mcu_models: List[str] = field(default_factory=list)
    apis_used: List[str] = field(default_factory=list)


@dataclass
class PeripheralConfig:
    """Peripheral configuration options"""
    baudrates: List[int] = field(default_factory=list)
    data_bits: List[int] = field(default_factory=list)
    parity_options: List[str] = field(default_factory=list)
    stop_bits: List[int] = field(default_factory=list)


@dataclass
class PeripheralConstraints:
    """Peripheral constraints"""
    max_baudrate: Optional[int] = None
    min_baudrate: Optional[int] = None
    max_instances: Optional[int] = None
    pin_conflicts: List[str] = field(default_factory=list)


@dataclass
class Peripheral:
    """Peripheral information from knowledge graph"""
    name: str
    description: str
    instances: List[str] = field(default_factory=list)
    category: str = "general"

    # Driver files
    driver_files: Dict[str, str] = field(default_factory=dict)
    apis: Dict[str, str] = field(default_factory=dict)

    # Examples
    examples: List[ExampleCode] = field(default_factory=list)

    # Dependencies
    hardware_dependencies: HardwareDependency = field(default_factory=HardwareDependency)
    software_dependencies: SoftwareDependency = field(default_factory=SoftwareDependency)

    # Configuration and constraints
    configuration: PeripheralConfig = field(default_factory=PeripheralConfig)
    constraints: PeripheralConstraints = field(default_factory=PeripheralConstraints)

    # Documentation
    documentation: Dict[str, str] = field(default_factory=dict)

    # Relationships
    related_peripherals: List[str] = field(default_factory=list)
    conflicts_with: List[str] = field(default_factory=list)


@dataclass
class SDKKnowledgeGraph:
    """Complete SDK knowledge graph"""
    peripherals: Dict[str, Peripheral] = field(default_factory=dict)
    version: str = "1.0"
    sdk_path: str = ""
    build_date: str = ""
    metadata: Dict = field(default_factory=dict)

    def get_peripheral(self, name: str) -> Optional[Peripheral]:
        """Get peripheral by name (case-insensitive)"""
        return self.peripherals.get(name.upper())

    def find_examples_by_difficulty(
        self,
        peripheral: str,
        difficulty: str
    ) -> List[ExampleCode]:
        """Find examples by difficulty level"""
        p = self.get_peripheral(peripheral)
        if not p:
            return []
        return [ex for ex in p.examples if ex.difficulty == difficulty]

    def check_pin_conflict(
        self,
        pin: str,
        configured_peripherals: List[str]
    ) -> bool:
        """Check if a pin conflicts with configured peripherals"""
        for periph_name in configured_peripherals:
            periph = self.get_peripheral(periph_name)
            if not periph:
                continue

            for instance, pins in periph.hardware_dependencies.gpio_pins.items():
                for func, pin_list in pins.items():
                    if isinstance(pin_list, list) and pin in pin_list:
                        return True
        return False

    def get_dependencies_chain(self, peripheral: str) -> Dict[str, List[str]]:
        """Get complete dependency chain for a peripheral"""
        periph = self.get_peripheral(peripheral)
        if not periph:
            return {}

        return {
            "hardware": periph.hardware_dependencies.required_peripherals,
            "software": periph.software_dependencies.required_headers
        }


# ============================================================================
# Graph Loader
# ============================================================================

def load_knowledge_graph(
    graph_path: str,
    auto_build: bool = False
) -> Optional[SDKKnowledgeGraph]:
    """
    Load SDK knowledge graph from file

    Args:
        graph_path: Path to knowledge graph JSON file
        auto_build: If True, attempt to build graph if not found

    Returns:
        SDKKnowledgeGraph instance or None if failed
    """
    graph_file = Path(graph_path)

    if not graph_file.exists():
        logger.warning(f"Knowledge graph not found: {graph_path}")

        if auto_build:
            logger.info("Attempting to build knowledge graph...")
            try:
                from sdk_indexer import build_knowledge_graph

                sdk_path = str(graph_file.parent.parent)
                build_knowledge_graph(sdk_path, str(graph_file))

                if graph_file.exists():
                    logger.info("Knowledge graph built successfully")
                else:
                    return None
            except ImportError:
                logger.error("Cannot build graph: sdk_indexer not available")
                return None
            except Exception as e:
                logger.error(f"Failed to build graph: {e}")
                return None
        else:
            return None

    try:
        logger.info(f"Loading knowledge graph from {graph_path}")

        with open(graph_file, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Convert dict to dataclass objects
        graph = dict_to_graph(data)

        logger.info(f"Loaded {len(graph.peripherals)} peripherals")

        return graph

    except json.JSONDecodeError as e:
        logger.error(f"Invalid JSON in knowledge graph: {e}")
        return None
    except Exception as e:
        logger.error(f"Failed to load knowledge graph: {e}")
        return None


def dict_to_graph(data: dict) -> SDKKnowledgeGraph:
    """Convert dictionary to SDKKnowledgeGraph object"""
    graph = SDKKnowledgeGraph(
        version=data.get("version", "1.0"),
        sdk_path=data.get("sdk_path", ""),
        build_date=data.get("build_date", ""),
        metadata=data.get("metadata", {})
    )

    # Convert peripherals
    for periph_name, periph_data in data.get("peripherals", {}).items():
        peripheral = Peripheral(
            name=periph_data.get("name", periph_name),
            description=periph_data.get("description", ""),
            instances=periph_data.get("instances", []),
            category=periph_data.get("category", "general"),
            driver_files=periph_data.get("driver_files", {}),
            apis=periph_data.get("apis", {}),
            documentation=periph_data.get("documentation", {}),
            related_peripherals=periph_data.get("related_peripherals", []),
            conflicts_with=periph_data.get("conflicts_with", [])
        )

        # Convert examples
        for ex_data in periph_data.get("examples", []):
            example = ExampleCode(
                id=ex_data.get("id", ""),
                name=ex_data.get("name", ""),
                path=ex_data.get("path", ""),
                difficulty=ex_data.get("difficulty", "basic"),
                features=ex_data.get("features", []),
                verified=ex_data.get("verified", False),
                mcu_models=ex_data.get("mcu_models", []),
                apis_used=ex_data.get("apis_used", [])
            )
            peripheral.examples.append(example)

        # Convert hardware dependencies
        hw_data = periph_data.get("hardware_dependencies", {})
        peripheral.hardware_dependencies = HardwareDependency(
            required_peripherals=hw_data.get("required_peripherals", []),
            gpio_pins=hw_data.get("gpio_pins", {}),
            clocks=hw_data.get("clocks", {}),
            interrupts=hw_data.get("interrupts", {}),
            dma=hw_data.get("dma", {})
        )

        # Convert software dependencies
        sw_data = periph_data.get("software_dependencies", {})
        peripheral.software_dependencies = SoftwareDependency(
            required_headers=sw_data.get("required_headers", []),
            dependent_peripherals=sw_data.get("dependent_peripherals", []),
            middlewares=sw_data.get("middlewares", []),
            libraries=sw_data.get("libraries", [])
        )

        # Convert configuration
        config_data = periph_data.get("configuration", {})
        peripheral.configuration = PeripheralConfig(
            baudrates=config_data.get("baudrates", []),
            data_bits=config_data.get("data_bits", []),
            parity_options=config_data.get("parity", []),
            stop_bits=config_data.get("stop_bits", [])
        )

        # Convert constraints
        constraints_data = periph_data.get("constraints", {})
        peripheral.constraints = PeripheralConstraints(
            max_baudrate=constraints_data.get("max_baudrate"),
            min_baudrate=constraints_data.get("min_baudrate"),
            max_instances=constraints_data.get("max_instances"),
            pin_conflicts=constraints_data.get("pin_conflicts", [])
        )

        graph.peripherals[periph_name] = peripheral

    return graph


# ============================================================================
# Graph Cache
# ============================================================================

_graph_cache: Optional[SDKKnowledgeGraph] = None


def get_cached_graph() -> Optional[SDKKnowledgeGraph]:
    """Get cached knowledge graph"""
    return _graph_cache


def set_cached_graph(graph: SDKKnowledgeGraph):
    """Set cached knowledge graph"""
    global _graph_cache
    _graph_cache = graph


def clear_cached_graph():
    """Clear cached knowledge graph"""
    global _graph_cache
    _graph_cache = None
