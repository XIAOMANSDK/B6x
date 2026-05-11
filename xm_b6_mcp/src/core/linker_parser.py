"""
Linker Script Parser - Parse .ld files for Memory Layout
==========================================================

Extracts:
- MEMORY region definitions (FLASH, SRAM, XIP regions)
- SECTION definitions and allocations
- Symbol addresses and mappings
- Entry point definition

AI Use Cases:
- Memory boundary violation detection
- XIP region identification
- Stack/heap size calculation
- Address range validation
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
class MemoryRegion:
    """A memory region defined in MEMORY block."""
    name: str                          # e.g., "FLASH", "RAM"
    origin: int                        # Start address (hex or decimal)
    length: int                        # Size in bytes
    attributes: str                    # e.g., "rx", "rwx", "x"
    region_type: str = "unknown"       # "flash", "ram", "xip", etc.

    def __str__(self) -> str:
        return f"{self.name} (0x{self.origin:08X} - 0x{self.origin + self.length:08X}, {self.attributes})"

    @property
    def origin_hex(self) -> str:
        return f"0x{self.origin:08X}"

    @property
    def end_address(self) -> int:
        return self.origin + self.length

    @property
    def end_address_hex(self) -> str:
        return f"0x{self.origin + self.length:08X}"

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "origin": self.origin,
            "origin_hex": self.origin_hex,
            "length": self.length,
            "length_kb": self.length // 1024,
            "end_address": self.end_address,
            "end_address_hex": self.end_address_hex,
            "attributes": self.attributes,
            "region_type": self.region_type
        }


@dataclass
class SectionDefinition:
    """A section defined in SECTIONS block."""
    name: str                          # e.g., ".text", ".data", ".bss"
    memory_region: str = ""            # Target memory region
    start_addr: Optional[int] = None   # VMA (Virtual Memory Address)
    load_addr: Optional[int] = None    # LMA (Load Memory Address)
    size: int = 0
    attributes: List[str] = field(default_factory=list)  # (COPY), (NOLOAD), etc.
    description: str = ""

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "memory_region": self.memory_region,
            "start_addr": self.start_addr,
            "start_addr_hex": f"0x{self.start_addr:08X}" if self.start_addr else None,
            "load_addr": self.load_addr,
            "load_addr_hex": f"0x{self.load_addr:08X}" if self.load_addr else None,
            "size": self.size,
            "attributes": self.attributes,
            "description": self.description
        }


@dataclass
class SymbolDefinition:
    """A symbol definition (e.g., __StackTop, __HeapLimit)."""
    name: str
    value: int                         # Address or value
    expression: str = ""               # Original expression if computed

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "value": self.value,
            "value_hex": f"0x{self.value:08X}",
            "expression": self.expression
        }


@dataclass
class LinkerScriptParseResult:
    """Complete parse result for a linker script."""
    file_path: str
    entry_point: Optional[str] = None  # e.g., "Reset_Handler"
    memory_regions: List[MemoryRegion] = field(default_factory=list)
    sections: List[SectionDefinition] = field(default_factory=list)
    symbols: Dict[str, SymbolDefinition] = field(default_factory=dict)

    def to_dict(self) -> Dict:
        return {
            "file_path": self.file_path,
            "entry_point": self.entry_point,
            "memory_regions": [r.to_dict() for r in self.memory_regions],
            "sections": [s.to_dict() for s in self.sections],
            "symbols": {name: s.to_dict() for name, s in self.symbols.items()},
            "summary": self.get_summary()
        }

    def get_summary(self) -> Dict:
        """Get summary statistics."""
        total_flash = sum(r.length for r in self.memory_regions if "flash" in r.name.lower())
        total_ram = sum(r.length for r in self.memory_regions if "ram" in r.name.lower())

        return {
            "num_regions": len(self.memory_regions),
            "num_sections": len(self.sections),
            "num_symbols": len(self.symbols),
            "total_flash_bytes": total_flash,
            "total_flash_kb": total_flash // 1024,
            "total_ram_bytes": total_ram,
            "total_ram_kb": total_ram // 1024
        }


# ============================================================================
# Parser Implementation
# ============================================================================

class LinkerScriptParser:
    """Parser for GNU linker script (.ld) files."""

    def __init__(self):
        self.file_path: Optional[str] = None
        self.content: str = ""
        self.parse_result: Optional[LinkerScriptParseResult] = None

    # ========================================================================
    # Public API
    # ========================================================================

    def parse_file(self, file_path: str) -> LinkerScriptParseResult:
        """Parse a linker script file.

        Args:
            file_path: Path to .ld file

        Returns:
            LinkerScriptParseResult with all extracted information
        """
        self.file_path = file_path

        # Read file
        with open(file_path, 'r', encoding='utf-8') as f:
            self.content = f.read()

        # Parse components
        self.parse_result = LinkerScriptParseResult(file_path=file_path)
        self.parse_result.entry_point = self._extract_entry_point()
        self.parse_result.memory_regions = self._extract_memory_regions()
        self.parse_result.sections = self._extract_sections()
        self.parse_result.symbols = self._extract_symbols()

        # Post-process: enhance section info with memory regions
        self._link_sections_to_regions()
        self._infer_region_types()

        return self.parse_result

    def get_memory_region_by_addr(self, address: int) -> Optional[MemoryRegion]:
        """Find which memory region contains an address."""
        if not self.parse_result:
            return None

        for region in self.parse_result.memory_regions:
            if region.origin <= address < (region.origin + region.length):
                return region
        return None

    def get_section_by_name(self, name: str) -> Optional[SectionDefinition]:
        """Get a section by name."""
        if not self.parse_result:
            return None

        for section in self.parse_result.sections:
            if section.name == name:
                return section
        return None

    def export_to_json(self, output_path: str, indent: int = 2) -> None:
        """Export parse result to JSON file."""
        if not self.parse_result:
            raise RuntimeError("No parse result available. Call parse_file() first.")

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(self.parse_result.to_dict(), f, indent=indent, ensure_ascii=False)

    # ========================================================================
    # Entry Point Extraction
    # ========================================================================

    def _extract_entry_point(self) -> Optional[str]:
        """Extract ENTRY() directive."""
        # Match: ENTRY(Reset_Handler)
        match = re.search(r'ENTRY\s*\(\s*(\w+)\s*\)', self.content)
        return match.group(1) if match else None

    # ========================================================================
    # MEMORY Block Extraction
    # ========================================================================

    def _extract_memory_regions(self) -> List[MemoryRegion]:
        """Extract MEMORY block definitions.

        Format:
        MEMORY
        {
          FLASH (rx) : ORIGIN = 0x18008000, LENGTH = 0x38000
          RAM (rwx) :  ORIGIN = 0x20000000, LENGTH = 0x4000
        }
        """
        regions = []

        # Find MEMORY block
        memory_match = re.search(
            r'MEMORY\s*\{([^}]+)\}',
            self.content,
            re.DOTALL
        )

        if not memory_match:
            return regions

        memory_content = memory_match.group(1)

        # Match each region definition
        # Pattern: NAME (attrs) : ORIGIN = addr, LENGTH = size
        region_pattern = re.compile(
            r'(\w+)\s*\(([^)]*)\)\s*:\s*ORIGIN\s*=\s*(0x[0-9A-Fa-f]+|\d+),\s*LENGTH\s*=\s*(0x[0-9A-Fa-f]+|\d+)',
            re.MULTILINE
        )

        for match in region_pattern.finditer(memory_content):
            name = match.group(1)
            attrs = match.group(2).strip()
            origin = self._parse_number(match.group(3))
            length = self._parse_number(match.group(4))

            region = MemoryRegion(
                name=name,
                origin=origin,
                length=length,
                attributes=attrs
            )
            regions.append(region)

        return regions

    # ========================================================================
    # SECTIONS Block Extraction
    # ========================================================================

    def _extract_sections(self) -> List[SectionDefinition]:
        """Extract SECTIONS block definitions.

        Format:
        SECTIONS
        {
            .text :
            {
                *(.text*)
            } > FLASH

            .data : AT (__etext)
            {
                __data_start__ = .;
                *(.data*)
                __data_end__ = .;
            } > RAM
        }
        """
        sections = []

        # Find SECTIONS block
        sections_match = re.search(
            r'SECTIONS\s*\{([^}]+)\}',
            self.content,
            re.DOTALL
        )

        if not sections_match:
            return sections

        sections_content = sections_match.group(1)

        # Match section definitions
        # Pattern: .section_name : [AT(addr)] { ... } > REGION
        section_pattern = re.compile(
            r'(\.[\w.]+)\s*:\s*(?:AT\s*\(\s*(\w+)\s*\))?\s*\{([^}]*(?:\{[^}]*\}[^}]*)*)\}\s*(?:>\s*(\w+))?',
            re.MULTILINE | re.DOTALL
        )

        for match in section_pattern.finditer(sections_content):
            name = match.group(1)
            at_symbol = match.group(2)
            body = match.group(3)
            region = match.group(4) if match.group(4) else ""

            # Extract attributes from section name (COPY, NOLOAD, etc.)
            attributes = re.findall(r'\((\w+)\)', name)

            # Extract symbols defined in section body
            symbols = re.findall(r'(\w+)\s*=\s*\.', body)
            symbols += re.findall(r'PROVIDE(?:_HIDDEN)?\s*\((\w+)\s*=\s*\.', body)

            section = SectionDefinition(
                name=name,
                memory_region=region,
                attributes=attributes,
                description=f"Section with symbols: {', '.join(symbols[:5])}" if symbols else ""
            )
            sections.append(section)

        return sections

    # ========================================================================
    # Symbol Extraction
    # ========================================================================

    def _extract_symbols(self) -> Dict[str, SymbolDefinition]:
        """Extract symbol definitions.

        Matches:
        - __symbol_name = value
        - __symbol_name = expression
        - PROVIDE(__symbol_name = value)
        - PROVIDE_HIDDEN(__symbol_name = value)
        """
        symbols = {}

        # Pattern 1: Simple assignment: symbol = value
        simple_pattern = re.compile(r'(\w+)\s*=\s*([^;]+);')
        for match in simple_pattern.finditer(self.content):
            name = match.group(1)
            expression = match.group(2).strip()

            # Try to evaluate as number
            try:
                value = self._evaluate_expression(expression)
            except (ValueError, SyntaxError):
                value = 0

            symbols[name] = SymbolDefinition(
                name=name,
                value=value,
                expression=expression
            )

        # Pattern 2: PROVIDE(symbol = value)
        provide_pattern = re.compile(
            r'PROVIDE(?:_HIDDEN)?\s*\(\s*(\w+)\s*=\s*([^)]+)\)'
        )
        for match in provide_pattern.finditer(self.content):
            name = match.group(1)
            expression = match.group(2).strip()

            try:
                value = self._evaluate_expression(expression)
            except (ValueError, SyntaxError):
                value = 0

            symbols[name] = SymbolDefinition(
                name=name,
                value=value,
                expression=expression
            )

        return symbols

    # ========================================================================
    # Helper Methods
    # ========================================================================

    def _parse_number(self, num_str: str) -> int:
        """Parse a number (hex or decimal)."""
        num_str = num_str.strip().upper()

        if num_str.startswith('0X') or num_str.startswith('0X'):
            return int(num_str, 16)
        else:
            return int(num_str)

    def _evaluate_expression(self, expr: str) -> int:
        """Evaluate a linker expression.

        Simple evaluator for common patterns:
        - Numbers (0x100, 256)
        - Alignment: ALIGN(n)
        - Current location: .
        - Section size: SIZEOF(.section)
        - Region origin/length: ORIGIN(REGION), LENGTH(REGION)
        """
        expr = expr.strip()

        # Try direct number
        try:
            return self._parse_number(expr)
        except ValueError:
            pass

        # Handle ALIGN(n) - return aligned value (use n as placeholder)
        match = re.match(r'ALIGN\s*\(\s*(\w+)\s*\)', expr)
        if match:
            align_value = self._parse_number(match.group(1)) if match.group(1).startswith('0x') or match.group(1).isdigit() else 4
            return align_value

        # Handle ORIGIN(region) or LENGTH(region)
        match = re.match(r'(ORIGIN|LENGTH)\s*\(\s*(\w+)\s*\)', expr)
        if match:
            func = match.group(1)
            region = match.group(2)

            if self.parse_result:
                for r in self.parse_result.memory_regions:
                    if r.name == region:
                        return r.origin if func == "ORIGIN" else r.length

        # Handle . (current location) - return 0 as placeholder
        if expr.strip() == '.':
            return 0

        # Handle SIZEOF(.section)
        match = re.match(r'SIZEOF\s*\(\s*(\.\w+)\s*\)', expr)
        if match:
            return 0  # Would need section size calculation

        # Default: try eval as Python expression
        try:
            # Replace . with 0 for simple evaluation
            safe_expr = expr.replace('.', '0')
            return eval(safe_expr, {"__builtins__": {}}, {})
        except:
            return 0

    def _link_sections_to_regions(self) -> None:
        """Link sections to their target memory regions."""
        if not self.parse_result:
            return

        for section in self.parse_result.sections:
            if section.memory_region:
                # Find matching region
                for region in self.parse_result.memory_regions:
                    if region.name == section.memory_region:
                        # Set start address from region origin
                        if section.start_addr is None:
                            section.start_addr = region.origin
                        break

    def _infer_region_types(self) -> None:
        """Infer region types from name and attributes."""
        if not self.parse_result:
            return

        for region in self.parse_result.memory_regions:
            name_upper = region.name.upper()

            # Infer from name
            if 'FLASH' in name_upper or 'ROM' in name_upper:
                region.region_type = 'flash'
            elif 'RAM' in name_upper or 'SRAM' in name_upper:
                region.region_type = 'ram'
            elif 'XIP' in name_upper:
                region.region_type = 'xip'

            # Infer from attributes
            if region.region_type == 'unknown':
                if 'x' in region.attributes.lower():  # Executable
                    if 'r' in region.attributes.lower():  # Readable
                        region.region_type = 'flash' if 'w' not in region.attributes.lower() else 'ram'
                    else:
                        region.region_type = 'flash'


# ============================================================================
# Utility Functions
# ============================================================================

def parse_linker_script(file_path: str) -> LinkerScriptParseResult:
    """Convenience function to parse a linker script."""
    parser = LinkerScriptParser()
    return parser.parse_file(file_path)


def parse_multiple_linker_scripts(file_patterns: List[str]) -> Dict[str, LinkerScriptParseResult]:
    """Parse multiple linker scripts matching patterns."""
    from glob import glob

    results = {}
    for pattern in file_patterns:
        for file_path in glob(pattern):
            try:
                results[file_path] = parse_linker_script(file_path)
            except Exception as e:
                print(f"Warning: Failed to parse {file_path}: {e}")

    return results


def export_combined_memory_map(results: Dict[str, LinkerScriptParseResult], output_path: str) -> None:
    """Export combined memory map from multiple linker scripts."""
    combined = {
        "scripts": {},
        "all_regions": [],
        "all_symbols": {}
    }

    for file_path, result in results.items():
        combined["scripts"][file_path] = result.to_dict()
        combined["all_regions"].extend([r.to_dict() for r in result.memory_regions])
        combined["all_symbols"].update({k: v.to_dict() for k, v in result.symbols.items()})

    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(combined, f, indent=2, ensure_ascii=False)


# ============================================================================
# Main for Testing
# ============================================================================

if __name__ == "__main__":
    # Test with SDK linker script
    import sys

    if len(sys.argv) > 1:
        ld_file = sys.argv[1]
    else:
        ld_file = "core/gnu/link_xip.ld"

    parser = LinkerScriptParser()
    result = parser.parse_file(ld_file)

    print(f"\n{'='*70}")
    print(f"Linker Script Analysis: {ld_file}")
    print(f"{'='*70}")

    print(f"\nEntry Point: {result.entry_point}")

    print(f"\nMemory Regions ({len(result.memory_regions)}):")
    for region in result.memory_regions:
        print(f"  {region}")

    print(f"\nSections ({len(result.sections)}):")
    for section in result.sections[:10]:
        mem_info = f" -> {section.memory_region}" if section.memory_region else ""
        print(f"  {section.name}{mem_info}")
    if len(result.sections) > 10:
        print(f"  ... and {len(result.sections) - 10} more")

    print(f"\nSymbols ({len(result.symbols)}):")
    for name, symbol in list(result.symbols.items())[:10]:
        print(f"  {name} = 0x{symbol.value:08X}")
    if len(result.symbols) > 10:
        print(f"  ... and {len(result.symbols) - 10} more")

    # Export JSON
    parser.export_to_json("memory_map.json")
    print(f"\nExported to: memory_map.json")
