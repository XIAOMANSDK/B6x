"""
Configuration Loader
====================

Centralized loader for YAML configuration files used across the MCP server.

This module provides:
1. Dependency overrides loading (dependency_overrides.yaml)
2. API-register mapping loading (api_register_mapping.yaml)
3. Register reverse mapping (register -> APIs)
4. Conflict detection (APIs accessing same register)
5. Caching for performance
6. Validation and error handling

Author: B6x MCP Server Team
Version: 2.0.0 (Added register mapping support)
"""

import yaml
import logging
from pathlib import Path
from typing import Dict, List, Any, Optional
from dataclasses import dataclass

logger = logging.getLogger(__name__)


@dataclass
class DependencyOverride:
    """
    Dependency override entry from dependency_overrides.yaml.

    Attributes:
        api: API function name
        call_sequence: List of function calls in sequence
        notes: List of special notes/warnings
        pre_requisites: List of prerequisite APIs
        requires_clock: Whether peripheral clock is required
        requires_gpio: Whether GPIO configuration is required
        requires_dma: Whether DMA configuration is required
        requires_interrupt: Whether interrupt configuration is required
        see_also: List of related APIs
        parameters: List of parameter definitions
        source_file: Source file name
        source_line: Line number in source
    """
    api: str
    call_sequence: List[str]
    notes: List[str]
    pre_requisites: List[str]
    requires_clock: bool
    requires_gpio: bool
    requires_dma: bool
    requires_interrupt: bool
    see_also: List[str]
    parameters: List[Dict[str, str]]
    source_file: str
    source_line: int


@dataclass
class APIRegisterMapping:
    """
    API-register mapping entry from api_register_mapping.yaml.

    Attributes:
        api: API function name
        registers: List of register names
        fields: List of field names (optional)
        confidence: Mapping confidence (0.0-1.0)
        access_type: How registers are accessed
    """
    api: str
    registers: List[str]
    fields: List[str]
    confidence: float
    access_type: str


class ConfigLoader:
    """
    Centralized configuration loader with caching.

    Usage:
        loader = ConfigLoader()

        # Load dependency overrides
        deps = loader.load_dependency_overrides()
        uart_init = deps.get('uart_init')

        # Load API-register mappings
        mappings = loader.load_api_register_mapping()
        uart_mapping = mappings.get('B6x_UART_Init')
    """

    def __init__(self, config_dir: Optional[Path] = None):
        """
        Initialize the configuration loader.

        Args:
            config_dir: Directory containing config files (default: xm_b6_mcp/config)
        """
        if config_dir is None:
            # Default to xm_b6_mcp/config directory
            # File is in: sdk6/xm_b6_mcp/src/core/config_loader.py
            # Config is in: sdk6/xm_b6_mcp/config/
            base_path = Path(__file__).parent.parent.parent
            config_dir = base_path / "config"

        self.config_dir = Path(config_dir)
        self._dependency_cache: Optional[Dict[str, DependencyOverride]] = None
        self._mapping_cache: Optional[Dict[str, APIRegisterMapping]] = None

        logger.info(f"ConfigLoader initialized with config_dir: {self.config_dir}")

    def load_dependency_overrides(self, force_reload: bool = False) -> Dict[str, DependencyOverride]:
        """
        Load dependency overrides from dependency_overrides.yaml.

        Args:
            force_reload: Force reload even if cached

        Returns:
            Dict mapping API names to DependencyOverride objects
        """
        if self._dependency_cache is not None and not force_reload:
            logger.debug(f"Using cached dependency overrides ({len(self._dependency_cache)} entries)")
            return self._dependency_cache

        config_path = self.config_dir / "dependency_overrides.yaml"

        if not config_path.exists():
            logger.warning(f"Dependency overrides file not found: {config_path}")
            return {}

        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)

            dependencies = {}

            for entry in data.get('dependencies', []):
                dep = DependencyOverride(
                    api=entry['api'],
                    call_sequence=entry.get('call_sequence', []),
                    notes=entry.get('notes', []),
                    pre_requisites=entry.get('pre_requisites', []),
                    requires_clock=entry.get('requires_clock', False),
                    requires_gpio=entry.get('requires_gpio', False),
                    requires_dma=entry.get('requires_dma', False),
                    requires_interrupt=entry.get('requires_interrupt', False),
                    see_also=entry.get('see_also', []),
                    parameters=entry.get('parameters', []),
                    source_file=entry.get('source_file', ''),
                    source_line=entry.get('source_line', 0)
                )
                dependencies[dep.api] = dep

            self._dependency_cache = dependencies
            logger.info(f"Loaded {len(dependencies)} dependency overrides from {config_path}")

            return dependencies

        except Exception as e:
            logger.error(f"Failed to load dependency overrides: {e}")
            return {}

    def load_api_register_mapping(self, force_reload: bool = False) -> Dict[str, APIRegisterMapping]:
        """
        Load API-register mappings from api_register_mapping.yaml.

        Args:
            force_reload: Force reload even if cached

        Returns:
            Dict mapping API names to APIRegisterMapping objects
        """
        if self._mapping_cache is not None and not force_reload:
            logger.debug(f"Using cached API mappings ({len(self._mapping_cache)} entries)")
            return self._mapping_cache

        config_path = self.config_dir / "api_register_mapping.yaml"

        if not config_path.exists():
            logger.warning(f"API mapping file not found: {config_path}")
            return {}

        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)

            mappings = {}

            for entry in data.get('mappings', []):
                mapping = APIRegisterMapping(
                    api=entry['api'],
                    registers=entry.get('registers', []),
                    fields=entry.get('fields', []),
                    confidence=entry.get('confidence', 0.5),
                    access_type=entry.get('access_type', 'configure')
                )
                mappings[mapping.api] = mapping

            self._mapping_cache = mappings
            logger.info(f"Loaded {len(mappings)} API mappings from {config_path}")

            return mappings

        except Exception as e:
            logger.error(f"Failed to load API mappings: {e}")
            return {}

    def get_dependency(self, api_name: str) -> Optional[DependencyOverride]:
        """
        Get dependency information for a specific API.

        Args:
            api_name: API function name

        Returns:
            DependencyOverride object or None if not found
        """
        deps = self.load_dependency_overrides()
        return deps.get(api_name)

    def get_register_mapping(self, api_name: str) -> Optional[APIRegisterMapping]:
        """
        Get register mapping for a specific API.

        Args:
            api_name: API function name

        Returns:
            APIRegisterMapping object or None if not found
        """
        mappings = self.load_api_register_mapping()
        return mappings.get(api_name)

    def find_apis_by_requirement(
        self,
        requires_clock: Optional[bool] = None,
        requires_gpio: Optional[bool] = None,
        requires_dma: Optional[bool] = None,
        requires_interrupt: Optional[bool] = None
    ) -> List[str]:
        """
        Find APIs that match specific requirements.

        Args:
            requires_clock: Filter by clock requirement
            requires_gpio: Filter by GPIO requirement
            requires_dma: Filter by DMA requirement
            requires_interrupt: Filter by interrupt requirement

        Returns:
            List of API names matching the criteria
        """
        deps = self.load_dependency_overrides()
        results = []

        for api_name, dep in deps.items():
            if requires_clock is not None and dep.requires_clock != requires_clock:
                continue
            if requires_gpio is not None and dep.requires_gpio != requires_gpio:
                continue
            if requires_dma is not None and dep.requires_dma != requires_dma:
                continue
            if requires_interrupt is not None and dep.requires_interrupt != requires_interrupt:
                continue

            results.append(api_name)

        return results

    def find_apis_with_notes(self, keyword: str) -> List[str]:
        """
        Find APIs that have notes containing a specific keyword.

        Args:
            keyword: Keyword to search for in notes (case-insensitive)

        Returns:
            List of API names with matching notes
        """
        deps = self.load_dependency_overrides()
        keyword_lower = keyword.lower()
        results = []

        for api_name, dep in deps.items():
            for note in dep.notes:
                if keyword_lower in str(note).lower():
                    results.append(api_name)
                    break

        return results

    def get_call_sequence(self, api_name: str) -> List[str]:
        """
        Get the complete call sequence for an API including all prerequisites.

        Args:
            api_name: API function name

        Returns:
            Ordered list of API names to call
        """
        deps = self.load_dependency_overrides()

        if api_name not in deps:
            return [api_name]

        dep = deps[api_name]

        # If call_sequence is defined in YAML, use it directly
        if dep.call_sequence:
            return dep.call_sequence

        # Fallback: build from pre_requisites via topological sort
        sequence = []
        visited = set()

        def build_sequence(api: str) -> None:
            if api in visited:
                return

            visited.add(api)

            # Get prerequisites
            if api in deps:
                dep_entry = deps[api]
                for prereq in dep_entry.pre_requisites:
                    build_sequence(prereq)

            # Add this API
            if api not in sequence:
                sequence.append(api)

        build_sequence(api_name)
        return sequence

    def validate_api_requirements(
        self,
        api_name: str,
        provided_calls: List[str]
    ) -> tuple[bool, List[str]]:
        """
        Validate if all requirements for an API are satisfied.

        Args:
            api_name: API function name
            provided_calls: List of function calls already provided

        Returns:
            Tuple of (is_valid, list_of_missing_requirements)
        """
        dep = self.get_dependency(api_name)

        if not dep:
            return True, []  # No dependency info, assume valid

        missing = []

        # Check prerequisites
        for prereq in dep.pre_requisites:
            if prereq not in provided_calls:
                missing.append(f"Missing prerequisite: {prereq}")

        # Check clock requirement
        if dep.requires_clock:
            has_clock = any('RCC' in call and 'CLK_EN' in call for call in provided_calls)
            if not has_clock:
                missing.append("Missing clock enable call (RCC_APBCLK_EN or RCC_AHBCLK_EN)")

        # Check GPIO requirement
        if dep.requires_gpio:
            has_gpio = any(func in provided_calls for func in ['iom_ctrl', 'csc_output', 'csc_input', 'GPIO_Init'])
            if not has_gpio:
                missing.append("Missing GPIO configuration (iom_ctrl, csc_output, or GPIO_Init)")

        # Check DMA requirement
        if dep.requires_dma:
            has_dma = any('dma' in call.lower() for call in provided_calls)
            if not has_dma:
                missing.append("Missing DMA configuration")

        # Check interrupt requirement
        if dep.requires_interrupt:
            has_irq = any(any(keyword in call for keyword in ['NVIC', 'IER', 'ICR', 'IRQ'])
                          for call in provided_calls)
            if not has_irq:
                missing.append("Missing interrupt configuration")

        return len(missing) == 0, missing

    # ========================================================================
    # API Register Mapping Methods
    # ========================================================================

    def _build_register_reverse_map(self) -> Dict[str, Dict[str, List[str]]]:
        """
        Build reverse mapping: register -> APIs that access it.

        Returns:
            Dict mapping register names to access info
        """
        mappings = self.load_api_register_mapping()

        reverse_map = {}

        for api_name, mapping in mappings.items():
            for reg in mapping.registers:
                if reg not in reverse_map:
                    reverse_map[reg] = {
                        "read": [],
                        "write": [],
                        "configure": []
                    }

                # Add API to appropriate access type list
                if mapping.access_type == 'read':
                    reverse_map[reg]["read"].append(api_name)
                elif mapping.access_type == 'write':
                    reverse_map[reg]["write"].append(api_name)
                else:  # configure
                    reverse_map[reg]["configure"].append(api_name)

        self._register_reverse_map = reverse_map
        logger.info(f"Built register reverse map: {len(reverse_map)} registers")

        return reverse_map

    def get_apis_for_register(self, register_name: str) -> Dict[str, List[str]]:
        """
        Get all APIs that access a specific register.

        Args:
            register_name: Register name (e.g., "RCC_APBCLK_EN_RUN")

        Returns:
            Dict with 'read', 'write', 'configure' keys listing APIs
        """
        if not hasattr(self, '_register_reverse_map'):
            self._build_register_reverse_map()

        reg_info = self._register_reverse_map.get(register_name)
        if reg_info:
            return reg_info

        # Return empty structure if not found
        return {"read": [], "write": [], "configure": []}

    def get_registers_for_api(self, api_name: str) -> List[str]:
        """
        Get all registers accessed by a specific API.

        Args:
            api_name: API function name

        Returns:
            List of register names
        """
        mapping = self.get_register_mapping(api_name)
        if not mapping:
            return []

        return mapping.registers

    def find_register_conflicts(self, api_list: List[str]) -> List[Dict[str, Any]]:
        """
        Find register conflicts between APIs.

        Args:
            api_list: List of API names to check

        Returns:
            List of conflict descriptions
        """
        if not hasattr(self, '_register_reverse_map'):
            self._build_register_reverse_map()

        conflicts = []
        checked_regs = set()

        for api in api_list:
            mapping = self.get_register_mapping(api)
            if not mapping:
                continue

            for reg in mapping.registers:
                if reg in checked_regs:
                    continue
                checked_regs.add(reg)

                reg_info = self.get_apis_for_register(reg)

                # Check for other APIs in our list
                other_apis = []
                other_apis.extend(reg_info.get("read", []))
                other_apis.extend(reg_info.get("write", []))
                other_apis.extend(reg_info.get("configure", []))

                # Filter to APIs in our list (excluding current)
                other_apis = [a for a in other_apis if a in api_list and a != api]

                if other_apis:
                    conflicts.append({
                        "register": reg,
                        "api": api,
                        "conflicts_with": other_apis,
                        "access_type": mapping.access_type,
                        "severity": "error" if mapping.access_type == "write" else "warning"
                    })

        return conflicts


# Global singleton instance
_global_loader: Optional[ConfigLoader] = None


def get_config_loader(config_dir: Optional[Path] = None) -> ConfigLoader:
    """
    Get the global ConfigLoader singleton instance.

    Args:
        config_dir: Optional config directory (only used on first call)

    Returns:
        ConfigLoader instance
    """
    global _global_loader

    if _global_loader is None:
        _global_loader = ConfigLoader(config_dir)

    return _global_loader


# Convenience functions
def get_dependency(api_name: str) -> Optional[DependencyOverride]:
    """Get dependency info for an API using global loader."""
    return get_config_loader().get_dependency(api_name)


def get_call_sequence(api_name: str) -> List[str]:
    """Get call sequence for an API using global loader."""
    return get_config_loader().get_call_sequence(api_name)


def validate_requirements(api_name: str, provided_calls: List[str]) -> tuple[bool, List[str]]:
    """Validate API requirements using global loader."""
    return get_config_loader().validate_api_requirements(api_name, provided_calls)


if __name__ == "__main__":
    # Test the loader
    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    loader = ConfigLoader()

    # Test dependency loading
    print("\n" + "=" * 80)
    print("Testing Dependency Overrides Loading")
    print("=" * 80)

    uart_init = loader.get_dependency('uart_init')
    if uart_init:
        print(f"\nAPI: {uart_init.api}")
        print(f"Requires clock: {uart_init.requires_clock}")
        print(f"Requires GPIO: {uart_init.requires_gpio}")
        print(f"Call sequence ({len(uart_init.call_sequence)} steps):")
        for i, call in enumerate(uart_init.call_sequence[:3], 1):
            print(f"  {i}. {call}")
        if len(uart_init.call_sequence) > 3:
            print(f"  ... and {len(uart_init.call_sequence) - 3} more")
        print(f"\nNotes:")
        for note in uart_init.notes:
            print(f"  - {note}")

    # Test requirement validation
    print("\n" + "=" * 80)
    print("Testing Requirement Validation")
    print("=" * 80)

    # Missing clock
    is_valid, missing = loader.validate_api_requirements('uart_init', ['iom_ctrl'])
    print(f"\nValidation (missing clock): {is_valid}")
    for m in missing:
        print(f"  - {m}")

    # Complete
    is_valid, missing = loader.validate_api_requirements(
        'uart_init',
        ['RCC_APBCLK_EN', 'iom_ctrl', 'csc_output']
    )
    print(f"\nValidation (complete): {is_valid}")
    if missing:
        for m in missing:
            print(f"  - {m}")

    # Test search
    print("\n" + "=" * 80)
    print("Testing Search Functions")
    print("=" * 80)

    clock_apis = loader.find_apis_by_requirement(requires_clock=True)
    print(f"\nAPIs requiring clock: {len(clock_apis)}")

    critical_apis = loader.find_apis_with_notes('CRITICAL')
    print(f"APIs with CRITICAL notes: {critical_apis}")

    delay_apis = loader.find_apis_with_notes('delay')
    print(f"APIs with 'delay' in notes: {delay_apis}")
