"""
SVD Parser for B6x Chip
========================

Parser for CMSIS-SVD (System View Description) files to extract
hardware register definitions from official vendor SVD files.

The SVD file contains complete register definitions including:
- Peripheral base addresses
- Register offsets and full addresses
- Register access permissions (read/write/read-write)
- Reset values
- Bit field definitions
- Enumerated values for bit fields

Author: B6x MCP Server Team
Version: 1.0.0
"""

import logging
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import List, Dict, Optional
from dataclasses import dataclass, field

logger = logging.getLogger(__name__)


@dataclass
class SVDField:
    """
    Register bit field definition.

    Attributes:
        name: Field name (e.g., "TXE", "DIV_MANTISSA")
        bit_range: Bit range string (e.g., "[7:0]", "[15:0]")
        bit_offset: Starting bit position (0-based)
        bit_width: Width of the field in bits
        description: Field description from SVD
        access: Access permission ('read', 'write', 'read-write')
        enumerated_values: List of enumerated value dicts [{name, value, description}]
    """
    name: str
    bit_range: str
    bit_offset: int
    bit_width: int
    description: str
    access: str = "read-write"
    enumerated_values: List[Dict] = field(default_factory=list)


@dataclass
class SVDRegister:
    """
    Hardware register definition.

    Attributes:
        peripheral: Parent peripheral name (e.g., "UART", "GPIO")
        name: Register name (e.g., "CR", "BRR", "IDR")
        full_address: Full hex address (e.g., "0x40000008")
        base_address: Peripheral base address (e.g., "0x40000000")
        address_offset: Offset from base (e.g., "0x08")
        size: Register size in bits (typically 32)
        access: Access permission ('read-only', 'write-only', 'read-write')
        reset_value: Hex reset value (e.g., "0x00000000")
        description: Register description
        fields: List of bit fields in this register
    """
    peripheral: str
    name: str
    full_address: str
    base_address: str
    address_offset: str
    size: int
    access: str
    reset_value: str
    description: str
    fields: List[SVDField] = field(default_factory=list)


@dataclass
class SVDPeripheral:
    """
    Peripheral definition from SVD file.

    Attributes:
        name: Peripheral name (e.g., "UART", "GPIO", "SYSCFG")
        base_address: Base address (e.g., "0x40000000")
        description: Peripheral description
        registers: List of registers belonging to this peripheral
    """
    name: str
    base_address: str
    description: str
    registers: List[SVDRegister] = field(default_factory=list)


class SVDParser:
    """
    Parser for CMSIS-SVD XML files.

    Extracts peripheral, register, and field definitions from
    System View Description files.
    """

    # CMSIS-SVD 1.1 namespace
    SVD_NS = {"svd": "http://www.keil.com/pack/xml/CMSIS-SVD.xsd"}

    def __init__(self, svd_path: Optional[str] = None, remove_namespace: bool = True):
        """
        Initialize SVD parser.

        Args:
            svd_path: Optional path to .svd file. If provided, automatically parses it.
            remove_namespace: If True, removes XML namespace for easier parsing
        """
        self.remove_namespace = remove_namespace
        self.peripherals: List[SVDPeripheral] = []
        self._register_cache: Dict[str, SVDRegister] = {}

        if svd_path:
            self.peripherals = self.parse_svd_file(svd_path)
            self._build_register_cache()

    def _build_register_cache(self):
        """Build a cache for quick register lookup by name."""
        for periph in self.peripherals:
            for reg in periph.registers:
                # Cache by full name (e.g., "UART_CR")
                cache_key = f"{periph.name}_{reg.name}".upper()
                self._register_cache[cache_key] = reg
                # Also cache by short name (e.g., "CR") - may overwrite
                self._register_cache[reg.name.upper()] = reg

    def parse_svd_file(self, svd_path: str) -> List[SVDPeripheral]:
        """
        Parse SVD file and extract all peripherals.

        Args:
            svd_path: Path to .svd file

        Returns:
            List of SVDPeripheral objects

        Raises:
            FileNotFoundError: If SVD file doesn't exist
            ET.ParseError: If XML parsing fails
        """
        svd_file = Path(svd_path)

        if not svd_file.exists():
            raise FileNotFoundError(f"SVD file not found: {svd_path}")

        logger.info(f"Parsing SVD file: {svd_path}")

        # Parse XML
        tree = ET.parse(svd_path)
        root = tree.getroot()

        # Remove namespace for easier parsing if requested
        if self.remove_namespace:
            self._remove_namespace(root)

        # Extract peripherals
        peripherals = self._extract_peripherals(root)

        logger.info(f"Parsed {len(peripherals)} peripherals from SVD")

        return peripherals

    def _remove_namespace(self, element: ET.Element):
        """
        Remove XML namespace from element and all children.

        This makes XPath queries simpler.
        """
        for elem in element.iter():
            if not hasattr(elem.tag, 'find'):
                continue
            i = elem.tag.find('}')
            if i >= 0:
                elem.tag = elem.tag[i+1:]

    def _extract_peripherals(self, root: ET.Element) -> List[SVDPeripheral]:
        """
        Extract all peripheral definitions from SVD root.

        Args:
            root: XML root element

        Returns:
            List of SVDPeripheral objects
        """
        peripherals = []

        # Find all <peripheral> elements
        for periph_elem in root.findall('.//peripheral'):
            try:
                peripheral = self._parse_peripheral(periph_elem)
                peripherals.append(peripheral)
            except Exception as e:
                logger.warning(f"Failed to parse peripheral: {e}")
                continue

        return peripherals

    def _parse_peripheral(self, periph_elem: ET.Element) -> SVDPeripheral:
        """
        Parse a single peripheral element.

        Args:
            periph_elem: <peripheral> XML element

        Returns:
            SVDPeripheral object
        """
        # Extract basic info
        name = self._get_text(periph_elem, 'name')
        base_address = self._get_text(periph_elem, 'baseAddress')
        description = self._get_text(periph_elem, 'description', default="")

        # Extract registers (if present)
        registers = []
        registers_elem = periph_elem.find('registers')

        if registers_elem is not None:
            for reg_elem in registers_elem.findall('register'):
                try:
                    register = self._parse_register(reg_elem, name, base_address)
                    registers.append(register)
                except Exception as e:
                    logger.debug(f"Failed to parse register in {name}: {e}")
                    continue

        return SVDPeripheral(
            name=name,
            base_address=base_address,
            description=description,
            registers=registers
        )

    def _parse_register(
        self,
        reg_elem: ET.Element,
        peripheral_name: str,
        base_address: str
    ) -> SVDRegister:
        """
        Parse a single register element.

        Args:
            reg_elem: <register> XML element
            peripheral_name: Parent peripheral name
            base_address: Peripheral base address

        Returns:
            SVDRegister object
        """
        # Extract basic info
        name = self._get_text(reg_elem, 'name')
        address_offset = self._get_text(reg_elem, 'addressOffset')
        size = int(self._get_text(reg_elem, 'size', default="32"))
        access = self._get_text(reg_elem, 'access', default="read-write")
        reset_value = self._get_text(reg_elem, 'resetValue', default="0")
        description = self._get_text(reg_elem, 'description', default="")

        # Calculate full address
        offset_int = int(address_offset, 16) if address_offset.startswith('0x') else int(address_offset)
        base_int = int(base_address, 16) if base_address.startswith('0x') else int(base_address)
        full_address_int = base_int + offset_int
        full_address = f"0x{full_address_int:08X}"

        # Extract fields
        fields = []
        fields_elem = reg_elem.find('fields')

        if fields_elem is not None:
            for field_elem in fields_elem.findall('field'):
                try:
                    svd_field = self._parse_field(field_elem)
                    fields.append(svd_field)
                except Exception as e:
                    logger.debug(f"Failed to parse field in {name}: {e}")
                    continue

        return SVDRegister(
            peripheral=peripheral_name,
            name=name,
            full_address=full_address,
            base_address=base_address,
            address_offset=address_offset,
            size=size,
            access=access,
            reset_value=reset_value,
            description=description,
            fields=fields
        )

    def _parse_field(self, field_elem: ET.Element) -> SVDField:
        """
        Parse a single field element.

        Args:
            field_elem: <field> XML element

        Returns:
            SVDField object
        """
        # Extract basic info
        name = self._get_text(field_elem, 'name')
        bit_range = self._get_text(field_elem, 'bitRange', default="")
        description = self._get_text(field_elem, 'description', default="")
        access = self._get_text(field_elem, 'access', default="read-write")

        # Parse bit range
        bit_offset, bit_width = self._parse_bit_range(bit_range)

        # Extract enumerated values
        enumerated_values = []
        enums_elem = field_elem.find('enumeratedValues')

        if enums_elem is not None:
            for enum_elem in enums_elem.findall('enumeratedValue'):
                enum_name = self._get_text(enum_elem, 'name')
                enum_value = self._get_text(enum_elem, 'value')
                enum_desc = self._get_text(enum_elem, 'description', default="")

                enumerated_values.append({
                    'name': enum_name,
                    'value': enum_value,
                    'description': enum_desc
                })

        return SVDField(
            name=name,
            bit_range=bit_range,
            bit_offset=bit_offset,
            bit_width=bit_width,
            description=description,
            access=access,
            enumerated_values=enumerated_values
        )

    def _parse_bit_range(self, bit_range: str) -> tuple[int, int]:
        """
        Parse bit range string to offset and width.

        Examples:
            "[7:0]" -> (0, 8)
            "[15:0]" -> (0, 16)
            "[31:16]" -> (16, 16)

        Args:
            bit_range: Bit range string like "[7:0]"

        Returns:
            Tuple of (bit_offset, bit_width)
        """
        if not bit_range:
            return (0, 1)

        # Remove brackets and split
        range_str = bit_range.strip('[]')
        parts = range_str.split(':')

        if len(parts) == 2:
            high = int(parts[0])
            low = int(parts[1])
            offset = low
            width = high - low + 1
            return (offset, width)

        # Single bit
        return (int(range_str), 1)

    def _get_text(self, element: ET.Element, tag: str, default: str = "") -> str:
        """
        Safely get text from child element.

        Args:
            element: Parent element
            tag: Child tag name
            default: Default value if not found

        Returns:
            Text content or default
        """
        child = element.find(tag)
        if child is not None and child.text:
            return child.text.strip()
        return default

    def get_all_registers(self, peripherals: List[SVDPeripheral]) -> List[SVDRegister]:
        """
        Flatten all registers from all peripherals.

        Args:
            peripherals: List of peripherals

        Returns:
            List of all registers
        """
        all_registers = []

        for periph in peripherals:
            all_registers.extend(periph.registers)

        return all_registers

    def get_peripheral_by_name(
        self,
        peripherals: List[SVDPeripheral],
        name: str
    ) -> Optional[SVDPeripheral]:
        """
        Find peripheral by name (case-insensitive).

        Args:
            peripherals: List of peripherals
            name: Peripheral name to find

        Returns:
            SVDPeripheral or None
        """
        for periph in peripherals:
            if periph.name.upper() == name.upper():
                return periph
        return None

    def get_registers_by_peripheral(
        self,
        peripherals: List[SVDPeripheral],
        peripheral_name: str
    ) -> List[SVDRegister]:
        """
        Get all registers for a specific peripheral.

        Args:
            peripherals: List of peripherals
            peripheral_name: Peripheral name

        Returns:
            List of registers for the peripheral
        """
        periph = self.get_peripheral_by_name(peripherals, peripheral_name)

        if periph:
            return periph.registers

        return []

    def search_registers(
        self,
        peripherals: Optional[List[SVDPeripheral]] = None,
        query: str = "",
        search_in: str = "both",
        limit: int = 50
    ) -> List[Dict]:
        """
        Search registers by name or description.

        Supports multiple search patterns:
        - Short name: "BRR" matches all BRR registers
        - Full name: "UART1_BRR" matches specific register
        - Peripheral prefix: "UART1" matches all UART1 registers

        Args:
            peripherals: List of peripherals (uses cached if not provided)
            query: Search query (case-insensitive)
            search_in: Where to search - "name", "description", or "both"
            limit: Maximum number of results

        Returns:
            List of matching registers as dictionaries
        """
        # Use provided peripherals or cached ones
        search_peripherals = peripherals if peripherals is not None else self.peripherals

        if not search_peripherals:
            logger.warning("No peripherals available for register search")
            return []

        results = []
        query_lower = query.lower()

        for periph in search_peripherals:
            for reg in periph.registers:
                match = False
                relevance = 0.5

                # Build full register name (e.g., "UART1_BRR")
                full_name = f"{periph.name}_{reg.name}".lower()
                short_name = reg.name.lower()
                periph_name = periph.name.lower()

                # Search in name (supports full name, short name, peripheral prefix)
                if search_in in ["name", "both"]:
                    # Exact full name match (highest relevance)
                    if query_lower == full_name:
                        match = True
                        relevance = 1.0
                    # Full name partial match
                    elif query_lower in full_name:
                        match = True
                        relevance = 0.9
                    # Short name match
                    elif query_lower in short_name:
                        match = True
                        relevance = 0.8
                    # Peripheral name match (finds all registers of a peripheral)
                    elif query_lower in periph_name:
                        match = True
                        relevance = 0.6

                # Search in description
                if search_in in ["description", "both"]:
                    if query_lower in reg.description.lower():
                        match = True
                        relevance = max(relevance, 0.7)

                if match:
                    results.append((reg, relevance))
                    if len(results) >= limit:
                        break

            if len(results) >= limit:
                break

        # Sort by relevance (highest first)
        results.sort(key=lambda x: x[1], reverse=True)

        # Convert SVDRegister to Dict for consistent API
        return [
            {
                "name": reg.name,
                "peripheral": reg.peripheral,
                "description": reg.description,
                "address": reg.full_address,
                "offset": reg.address_offset,
                "access": reg.access,
                "relevance": relevance,
            }
            for reg, relevance in results[:limit]
        ]

    def get_register_info(
        self,
        register_name: str,
        peripheral_name: Optional[str] = None,
        peripherals: Optional[List[SVDPeripheral]] = None
    ) -> Optional[Dict]:
        """
        Get detailed register information by name.

        Args:
            register_name: Register name to find (e.g., "CR", "BRR", "UART_CR")
            peripheral_name: Optional peripheral name to disambiguate
            peripherals: Optional list of peripherals (uses cached if not provided)

        Returns:
            Dictionary with register information or None if not found
        """
        # Use provided peripherals or cached ones
        search_peripherals = peripherals if peripherals is not None else self.peripherals

        if not search_peripherals:
            logger.warning("No peripherals available for register search")
            return None

        register_name_upper = register_name.upper()

        # First, try cache for quick lookup
        if register_name_upper in self._register_cache:
            reg = self._register_cache[register_name_upper]
            # If peripheral_name specified, verify match
            if peripheral_name and reg.peripheral.upper() != peripheral_name.upper():
                # Continue to full search
                pass
            else:
                return self._register_to_dict(reg)

        # Full search if cache miss or peripheral mismatch
        for periph in search_peripherals:
            # If peripheral_name specified, only search in that peripheral
            if peripheral_name and periph.name.upper() != peripheral_name.upper():
                continue

            for reg in periph.registers:
                # Match by full name or short name
                if reg.name.upper() == register_name_upper:
                    return self._register_to_dict(reg)

        return None

    def _register_to_dict(self, reg: SVDRegister) -> Dict:
        """Convert SVDRegister to dictionary format."""
        return {
            "name": reg.name,
            "peripheral": reg.peripheral,
            "full_address": reg.full_address,
            "base_address": reg.base_address,
            "address_offset": reg.address_offset,
            "size": reg.size,
            "access": reg.access,
            "reset_value": reg.reset_value,
            "description": reg.description,
            "fields": [
                {
                    "name": f.name,
                    "bit_range": f.bit_range,
                    "bit_offset": f.bit_offset,
                    "bit_width": f.bit_width,
                    "description": f.description,
                    "access": f.access,
                    "enumerated_values": f.enumerated_values
                }
                for f in reg.fields
            ]
        }

    def get_register_fields(
        self,
        register_name: str,
        peripheral_name: Optional[str] = None,
        peripherals: Optional[List[SVDPeripheral]] = None
    ) -> Optional[List[Dict]]:
        """
        Get bit fields for a specific register.

        Args:
            register_name: Register name to find
            peripheral_name: Optional peripheral name to disambiguate
            peripherals: Optional list of peripherals (uses cached if not provided)

        Returns:
            List of field dictionaries or None if register not found
        """
        reg_info = self.get_register_info(register_name, peripheral_name, peripherals)
        if reg_info:
            return reg_info.get("fields", [])
        return None


# Convenience functions

def parse_svd_file(svd_path: str) -> List[SVDPeripheral]:
    """
    Convenience function to parse SVD file.

    Args:
        svd_path: Path to .svd file

    Returns:
        List of SVDPeripheral objects
    """
    parser = SVDParser()
    return parser.parse_svd_file(svd_path)


if __name__ == "__main__":
    import sys
    import json

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    # Test parsing the B6x SVD file
    if len(sys.argv) > 1:
        svd_path = sys.argv[1]
    else:
        # Default SVD path (updated to sdk6/core/B6x.svd)
        # Path: xm_b6_mcp/src/core/svd_parser.py → sdk6/core/B6x.svd
        svd_path = Path(__file__).parent.parent.parent.parent / "core" / "B6x.svd"

        if not svd_path.exists():
            # Fallback to previous path
            svd_path = Path(__file__).parent.parent / "SVD" / "DragonC.svd"

    try:
        parser = SVDParser()
        peripherals = parser.parse_svd_file(str(svd_path))

        print(f"\n{'='*70}")
        print(f"SVD File: {svd_path}")
        print(f"Total Peripherals: {len(peripherals)}")
        print(f"{'='*70}\n")

        # Print summary for each peripheral
        for periph in peripherals[:10]:  # First 10 peripherals
            print(f"Peripheral: {periph.name}")
            print(f"  Base Address: {periph.base_address}")
            print(f"  Registers: {len(periph.registers)}")
            if periph.description:
                desc = periph.description[:60] + "..." if len(periph.description) > 60 else periph.description
                print(f"  Description: {desc}")

            # Show first 3 registers with fields
            for reg in periph.registers[:3]:
                print(f"    {reg.name}: {reg.address_offset} ({reg.access})")
                if reg.fields:
                    print(f"      Fields: {', '.join([f.name for f in reg.fields[:3]])}")
            print()

        # Statistics
        total_registers = sum(len(p.registers) for p in peripherals)
        total_fields = sum(
            sum(len(r.fields) for r in p.registers)
            for p in peripherals
        )

        print(f"{'='*70}")
        print(f"Total Statistics")
        print(f"{'='*70}")
        print(f"  Peripherals:   {len(peripherals)}")
        print(f"  Registers:     {total_registers}")
        print(f"  Fields:        {total_fields}")

    except FileNotFoundError as e:
        print(f"Error: {e}")
        sys.exit(1)
    except ET.ParseError as e:
        print(f"XML Parse Error: {e}")
        sys.exit(1)
