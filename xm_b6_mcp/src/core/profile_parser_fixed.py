"""
Profile Parser - BLE GATT Profile Definition Parser (FIXED)
============================================================

Extracts:
- GATT Service definitions (UUID, name)
- Characteristic definitions (UUID, properties, permissions)
- Descriptor definitions (CCC, etc.)
- Service-to-characteristic relationships

Supports:
- Profile .c files with C struct initialization syntax
- att.h for standard UUID definitions

AI Use Cases:
- Generate BLE service configuration code
- Create characteristic read/write handlers
- Understand GATT database structure
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple


# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class GATTCharacteristic:
    """GATT Characteristic definition."""
    name: str
    uuid: str
    uuid_hex: str
    properties: List[str]
    permissions: List[str] = field(default_factory=list)
    max_length: int = 256
    variable_length: bool = True
    description: str = ""
    is_standard: bool = False

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "uuid": self.uuid,
            "uuid_hex": self.uuid_hex,
            "properties": self.properties,
            "is_standard": self.is_standard
        }


@dataclass
class GATTService:
    """GATT Service definition."""
    name: str
    uuid: str
    uuid_hex: str
    characteristics: List[GATTCharacteristic] = field(default_factory=list)
    is_primary: bool = True
    is_standard: bool = False
    description: str = ""
    source_file: str = ""
    profile_api_prefix: str = ""

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
            "characteristics": [c.to_dict() for c in self.characteristics]
        }


# ============================================================================
# Standard BLE SIG UUIDs
# ============================================================================

STANDARD_SERVICES = {
    "0x1800": "Generic Access",
    "0x1801": "Generic Attribute",
    "0x180D": "Heart Rate",
    "0x180A": "Device Information",
    "0x180F": "Battery Service",
    "0x1812": "HID Service",
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
}


# ============================================================================
# Fixed Profile Parser
# ============================================================================

class ProfileParser:
    """Parse BLE GATT Profile definitions - FIXED VERSION."""

    def __init__(self):
        self.services: List[GATTService] = []
        self.uuid_cache: Dict[str, str] = {}

    def parse_profile_file(self, file_path: str) -> List[GATTService]:
        """Parse a profile implementation file (.c)."""
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        services = self._parse_profile_content(content, file_path)
        self.services.extend(services)
        return services

    def parse_att_header(self, header_path: str) -> Dict[str, str]:
        """Parse att.h for standard UUID definitions."""
        with open(header_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        return self._parse_att_definitions(content)

    def parse_directory(self, directory: str) -> List[GATTService]:
        """Parse all profile files in a directory."""
        from glob import glob

        all_services = []
        for file_path in glob(f"{directory}/prf_*.c"):
            try:
                services = self.parse_profile_file(file_path)
                all_services.extend(services)
            except Exception as e:
                print(f"Warning: Failed to parse {file_path}: {e}")

        return all_services

    # ========================================================================
    # Content Parsing - FIXED
    # ========================================================================

    def _parse_profile_content(self, content: str, file_path: str) -> List[GATTService]:
        """Parse profile C file for service definitions."""
        services = []

        # Find svc_decl structure using more robust method
        # Match from "const struct svc_decl" to the closing "};"
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

        # Extract UUID field
        uuid_match = re.search(r'\.uuid\s*=\s*(\w+)', svc_body)
        if not uuid_match:
            return None

        uuid_macro = uuid_match.group(1)

        # Resolve UUID from macro definitions
        uuid_value = self._resolve_uuid(uuid_macro, full_content)
        if not uuid_value:
            return None

        # Determine API prefix
        api_prefix = svc_name.replace('_svc_db', '')

        # Get service name
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

        # Extract description from file comments
        service.description = self._extract_service_description(full_content, api_prefix)

        # Parse characteristics
        service.characteristics = self._parse_characteristics_from_content(full_content, api_prefix)

        return service

    def _parse_characteristics_from_content(
        self,
        content: str,
        api_prefix: str
    ) -> List[GATTCharacteristic]:
        """Parse characteristics from profile content."""
        characteristics = []

        # Find characteristic UUID usages in ATT_ELMT macros
        # Pattern: ATT_ELMT(..., ATT_CHAR_XXX, PROP_XXX, ...)
        char_pattern = re.compile(
            r'ATT_ELMT\s*\(\s*\w+,\s*(ATT_CHAR_\w+),\s*([^)]+)\)',
            re.MULTILINE
        )

        for match in char_pattern.finditer(content):
            char_macro = match.group(1)
            props_macro = match.group(2)

            # Resolve UUID
            uuid_value = self._resolve_uuid(char_macro, content)
            if not uuid_value:
                continue

            # Parse properties
            properties = self._parse_properties(props_macro)

            # Get characteristic name
            char_name = STANDARD_CHARACTERISTICS.get(
                uuid_value,
                self._guess_char_name(char_macro)
            )

            characteristic = GATTCharacteristic(
                name=char_name,
                uuid=uuid_value,
                uuid_hex=uuid_value,
                properties=properties,
                is_standard=(uuid_value in STANDARD_CHARACTERISTICS)
            )
            characteristics.append(characteristic)

        return characteristics

    def _resolve_uuid(self, macro: str, content: str) -> Optional[str]:
        """Resolve UUID macro to hex value."""
        # Check cache first
        if macro in self.uuid_cache:
            return self.uuid_cache[macro]

        # If it's already a hex value
        if macro.startswith('0x'):
            self.uuid_cache[macro] = macro
            return macro

        # Known UUID mappings (from BLE SIG)
        known_uuids = {
            'ATT_SVC_BATTERY_SERVICE': '0x180F',
            'ATT_CHAR_BATTERY_LEVEL': '0x2A19',
            'ATT_CHAR_BATTERY_POWER_STATE': '0x2A1A',
            'ATT_CHAR_DEVICE_NAME': '0x2A00',
            'ATT_CHAR_APPEARANCE': '0x2A01',
        }

        if macro in known_uuids:
            self.uuid_cache[macro] = known_uuids[macro]
            return known_uuids[macro]

        # Look for ATT_UUID16 definition
        # Pattern: #define ATT_SVC_XXX  ATT_UUID16(0xXXXX)
        define_pattern = rf'#define\s+{re.escape(macro)}\s+ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)'
        match = re.search(define_pattern, content)
        if match:
            uuid_value = match.group(1)
            self.uuid_cache[macro] = uuid_value
            return uuid_value

        # Look for enum pattern: ATT_SVC_XXX = ATT_UUID16(0xXXXX)
        enum_pattern = rf'{re.escape(macro)}\s*=\s*ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)'
        match = re.search(enum_pattern, content)
        if match:
            uuid_value = match.group(1)
            self.uuid_cache[macro] = uuid_value
            return uuid_value

        # Check if it's a known standard service/characteristic
        if macro in STANDARD_SERVICES or macro in STANDARD_CHARACTERISTICS:
            # Macro name format: ATT_SVC_BATTERY_SERVICE -> 0x180F
            # Need to map macro name to UUID
            # This is a simplified approach
            return None

        return None

    def _parse_properties(self, props_macro: str) -> List[str]:
        """Parse property macros."""
        properties = []

        # Handle combined properties (PROP_RD | PROP_NTF)
        parts = [p.strip() for p in props_macro.split('|')]
        for part in parts:
            if part.startswith('PROP_'):
                prop_name = part.replace('PROP_', '').lower()
                properties.append(prop_name)

        return properties

    def _parse_att_definitions(self, content: str) -> Dict[str, str]:
        """Parse att.h for UUID definitions."""
        uuid_map = {}

        # Parse service UUIDs
        svc_pattern = re.compile(
            r'ATT_SVC_(\w+)\s*=\s*ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)',
            re.MULTILINE
        )
        for match in svc_pattern.finditer(content):
            name = match.group(1)
            uuid = match.group(2)
            uuid_map[f"ATT_SVC_{name}"] = uuid

        # Parse characteristic UUIDs
        char_pattern = re.compile(
            r'ATT_CHAR_(\w+)\s*=\s*ATT_UUID16\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)',
            re.MULTILINE
        )
        for match in char_pattern.finditer(content):
            name = match.group(1)
            uuid = match.group(2)
            uuid_map[f"ATT_CHAR_{name}"] = uuid

        return uuid_map

    def _guess_service_name(self, api_prefix: str) -> str:
        """Guess service name from API prefix."""
        prefix_to_name = {
            "bas": "Battery Service",
            "bass": "Battery Service Server",
            "hids": "HID Service",
            "diss": "Device Information Service",
            "scps": "Scan Parameters Service",
        }
        return prefix_to_name.get(api_prefix.lower(), f"{api_prefix.upper()} Service")

    def _guess_char_name(self, uuid_macro: str) -> str:
        """Guess characteristic name from UUID macro."""
        name = uuid_macro.replace("ATT_CHAR_", "").replace("_", " ")
        return name.title()

    def _extract_service_description(self, content: str, api_prefix: str) -> str:
        """Extract service description from comments."""
        # Look for brief comment
        brief_match = re.search(r'@brief\s+([^\n]+)', content)
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

    def get_standard_services(self) -> List[GATTService]:
        """Get all standard BLE SIG services."""
        return [s for s in self.services if s.is_standard]

    # ========================================================================
    # Export
    # ========================================================================

    def export_to_json(self, output_path: str, indent: int = 2) -> None:
        """Export to JSON."""
        data = {
            "total_services": len(self.services),
            "standard_services": len(self.get_standard_services()),
            "services": [s.to_dict() for s in self.services]
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=indent, ensure_ascii=False)


# ============================================================================
# Test Function
# ============================================================================

if __name__ == "__main__":
    import sys

    sdk_path = r"D:\svn\bxx_DragonC1\sdk6"
    profile_file = f"{sdk_path}/ble/prf/prf_bass.c"

    parser = ProfileParser()

    print(f"Parsing: {profile_file}")
    services = parser.parse_profile_file(profile_file)

    print(f"\nFound {len(services)} service(s)")

    for svc in services:
        print(f"\nService: {svc.name}")
        print(f"UUID: {svc.uuid_hex}")
        print(f"Characteristics: {len(svc.characteristics)}")

        for char in svc.characteristics:
            props = ', '.join(char.properties)
            print(f"  - {char.name}: {props}")
