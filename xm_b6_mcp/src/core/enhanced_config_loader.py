#!/usr/bin/env python3
"""
Enhanced Config Loader - API Register Mapping Support
====================================================

Extended config loader with full api_register_mapping.yaml integration.

Author: B6x MCP Server Team
Date: 2026-02-28
"""

import yaml
import logging
from pathlib import Path
from typing import Dict, List, Any, Optional, Set
from dataclasses import dataclass

logger = logging.getLogger(__name__)


@dataclass
class RegisterAccess:
    """
    Register access information.

    Attributes:
        register_name: Register name (e.g., "RCC_APBCLK_EN_RUN")
        apis_that_read: APIs that read this register
        apis_that_write: APIs that write this register
        apis_that_configure: APIs that configure this register
        confidence: Mapping confidence
    """
    register_name: str
    apis_that_read: List[str]
    apis_that_write: List[str]
    apis_that_configure: List[str]
    confidence: float


class EnhancedConfigLoader:
    """
    Enhanced configuration loader with register mapping capabilities.

    Extends ConfigLoader to provide:
    1. Reverse mapping: Register -> APIs
    2. Conflict detection: APIs that access the same register
    3. Register info for Layer 2
    4. Conflict validation for Layer 3
    """

    def __init__(self, config_dir: Optional[Path] = None):
        """Initialize the enhanced loader."""
        # Import base loader
        try:
            from .config_loader import ConfigLoader, DependencyOverride, APIRegisterMapping
            self.base_loader = ConfigLoader(config_dir)
        except ImportError as e:
            logger.error(f"Failed to import base ConfigLoader: {e}")
            raise

        self.register_reverse_map: Optional[Dict[str, RegisterAccess]] = None
        self._build_reverse_map()

    def _build_reverse_map(self) -> None:
        """Build reverse mapping: register -> APIs that access it."""
        mappings = self.base_loader.load_api_register_mapping()

        reverse_map = {}

        for api_name, mapping in mappings.items():
            # For each register in the mapping
            for reg in mapping.registers:
                if reg not in reverse_map:
                    reverse_map[reg] = RegisterAccess(
                        register_name=reg,
                        apis_that_read=[],
                        apis_that_write=[],
                        apis_that_configure=[],
                        confidence=1.0
                    )

                # Add API to appropriate list
                reg_access = reverse_map[reg]
                if mapping.access_type == 'read':
                    reg_access.apis_that_read.append(api_name)
                elif mapping.access_type == 'write':
                    reg_access.apis_that_write.append(api_name)
                else:  # configure
                    reg_access.apis_that_configure.append(api_name)

        self.register_reverse_map = reverse_map
        logger.info(f"Built reverse map: {len(reverse_map)} registers")

    def get_register_access(self, register_name: str) -> Optional[RegisterAccess]:
        """
        Get access information for a specific register.

        Args:
            register_name: Register name (e.g., "RCC_APBCLK_EN_RUN")

        Returns:
            RegisterAccess object or None
        """
        if not self.register_reverse_map:
            self._build_reverse_map()

        return self.register_reverse_map.get(register_name)

    def find_register_conflicts(self, api_list: List[str]) -> List[Dict[str, Any]]:
        """
        Find register conflicts between APIs.

        Args:
            api_list: List of API names to check

        Returns:
            List of conflict descriptions
        """
        if not self.register_reverse_map:
            self._build_reverse_map()

        conflicts = []
        checked_regs = set()

        for api in api_list:
            mapping = self.base_loader.get_register_mapping(api)
            if not mapping:
                continue

            for reg in mapping.registers:
                if reg in checked_regs:
                    continue
                checked_regs.add(reg)

                reg_access = self.get_register_access(reg)
                if not reg_access:
                    continue

                # Check if other APIs also access this register
                other_apis = set()
                other_apis.update(reg_access.apis_that_read)
                other_apis.update(reg_access.apis_that_write)
                other_apis.update(reg_access.apis_that_configure)

                # Remove current API
                other_apis.discard(api)

                # Filter to APIs in our list
                other_apis = other_apis.intersection(api_list)

                if other_apis:
                    conflicts.append({
                        "register": reg,
                        "api": api,
                        "conflicts_with": list(other_apis),
                        "access_type": mapping.access_type,
                        "severity": "error" if mapping.access_type == "write" else "warning"
                    })

        return conflicts

    def get_apis_for_register(self, register_name: str) -> List[str]:
        """
        Get all APIs that access a specific register.

        Args:
            register_name: Register name

        Returns:
            List of API names
        """
        reg_access = self.get_register_access(register_name)
        if not reg_access:
            return []

        apis = []
        apis.extend(reg_access.apis_that_read)
        apis.extend(reg_access.apis_that_write)
        apis.extend(reg_access.apis_that_configure)

        return list(set(apis))

    def get_registers_for_api(self, api_name: str) -> List[str]:
        """
        Get all registers accessed by a specific API.

        Args:
            api_name: API name

        Returns:
            List of register names
        """
        mapping = self.base_loader.get_register_mapping(api_name)
        if not mapping:
            return []

        return mapping.registers

    def analyze_register_access_pattern(self, api_list: List[str]) -> Dict[str, Any]:
        """
        Analyze register access patterns for a list of APIs.

        Args:
            api_list: List of API names

        Returns:
            Analysis results including conflicts, shared registers, etc.
        """
        if not self.register_reverse_map:
            self._build_reverse_map()

        all_regs = set()
        read_regs = set()
        write_regs = set()

        # Collect all registers accessed
        for api in api_list:
            regs = self.get_registers_for_api(api)
            all_regs.update(regs)

            mapping = self.base_loader.get_register_mapping(api)
            if mapping:
                if mapping.access_type == 'read':
                    read_regs.update(regs)
                elif mapping.access_type == 'write':
                    write_regs.update(regs)
                else:
                    # configure could be both
                    read_regs.update(regs)
                    write_regs.update(regs)

        # Find conflicts
        conflicts = self.find_register_conflicts(api_list)

        # Find shared registers
        shared_regs = {}
        for reg in all_regs:
            apis = self.get_apis_for_register(reg)
            if len(apis) > 1:
                shared_regs[reg] = apis

        return {
            "total_registers": len(all_regs),
            "read_registers": len(read_regs),
            "write_registers": len(write_regs),
            "shared_registers": len(shared_regs),
            "conflicts": len(conflicts),
            "conflict_details": conflicts,
            "shared_register_details": dict(shared_regs)
        }


# Singleton instance
_enhanced_loader: Optional[EnhancedConfigLoader] = None


def get_enhanced_loader(config_dir: Optional[Path] = None) -> EnhancedConfigLoader:
    """Get the global enhanced loader instance."""
    global _enhanced_loader
    if _enhanced_loader is None:
        _enhanced_loader = EnhancedConfigLoader(config_dir)
    return _enhanced_loader


if __name__ == "__main__":
    # Test the enhanced loader
    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    loader = EnhancedConfigLoader()

    print("\n" + "=" * 80)
    print("Enhanced Config Loader Test")
    print("=" * 80)

    # Test 1: Get register access info
    print("\n[TEST 1] Get Register Access for RCC_APBCLK_EN_RUN")
    reg_access = loader.get_register_access('RCC_APBCLK_EN_RUN')
    if reg_access:
        print(f"  Register: {reg_access.register_name}")
        print(f"  APIs that read ({len(reg_access.apis_that_read)}):")
        for api in reg_access.apis_that_read[:5]:
            print(f"    - {api}")
        if len(reg_access.apis_that_read) > 5:
            print(f"    ... and {len(reg_access.apis_that_read) - 5} more")
        print(f"  APIs that write ({len(reg_access.apis_that_write)}):")
        for api in reg_access.apis_that_write[:5]:
            print(f"    - {api}")
        if len(reg_access.apis_that_write) > 5:
            print(f"    ... and {len(reg_access.apis_that_write) - 5} more")
        print(f"  APIs that configure ({len(reg_access.apis_that_configure)}):")
        for api in reg_access.apis_that_configure[:5]:
            print(f"    - {api}")
        if len(reg_access.apis_that_configure) > 5:
            print(f"    ... and {len(reg_access.apis_that_configure) - 5} more")

    # Test 2: Find conflicts
    print("\n[TEST 2] Find Conflicts Between UART APIs")
    uart_apis = ['uart_init', 'uart_conf', 'uart_hwfc']
    conflicts = loader.find_register_conflicts(uart_apis)
    print(f"  Checking APIs: {uart_apis}")
    print(f"  Found {len(conflicts)} conflicts")
    for conflict in conflicts:
        print(f"\n  Register: {conflict['register']}")
        print(f"  API: {conflict['api']}")
        print(f"  Conflicts with: {conflict['conflicts_with']}")
        print(f"  Access type: {conflict['access_type']}")
        print(f"  Severity: {conflict['severity']}")

    # Test 3: Analyze access pattern
    print("\n[TEST 3] Analyze UART Access Pattern")
    analysis = loader.analyze_register_access_pattern(uart_apis)
    print(f"  Total registers: {analysis['total_registers']}")
    print(f"  Read registers: {analysis['read_registers']}")
    print(f"  Write registers: {analysis['write_registers']}")
    print(f"  Shared registers: {analysis['shared_registers']}")
    print(f"  Conflicts: {analysis['conflicts']}")

    print("\n[SUCCESS] Enhanced loader working!")
