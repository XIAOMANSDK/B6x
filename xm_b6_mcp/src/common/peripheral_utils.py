"""
Shared Peripheral Type Detection Utilities
============================================

Provides common peripheral-to-API mapping functions used across
multiple layers (layer2_detail, layer3_validation).

This module eliminates code duplication and ensures consistent
peripheral type detection throughout the codebase.

Author: B6x MCP Server Team
Version: 1.0.0
"""

from typing import Optional, List, Dict, Set


# Mapping from peripheral name (uppercase) to initialization API
PERIPHERAL_API_MAP: Dict[str, str] = {
    "UART": "uart_init",
    "USART": "uart_init",
    "I2C": "i2c_init",
    "SPI": "spim_init",
    "SPIM": "spim_init",
    "SPIS": "spis_init",
    "DMA": "dma_init",
    "TIMER": "ctmr_init",
    "TIM": "ctmr_init",
    "CTMR": "ctmr_init",
    "EXTI": "exti_init",
    "PWM": "pwm_init",
    "GPIO": "gpio_init",
    "ADC": "adc_init",
    "DAC": "dac_init",
    "RTC": "rtc_init",
    "WDT": "wdt_init",
    "FLASH": "flash_init",
}

# Reverse mapping: API name to peripheral types
API_PERIPHERAL_MAP: Dict[str, Set[str]] = {}
for periph, api in PERIPHERAL_API_MAP.items():
    if api not in API_PERIPHERAL_MAP:
        API_PERIPHERAL_MAP[api] = set()
    API_PERIPHERAL_MAP[api].add(periph)


def get_api_for_peripheral(periph_name: str) -> Optional[str]:
    """
    Get the initialization API for a peripheral type.

    Args:
        periph_name: Peripheral name (case-insensitive)
                    Examples: "UART", "uart", "UART1", "SPI2"

    Returns:
        API name (e.g., "uart_init") or None if not found
    """
    if not periph_name:
        return None

    # Normalize: uppercase and strip numbers
    periph_upper = periph_name.upper().rstrip("0123456789")

    return PERIPHERAL_API_MAP.get(periph_upper)


def get_apis_from_peripherals(peripherals: Dict[str, any]) -> List[str]:
    """
    Extract all required API calls from peripheral configuration.

    Args:
        peripherals: Dict mapping peripheral names to their config
                    Examples: {"UART": {...}, "SPI": {...}}

    Returns:
        List of unique API names needed for these peripherals
    """
    apis: Set[str] = set()

    for periph_name in peripherals.keys():
        api = get_api_for_peripheral(periph_name)
        if api:
            apis.add(api)

    return list(apis)


def get_peripheral_type_from_name(periph_name: str) -> Optional[str]:
    """
    Extract the peripheral type from a full name.

    Args:
        periph_name: Full peripheral name
                    Examples: "UART1", "SPI2", "GPIOA"

    Returns:
        Peripheral type (e.g., "UART", "SPI") or None if not recognized
    """
    if not periph_name:
        return None

    # Normalize: uppercase and strip trailing digits/letters
    periph_upper = periph_name.upper()

    # Try exact match first
    if periph_upper in PERIPHERAL_API_MAP:
        return periph_upper

    # Try stripping trailing characters (UART1 -> UART)
    for i in range(len(periph_upper) - 1, 0, -1):
        candidate = periph_upper[:i]
        if candidate in PERIPHERAL_API_MAP:
            return candidate

    return None


def get_peripherals_for_api(api_name: str) -> Set[str]:
    """
    Get all peripheral types that use a given API.

    Args:
        api_name: API name (e.g., "uart_init")

    Returns:
        Set of peripheral type names
    """
    return API_PERIPHERAL_MAP.get(api_name.lower(), set())


def is_valid_peripheral_type(periph_name: str) -> bool:
    """
    Check if a peripheral type is recognized.

    Args:
        periph_name: Peripheral name to check

    Returns:
        True if the peripheral type is in the known list
    """
    if not periph_name:
        return False

    periph_upper = periph_name.upper().rstrip("0123456789")
    return periph_upper in PERIPHERAL_API_MAP


def normalize_peripheral_name(periph_name: str) -> str:
    """
    Normalize a peripheral name to standard form.

    Args:
        periph_name: Input peripheral name

    Returns:
        Normalized name (uppercase, type only)
    """
    if not periph_name:
        return ""

    return periph_name.upper().rstrip("0123456789")


# Category classification
PERIPHERAL_CATEGORIES = {
    "communication": {"UART", "USART", "SPI", "SPIM", "SPIS", "I2C"},
    "timer": {"TIMER", "TIM", "CTMR", "PWM", "RTC", "WDT"},
    "analog": {"ADC", "DAC"},
    "digital": {"GPIO", "EXTI"},
    "system": {"DMA", "FLASH"},
}


def get_peripheral_category(periph_name: str) -> Optional[str]:
    """
    Get the category for a peripheral type.

    Args:
        periph_name: Peripheral name

    Returns:
        Category name or None if not found
    """
    periph_type = normalize_peripheral_name(periph_name)

    for category, periphs in PERIPHERAL_CATEGORIES.items():
        if periph_type in periphs:
            return category

    return None


# Export symbols
__all__ = [
    "PERIPHERAL_API_MAP",
    "API_PERIPHERAL_MAP",
    "PERIPHERAL_CATEGORIES",
    "get_api_for_peripheral",
    "get_apis_from_peripherals",
    "get_peripheral_type_from_name",
    "get_peripherals_for_api",
    "is_valid_peripheral_type",
    "normalize_peripheral_name",
    "get_peripheral_category",
]
