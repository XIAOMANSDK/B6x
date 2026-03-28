"""
Interrupt Vector Parser - Parse Interrupt Vector Tables
======================================================

Extracts:
- Interrupt number to handler mappings
- System exception vectors
- Peripheral interrupt definitions
- Interrupt priority configuration info

Supports:
- Assembly startup files (.s, .S)
- C startup files (.c)
- CMSIS header files (IRQn definitions)

AI Use Cases:
- Interrupt conflict detection
- ISR configuration code generation
- Interrupt priority assignment
- Vector table analysis
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
class InterruptVector:
    """A single interrupt vector entry."""
    irq_number: int                 # Interrupt number (-14 to N for peripherals)
    name: str                       # Vector name (e.g., "UART1_IRQHandler")
    handler_name: str               # Handler function name
    description: str = ""           # Description
    peripheral: str = ""            # Associated peripheral (e.g., "UART1")
    is_exception: bool = False      # True for Cortex-M exceptions
    priority_range: Tuple[int, int] = (0, 7)  # NVIC priority range
    default_handler: bool = False   # Uses default handler

    def __str__(self) -> str:
        exc = "Exception" if self.is_exception else "IRQ"
        return f"[{self.irq_number:3d}] {exc:12s} {self.name}"

    def to_dict(self) -> Dict:
        return {
            "irq_number": self.irq_number,
            "name": self.name,
            "handler_name": self.handler_name,
            "description": self.description,
            "peripheral": self.peripheral,
            "is_exception": self.is_exception,
            "priority_range": list(self.priority_range),
            "default_handler": self.default_handler
        }


@dataclass
class InterruptTable:
    """Complete interrupt vector table."""
    source_file: str
    vectors: List[InterruptVector] = field(default_factory=list)
    cortex_exceptions: List[InterruptVector] = field(default_factory=list)
    peripheral_irqs: List[InterruptVector] = field(default_factory=list)
    irqn_enum_values: Dict[str, int] = field(default_factory=dict)

    def __post_init__(self):
        """Separate exceptions from peripheral IRQs."""
        for vec in self.vectors:
            if vec.is_exception:
                self.cortex_exceptions.append(vec)
            else:
                self.peripheral_irqs.append(vec)

    def get_vector_by_name(self, name: str) -> Optional[InterruptVector]:
        """Find vector by name."""
        for vec in self.vectors:
            if vec.name == name or vec.handler_name == name:
                return vec
        return None

    def get_vector_by_irq(self, irq_number: int) -> Optional[InterruptVector]:
        """Find vector by IRQ number."""
        for vec in self.vectors:
            if vec.irq_number == irq_number:
                return vec
        return None

    def get_vectors_for_peripheral(self, peripheral: str) -> List[InterruptVector]:
        """Get all vectors for a specific peripheral."""
        return [v for v in self.peripheral_irqs if v.peripheral == peripheral]

    def to_dict(self) -> Dict:
        """Convert to dictionary."""
        return {
            "source_file": self.source_file,
            "summary": {
                "total_vectors": len(self.vectors),
                "cortex_exceptions": len(self.cortex_exceptions),
                "peripheral_irqs": len(self.peripheral_irqs),
                "irqn_enum_values": len(self.irqn_enum_values)
            },
            "cortex_exceptions": [v.to_dict() for v in self.cortex_exceptions],
            "peripheral_irqs": [v.to_dict() for v in self.peripheral_irqs],
            "irqn_enum": self.irqn_enum_values
        }


# ============================================================================
# Assembly Startup File Parser
# ============================================================================

class AssemblyStartupParser:
    """Parse ARM assembly startup files (.s, .S).

    Format:
    __Vectors   DCD     __initial_sp       ; 0,  Stack top
                DCD     Reset_Handler      ; 1,  Reset Handler
                DCD     NMI_Handler        ; 2,  NMI Handler
                ...
                ; External interrupts
                DCD     EXTI_IRQHandler     ; 0,  EXTI
                DCD     IWDT_IRQHandler     ; 1,  IWDT
    """

    def __init__(self):
        self.vectors: List[InterruptVector] = []
        self.irqn_values: Dict[str, int] = {}

    # Cortex-M exception handlers (standard names)
    CORTEX_EXCEPTIONS = {
        -14: ("NMI_Handler", "Non-Maskable Interrupt"),
        -13: ("HardFault_Handler", "Hard Fault"),
        -12: ("MemManage_Handler", "Memory Management"),  # M0+ doesn't have
        -11: ("BusFault_Handler", "Bus Fault"),
        -10: ("UsageFault_Handler", "Usage Fault"),
        -5: ("SVCall_Handler", "SV Call"),
        -4: ("DebugMon_Handler", "Debug Monitor"),
        -2: ("PendSV_Handler", "Pend SV"),
        -1: ("SysTick_Handler", "System Tick"),
        0: ("Reset_Handler", "Reset")
    }

    def parse_file(self, file_path: str) -> InterruptTable:
        """Parse assembly startup file."""
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        table = InterruptTable(source_file=file_path)

        # Find vector table section
        vector_section = self._extract_vector_section(content)
        if not vector_section:
            return table

        # Parse vectors
        self.vectors = self._parse_vectors(vector_section)

        # Find default handler section
        default_handlers = self._extract_default_handlers(content)
        self._mark_default_handlers(default_handlers)

        table.vectors = self.vectors
        return table

    def _extract_vector_section(self, content: str) -> str:
        """Extract the vector table section."""
        # Look for __Vectors label
        match = re.search(
            r'__Vectors\s+(?:DCD|dcd|\.long|\.word)\s+([^\n]+)'
            r'(?:\s+(?:DCD|dcd|\.long|\.word)\s+([^\n]+))*',
            content,
            re.DOTALL | re.MULTILINE
        )

        if match:
            # Extract until __Vectors_End
            end_match = re.search(r'__Vectors_End', content[match.start():])
            if end_match:
                return content[match.start():match.start() + end_match.end()]

        return ""

    def _parse_vectors(self, vector_section: str) -> List[InterruptVector]:
        """Parse vector entries from section."""
        vectors = []

        # Pattern to match DCD lines
        # Matches: DCD     handler_name      ; comment
        dcd_pattern = re.compile(
            r'(?:DCD|dcd|\.long|\.word)\s+([^;\s]+)(?:\s*;\s*(\d+),?\s*([^;\n]*))?',
            re.MULTILINE
        )

        lines = vector_section.split('\n')
        position = 0  # Position in vector table

        # First entry is stack pointer, skip it
        is_first = True

        for line in lines:
            match = dcd_pattern.search(line)
            if match:
                handler = match.group(1).strip()

                # Skip stack pointer and zero entries
                if is_first or handler == '0':
                    is_first = False
                    if handler != '0':
                        position += 1
                    continue

                is_first = False

                # Get comment info
                comment = match.group(3) if match.group(3) else ""
                irq_num_str = match.group(2) if match.group(2) else ""

                # Determine if this is a Cortex exception or peripheral IRQ
                vector = self._create_vector(handler, position, comment)
                if vector:
                    vectors.append(vector)

                position += 1

        return vectors

    def _create_vector(self, handler: str, position: int, comment: str) -> Optional[InterruptVector]:
        """Create a vector from handler name and position."""
        # Map position to Cortex exception for first entries
        cortex_map = {
            1: (-13, "HardFault"),   # Position 1 = IRQ -13
            2: (-14, "NMI"),
            11: (-5, "SVCall"),
            14: (-2, "PendSV"),
            15: (-1, "SysTick")
        }

        # Check if it's a Cortex exception handler
        for pos, (irq_num, name) in cortex_map.items():
            if position == pos or handler.replace("_Handler", "") == name:
                return InterruptVector(
                    irq_number=irq_num,
                    name=name,
                    handler_name=handler,
                    description=f"Cortex-M0+ {name} exception",
                    is_exception=True
                )

        # Check for Reset handler
        if position == 1 or "Reset" in handler:
            return InterruptVector(
                irq_number=0,
                name="Reset",
                handler_name=handler,
                description="Reset handler",
                is_exception=True
            )

        # Peripheral IRQ handler
        # Extract peripheral name from handler (e.g., UART1_IRQHandler -> UART1)
        peripheral = ""
        if "_IRQHandler" in handler:
            peripheral = handler.replace("_IRQHandler", "")
        elif "_IRQ" in handler:
            peripheral = handler.replace("_IRQ", "")
        elif "_Handler" in handler:
            peripheral = handler.replace("_Handler", "")

        # For startup file, IRQ number is position - 16 (first 16 are exceptions)
        irq_num = position - 16

        return InterruptVector(
            irq_number=irq_num,
            name=handler.replace("_Handler", ""),
            handler_name=handler,
            description=comment.strip(),
            peripheral=peripheral,
            is_exception=False
        )

    def _extract_default_handlers(self, content: str) -> List[str]:
        """Extract list of default (weak) handler names."""
        default_handlers = []

        # Look for EXPORT statements with [WEAK]
        weak_pattern = re.compile(r'EXPORT\s+(\w+)\s+\[WEAK\]', re.IGNORECASE)
        for match in weak_pattern.finditer(content):
            handler = match.group(1)
            # Skip Reset and known Cortex exceptions
            if "Handler" in handler or "IRQ" in handler:
                default_handlers.append(handler)

        return default_handlers

    def _mark_default_handlers(self, default_handlers: List[str]) -> None:
        """Mark vectors that use default handlers."""
        for vec in self.vectors:
            if vec.handler_name in default_handlers:
                vec.default_handler = True


# ============================================================================
# CMSIS Header Parser
# ============================================================================

class CMSISHeaderParser:
    """Parse CMSIS header files for IRQn definitions.

    Format:
    typedef enum IRQn
    {
        NMI_IRQn              = -14,
        HardFault_IRQn        = -13,
        ...
        EXTI_IRQn             = 0,
        UART1_IRQn            = 12,
        ...
    } IRQn_Type;
    """

    def __init__(self):
        self.irqn_values: Dict[str, int] = {}

    def parse_file(self, file_path: str) -> Dict[str, int]:
        """Parse CMSIS header for IRQn enum."""
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Find IRQn enum
        enum_match = re.search(
            r'typedef\s+enum\s+IRQn\s*\{([^}]+)\}',
            content,
            re.DOTALL
        )

        if not enum_match:
            return {}

        enum_content = enum_match.group(1)

        # Parse enum values
        # Pattern: NAME = value,
        enum_pattern = re.compile(
            r'(\w+(?:_n|\w+))\s*=\s*([^,\n]+)',
            re.MULTILINE
        )

        for match in enum_pattern.finditer(enum_content):
            name = match.group(1)
            value_str = match.group(2).strip()

            try:
                # Evaluate value (could be hex or decimal)
                value = eval(value_str, {"__builtins__": {}})
                self.irqn_values[name] = value
            except:
                pass

        return self.irqn_values


# ============================================================================
# Combined Interrupt Parser
# ============================================================================

class InterruptVectorParser:
    """Main parser that combines multiple sources."""

    def __init__(self):
        self.assembly_parser = AssemblyStartupParser()
        self.cmsis_parser = CMSISHeaderParser()

    def parse_startup_file(self, file_path: str) -> InterruptTable:
        """Parse assembly startup file."""
        return self.assembly_parser.parse_file(file_path)

    def parse_cmsis_header(self, header_path: str) -> Dict[str, int]:
        """Parse CMSIS header for IRQn definitions."""
        return self.cmsis_parser.parse_file(header_path)

    def parse_combined(
        self,
        startup_file: str,
        cmsis_header: Optional[str] = None
    ) -> InterruptTable:
        """Parse combining startup file and optional CMSIS header."""
        table = self.parse_startup_file(startup_file)

        if cmsis_header:
            table.irqn_enum_values = self.parse_cmsis_header(cmsis_header)

        return table

    def export_to_json(self, table: InterruptTable, output_path: str, indent: int = 2) -> None:
        """Export table to JSON."""
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(table.to_dict(), f, indent=indent, ensure_ascii=False)

    def build_peripheral_index(self, table: InterruptTable) -> Dict[str, List[InterruptVector]]:
        """Build index of peripherals to their IRQs."""
        index = {}

        for vec in table.peripheral_irqs:
            if vec.peripheral:
                if vec.peripheral not in index:
                    index[vec.peripheral] = []
                index[vec.peripheral].append(vec)

        return index

    def get_interrupt_conflicts(self, table: InterruptTable) -> List[Dict]:
        """Check for potential interrupt configuration issues."""
        conflicts = []

        # Check for duplicate IRQ numbers
        irq_numbers = {}
        for vec in table.vectors:
            if vec.irq_number in irq_numbers:
                conflicts.append({
                    "type": "duplicate_irq",
                    "irq_number": vec.irq_number,
                    "vectors": [irq_numbers[vec.irq_number].name, vec.name]
                })
            irq_numbers[vec.irq_number] = vec

        # Check for default handlers on peripheral IRQs
        for vec in table.peripheral_irqs:
            if vec.default_handler:
                conflicts.append({
                    "type": "default_handler",
                    "irq": vec.irq_number,
                    "name": vec.name,
                    "message": f"{vec.name} uses default handler (may need implementation)"
                })

        return conflicts


# ============================================================================
# Utility Functions
# ============================================================================

def parse_interrupt_vector(file_path: str, cmsis_header: Optional[str] = None) -> InterruptTable:
    """Convenience function to parse interrupt vectors."""
    parser = InterruptVectorParser()

    if cmsis_header:
        return parser.parse_combined(file_path, cmsis_header)
    else:
        return parser.parse_startup_file(file_path)


def find_startup_files(sdk_path: str) -> List[str]:
    """Find all startup files in SDK."""
    from glob import glob

    patterns = [
        f"{sdk_path}/core/**/startup*.*",
        f"{sdk_path}/core/**/startup*.s",
        f"{sdk_path}/**/startup*.s",
    ]

    files = []
    for pattern in patterns:
        files.extend(glob(pattern, recursive=True))

    return list(set(files))


def find_cmsis_headers(sdk_path: str) -> List[str]:
    """Find CMSIS headers with IRQn definitions."""
    from glob import glob

    # Look in core directory
    patterns = [
        f"{sdk_path}/core/*.h",
        f"{sdk_path}/core/cmsis/*.h",
        f"{sdk_path}/drivers/api/*.h",  # Sometimes IRQn is defined in driver headers
    ]

    files = []
    for pattern in patterns:
        for file_path in glob(pattern):
            # Check if file contains IRQn
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    if 'IRQn' in f.read():
                        files.append(file_path)
            except:
                pass

    return files


# ============================================================================
# Main for Testing
# ============================================================================

if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        startup_file = sys.argv[1]
        cmsis_file = sys.argv[2] if len(sys.argv) > 2 else None
    else:
        # Default to SDK paths
        startup_file = "core/mdk/startup.s"
        cmsis_file = "core/b6x.h"

    parser = InterruptVectorParser()

    # Parse
    if cmsis_file:
        table = parser.parse_combined(startup_file, cmsis_file)
    else:
        table = parser.parse_startup_file(startup_file)

    # Print results
    print(f"\n{'='*70}")
    print(f"Interrupt Vector Table: {startup_file}")
    print(f"{'='*70}")

    print(f"\nCortex-M0+ Exceptions ({len(table.cortex_exceptions)}):")
    for vec in table.cortex_exceptions:
        print(f"  {vec}")

    print(f"\nPeripheral IRQs ({len(table.peripheral_irqs)}):")
    for vec in table.peripheral_irqs:
        default = " [DEFAULT]" if vec.default_handler else ""
        print(f"  {vec}{default}")

    # IRQn enum values
    if table.irqn_enum_values:
        print(f"\nIRQn Enum Values ({len(table.irqn_enum_values)}):")
        for name, value in sorted(table.irqn_enum_values.items(), key=lambda x: x[1]):
            print(f"  {name:20s} = {value:3d}")

    # Peripheral index
    index = parser.build_peripheral_index(table)
    print(f"\nPeripheral Index ({len(index)} peripherals):")
    for peri, vectors in sorted(index.items()):
        irq_nums = [str(v.irq_number) for v in vectors]
        print(f"  {peri:15s}: IRQ {', '.join(irq_nums)}")

    # Check for conflicts
    conflicts = parser.get_interrupt_conflicts(table)
    if conflicts:
        print(f"\nPotential Issues ({len(conflicts)}):")
        for conf in conflicts:
            print(f"  [{conf['type']}] {conf}")

    # Export JSON
    parser.export_to_json(table, "interrupt_vectors.json")
    print(f"\nExported to: interrupt_vectors.json")
