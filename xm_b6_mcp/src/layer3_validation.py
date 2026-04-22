"""
Layer 3: Validation Layer - Configuration Validator
===================================================

解决"配置行不行"的问题

This layer validates hardware configurations:
- Pin conflicts
- Clock tree validity
- DMA allocation
- Interrupt priorities
- Memory usage
- Power estimation
- Compatibility checks

Pattern: Chain of Responsibility Pattern (顺序验证链)

Author: B6x MCP Server Team
Version: 1.0.0
"""

import json
import logging
import hashlib
import yaml
from typing import Dict, List, Any, Optional, Literal
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from enum import Enum

# Import existing validators
try:
    from src.modules.module_b.constraint_tools import (
        validate_pin_config,
        check_flash_usage,
        check_sram_allocation,
        estimate_battery_life,
    )
    from src.core.config_loader import get_config_loader, DependencyOverride
except ImportError as e:
    logging.warning(f"Layer 3: Import failed: {e}")

# Import map file parser for static memory analysis
try:
    from src.core.map_file_parser import parse_map_file, MemoryUsageReport
    MAP_PARSER_AVAILABLE = True
except ImportError:
    MAP_PARSER_AVAILABLE = False
    logging.warning("Layer 3: map_file_parser not available")

# Import ConfigSchema for incremental validation
try:
    from src.common.config_schema import ConfigSchema, ValidationMode
except ImportError:
    # Define fallback if common module not available
    class ValidationMode(str, Enum):
        STRICT = "strict"
        INCREMENTAL = "incremental"
        ESTIMATE = "estimate"

    ConfigSchema = None
    logging.warning("Layer 3: ConfigSchema module not available, using fallback")


logger = logging.getLogger(__name__)


# ============================================================================
# Data Classes
# ============================================================================

@dataclass
class ValidationIssue:
    """Single validation issue"""
    severity: Literal["error", "warning", "info"]
    category: str
    message: str
    location: Optional[str] = None
    suggestion: Optional[str] = None
    affected_items: List[str] = field(default_factory=list)


@dataclass
class ValidationReport:
    """Complete validation report with incremental mode support"""
    is_valid: bool
    mode: str  # Validation mode (strict/incremental/estimate)
    total_issues: int
    errors: List[ValidationIssue]
    warnings: List[ValidationIssue]
    info: List[ValidationIssue]
    validated_items: Dict[str, bool]
    validated_fields: List[str]  # Fields that were validated
    missing_fields: List[str]  # Recommended fields that are missing
    metadata: Dict[str, Any] = field(default_factory=dict)


# ============================================================================
# Main Configuration Validator
# ============================================================================

class ConfigValidator:
    """
    Configuration validator using Chain of Responsibility Pattern.

    Validation chain (in order of priority):
    1. Pin conflicts (must pass) - ERROR
    2. Clock tree validity (must pass) - ERROR
    3. DMA allocation (must pass) - ERROR
    4. Interrupt priority - WARNING
    5. Memory usage - ERROR for static memory overflow (v3.6)
    6. Power estimation - INFO
    7. Compatibility check - INFO
    8. API dependencies - ERROR
    9. Register conflicts - WARNING/ERROR

    Memory validation severity changes (v3.6):
    - Static memory > BLE mode user space limit: ERROR (was WARNING)
    - Static memory > 90% of limit: ERROR (was WARNING)
    - Stack depth estimate > Stack_Size: ERROR (NEW)
    """

    def __init__(self):
        self.chain = self._get_default_validators()
        self.constraint_cache: Dict[str, Any] = {}

        # Load dependency overrides configuration
        try:
            self.config_loader = get_config_loader()
            self.dependencies: Dict[str, DependencyOverride] = self.config_loader.load_dependency_overrides()
            logging.info(f"ConfigValidator: Loaded {len(self.dependencies)} dependency overrides")
        except Exception as e:
            logging.warning(f"ConfigValidator: Failed to load dependency overrides: {e}")
            self.config_loader = None
            self.dependencies = {}

    def _get_default_validators(self) -> List[callable]:
        """Get default list of validator methods (used by __init__)"""
        return [
            self._validate_api_dependencies,  # Check API dependencies first
            self._validate_register_conflicts,  # NEW: Check register access conflicts
            self._validate_pins,
            self._validate_clock,
            self._validate_dma,
            self._validate_interrupts,
            self._validate_memory,
            self._estimate_power,
            self._check_compatibility,
        ]

    async def validate(
        self,
        config_data: Dict[str, Any],
        stop_on_first_error: bool = False,
        include_suggestions: bool = True,
        mode: str = "incremental"
    ) -> Dict[str, Any]:
        """
        Execute configuration validation with incremental support.

        Args:
            config_data: Configuration data with sections:
                - pins: Dict[str, str] - Pin configuration
                - clock: Dict - Clock configuration
                - dma: List[Dict] - DMA configuration
                - interrupts: List[Dict] - Interrupt configuration
                - memory: Dict - Memory configuration
                - ble: Dict - BLE configuration
            stop_on_first_error: Stop at first error
            include_suggestions: Include fixing suggestions
            mode: Validation mode (strict/incremental/estimate)

        Returns:
            Complete validation report with incremental info
        """
        # Parse validation mode
        if ValidationMode is not None:
            try:
                validation_mode = ValidationMode(mode)
            except ValueError:
                validation_mode = ValidationMode.INCREMENTAL
        else:
            validation_mode = mode

        # Parse configuration schema
        if ConfigSchema is not None:
            try:
                config_schema = ConfigSchema.from_dict(config_data)

                # Validate schema itself
                schema_errors = config_schema.validate(validation_mode)
                if schema_errors and validation_mode == ValidationMode.STRICT:
                    # Schema validation failed in strict mode
                    return {
                        "is_valid": False,
                        "mode": mode,
                        "errors": [{"message": e} for e in schema_errors],
                        "validated_fields": [],
                        "missing_fields": config_schema.get_missing_fields(),
                    }
            except Exception as e:
                logging.warning(f"ConfigSchema parsing failed: {e}, using raw config")
                config_schema = None
        else:
            config_schema = None

        # Build validation chain based on present fields
        chain = self._build_validation_chain(config_data, config_schema, validation_mode)

        # Initialize report
        report = ValidationReport(
            is_valid=True,
            mode=mode,
            total_issues=0,
            errors=[],
            warnings=[],
            info=[],
            validated_items={},
            validated_fields=config_schema.get_present_fields() if config_schema else list(config_data.keys()),
            missing_fields=config_schema.get_missing_fields() if config_schema else [],
            metadata={
                "validation_time": datetime.now().isoformat(),
                "config_hash": self._hash_config(config_data)
            }
        )

        # Execute validation chain
        for validator in chain:
            validator_name = validator.__name__.replace("_validate_", "")

            try:
                issues = await validator(config_data)

                # Classify issues
                for issue in issues:
                    report.total_issues += 1

                    if issue.severity == "error":
                        report.errors.append(issue)
                        report.is_valid = False
                        if stop_on_first_error:
                            break
                    elif issue.severity == "warning":
                        report.warnings.append(issue)
                    else:
                        report.info.append(issue)

                # Record validation result
                report.validated_items[validator_name] = len([i for i in issues if i.severity == "error"]) == 0

            except Exception as e:
                # Validator itself failed
                report.errors.append(ValidationIssue(
                    severity="error",
                    category="validation_error",
                    message=f"Validator {validator_name} failed: {str(e)}",
                    suggestion="Check validator implementation and configuration files"
                ))
                report.is_valid = False
                if stop_on_first_error:
                    break

        # Format report
        return self._format_report(report, include_suggestions)

    def _build_validation_chain(
        self,
        config: Dict[str, Any],
        config_schema: Optional["ConfigSchema"],
        mode: str
    ) -> List[callable]:
        """
        Build validation chain based on present fields and mode.

        In incremental mode, only include validators for fields that are present.
        In strict mode, include all validators.
        """
        chain = []

        # Always run if dependencies are available
        if self.dependencies:
            chain.append(self._validate_api_dependencies)

        # Always run register conflict check if data available
        chain.append(self._validate_register_conflicts)

        if config_schema is not None:
            # Use ConfigSchema to determine which validators to run
            if config_schema.pins:
                chain.append(self._validate_pins)

            if config_schema.clock:
                chain.append(self._validate_clock)

            if config_schema.dma:
                chain.append(self._validate_dma)

            if config_schema.interrupts:
                chain.append(self._validate_interrupts)

            if config_schema.memory:
                chain.append(self._validate_memory)

            if config_schema.ble:
                # BLE validation through compatibility check
                chain.append(self._check_compatibility)
        else:
            # Fallback: use raw config to determine validators
            if config.get("pins"):
                chain.append(self._validate_pins)

            if config.get("clock"):
                chain.append(self._validate_clock)

            if config.get("dma"):
                chain.append(self._validate_dma)

            if config.get("interrupts"):
                chain.append(self._validate_interrupts)

            if config.get("memory"):
                chain.append(self._validate_memory)

            if config.get("ble"):
                chain.append(self._check_compatibility)

        # Optional: Always run (info level)
        chain.append(self._estimate_power)

        return chain

    # ========================================================================
    # Validation Chain Methods
    # ========================================================================

    async def _validate_api_dependencies(self, config: Dict) -> List[ValidationIssue]:
        """
        Validate API dependencies using dependency_overrides.yaml.

        Checks:
        1. All prerequisites are satisfied
        2. Clock requirements are met
        3. GPIO requirements are met
        4. DMA requirements are met
        5. Interrupt requirements are met
        6. Special case requirements (I2C delay, etc.)
        """
        issues = []

        if not self.config_loader or not self.dependencies:
            # No dependency data available, skip this validation
            return []

        # Get list of APIs to be used from config
        api_calls = config.get("api_calls", [])
        peripherals = config.get("peripherals", {})

        # Build list of all APIs that will be called
        all_apis = list(api_calls)

        # Add APIs inferred from peripherals
        for periph_name, periph_config in peripherals.items():
            periph_upper = periph_name.upper()
            if periph_upper in ["UART", "USART"]:
                all_apis.append("uart_init")
            elif periph_upper == "I2C":
                all_apis.append("i2c_init")
            elif periph_upper in ["SPI", "SPIM"]:
                all_apis.append("spim_init")
            elif periph_upper == "DMA":
                all_apis.append("dma_init")
            elif periph_upper in ["TIMER", "TIM"]:
                all_apis.append("ctmr_init")
            elif periph_upper == "EXTI":
                all_apis.append("exti_init")
            elif periph_upper == "PWM":
                all_apis.append("pwm_init")

        # Validate each API
        for api_name in all_apis:
            dep = self.dependencies.get(api_name)
            if not dep:
                # No dependency info for this API, skip
                continue

            # Check prerequisites
            provided_calls = config.get("provided_calls", api_calls)

            is_valid, missing = self.config_loader.validate_api_requirements(api_name, provided_calls)

            if not is_valid:
                for m in missing:
                    if "Missing prerequisite" in m:
                        issues.append(ValidationIssue(
                            severity="error",
                            category="api_dependencies",
                            message=f"API {api_name}: {m}",
                            location=f"API: {api_name}",
                            suggestion=f"Call {m.split(': ')[1]} before {api_name}",
                            affected_items=[api_name]
                        ))
                    elif "Missing clock" in m:
                        issues.append(ValidationIssue(
                            severity="error",
                            category="clock",
                            message=f"API {api_name}: {m}",
                            location=f"API: {api_name}",
                            suggestion="Add RCC_APBCLK_EN or RCC_AHBCLK_EN before calling this API",
                            affected_items=[api_name]
                        ))
                    elif "Missing GPIO" in m:
                        issues.append(ValidationIssue(
                            severity="error",
                            category="gpio",
                            message=f"API {api_name}: {m}",
                            location=f"API: {api_name}",
                            suggestion="Configure GPIO pins using iom_ctrl, csc_output, or csc_input",
                            affected_items=[api_name]
                        ))
                    elif "Missing DMA" in m or "Missing interrupt" in m:
                        issues.append(ValidationIssue(
                            severity="error",
                            category="api_dependencies",
                            message=f"API {api_name}: {m}",
                            location=f"API: {api_name}",
                            suggestion=m,
                            affected_items=[api_name]
                        ))

            # Check for special case requirements
            if dep.notes:
                for note in dep.notes:
                    note_str = str(note).lower()

                    # I2C 180us delay
                    if "i2c" in api_name.lower() and "180us" in note_str:
                        if "i2c_delay" not in config:
                            issues.append(ValidationIssue(
                                severity="error",
                                category="api_dependencies",
                                message=f"CRITICAL: I2C requires 180us delay after GPIO config",
                                location=f"API: {api_name}",
                                suggestion="Add 180us delay (750 iterations at 64MHz) after initial GPIO configuration",
                                affected_items=[api_name]
                            ))

                    # UART RX pin wait
                    if "uart" in api_name.lower() and ("rx pin" in note_str or "wait" in note_str):
                        if "uart_rx_wait" not in config:
                            issues.append(ValidationIssue(
                                severity="warning",
                                category="api_dependencies",
                                message=f"UART requires waiting for RX pin to go high after reset",
                                location=f"API: {api_name}",
                                suggestion="Add loop to wait for RX pin to go high (check GPIO->PIN)",
                                affected_items=[api_name]
                            ))

                    # DMA init check
                    if "dma" in api_name.lower() and "already initialized" in note_str:
                        if not config.get("dma_init_checked", False):
                            issues.append(ValidationIssue(
                                severity="warning",
                                category="api_dependencies",
                                message=f"DMA should check if already initialized",
                                location=f"API: {api_name}",
                                suggestion="Check DMA->CTRLBASE_POINTER.Word before initializing",
                                affected_items=[api_name]
                            ))

        return issues

    async def _validate_register_conflicts(self, config: Dict) -> List[ValidationIssue]:
        """
        Validate register access conflicts using api_register_mapping.yaml.

        Checks:
        1. Multiple APIs writing to the same register (error)
        2. APIs with different access types to same register (warning)
        3. Shows which APIs conflict and which registers
        """
        issues = []

        if not self.config_loader:
            # No config loader available, skip this validation
            return []

        # Get list of APIs to be used from config
        api_calls = config.get("api_calls", [])
        peripherals = config.get("peripherals", {})

        # Build list of all APIs that will be called
        all_apis = list(api_calls)

        # Add APIs inferred from peripherals
        for periph_name, periph_config in peripherals.items():
            periph_upper = periph_name.upper()
            if periph_upper in ["UART", "USART"]:
                all_apis.append("uart_init")
            elif periph_upper == "I2C":
                all_apis.append("i2c_init")
            elif periph_upper in ["SPI", "SPIM"]:
                all_apis.append("spim_init")
            elif periph_upper == "DMA":
                all_apis.append("dma_init")
            elif periph_upper in ["TIMER", "TIM"]:
                all_apis.append("ctmr_init")
            elif periph_upper == "EXTI":
                all_apis.append("exti_init")
            elif periph_upper == "PWM":
                all_apis.append("pwm_init")

        # Remove duplicates
        all_apis = list(set(all_apis))

        # Find register conflicts
        try:
            conflicts = self.config_loader.find_register_conflicts(all_apis)

            if conflicts:
                for conflict in conflicts:
                    register = conflict.get("register", "Unknown")
                    api = conflict.get("api", "Unknown")
                    conflicts_with = conflict.get("conflicts_with", [])
                    access_type = conflict.get("access_type", "configure")
                    severity = conflict.get("severity", "warning")

                    # Build list of conflicting APIs
                    conflicting_apis = [api] + conflicts_with

                    # Generate message
                    if access_type == "write":
                        message = (
                            f"Register conflict: {len(conflicting_apis)} APIs write to register {register}. "
                            f"This may cause unexpected behavior."
                        )
                        suggestion = (
                            f"Ensure APIs are called in correct order or consider using "
                            f"read-modify-write operations. APIs: {', '.join(conflicting_apis)}"
                        )
                    else:
                        message = (
                            f"Register sharing: {len(conflicting_apis)} APIs access register {register}. "
                            f"Verify this is intended."
                        )
                        suggestion = (
                            f"Review API call order to ensure correct configuration. "
                            f"APIs: {', '.join(conflicting_apis)}"
                        )

                    issues.append(ValidationIssue(
                        severity=severity,
                        category="register_conflict",
                        message=message,
                        location=f"Register: {register}",
                        suggestion=suggestion,
                        affected_items=conflicting_apis
                    ))

        except Exception as e:
            logger.warning(f"Register conflict detection failed: {e}")
            # Don't fail validation if conflict detection fails
            pass

        return issues

    async def _validate_pins(self, config: Dict) -> List[ValidationIssue]:
        """Validate pin configuration (First priority)"""
        issues = []

        pin_config = config.get("pins", {})
        if not pin_config:
            return [ValidationIssue(
                severity="warning",
                category="pins",
                message="No pin configuration provided"
            )]

        # Load constraints
        io_map = self._load_constraint("io_map.json")
        if not io_map:
            return [ValidationIssue(
                severity="error",
                category="pins",
                message="Pin constraints not loaded",
                suggestion="Run build_excel_constraints.py first"
            )]

        # Build pin lookup
        pins_dict = {pin["pin_name"]: pin for pin in io_map.get("pins", [])}

        # Check each pin
        used_functions = {}  # function -> [pins]

        for pin_name, requested_function in pin_config.items():
            if pin_name not in pins_dict:
                issues.append(ValidationIssue(
                    severity="error",
                    category="pins",
                    message=f"Pin {pin_name} not found in IO map",
                    location=pin_name,
                    suggestion="Check pin name (e.g., PA0, PB1)"
                ))
                continue

            pin_data = pins_dict[pin_name]
            available_functions = [f["name"] for f in pin_data.get("functions", [])]

            # Check if function is available
            function_exists = False
            for func in pin_data.get("functions", []):
                if func["name"] == requested_function or func["peripheral"].upper() in requested_function.upper():
                    function_exists = True

                    # Check default function override
                    if func.get("is_default") == False and pin_data.get("default_function"):
                        if pin_data["default_function"] not in ["GPIO", "SWCLK", "SWDIO"]:
                            issues.append(ValidationIssue(
                                severity="warning",
                                category="pins",
                                message=f"{pin_name} default function is {pin_data['default_function']}",
                                location=pin_name,
                                suggestion=f"Ensure {pin_data['default_function']} is not needed"
                            ))
                    break

            if not function_exists:
                issues.append(ValidationIssue(
                    severity="error",
                    category="pins",
                    message=f"Pin {pin_name} does not support {requested_function}",
                    location=pin_name,
                    suggestion=f"Available functions: {', '.join(available_functions[:5])}"
                ))

            # Track function usage
            if requested_function not in used_functions:
                used_functions[requested_function] = []
            used_functions[requested_function].append(pin_name)

        # Check for function conflicts
        for func, pins in used_functions.items():
            if len(pins) > 1 and func != "GPIO":
                issues.append(ValidationIssue(
                    severity="error",
                    category="pins",
                    message=f"Function {func} assigned to multiple pins: {', '.join(pins)}",
                    affected_items=pins,
                    suggestion="Use each function on only one pin"
                ))

        return issues

    async def _validate_clock(self, config: Dict) -> List[ValidationIssue]:
        """Validate clock configuration (Second priority)"""
        issues = []
        clock_config = config.get("clock", {})

        # Check system clock
        sys_clk = clock_config.get("system_clock")
        if sys_clk:
            valid_sys_clks = [16, 32, 48, 64]  # MHz
            if sys_clk not in valid_sys_clks:
                issues.append(ValidationIssue(
                    severity="error",
                    category="clock",
                    message=f"Invalid system clock: {sys_clk} MHz",
                    suggestion=f"Valid values: {valid_sys_clks}"
                ))

        # Check peripheral clock enable
        pins = config.get("pins", {})
        required_peripherals = set()

        for func in pins.values():
            if "UART" in func:
                required_peripherals.add("UART")
            elif "SPI" in func:
                required_peripherals.add("SPI")
            elif "I2C" in func:
                required_peripherals.add("I2C")

        enabled_clocks = clock_config.get("enabled_peripherals", [])
        for periph in required_peripherals:
            if periph.lower() not in [c.lower() for c in enabled_clocks]:
                issues.append(ValidationIssue(
                    severity="error",
                    category="clock",
                    message=f"{periph} peripheral used but clock not enabled",
                    suggestion=f"Add {periph} to enabled_peripherals"
                ))

        return issues

    async def _validate_dma(self, config: Dict) -> List[ValidationIssue]:
        """Validate DMA configuration (Third priority)"""
        issues = []
        dma_config = config.get("dma", [])

        if not dma_config:
            return issues

        # Check DMA channel conflicts
        used_channels = {}
        for dma_req in dma_config:
            channel = dma_req.get("channel")
            peripheral = dma_req.get("peripheral")

            if channel in used_channels:
                issues.append(ValidationIssue(
                    severity="error",
                    category="dma",
                    message=f"DMA channel {channel} conflict",
                    affected_items=[used_channels[channel], peripheral],
                    suggestion=f"Use different DMA channels for {used_channels[channel]} and {peripheral}"
                ))
            else:
                used_channels[channel] = peripheral

        return issues

    async def _validate_interrupts(self, config: Dict) -> List[ValidationIssue]:
        """Validate interrupt configuration (Fourth priority - warning level)"""
        issues = []
        irq_config = config.get("interrupts", [])

        if not irq_config:
            return issues

        # Check interrupt priority reasonableness
        for irq in irq_config:
            priority = irq.get("priority", 0)
            if priority < 0 or priority > 15:
                issues.append(ValidationIssue(
                    severity="warning",
                    category="interrupts",
                    message=f"Invalid interrupt priority: {priority}",
                    location=irq.get("irq_name"),
                    suggestion="Priority should be 0-15 (lower = higher priority)"
                ))

        return issues

    async def _validate_memory(self, config: Dict) -> List[ValidationIssue]:
        """
        Validate memory usage with strict static memory checks.

        Checks:
        1. Flash usage > 90% → WARNING
        2. SRAM usage > total → ERROR
        3. Static memory > BLE mode user space limit → ERROR (NEW)
        4. Static memory > 90% of user space limit → ERROR (NEW)
        5. Stack depth estimate > Stack_Size → ERROR (NEW)

        Priority changed: Static memory overflow now causes ERROR instead of WARNING.
        """
        issues = []
        memory_config = config.get("memory", {})

        # ====================================================================
        # 1. Parse .map file if provided (most accurate)
        # ====================================================================
        map_file = memory_config.get("map_file")
        map_report = None

        if map_file and MAP_PARSER_AVAILABLE:
            try:
                map_report = parse_map_file(map_file)
                # Override manual values with parsed values
                data_bytes = map_report.data_bytes
                bss_bytes = map_report.bss_bytes
                vector_bytes = map_report.vector_bytes
                stack_bytes = map_report.stack_bytes

                logging.info(f"Parsed map file: data={data_bytes}, bss={bss_bytes}, vector={vector_bytes}")
            except Exception as e:
                logging.warning(f"Failed to parse map file {map_file}: {e}")
                data_bytes = 0
                bss_bytes = 0
                vector_bytes = 0
                stack_bytes = 1536  # Default from linker script
        else:
            # Use manual configuration
            data_bytes = memory_config.get("data_bytes", 0)
            bss_bytes = memory_config.get("bss_bytes", 0)
            vector_bytes = memory_config.get("vector_bytes", 152)  # Default ISR table
            stack_bytes = memory_config.get("stack_bytes", 1536)  # Default 0x600

        # Calculate total static memory
        static_memory_bytes = data_bytes + bss_bytes + vector_bytes
        static_memory_kb = static_memory_bytes / 1024

        # ====================================================================
        # 2. Check Flash usage (WARNING level)
        # ====================================================================
        flash_usage = memory_config.get("flash_used_kb", 0)
        flash_total = memory_config.get("flash_total_kb", 256)

        if map_report and map_report.text_bytes > 0:
            flash_usage = (map_report.text_bytes + map_report.rodata_bytes) / 1024

        if flash_usage > flash_total * 0.9:
            issues.append(ValidationIssue(
                severity="warning",
                category="memory",
                message=f"Flash usage {flash_usage:.2f}KB exceeds 90% of {flash_total}KB",
                suggestion="Consider optimizing code or using XIP"
            ))

        # ====================================================================
        # 3. Check SRAM usage against chip total (ERROR level)
        # ====================================================================
        sram_usage = memory_config.get("sram_used_kb", 0)
        sram_total = memory_config.get("sram_total_kb", 32)

        if map_report:
            sram_usage = map_report.sram_used_bytes / 1024
            sram_total = map_report.sram_total_bytes / 1024

        if sram_usage > sram_total:
            issues.append(ValidationIssue(
                severity="error",
                category="memory",
                message=f"SRAM usage {sram_usage:.2f}KB exceeds total {sram_total:.2f}KB",
                suggestion="Reduce global variables or increase SRAM budget"
            ))

        # ====================================================================
        # 4. Check static memory against BLE mode user space limit (ERROR)
        # ====================================================================
        ble_mode = memory_config.get("ble_mode", "mode1_no_ble")

        # Load BLE mode constraints from sram_map.yaml
        user_space_limit_kb = self._get_ble_user_space_limit(ble_mode)

        if user_space_limit_kb and static_memory_kb > 0:
            user_space_limit_bytes = user_space_limit_kb * 1024

            # ERROR: Static memory exceeds user space limit
            if static_memory_bytes > user_space_limit_bytes:
                issues.append(ValidationIssue(
                    severity="error",
                    category="memory",
                    message=f"CRITICAL: Static memory ({static_memory_kb:.2f}KB / {static_memory_bytes} bytes) exceeds user space limit ({user_space_limit_kb:.2f}KB) for BLE mode '{ble_mode}'",
                    location="memory.static_memory",
                    suggestion=f"Reduce global variables or switch to a BLE mode with more user space. Current: {ble_mode}, Limit: {user_space_limit_kb:.2f}KB",
                    affected_items=[f"data={data_bytes}B", f"bss={bss_bytes}B", f"vector={vector_bytes}B"]
                ))

            # ERROR: Static memory approaches limit (90% threshold)
            elif static_memory_bytes > user_space_limit_bytes * 0.9:
                usage_percent = static_memory_bytes / user_space_limit_bytes * 100
                issues.append(ValidationIssue(
                    severity="error",  # Changed from warning to error
                    category="memory",
                    message=f"Static memory ({static_memory_kb:.2f}KB) at {usage_percent:.1f}% of user space limit ({user_space_limit_kb:.2f}KB) for BLE mode '{ble_mode}'",
                    location="memory.static_memory",
                    suggestion=f"Consider reducing static memory usage. Only {user_space_limit_kb - static_memory_kb:.2f}KB remaining for runtime heap.",
                    affected_items=[f"data={data_bytes}B", f"bss={bss_bytes}B", f"vector={vector_bytes}B"]
                ))

            # INFO: Memory usage summary
            else:
                usage_percent = static_memory_bytes / user_space_limit_bytes * 100
                issues.append(ValidationIssue(
                    severity="info",
                    category="memory",
                    message=f"Static memory: {static_memory_kb:.2f}KB / {user_space_limit_kb:.2f}KB ({usage_percent:.1f}%) for BLE mode '{ble_mode}'",
                    location="memory.static_memory"
                ))

        # ====================================================================
        # 5. Check stack depth estimation (ERROR)
        # ====================================================================
        stack_depth_estimate = memory_config.get("stack_depth_estimate", 0)

        if stack_depth_estimate > 0:
            # Each stack frame is approximately 64 bytes on ARM Cortex-M0+
            bytes_per_frame = 64
            estimated_stack_bytes = stack_depth_estimate * bytes_per_frame

            # Default stack size is 1536 bytes (0x600) from linker script
            actual_stack_bytes = stack_bytes if stack_bytes > 0 else 1536

            # ERROR: Stack depth estimate exceeds or equals stack size
            if estimated_stack_bytes >= actual_stack_bytes:
                issues.append(ValidationIssue(
                    severity="error",
                    category="memory",
                    message=f"CRITICAL: Estimated stack usage ({estimated_stack_bytes} bytes, depth={stack_depth_estimate}) exceeds Stack_Size ({actual_stack_bytes} bytes)",
                    location="memory.stack",
                    suggestion=f"Increase Stack_Size in linker script or reduce call chain depth. Current: {actual_stack_bytes} bytes, Need: at least {estimated_stack_bytes} bytes",
                    affected_items=[f"depth={stack_depth_estimate}", f"frame_size={bytes_per_frame}B"]
                ))

            # WARNING: Stack depth approaches limit (80% threshold)
            elif estimated_stack_bytes > actual_stack_bytes * 0.8:
                usage_percent = estimated_stack_bytes / actual_stack_bytes * 100
                issues.append(ValidationIssue(
                    severity="warning",
                    category="memory",
                    message=f"Stack usage estimate ({estimated_stack_bytes} bytes) at {usage_percent:.1f}% of Stack_Size ({actual_stack_bytes} bytes)",
                    location="memory.stack",
                    suggestion=f"Consider increasing stack size or analyzing worst-case call depth"
                ))

        return issues

    def _get_ble_user_space_limit(self, ble_mode: str) -> Optional[float]:
        """
        Get user space limit (in KB) for a BLE mode from sram_map.yaml.

        Args:
            ble_mode: BLE mode string (e.g., "mode4_ble_lite")

        Returns:
            User space limit in KB, or None if not found
        """
        try:
            sram_map_path = Path(__file__).parent.parent / "config" / "sram_map.yaml"

            if not sram_map_path.exists():
                logging.warning(f"SRAM map config not found: {sram_map_path}")
                return None

            with open(sram_map_path, 'r', encoding='utf-8') as f:
                sram_config = yaml.safe_load(f)

            modes = sram_config.get("allocation_modes", {})
            mode_config = modes.get(ble_mode, {})
            user_space = mode_config.get("user_space_total", {})

            return user_space.get("kb", None)

        except Exception as e:
            logging.warning(f"Failed to load BLE mode constraints: {e}")
            return None

    async def _estimate_power(self, config: Dict) -> List[ValidationIssue]:
        """Power estimation (Sixth priority - info level)"""
        issues = []

        # Simplified power estimation
        pins = config.get("pins", {})
        clock = config.get("clock", {})
        sys_clk = clock.get("system_clock", 16)

        # Estimate current (uA)
        base_current = 5000  # 5mA base current
        clock_current = sys_clk * 100  # Clock-related current
        peripheral_current = len(pins) * 50  # Peripheral current

        total_current = base_current + clock_current + peripheral_current

        issues.append(ValidationIssue(
            severity="info",
            category="power",
            message=f"Estimated current: {total_current/1000:.2f} mA at {sys_clk} MHz",
            suggestion=f"For battery life estimation: {self._estimate_battery_life(total_current)}"
        ))

        return issues

    async def _check_compatibility(self, config: Dict) -> List[ValidationIssue]:
        """Compatibility check (Seventh priority - info level)"""
        issues = []

        # BLE configuration check
        ble_config = config.get("ble", {})
        if ble_config:
            roles = ble_config.get("roles", [])
            if "central" in roles and "peripheral" in roles:
                issues.append(ValidationIssue(
                    severity="info",
                    category="compatibility",
                    message="Dual role (central + peripheral) may have compatibility issues",
                    suggestion="Test with target phone models extensively"
                ))

        return issues

    # ========================================================================
    # Helper Methods
    # ========================================================================

    def _load_constraint(self, filename: str) -> Optional[Dict]:
        """Load constraint file with caching"""
        if filename in self.constraint_cache:
            return self.constraint_cache[filename]

        # Try to load from data/constraints/
        constraint_path = Path(__file__).parent.parent / "data" / "constraints" / filename

        if not constraint_path.exists():
            logger.warning(f"Constraint file not found: {constraint_path}")
            return None

        try:
            with open(constraint_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
                self.constraint_cache[filename] = data
                logger.info(f"Loaded constraint: {filename}")
                return data
        except Exception as e:
            logger.error(f"Error loading constraint {filename}: {e}")
            return None

    def _hash_config(self, config: Dict) -> str:
        """Generate configuration hash"""
        config_str = json.dumps(config, sort_keys=True)
        return hashlib.md5(config_str.encode()).hexdigest()[:8]

    def _estimate_battery_life(self, current_ma: float) -> str:
        """Estimate battery life"""
        capacity_mah = 225  # CR2032 battery
        hours = capacity_mah / current_ma
        return f"~{hours:.1f} hours on CR2032 battery"

    def _format_report(self, report: ValidationReport, include_suggestions: bool) -> Dict:
        """Format validation report with incremental info"""
        formatted = {
            "is_valid": report.is_valid,
            "mode": report.mode,
            "summary": {
                "total_issues": report.total_issues,
                "errors": len(report.errors),
                "warnings": len(report.warnings),
                "info": len(report.info)
            },
            "validated_fields": report.validated_fields,
            "issues_by_severity": {
                "errors": [self._format_issue(i, include_suggestions) for i in report.errors],
                "warnings": [self._format_issue(i, include_suggestions) for i in report.warnings],
                "info": [self._format_issue(i, include_suggestions) for i in report.info]
            },
            "validated_items": report.validated_items,
            "metadata": report.metadata
        }

        # Add missing fields info for incremental mode
        if report.missing_fields:
            formatted["missing_fields"] = report.missing_fields
            formatted["next_steps"] = [
                f"Add '{field}' configuration for complete validation"
                for field in report.missing_fields
            ]

        # Add suggestions summary
        if include_suggestions and (report.errors or report.warnings):
            formatted["suggestions"] = self._generate_suggestions(report)

        return formatted

    def _format_issue(self, issue: ValidationIssue, include_suggestions: bool) -> Dict:
        """Format single issue"""
        formatted = {
            "category": issue.category,
            "message": issue.message
        }

        if issue.location:
            formatted["location"] = issue.location
        if issue.affected_items:
            formatted["affected_items"] = issue.affected_items
        if include_suggestions and issue.suggestion:
            formatted["suggestion"] = issue.suggestion

        return formatted

    def _generate_suggestions(self, report: ValidationReport) -> List[str]:
        """Generate fixing suggestions summary"""
        suggestions = []

        for issue in report.errors + report.warnings:
            if issue.suggestion:
                suggestions.append(f"[{issue.category}] {issue.suggestion}")

        return list(set(suggestions))  # Deduplicate


# ============================================================================
# Global Instance and Tool Registration
# ============================================================================

_validator_instance: Optional[ConfigValidator] = None


def get_config_validator() -> ConfigValidator:
    """Get or create config validator singleton"""
    global _validator_instance
    if _validator_instance is None:
        try:
            # config removed
            _validator_instance = ConfigValidator()
        except Exception as e:
            logger.error(f"Failed to create ConfigValidator: {e}")
            _validator_instance = ConfigValidator()
    return _validator_instance


# Export for use in main.py
__all__ = [
    "ConfigValidator",
    "ValidationIssue",
    "ValidationReport",
    "get_config_validator"
]
