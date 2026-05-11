"""
Knowledge Graph Query Interface - AI Agent Access Layer
=========================================================

Provides high-level query interface for AI agents to access the
four-dimensional knowledge graph. Each query method is tailored
for specific AI use cases.

Domain-Specific Queries:
- Hardware: Pin conflicts, memory regions, interrupt vectors
- Drivers: Init APIs, config structs, dependencies
- BLE: GATT services, error codes, profile configs
- Applications: Example code, call chains, init sequences

Cross-Domain Queries:
- Full call chains (API → register level)
- Dependency resolution
- Example lookup

AI Use Cases:
- Generate initialization code
- Resolve pin conflicts
- Find usage examples
- Trace API dependencies
"""

import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional, Set, Tuple, Any
from collections import defaultdict

from .knowledge_graph_schema import (
    DomainType, EntryType, RelationType, KnowledgeGraphEntry
)


# ============================================================================
# Query Result Structures
# ============================================================================

@dataclass
class QueryResult:
    """Generic query result with metadata."""
    success: bool
    data: Any
    message: str = ""
    source_domain: Optional[DomainType] = None
    confidence: float = 1.0
    related_entries: List[Dict] = None

    def to_dict(self) -> Dict:
        return {
            "success": self.success,
            "data": self.data,
            "message": self.message,
            "source_domain": self.source_domain,
            "confidence": self.confidence,
            "related_entries": self.related_entries or []
        }


# ============================================================================
# Knowledge Graph Query Interface
# ============================================================================

class KnowledgeGraphQuery:
    """
    Main query interface for the 4D knowledge graph.

    Provides semantic, domain-specific, and cross-domain queries
    optimized for AI agent consumption.
    """

    def __init__(self, knowledge_graph_path: Optional[str] = None, sdk_path: Optional[str] = None):
        """
        Initialize query interface.

        Args:
            knowledge_graph_path: Path to knowledge_graph.json
            sdk_path: SDK root path (will use default domain data paths)
        """
        self.kg_path = knowledge_graph_path
        self.sdk_path = sdk_path

        # Loaded data
        self.knowledge_graph: Optional[Dict] = None
        self.domain_data: Dict[DomainType, Dict] = {d: {} for d in ["hardware", "drivers", "ble", "applications"]}
        self.relation_graph: Optional[Dict] = None

        # Index structures for fast lookup
        self.pin_mux_index: Dict[str, List[Dict]] = {}
        self.api_index: Dict[str, Dict] = {}
        self.service_index: Dict[str, Dict] = {}
        self.example_index: Dict[str, List[Dict]] = {}

        if knowledge_graph_path:
            self.load_knowledge_graph(knowledge_graph_path)
        elif sdk_path:
            self.load_from_sdk_path(sdk_path)

    # ========================================================================
    # Data Loading
    # ========================================================================

    def load_knowledge_graph(self, kg_path: str) -> None:
        """Load unified knowledge graph from JSON."""
        kg_file = Path(kg_path)
        if not kg_file.exists():
            raise FileNotFoundError(f"Knowledge graph not found: {kg_path}")

        with open(kg_file, 'r', encoding='utf-8') as f:
            self.knowledge_graph = json.load(f)

        # Extract domain data
        if "domains" in self.knowledge_graph:
            for domain, data in self.knowledge_graph["domains"].items():
                if domain in self.domain_data:
                    self.domain_data[domain] = data

        # Extract relations
        if "relations" in self.knowledge_graph:
            self.relation_graph = {"relations": self.knowledge_graph["relations"]}

        self._build_indexes()

    def load_from_sdk_path(self, sdk_path: str) -> None:
        """Load domain data from SDK exported files."""
        self.sdk_path = sdk_path
        base_path = Path(sdk_path) / "xm_b6_mcp" / "data"

        # Load each domain
        domain_dirs = {
            "hardware": base_path / "domain" / "hardware",
            "drivers": base_path / "domain" / "drivers",
            "ble": base_path / "domain" / "ble",
            "applications": base_path / "domain" / "applications"
        }

        for domain, dir_path in domain_dirs.items():
            if dir_path.exists():
                for json_file in dir_path.glob("*.json"):
                    with open(json_file, 'r', encoding='utf-8') as f:
                        data = json.load(f)
                        key = json_file.stem
                        if isinstance(data, dict):
                            self.domain_data[domain].update(data)
                        elif isinstance(data, list):
                            self.domain_data[domain][key] = data
                        else:
                            logger.warning(
                                f"Unexpected data type in {json_file.name}: {type(data)}"
                            )

        # Load relations
        rel_path = base_path / "relations.json"
        if rel_path.exists():
            with open(rel_path, 'r', encoding='utf-8') as f:
                self.relation_graph = json.load(f)

        self._build_indexes()

    def _build_indexes(self) -> None:
        """Build in-memory indexes for fast queries."""
        # Pin mux index
        if "pin_mux" in self.domain_data["hardware"]:
            for pin in self.domain_data["hardware"].get("pin_mux", []):
                pin_name = pin.get("pin_name", "") or pin.get("pin", "")
                if pin_name:
                    if pin_name not in self.pin_mux_index:
                        self.pin_mux_index[pin_name] = []
                    self.pin_mux_index[pin_name].append(pin)

        # API index
        for api in self.domain_data["drivers"].get("apis", []):
            api_name = api.get("name", "")
            if api_name:
                self.api_index[api_name] = api

        # Service index
        for svc in self.domain_data["ble"].get("profiles", []):
            uuid = svc.get("uuid", "")
            name = svc.get("name", "")
            if uuid:
                self.service_index[uuid] = svc
            if name:
                self.service_index[name] = svc

        # Example index
        for ex in self.domain_data["applications"].get("examples", []):
            for api in ex.get("used_apis", []):
                if api not in self.example_index:
                    self.example_index[api] = []
                self.example_index[api].append(ex)

    # ========================================================================
    # Domain 1: Hardware Queries
    # ========================================================================

    def get_pin_mux(self, pin: str) -> QueryResult:
        """
        Query: What functions can this pin perform?

        Args:
            pin: Pin name (e.g., "PA00", "PB15")

        Returns:
            QueryResult with list of possible functions
        """
        pin_upper = pin.upper().strip()

        mux_options = self.pin_mux_index.get(pin_upper, [])

        if not mux_options:
            # Try to find in raw data
            hw_data = self.domain_data.get("hardware", {})
            all_pins = hw_data.get("pin_mux", [])
            mux_options = [p for p in all_pins if p.get("pin", "").upper() == pin_upper]

        return QueryResult(
            success=len(mux_options) > 0,
            data=mux_options,
            message=f"Found {len(mux_options)} mux options for {pin}",
            source_domain="hardware"
        )

    def check_pin_conflict(self, pins: List[str], functions: List[str]) -> QueryResult:
        """
        Query: Do these pin assignments conflict?

        Args:
            pins: List of pin names
            functions: List of desired functions

        Returns:
            QueryResult with conflict analysis
        """
        conflicts = []
        used_pins = set()

        for i, (pin, func) in enumerate(zip(pins, functions)):
            result = self.get_pin_mux(pin)
            if result.success:
                available_funcs = [f.get("function", "") for f in result.data]
                if func not in available_funcs:
                    conflicts.append({
                        "pin": pin,
                        "requested_function": func,
                        "available_functions": available_funcs,
                        "index": i
                    })

            # Check for duplicate pins
            if pin in used_pins:
                conflicts.append({
                    "pin": pin,
                    "issue": "duplicate_pin",
                    "message": f"Pin {pin} used multiple times"
                })
            used_pins.add(pin)

        return QueryResult(
            success=len(conflicts) == 0,
            data={"conflicts": conflicts, "total_pins": len(pins)},
            message=f"Found {len(conflicts)} conflict(s)" if conflicts else "No conflicts",
            source_domain="hardware"
        )

    def get_memory_region(self, address: int) -> QueryResult:
        """
        Query: Which memory region does this address belong to?

        Args:
            address: Memory address (integer)

        Returns:
            QueryResult with region information
        """
        regions = self.domain_data["hardware"].get("memory_regions", [])

        for region in regions:
            start = region.get("origin", 0)
            end = start + region.get("length", 0)
            if start <= address < end:
                return QueryResult(
                    success=True,
                    data=region,
                    message=f"Address 0x{address:08X} in {region.get('name', 'Unknown')}",
                    source_domain="hardware"
                )

        return QueryResult(
            success=False,
            data=None,
            message=f"Address 0x{address:08X} not in any defined region",
            source_domain="hardware"
        )

    def check_memory_overlap(self, address: int, size: int) -> QueryResult:
        """
        Query: Does this memory range overlap with defined regions?

        Args:
            address: Start address
            size: Size in bytes

        Returns:
            QueryResult with overlap analysis
        """
        start = address
        end = address + size

        regions = self.domain_data["hardware"].get("memory_regions", [])
        overlaps = []

        for region in regions:
            reg_start = region.get("origin", 0)
            reg_end = reg_start + region.get("length", 0)

            # Check for overlap
            if not (end <= reg_start or start >= reg_end):
                overlaps.append({
                    "region": region.get("name"),
                    "overlap_start": max(start, reg_start),
                    "overlap_end": min(end, reg_end),
                    "overlap_size": min(end, reg_end) - max(start, reg_start)
                })

        return QueryResult(
            success=len(overlaps) == 0,
            data={"overlaps": overlaps, "range": {"start": start, "end": end, "size": size}},
            message=f"Found {len(overlaps)} overlap(s)" if overlaps else "No overlaps",
            source_domain="hardware"
        )

    def get_interrupt_handler(self, irq_name: str) -> QueryResult:
        """
        Query: What is the handler for this interrupt?

        Args:
            irq_name: Interrupt name (e.g., "UART1_IRQHandler")

        Returns:
            QueryResult with handler information
        """
        vectors = self.domain_data["hardware"].get("interrupts", [])

        for vec in vectors:
            if vec.get("name", "") == irq_name or vec.get("handler_name", "") == irq_name:
                return QueryResult(
                    success=True,
                    data=vec,
                    message=f"Found interrupt vector: {irq_name}",
                    source_domain="hardware"
                )

        return QueryResult(
            success=False,
            data=None,
            message=f"Interrupt vector not found: {irq_name}",
            source_domain="hardware"
        )

    # ========================================================================
    # Domain 2: Driver Queries
    # ========================================================================

    def get_peripheral_init_api(self, peripheral: str) -> QueryResult:
        """
        Query: What is the initialization API for this peripheral?

        Args:
            peripheral: Peripheral name (e.g., "UART", "SPI", "GPIO")

        Returns:
            QueryResult with init API information
        """
        peri_lower = peripheral.lower()

        # Search APIs
        for api_name, api in self.api_index.items():
            if peri_lower in api_name.lower():
                # Check if it's an init function
                brief = api.get("brief", "").lower()
                if "init" in brief or api_name.lower().startswith("init"):
                    return QueryResult(
                        success=True,
                        data=api,
                        message=f"Found init API for {peripheral}",
                        source_domain="drivers"
                    )

        return QueryResult(
            success=False,
            data=None,
            message=f"No init API found for {peripheral}",
            source_domain="drivers"
        )

    def get_config_struct(self, peripheral: str) -> QueryResult:
        """
        Query: What is the configuration structure for this peripheral?

        Args:
            peripheral: Peripheral name

        Returns:
            QueryResult with struct definition
        """
        peri_lower = peripheral.lower()
        structs = self.domain_data["drivers"].get("structs", [])

        for struct in structs:
            name = struct.get("name", "")
            category = struct.get("category", "")

            if (peri_lower in name.lower() and
                category == "peripheral_config"):
                return QueryResult(
                    success=True,
                    data=struct,
                    message=f"Found config struct for {peripheral}",
                    source_domain="drivers"
                )

        return QueryResult(
            success=False,
            data=None,
            message=f"No config struct found for {peripheral}",
            source_domain="drivers"
        )

    def get_api_dependencies(self, api_name: str) -> QueryResult:
        """
        Query: What are the dependencies for this API?

        Args:
            api_name: API function name

        Returns:
            QueryResult with dependency information
        """
        api = self.api_index.get(api_name)
        if not api:
            return QueryResult(
                success=False,
                data=None,
                message=f"API not found: {api_name}",
                source_domain="drivers"
            )

        deps = api.get("dependencies", {})

        return QueryResult(
            success=True,
            data=deps,
            message=f"Found dependencies for {api_name}",
            source_domain="drivers"
        )

    # ========================================================================
    # Domain 3: BLE Queries
    # ========================================================================

    def get_gatt_service(self, uuid: str) -> QueryResult:
        """
        Query: Get GATT service by UUID

        Args:
            uuid: Service UUID (e.g., "0x180F", "0000180f-0000-1000-8000-00805f9b34fb")

        Returns:
            QueryResult with service definition
        """
        # Normalize UUID
        uuid_norm = uuid.lower()
        if not uuid_norm.startswith("0x"):
            uuid_norm = f"0x{uuid_norm}"

        service = self.service_index.get(uuid_norm)

        if service:
            return QueryResult(
                success=True,
                data=service,
                message=f"Found service: {service.get('name', 'Unknown')}",
                source_domain="ble"
            )

        return QueryResult(
            success=False,
            data=None,
            message=f"Service not found: {uuid}",
            source_domain="ble"
        )

    def get_characteristic_properties(self, char_uuid: str) -> QueryResult:
        """
        Query: What are the properties of this characteristic?

        Args:
            char_uuid: Characteristic UUID

        Returns:
            QueryResult with properties
        """
        # Search across all services
        for svc in self.domain_data["ble"].get("profiles", []):
            for char in svc.get("characteristics", []):
                if char.get("uuid", "") == char_uuid or char.get("uuid_hex", "") == char_uuid:
                    return QueryResult(
                        success=True,
                        data=char,
                        message=f"Found characteristic: {char.get('name', 'Unknown')}",
                        source_domain="ble"
                    )

        return QueryResult(
            success=False,
            data=None,
            message=f"Characteristic not found: {char_uuid}",
            source_domain="ble"
        )

    def get_ble_error_description(self, error_code: int) -> QueryResult:
        """
        Query: What does this BLE error code mean?

        Args:
            error_code: Error code value

        Returns:
            QueryResult with error description
        """
        errors = self.domain_data["ble"].get("error_codes", {})

        # Check different fields
        error_list = errors.get("error_codes", [])
        for err in error_list:
            if err.get("code") == error_code or err.get("value") == error_code:
                return QueryResult(
                    success=True,
                    data=err,
                    message=f"Error {error_code}: {err.get('name', 'Unknown')}",
                    source_domain="ble"
                )

        return QueryResult(
            success=False,
            data=None,
            message=f"Error code not found: {error_code}",
            source_domain="ble"
        )

    # ========================================================================
    # Domain 4: Application Queries
    # ========================================================================

    def get_example_for_api(self, api_name: str) -> QueryResult:
        """
        Query: Find examples that use this API

        Args:
            api_name: API function name

        Returns:
            QueryResult with list of examples
        """
        examples = self.example_index.get(api_name, [])

        return QueryResult(
            success=len(examples) > 0,
            data=examples,
            message=f"Found {len(examples)} example(s) using {api_name}",
            source_domain="applications"
        )

    def get_init_sequence(self, example_name: str) -> QueryResult:
        """
        Query: What is the initialization sequence for this example?

        Args:
            example_name: Example or project name

        Returns:
            QueryResult with init sequence
        """
        call_chains = self.domain_data["applications"].get("call_chains", [])

        for chain in call_chains:
            if chain.get("example_name", "") == example_name:
                sequence = chain.get("init_sequence", [])
                return QueryResult(
                    success=len(sequence) > 0,
                    data={"sequence": sequence, "example": chain},
                    message=f"Found init sequence with {len(sequence)} step(s)",
                    source_domain="applications"
                )

        return QueryResult(
            success=False,
            data=None,
            message=f"No init sequence found for: {example_name}",
            source_domain="applications"
        )

    # ========================================================================
    # Cross-Domain Queries
    # ========================================================================

    def get_full_call_chain(self, api_name: str, domain: DomainType = "drivers") -> QueryResult:
        """
        Query: Get complete call chain from API to register level

        Args:
            api_name: API function name
            domain: Domain of the API

        Returns:
            QueryResult with full call chain including registers
        """
        chain = [f"{domain}:api:{api_name}"]

        if not self.relation_graph:
            return QueryResult(
                success=False,
                data={"chain": chain},
                message="Relation graph not loaded",
                source_domain=domain
            )

        # Follow relations
        visited = set()
        current = chain[0]

        for _ in range(10):  # Max depth limit
            if current in visited:
                break
            visited.add(current)

            # Find outgoing relations
            found = False
            for rel in self.relation_graph.get("relations", []):
                if rel.get("source_id") == current:
                    next_id = rel.get("target_id")
                    chain.append(next_id)
                    current = next_id
                    found = True

                    # Stop at hardware
                    if "hardware" in next_id:
                        break

            if not found:
                break

        return QueryResult(
            success=len(chain) > 1,
            data={"chain": chain, "depth": len(chain)},
            message=f"Traced call chain with {len(chain)} level(s)",
            source_domain=domain
        )

    def resolve_dependencies(self, api_name: str) -> QueryResult:
        """
        Query: What are ALL dependencies (clock, GPIO, etc.) for this API?

        Args:
            api_name: API function name

        Returns:
            QueryResult with complete dependency tree
        """
        deps = {
            "api": api_name,
            "clocks": [],
            "gpio": [],
            "other_apis": [],
            "registers": []
        }

        # Get basic API dependencies
        api = self.api_index.get(api_name)
        if api:
            api_deps = api.get("dependencies", {})
            if api_deps.get("requires_clock"):
                deps["clocks"].append(api_deps["requires_clock"])
            if api_deps.get("requires_gpio"):
                deps["gpio"].append(api_deps["requires_gpio"])

        # Follow relation graph for more dependencies
        if self.relation_graph:
            for rel in self.relation_graph.get("relations", []):
                if api_name in rel.get("source_id", ""):
                    target = rel.get("target_id", "")
                    if "register" in target:
                        deps["registers"].append(target)
                    elif "api" in target:
                        deps["other_apis"].append(target)

        return QueryResult(
            success=True,
            data=deps,
            message=f"Resolved dependencies for {api_name}",
            confidence=0.9
        )

    # ========================================================================
    # Semantic Search
    # ========================================================================

    def search(self, query: str, domain: Optional[DomainType] = None) -> QueryResult:
        """
        Query: Semantic search across all domains

        Args:
            query: Search query string
            domain: Optional domain filter

        Returns:
            QueryResult with matching entries
        """
        query_lower = query.lower()
        results = []

        domains_to_search = [domain] if domain else ["hardware", "drivers", "ble", "applications"]

        for dom in domains_to_search:
            dom_data = self.domain_data.get(dom, {})

            # Search different data types
            for key, data in dom_data.items():
                if isinstance(data, list):
                    for item in data:
                        # Search in string fields
                        for field_name, field_value in item.items():
                            if isinstance(field_value, str) and query_lower in field_value.lower():
                                results.append({
                                    "domain": dom,
                                    "source_type": key,
                                    "entry": item,
                                    "matched_field": field_name
                                })
                                break

        return QueryResult(
            success=len(results) > 0,
            data={"results": results, "total": len(results)},
            message=f"Found {len(results)} result(s) for '{query}'"
        )

    # ========================================================================
    # P1 Enhancement: Source File Mapping Queries
    # ========================================================================

    def get_api_source_files(self, api_name: str) -> QueryResult:
        """
        Get source file location for an API (P1 enhancement).

        Args:
            api_name: Name of the API function

        Returns:
            QueryResult with header_file and source_file paths
        """
        # Search in relations for IMPLEMENTED_IN
        if not self.relation_graph:
            return QueryResult(
                success=False,
                data={},
                message="Relation graph not loaded"
            )

        for rel in self.relation_graph.get("relations", []):
            if (rel.get("predicate") == "implemented_in" and
                api_name in rel.get("subject_id", "")):

                return QueryResult(
                    success=True,
                    data={
                        "api_name": api_name,
                        "header_file": rel.get("metadata", {}).get("header_file"),
                        "source_file": rel.get("metadata", {}).get("source_file")
                    },
                    message=f"Found source files for {api_name}",
                    source_domain="drivers",
                    confidence=1.0
                )

        return QueryResult(
            success=False,
            data={"api_name": api_name},
            message=f"Source files not found for {api_name}"
        )

    def get_profile_api_dependencies(self, profile_name: str) -> QueryResult:
        """
        Get all API dependencies for a BLE profile (P1 enhancement).

        Uses AST-based parsing to extract actual API calls from profile code.

        Args:
            profile_name: Name of the BLE profile (e.g., "hids", "battery")

        Returns:
            QueryResult with GATT APIs, driver APIs, and kernel message APIs
        """
        from .profile_dependency_parser import ProfileDependencyParser

        if not self.sdk_path:
            return QueryResult(
                success=False,
                data={},
                message="SDK path not configured"
            )

        parser = ProfileDependencyParser(self.sdk_path)
        dependencies = parser.parse_profile_directory()

        dep = dependencies.get(profile_name)
        if not dep:
            return QueryResult(
                success=False,
                data={"profile_name": profile_name},
                message=f"Profile '{profile_name}' not found"
            )

        return QueryResult(
            success=True,
            data=dep.to_dict(),
            message=f"Found dependencies for {profile_name}",
            source_domain="ble",
            confidence=1.0
        )

    # ========================================================================
    # P2 Enhancement: Power Consumption Queries
    # ========================================================================

    def get_power_consumption(self, peripheral: str, mode: str) -> QueryResult:
        """
        Get power consumption for a peripheral in specific mode (P2 enhancement).

        Args:
            peripheral: Peripheral name (e.g., "BLE", "UART")
            mode: Power mode (e.g., "SLEEP", "ACTIVE", "DEEP_SLEEP")

        Returns:
            QueryResult with current (uA), voltage (mV), power (mW)
        """
        from .excel_parser import ExcelHardwareParser
        import os

        if not self.sdk_path:
            return QueryResult(
                success=False,
                data={},
                message="SDK path not configured"
            )

        # Parse power data
        power_file = os.path.join(self.sdk_path, "doc/SW_Spec/B6x_BLE-功耗参考.xlsx")
        parser = ExcelHardwareParser()
        entries = parser.parse_power_map(power_file)

        # Find matching entries
        matching = []
        for entry in entries:
            if (entry.mode == mode or mode in entry.mode) and entry.peripheral == peripheral:
                matching.append(entry)

        if not matching:
            # Try partial mode match
            for entry in entries:
                if mode.lower() in entry.mode.lower():
                    matching.append(entry)

        if matching:
            # Use first match
            entry = matching[0]
            return QueryResult(
                success=True,
                data={
                    "peripheral": peripheral,
                    "mode": mode,
                    "current_ua": entry.current_ua,
                    "voltage_mv": entry.voltage_mv,
                    "power_mw": entry.power_mw,
                    "conditions": entry.conditions
                },
                message=f"Power consumption for {peripheral} in {mode} mode",
                source_domain="hardware",
                confidence=1.0
            )

        return QueryResult(
            success=False,
            data={"peripheral": peripheral, "mode": mode},
            message=f"Power data not found for {peripheral} in {mode} mode"
        )

    def estimate_total_power(self, active_peripherals: List[str], mode: str = "ACTIVE") -> QueryResult:
        """
        Estimate total power consumption for multiple peripherals (P2 enhancement).

        Args:
            active_peripherals: List of peripheral names
            mode: Power mode (default "ACTIVE")

        Returns:
            QueryResult with total current, voltage, power
        """
        from .excel_parser import ExcelHardwareParser
        import os

        if not self.sdk_path:
            return QueryResult(
                success=False,
                data={},
                message="SDK path not configured"
            )

        # Parse power data
        power_file = os.path.join(self.sdk_path, "doc/SW_Spec/B6x_BLE-功耗参考.xlsx")
        parser = ExcelHardwareParser()
        entries = parser.parse_power_map(power_file)

        # Sum power for all peripherals
        total_current_ua = 0.0
        total_power_mw = 0.0
        details = []

        for peripheral in active_peripherals:
            matching = [e for e in entries if peripheral in e.peripheral and mode in e.mode]
            if matching:
                entry = matching[0]
                total_current_ua += entry.current_ua
                total_power_mw += entry.power_mw
                details.append({
                    "peripheral": peripheral,
                    "current_ua": entry.current_ua,
                    "power_mw": entry.power_mw
                })

        if total_current_ua > 0:
            return QueryResult(
                success=True,
                data={
                    "mode": mode,
                    "total_current_ua": total_current_ua,
                    "total_current_ma": total_current_ua / 1000,
                    "total_power_mw": total_power_mw,
                    "peripherals": active_peripherals,
                    "details": details
                },
                message=f"Total power consumption for {len(active_peripherals)} peripherals in {mode} mode",
                source_domain="hardware",
                confidence=1.0
            )

        return QueryResult(
            success=False,
            data={"peripherals": active_peripherals, "mode": mode},
            message=f"Could not estimate power for given peripherals in {mode} mode"
        )

    # ========================================================================
    # Utility Methods
    # ========================================================================

    def get_statistics(self) -> Dict:
        """Get knowledge graph statistics."""
        stats = {
            "domains": {},
            "total_entries": 0,
            "total_relations": 0
        }

        for domain in ["hardware", "drivers", "ble", "applications"]:
            dom_data = self.domain_data.get(domain, {})
            entry_count = sum(len(v) if isinstance(v, list) else 1 for v in dom_data.values())
            stats["domains"][domain] = {
                "entries": entry_count,
                "data_types": list(dom_data.keys())
            }
            stats["total_entries"] += entry_count

        if self.relation_graph:
            stats["total_relations"] = len(self.relation_graph.get("relations", []))

        return stats


# ============================================================================
# Convenience Functions
# ============================================================================

def create_query_interface(sdk_path: str) -> KnowledgeGraphQuery:
    """Convenience function to create query interface from SDK path."""
    return KnowledgeGraphQuery(sdk_path=sdk_path)


def quick_query(query: str, sdk_path: str) -> QueryResult:
    """Convenience function for quick semantic query."""
    interface = create_query_interface(sdk_path)
    return interface.search(query)


# ============================================================================
# Main for Testing
# ============================================================================

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        sdk_path = sys.argv[1]
    else:
        sdk_path = "."

    print(f"\n{'='*70}")
    print(f"Knowledge Graph Query Interface")
    print(f"{'='*70}")

    try:
        interface = KnowledgeGraphQuery(sdk_path=sdk_path)

        # Show statistics
        stats = interface.get_statistics()
        print(f"\nKnowledge Graph Statistics:")
        print(f"  Total Entries: {stats['total_entries']}")
        print(f"  Total Relations: {stats['total_relations']}")
        for domain, domain_stats in stats['domains'].items():
            print(f"  {domain.capitalize()}: {domain_stats['entries']} entries")

        # Example queries
        print(f"\n{'='*70}")
        print("Example Queries:")

        # Domain 1
        print("\n[Hardware]")
        result = interface.get_pin_mux("PA00")
        print(f"  get_pin_mux('PA00'): {result.message}")

        # Domain 2
        print("\n[Drivers]")
        result = interface.get_peripheral_init_api("UART")
        print(f"  get_peripheral_init_api('UART'): {result.message}")

        # Domain 3
        print("\n[BLE]")
        result = interface.get_gatt_service("0x180F")
        print(f"  get_gatt_service('0x180F'): {result.message}")

        # Domain 4
        print("\n[Applications]")
        result = interface.get_example_for_api("uart_init")
        print(f"  get_example_for_api('uart_init'): {result.message}")

        # Cross-domain
        print("\n[Cross-Domain]")
        result = interface.search("battery")
        print(f"  search('battery'): {result.message}")

    except FileNotFoundError as e:
        print(f"\nError: {e}")
        print("\nPlease ensure domain data files exist:")
        print("  xm_b6_mcp/data/domain/{hardware,drivers,ble,applications}/*.json")
        print("\nRun build_index.py first to generate these files.")
