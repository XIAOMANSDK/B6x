"""
Cross-Domain Relation Mapper - Building Knowledge Graph Relationships
=====================================================================

Constructs the "four-dimensional" relationships between domains:
- Hardware ↔ Drivers: API controls registers
- Drivers ↔ BLE: BLE uses driver APIs
- All → Applications: API usage in examples

This module integrates data from all parsers to build a unified
relationship graph that enables AI agents to reason across domains.

AI Use Cases:
- Trace API call chain to register level
- Find all examples using a specific peripheral
- Identify dependencies between BLE and drivers
- Generate complete initialization sequences
"""

import re
import json
import logging
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Set, Tuple, Any
from collections import defaultdict

# Import schema types
from .knowledge_graph_schema import (
    DomainType, RelationType, CrossDomainRelation, KnowledgeGraphEntry
)

logger = logging.getLogger(__name__)


# ============================================================================
# Relationship Data Structures
# ============================================================================

@dataclass
class RelationTriple:
    """A single relationship: subject → predicate → object."""
    subject_id: str
    predicate: RelationType
    object_id: str
    subject_domain: DomainType
    object_domain: DomainType
    confidence: float = 1.0
    metadata: Dict = field(default_factory=dict)

    def to_dict(self) -> Dict:
        return {
            "subject_id": self.subject_id,
            "predicate": self.predicate.value,
            "object_id": self.object_id,
            "subject_domain": self.subject_domain,
            "object_domain": self.object_domain,
            "confidence": self.confidence,
            "metadata": self.metadata
        }


@dataclass
class DomainRelations:
    """Relations for a specific domain."""
    domain: DomainType
    outgoing_relations: List[RelationTriple] = field(default_factory=list)
    incoming_relations: List[RelationTriple] = field(default_factory=list)

    def get_relations_to_domain(self, target_domain: DomainType) -> List[RelationTriple]:
        """Get all relations pointing to a specific domain."""
        return [r for r in self.outgoing_relations if r.object_domain == target_domain]

    def get_relations_from_domain(self, source_domain: DomainType) -> List[RelationTriple]:
        """Get all relations from a specific domain."""
        return [r for r in self.incoming_relations if r.subject_domain == source_domain]


@dataclass
class RelationGraph:
    """Complete cross-domain relation graph."""
    domains: Dict[DomainType, DomainRelations] = field(default_factory=dict)
    all_relations: List[RelationTriple] = field(default_factory=list)
    entity_index: Dict[str, str] = field(default_factory=dict)  # name -> domain mapping

    def add_relation(self, relation: RelationTriple) -> None:
        """Add a relation to the graph."""
        self.all_relations.append(relation)

        # Update domain relations
        if relation.subject_domain not in self.domains:
            self.domains[relation.subject_domain] = DomainRelations(domain=relation.subject_domain)
        if relation.object_domain not in self.domains:
            self.domains[relation.object_domain] = DomainRelations(domain=relation.object_domain)

        self.domains[relation.subject_domain].outgoing_relations.append(relation)
        self.domains[relation.object_domain].incoming_relations.append(relation)

        # Update entity index
        self.entity_index[relation.subject_id] = relation.subject_domain
        self.entity_index[relation.object_id] = relation.object_domain

    def get_entity_domain(self, entity_id: str) -> Optional[DomainType]:
        """Get the domain of an entity."""
        return self.entity_index.get(entity_id)

    def get_path(
        self,
        start_entity: str,
        end_entity: str,
        max_depth: int = 3
    ) -> List[List[str]]:
        """Find relation paths between two entities (BFS)."""
        paths = []
        queue = [(start_entity, [start_entity])]
        visited = set()

        while queue and len(paths) < 10:
            current, path = queue.pop(0)

            if len(path) > max_depth + 1:
                continue

            if current == end_entity and len(path) > 1:
                paths.append(path)
                continue

            if current in visited:
                continue
            visited.add(current)

            # Find all relations from current entity
            for rel in self.all_relations:
                if rel.subject_id == current:
                    queue.append((rel.object_id, path + [rel.object_id]))

        return paths

    def to_dict(self) -> Dict:
        """Export graph to dictionary."""
        return {
            "total_relations": len(self.all_relations),
            "domains": {
                d: {
                    "outgoing": len(dr.outgoing_relations),
                    "incoming": len(dr.incoming_relations)
                }
                for d, dr in self.domains.items()
            },
            "relations": [r.to_dict() for r in self.all_relations]
        }


# ============================================================================
# Cross-Domain Relation Mapper
# ============================================================================

class CrossDomainRelationMapper:
    """Main mapper for building cross-domain relationships."""

    def __init__(self):
        self.graph = RelationGraph()
        self.domain_data: Dict[DomainType, Dict] = {
            "hardware": {},
            "drivers": {},
            "ble": {},
            "applications": {}
        }

    # ========================================================================
    # Data Loading
    # ========================================================================

    def load_domain_data(self, domain: DomainType, data: Dict) -> None:
        """Load parsed data for a domain."""
        self.domain_data[domain] = data

    def load_from_files(self, sdk_path: str) -> None:
        """Load all domain data from exported JSON files."""
        data_dir = Path(sdk_path) / "xm_b6_mcp" / "data" / "domain"

        # Load each domain
        domain_files = {
            "hardware": ["registers.json", "memory_map.json"],
            "drivers": ["apis.json", "structs.json"],
            "ble": ["apis.json", "profiles.json"],
            "applications": ["examples.json"]
        }

        for domain, files in domain_files.items():
            domain_path = data_dir / domain
            for file_name in files:
                file_path = domain_path / file_name
                if file_path.exists():
                    with open(file_path, 'r', encoding='utf-8') as f:
                        data = json.load(f)
                        self.domain_data[domain].update(data)

    # ========================================================================
    # Relation Mapping: Hardware ↔ Drivers
    # ========================================================================

    def map_hardware_to_drivers(self) -> None:
        """Map Hardware registers to Driver APIs."""
        hw_data = self.domain_data.get("hardware", {})
        drv_data = self.domain_data.get("drivers", {})

        # Register → API mapping
        registers = hw_data.get("registers", [])
        apis = drv_data.get("apis", [])

        # Semantic name matching
        for reg in registers:
            reg_name = reg.get("name", "")

            # Find APIs that reference this register
            for api in apis:
                api_name = api.get("name", "")
                api_brief = api.get("brief", "")

                # Check for peripheral name match
                if self._peripheral_match(reg_name, api_name):
                    # Determine relation type
                    if "init" in api_name.lower() or "config" in api_name.lower():
                        rel_type = RelationType.CONFIGURES_REGISTER
                    elif "read" in api_name.lower():
                        rel_type = RelationType.READS_REGISTER
                    elif "write" in api_name.lower():
                        rel_type = RelationType.CONTROLS_REGISTER
                    else:
                        rel_type = RelationType.CONTROLS_REGISTER

                    self.graph.add_relation(RelationTriple(
                        subject_id=f"drivers:api:{api_name}",
                        predicate=rel_type,
                        object_id=f"hardware:register:{reg_name}",
                        subject_domain="drivers",
                        object_domain="hardware",
                        metadata={"peripheral": self._extract_peripheral(reg_name)}
                    ))

    def map_structs_to_registers(self) -> None:
        """Map driver config structs to registers."""
        drv_data = self.domain_data.get("drivers", {})
        hw_data = self.domain_data.get("hardware", {})

        structs = drv_data.get("structs", [])
        registers = hw_data.get("registers", [])

        for struct in structs:
            struct_name = struct.get("name", "")
            members = struct.get("members", [])

            # Check if struct is a peripheral config type
            if self._is_config_struct(struct_name):
                # Map each member to a register
                for member in members:
                    member_name = member.get("name", "")

                    # Find matching register
                    for reg in registers:
                        reg_name = reg.get("name", "")
                        if self._member_matches_register(member_name, reg_name):
                            self.graph.add_relation(RelationTriple(
                                subject_id=f"drivers:struct:{struct_name}.{member_name}",
                                predicate=RelationType.CONFIGURES_REGISTER,
                                object_id=f"hardware:register:{reg_name}",
                                subject_domain="drivers",
                                object_domain="hardware",
                                metadata={"offset": member.get("offset", 0)}
                            ))

    # ========================================================================
    # Relation Mapping: Drivers ↔ BLE
    # ========================================================================

    def map_ble_to_drivers(self) -> None:
        """Map BLE APIs to their driver dependencies."""
        ble_data = self.domain_data.get("ble", {})
        drv_data = self.domain_data.get("drivers", {})

        ble_apis = ble_data.get("apis", [])
        driver_apis = drv_data.get("apis", [])

        # Known BLE-Driver dependencies
        dependency_patterns = {
            "uart": ["ble_uart", "ble UART"],
            "dma": ["ble_dma", "BLE DMA"],
            "timer": ["ble_timer", "BLE timer"],
            "flash": ["ble_flash", "flash", "ota"],
        }

        for ble_api in ble_apis:
            ble_name = ble_api.get("name", "")
            ble_desc = ble_api.get("brief", "") + " " + ble_api.get("detailed", "")

            # Check for driver references in description
            for driver_peri, patterns in dependency_patterns.items():
                if any(p.lower() in ble_desc.lower() for p in patterns):
                    # Find matching driver APIs
                    for drv_api in driver_apis:
                        drv_name = drv_api.get("name", "")
                        if driver_peri in drv_name.lower():
                            self.graph.add_relation(RelationTriple(
                                subject_id=f"ble:api:{ble_name}",
                                predicate=RelationType.USES_DRIVER,
                                object_id=f"drivers:api:{drv_name}",
                                subject_domain="ble",
                                object_domain="drivers",
                                metadata={"reason": f"References {driver_peri}"}
                            ))

    def map_profiles_to_apis_with_ast(self, sdk_path: str) -> None:
        """Map BLE profiles to GATT APIs using AST analysis (P1 enhancement)."""
        from .profile_dependency_parser import ProfileDependencyParser

        logger.info("Mapping BLE profiles to APIs using AST analysis...")

        parser = ProfileDependencyParser(sdk_path)
        dependencies = parser.parse_profile_directory()

        # Create relations for each profile's API calls
        for profile_name, dep in dependencies.items():
            for call in dep.gatt_api_calls:
                self.graph.add_relation(RelationTriple(
                    subject_id=f"ble:profile:{profile_name}",
                    predicate=RelationType.IMPLEMENTS_PROFILE,
                    object_id=f"ble:api:{call.function_name}",
                    subject_domain="ble",
                    object_domain="ble",
                    metadata={
                        "source_file": dep.source_file,
                        "line_number": call.line_number,
                        "context": call.context
                    }
                ))

            for call in dep.driver_api_calls:
                self.graph.add_relation(RelationTriple(
                    subject_id=f"ble:profile:{profile_name}",
                    predicate=RelationType.USES_DRIVER,
                    object_id=f"drivers:api:{call.function_name}",
                    subject_domain="ble",
                    object_domain="drivers",
                    metadata={
                        "source_file": dep.source_file,
                        "line_number": call.line_number,
                        "call_type": "driver_api"
                    }
                ))

        logger.info(f"Mapped {len(dependencies)} profiles to APIs")

    # ========================================================================
    # Relation Mapping: Applications → All
    # ========================================================================

    def map_examples_to_apis(self) -> None:
        """Map example code to APIs they use."""
        app_data = self.domain_data.get("applications", {})
        drv_data = self.domain_data.get("drivers", {})
        ble_data = self.domain_data.get("ble", {})

        examples = app_data.get("examples", [])
        driver_apis = drv_data.get("apis", [])
        ble_apis = ble_data.get("apis", [])

        # Build API name sets for quick lookup
        drv_api_names = set(api.get("name", "") for api in driver_apis)
        ble_api_names = set(api.get("name", "") for api in ble_apis)

        for example in examples:
            example_name = example.get("name", "")
            used_apis = example.get("used_apis", [])

            for api_name in used_apis:
                # Determine domain
                if api_name in drv_api_names:
                    domain = "drivers"
                elif api_name in ble_api_names:
                    domain = "ble"
                else:
                    continue

                self.graph.add_relation(RelationTriple(
                    subject_id=f"applications:example:{example_name}",
                    predicate=RelationType.USES_DRIVER if domain == "drivers" else RelationType.DEMONSTRATES,
                    object_id=f"{domain}:api:{api_name}",
                    subject_domain="applications",
                    object_domain=domain,
                    metadata={"example_file": example.get("file_path", "")}
                ))

    def map_init_sequences(self) -> None:
        """Extract and map initialization sequences from examples."""
        app_data = self.domain_data.get("applications", {})
        call_chains = app_data.get("call_chains", {})

        # call_chains is a dict with project names as keys
        for example_name, chain_data in (call_chains.items() if isinstance(call_chains, dict) else []):
            if not isinstance(chain_data, dict):
                continue
            sequence = chain_data.get("init_sequence", [])

            # Create sequential relations
            for i in range(len(sequence) - 1):
                current = sequence[i]
                next_api = sequence[i + 1]

                # Determine domains
                current_domain = self._guess_api_domain(current)
                next_domain = self._guess_api_domain(next_api)

                if current_domain and next_domain:
                    self.graph.add_relation(RelationTriple(
                        subject_id=f"{current_domain}:api:{current}",
                        predicate=RelationType.CALLED_IN_SEQUENCE,
                        object_id=f"{next_domain}:api:{next_api}",
                        subject_domain=current_domain,
                        object_domain=next_domain,
                        metadata={
                            "sequence_order": i,
                            "example": example_name
                        }
                    ))

    def map_declarations_to_implementations(self, sdk_path: str) -> None:
        """Map API declarations (.h) to implementations (.c) using file mapping (P1 enhancement)."""
        from .source_file_mapper import SourceFileMapper

        logger.info("Mapping API declarations to implementations...")

        mapper = SourceFileMapper(sdk_path)
        mappings = mapper.build_mappings()

        # Create relations for each mapping
        for header_path, mapping in mappings.items():
            header_id = f"drivers:header:{Path(header_path).stem}"
            source_id = f"drivers:source:{Path(mapping.source_file).stem}"

            # Header → Source file relation
            self.graph.add_relation(RelationTriple(
                subject_id=header_id,
                predicate=RelationType.IMPLEMENTED_IN,
                object_id=source_id,
                subject_domain="drivers",
                object_domain="drivers",
                metadata={
                    "header_file": mapping.header_file,
                    "source_file": mapping.source_file,
                    "coverage": mapping.get_coverage()
                }
            ))

            # Individual API → implementation file relations
            for api_name in mapping.declared_apis:
                if api_name in mapping.implemented_apis:
                    self.graph.add_relation(RelationTriple(
                        subject_id=f"drivers:api:{api_name}",
                        predicate=RelationType.IMPLEMENTED_IN,
                        object_id=source_id,
                        subject_domain="drivers",
                        object_domain="drivers",
                        metadata={
                            "header_file": mapping.header_file,
                            "source_file": mapping.source_file
                        }
                    ))

        logger.info(f"Mapped {len(mappings)} header→source pairs")

    def map_power_to_apis(self, sdk_path: str) -> None:
        """Map power consumption data to BLE APIs (P2 enhancement)."""
        from .excel_parser import ExcelHardwareParser

        logger.info("Mapping power consumption to APIs...")

        parser = ExcelHardwareParser()
        power_file = Path(sdk_path) / "doc" / "SW_Spec" / "B6x_BLE-功耗参考.xlsx"

        if not power_file.exists():
            logger.warning(f"Power file not found: {power_file}")
            return

        power_entries = parser.parse_power_map(str(power_file))

        # Map power modes to APIs
        # Different BLE activities use different APIs
        mode_api_mapping = {
            'SLEEP': ['ke_sleep', 'ke_timer_set'],
            'DEEP_SLEEP': ['ke_sleep', 'ke_power_off'],
            'ACTIVE_ADVERTISING': ['gapm_start_advertise', 'gapm_update_advertise_data'],
            'ACTIVE_CONNECTED': ['gapc_connect', 'gapc_disconnect'],
            'ACTIVE_TX': ['gapm_start_advertise', 'ble_send_data'],
            'ACTIVE_RX': ['ke_msg_alloc', 'ke_msg_send'],
        }

        for entry in power_entries:
            # Find APIs related to this power mode
            mode = entry.mode

            # Create a power entity in hardware domain
            power_entity_id = f"hardware:power:{mode}"

            # Map to BLE APIs
            for api_name in mode_api_mapping.get(mode, []):
                self.graph.add_relation(RelationTriple(
                    subject_id=f"ble:api:{api_name}",
                    predicate=RelationType.CONSUMES_POWER,
                    object_id=power_entity_id,
                    subject_domain="ble",
                    object_domain="hardware",
                    metadata={
                        "mode": mode,
                        "current_ua": entry.current_ua,
                        "power_mw": entry.power_mw,
                        "conditions": entry.conditions
                    }
                ))

        logger.info(f"Mapped {len(power_entries)} power entries to APIs")

    # ========================================================================
    # Inference Rules
    # ========================================================================

    def infer_transitive_relations(self) -> None:
        """Infer transitive relations (e.g., A→B, B→C => A→C)."""
        new_relations = []

        # Application → BLE API → Driver API => Application → Driver API
        for rel1 in self.graph.all_relations:
            if rel1.subject_domain == "applications" and rel1.object_domain == "ble":
                # Find BLE API → Driver relations
                for rel2 in self.graph.all_relations:
                    if (rel2.subject_domain == "ble" and
                        rel2.object_domain == "drivers" and
                        rel1.object_id == rel2.subject_id):

                        # Create inferred relation
                        inferred = RelationTriple(
                            subject_id=rel1.subject_id,
                            predicate=RelationType.USES_DRIVER,
                            object_id=rel2.object_id,
                            subject_domain="applications",
                            object_domain="drivers",
                            confidence=0.8,  # Lower confidence for inferred
                            metadata={"inferred": True, "path": [rel1.subject_id, rel2.object_id]}
                        )
                        new_relations.append(inferred)

        # Add inferred relations
        for rel in new_relations:
            # Check for duplicates
            exists = any(
                r.subject_id == rel.subject_id and
                r.object_id == rel.object_id and
                r.predicate == rel.predicate
                for r in self.graph.all_relations
            )
            if not exists:
                self.graph.add_relation(rel)

    # ========================================================================
    # Query Methods
    # ========================================================================

    def get_dependencies(self, entity_id: str) -> List[RelationTriple]:
        """Get all dependencies for an entity (outgoing relations)."""
        return [r for r in self.graph.all_relations if r.subject_id == entity_id]

    def get_dependents(self, entity_id: str) -> List[RelationTriple]:
        """Get all entities that depend on this one (incoming relations)."""
        return [r for r in self.graph.all_relations if r.object_id == entity_id]

    def get_call_chain(self, api_name: str, domain: DomainType) -> List[str]:
        """Get complete call chain for an API including register access."""
        chain = [api_name]

        # Find what this API calls
        for rel in self.graph.all_relations:
            if rel.subject_id == f"{domain}:api:{api_name}":
                # Extract target name
                target_name = rel.object_id.split(":")[-1]
                chain.append(target_name)

                # If it's a register, stop
                if rel.object_domain == "hardware":
                    break

        return chain

    def get_examples_using_api(self, api_name: str) -> List[str]:
        """Get all examples that use a specific API."""
        examples = []

        for rel in self.graph.all_relations:
            if (rel.predicate in [RelationType.USES_DRIVER, RelationType.DEMONSTRATES] and
                api_name in rel.object_id):
                example_name = rel.subject_id.split(":")[-1]
                examples.append(example_name)

        return examples

    def find_conflicts(self) -> List[Dict]:
        """Find potential conflicts (e.g., pin conflicts, resource conflicts)."""
        conflicts = []

        # Check for pin conflicts (hardware domain)
        # This would require analyzing pin_mux data

        # Check for API misuse (examples using wrong APIs)
        # This would require domain knowledge

        return conflicts

    # ========================================================================
    # Export
    # ========================================================================

    def export_to_json(self, output_path: str, indent: int = 2) -> None:
        """Export relation graph to JSON."""
        data = self.graph.to_dict()

        # Add statistics
        data["statistics"] = self._get_statistics()

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=indent, ensure_ascii=False)

    def export_for_domain(self, domain: DomainType, output_path: str) -> None:
        """Export relations for a specific domain."""
        if domain not in self.graph.domains:
            return

        domain_rels = self.graph.domains[domain]

        data = {
            "domain": domain,
            "outgoing_relations": [r.to_dict() for r in domain_rels.outgoing_relations],
            "incoming_relations": [r.to_dict() for r in domain_rels.incoming_relations],
            "statistics": {
                "total_outgoing": len(domain_rels.outgoing_relations),
                "total_incoming": len(domain_rels.incoming_relations),
                "connected_domains": self._get_connected_domains(domain)
            }
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)

    def _get_statistics(self) -> Dict:
        """Get graph statistics."""
        stats = {
            "total_relations": len(self.graph.all_relations),
            "by_predicate": defaultdict(int),
            "by_domain_pair": defaultdict(int)
        }

        for rel in self.graph.all_relations:
            stats["by_predicate"][rel.predicate.value] += 1
            pair = f"{rel.subject_domain}->{rel.object_domain}"
            stats["by_domain_pair"][pair] += 1

        return dict(stats)

    def _get_connected_domains(self, domain: DomainType) -> List[DomainType]:
        """Get list of domains connected to the given domain."""
        connected = set()

        for rel in self.graph.all_relations:
            if rel.subject_domain == domain:
                connected.add(rel.object_domain)
            elif rel.object_domain == domain:
                connected.add(rel.subject_domain)

        return list(connected)

    # ========================================================================
    # Helper Methods
    # ========================================================================

    def _peripheral_match(self, reg_name: str, api_name: str) -> bool:
        """Check if register and API belong to same peripheral."""
        # Extract peripheral names
        reg_peri = self._extract_peripheral(reg_name)
        api_peri = self._extract_peripheral(api_name)

        return reg_peri and api_peri and reg_peri.lower() == api_peri.lower()

    def _extract_peripheral(self, name: str) -> Optional[str]:
        """Extract peripheral name from register or API name."""
        # Common patterns
        patterns = [
            r'(\w+)_\w+',           # PERIPHERAL_SOMETHING
            r'(\w+)(?=[A-Z])',      # peripheralSomething
        ]

        for pattern in patterns:
            match = re.match(pattern, name)
            if match:
                peri = match.group(1)
                # Filter out common prefixes
                if peri not in ['ATT', 'GATT', 'BLE', 'API', 'REG', 'HAL']:
                    return peri

        return None

    def _is_config_struct(self, struct_name: str) -> bool:
        """Check if struct is a configuration type."""
        config_indicators = ['config', 'init', 'cfg', 'conf', 'setup']
        return any(ind in struct_name.lower() for ind in config_indicators)

    def _member_matches_register(self, member_name: str, reg_name: str) -> bool:
        """Check if struct member matches a register."""
        # Direct name match
        if member_name.lower() in reg_name.lower():
            return True

        # Abbreviated match
        member_abbr = ''.join(c for c in member_name if c.isupper())
        reg_abbr = ''.join(c for c in reg_name if c.isupper())
        return member_abbr and member_abbr == reg_abbr

    def _guess_api_domain(self, api_name: str) -> Optional[DomainType]:
        """Guess which domain an API belongs to based on name."""
        name_lower = api_name.lower()

        if any(p in name_lower for p in ['uart', 'gpio', 'spi', 'i2c', 'timer', 'dma', 'adc']):
            return "drivers"
        elif any(p in name_lower for p in ['ble', 'gatt', 'gap', 'ble_']):
            return "ble"
        elif any(p in name_lower for p in ['app', 'main', 'example']):
            return "applications"

        return None


# ============================================================================
# Utility Functions
# ============================================================================

def build_relation_graph(sdk_path: str) -> RelationGraph:
    """Build complete cross-domain relation graph."""
    mapper = CrossDomainRelationMapper()
    mapper.load_from_files(sdk_path)

    # Map all domain relationships
    mapper.map_hardware_to_drivers()
    mapper.map_structs_to_registers()
    mapper.map_ble_to_drivers()
    mapper.map_examples_to_apis()
    mapper.map_init_sequences()

    # Infer transitive relations
    mapper.infer_transitive_relations()

    return mapper.graph


def export_all_relations(sdk_path: str, output_dir: str) -> None:
    """Export relation graphs for all domains."""
    mapper = CrossDomainRelationMapper()
    mapper.load_from_files(sdk_path)

    # Build all relations
    mapper.map_hardware_to_drivers()
    mapper.map_structs_to_registers()
    mapper.map_ble_to_drivers()
    mapper.map_examples_to_apis()
    mapper.map_init_sequences()
    mapper.infer_transitive_relations()

    # Export full graph
    output_path = Path(output_dir)
    mapper.export_to_json(str(output_path / "relations.json"))

    # Export per-domain
    for domain in ["hardware", "drivers", "ble", "applications"]:
        mapper.export_for_domain(domain, str(output_path / f"relations_{domain}.json"))


# ============================================================================
# Main for Testing
# ============================================================================

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        sdk_path = sys.argv[1]
    else:
        sdk_path = "."

    mapper = CrossDomainRelationMapper()

    print(f"\n{'='*70}")
    print(f"Cross-Domain Relation Mapper")
    print(f"{'='*70}")

    # Build relations (would need actual data files)
    print("\nNote: This mapper requires exported domain data files.")
    print("Use build_index.py to generate domain data first.")

    print("\nTo use:")
    print("  1. Run build_index.py to generate domain JSON files")
    print("  2. Run this script with SDK path as argument")
    print("  3. Relation graphs will be exported to data/relations/")
