"""
Profile Parser - BLE GATT Profile Definition Parser
====================================================

Extracts:
- GATT Service definitions (UUID, name)
- Characteristic definitions (UUID, properties, permissions)
- Descriptor definitions (CCC, etc.)
- Service-to-characteristic relationships
- Standard Bluetooth SIG UUIDs

Supports:
- Profile .c files with svc_decl definitions
- att.h for standard UUID definitions
- Custom profile parsing

AI Use Cases:
- Generate BLE service configuration code
- Create characteristic read/write handlers
- Understand GATT database structure
- Validate service/characteristic combinations
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Set, Tuple


# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class GATTCharacteristic:
    """GATT Characteristic definition."""
    name: str                       # Characteristic name (e.g., "Battery Level")
    uuid: str                       # UUID (16-bit or 128-bit)
    uuid_hex: str                   # Formatted UUID (e.g., "0x2A19")
    properties: List[str]           # read, write, notify, indicate, etc.
    permissions: List[str] = field(default_factory=list)  # Additional permissions
    max_length: int = 256           # Max data length
    variable_length: bool = True
    description: str = ""
    is_standard: bool = False       # True for BLE SIG assigned UUIDs

    def supports_read(self) -> bool:
        return "read" in self.properties or "rd" in self.properties

    def supports_write(self) -> bool:
        return any(p in self.properties for p in ["write", "wr", "write_no_resp"])

    def supports_notify(self) -> bool:
        return "notify" in self.properties or "ntf" in self.properties

    def supports_indicate(self) -> bool:
        return "indicate" in self.properties or "ind" in self.properties

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "uuid": self.uuid,
            "uuid_hex": self.uuid_hex,
            "properties": self.properties,
            "permissions": self.permissions,
            "max_length": self.max_length,
            "variable_length": self.variable_length,
            "description": self.description,
            "is_standard": self.is_standard,
            "capabilities": {
                "read": self.supports_read(),
                "write": self.supports_write(),
                "notify": self.supports_notify(),
                "indicate": self.supports_indicate()
            }
        }


@dataclass
class GATTDescriptor:
    """GATT Descriptor definition."""
    name: str
    uuid: str
    uuid_hex: str
    type: str                       # "ccc", "ccc", "user_defined"
    description: str = ""

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "uuid": self.uuid,
            "uuid_hex": self.uuid_hex,
            "type": self.type,
            "description": self.description
        }


@dataclass
class GATTService:
    """GATT Service definition."""
    name: str                       # Service name (e.g., "Battery Service")
    uuid: str                       # Service UUID
    uuid_hex: str                   # Formatted UUID
    characteristics: List[GATTCharacteristic] = field(default_factory=list)
    descriptors: List[GATTDescriptor] = field(default_factory=list)
    is_primary: bool = True
    is_standard: bool = False       # True for BLE SIG assigned UUIDs
    description: str = ""
    source_file: str = ""
    profile_api_prefix: str = ""    # e.g., "bass" for Battery Service Server

    def get_characteristic_by_uuid(self, uuid: str) -> Optional[GATTCharacteristic]:
        """Find characteristic by UUID."""
        for char in self.characteristics:
            if char.uuid == uuid or char.uuid_hex == uuid:
                return char
        return None

    def get_characteristics_with_property(self, prop: str) -> List[GATTCharacteristic]:
        """Get all characteristics with a specific property."""
        return [c for c in self.characteristics if prop in c.properties]

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "uuid": self.uuid,
            "uuid_hex": self.uuid_hex,
            "is_primary": self.is_primary,
            "is_standard": self.is_standard,
            "description": self.description,
            "source_file": self.source_file,
            "profile_api_prefix": self.profile_api_prefix,
            "num_characteristics": len(self.characteristics),
            "characteristics": [c.to_dict() for c in self.characteristics],
            "descriptors": [d.to_dict() for d in self.descriptors]
        }


# ============================================================================
# Standard BLE SIG UUIDs Database
# ============================================================================

STANDARD_SERVICES = {
    "0x1800": "Generic Access",
    "0x1801": "Generic Attribute",
    "0x180D": "Heart Rate",
    "0x180A": "Device Information",
    "0x180F": "Battery Service",
    "0x1812": "HID Service",
    "0x1819": "Bond Management",
}

STANDARD_CHARACTERISTICS = {
    "0x2A00": "Device Name",
    "0x2A01": "Appearance",
    "0x2A19": "Battery Level",
    "0x2A1A": "Battery Power State",
    "0x2A29": "Manufacturer Name",
    "0x2A24": "Model Number",
    "0x2A25": "Serial Number",
    "0x2A26": "Firmware Revision",
    "0x2A27": "Hardware Revision",
    "0x2A28": "Software Revision",
    "0x2A38": "Body Sensor Location",
    "0x2A37": "Heart Rate Measurement",
    "0x2A39": "Heart Rate Control Point",
    "0x2A4D": "HID Information",
    "0x2A4A": "HID Control Point",
}

PROPERTY_NAMES = {
    "RD": "read",
    "WR": "write",
    "WR_NO_RESP": "write_no_response",
    "NTF": "notify",
    "IND": "indicate",
    "AUTH": "authenticated_signed_writes",
    "BCAST": "broadcast",
    "EXT_PROP": "extended_properties",
}


# ============================================================================
# Profile File Parser
# ============================================================================

class ProfileParser:
    """Parse BLE GATT Profile definitions from source files."""

    def __init__(self):
        self.services: List[GATTService] = []
        self.standard_uuids: Dict[str, str] = {}

    # ========================================================================
    # Main Parsing Methods
    # ========================================================================

    def parse_profile_file(self, file_path: str) -> List[GATTService]:
        """Parse a profile implementation file (.c)."""
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        services = self._parse_profile_content(content, file_path)
        self.services.extend(services)
        return services

    def parse_att_header(self, header_path: str) -> Tuple[Dict[str, str], Dict[str, str]]:
        """Parse att.h for standard UUID definitions."""
        with open(header_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        return self._parse_att_definitions(content)

    def parse_directory(self, directory: str) -> List[GATTService]:
        """Parse all profile files in a directory."""
        from glob import glob
        from pathlib import Path

        # Try to load att.h for UUID definitions
        dir_path = Path(directory)
        sdk_path = dir_path.parent.parent  # directory is ble/prf, sdk is parent.parent
        att_h_path = sdk_path / "ble" / "api" / "att.h"

        if att_h_path.exists():
            try:
                svc_uuids, char_uuids = self.parse_att_header(str(att_h_path))
                self.standard_uuids.update(svc_uuids)
                self.standard_uuids.update(char_uuids)
            except Exception as e:
                print(f"Warning: Failed to parse att.h: {e}")

        all_services = []
        for file_path in glob(f"{directory}/prf_*.c"):
            try:
                services = self.parse_profile_file(file_path)
                all_services.extend(services)
            except Exception as e:
                print(f"Warning: Failed to parse {file_path}: {e}")

        return all_services

    # ========================================================================
    # Content Parsing
    # ========================================================================

    def _parse_profile_content(self, content: str, file_path: str) -> List[GATTService]:
        """Parse profile C file for service definitions."""
        services = []

        # Find svc_decl structure
        # Pattern: const struct svc_decl <name>_svc_db = { ... };
        svc_pattern = re.compile(
            r'const\s+struct\s+svc_decl\s+(\w+_svc_db)\s*=\s*\{(.*?)\};',
            re.DOTALL
        )

        for match in svc_pattern.finditer(content):
            svc_name = match.group(1)
            svc_body = match.group(2)

            service = self._parse_service_decl(svc_name, svc_body, content, file_path)
            if service:
                services.append(service)

        return services

    def _parse_service_decl(
        self,
        svc_name: str,
        svc_body: str,
        full_content: str,
        file_path: str
    ) -> Optional[GATTService]:
        """Parse a service declaration structure."""

        # Extract UUID (either .uuid or .uuid128)
        # Note: uuid128? matches 'uuid1' or 'uuid128', NOT 'uuid'
        # Fixed: match .uuid, .uuid16, .uuid128, etc.
        uuid_match = re.search(r'\.uuid\d*\s*=\s*(\w+)', svc_body)
        if not uuid_match:
            return None

        uuid_macro = uuid_match.group(1)

        # Extract the actual UUID value
        uuid_value = self._resolve_uuid(uuid_macro, full_content)
        if not uuid_value:
            return None

        # Determine API prefix from service name
        # e.g., bas_svc_db -> bass
        api_prefix = svc_name.replace('_svc_db', '')

        # Get service name from UUID
        service_name = STANDARD_SERVICES.get(uuid_value, self._guess_service_name(api_prefix))

        # Create service
        service = GATTService(
            name=service_name,
            uuid=uuid_value,
            uuid_hex=uuid_value,
            is_standard=(uuid_value in STANDARD_SERVICES),
            source_file=file_path,
            profile_api_prefix=api_prefix
        )

        # Parse attributes array
        atts_match = re.search(r'\.atts\s*=\s*(\w+)', svc_body)
        if atts_match:
            atts_array_name = atts_match.group(1)
            service.characteristics = self._parse_attributes_array(
                atts_array_name,
                full_content
            )

        # Extract description from comments
        service.description = self._extract_service_description(full_content, api_prefix)

        return service

    def _parse_attributes_array(
        self,
        array_name: str,
        content: str
    ) -> List[GATTCharacteristic]:
        """Parse the attributes array to extract characteristics."""
        characteristics = []

        # Find the att_decl_t array definition
        # Note: array size may be empty [] or have a size [BAS_IDX_NB]
        array_pattern = re.compile(
            rf'const\s+att_decl_t\s+{array_name}\s*\[[^\]]*\]\s*=\s*\{{([^}}]+)\}}',
            re.DOTALL
        )

        match = array_pattern.search(content)
        if not match:
            return characteristics

        array_body = match.group(1)

        # Parse individual attribute entries
        # Patterns:
        # ATT_ELMT_DECL_CHAR( IDX_CHAR )
        # ATT_ELMT( IDX_VAL, ATT_CHAR_XXX, PROP_RD | PROP_NTF, max_len )
        # ATT_ELMT128( IDX_VAL, custom_uuid, PROP_RD | PROP_NTF, max_len )  <- 128-bit UUID
        current_char = None

        lines = array_body.split('\n')
        for line in lines:
            line = line.strip()

            # Characteristic declaration
            if 'ATT_ELMT_DECL_CHAR' in line:
                if current_char:
                    characteristics.append(current_char)
                current_char = None

            # Characteristic value (16-bit or 128-bit UUID)
            # Match both ATT_ELMT and ATT_ELMT128
            elif 'ATT_ELMT' in line and 'ATT_ELMT_DECL_CHAR' not in line and 'ATT_ELMT_DESC' not in line:
                # Extract: ATT_ELMT( IDX, UUID, PROPS, max_len ) or ATT_ELMT128( ... )
                # Use more flexible pattern to match both formats
                char_match = re.search(
                    r'ATT_ELMT(?:128)?\s*\(\s*\w+,\s*(\w+),\s*([^,]+),\s*(\d+|\w+)\s*\)',
                    line
                )
                if char_match:
                    uuid_macro = char_match.group(1)
                    props_macro = char_match.group(2)

                    uuid_value = self._resolve_uuid(uuid_macro, content)
                    properties = self._parse_properties(props_macro)

                    # If UUID not resolved, use macro name as fallback
                    if not uuid_value:
                        uuid_value = f"custom:{uuid_macro}"

                    current_char = GATTCharacteristic(
                        name=STANDARD_CHARACTERISTICS.get(uuid_value, self._guess_char_name(uuid_macro)),
                        uuid=uuid_value,
                        uuid_hex=uuid_value,
                        properties=properties,
                        is_standard=(uuid_value in STANDARD_CHARACTERISTICS)
                    )

            # CCC Descriptor
            elif 'ATT_ELMT_DESC_CLI_CHAR_CFG' in line:
                if current_char:
                    # Has notify/indicate capability
                    current_char.descriptors = [
                        GATTDescriptor(
                            name="Client Characteristic Configuration",
                            uuid="0x2902",
                            uuid_hex="0x2902",
                            type="ccc",
                            description="Enable/Disable notifications/indications"
                        )
                    ]

        # Add last characteristic
        if current_char:
            characteristics.append(current_char)

        return characteristics

    def _resolve_uuid(self, macro: str, content: str) -> Optional[str]:
        """Resolve a UUID macro to its hex value."""
        # Check if already a hex value
        if macro.startswith('0x') or macro.startswith('ATT_UUID16'):
            if macro.startswith('ATT_UUID16'):
                # Extract value from ATT_UUID16(0xXXXX)
                match = re.search(r'ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)', macro)
                return match.group(1) if match else None
            return macro

        # Check pre-loaded UUIDs from att.h
        if macro in self.standard_uuids:
            return self.standard_uuids[macro]

        # Look for #define MACRO 0xXXXX or #define MACRO 123
        define_pattern = rf'#define\s+{re.escape(macro)}\s+(0x[0-9A-Fa-f]+|\d+)'
        match = re.search(define_pattern, content)
        if match:
            return match.group(1)

        # Look for #define MACRO ATT_UUID16(0xXXXX)
        define_att_pattern = rf'#define\s+{re.escape(macro)}\s+ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)'
        match = re.search(define_att_pattern, content)
        if match:
            return match.group(1)

        # Look for enum assignment: MACRO = ATT_UUID16(0xXXXX)
        enum_pattern = rf'{re.escape(macro)}\s*=\s*ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)'
        match = re.search(enum_pattern, content)
        if match:
            return match.group(1)

        # Try standard UUID database
        if macro in STANDARD_CHARACTERISTICS:
            return macro.replace("ATT_CHAR_", "0x").replace("_", "").lower()
        if macro in STANDARD_SERVICES:
            return macro.replace("ATT_SVC_", "0x").replace("_", "").lower()

        return None

    def _parse_properties(self, props_macro: str) -> List[str]:
        """Parse property macros into readable names."""
        properties = []

        # Clean up the macro
        props_macro = props_macro.strip()

        # Check for combined properties (PROP_RD | PROP_NTF)
        if '|' in props_macro:
            parts = props_macro.split('|')
            for part in parts:
                prop = part.strip()
                if prop.startswith('PROP_'):
                    prop_name = prop.replace('PROP_', '').lower()
                    properties.append(prop_name)
        else:
            # Single property
            if props_macro.startswith('PROP_'):
                prop_name = props_macro.replace('PROP_', '').lower()
                properties.append(prop_name)

        return properties

    def _parse_att_definitions(self, content: str) -> Tuple[Dict[str, str], Dict[str, str]]:
        """Parse att.h for UUID definitions.

        Returns:
            Tuple of (services, characteristics) where each dict maps
            MACRO_NAME -> uuid_hex (e.g., 'ATT_SVC_BATTERY_SERVICE' -> '0x180F')
            This format is needed for _resolve_uuid() to lookup UUIDs by macro name.
        """
        services = {}
        characteristics = {}

        # Parse service UUIDs - store {MACRO_NAME: uuid}
        svc_pattern = re.compile(
            r'(ATT_SVC_\w+)\s*=\s*ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)',
            re.MULTILINE
        )
        for match in svc_pattern.finditer(content):
            macro_name = match.group(1)
            uuid = match.group(2)
            services[macro_name] = uuid

        # Parse characteristic UUIDs - store {MACRO_NAME: uuid}
        char_pattern = re.compile(
            r'(ATT_CHAR_\w+)\s*=\s*ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)',
            re.MULTILINE
        )
        for match in char_pattern.finditer(content):
            macro_name = match.group(1)
            uuid = match.group(2)
            characteristics[macro_name] = uuid

        return services, characteristics

    def _guess_service_name(self, api_prefix: str) -> str:
        """Guess service name from API prefix."""
        # Common mappings
        prefix_to_name = {
            "bas": "Battery Service",
            "bass": "Battery Service Server",
            "hids": "HID Service",
            "diss": "Device Information Service",
            "scps": "Scan Parameters Service",
        }
        return prefix_to_name.get(api_prefix.lower(), api_prefix.upper() + " Service")

    def _guess_char_name(self, uuid_macro: str) -> str:
        """Guess characteristic name from UUID macro."""
        name = uuid_macro.replace("ATT_CHAR_", "").replace("_", " ")
        return name.title()

    def _extract_service_description(self, content: str, api_prefix: str) -> str:
        """Extract service description from comments."""
        # Look for file header comment
        header_match = re.search(
            rf'/\*\*[\s\S]*?\b{re.escape(api_prefix.upper())}\b[\s\S]*?\*/',
            content,
            re.IGNORECASE
        )
        if header_match:
            comment = header_match.group(0)
            # Extract brief description
            brief_match = re.search(r'@brief\s+([^\n]+)', comment)
            if brief_match:
                return brief_match.group(1).strip()

        return ""

    # ========================================================================
    # Query Methods
    # ========================================================================

    def get_service_by_uuid(self, uuid: str) -> Optional[GATTService]:
        """Find service by UUID."""
        for svc in self.services:
            if svc.uuid == uuid or svc.uuid_hex == uuid:
                return svc
        return None

    def get_services_with_characteristic(self, char_uuid: str) -> List[GATTService]:
        """Find all services that contain a specific characteristic."""
        return [
            svc for svc in self.services
            if svc.get_characteristic_by_uuid(char_uuid)
        ]

    def get_standard_services(self) -> List[GATTService]:
        """Get all standard BLE SIG services."""
        return [s for s in self.services if s.is_standard]

    def get_custom_services(self) -> List[GATTService]:
        """Get all custom/vendor services."""
        return [s for s in self.services if not s.is_standard]

    def build_uuid_index(self) -> Dict[str, GATTService]:
        """Build {uuid: service} index."""
        return {s.uuid: s for s in self.services}

    # ========================================================================
    # Export
    # ========================================================================

    def export_to_json(self, output_path: str, indent: int = 2) -> None:
        """Export all services to JSON."""
        data = {
            "total_services": len(self.services),
            "standard_services": len(self.get_standard_services()),
            "custom_services": len(self.get_custom_services()),
            "services": [s.to_dict() for s in self.services],
            "summary": self._get_summary()
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=indent, ensure_ascii=False)

    def _get_summary(self) -> Dict:
        """Get summary statistics."""
        total_chars = sum(len(s.characteristics) for s in self.services)

        char_props = {}
        for svc in self.services:
            for char in svc.characteristics:
                for prop in char.properties:
                    char_props[prop] = char_props.get(prop, 0) + 1

        return {
            "total_characteristics": total_chars,
            "characteristic_properties": char_props,
            "services_with_notify": len([s for s in self.services if any(c.supports_notify() for c in s.characteristics)]),
            "services_with_indicate": len([s for s in self.services if any(c.supports_indicate() for c in s.characteristics)])
        }


# ============================================================================
# Utility Functions
# ============================================================================

def parse_ble_profiles(sdk_path: str) -> List[GATTService]:
    """Convenience function to parse all BLE profiles."""
    parser = ProfileParser()

    # Parse att.h for standard UUIDs
    att_header = f"{sdk_path}/ble/api/att.h"
    if Path(att_header).exists():
        parser.parse_att_header(att_header)

    # Parse profile directory
    prf_dir = f"{sdk_path}/ble/prf"
    if Path(prf_dir).exists():
        return parser.parse_directory(prf_dir)

    return []


def find_services_by_characteristic(char_uuid: str, sdk_path: str) -> List[GATTService]:
    """Find all services containing a specific characteristic."""
    parser = ProfileParser()
    services = parser.parse_directory(f"{sdk_path}/ble/prf")
    return parser.get_services_with_characteristic(char_uuid)


# ============================================================================
# Main for Testing
# ============================================================================

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        path = sys.argv[1]
    else:
        path = "ble/prf"

    parser = ProfileParser()

    print(f"\n{'='*70}")
    print(f"BLE GATT Profile Parser")
    print(f"{'='*70}")

    # Check if path is file or directory
    if Path(path).is_file():
        services = parser.parse_profile_file(path)
        print(f"\nParsed {len(services)} service(s) from {path}")
    elif Path(path).is_dir():
        services = parser.parse_directory(path)
        print(f"\nParsed {len(services)} service(s) from {path}")
    else:
        print(f"\nPath not found: {path}")
        services = []

    # Print service details
    for svc in services:
        print(f"\n{'─'*70}")
        print(f"Service: {svc.name}")
        print(f"UUID: {svc.uuid_hex} {'[Standard]' if svc.is_standard else '[Custom]'}")
        print(f"API Prefix: {svc.profile_api_prefix}")
        print(f"Characteristics ({len(svc.characteristics)}):")

        for char in svc.characteristics:
            props = ', '.join(char.properties)
            caps = []
            if char.supports_read():
                caps.append("R")
            if char.supports_write():
                caps.append("W")
            if char.supports_notify():
                caps.append("N")
            if char.supports_indicate():
                caps.append("I")

            caps_str = f"[{','.join(caps)}]" if caps else ""
            print(f"  - {char.name} ({char.uuid_hex})")
            print(f"    Properties: {props} {caps_str}")

    # Print summary
    print(f"\n{'='*70}")
    print("Summary:")
    print(f"  Total Services: {len(services)}")
    print(f"  Standard: {len(parser.get_standard_services())}")
    print(f"  Custom: {len(parser.get_custom_services())}")

    # Export JSON
    if services:
        parser.export_to_json("gatt_profiles.json")
        print(f"\nExported to: gatt_profiles.json")
