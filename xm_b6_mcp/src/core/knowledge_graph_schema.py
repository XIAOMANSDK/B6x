"""
Knowledge Graph Schema for Four-Dimensional Knowledge Representation
====================================================================

Defines the data structures for the unified knowledge graph:
- EntryType: All types of knowledge entries
- RelationType: Cross-domain relationship types
- KnowledgeGraphEntry: Universal entry format
- CrossDomainRelation: Inter-domain links

This schema enables:
1. Semantic search across all SDK knowledge
2. Cross-domain reasoning (e.g., API -> registers it controls)
3. AI-augmented context for code generation
4. Knowledge graph visualization
"""

from dataclasses import dataclass, field
from typing import List, Dict, Optional, Any, Literal
from enum import Enum
from datetime import datetime
import json


# ============================================================================
# Domain Type Definitions
# ============================================================================

DomainType = Literal["hardware", "drivers", "ble", "applications"]


# ============================================================================
# Entry Type Enumeration
# ============================================================================

class EntryType(Enum):
    """All possible knowledge entry types across 4 domains."""

    # ==================== Domain 1: Hardware ====================
    REGISTER = "register"
    """Hardware register definition"""

    PERIPHERAL = "peripheral"
    """Hardware peripheral (UART, SPI, etc.)"""

    PIN_MUX = "pin_mux"
    """Pin multiplexing configuration"""

    MEMORY_REGION = "memory_region"
    """Memory region (FLASH, SRAM, XIP)"""

    INTERRUPT_VECTOR = "interrupt_vector"
    """Interrupt vector / ISR handler"""

    # ==================== Domain 2: Drivers ====================
    FUNCTION = "function"
    """Driver function / API"""

    MACRO = "macro"
    """Preprocessor macro / constant"""

    ENUM = "enum"
    """Enumeration type"""

    STRUCT = "struct"
    """Structure definition (especially config structs)"""

    # ==================== Domain 3: BLE ====================
    BLE_API = "ble_api"
    """BLE protocol stack API"""

    GATT_SERVICE = "gatt_service"
    """GATT service definition"""

    GATT_CHARACTERISTIC = "gatt_characteristic"
    """GATT characteristic definition"""

    BLE_ERROR = "ble_error"
    """BLE error code definition"""

    # ==================== Domain 4: Applications ====================
    EXAMPLE = "example"
    """Code example / snippet"""

    CALL_CHAIN = "call_chain"
    """Function call chain analysis"""

    INIT_SEQUENCE = "init_sequence"
    """Initialization sequence for a peripheral/feature"""

    PROJECT_CONFIG = "project_config"
    """Project configuration (compile options, etc.)"""


# ============================================================================
# Relation Type Enumeration
# ============================================================================

class RelationType(Enum):
    """Types of cross-domain relationships."""

    # ==================== Hardware <-> Drivers ====================
    CONTROLS_REGISTER = "controls_register"
    """API controls/writes to a register"""

    READS_REGISTER = "reads_register"
    """API reads from a register"""

    CONFIGURES_REGISTER = "configures_register"
    """API configures a register (typically init functions)"""

    REGISTERS_FOR = "registers_for"
    """Registers belong to peripheral"""

    # ==================== Drivers <-> BLE ====================
    USES_DRIVER = "uses_driver"
    """BLE API uses a driver API (e.g., BLE UART uses DMA)"""

    DEPENDS_ON = "depends_on"
    """General dependency relation"""

    IMPLEMENTS_PROFILE = "implements_profile"
    """Profile implements GATT services"""

    # ==================== All -> Applications ====================
    USED_IN_EXAMPLE = "used_in_example"
    """API/struct is used in an example"""

    DEMONSTRATES = "demonstrates"
    """Example demonstrates an API/feature"""

    CALLED_IN_SEQUENCE = "called_in_sequence"
    """API called in specific order (init sequence)"""

    # ==================== Source Code Relations (P1) ====================
    IMPLEMENTED_IN = "implemented_in"
    """API declaration is implemented in source file"""

    DECLARED_IN = "declared_in"
    """API is declared in header file"""

    # ==================== Power Relations (P2) ====================
    CONSUMES_POWER = "consumes_power"
    """API/function consumes power in specific mode"""

    # ==================== Dependency Relations ====================
    REQUIRES = "requires"
    """Requires something (prerequisite)"""

    REQUIRES_CLOCK = "requires_clock"
    """API requires peripheral clock enabled"""

    REQUIRES_GPIO = "requires_gpio"
    """API requires GPIO configuration"""

    CONFLICTS_WITH = "conflicts_with"
    """Mutually exclusive with (e.g., pin conflict)"""

    # ==================== Structural Relations ====================
    USES_CONFIG = "uses_config"
    """API uses a configuration struct"""

    DEFINED_IN_HEADER = "defined_in_header"
    """Macro/constant defined in a header"""

    BELONGS_TO_PERIPHERAL = "belongs_to_peripheral"
    """Register belongs to a hardware peripheral"""

    CAN_RETURN = "can_return"
    """API can return this error code"""

    BELONGS_TO_MODULE = "belongs_to_module"
    """Entity belongs to a software module"""


# ============================================================================
# Cross-Domain Relation
# ============================================================================

@dataclass
class CrossDomainRelation:
    """A relationship between two knowledge entries across domains.

    Attributes:
        source_id: Unique ID of source entry
        target_id: Unique ID of target entry
        relation_type: Type of relationship
        source_domain: Domain of source entry
        target_domain: Domain of target entry
        metadata: Additional relation-specific data
        confidence: Confidence score (0-1)
    """
    source_id: str
    target_id: str
    relation_type: RelationType
    source_domain: DomainType
    target_domain: DomainType
    metadata: Dict[str, Any] = field(default_factory=dict)
    confidence: float = 1.0

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "source_id": self.source_id,
            "target_id": self.target_id,
            "relation_type": self.relation_type.value,
            "source_domain": self.source_domain,
            "target_domain": self.target_domain,
            "metadata": self.metadata,
            "confidence": self.confidence
        }

    @classmethod
    def from_dict(cls, data: Dict) -> "CrossDomainRelation":
        """Create from dictionary."""
        return cls(
            source_id=data["source_id"],
            target_id=data["target_id"],
            relation_type=RelationType(data["relation_type"]),
            source_domain=data["source_domain"],
            target_domain=data["target_domain"],
            metadata=data.get("metadata", {}),
            confidence=data.get("confidence", 1.0)
        )


# ============================================================================
# Knowledge Graph Entry
# ============================================================================

@dataclass
class KnowledgeGraphEntry:
    """Universal entry format for all knowledge types.

    This unified schema allows all types of SDK knowledge to be stored,
    searched, and related in a consistent manner.

    Attributes:
        id: Globally unique ID (format: domain:type:name)
        domain: Which domain this belongs to
        entry_type: Specific type of entry
        name: Primary name/identifier
        brief: Short description
        detailed: Detailed description
        ai_use_case: AI context hint (how to use this)
        ai_context: Additional AI-specific context
        file_path: Source file path
        line_number: Line number in source
        relations: List of cross-domain relations
        attributes: Domain-specific attributes (flexible JSON)
        tags: Search/organization tags
        confidence: Parsing confidence (0-1)
        created_at: Timestamp of creation
        updated_at: Timestamp of last update
    """
    # Basic identification
    id: str
    domain: DomainType
    entry_type: EntryType

    # Content
    name: str
    brief: str
    detailed: str = ""

    # AI context
    ai_use_case: str = ""
    ai_context: Dict[str, Any] = field(default_factory=dict)

    # Source location
    file_path: str = ""
    line_number: int = 0

    # Relationships
    relations: List[CrossDomainRelation] = field(default_factory=list)

    # Domain-specific data (extensible)
    attributes: Dict[str, Any] = field(default_factory=dict)

    # Metadata
    tags: List[str] = field(default_factory=list)
    confidence: float = 1.0
    created_at: str = field(default_factory=lambda: datetime.now().isoformat())
    updated_at: str = field(default_factory=lambda: datetime.now().isoformat())

    def generate_id(self) -> str:
        """Generate ID from domain, type, and name."""
        # Sanitize name for use in ID
        safe_name = self.name.replace("/", "_").replace(" ", "_")
        return f"{self.domain}:{self.entry_type.value}:{safe_name}"

    def to_dict(self, include_relations: bool = True) -> Dict:
        """Convert to dictionary for JSON serialization."""
        data = {
            "id": self.id,
            "domain": self.domain,
            "entry_type": self.entry_type.value,
            "name": self.name,
            "brief": self.brief,
            "detailed": self.detailed,
            "ai_use_case": self.ai_use_case,
            "ai_context": self.ai_context,
            "file_path": self.file_path,
            "line_number": self.line_number,
            "attributes": self.attributes,
            "tags": self.tags,
            "confidence": self.confidence,
            "created_at": self.created_at,
            "updated_at": self.updated_at,
        }

        if include_relations:
            data["relations"] = [r.to_dict() for r in self.relations]

        return data

    def to_json(self, indent: int = 2) -> str:
        """Convert to JSON string."""
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)

    @classmethod
    def from_dict(cls, data: Dict) -> "KnowledgeGraphEntry":
        """Create from dictionary."""
        relations = []
        if "relations" in data:
            relations = [CrossDomainRelation.from_dict(r) for r in data["relations"]]

        # Convert entry_type string back to enum
        entry_type = EntryType(data["entry_type"])

        return cls(
            id=data["id"],
            domain=data["domain"],
            entry_type=entry_type,
            name=data["name"],
            brief=data.get("brief", ""),
            detailed=data.get("detailed", ""),
            ai_use_case=data.get("ai_use_case", ""),
            ai_context=data.get("ai_context", {}),
            file_path=data.get("file_path", ""),
            line_number=data.get("line_number", 0),
            relations=relations,
            attributes=data.get("attributes", {}),
            tags=data.get("tags", []),
            confidence=data.get("confidence", 1.0),
            created_at=data.get("created_at", ""),
            updated_at=data.get("updated_at", "")
        )

    def add_relation(self, relation: CrossDomainRelation) -> None:
        """Add a cross-domain relation."""
        # Check for duplicates
        for existing in self.relations:
            if (existing.target_id == relation.target_id and
                existing.relation_type == relation.relation_type):
                return  # Already exists
        self.relations.append(relation)
        self.updated_at = datetime.now().isoformat()

    def get_relations_by_type(self, relation_type: RelationType) -> List[CrossDomainRelation]:
        """Get all relations of a specific type."""
        return [r for r in self.relations if r.relation_type == relation_type]

    def get_relations_to_domain(self, target_domain: DomainType) -> List[CrossDomainRelation]:
        """Get all relations pointing to a specific domain."""
        return [r for r in self.relations if r.target_domain == target_domain]


# ============================================================================
# Domain-Specific Entry Builders
# ============================================================================

@dataclass
class HardwareRegisterEntry(KnowledgeGraphEntry):
    """Specialized entry for hardware registers."""

    def __init__(self, **kwargs):
        # Set required fields for register entry
        kwargs.setdefault("domain", "hardware")
        kwargs.setdefault("entry_type", EntryType.REGISTER)
        super().__init__(**kwargs)

        # Default AI context for registers
        if not self.ai_use_case:
            self.ai_use_case = "Hardware register - check bitfields before read/write operations"


@dataclass
class DriverFunctionEntry(KnowledgeGraphEntry):
    """Specialized entry for driver functions."""

    def __init__(self, **kwargs):
        kwargs.setdefault("domain", "drivers")
        kwargs.setdefault("entry_type", EntryType.FUNCTION)
        super().__init__(**kwargs)

        if not self.ai_use_case:
            self.ai_use_case = f"Driver API for {kwargs.get('name', 'peripheral')} operations"


@dataclass
class BLEAPIEntry(KnowledgeGraphEntry):
    """Specialized entry for BLE APIs."""

    def __init__(self, **kwargs):
        kwargs.setdefault("domain", "ble")
        kwargs.setdefault("entry_type", EntryType.BLE_API)
        super().__init__(**kwargs)

        if not self.ai_use_case:
            self.ai_use_case = f"BLE protocol API - {kwargs.get('brief', 'BLE operation')}"


@dataclass
class ApplicationExampleEntry(KnowledgeGraphEntry):
    """Specialized entry for application examples."""

    def __init__(self, **kwargs):
        kwargs.setdefault("domain", "applications")
        kwargs.setdefault("entry_type", EntryType.EXAMPLE)
        super().__init__(**kwargs)

        if not self.ai_use_case:
            self.ai_use_case = f"Reference example showing {kwargs.get('name', 'SDK usage')}"


# ============================================================================
# Knowledge Graph Container
# ============================================================================

@dataclass
class KnowledgeGraph:
    """Container for the complete knowledge graph.

    Attributes:
        entries: All entries indexed by ID
        domains: Domain-specific entry lists
        relations: All cross-domain relations
        metadata: Graph metadata
    """
    entries: Dict[str, KnowledgeGraphEntry] = field(default_factory=dict)
    domains: Dict[DomainType, List[str]] = field(default_factory=dict)
    relations: List[CrossDomainRelation] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)

    def add_entry(self, entry: KnowledgeGraphEntry) -> None:
        """Add an entry to the graph."""
        # Generate ID if not set
        if not entry.id or entry.id == "":
            entry.id = entry.generate_id()

        self.entries[entry.id] = entry

        # Add to domain index
        if entry.domain not in self.domains:
            self.domains[entry.domain] = []
        if entry.id not in self.domains[entry.domain]:
            self.domains[entry.domain].append(entry.id)

        # Index relations
        for relation in entry.relations:
            if relation not in self.relations:
                self.relations.append(relation)

    def get_entry(self, entry_id: str) -> Optional[KnowledgeGraphEntry]:
        """Get entry by ID."""
        return self.entries.get(entry_id)

    def get_domain_entries(self, domain: DomainType) -> List[KnowledgeGraphEntry]:
        """Get all entries for a domain."""
        ids = self.domains.get(domain, [])
        return [self.entries[id] for id in ids if id in self.entries]

    def find_by_name(self, name: str, domain: Optional[DomainType] = None) -> List[KnowledgeGraphEntry]:
        """Find entries by name."""
        results = []
        for entry in self.entries.values():
            if domain and entry.domain != domain:
                continue
            if entry.name == name:
                results.append(entry)
        return results

    def get_statistics(self) -> Dict[str, Any]:
        """Get graph statistics."""
        entry_types = {}
        for entry in self.entries.values():
            et = entry.entry_type.value
            entry_types[et] = entry_types.get(et, 0) + 1

        domain_counts = {}
        for domain, ids in self.domains.items():
            domain_counts[domain] = len(ids)

        return {
            "total_entries": len(self.entries),
            "total_relations": len(self.relations),
            "domains": domain_counts,
            "entry_types": entry_types,
            "metadata": self.metadata
        }

    def to_dict(self) -> Dict:
        """Export to dictionary."""
        return {
            "entries": {id: e.to_dict(include_relations=False) for id, e in self.entries.items()},
            "domains": self.domains,
            "relations": [r.to_dict() for r in self.relations],
            "statistics": self.get_statistics(),
            "metadata": self.metadata
        }

    def export_json(self, file_path: str, indent: int = 2) -> None:
        """Export to JSON file."""
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(self.to_dict(), f, indent=indent, ensure_ascii=False)


# ============================================================================
# Utility Functions
# ============================================================================

def create_entry_id(domain: DomainType, entry_type: EntryType, name: str) -> str:
    """Create a unique entry ID."""
    safe_name = name.replace("/", "_").replace(" ", "_").replace(":", "_")
    return f"{domain}:{entry_type.value}:{safe_name}"


def parse_entry_id(entry_id: str) -> tuple[DomainType, EntryType, str]:
    """Parse an entry ID into components."""
    parts = entry_id.split(":", 2)
    if len(parts) != 3:
        raise ValueError(f"Invalid entry ID format: {entry_id}")

    domain = DomainType(parts[0])
    entry_type = EntryType(parts[1])
    name = parts[2]

    return domain, entry_type, name
