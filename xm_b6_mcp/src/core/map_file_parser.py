"""
Map File Parser - Parse .map files for Memory Usage Analysis
=============================================================

Supports:
- GNU ld map files (arm-none-eabi-gcc)
- Keil MDK map files

Extracts:
- Memory region definitions (FLASH, RAM, SRAM blocks)
- Section sizes (.text, .data, .bss, .stack, .heap)
- Module-level memory breakdown
- Total SRAM usage for static memory analysis

AI Use Cases:
- Detect static memory overflow before runtime
- Validate stack depth estimation
- Compare memory usage across builds
- Identify memory-hungry modules

Author: B6x MCP Server Team
Version: 1.0.0
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple, Literal
from enum import Enum


class MapFileType(str, Enum):
    """Map file type enumeration."""
    GNU_LD = "gnu_ld"       # GNU linker (arm-none-eabi-gcc)
    KEIL_MDK = "keil_mdk"   # Keil MDK


@dataclass
class MemoryRegion:
    """Memory region definition from map file."""
    name: str
    origin: int
    length: int
    attributes: str = ""

    @property
    def end_address(self) -> int:
        return self.origin + self.length

    @property
    def length_kb(self) -> float:
        return self.length / 1024

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "origin": f"0x{self.origin:08X}",
            "length": self.length,
            "length_kb": round(self.length_kb, 2),
            "end_address": f"0x{self.end_address:08X}",
            "attributes": self.attributes
        }


@dataclass
class SectionInfo:
    """Section information from map file."""
    name: str
    start_address: int
    size: int
    memory_region: str = ""

    @property
    def size_kb(self) -> float:
        return self.size / 1024

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "start_address": f"0x{self.start_address:08X}",
            "size": self.size,
            "size_kb": round(self.size_kb, 2),
            "memory_region": self.memory_region
        }


@dataclass
class MemoryUsageReport:
    """Complete memory usage report from map file analysis."""
    file_path: str
    file_type: MapFileType

    # Memory regions
    regions: List[MemoryRegion] = field(default_factory=list)

    # Key sections
    sections: Dict[str, SectionInfo] = field(default_factory=dict)

    # SRAM analysis
    sram_total_bytes: int = 0
    sram_used_bytes: int = 0
    sram_free_bytes: int = 0

    # Static memory breakdown
    data_bytes: int = 0      # .data (initialized globals)
    bss_bytes: int = 0       # .bss (uninitialized globals)
    vector_bytes: int = 0    # Interrupt vector table
    stack_bytes: int = 0     # Stack size (from linker script)
    heap_bytes: int = 0      # Heap size (from linker script)

    # Flash breakdown
    flash_total_bytes: int = 0
    text_bytes: int = 0      # .text (code)
    rodata_bytes: int = 0    # .rodata (constants)

    # Module breakdown (top memory consumers)
    top_modules: List[Dict] = field(default_factory=list)

    def to_dict(self) -> Dict:
        return {
            "file_path": self.file_path,
            "file_type": self.file_type.value,
            "memory_regions": [r.to_dict() for r in self.regions],
            "sections": {k: v.to_dict() for k, v in self.sections.items()},
            "sram_summary": {
                "total_bytes": self.sram_total_bytes,
                "total_kb": round(self.sram_total_bytes / 1024, 2),
                "used_bytes": self.sram_used_bytes,
                "used_kb": round(self.sram_used_bytes / 1024, 2),
                "free_bytes": self.sram_free_bytes,
                "free_kb": round(self.sram_free_bytes / 1024, 2),
                "usage_percent": round(self.sram_used_bytes / self.sram_total_bytes * 100, 2) if self.sram_total_bytes > 0 else 0
            },
            "static_memory_breakdown": {
                "data_bytes": self.data_bytes,
                "bss_bytes": self.bss_bytes,
                "vector_bytes": self.vector_bytes,
                "stack_bytes": self.stack_bytes,
                "heap_bytes": self.heap_bytes,
                "total_static_bytes": self.data_bytes + self.bss_bytes + self.vector_bytes,
                "total_static_kb": round((self.data_bytes + self.bss_bytes + self.vector_bytes) / 1024, 2)
            },
            "flash_summary": {
                "total_bytes": self.flash_total_bytes,
                "total_kb": round(self.flash_total_bytes / 1024, 2),
                "text_bytes": self.text_bytes,
                "text_kb": round(self.text_bytes / 1024, 2),
                "rodata_bytes": self.rodata_bytes
            },
            "top_modules": self.top_modules
        }


class MapFileParser:
    """
    Parser for linker map files.

    Supports:
    - GNU ld map files (arm-none-eabi-gcc)
    - Keil MDK map files
    """

    def __init__(self):
        self.content: str = ""
        self.file_type: Optional[MapFileType] = None

    def parse_file(self, file_path: str) -> MemoryUsageReport:
        """
        Parse a map file and return memory usage report.

        Args:
            file_path: Path to .map file

        Returns:
            MemoryUsageReport with all extracted information
        """
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            self.content = f.read()

        # Detect file type
        self.file_type = self._detect_file_type()

        # Parse based on type
        if self.file_type == MapFileType.GNU_LD:
            return self._parse_gnu_ld(file_path)
        elif self.file_type == MapFileType.KEIL_MDK:
            return self._parse_keil_mdk(file_path)
        else:
            raise ValueError(f"Unknown map file type: {file_path}")

    def _detect_file_type(self) -> MapFileType:
        """Detect map file type based on content patterns."""
        # GNU ld patterns
        if "Memory Configuration" in self.content and "Linker script" in self.content:
            return MapFileType.GNU_LD

        # Keil MDK patterns
        if "Program Size" in self.content or "Code (inc. data)" in self.content:
            return MapFileType.KEIL_MDK

        # Default to GNU ld
        return MapFileType.GNU_LD

    # ========================================================================
    # GNU ld Parser
    # ========================================================================

    def _parse_gnu_ld(self, file_path: str) -> MemoryUsageReport:
        """Parse GNU ld map file format."""
        report = MemoryUsageReport(
            file_path=file_path,
            file_type=MapFileType.GNU_LD
        )

        # Parse memory regions
        report.regions = self._parse_gnu_memory_regions()

        # Parse sections
        sections = self._parse_gnu_sections()
        report.sections = sections

        # Extract key section sizes
        report.text_bytes = sections.get(".text", SectionInfo(".text", 0, 0)).size
        report.data_bytes = sections.get(".data", SectionInfo(".data", 0, 0)).size
        report.bss_bytes = sections.get(".bss", SectionInfo(".bss", 0, 0)).size
        report.vector_bytes = sections.get(".vector", SectionInfo(".vector", 0, 0)).size

        # Calculate SRAM totals
        for region in report.regions:
            if "RAM" in region.name.upper() or "SRAM" in region.name.upper():
                report.sram_total_bytes += region.length
            elif "FLASH" in region.name.upper():
                report.flash_total_bytes += region.length

        # Calculate SRAM used (static memory)
        report.sram_used_bytes = report.vector_bytes + report.data_bytes + report.bss_bytes
        report.sram_free_bytes = report.sram_total_bytes - report.sram_used_bytes

        # Try to extract stack/heap from symbols
        self._extract_stack_heap_symbols(report)

        return report

    def _parse_gnu_memory_regions(self) -> List[MemoryRegion]:
        """Parse Memory Configuration section."""
        regions = []

        # Find Memory Configuration section
        pattern = r'Memory Configuration\s+Name\s+Origin\s+Length\s+Attributes\s*\n(.*?)(?=\n\n|\nLinker script)'
        match = re.search(pattern, self.content, re.DOTALL)

        if not match:
            return regions

        content = match.group(1)

        # Parse each region line
        # Format: NAME             ORIGIN             LENGTH             ATTRIBUTES
        region_pattern = re.compile(
            r'(\w+)\s+(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+)\s+(\w*)'
        )

        for m in region_pattern.finditer(content):
            name = m.group(1)
            if name == "*default*":
                continue

            origin = int(m.group(2), 16)
            length = int(m.group(3), 16)
            attrs = m.group(4) if len(m.groups()) >= 4 else ""

            regions.append(MemoryRegion(
                name=name,
                origin=origin,
                length=length,
                attributes=attrs
            ))

        return regions

    def _parse_gnu_sections(self) -> Dict[str, SectionInfo]:
        """Parse section definitions.

        Handles GNU ld map file formats like:
        - .text           0x18004000    0x13318
        - .data           0x20003098       0x5c load address 0x180190a0
        """
        sections = {}

        # Generic pattern for section lines at the start of a line
        # Matches: .section_name    0xADDRESS    0xSIZE [optional load address]
        section_pattern = re.compile(
            r'^\.(\w+)\s+(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+)',
            re.MULTILINE
        )

        for m in section_pattern.finditer(self.content):
            name = "." + m.group(1)
            start_addr = int(m.group(2), 16)
            size = int(m.group(3), 16)

            # Skip sections with 0 size or 0 address (discarded)
            if size == 0 or start_addr == 0:
                continue

            # Only capture main sections
            if name in [".text", ".data", ".bss", ".vector", ".rodata", ".heap", ".stack"]:
                # Determine memory region based on address
                if start_addr >= 0x20000000:
                    region = "RAM"
                else:
                    region = "FLASH"

                sections[name] = SectionInfo(
                    name=name,
                    start_address=start_addr,
                    size=size,
                    memory_region=region
                )

        return sections

    def _extract_stack_heap_symbols(self, report: MemoryUsageReport) -> None:
        """Extract stack and heap sizes from symbols."""
        # Look for Stack_Size and Heap_Size symbols
        stack_match = re.search(r'Stack_Size\s*=\s*0x([0-9A-Fa-f]+)', self.content)
        if stack_match:
            report.stack_bytes = int(stack_match.group(1), 16)

        heap_match = re.search(r'Heap_Size\s*=\s*0x([0-9A-Fa-f]+)', self.content)
        if heap_match:
            report.heap_bytes = int(heap_match.group(1), 16)

        # Also check for __StackTop and __HeapBase patterns
        if report.stack_bytes == 0:
            # Try to infer from linker script symbols
            stack_top = re.search(r'__StackTop\s*=\s*0x([0-9A-Fa-f]+)', self.content)
            stack_base = re.search(r'__StackLimit\s*=\s*0x([0-9A-Fa-f]+)', self.content)
            if stack_top and stack_base:
                report.stack_bytes = int(stack_top.group(1), 16) - int(stack_base.group(1), 16)

    # ========================================================================
    # Keil MDK Parser
    # ========================================================================

    def _parse_keil_mdk(self, file_path: str) -> MemoryUsageReport:
        """Parse Keil MDK map file format."""
        report = MemoryUsageReport(
            file_path=file_path,
            file_type=MapFileType.KEIL_MDK
        )

        # Keil map files have a "Program Size" section
        # Format: Code (inc. data)   RO Data    RW Data    ZI Data      Debug
        size_match = re.search(
            r'Program Size:\s+Code=([0-9]+)\s+RO-data=([0-9]+)\s+RW-data=([0-9]+)\s+ZI-data=([0-9]+)',
            self.content
        )

        if size_match:
            report.text_bytes = int(size_match.group(1))
            report.rodata_bytes = int(size_match.group(2))
            report.data_bytes = int(size_match.group(3))
            report.bss_bytes = int(size_match.group(4))

        # Alternative format: "Total ROM Size"
        if report.text_bytes == 0:
            rom_match = re.search(
                r'Total ROM Size\s*\(Code \+ RO Data \+ RW Data\)\s*=\s*([0-9]+)',
                self.content
            )
            if rom_match:
                report.text_bytes = int(rom_match.group(1))

        # Calculate totals
        report.sram_used_bytes = report.data_bytes + report.bss_bytes

        # Try to extract memory regions from Keil map
        report.regions = self._parse_keil_memory_regions()
        for region in report.regions:
            if "RAM" in region.name.upper():
                report.sram_total_bytes += region.length
            elif "ROM" in region.name.upper() or "FLASH" in region.name.upper():
                report.flash_total_bytes += region.length

        report.sram_free_bytes = report.sram_total_bytes - report.sram_used_bytes

        return report

    def _parse_keil_memory_regions(self) -> List[MemoryRegion]:
        """Parse memory regions from Keil map file."""
        regions = []

        # Look for Execution Region definitions
        # Format: Execution Region RW_IRAM1 (Exec base: 0x20000000, Load base: 0x08004000, Size: 0x1000, Max: 0x10000, ABSOLUTE)
        pattern = re.compile(
            r'Execution Region\s+(\w+)\s+\(Exec base:\s*(0x[0-9A-Fa-f]+),\s*Load base:\s*(?:0x[0-9A-Fa-f]+),\s*Size:\s*(0x[0-9A-Fa-f]+),\s*Max:\s*(0x[0-9A-Fa-f]+)'
        )

        for m in pattern.finditer(self.content):
            name = m.group(1)
            origin = int(m.group(2), 16)
            size = int(m.group(3), 16)
            max_size = int(m.group(4), 16)

            regions.append(MemoryRegion(
                name=name,
                origin=origin,
                length=max_size,  # Use max size as region size
                attributes="rw" if "RAM" in name.upper() else "rx"
            ))

        return regions


# ============================================================================
# Utility Functions
# ============================================================================

def parse_map_file(file_path: str) -> MemoryUsageReport:
    """Convenience function to parse a map file."""
    parser = MapFileParser()
    return parser.parse_file(file_path)


def get_static_memory_usage(map_file: str) -> Tuple[int, int, int]:
    """
    Get static memory usage from map file.

    Returns:
        Tuple of (data_bytes, bss_bytes, total_bytes)
    """
    report = parse_map_file(map_file)
    return (report.data_bytes, report.bss_bytes, report.data_bytes + report.bss_bytes)


def validate_memory_usage(
    map_file: str,
    ble_mode: str = "mode1_no_ble",
    sram_map_path: Optional[str] = None
) -> Dict:
    """
    Validate memory usage against BLE mode constraints.

    Args:
        map_file: Path to .map file
        ble_mode: BLE mode string (e.g., "mode4_ble_lite")
        sram_map_path: Optional path to sram_map.yaml

    Returns:
        Validation result with errors/warnings
    """
    import yaml

    report = parse_map_file(map_file)
    result = {
        "is_valid": True,
        "errors": [],
        "warnings": [],
        "usage_report": report.to_dict()
    }

    # Load SRAM constraints
    if sram_map_path and Path(sram_map_path).exists():
        with open(sram_map_path, 'r', encoding='utf-8') as f:
            sram_config = yaml.safe_load(f)

        # Get user space limit for BLE mode
        modes = sram_config.get("allocation_modes", {})
        mode_config = modes.get(ble_mode, {})
        user_space = mode_config.get("user_space_total", {})

        if user_space:
            user_space_bytes = user_space.get("bytes", 0)
            user_space_kb = user_space.get("kb", 0)

            static_memory = report.data_bytes + report.bss_bytes + report.vector_bytes

            # ERROR: Static memory exceeds user space
            if static_memory > user_space_bytes:
                result["is_valid"] = False
                result["errors"].append({
                    "type": "static_memory_overflow",
                    "message": f"Static memory ({static_memory} bytes) exceeds user space limit ({user_space_bytes} bytes) for {ble_mode}",
                    "static_memory_bytes": static_memory,
                    "user_space_bytes": user_space_bytes,
                    "ble_mode": ble_mode
                })

            # ERROR: Static memory approaches limit (90% threshold)
            elif static_memory > user_space_bytes * 0.9:
                result["warnings"].append({
                    "type": "static_memory_warning",
                    "message": f"Static memory ({static_memory} bytes) approaches user space limit ({user_space_bytes} bytes, 90% threshold)",
                    "static_memory_bytes": static_memory,
                    "user_space_bytes": user_space_bytes,
                    "usage_percent": round(static_memory / user_space_bytes * 100, 2)
                })

    return result


# ============================================================================
# Main for Testing
# ============================================================================

if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: python map_file_parser.py <map_file>")
        sys.exit(1)

    map_file = sys.argv[1]
    parser = MapFileParser()

    try:
        report = parser.parse_file(map_file)

        print(f"\n{'='*70}")
        print(f"Memory Usage Report: {map_file}")
        print(f"{'='*70}")

        print(f"\nFile Type: {report.file_type.value}")

        print(f"\nMemory Regions:")
        for region in report.regions:
            print(f"  {region.name}: {region.length_kb:.2f} KB @ 0x{region.origin:08X}")

        print(f"\nKey Sections:")
        for name, section in report.sections.items():
            print(f"  {name}: {section.size} bytes ({section.size_kb:.2f} KB)")

        print(f"\nSRAM Summary:")
        print(f"  Total: {report.sram_total_bytes} bytes ({report.sram_total_bytes/1024:.2f} KB)")
        print(f"  Used:  {report.sram_used_bytes} bytes ({report.sram_used_bytes/1024:.2f} KB)")
        print(f"  Free:  {report.sram_free_bytes} bytes ({report.sram_free_bytes/1024:.2f} KB)")
        if report.sram_total_bytes > 0:
            print(f"  Usage: {report.sram_used_bytes / report.sram_total_bytes * 100:.2f}%")

        print(f"\nStatic Memory Breakdown:")
        print(f"  .data:   {report.data_bytes} bytes ({report.data_bytes/1024:.2f} KB)")
        print(f"  .bss:    {report.bss_bytes} bytes ({report.bss_bytes/1024:.2f} KB)")
        print(f"  .vector: {report.vector_bytes} bytes")
        print(f"  Total:   {report.data_bytes + report.bss_bytes + report.vector_bytes} bytes")

        if report.stack_bytes:
            print(f"\nStack: {report.stack_bytes} bytes")
        if report.heap_bytes:
            print(f"Heap:  {report.heap_bytes} bytes")

    except Exception as e:
        print(f"Error parsing map file: {e}")
        sys.exit(1)
