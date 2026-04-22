"""
API-Register Mapper
===================

Maps B6x SDK API functions to hardware registers using heuristic rules.

This module establishes bidirectional relationships between:
- API functions (e.g., B6x_UART_Init)
- Hardware registers (e.g., UART_CR, UART_BRR)

Mapping strategies:
1. Peripheral name matching (high confidence)
2. Implementation parsing (very high confidence)
3. Parameter type analysis (medium confidence)
4. Manual override configuration (100% confidence)

Author: B6x MCP Server Team
Version: 1.0.0
"""

import logging
import re
import yaml
from pathlib import Path
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass, field

logger = logging.getLogger(__name__)


@dataclass
class APIRegisterMapping:
    """
    API to register mapping result.

    Attributes:
        api_name: API function name
        register_names: List of register names accessed
        field_names: List of field names accessed (if specific bits)
        confidence: Mapping confidence (0.0 - 1.0)
        mapping_source: Source of mapping ('name_match', 'implementation', 'manual')
        access_type: How registers are accessed ('read', 'write', 'configure')
    """
    api_name: str
    register_names: List[str]
    field_names: List[str] = field(default_factory=list)
    confidence: float = 0.5
    mapping_source: str = 'name_match'
    access_type: str = 'configure'  # 'read', 'write', 'configure'


@dataclass
class RegisterAPIMapping:
    """
    Register to API mapping result.

    Attributes:
        register_name: Register name (e.g., "UART_CR")
        peripheral: Peripheral name (e.g., "UART")
        apis_that_read: APIs that read this register
        apis_that_write: APIs that write this register
        apis_that_configure: APIs that configure this register
    """
    register_name: str
    peripheral: str
    apis_that_read: List[str] = field(default_factory=list)
    apis_that_write: List[str] = field(default_factory=list)
    apis_that_configure: List[str] = field(default_factory=list)


class APIRegisterMapper:
    """
    Maps API functions to hardware registers using heuristic rules.
    """

    # Peripheral name patterns
    PERIPHERAL_PATTERNS = {
        'UART': ['UART', 'Uart', 'uart'],
        'GPIO': ['GPIO', 'Gpio', 'gpio'],
        'SPI': ['SPI', 'Spi', 'spi'],
        'I2C': ['I2C', 'I2c', 'i2c', 'IIC'],
        'ADC': ['ADC', 'Adc', 'adc'],
        'DAC': ['DAC', 'Dac', 'dac'],
        'TIM': ['TIM', 'Tim', 'Timer', 'timer'],
        'PWM': ['PWM', 'Pwm', 'pwm'],
        'RCC': ['RCC', 'Rcc', 'rcc'],
        'DMA': ['DMA', 'Dma', 'dma'],
    }

    # Register suffix patterns
    REGISTER_SUFFIXES = {
        'control': ['CR', 'CTRL', 'Control', 'CON'],
        'status': ['SR', 'STAT', 'Status', 'STS'],
        'data': ['DR', 'DATA', 'Data', 'BUF'],
        'baudrate': ['BRR', 'BR', 'BaudRate'],
        'config': ['CFGR', 'CFG', 'Config'],
        'enable': ['EN', 'Enable', 'ENA'],
        'interrupt': ['IER', 'IE', 'IRQ', 'Int', 'IMR'],
    }

    def __init__(self):
        """Initialize the mapper."""
        self.manual_mappings: List[APIRegisterMapping] = []
        self.register_map: Dict[str, List[str]] = {}  # peripheral -> register names

    def map_apis_to_registers(
        self,
        functions: List,  # List[FunctionDeclaration]
        registers: List,  # List[SVDRegister]
        manual_mapping_file: Optional[str] = None
    ) -> List[APIRegisterMapping]:
        """
        Map API functions to registers using all available strategies.

        Args:
            functions: List of function declarations from Tree-sitter
            registers: List of register definitions from SVD
            manual_mapping_file: Optional YAML file with manual mappings

        Returns:
            List of APIRegisterMapping objects
        """
        logger.info(f"Mapping {len(functions)} APIs to {len(registers)} registers")

        # Build register lookup by peripheral
        self._build_register_map(registers)

        # Load manual overrides if provided
        if manual_mapping_file:
            self.manual_mappings = self.load_manual_overrides(manual_mapping_file)

        all_mappings = []

        for func in functions:
            # Try manual override first
            manual = self._get_manual_mapping(func.name)
            if manual:
                all_mappings.append(manual)
                continue

            # Apply heuristic rules
            mappings = self._apply_heuristic_rules(func, registers)

            if mappings:
                all_mappings.extend(mappings)

        logger.info(f"Generated {len(all_mappings)} API-Register mappings")

        return all_mappings

    def _build_register_map(self, registers: List) -> None:
        """
        Build a lookup map of registers by peripheral.

        Args:
            registers: List of SVDRegister objects
        """
        self.register_map = {}

        for reg in registers:
            periph = reg.peripheral.upper()
            if periph not in self.register_map:
                self.register_map[periph] = []

            # Full register name (e.g., "UART_CR")
            full_name = f"{periph}_{reg.name}"
            self.register_map[periph].append(full_name)
            self.register_map[periph].append(reg.name)  # Also just "CR"

    def load_manual_overrides(self, yaml_path: str) -> List[APIRegisterMapping]:
        """
        Load manual API-register mappings from YAML file.

        Args:
            yaml_path: Path to YAML configuration file

        Returns:
            List of APIRegisterMapping objects
        """
        mappings = []

        try:
            with open(yaml_path, 'r') as f:
                config = yaml.safe_load(f)

            for entry in config.get('mappings', []):
                mapping = APIRegisterMapping(
                    api_name=entry['api'],
                    register_names=entry['registers'],
                    field_names=entry.get('fields', []),
                    confidence=entry.get('confidence', 1.0),
                    mapping_source='manual',
                    access_type=entry.get('access_type', 'configure')
                )
                mappings.append(mapping)

            logger.info(f"Loaded {len(mappings)} manual mappings from {yaml_path}")

        except FileNotFoundError:
            logger.warning(f"Manual mapping file not found: {yaml_path}")
        except Exception as e:
            logger.error(f"Failed to load manual mappings: {e}")

        return mappings

    def _get_manual_mapping(self, api_name: str) -> Optional[APIRegisterMapping]:
        """Get manual mapping for an API if exists."""
        for mapping in self.manual_mappings:
            if mapping.api_name == api_name:
                return mapping
        return None

    def _apply_heuristic_rules(
        self,
        func,  # FunctionDeclaration
        registers: List
    ) -> List[APIRegisterMapping]:
        """
        Apply heuristic rules to map function to registers.

        Rules:
        1. Function name contains peripheral and register suffix
        2. Parameter type indicates peripheral
        3. Common patterns (_Init, _DeInit, _SetConfig, etc.)

        Args:
            func: Function declaration
            registers: List of registers

        Returns:
            List of mappings
        """
        mappings = []
        func_name = func.name.upper()

        # Rule 1: Extract peripheral from function name
        peripheral = self._extract_peripheral_from_name(func_name)

        if peripheral and peripheral in self.register_map:
            # Rule 2: Match based on function action
            action, target = self._extract_action_and_target(func_name)

            if action and target:
                # Look for matching register
                register_name = self._find_register_by_action(
                    peripheral, action, target, registers
                )

                if register_name:
                    confidence = self._calculate_confidence(func_name, action)
                    mappings.append(APIRegisterMapping(
                        api_name=func.name,
                        register_names=[register_name],
                        confidence=confidence,
                        mapping_source='name_match',
                        access_type=self._infer_access_type(action)
                    ))

        # Rule 3: Parameter type analysis
        param_registers = self._extract_registers_from_params(func, registers)
        for reg_name in param_registers:
            mappings.append(APIRegisterMapping(
                api_name=func.name,
                register_names=[reg_name],
                confidence=0.7,
                mapping_source='parameter_type',
                access_type='configure'
            ))

        return mappings

    def _extract_peripheral_from_name(self, func_name: str) -> Optional[str]:
        """Extract peripheral name from function name."""
        for periph, patterns in self.PERIPHERAL_PATTERNS.items():
            for pattern in patterns:
                if pattern in func_name:
                    return periph
        return None

    def _extract_action_and_target(self, func_name: str) -> Tuple[Optional[str], Optional[str]]:
        """
        Extract action and target from function name.

        Examples:
            "UART_SET_BAUDRATE" -> ("SET", "BAUDRATE")
            "GPIO_INIT" -> ("INIT", None)
            "ADC_ENABLE" -> ("ENABLE", None)
        """
        # Common action patterns
        actions = ['INIT', 'DEINIT', 'ENABLE', 'DISABLE', 'SET', 'GET', 'READ', 'WRITE',
                   'CONFIG', 'RESET', 'CLEAR', 'TOGGLE', 'START', 'STOP']

        for action in actions:
            if f"_{action}_" in func_name or func_name.endswith(f"_{action}"):
                return action, None

        return None, None

    def _find_register_by_action(
        self,
        peripheral: str,
        action: str,
        target: Optional[str],
        registers: List
    ) -> Optional[str]:
        """Find register by matching action to register suffix."""
        # Action to register suffix mapping
        action_to_suffix = {
            'INIT': 'CR',      # Control Register
            'ENABLE': 'CR',    # Control Register
            'DISABLE': 'CR',
            'SET': 'CR',       # Control Register or specific register
            'GET': 'SR',       # Status Register
            'READ': 'DR',      # Data Register
            'WRITE': 'DR',
            'CONFIG': 'CFGR',  # Configuration Register
            'RESET': 'CR',
            'CLEAR': 'CR',
        }

        # Look for matching register in this peripheral
        for reg in registers:
            if reg.peripheral.upper() != peripheral:
                continue

            reg_name = reg.name.upper()

            # Check if register name matches action
            suffix = action_to_suffix.get(action, '')
            if suffix and reg_name.endswith(suffix):
                return f"{peripheral}_{reg.name}"

            # Special case: baudrate
            if 'BAUD' in action or 'BAUD' in str(target).upper():
                if 'BRR' in reg_name or 'BAUD' in reg_name:
                    return f"{peripheral}_{reg.name}"

        return None

    def _calculate_confidence(self, func_name: str, action: str) -> float:
        """Calculate mapping confidence based on pattern strength."""
        # High confidence patterns
        if action in ['INIT', 'ENABLE', 'DISABLE']:
            return 0.8
        if 'BAUD' in func_name or 'CONFIG' in func_name:
            return 0.85

        # Medium confidence
        if action in ['SET', 'GET', 'READ', 'WRITE']:
            return 0.7

        # Lower confidence
        return 0.5

    def _infer_access_type(self, action: str) -> str:
        """Infer how registers are accessed based on action."""
        if action in ['GET', 'READ']:
            return 'read'
        if action in ['SET', 'WRITE', 'INIT', 'ENABLE', 'DISABLE', 'CONFIG']:
            return 'write'
        return 'configure'

    def _extract_registers_from_params(
        self,
        func,  # FunctionDeclaration
        registers: List
    ) -> List[str]:
        """Extract register names from function parameters."""
        found = []

        for param in func.parameters:
            # Check for pointer to peripheral type
            param_type = param.type.upper()

            if 'TYPEDEF' in param_type:
                # Extract peripheral name from typedef
                for periph in self.register_map.keys():
                    if periph in param_type:
                        # Return all registers for this peripheral
                        found.extend(self.register_map[periph])
                        break

        return found

    def build_bidirectional_mappings(
        self,
        mappings: List[APIRegisterMapping]
    ) -> Dict[str, RegisterAPIMapping]:
        """
        Build reverse mappings: register -> APIs.

        Args:
            mappings: API to register mappings

        Returns:
            Dict mapping register names to RegisterAPIMapping objects
        """
        register_mappings: Dict[str, RegisterAPIMapping] = {}

        for mapping in mappings:
            for reg_name in mapping.register_names:
                if reg_name not in register_mappings:
                    # Extract peripheral from register name
                    parts = reg_name.split('_')
                    peripheral = parts[0] if parts else "UNKNOWN"

                    register_mappings[reg_name] = RegisterAPIMapping(
                        register_name=reg_name,
                        peripheral=peripheral
                    )

                # Add API to appropriate access list
                reg_mapping = register_mappings[reg_name]
                api_name = mapping.api_name

                if mapping.access_type == 'read':
                    if api_name not in reg_mapping.apis_that_read:
                        reg_mapping.apis_that_read.append(api_name)
                elif mapping.access_type == 'write':
                    if api_name not in reg_mapping.apis_that_write:
                        reg_mapping.apis_that_write.append(api_name)
                else:  # configure
                    if api_name not in reg_mapping.apis_that_configure:
                        reg_mapping.apis_that_configure.append(api_name)

        return register_mappings


def create_sample_mapping_config(output_path: str) -> None:
    """
    Create a sample manual mapping configuration file.

    Args:
        output_path: Path where to create the sample file
    """
    sample_config = {
        'mappings': [
            {
                'api': 'B6x_UART_Init',
                'registers': ['UART_CR', 'UART_BRR', 'UART_LCR'],
                'fields': [],
                'confidence': 1.0,
                'access_type': 'configure',
                'note': 'Manual mapping for UART initialization'
            },
            {
                'api': 'B6x_UART_SetBaudRate',
                'registers': ['UART_BRR'],
                'fields': ['DIV_MANTISSA', 'DIV_FRACTION'],
                'confidence': 1.0,
                'access_type': 'write',
                'note': 'Baud rate configuration'
            },
            {
                'api': 'RCC_EnablePeriphClock',
                'registers': ['RCC_CLK_EN_ST'],
                'fields': [],
                'confidence': 1.0,
                'access_type': 'write',
                'note': 'Enable peripheral clock'
            }
        ]
    }

    with open(output_path, 'w') as f:
        yaml.dump(sample_config, f, default_flow_style=False)

    logger.info(f"Created sample mapping config: {output_path}")


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    # Create sample config
    config_dir = Path(__file__).parent.parent.parent / "config"
    config_dir.mkdir(exist_ok=True)

    sample_path = config_dir / "api_register_mapping.yaml"
    create_sample_mapping_config(str(sample_path))

    print(f"Sample configuration created at: {sample_path}")
