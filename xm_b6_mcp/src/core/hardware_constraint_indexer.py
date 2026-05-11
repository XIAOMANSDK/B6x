"""
Hardware Constraint Indexer
===========================

Build index from Excel parsed data.

Creates mappings:
- Pin -> Peripherals
- Peripheral -> Pins
- API -> Required Pins (based on peripheral)
- Pin conflict detection

Author: B6x MCP Server Team
Version: 0.1.0
"""

import logging
import json
from pathlib import Path
from typing import Dict, List, Set, Optional, Any
from collections import defaultdict
from dataclasses import dataclass, asdict

from .excel_parser import ExcelHardwareParser, PinMuxEntry, MemoryRegion

logger = logging.getLogger(__name__)


@dataclass
class PinConflict:
    """Pin conflict information."""
    pin: str
    requested_function: str
    occupied_by: str
    conflicting_peripheral: str
    suggestion: str = ""


@dataclass
class PeripheralPinInfo:
    """Information about pins for a peripheral."""
    peripheral: str
    available_pins: List[str]
    pin_functions: Dict[str, str]  # pin -> function
    required_pin_count: int = 0    # Minimum pins needed


class HardwareConstraintIndexer:
    """
    Build hardware constraint index from Excel parsed data.

    Provides:
    - Pin to peripheral mapping
    - Peripheral to pin mapping
    - Pin conflict detection
    - Peripheral pin requirements
    """

    def __init__(self, io_map_path: Optional[str] = None, flash_map_path: Optional[str] = None):
        """
        Initialize the hardware constraint indexer.

        Args:
            io_map_path: Path to IO_MAP.xlsx file
            flash_map_path: Path to Flash-Map.xlsx file
        """
        self.parser = ExcelHardwareParser()

        # Parse Excel files if paths provided
        self.pin_mux_entries: List[PinMuxEntry] = []
        self.memory_regions: List[MemoryRegion] = []

        if io_map_path:
            self.pin_mux_entries = self.parser.parse_io_map(io_map_path)

        if flash_map_path:
            self.memory_regions = self.parser.parse_flash_map(flash_map_path)

        # Build mappings
        self._pin_to_peripheral_map: Dict[str, Set[str]] = defaultdict(set)
        self._peripheral_to_pin_map: Dict[str, Set[str]] = defaultdict(set)
        self._pin_to_functions_map: Dict[str, List[PinMuxEntry]] = defaultdict(list)

        self._build_mappings()

    def _build_mappings(self):
        """Build internal mappings from pin mux entries."""
        for entry in self.pin_mux_entries:
            # Pin -> peripherals
            self._pin_to_peripheral_map[entry.pin_number].add(entry.peripheral)

            # Peripheral -> pins
            self._peripheral_to_pin_map[entry.peripheral].add(entry.pin_number)

            # Pin -> all functions
            self._pin_to_functions_map[entry.pin_number].append(entry)

    def build_pin_to_peripheral_map(self) -> Dict[str, List[str]]:
        """
        Build pin to peripheral mapping.

        Returns:
            Dict mapping pin numbers to list of peripherals
        """
        return {
            pin: sorted(peripherals)
            for pin, peripherals in self._pin_to_peripheral_map.items()
        }

    def build_peripheral_to_pin_map(self) -> Dict[str, List[str]]:
        """
        Build peripheral to pin mapping.

        Returns:
            Dict mapping peripheral names to list of pins
        """
        return {
            peripheral: sorted(pins)
            for peripheral, pins in self._peripheral_to_pin_map.items()
        }

    def check_pin_conflict(
        self,
        pin_config: Dict[str, str]
    ) -> List[PinConflict]:
        """
        Check for pin conflicts in a pin configuration.

        Args:
            pin_config: Dictionary mapping pin numbers to requested functions
                        e.g., {"P0_0": "UART0_TX", "P1_5": "SPI0_SCK"}

        Returns:
            List of PinConflict objects (empty if no conflicts)
        """
        conflicts = []

        # Build a map of which pins are used by which peripherals
        pin_usage: Dict[str, Set[str]] = defaultdict(set)

        for pin, requested_function in pin_config.items():
            # Find all possible functions for this pin
            if pin not in self._pin_to_functions_map:
                # Pin not found in database
                conflicts.append(PinConflict(
                    pin=pin,
                    requested_function=requested_function,
                    occupied_by="UNKNOWN",
                    conflicting_peripheral="UNKNOWN",
                    suggestion=f"Pin {pin} not found in IO map database"
                ))
                continue

            # Check if requested function exists for this pin
            available_functions = [e.alternate_function for e in self._pin_to_functions_map[pin]]
            if requested_function not in available_functions:
                conflicts.append(PinConflict(
                    pin=pin,
                    requested_function=requested_function,
                    occupied_by="N/A",
                    conflicting_peripheral="N/A",
                    suggestion=f"Pin {pin} does not support {requested_function}. Available: {', '.join(available_functions[:5])}"
                ))
                continue

            # Extract peripheral from function name
            peripheral = self._extract_peripheral_from_function(requested_function)
            pin_usage[pin].add(peripheral)

        # Check for peripheral conflicts (same peripheral on same pin)
        for pin, peripherals in pin_usage.items():
            if len(peripherals) > 1:
                # Multiple peripherals trying to use the same pin
                for peripheral in peripherals:
                    conflicts.append(PinConflict(
                        pin=pin,
                        requested_function=f"{peripheral} function",
                        occupied_by=', '.join(peripherals - {peripheral}),
                        conflicting_peripheral=', '.join(peripherals),
                        suggestion=f"Pin {pin} cannot be shared by multiple peripherals"
                    ))

        return conflicts

    def _extract_peripheral_from_function(self, function: str) -> str:
        """Extract peripheral name from alternate function."""
        if not function:
            return "UNKNOWN"

        # Same logic as in ExcelHardwareParser
        for peripheral in ['UART', 'SPI', 'I2C', 'I2S', 'ADC', 'DAC', 'PWM', 'TIMER',
                          'GPIO', 'USB', 'CAN', 'SDMMC', 'QUADSPI', 'ETH', 'BLE']:
            if function.startswith(peripheral):
                idx = len(peripheral)
                while idx < len(function) and function[idx].isdigit():
                    idx += 1
                return function[:idx]

        return "UNKNOWN"

    def get_peripheral_pins(self, peripheral: str) -> PeripheralPinInfo:
        """
        Get available pins for a peripheral.

        Args:
            peripheral: Peripheral name (e.g., "UART0", "SPI0")

        Returns:
            PeripheralPinInfo with pin information
        """
        # Normalize peripheral name
        peripheral = peripheral.upper()

        # Get all pins for this peripheral
        pins = sorted(self._peripheral_to_pin_map.get(peripheral, []))

        # Build pin functions map
        pin_functions = {}
        for pin in pins:
            for entry in self._pin_to_functions_map.get(pin, []):
                if entry.peripheral.upper() == peripheral:
                    pin_functions[pin] = entry.alternate_function
                    break

        # Estimate minimum pins needed based on peripheral type
        required_pin_count = self._estimate_required_pins(peripheral)

        return PeripheralPinInfo(
            peripheral=peripheral,
            available_pins=pins,
            pin_functions=pin_functions,
            required_pin_count=required_pin_count
        )

    def _estimate_required_pins(self, peripheral: str) -> int:
        """Estimate minimum pins required for a peripheral."""
        peripheral_upper = peripheral.upper()

        if 'UART' in peripheral_upper:
            return 2  # TX + RX
        elif 'SPI' in peripheral_upper or 'I2C' in peripheral_upper:
            return 2  # SCK + MISO / SDA + SCL
        elif 'I2S' in peripheral_upper:
            return 3  # SCK + WS + SD
        elif 'ADC' in peripheral_upper:
            return 1  # At least 1 input
        elif 'PWM' in peripheral_upper or 'TIMER' in peripheral_upper:
            return 1  # At least 1 output
        else:
            return 1  # Default

    def suggest_alternative_pins(
        self,
        peripheral: str,
        function: str,
        exclude_pins: Optional[Set[str]] = None
    ) -> List[str]:
        """
        Suggest alternative pins for a peripheral function.

        Args:
            peripheral: Peripheral name
            function: Alternate function name (e.g., "UART0_TX")
            exclude_pins: Pins to exclude from suggestions

        Returns:
            List of suggested pin numbers
        """
        exclude_pins = exclude_pins or set()
        suggestions = []

        for pin, entries in self._pin_to_functions_map.items():
            if pin in exclude_pins:
                continue

            for entry in entries:
                if entry.peripheral.upper() == peripheral.upper():
                    if entry.alternate_function == function:
                        suggestions.append(pin)
                        break

        return sorted(suggestions)

    def get_memory_regions(self) -> List[MemoryRegion]:
        """Get all memory regions."""
        return self.memory_regions

    def get_xip_regions(self) -> List[MemoryRegion]:
        """Get memory regions that support XIP."""
        return [r for r in self.memory_regions if r.is_xip]

    def export_to_json(self, output_path: str):
        """
        Export hardware constraints to JSON file.

        Args:
            output_path: Path to output JSON file
        """
        data = {
            "version": "1.0.0",
            "pin_mux": [asdict(e) for e in self.pin_mux_entries],
            "memory_regions": [asdict(r) for r in self.memory_regions],
            "pin_to_peripheral": self.build_pin_to_peripheral_map(),
            "peripheral_to_pin": self.build_peripheral_to_pin_map()
        }

        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)

        logger.info(f"Hardware constraints exported to: {output_path}")

    def load_from_json(self, json_path: str):
        """
        Load hardware constraints from JSON file.

        Args:
            json_path: Path to JSON file
        """
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Restore pin mux entries
        self.pin_mux_entries = [
            PinMuxEntry(**e) for e in data.get("pin_mux", [])
        ]

        # Restore memory regions
        self.memory_regions = [
            MemoryRegion(**r) for r in data.get("memory_regions", [])
        ]

        # Rebuild mappings
        self._build_mappings()

        logger.info(f"Hardware constraints loaded from: {json_path}")


def check_hardware_constraints(
    io_map_path: str,
    pin_config: Dict[str, str]
) -> List[PinConflict]:
    """
    Convenience function to check pin conflicts.

    Args:
        io_map_path: Path to IO_MAP.xlsx
        pin_config: Pin configuration dictionary

    Returns:
        List of conflicts
    """
    indexer = HardwareConstraintIndexer(io_map_path=io_map_path)
    return indexer.check_pin_conflict(pin_config)


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python hardware_constraint_indexer.py <io_map.xlsx> [flash_map.xlsx]")
        sys.exit(1)

    io_map_path = sys.argv[1]
    flash_map_path = sys.argv[2] if len(sys.argv) > 2 else None

    indexer = HardwareConstraintIndexer(io_map_path, flash_map_path)

    # Print statistics
    print("\n" + "=" * 70)
    print("Hardware Constraint Indexer")
    print("=" * 70)

    print(f"\nPin Mux Entries: {len(indexer.pin_mux_entries)}")
    print(f"Memory Regions: {len(indexer.memory_regions)}")

    # Sample peripheral check
    for peripheral in ['UART0', 'SPI0', 'I2C0']:
        info = indexer.get_peripheral_pins(peripheral)
        if info.available_pins:
            print(f"\n{peripheral}:")
            print(f"  Available pins: {', '.join(info.available_pins[:10])}")
            print(f"  Minimum pins required: {info.required_pin_count}")

    # Sample conflict check
    test_config = {
        "P0_0": "UART0_TX",
        "P0_1": "UART0_RX",
        "P0_0": "SPI0_SCK"  # Conflict: P0_0 used twice
    }

    conflicts = indexer.check_pin_conflict(test_config)
    if conflicts:
        print(f"\nFound {len(conflicts)} conflicts:")
        for conflict in conflicts:
            print(f"  {conflict.pin}: {conflict.suggestion}")

    # Export to JSON
    output_path = Path(__file__).parent.parent.parent / "data" / "hardware_constraints.json"
    indexer.export_to_json(str(output_path))
    print(f"\nExported to: {output_path}")
