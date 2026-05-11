"""
Struct Extractor - Enhanced Structure Definition Extraction
===========================================================

Extracts:
- C structure definitions from headers
- Member types, names, bit-fields
- Member offsets (calculated)
- Structure size and alignment
- Peripheral config structure identification

Uses Tree-sitter for accurate C parsing.

AI Use Cases:
- Generate peripheral configuration code
- Create struct initialization examples
- Understand register mapping layouts
- Type-safe API code generation
"""

import re
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple, Set
from collections import OrderedDict

# Tree-sitter is REQUIRED dependency
from tree_sitter import Language, Parser, Node


# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class StructMember:
    """A structure member."""
    name: str
    type: str                    # Full type name (e.g., "uint32_t", "const char *")
    bit_field: Optional[int] = None  # Bit-field width (if applicable)
    offset: int = 0              # Byte offset from struct start (calculated)
    size: int = 0                # Size in bytes (estimated)
    array_size: Optional[int] = None  # Array size (if array)
    description: str = ""        # Comment/description

    def is_array(self) -> bool:
        """Check if member is an array."""
        return self.array_size is not None

    def is_bit_field(self) -> bool:
        """Check if member is a bit-field."""
        return self.bit_field is not None

    def is_pointer(self) -> bool:
        """Check if member is a pointer type."""
        return "*" in self.type

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "type": self.type,
            "bit_field": self.bit_field,
            "offset": self.offset,
            "offset_hex": f"0x{self.offset:04X}",
            "size": self.size,
            "array_size": self.array_size,
            "description": self.description
        }


@dataclass
class StructDefinition:
    """Complete structure definition."""
    name: str
    members: List[StructMember]
    is_union: bool = False
    is_anonymous: bool = False
    size_bytes: int = 0           # Total size (calculated)
    alignment: int = 0            # Alignment requirement
    file_path: str = ""
    line_number: int = 0
    description: str = ""
    category: str = ""            # peripheral_config / generic / register_map
    underlying_type: str = ""     # For typedef: e.g., "struct __FILE"

    def get_member_by_name(self, name: str) -> Optional[StructMember]:
        """Get member by name."""
        for m in self.members:
            if m.name == name:
                return m
        return None

    def get_bit_fields(self) -> List[StructMember]:
        """Get all bit-field members."""
        return [m for m in self.members if m.is_bit_field()]

    def get_arrays(self) -> List[StructMember]:
        """Get all array members."""
        return [m for m in self.members if m.is_array()]

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "members": [m.to_dict() for m in self.members],
            "is_union": self.is_union,
            "is_anonymous": self.is_anonymous,
            "size_bytes": self.size_bytes,
            "alignment": self.alignment,
            "file_path": self.file_path,
            "line_number": self.line_number,
            "description": self.description,
            "category": self.category,
            "underlying_type": self.underlying_type
        }


# ============================================================================
# Fallback Regex Parser (when tree-sitter unavailable)
# ============================================================================

class RegexStructParser:
    """Fallback regex-based struct parser."""

    # Basic type keywords
    TYPE_KEYWORDS = {
        'void', 'char', 'short', 'int', 'long', 'float', 'double',
        'signed', 'unsigned', 'bool', '_Bool'
    }

    # Common stdint types
    STDINT_TYPES = {
        'int8_t', 'uint8_t', 'int16_t', 'uint16_t',
        'int32_t', 'uint32_t', 'int64_t', 'uint64_t',
        'intptr_t', 'uintptr_t', 'size_t', 'ssize_t'
    }

    def __init__(self):
        self.structs: List[StructDefinition] = []

    def parse_file(self, file_path: str) -> List[StructDefinition]:
        """Parse C header file for struct definitions."""
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        return self.parse_content(content, file_path)

    def parse_content(self, content: str, file_path: str = "") -> List[StructDefinition]:
        """Parse content for struct definitions."""
        structs = []

        # Remove comments
        content = self._remove_comments(content)

        # Find struct definitions
        # Pattern: struct [name] { ... } [var_name];
        struct_pattern = re.compile(
            r'(?:typedef\s+)?struct\s+(\w*)\s*\{([^}]+)\}\s*(\w+)\s*;',
            re.MULTILINE | re.DOTALL
        )

        for match in struct_pattern.finditer(content):
            struct_name = match.group(1) or match.group(3)  # Name can be before {} or after
            body = match.group(2)

            struct = self._parse_struct_body(struct_name, body, file_path)
            if struct:
                structs.append(struct)

        return structs

    def _remove_comments(self, content: str) -> str:
        """Remove C and C++ comments."""
        # Remove single-line comments
        content = re.sub(r'//.*?$', '', content, flags=re.MULTILINE)
        # Remove multi-line comments
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        return content

    def _parse_struct_body(
        self,
        name: str,
        body: str,
        file_path: str
    ) -> Optional[StructDefinition]:
        """Parse struct body."""
        members = []

        # Split by semicolons
        lines = body.split(';')

        offset = 0
        for line in lines:
            line = line.strip()
            if not line:
                continue

            # Parse member: type name [array_size];
            # Handle bit fields: type name : width;
            member = self._parse_member(line, offset)
            if member:
                members.append(member)
                offset += member.size

        if not members:
            return None

        return StructDefinition(
            name=name,
            members=members,
            size_bytes=offset,
            file_path=file_path,
            category="generic"
        )

    def _parse_member(self, line: str, offset: int) -> Optional[StructMember]:
        """Parse a member declaration."""
        # Pattern: type name [: bits] [array_size]
        match = re.match(
            r'^(.+?)\s+(\w+)\s*(?::\s*(\d+))?\s*(?:\[(\d+)\])?\s*$',
            line.strip()
        )

        if not match:
            return None

        type_name = match.group(1).strip()
        member_name = match.group(2)
        bit_field = int(match.group(3)) if match.group(3) else None
        array_size = int(match.group(4)) if match.group(4) else None

        # Estimate size
        size = self._estimate_type_size(type_name, array_size)

        return StructMember(
            name=member_name,
            type=type_name,
            bit_field=bit_field,
            offset=offset,
            size=size,
            array_size=array_size
        )

    def _estimate_type_size(self, type_name: str, array_size: Optional[int]) -> int:
        """Estimate type size in bytes."""
        type_name = type_name.strip().lower()
        base_size = 4  # Default

        # Check base types
        if 'char' in type_name or 'int8' in type_name or 'uint8' in type_name:
            base_size = 1
        elif 'short' in type_name or 'int16' in type_name or 'uint16' in type_name:
            base_size = 2
        elif 'long' in type_name or 'int32' in type_name or 'uint32' in type_name:
            base_size = 4
        elif 'long long' in type_name or 'int64' in type_name or 'uint64' in type_name:
            base_size = 8
        elif 'float' in type_name:
            base_size = 4
        elif 'double' in type_name:
            base_size = 8

        # Account for pointers
        if '*' in type_name:
            base_size = 4  # 32-bit pointer

        # Account for arrays
        if array_size:
            base_size *= array_size

        return base_size


# ============================================================================
# Tree-sitter Based Parser
# ============================================================================

class TreeSitterStructParser:
    """Accurate struct parser using tree-sitter."""

    def __init__(self):
        self.language = None
        self.parser = None
        self._init_language()

    def _init_language(self):
        """Initialize tree-sitter C language and parser."""
        try:
            # Try to load from common locations
            for lib_path in [
                Path(__file__).parent.parent.parent / "lib" / "tree-sitter-c" / "build.so",
                Path.home() / ".tree-sitter" / "lib" / "tree-sitter" / "c.so",
            ]:
                if lib_path.exists():
                    self.language = Language(str(lib_path), 'c')
                    break

            if not self.language:
                # Fallback: try pip installed location
                # Note: tree_sitter_c.language() returns PyCapsule, wrap with Language()
                import tree_sitter_c
                self.language = Language(tree_sitter_c.language())

            # Create parser with language (new API)
            self.parser = Parser(self.language)

        except Exception as e:
            print(f"Warning: Failed to load tree-sitter C language: {e}")
            raise

    def parse_file(self, file_path: str) -> List[StructDefinition]:
        """Parse C header file for struct definitions."""
        with open(file_path, 'rb') as f:
            source_code = f.read()

        return self.parse_source(source_code, file_path)

    def parse_source(self, source_code: bytes, file_path: str = "") -> List[StructDefinition]:
        """Parse source code for struct definitions."""
        tree = self.parser.parse(source_code)
        root_node = tree.root_node

        structs = []

        # Find all struct declarations
        for node in root_node.children:
            structs.extend(self._find_structs(node, source_code, file_path))

        # Calculate layouts
        for struct in structs:
            self._calculate_layout(struct)

        # Identify categories
        for struct in structs:
            struct.category = self._identify_category(struct)

        return structs

    def _find_structs(
        self,
        node: Node,
        source_code: bytes,
        file_path: str
    ) -> List[StructDefinition]:
        """Recursively find struct definitions."""
        structs = []

        # Check if this is a struct declaration
        if node.type == 'struct_specifier':
            struct = self._parse_struct(node, source_code, file_path)
            if struct:
                structs.append(struct)

        # Recurse into children
        for child in node.children:
            structs.extend(self._find_structs(child, source_code, file_path))

        return structs

    def _parse_struct(
        self,
        node: Node,
        source_code: bytes,
        file_path: str
    ) -> Optional[StructDefinition]:
        """Parse a struct_specifier node."""
        # Get struct name (field_declaration or type_identifier)
        name = ""
        name_node = node.child_by_field_name('name')
        if name_node:
            name = name_node.text.decode('utf-8', errors='ignore')

        # Get body (field_declaration_list)
        body_node = node.child_by_field_name('body')
        if not body_node:
            return None

        # Parse members
        members = []
        if body_node.type == 'field_declaration_list':
            for child in body_node.children:
                if child.type == 'field_declaration':
                    member = self._parse_member(child, source_code)
                    if member:
                        members.append(member)

        if not members:
            return None

        # Get line number
        line_number = node.start_point[0] + 1

        return StructDefinition(
            name=name or "<anonymous>",
            members=members,
            is_union=False,
            file_path=file_path,
            line_number=line_number
        )

    def _parse_member(self, node: Node, source_code: bytes) -> Optional[StructMember]:
        """Parse a field_declaration node."""
        # Get type
        type_node = node.child_by_field_name('type')
        if not type_node:
            return None

        type_text = type_node.text.decode('utf-8', errors='ignore').strip()

        # Get name
        name_node = node.child_by_field_name('declarator')
        if name_node:
            name_text = name_node.text.decode('utf-8', errors='ignore').strip()

            # Handle arrays
            array_size = None
            if '[' in name_text:
                match = re.search(r'\[(\d+)\]', name_text)
                if match:
                    array_size = int(match.group(1))
                name_text = name_text.split('[')[0].strip()

            # Handle bit fields
            bit_field = None
            if ':' in name_text:
                match = re.search(r':\s*(\d+)', name_text)
                if match:
                    bit_field = int(match.group(1))
                name_text = name_text.split(':')[0].strip()
        else:
            name_text = ""

        return StructMember(
            name=name_text,
            type=type_text,
            bit_field=bit_field,
            array_size=array_size
        )

    def _calculate_layout(self, struct: StructDefinition) -> None:
        """Calculate member offsets and struct size."""
        offset = 0
        max_alignment = 1

        for member in struct.members:
            # Calculate member size
            member.size = self._estimate_type_size(member.type, member.array_size)

            # Handle bit fields
            if member.bit_field:
                # Bit fields share storage units
                pass

            # Alignment
            alignment = self._get_type_alignment(member.type)
            max_alignment = max(max_alignment, alignment)

            # Align offset
            if not member.bit_field:
                offset = (offset + alignment - 1) & ~(alignment - 1)

            member.offset = offset
            offset += member.size

        # Total size with padding
        struct.size_bytes = (offset + max_alignment - 1) & ~(max_alignment - 1)
        struct.alignment = max_alignment

    def _estimate_type_size(self, type_name: str, array_size: Optional[int]) -> int:
        """Estimate type size."""
        type_lower = type_name.lower().strip()

        # Base types
        if 'char' in type_lower or 'int8' in type_lower or 'uint8' in type_lower:
            base = 1
        elif 'short' in type_lower or 'int16' in type_lower or 'uint16' in type_lower:
            base = 2
        elif 'long' in type_lower or 'int32' in type_lower or 'uint32' in type_lower:
            base = 4
        elif 'int64' in type_lower or 'uint64' in type_lower:
            base = 8
        elif 'float' in type_lower:
            base = 4
        elif 'double' in type_lower:
            base = 8
        elif 'bool' in type_lower:
            base = 1
        elif 'void' in type_lower:
            base = 0
        elif '*' in type_lower:
            base = 4  # Pointer
        else:
            base = 4  # Default

        if array_size:
            base *= array_size

        return base

    def _get_type_alignment(self, type_name: str) -> int:
        """Get type alignment requirement."""
        type_lower = type_name.lower()

        if 'char' in type_lower or 'int8' in type_lower or 'uint8' in type_lower or 'bool' in type_lower:
            return 1
        elif 'short' in type_lower or 'int16' in type_lower or 'uint16' in type_lower:
            return 2
        elif any(t in type_lower for t in ['long', 'int32', 'uint32', 'float', '*']):
            return 4
        elif 'int64' in type_lower or 'uint64' in type_lower or 'double' in type_lower:
            return 8
        else:
            return 4

    def _identify_category(self, struct: StructDefinition) -> str:
        """Identify struct category based on naming patterns."""
        name_lower = struct.name.lower()

        # Peripheral config structures
        if any(pattern in name_lower for pattern in ['config', 'init', 'cfg', 'conf']):
            return "peripheral_config"

        # Register maps
        if any(pattern in name_lower for pattern in ['regs', 'reg', 'registers']):
            return "register_map"

        # Handle types
        if 'handle' in name_lower or 'hdl' in name_lower:
            return "handle_type"

        # Default
        return "generic"


# ============================================================================
# Main Struct Extractor
# ============================================================================

class StructExtractor:
    """Main struct extraction coordinator."""

    def __init__(self):
        # Tree-sitter is required - use TreeSitterStructParser
        self.parser = TreeSitterStructParser()
        self.using_tree_sitter = True

    def extract_from_file(self, file_path: str) -> List[StructDefinition]:
        """Extract struct definitions from a file."""
        return self.parser.parse_file(file_path)

    def extract_from_directory(
        self,
        directory: str,
        pattern: str = "*.h"
    ) -> Dict[str, List[StructDefinition]]:
        """Extract structs from all matching files in directory."""
        from glob import glob

        results = {}
        for file_path in glob(f"{directory}/{pattern}"):
            try:
                structs = self.extract_from_file(file_path)
                if structs:
                    results[file_path] = structs
            except Exception as e:
                print(f"Warning: Failed to parse {file_path}: {e}")

        return results

    def identify_peripheral_configs(
        self,
        structs: List[StructDefinition]
    ) -> List[StructDefinition]:
        """Identify peripheral configuration structures."""
        return [s for s in structs if s.category == "peripheral_config"]

    def get_struct_by_name(
        self,
        structs: List[StructDefinition],
        name: str
    ) -> Optional[StructDefinition]:
        """Find struct by name."""
        for struct in structs:
            if struct.name == name:
                return struct
        return None

    def export_to_json(
        self,
        structs: List[StructDefinition],
        output_path: str,
        indent: int = 2
    ) -> None:
        """Export to JSON."""
        import json

        data = {
            "parser": "tree_sitter" if self.using_tree_sitter else "regex",
            "total_structs": len(structs),
            "structs": [s.to_dict() for s in structs],
            "summary": self._get_summary(structs)
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=indent, ensure_ascii=False)

    def _get_summary(self, structs: List[StructDefinition]) -> Dict:
        """Get summary statistics."""
        categories = {}
        for s in structs:
            categories[s.category] = categories.get(s.category, 0) + 1

        total_members = sum(len(s.members) for s in structs)

        return {
            "by_category": categories,
            "total_members": total_members,
            "peripheral_config_structs": categories.get("peripheral_config", 0)
        }


# ============================================================================
# Utility Functions
# ============================================================================

def extract_structs(file_path: str) -> List[StructDefinition]:
    """Convenience function to extract structs."""
    extractor = StructExtractor()
    return extractor.extract_from_file(file_path)


def extract_all_structs(sdk_path: str) -> Dict[str, List[StructDefinition]]:
    """Extract all structs from SDK driver headers."""
    extractor = StructExtractor()

    api_dir = f"{sdk_path}/drivers/api"
    return extractor.extract_from_directory(api_dir, "*.h")


# ============================================================================
# Main for Testing
# ============================================================================

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        header_file = sys.argv[1]
    else:
        header_file = "drivers/api/gpio.h"

    extractor = StructExtractor()

    print(f"\n{'='*70}")
    print(f"Struct Extraction: {header_file}")
    print(f"Parser: {'tree_sitter' if extractor.using_tree_sitter else 'regex (fallback)'}")
    print(f"{'='*70}")

    structs = extractor.extract_from_file(header_file)

    print(f"\nFound {len(structs)} structure(s)")

    for struct in structs:
        print(f"\n{'─'*70}")
        print(f"struct {struct.name} {{")
        print(f"  Size: {struct.size_bytes} bytes, Alignment: {struct.alignment}")
        print(f"  Category: {struct.category}")
        print(f"  Members ({len(struct.members)}):")

        for member in struct.members:
            bits = f":{member.bit_field}" if member.bit_field else ""
            array = f"[{member.array_size}]" if member.array_size else ""
            print(f"    {member.type} {member.name}{array}{bits};"
                  f"  // offset +{member.offset} (0x{member.offset:04X})")

        print(f"}};")

    # Export JSON
    extractor.export_to_json(structs, "structs.json")
    print(f"\n\nExported to: structs.json")
