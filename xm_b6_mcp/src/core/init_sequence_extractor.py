"""
Init Sequence Extractor
=======================

Extract initialization sequences from SDK projects.

Identifies:
- System initialization: iwdt_disable, rcc_ble_en, rcc_adc_en
- BLE initialization: ble_init, app_init, rfmdm_init
- Profile registration: gap_svc_init, diss_svc_init, bass_svc_init, hids_prf_init
- Advertising sequence: gapm_create_advertising, gapm_set_adv_data, gapm_start_advertising

Author: B6x MCP Server Team
Version: 0.1.0
"""

import re
import logging
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple
from dataclasses import dataclass, field
from collections import defaultdict
import json

try:
    from .tree_sitter_parser import TreeSitterCParser
except ImportError:
    TreeSitterCParser = None

logger = logging.getLogger(__name__)


# ============================================================================
# Known Initialization Patterns
# ============================================================================

# System-level initialization (called first)
SYSTEM_INIT_APIS = {
    'iwdt_disable', 'iwdt_init', 'wdt_disable', 'wdt_init',
    'rcc_ble_en', 'rcc_adc_en', 'rcc_fshclk_set',
    'clk_init', 'sys_clk_init', 'clock_config',
    'rstrsn', 'reset_reason_get',
}

# Peripheral initialization (called after system)
PERIPHERAL_INIT_APIS = {
    'uart_init', 'uart1Rb_Init', 'uart1_init', 'uart2_init',
    'spi_init', 'i2c_init', 'gpio_init',
    'sftmr_init', 'timer_init', 'rtc_init',
    'adc_init', 'sadc_init', 'dma_init',
    'leds_init', 'keybd_init', 'batt_init',
}

# BLE core initialization
BLE_CORE_INIT_APIS = {
    'app_init', 'ble_init', 'ble_stack_init',
    'rfmdm_init', 'rf_init', 'phy_init',
    'ke_init', 'kernel_init',
}

# GATT Profile initialization
PROFILE_INIT_APIS = {
    'gap_svc_init', 'gatt_svc_init',
    'diss_svc_init', 'diss_init',      # Device Information Service
    'bass_svc_init', 'bass_init',      # Battery Service
    'hids_prf_init', 'hids_init',      # HID Service
    'otas_svc_init', 'otas_init',      # OTA Service
    'hrs_prf_init', 'hrs_init',        # Heart Rate Service
    'scps_svc_init', 'scps_init',      # Scan Parameters Service
    'user_prf_init',                    # User-defined Profile
}

# Advertising/Connection initialization
ADVERTISING_INIT_APIS = {
    'gapm_create_advertising', 'gapm_start_advertising',
    'gapm_set_adv_data', 'gapm_set_scan_rsp_data',
    'gapc_connection_cfm', 'gapc_param_update_cmd',
    'app_adv_start', 'app_adv_create',
}

# Mesh initialization
MESH_INIT_APIS = {
    'mesh_init', 'mesh_stack_init',
    'mesh_model_init', 'mesh_node_init',
    'mesh_prov_init', 'mesh_net_init',
}

# Category mapping
CATEGORY_MAP = {
    'system': SYSTEM_INIT_APIS,
    'peripheral': PERIPHERAL_INIT_APIS,
    'ble_core': BLE_CORE_INIT_APIS,
    'profile': PROFILE_INIT_APIS,
    'advertising': ADVERTISING_INIT_APIS,
    'mesh': MESH_INIT_APIS,
}


@dataclass
class InitSequenceStep:
    """
    A single step in an initialization sequence.

    Attributes:
        api_name: Name of the API function called
        category: Category of initialization (system, peripheral, ble_core, etc.)
        order: Order in the sequence (0-based)
        file_path: Source file where this call appears
        line_number: Line number in source
        calling_function: Function that contains this call
        is_conditional: Whether this call is inside a conditional (#if, if)
        context: Surrounding code context
    """
    api_name: str
    category: str = "unknown"
    order: int = 0
    file_path: str = ""
    line_number: int = 0
    calling_function: str = ""
    is_conditional: bool = False
    context: str = ""

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "api_name": self.api_name,
            "category": self.category,
            "order": self.order,
            "file_path": self.file_path,
            "line_number": self.line_number,
            "calling_function": self.calling_function,
            "is_conditional": self.is_conditional,
            "context": self.context,
        }


@dataclass
class InitSequence:
    """
    Complete initialization sequence for a project or feature.

    Attributes:
        id: Unique identifier (e.g., "ble_hid_device_init")
        name: Human-readable name
        category: Main category (system, peripheral, ble, profile, mesh)
        sequence: Ordered list of InitSequenceStep
        source_project: Project where this sequence was extracted
        prerequisites: List of required configs/macros
        description: Brief description of what this sequence initializes
    """
    id: str
    name: str
    category: str
    sequence: List[InitSequenceStep] = field(default_factory=list)
    source_project: str = ""
    prerequisites: List[str] = field(default_factory=list)
    description: str = ""

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "id": self.id,
            "name": self.name,
            "category": self.category,
            "sequence": [s.to_dict() for s in self.sequence],
            "source_project": self.source_project,
            "prerequisites": self.prerequisites,
            "description": self.description,
        }


class InitSequenceExtractor:
    """
    Extract initialization sequences from SDK projects.

    Analyzes main.c and other entry points to identify:
    - System initialization order
    - BLE stack initialization order
    - Profile registration order
    - Advertising start sequence
    """

    def __init__(self, parser: Optional['TreeSitterCParser'] = None):
        """
        Initialize the extractor.

        Args:
            parser: Optional TreeSitterCParser instance
        """
        self.parser = parser
        if not self.parser and TreeSitterCParser:
            self.parser = TreeSitterCParser()
            self.parser.initialize()

        # Known SDK APIs (populated from drivers/ble domains)
        self.known_apis: Set[str] = set()
        for apis in CATEGORY_MAP.values():
            self.known_apis.update(apis)

    def set_known_apis(self, apis: Set[str]) -> None:
        """Set known SDK API names for better detection."""
        self.known_apis.update(apis)

    def extract_from_file(
        self,
        source_file: str,
        project_name: str = ""
    ) -> Optional[InitSequence]:
        """
        Extract initialization sequence from a source file.

        Args:
            source_file: Path to source file (typically main.c)
            project_name: Project name for identification

        Returns:
            InitSequence or None
        """
        if not self.parser or not self.parser._initialized:
            logger.warning("Parser not initialized")
            return None

        source_path = Path(source_file)
        if not source_path.exists():
            logger.warning(f"File not found: {source_file}")
            return None

        # Parse file
        result = self.parser.parse_example_file(
            source_file,
            self.known_apis,
            'project',
            extract_context=True,
            context_lines=3
        )

        if not result:
            return None

        # Extract init sequence
        steps = []
        order = 0

        # Look for init functions (sysInit, devInit, etc.)
        init_functions = self._find_init_functions(result)

        for func_name in init_functions:
            # Get calls within this init function
            func_calls = [
                c for c in result.function_calls
                if c.calling_function == func_name and c.is_sdk_api
            ]

            # Sort by line number
            func_calls.sort(key=lambda c: c.line_number)

            for call in func_calls:
                category = self._categorize_api(call.function_name)

                step = InitSequenceStep(
                    api_name=call.function_name,
                    category=category,
                    order=order,
                    file_path=str(source_file),
                    line_number=call.line_number,
                    calling_function=func_name,
                    is_conditional=self._is_conditional(call.context_snippet),
                    context=call.context_snippet[:200] if call.context_snippet else ""
                )
                steps.append(step)
                order += 1

        # Also check main() for direct init calls
        main_calls = [
            c for c in result.function_calls
            if c.calling_function == 'main' and c.is_sdk_api
        ]
        main_calls.sort(key=lambda c: c.line_number)

        for call in main_calls:
            # Skip ble_schedule and main loop APIs
            if 'schedule' in call.function_name.lower():
                continue

            category = self._categorize_api(call.function_name)

            step = InitSequenceStep(
                api_name=call.function_name,
                category=category,
                order=order,
                file_path=str(source_file),
                line_number=call.line_number,
                calling_function='main',
                is_conditional=self._is_conditional(call.context_snippet),
                context=call.context_snippet[:200] if call.context_snippet else ""
            )
            steps.append(step)
            order += 1

        if not steps:
            return None

        # Determine main category
        main_category = self._determine_main_category(steps)

        # Generate ID
        seq_id = f"{project_name or source_path.stem}_init"
        seq_id = seq_id.replace('-', '_').replace(' ', '_').lower()

        return InitSequence(
            id=seq_id,
            name=f"{project_name or source_path.stem} Initialization",
            category=main_category,
            sequence=steps,
            source_project=project_name,
            prerequisites=[],
            description=self._generate_description(steps)
        )

    def extract_from_project(
        self,
        project_path: str,
        known_sdk_apis: Optional[Set[str]] = None
    ) -> List[InitSequence]:
        """
        Extract all initialization sequences from a project.

        Args:
            project_path: Path to project directory
            known_sdk_apis: Set of known SDK API names

        Returns:
            List of InitSequence objects
        """
        if known_sdk_apis:
            self.set_known_apis(known_sdk_apis)

        project_path = Path(project_path)
        project_name = project_path.name
        sequences = []

        # Look for main.c
        main_files = list(project_path.rglob("main.c"))
        for main_file in main_files:
            seq = self.extract_from_file(str(main_file), project_name)
            if seq:
                sequences.append(seq)

        # Look for app.c (BLE app init)
        app_files = list(project_path.rglob("app.c"))
        for app_file in app_files:
            seq = self._extract_ble_app_init(str(app_file), project_name)
            if seq:
                sequences.append(seq)

        return sequences

    def extract_all_projects(
        self,
        projects_dir: str,
        known_sdk_apis: Optional[Set[str]] = None
    ) -> List[InitSequence]:
        """
        Extract init sequences from all projects in directory.

        Args:
            projects_dir: Path to projects/ directory
            known_sdk_apis: Set of known SDK API names

        Returns:
            List of all InitSequence objects
        """
        if known_sdk_apis:
            self.set_known_apis(known_sdk_apis)

        projects_path = Path(projects_dir)
        all_sequences = []

        for project_dir in sorted(projects_path.iterdir()):
            if project_dir.is_dir() and not project_dir.name.startswith('.'):
                sequences = self.extract_from_project(str(project_dir))
                all_sequences.extend(sequences)

        logger.info(f"Extracted {len(all_sequences)} init sequences from {projects_dir}")
        return all_sequences

    def _find_init_functions(self, parse_result) -> List[str]:
        """Find initialization functions in parse result."""
        init_funcs = []

        # Common init function names
        init_patterns = ['init', 'Init', 'setup', 'Setup', 'begin', 'Begin']

        for func_name in parse_result.functions_defined:
            if any(p in func_name for p in init_patterns):
                init_funcs.append(func_name)

        # Always include main
        if 'main' not in init_funcs:
            init_funcs.append('main')

        return init_funcs

    def _categorize_api(self, api_name: str) -> str:
        """Determine category of an API."""
        for category, apis in CATEGORY_MAP.items():
            if api_name in apis:
                return category

        # Pattern matching
        api_lower = api_name.lower()
        if any(p in api_lower for p in ['iwdt', 'wdt', 'rcc', 'clk', 'sys']):
            return 'system'
        if any(p in api_lower for p in ['uart', 'spi', 'i2c', 'gpio', 'adc', 'dma', 'timer']):
            return 'peripheral'
        if any(p in api_lower for p in ['ble', 'gap', 'app_', 'ke_']):
            return 'ble_core'
        if any(p in api_lower for p in ['_svc_', '_prf_', 'diss', 'bass', 'hids', 'otas']):
            return 'profile'
        if any(p in api_lower for p in ['adv', 'mesh']):
            return api_lower.split('_')[0]

        return 'unknown'

    def _determine_main_category(self, steps: List[InitSequenceStep]) -> str:
        """Determine main category of sequence based on steps."""
        if not steps:
            return 'unknown'

        category_counts = defaultdict(int)
        for step in steps:
            category_counts[step.category] += 1

        # Priority order
        priority = ['system', 'peripheral', 'ble_core', 'profile', 'advertising', 'mesh']

        for cat in priority:
            if category_counts.get(cat, 0) > 0:
                return cat

        return 'unknown'

    def _is_conditional(self, context: str) -> bool:
        """Check if code is inside a conditional block."""
        if not context:
            return False

        patterns = ['#if', '#ifdef', '#ifndef', 'if (', 'if(']
        return any(p in context for p in patterns)

    def _extract_ble_app_init(
        self,
        app_file: str,
        project_name: str
    ) -> Optional[InitSequence]:
        """Extract BLE app_init sequence."""
        if not self.parser or not self.parser._initialized:
            return None

        result = self.parser.parse_example_file(
            app_file,
            self.known_apis,
            'project',
            extract_context=False
        )

        if not result:
            return None

        # Look for app_init function calls
        app_init_calls = [
            c for c in result.function_calls
            if c.calling_function == 'app_init' and c.is_sdk_api
        ]

        if not app_init_calls:
            return None

        steps = []
        for i, call in enumerate(sorted(app_init_calls, key=lambda c: c.line_number)):
            steps.append(InitSequenceStep(
                api_name=call.function_name,
                category=self._categorize_api(call.function_name),
                order=i,
                file_path=str(app_file),
                line_number=call.line_number,
                calling_function='app_init',
            ))

        return InitSequence(
            id=f"{project_name}_ble_app_init",
            name=f"{project_name} BLE App Initialization",
            category='ble_core',
            sequence=steps,
            source_project=project_name,
            description="BLE application initialization sequence"
        )

    def _generate_description(self, steps: List[InitSequenceStep]) -> str:
        """Generate description from steps."""
        categories = set(s.category for s in steps)
        apis = [s.api_name for s in steps[:5]]

        desc = f"Initialization sequence with {len(steps)} steps"
        if categories:
            desc += f" covering: {', '.join(sorted(categories))}"
        if apis:
            desc += f". Starts with: {' -> '.join(apis)}..."

        return desc


def extract_init_sequences(
    projects_dir: str,
    known_apis: Optional[Set[str]] = None
) -> List[InitSequence]:
    """Convenience function to extract init sequences."""
    extractor = InitSequenceExtractor()
    return extractor.extract_all_projects(projects_dir, known_apis)


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python init_sequence_extractor.py <projects_dir>")
        sys.exit(1)

    sequences = extract_init_sequences(sys.argv[1])

    print(f"\nExtracted {len(sequences)} initialization sequences:\n")
    for seq in sequences:
        print(f"  [{seq.category}] {seq.name}")
        for step in seq.sequence[:5]:
            print(f"    {step.order}. {step.api_name} ({step.category})")
        if len(seq.sequence) > 5:
            print(f"    ... and {len(seq.sequence) - 5} more steps")
