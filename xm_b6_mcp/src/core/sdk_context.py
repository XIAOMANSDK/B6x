"""
SDK Context Manager for Clang Parsing
======================================

Provides compilation context (include paths, macros) for Clang-based code analysis.
This module automatically detects SDK paths and provides the necessary compilation
flags for parsing B6x SDK C code.

Author: B6x MCP Server Team
Version: 1.0.0
"""

import os
import logging
from pathlib import Path
from typing import List, Dict, Optional
from dataclasses import dataclass, field

logger = logging.getLogger(__name__)


# ============================================================================
# Data Classes
# ============================================================================

@dataclass
class SDKPaths:
    """SDK directory paths"""
    sdk_root: Path
    drivers_api: Path = None
    drivers_src: Path = None
    core: Path = None
    core_reg: Path = None  # Register definitions (UART1_BASE, UART_TypeDef, etc.)
    ble_api: Path = None
    ble_lib: Path = None
    ble_prf: Path = None
    projects: Path = None
    examples: Path = None

    def __post_init__(self):
        """Initialize sub-paths if sdk_root is provided"""
        if self.sdk_root:
            self.sdk_root = Path(self.sdk_root)
            self.drivers_api = self.sdk_root / "drivers" / "api"
            self.drivers_src = self.sdk_root / "drivers" / "src"
            self.core = self.sdk_root / "core"
            self.core_reg = self.sdk_root / "core" / "reg"  # Register definitions
            self.ble_api = self.sdk_root / "ble" / "api"
            self.ble_lib = self.sdk_root / "ble" / "lib"
            self.ble_prf = self.sdk_root / "ble" / "prf"
            self.projects = self.sdk_root / "projects"
            self.examples = self.sdk_root / "examples"


@dataclass
class SDKDefines:
    """SDK preprocessor macros"""
    # Chip family
    __B6X__: str = "1"

    # ARM Cortex-M0+ specific
    ARM_MATH_CM0_PLUS: str = "1"
    __ARM_ARCH_6M__: str = "1"

    # Target triple
    __ARM_ARCH_PROFILE: str = "M"

    # Common embedded macros
    __STATIC_FORCEINLINE: str = "static inline __attribute__((always_inline))"
    __STATIC_INLINE: str = "static inline"

    # Custom macros can be added via add_define()
    custom: Dict[str, str] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, str]:
        """Convert to dictionary including custom macros"""
        result = {
            "__B6X__": self.__B6X__,
            "ARM_MATH_CM0_PLUS": self.ARM_MATH_CM0_PLUS,
            "__ARM_ARCH_6M__": self.__ARM_ARCH_6M__,
            "__ARM_ARCH_PROFILE": self.__ARM_ARCH_PROFILE,
            "__STATIC_FORCEINLINE": self.__STATIC_FORCEINLINE,
            "__STATIC_INLINE": self.__STATIC_INLINE,
        }
        result.update(self.custom)
        return result


# ============================================================================
# SDK Context Class
# ============================================================================

class SDKContext:
    """
    SDK compilation context for Clang parsing.

    Provides include paths, preprocessor macros, and compilation flags
    needed for Clang to properly parse B6x SDK C code.

    Usage:
        context = SDKContext("/path/to/sdk6")
        compile_args = context.get_compile_args()

        # Use with clang
        tu = index.parse(code, args=compile_args)
    """

    def __init__(self, sdk_path: Optional[str] = None):
        """
        Initialize SDK context.

        Args:
            sdk_path: Path to SDK root. If None, auto-detects from module location.
        """
        self.sdk_path = self._detect_sdk_path(sdk_path)
        self.paths = SDKPaths(sdk_root=self.sdk_path)
        self.defines = SDKDefines()

        # Additional include paths (user-specified)
        self.extra_includes: List[str] = []

        # Additional defines (user-specified)
        self.extra_defines: Dict[str, str] = {}

        # Target platform
        self.target = "arm-none-eabi"

        # Language standard
        self.std = "c11"

        # Validate SDK path
        self._validate_sdk()

        logger.info(f"SDKContext initialized with SDK path: {self.sdk_path}")

    def _detect_sdk_path(self, sdk_path: Optional[str]) -> Path:
        """Auto-detect SDK path from module location"""
        if sdk_path:
            return Path(sdk_path)

        # Try environment variable first
        env_path = os.getenv("B6X_SDK_PATH")
        if env_path:
            return Path(env_path)

        # Detect from module location (xm_b6_mcp is under SDK root)
        module_path = Path(__file__).resolve()

        # Walk up to find SDK root (parent of xm_b6_mcp)
        for parent in module_path.parents:
            if (parent / "drivers").exists() and (parent / "core").exists():
                # Check if we're inside xm_b6_mcp
                if "xm_b6_mcp" in str(module_path):
                    # SDK root is parent of xm_b6_mcp
                    xm_b6_mcp_parent = module_path
                    for _ in range(4):  # Walk up to find xm_b6_mcp parent
                        if xm_b6_mcp_parent.name == "xm_b6_mcp":
                            return xm_b6_mcp_parent.parent
                        xm_b6_mcp_parent = xm_b6_mcp_parent.parent
                return parent

        # Fallback: assume default structure
        return module_path.parent.parent.parent

    def _validate_sdk(self):
        """Validate SDK path exists and has required structure"""
        if not self.sdk_path.exists():
            logger.warning(f"SDK path does not exist: {self.sdk_path}")
            return

        required_dirs = ["drivers", "core"]
        for dir_name in required_dirs:
            if not (self.sdk_path / dir_name).exists():
                logger.warning(f"SDK missing required directory: {dir_name}")

    def get_include_paths(self) -> List[str]:
        """
        Get all include paths for compilation.

        Returns:
            List of include directory paths
        """
        includes = []

        # SDK standard include paths
        sdk_includes = [
            self.paths.drivers_api,
            self.paths.core,
            self.paths.core_reg,  # Register definitions for type resolution
            self.paths.ble_api,
            self.paths.ble_lib,
            self.paths.ble_prf,
            self.paths.drivers_src,
        ]

        for path in sdk_includes:
            if path and path.exists():
                includes.append(str(path))

        # Add user-specified include paths
        includes.extend(self.extra_includes)

        return includes

    def get_defines(self) -> Dict[str, str]:
        """
        Get all preprocessor macros.

        Returns:
            Dictionary of macro name -> value
        """
        defines = self.defines.to_dict()
        defines.update(self.extra_defines)
        return defines

    def get_compile_args(self) -> List[str]:
        """
        Generate Clang compilation arguments.

        Returns:
            List of command-line arguments for Clang
        """
        args = []

        # Language standard
        args.append(f"-std={self.std}")

        # Target (for cross-compilation awareness)
        args.append(f"--target={self.target}")

        # Include paths
        for include_path in self.get_include_paths():
            args.extend(["-I", include_path])

        # Preprocessor macros
        for name, value in self.get_defines().items():
            if value:
                args.append(f"-D{name}={value}")
            else:
                args.append(f"-D{name}")

        # Additional Clang flags for embedded code
        # Note: Removed -nostdinc to allow standard headers (stdint.h, etc.)
        # SDK headers take precedence via -I paths added above
        args.extend([
            "-fno-builtin",        # Don't use builtin functions
            "-ffreestanding",      # Freestanding environment
            "-fno-blocks",         # Disable blocks extension
            "-Wno-unknown-attributes",  # Ignore unknown attributes
            "-Wno-unknown-pragmas",     # Ignore unknown pragmas
            "-Wno-unused-value",        # Ignore unused value warnings
            "-Wno-empty-translation-unit",  # Allow empty translation units
        ])

        return args

    def add_include(self, path: str):
        """Add an extra include path"""
        if Path(path).exists():
            self.extra_includes.append(str(path))
        else:
            logger.warning(f"Include path does not exist: {path}")

    def add_define(self, name: str, value: str = "1"):
        """Add a preprocessor macro"""
        self.extra_defines[name] = value

    def with_project(self, project_path: str) -> "SDKContext":
        """
        Create a new context with project-specific includes.

        Args:
            project_path: Path to project directory

        Returns:
            New SDKContext with project includes added
        """
        new_context = SDKContext(str(self.sdk_path))
        new_context.extra_includes = self.extra_includes.copy()
        new_context.extra_defines = self.extra_defines.copy()

        # Add project-specific paths
        project_path = Path(project_path)
        if project_path.exists():
            # Common project subdirectories
            for subdir in ["src", "include", "inc", "config"]:
                subdir_path = project_path / subdir
                if subdir_path.exists():
                    new_context.add_include(str(subdir_path))

        return new_context

    def get_context_info(self) -> Dict:
        """Get context information for debugging/logging"""
        return {
            "sdk_path": str(self.sdk_path),
            "target": self.target,
            "std": self.std,
            "include_count": len(self.get_include_paths()),
            "define_count": len(self.get_defines()),
            "includes": self.get_include_paths()[:5],  # First 5 for brevity
            "defines": list(self.get_defines().keys())[:10],  # First 10
        }


# ============================================================================
# Global Instance
# ============================================================================

_context_cache: Dict[str, SDKContext] = {}


def get_sdk_context(sdk_path: Optional[str] = None) -> SDKContext:
    """
    Get or create SDK context (cached).

    Args:
        sdk_path: Path to SDK root. If None, auto-detects.

    Returns:
        SDKContext instance
    """
    global _context_cache

    # Use resolved path as cache key
    if sdk_path:
        cache_key = str(Path(sdk_path).resolve())
    else:
        # Auto-detect and cache
        temp_context = SDKContext()
        cache_key = str(temp_context.sdk_path)

    if cache_key not in _context_cache:
        _context_cache[cache_key] = SDKContext(sdk_path)
        logger.info(f"Created new SDKContext for: {cache_key}")

    return _context_cache[cache_key]


def clear_context_cache():
    """Clear the context cache"""
    global _context_cache
    _context_cache = {}


# ============================================================================
# Exports
# ============================================================================

__all__ = [
    "SDKContext",
    "SDKPaths",
    "SDKDefines",
    "get_sdk_context",
    "clear_context_cache",
]
