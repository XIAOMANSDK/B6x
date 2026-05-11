"""
Configuration schema for incremental validation.

This module defines the configuration schema used by Layer 3 validation.
It supports incremental validation where partial configurations can be
validated without requiring all fields to be present.

Author: B6x MCP Server Team
Version: 1.0.0
"""

import logging
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, field
from enum import Enum


logger = logging.getLogger(__name__)


class ValidationMode(str, Enum):
    """
    Validation mode for configuration validation.

    Modes:
        STRICT: All required fields must be present and valid
        INCREMENTAL: Only validate provided fields (default)
        ESTIMATE: Estimate missing fields with warnings
    """
    STRICT = "strict"
    INCREMENTAL = "incremental"
    ESTIMATE = "estimate"


@dataclass
class ConfigSchema:
    """
    SDK configuration schema with optional fields.

    This class defines the structure of SDK configuration data.
    Fields marked Optional can be omitted in INCREMENTAL mode,
    allowing partial validation during early development stages.

    Required Fields:
        api_calls: List of API functions to be called
        pins: Pin assignment mapping (pin_name -> function)

    Optional Fields (validated if present):
        clock: Clock configuration (system_clock, enabled_peripherals)
        dma: DMA channel configurations
        interrupts: Interrupt configurations
        memory: Memory usage estimates
        ble: BLE configuration (roles, connections, library type)
        peripherals: Peripheral configurations (alternative to api_calls)

    Attributes:
        api_calls: List of API function names
        pins: Dictionary mapping pin names to functions
        clock: Clock configuration dict
        dma: List of DMA configurations
        interrupts: List of interrupt configurations
        memory: Memory usage dict
        ble: BLE configuration dict
        peripherals: Peripheral config dict
        provided_calls: List of already-called API functions
    """

    # === Required Fields ===
    api_calls: List[str] = field(default_factory=list)
    pins: Dict[str, str] = field(default_factory=dict)

    # === Optional Fields (validated if present) ===
    clock: Optional[Dict[str, Any]] = None
    dma: Optional[List[Dict[str, Any]]] = None
    interrupts: Optional[List[Dict[str, Any]]] = None
    memory: Optional[Dict[str, Any]] = None
    ble: Optional[Dict[str, Any]] = None
    peripherals: Optional[Dict[str, Any]] = None

    # === Metadata ===
    provided_calls: List[str] = field(default_factory=list)

    def validate(self, mode: ValidationMode = ValidationMode.INCREMENTAL) -> List[str]:
        """
        Validate config against schema.

        Args:
            mode: Validation mode (strict/incremental/estimate)

        Returns:
            List of validation errors (empty if valid)

        Examples:
            >>> config = ConfigSchema(api_calls=["B6x_UART_Init"])
            >>> config.validate(ValidationMode.INCREMENTAL)
            []

            >>> config.validate(ValidationMode.STRICT)
            ["'clock' field is required in strict mode"]
        """
        errors = []

        # Always validate basic required fields
        if not self.api_calls and not self.peripherals:
            errors.append("Either 'api_calls' or 'peripherals' must be provided")

        # Strict mode requires all recommended fields
        if mode == ValidationMode.STRICT:
            if not self.clock:
                errors.append("'clock' field is required in strict mode")

            if not self.pins:
                errors.append("'pins' field is required in strict mode")

        return errors

    def get_present_fields(self) -> List[str]:
        """
        Get list of fields that are present (not None/empty).

        Returns:
            List of field names that have values

        Examples:
            >>> config = ConfigSchema(
            ...     api_calls=["B6x_UART_Init"],
            ...     pins={"PA9": "UART1_TX"},
            ...     clock={"system_clock": 32}
            ... )
            >>> config.get_present_fields()
            ['api_calls', 'pins', 'clock']
        """
        return [
            name for name, value in [
                ("api_calls", self.api_calls),
                ("pins", self.pins),
                ("clock", self.clock),
                ("dma", self.dma),
                ("interrupts", self.interrupts),
                ("memory", self.memory),
                ("ble", self.ble),
                ("peripherals", self.peripherals),
            ]
            if value
        ]

    def get_missing_fields(self) -> List[str]:
        """
        Get list of recommended fields that are missing.

        Returns:
            List of field names that are recommended but not present

        Examples:
            >>> config = ConfigSchema(api_calls=["B6x_UART_Init"])
            >>> config.get_missing_fields()
            ['pins', 'clock', 'dma', 'interrupts', 'memory', 'ble']
        """
        all_recommended = ["pins", "clock", "dma", "interrupts", "memory", "ble"]
        present = self.get_present_fields()
        return [f for f in all_recommended if f not in present]

    def is_complete(self) -> bool:
        """
        Check if configuration has all recommended fields.

        Returns:
            True if all recommended fields are present

        Examples:
            >>> config = ConfigSchema(api_calls=["B6x_UART_Init"])
            >>> config.is_complete()
            False

            >>> config = ConfigSchema(
            ...     api_calls=["B6x_UART_Init"],
            ...     pins={"PA9": "UART1_TX"},
            ...     clock={"system_clock": 32}
            ... )
            >>> config.is_complete()
            False  # Still missing dma, interrupts, memory, ble
        """
        missing = self.get_missing_fields()
        return len(missing) == 0

    def get_validation_summary(self) -> Dict[str, Any]:
        """
        Get a summary of the configuration state.

        Returns:
            Dictionary with validation state information

        Examples:
            >>> config = ConfigSchema(api_calls=["B6x_UART_Init"])
            >>> config.get_validation_summary()
            {
                'total_fields': 8,
                'present_fields': 1,
                'missing_fields': 7,
                'completeness': 0.125,
                'can_validate_incrementally': True
            }
        """
        present = self.get_present_fields()
        missing = self.get_missing_fields()
        total_recommended = len(present) + len(missing)
        completeness = len(present) / total_recommended if total_recommended > 0 else 0

        return {
            "total_fields": total_recommended,
            "present_fields": present,
            "missing_fields": missing,
            "completeness": round(completeness, 2),
            "can_validate_incrementally": len(present) > 0,
            "ready_for_strict_validation": self.is_complete(),
        }

    @classmethod
    def from_dict(cls, config: Dict[str, Any]) -> "ConfigSchema":
        """
        Create ConfigSchema from raw dict.

        Args:
            config: Configuration dictionary

        Returns:
            ConfigSchema instance

        Examples:
            >>> raw = {
            ...     "api_calls": ["B6x_UART_Init"],
            ...     "pins": {"PA9": "UART1_TX"},
            ...     "clock": {"system_clock": 32}
            ... }
            >>> ConfigSchema.from_dict(raw)
            ConfigSchema(api_calls=['B6x_UART_Init'], pins={'PA9': 'UART1_TX'}, ...)
        """
        return cls(
            api_calls=config.get("api_calls", []),
            pins=config.get("pins", {}),
            clock=config.get("clock"),
            dma=config.get("dma"),
            interrupts=config.get("interrupts"),
            memory=config.get("memory"),
            ble=config.get("ble"),
            peripherals=config.get("peripherals"),
            provided_calls=config.get("provided_calls", []),
        )

    def to_dict(self) -> Dict[str, Any]:
        """
        Convert to dictionary.

        Returns:
            Dictionary representation of the configuration

        Examples:
            >>> config = ConfigSchema(api_calls=["B6x_UART_Init"])
            >>> config.to_dict()
            {'api_calls': ['B6x_UART_Init'], 'pins': {}, ...}
        """
        result = {
            "api_calls": self.api_calls,
            "pins": self.pins,
        }

        # Only include non-None optional fields
        if self.clock is not None:
            result["clock"] = self.clock
        if self.dma is not None:
            result["dma"] = self.dma
        if self.interrupts is not None:
            result["interrupts"] = self.interrupts
        if self.memory is not None:
            result["memory"] = self.memory
        if self.ble is not None:
            result["ble"] = self.ble
        if self.peripherals is not None:
            result["peripherals"] = self.peripherals
        if self.provided_calls:
            result["provided_calls"] = self.provided_calls

        return result

    def merge(self, other: "ConfigSchema") -> "ConfigSchema":
        """
        Merge another ConfigSchema into this one.

        Args:
            other: Another ConfigSchema to merge

        Returns:
            New ConfigSchema with merged values

        Examples:
            >>> config1 = ConfigSchema(api_calls=["B6x_UART_Init"])
            >>> config2 = ConfigSchema(pins={"PA9": "UART1_TX"})
            >>> merged = config1.merge(config2)
            >>> merged.api_calls
            ['B6x_UART_Init']
            >>> merged.pins
            {'PA9': 'UART1_TX'}
        """
        return ConfigSchema(
            api_calls=list(set(self.api_calls + other.api_calls)),
            pins={**self.pins, **other.pins},
            clock=other.clock or self.clock,
            dma=(self.dma or []) + (other.dma or []),
            interrupts=(self.interrupts or []) + (other.interrupts or []),
            memory=other.memory or self.memory,
            ble=other.ble or self.ble,
            peripherals={**(self.peripherals or {}), **(other.peripherals or {})},
            provided_calls=list(set(self.provided_calls + other.provided_calls)),
        )


def create_incremental_config(**kwargs) -> ConfigSchema:
    """
    Helper function to create a ConfigSchema for incremental validation.

    This is a convenience function for creating partial configurations
    during incremental development.

    Args:
        **kwargs: Configuration fields (any field supported by ConfigSchema)

    Returns:
        ConfigSchema instance

    Examples:
        >>> config = create_incremental_config(
        ...     api_calls=["B6x_UART_Init"],
        ...     pins={"PA9": "UART1_TX"}
        ... )
        >>> config.get_present_fields()
        ['api_calls', 'pins']
    """
    return ConfigSchema.from_dict(kwargs)


def validate_config_dict(
    config: Dict[str, Any],
    mode: ValidationMode = ValidationMode.INCREMENTAL
) -> tuple[bool, List[str]]:
    """
    Validate a configuration dictionary.

    This is a convenience function that combines schema parsing
    and validation in one step.

    Args:
        config: Configuration dictionary
        mode: Validation mode

    Returns:
        Tuple of (is_valid, error_list)

    Examples:
        >>> config = {"api_calls": ["B6x_UART_Init"]}
        >>> is_valid, errors = validate_config_dict(config)
        >>> is_valid
        True
        >>> errors
        []
    """
    schema = ConfigSchema.from_dict(config)
    errors = schema.validate(mode)

    return len(errors) == 0, errors


# Export for convenience
__all__ = [
    "ConfigSchema",
    "ValidationMode",
    "create_incremental_config",
    "validate_config_dict",
]
