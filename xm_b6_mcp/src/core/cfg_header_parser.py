"""
CFG Header Parser
=================

Parse projects/*/src/cfg.h configuration headers.

Extracts:
- System configuration: SYS_CLK, DBG_MODE, CFG_SLEEP
- BLE configuration: BLE_NB_SLAVE, BLE_NB_MASTER, BLE_ADDR, BLE_DEV_NAME
- Profile switches: PRF_DISS, PRF_BASS, PRF_HIDS, PRF_OTA
- Other project-specific macros

Author: B6x MCP Server Team
Version: 0.1.0
"""

import re
import logging
from pathlib import Path
from typing import Dict, List, Optional, Set, Any
from dataclasses import dataclass, field
from collections import defaultdict
import json

logger = logging.getLogger(__name__)


# ============================================================================
# Known Configuration Macros
# ============================================================================

# System configuration
SYSTEM_CONFIG_MACROS = {
    'SYS_CLK': {
        'description': 'System Clock (0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)',
        'category': 'system',
        'type': 'enum',
        'values': ['16MHz', '32MHz', '48MHz', '64MHz'],
    },
    'DBG_MODE': {
        'description': 'Debug Mode (0=Disable, 1=UART, 2=RTT)',
        'category': 'system',
        'type': 'enum',
        'values': ['Disable', 'UART', 'RTT'],
    },
    'CFG_SLEEP': {
        'description': 'Sleep mode configuration',
        'category': 'system',
        'type': 'enum',
    },
    'LED_PLAY': {
        'description': 'LED animation support',
        'category': 'system',
        'type': 'bool',
    },
}

# BLE configuration
BLE_CONFIG_MACROS = {
    'BLE_NB_SLAVE': {
        'description': 'Number of slave connections',
        'category': 'ble',
        'type': 'int',
        'default': 1,
    },
    'BLE_NB_MASTER': {
        'description': 'Number of master connections',
        'category': 'ble',
        'type': 'int',
        'default': 0,
    },
    'BLE_ADDR': {
        'description': 'BLE device address',
        'category': 'ble',
        'type': 'array',
    },
    'BLE_DEV_NAME': {
        'description': 'BLE device name',
        'category': 'ble',
        'type': 'string',
    },
    'BLE_DEV_ICON': {
        'description': 'BLE appearance icon',
        'category': 'ble',
        'type': 'hex',
    },
    'BLE_PHY': {
        'description': 'BLE PHY configuration',
        'category': 'ble',
        'type': 'symbol',
    },
    'BLE_AUTH': {
        'description': 'BLE authentication requirements',
        'category': 'ble',
        'type': 'symbol',
    },
    'BLE_DBG_LTK': {
        'description': 'Debug LTK generation',
        'category': 'ble',
        'type': 'bool',
    },
}

# Profile configuration
PROFILE_CONFIG_MACROS = {
    'PRF_DISS': {
        'description': 'Device Information Service',
        'category': 'profile',
        'type': 'bool',
        'default': 0,
    },
    'PRF_BASS': {
        'description': 'Battery Service',
        'category': 'profile',
        'type': 'bool',
        'default': 0,
    },
    'PRF_HIDS': {
        'description': 'HID Service',
        'category': 'profile',
        'type': 'bool',
        'default': 0,
    },
    'PRF_OTA': {
        'description': 'OTA Upgrade Service',
        'category': 'profile',
        'type': 'bool',
        'default': 0,
    },
    'PRF_HRS': {
        'description': 'Heart Rate Service',
        'category': 'profile',
        'type': 'bool',
        'default': 0,
    },
    'PRF_SCPS': {
        'description': 'Scan Parameters Service',
        'category': 'profile',
        'type': 'bool',
        'default': 0,
    },
    'PRF_USER': {
        'description': 'User-defined Profile',
        'category': 'profile',
        'type': 'bool',
        'default': 0,
    },
}

# Debug configuration
DEBUG_CONFIG_MACROS = {
    'DBG_APP': {'description': 'App debug', 'category': 'debug', 'type': 'bool'},
    'DBG_PROC': {'description': 'Procedure debug', 'category': 'debug', 'type': 'bool'},
    'DBG_ACTV': {'description': 'Activity debug', 'category': 'debug', 'type': 'bool'},
    'DBG_GAPM': {'description': 'GAP Manager debug', 'category': 'debug', 'type': 'bool'},
    'DBG_GAPC': {'description': 'GAP Controller debug', 'category': 'debug', 'type': 'bool'},
    'DBG_HIDS': {'description': 'HID Service debug', 'category': 'debug', 'type': 'bool'},
    'DBG_BASS': {'description': 'Battery Service debug', 'category': 'debug', 'type': 'bool'},
}

# All known macros
ALL_KNOWN_MACROS = {}
ALL_KNOWN_MACROS.update(SYSTEM_CONFIG_MACROS)
ALL_KNOWN_MACROS.update(BLE_CONFIG_MACROS)
ALL_KNOWN_MACROS.update(PROFILE_CONFIG_MACROS)
ALL_KNOWN_MACROS.update(DEBUG_CONFIG_MACROS)


@dataclass
class ConfigMacro:
    """
    A configuration macro definition.

    Attributes:
        name: Macro name
        value: Macro value (as string)
        category: Category (system, ble, profile, debug, other)
        description: Description from comment or known database
        file_path: Source file path
        line_number: Line number in source
        value_type: Type of value (int, string, bool, array, hex, symbol)
    """
    name: str
    value: str
    category: str = "other"
    description: str = ""
    file_path: str = ""
    line_number: int = 0
    value_type: str = "unknown"

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "name": self.name,
            "value": self.value,
            "category": self.category,
            "description": self.description,
            "file_path": self.file_path,
            "line_number": self.line_number,
            "value_type": self.value_type,
        }

    def get_parsed_value(self) -> Any:
        """Parse value to appropriate Python type."""
        if self.value_type == 'bool':
            return self.value not in ('0', '(0)', 'false', 'FALSE')
        elif self.value_type == 'int':
            try:
                return int(self.value.strip('()'))
            except ValueError:
                return self.value
        elif self.value_type == 'hex':
            try:
                return int(self.value.strip('()'), 16)
            except ValueError:
                return self.value
        elif self.value_type == 'string':
            return self.value.strip('"').strip("'")
        elif self.value_type == 'array':
            # Parse array like {0x29, 0x08, 0x33, 0xA1, 0x01, 0xD2}
            content = self.value.strip('{}')
            items = [x.strip() for x in content.split(',')]
            try:
                return [int(x, 16) if x.startswith('0x') else int(x) for x in items if x]
            except ValueError:
                return items
        return self.value


@dataclass
class ProjectConfig:
    """
    Complete project configuration.

    Attributes:
        project_name: Name of the project
        file_path: Path to cfg.h file
        system_config: System configuration macros
        ble_config: BLE configuration macros
        profile_config: Profile configuration macros
        debug_config: Debug configuration macros
        other_macros: Other macros not in known categories
        enabled_profiles: List of enabled profile names
    """
    project_name: str
    file_path: str = ""
    system_config: Dict[str, ConfigMacro] = field(default_factory=dict)
    ble_config: Dict[str, ConfigMacro] = field(default_factory=dict)
    profile_config: Dict[str, ConfigMacro] = field(default_factory=dict)
    debug_config: Dict[str, ConfigMacro] = field(default_factory=dict)
    other_macros: Dict[str, ConfigMacro] = field(default_factory=dict)

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "project_name": self.project_name,
            "file_path": self.file_path,
            "system_config": {k: v.to_dict() for k, v in self.system_config.items()},
            "ble_config": {k: v.to_dict() for k, v in self.ble_config.items()},
            "profile_config": {k: v.to_dict() for k, v in self.profile_config.items()},
            "debug_config": {k: v.to_dict() for k, v in self.debug_config.items()},
            "other_macros": {k: v.to_dict() for k, v in self.other_macros.items()},
            "enabled_profiles": self.get_enabled_profiles(),
            "summary": self.get_summary(),
        }

    def get_enabled_profiles(self) -> List[str]:
        """Get list of enabled profiles."""
        enabled = []
        for name, macro in self.profile_config.items():
            if macro.get_parsed_value():
                enabled.append(name.replace('PRF_', '').lower())
        return enabled

    def get_summary(self) -> Dict[str, Any]:
        """Get configuration summary."""
        return {
            "sys_clk": self.system_config.get('SYS_CLK', ConfigMacro('SYS_CLK', '(0)')).get_parsed_value(),
            "ble_connections": {
                "slaves": self.ble_config.get('BLE_NB_SLAVE', ConfigMacro('BLE_NB_SLAVE', '(1)')).get_parsed_value(),
                "masters": self.ble_config.get('BLE_NB_MASTER', ConfigMacro('BLE_NB_MASTER', '(0)')).get_parsed_value(),
            },
            "debug_mode": self.system_config.get('DBG_MODE', ConfigMacro('DBG_MODE', '(0)')).get_parsed_value(),
            "profiles_enabled": len(self.get_enabled_profiles()),
        }


class CFGHeaderParser:
    """
    Parse cfg.h configuration header files.

    Extracts all #define macros and categorizes them.
    """

    def __init__(self):
        """Initialize the parser."""
        # Pattern to match #define statements
        self.define_pattern = re.compile(
            r'^\s*#\s*define\s+([A-Z_][A-Z0-9_]*)\s+(.+?)\s*(?://\s*(.*))?$',
            re.MULTILINE
        )

        # Pattern for inline comments
        self.comment_pattern = re.compile(r'//\s*(.+)$')

    def parse_file(self, cfg_file: str) -> Optional[ProjectConfig]:
        """
        Parse a cfg.h file.

        Args:
            cfg_file: Path to cfg.h file

        Returns:
            ProjectConfig or None
        """
        cfg_path = Path(cfg_file)
        if not cfg_path.exists():
            logger.warning(f"Config file not found: {cfg_file}")
            return None

        project_name = cfg_path.parent.parent.name  # cfg.h is in project/src/

        try:
            with open(cfg_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            logger.error(f"Failed to read {cfg_file}: {e}")
            return None

        config = ProjectConfig(
            project_name=project_name,
            file_path=str(cfg_path)
        )

        # Parse all #define statements
        for i, line in enumerate(content.split('\n'), 1):
            # Skip empty lines and #ifndef/#define guards
            if not line.strip() or line.strip().startswith('#ifndef') or line.strip().startswith('#define _'):
                continue

            match = self.define_pattern.match(line)
            if match:
                name = match.group(1)
                value = match.group(2).strip()
                inline_comment = match.group(3)

                # Determine category
                if name in SYSTEM_CONFIG_MACROS:
                    meta = SYSTEM_CONFIG_MACROS[name]
                    category = 'system'
                    desc = inline_comment or meta.get('description', '')
                    value_type = meta.get('type', 'unknown')
                    config.system_config[name] = ConfigMacro(
                        name=name, value=value, category=category,
                        description=desc, file_path=str(cfg_path),
                        line_number=i, value_type=value_type
                    )
                elif name in BLE_CONFIG_MACROS:
                    meta = BLE_CONFIG_MACROS[name]
                    category = 'ble'
                    desc = inline_comment or meta.get('description', '')
                    value_type = meta.get('type', 'unknown')
                    config.ble_config[name] = ConfigMacro(
                        name=name, value=value, category=category,
                        description=desc, file_path=str(cfg_path),
                        line_number=i, value_type=value_type
                    )
                elif name in PROFILE_CONFIG_MACROS:
                    meta = PROFILE_CONFIG_MACROS[name]
                    category = 'profile'
                    desc = inline_comment or meta.get('description', '')
                    value_type = meta.get('type', 'unknown')
                    config.profile_config[name] = ConfigMacro(
                        name=name, value=value, category=category,
                        description=desc, file_path=str(cfg_path),
                        line_number=i, value_type=value_type
                    )
                elif name in DEBUG_CONFIG_MACROS:
                    meta = DEBUG_CONFIG_MACROS[name]
                    category = 'debug'
                    desc = inline_comment or meta.get('description', '')
                    value_type = meta.get('type', 'unknown')
                    config.debug_config[name] = ConfigMacro(
                        name=name, value=value, category=category,
                        description=desc, file_path=str(cfg_path),
                        line_number=i, value_type=value_type
                    )
                else:
                    # Unknown macro
                    # Try to infer category from name
                    category, value_type = self._infer_category(name)
                    config.other_macros[name] = ConfigMacro(
                        name=name, value=value, category=category,
                        description=inline_comment or '',
                        file_path=str(cfg_path), line_number=i,
                        value_type=value_type
                    )

        logger.info(f"Parsed {cfg_file}: {len(config.system_config)} system, "
                   f"{len(config.ble_config)} BLE, {len(config.profile_config)} profile configs")
        return config

    def parse_project(self, project_path: str) -> Optional[ProjectConfig]:
        """
        Parse cfg.h from a project directory.

        Args:
            project_path: Path to project directory

        Returns:
            ProjectConfig or None
        """
        project_path = Path(project_path)

        # Look for cfg.h in common locations
        cfg_locations = [
            project_path / "src" / "cfg.h",
            project_path / "cfg.h",
            project_path / "config" / "cfg.h",
        ]

        for cfg_path in cfg_locations:
            if cfg_path.exists():
                return self.parse_file(str(cfg_path))

        logger.warning(f"No cfg.h found in {project_path}")
        return None

    def parse_all_projects(
        self,
        projects_dir: str
    ) -> Dict[str, ProjectConfig]:
        """
        Parse cfg.h from all projects in directory.

        Args:
            projects_dir: Path to projects/ directory

        Returns:
            Dict mapping project name to ProjectConfig
        """
        projects_path = Path(projects_dir)
        configs = {}

        for project_dir in sorted(projects_path.iterdir()):
            if project_dir.is_dir() and not project_dir.name.startswith('.'):
                config = self.parse_project(str(project_dir))
                if config:
                    configs[config.project_name] = config

        logger.info(f"Parsed {len(configs)} project configurations from {projects_dir}")
        return configs

    def _infer_category(self, name: str) -> tuple:
        """Infer category and type from macro name."""
        name_upper = name.upper()

        # Type inference
        if name_upper.startswith('PRF_'):
            return 'profile', 'bool'
        if 'CLK' in name_upper or 'CLOCK' in name_upper:
            return 'system', 'enum'
        if 'DBG' in name_upper or 'DEBUG' in name_upper:
            return 'debug', 'bool'
        if name_upper.startswith('BLE_'):
            return 'ble', 'unknown'
        if 'NUM' in name_upper or 'NB_' in name_upper or 'COUNT' in name_upper:
            return 'other', 'int'
        if 'EN' in name_upper or 'ENABLE' in name_upper:
            return 'other', 'bool'

        return 'other', 'unknown'

    def compare_configs(
        self,
        config1: ProjectConfig,
        config2: ProjectConfig
    ) -> Dict[str, Any]:
        """
        Compare two project configurations.

        Args:
            config1: First project config
            config2: Second project config

        Returns:
            Dict with differences
        """
        differences = {
            "project1": config1.project_name,
            "project2": config2.project_name,
            "system_diff": {},
            "ble_diff": {},
            "profile_diff": {},
        }

        # Compare system config
        for key in set(config1.system_config.keys()) | set(config2.system_config.keys()):
            v1 = config1.system_config.get(key, ConfigMacro(key, '')).value
            v2 = config2.system_config.get(key, ConfigMacro(key, '')).value
            if v1 != v2:
                differences["system_diff"][key] = {"project1": v1, "project2": v2}

        # Compare BLE config
        for key in set(config1.ble_config.keys()) | set(config2.ble_config.keys()):
            v1 = config1.ble_config.get(key, ConfigMacro(key, '')).value
            v2 = config2.ble_config.get(key, ConfigMacro(key, '')).value
            if v1 != v2:
                differences["ble_diff"][key] = {"project1": v1, "project2": v2}

        # Compare profile config
        for key in set(config1.profile_config.keys()) | set(config2.profile_config.keys()):
            v1 = config1.profile_config.get(key, ConfigMacro(key, '(0)')).get_parsed_value()
            v2 = config2.profile_config.get(key, ConfigMacro(key, '(0)')).get_parsed_value()
            if v1 != v2:
                differences["profile_diff"][key] = {"project1": v1, "project2": v2}

        return differences


def parse_cfg_file(cfg_file: str) -> Optional[ProjectConfig]:
    """Convenience function to parse a cfg.h file."""
    parser = CFGHeaderParser()
    return parser.parse_file(cfg_file)


def parse_all_projects(projects_dir: str) -> Dict[str, ProjectConfig]:
    """Convenience function to parse all projects."""
    parser = CFGHeaderParser()
    return parser.parse_all_projects(projects_dir)


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python cfg_header_parser.py <cfg.h_or_projects_dir>")
        sys.exit(1)

    path = sys.argv[1]

    parser = CFGHeaderParser()
    path_obj = Path(path)

    if path_obj.is_file():
        config = parser.parse_file(path)
        if config:
            print(f"\nProject: {config.project_name}")
            print(f"Summary: {config.get_summary()}")
            print(f"\nEnabled Profiles: {config.get_enabled_profiles()}")
    else:
        configs = parser.parse_all_projects(path)
        print(f"\nParsed {len(configs)} project configurations:\n")
        for name, cfg in sorted(configs.items()):
            print(f"  {name}:")
            print(f"    {cfg.get_summary()}")
