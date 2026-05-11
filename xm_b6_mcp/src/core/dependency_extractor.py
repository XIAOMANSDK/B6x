"""
Dependency Extractor
====================

Extracts API dependency relationships from Doxygen comments and code analysis.

This module:
1. Parses @requires, @note, @see tags from Doxygen comments
2. Infers dependency relationships (clock, GPIO, etc.)
3. Builds dependency trees for call sequence analysis
4. Generates recommended initialization sequences

Author: B6x MCP Server Team
Version: 1.0.0
"""

import logging
import re
import yaml
from pathlib import Path
from typing import List, Dict, Optional, Set
from dataclasses import dataclass, field

logger = logging.getLogger(__name__)


@dataclass
class APIDependency:
    """
    API dependency information.

    Attributes:
        api_name: API function name
        pre_requisites: List of prerequisite API names
        requires_peripheral_clock: Whether peripheral clock is required
        requires_gpio_config: Whether GPIO configuration is required
        requires_interrupt_config: Whether interrupt configuration is required
        requires_dma_config: Whether DMA configuration is required
        notes: List of @note tag contents
        see_also: List of @see tag contents
        call_sequence: Recommended call order
        dependency_source: Source of dependency ('doxygen', 'manual', 'inferred')
    """
    api_name: str
    pre_requisites: List[str] = field(default_factory=list)
    requires_peripheral_clock: bool = False
    requires_gpio_config: bool = False
    requires_interrupt_config: bool = False
    requires_dma_config: bool = False
    notes: List[str] = field(default_factory=list)
    see_also: List[str] = field(default_factory=list)
    call_sequence: List[str] = field(default_factory=list)
    dependency_source: str = 'inferred'


@dataclass
class DependencyTree:
    """
    Dependency tree structure for API relationships.

    Attributes:
        api_name: API function name
        level: Dependency depth (0 = no dependencies)
        dependencies: List of APIs this one depends on
        dependents: List of APIs that depend on this one
    """
    api_name: str
    level: int = 0
    dependencies: List['DependencyTree'] = field(default_factory=list)
    dependents: List['DependencyTree'] = field(default_factory=list)


class DoxygenDependencyExtractor:
    """
    Extracts dependency information from Doxygen comments.

    Parses @requires, @note, @see tags to understand API dependencies.
    """

    # Patterns for Doxygen tags
    REQUIRES_PATTERN = re.compile(r'@requires?\s+(.*?)(?:@|\n|$)', re.IGNORECASE)
    NOTE_PATTERN = re.compile(r'@note\s+(.*?)(?:@|\n|$)', re.IGNORECASE)
    SEE_PATTERN = re.compile(r'@see\s+(.*?)(?:@|\n|$)', re.IGNORECASE)
    PRE_PATTERN = re.compile(r'@pre\s+(.*?)(?:@|\n|$)', re.IGNORECASE)

    # Common prerequisite patterns
    CLOCK_PATTERNS = [
        r'RCC_EnablePeriphClock',
        r'RCC.*Enable.*Clock',
        r'clock.*enable',
        r'enable.*clock'
    ]

    GPIO_PATTERNS = [
        r'GPIO_Init',
        r'GPIO.*Config',
        r'GPIO.*Setup',
        r'pin.*config',
        r'config.*pin'
    ]

    def __init__(self, manual_config: Optional[str] = None):
        """
        Initialize the extractor.

        Args:
            manual_config: Optional path to manual dependency config YAML
        """
        self.manual_config = manual_config
        self.manual_dependencies: Dict[str, APIDependency] = {}

        if manual_config:
            self._load_manual_dependencies()

    def _load_manual_dependencies(self) -> None:
        """Load manual dependency overrides from YAML."""
        try:
            with open(self.manual_config, 'r') as f:
                config = yaml.safe_load(f)

            for entry in config.get('dependencies', []):
                dep = APIDependency(
                    api_name=entry['api'],
                    pre_requisites=entry.get('pre_requisites', []),
                    requires_peripheral_clock=entry.get('requires_clock', False),
                    requires_gpio_config=entry.get('requires_gpio', False),
                    requires_interrupt_config=entry.get('requires_interrupt', False),
                    requires_dma_config=entry.get('requires_dma', False),
                    notes=entry.get('notes', []),
                    see_also=entry.get('see_also', []),
                    call_sequence=entry.get('call_sequence', []),
                    dependency_source='manual'
                )
                self.manual_dependencies[dep.api_name] = dep

            logger.info(f"Loaded {len(self.manual_dependencies)} manual dependencies")

        except FileNotFoundError:
            logger.warning(f"Manual dependency config not found: {self.manual_config}")
        except Exception as e:
            logger.error(f"Failed to load manual dependencies: {e}")

    def extract_dependencies(
        self,
        func,  # FunctionDeclaration from Tree-sitter
    ) -> APIDependency:
        """
        Extract dependency information from a function's Doxygen comments.

        Args:
            func: Function declaration object

        Returns:
            APIDependency object
        """
        # Check for manual override first
        if func.name in self.manual_dependencies:
            return self.manual_dependencies[func.name]

        # Combine all doxygen text
        doxygen_text = func.detailed_description or ""
        doxygen_text += " " + (func.brief_description or "")

        # Extract tags
        requires = self._parse_requires_tag(doxygen_text)
        notes = self._parse_note_tags(doxygen_text)
        see_also = self._parse_see_tags(doxygen_text)

        # Infer dependencies
        requires_clock = self._infer_clock_dependency(func.name, doxygen_text)
        requires_gpio = self._infer_gpio_dependency(func, doxygen_text)
        requires_interrupt = self._infer_interrupt_dependency(func.name, doxygen_text)
        requires_dma = self._infer_dma_dependency(func.name, doxygen_text)

        # Build prerequisite list from @requires and inferences
        pre_requisites = list(set(requires))

        if requires_clock and 'RCC_EnablePeriphClock' not in pre_requisites:
            pre_requisites.insert(0, 'RCC_EnablePeriphClock')

        if requires_gpio and 'GPIO_Init' not in pre_requisites:
            pre_requisites.append('GPIO_Init')

        return APIDependency(
            api_name=func.name,
            pre_requisites=pre_requisites,
            requires_peripheral_clock=requires_clock,
            requires_gpio_config=requires_gpio,
            requires_interrupt_config=requires_interrupt,
            requires_dma_config=requires_dma,
            notes=notes,
            see_also=see_also,
            dependency_source='doxygen' if (requires or notes) else 'inferred'
        )

    def _parse_requires_tag(self, doxygen: str) -> List[str]:
        """
        Parse @requires tags from Doxygen comments.

        Examples:
            @requires RCC_EnablePeriphClock(RCC_PERIPH_UART1) must be called first
            @requires GPIO_Init must be called before using this function

        Returns:
            List of required API names
        """
        requires = []

        # Try @requires/@require tags
        matches = self.REQUIRES_PATTERN.findall(doxygen)
        requires.extend(matches)

        matches = self.PRE_PATTERN.findall(doxygen)
        requires.extend(matches)

        # Extract API names from requires text
        api_names = []
        for req in requires:
            # Look for function-like patterns: Name(...) or Name
            api_match = re.search(r'([A-Z][a-zA-Z0-9_]+)(?:\s*\(|\s+must)', req)
            if api_match:
                api_name = api_match.group(1)
                if api_name not in api_names:
                    api_names.append(api_name)

        return api_names

    def _parse_note_tags(self, doxygen: str) -> List[str]:
        """
        Parse @note tags from Doxygen comments.

        Returns:
            List of note contents
        """
        notes = self.NOTE_PATTERN.findall(doxygen)
        return [note.strip() for note in notes if note.strip()]

    def _parse_see_tags(self, doxygen: str) -> List[str]:
        """
        Parse @see tags from Doxygen comments.

        Returns:
            List of referenced API names
        """
        see_refs = []

        matches = self.SEE_PATTERN.findall(doxygen)
        for see in matches:
            # Extract API names (may have commas, parentheses, etc.)
            apis = re.findall(r'([A-Z][a-zA-Z0-9_]+)', see)
            see_refs.extend(apis)

        return list(set(see_refs))

    def _infer_clock_dependency(self, func_name: str, doxygen: str) -> bool:
        """
        Infer if function requires peripheral clock.

        Rules:
        - *_Init, *_DeInit functions typically need clock
        - If "clock" mentioned in @requires/@note
        - Peripheral initialization functions

        Args:
            func_name: Function name
            doxygen: Doxygen comment text

        Returns:
            True if clock likely required
        """
        # Direct mentions
        clock_keywords = ['clock must be enabled', 'clock enable', 'enable clock']
        if any(kw in doxygen.lower() for kw in clock_keywords):
            return True

        # Function name patterns
        init_patterns = ['_Init', '_DeInit', '_Enable', '_Disable']
        if any(pat in func_name for pat in init_patterns):
            # Exclude RCC functions themselves
            if not func_name.startswith('RCC_'):
                return True

        return False

    def _infer_gpio_dependency(self, func, doxygen: str) -> bool:
        """
        Infer if function requires GPIO configuration.

        Rules:
        - Parameter contains GPIO_TypeDef* or pin number
        - Function mentions "pin", "gpio" in @note/@requires
        - Peripheral initialization functions

        Args:
            func: Function declaration
            doxygen: Doxygen comment text

        Returns:
            True if GPIO config likely required
        """
        # Check parameters
        for param in func.parameters:
            param_lower = param.type.lower() + ' ' + param.name.lower()
            if 'gpio' in param_lower or 'pin' in param_lower:
                return True

        # Check documentation
        gpio_keywords = ['gpio must be', 'pin must be', 'gpio config', 'pin config']
        if any(kw in doxygen.lower() for kw in gpio_keywords):
            return True

        return False

    def _infer_interrupt_dependency(self, func_name: str, doxygen: str) -> bool:
        """Infer if function requires interrupt configuration."""
        # Function name patterns
        irq_patterns = ['_IT', '_IRQ', '_Interrupt', '_NVIC']
        if any(pat in func_name for pat in irq_patterns):
            return True

        # Documentation mentions
        irq_keywords = ['interrupt', 'irq', 'nvic', 'handler']
        if any(kw in doxygen.lower() for kw in irq_keywords):
            return True

        return False

    def _infer_dma_dependency(self, func_name: str, doxygen: str) -> bool:
        """Infer if function requires DMA configuration."""
        # Function name patterns
        dma_patterns = ['_DMA', '_Dma', '_Transfer']
        if any(pat in func_name for pat in dma_patterns):
            return True

        # Documentation mentions
        dma_keywords = ['dma must be', 'dma config', 'dma transfer']
        if any(kw in doxygen.lower() for kw in dma_keywords):
            return True

        return False


class DependencyTreeBuilder:
    """
    Builds dependency trees and computes call sequences.

    Analyzes API dependencies to create:
    - Dependency trees showing relationships
    - Topologically sorted call sequences
    - Dependency levels (how deep the dependency chain is)
    """

    def __init__(self):
        """Initialize the builder."""
        self.dependencies: Dict[str, APIDependency] = {}
        self.tree: Dict[str, DependencyTree] = {}

    def build_tree(
        self,
        dependencies: List[APIDependency]
    ) -> Dict[str, DependencyTree]:
        """
        Build complete dependency tree.

        Args:
            dependencies: List of API dependencies

        Returns:
            Dict mapping API names to DependencyTree objects
        """
        self.dependencies = {dep.api_name: dep for dep in dependencies}

        # Build tree nodes
        for api_name, dep in self.dependencies.items():
            self.tree[api_name] = DependencyTree(api_name=api_name)

        # Link dependencies
        for api_name, dep in self.dependencies.items():
            node = self.tree[api_name]

            for prereq in dep.pre_requisites:
                if prereq in self.tree:
                    prereq_node = self.tree[prereq]
                    node.dependencies.append(prereq_node)
                    prereq_node.dependents.append(node)

        # Calculate levels
        self._calculate_levels()

        return self.tree

    def _calculate_levels(self) -> None:
        """Calculate dependency level for each node."""
        # Use DFS to calculate max depth
        visited = set()

        def get_depth(api_name: str) -> int:
            if api_name in visited:
                return 0

            visited.add(api_name)

            if api_name not in self.tree:
                return 0

            node = self.tree[api_name]
            if not node.dependencies:
                node.level = 0
                return 0

            max_dep_depth = max((get_depth(dep.api_name) for dep in node.dependencies), default=0)
            node.level = max_dep_depth + 1
            return node.level

        for api_name in self.tree:
            if api_name not in visited:
                get_depth(api_name)

    def get_call_sequence(self, api_name: str) -> List[str]:
        """
        Get recommended call sequence for an API.

        Performs topological sort on dependencies.

        Args:
            api_name: API function name

        Returns:
            Ordered list of API names to call
        """
        if api_name not in self.dependencies:
            return [api_name]

        sequence = []
        visited = set()

        def visit(api: str) -> None:
            if api in visited:
                return

            visited.add(api)

            # Visit prerequisites first
            if api in self.dependencies:
                for prereq in self.dependencies[api].pre_requisites:
                    visit(prereq)

            # Then add this API
            if api not in sequence:
                sequence.append(api)

        visit(api_name)
        return sequence

    def get_dependency_level(self, api_name: str) -> int:
        """
        Get dependency level for an API.

        Level 0: No dependencies
        Level 1: Requires 1 prerequisite
        Level N: Requires chain of N prerequisites

        Args:
            api_name: API function name

        Returns:
            Dependency level
        """
        if api_name in self.tree:
            return self.tree[api_name].level
        return 0

    def find_initialization_sequence(
        self,
        peripheral: str
    ) -> List[Dict]:
        """
        Find complete initialization sequence for a peripheral.

        Args:
            peripheral: Peripheral name (e.g., "UART", "SPI")

        Returns:
            List of initialization steps with metadata
        """
        steps = []

        # Common initialization pattern
        common_sequence = [
            {'api': 'RCC_EnablePeriphClock', 'priority': 1, 'reason': 'Enable peripheral clock'},
        ]

        # Add GPIO if applicable
        if peripheral in ['UART', 'SPI', 'I2C']:
            common_sequence.append({
                'api': 'GPIO_Init',
                'priority': 2,
                'reason': f'Configure {peripheral} pins'
            })

        # Add peripheral-specific initialization
        init_api = f'B6x_{peripheral}_Init'
        if init_api in self.dependencies:
            common_sequence.append({
                'api': init_api,
                'priority': 3,
                'reason': f'Initialize {peripheral}'
            })

            # Get full dependency chain
            full_sequence = self.get_call_sequence(init_api)

            # Build detailed steps
            for i, api in enumerate(full_sequence):
                dep = self.dependencies.get(api)
                step = {
                    'step': i + 1,
                    'api': api,
                    'requires_clock': dep.requires_peripheral_clock if dep else False,
                    'requires_gpio': dep.requires_gpio_config if dep else False,
                    'notes': dep.notes if dep else []
                }
                steps.append(step)

        return steps


def create_sample_dependency_config(output_path: str) -> None:
    """
    Create a sample manual dependency configuration file.

    Args:
        output_path: Path where to create the sample file
    """
    sample_config = {
        'dependencies': [
            {
                'api': 'B6x_UART_Init',
                'pre_requisites': [
                    'RCC_EnablePeriphClock',
                    'GPIO_Init'
                ],
                'requires_clock': True,
                'requires_gpio': True,
                'requires_interrupt': False,
                'requires_dma': False,
                'notes': [
                    'Ensure UART clock is enabled before calling this function',
                    'GPIO pins must be configured with correct alternate function'
                ],
                'see_also': ['B6x_UART_DeInit', 'B6x_UART_Send'],
                'call_sequence': [
                    'RCC_EnablePeriphClock(RCC_PERIPH_UART1)',
                    'GPIO_Init(...)',
                    'B6x_UART_Init(...)'
                ]
            },
            {
                'api': 'B6x_SPI_Init',
                'pre_requisites': [
                    'RCC_EnablePeriphClock',
                    'GPIO_Init'
                ],
                'requires_clock': True,
                'requires_gpio': True,
                'requires_interrupt': False,
                'requires_dma': False,
                'notes': [
                    'SPI clock must be enabled',
                    'Configure GPIO pins for SPI alternate function'
                ],
                'see_also': ['B6x_SPI_DeInit'],
                'call_sequence': [
                    'RCC_EnablePeriphClock(RCC_PERIPH_SPI1)',
                    'GPIO_Init(...)',
                    'B6x_SPI_Init(...)'
                ]
            }
        ]
    }

    with open(output_path, 'w') as f:
        yaml.dump(sample_config, f, default_flow_style=False)

    logger.info(f"Created sample dependency config: {output_path}")


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    # Create sample config
    config_dir = Path(__file__).parent.parent.parent / "config"
    config_dir.mkdir(exist_ok=True)

    sample_path = config_dir / "dependency_overrides.yaml"
    create_sample_dependency_config(str(sample_path))

    print(f"Sample configuration created at: {sample_path}")
