"""
Call Chain Extractor
====================

Extract call chains from example/project source code.

Identifies:
- Entry point (main)
- Initialization sequence
- Main API usage
- Peripheral usage patterns
- Cross-file call chains
- SDK API vs user function distinction
- Complete call depth extraction

Author: B6x MCP Server Team
Version: 0.2.0
"""

import re
import logging
import json
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple, Any
from collections import Counter, defaultdict
from dataclasses import dataclass, field

try:
    from .tree_sitter_parser import TreeSitterCParser, FunctionCall, FunctionCallWithContext
except ImportError:
    # Allow standalone usage
    TreeSitterCParser = None
    FunctionCall = None
    FunctionCallWithContext = None

logger = logging.getLogger(__name__)


@dataclass
class CallChain:
    """
    Function call chain information.

    Attributes:
        entry_point: Entry function name (usually "main")
        init_sequence: Ordered list of initialization APIs
        main_loop_apis: APIs called in main loop
        peripheral_apis: APIs organized by peripheral
        call_graph: Adjacency list of call relationships
        api_call_counts: Frequency of each API call
    """
    entry_point: str
    init_sequence: List[str] = field(default_factory=list)
    main_loop_apis: List[str] = field(default_factory=list)
    peripheral_apis: Dict[str, List[str]] = field(default_factory=dict)
    call_graph: Dict[str, Set[str]] = field(default_factory=dict)
    api_call_counts: Dict[str, int] = field(default_factory=dict)


@dataclass
class ProjectCallChain:
    """
    Complete call chain analysis for a project.

    Attributes:
        project_name: Name of the project
        entry_point: Entry function name
        init_sequence: Initialization API sequence
        main_apis_used: Set of all APIs used
        peripheral_usage: Count of API usage by peripheral
        call_depths: Maximum call depth for each function
        library_dependencies: External libraries used
    """
    project_name: str
    entry_point: str = "main"
    init_sequence: List[str] = field(default_factory=list)
    main_apis_used: Set[str] = field(default_factory=set)
    peripheral_usage: Dict[str, int] = field(default_factory=dict)
    call_depths: Dict[str, int] = field(default_factory=dict)
    library_dependencies: List[str] = field(default_factory=list)


# ============================================================================
# Enhanced Data Structures (v0.2.0)
# ============================================================================

@dataclass
class CallChainNode:
    """
    A node in the call chain graph.

    Attributes:
        name: Function/API name
        is_sdk_api: Whether this is an SDK API
        is_user_func: Whether this is a user-defined function
        file_path: Source file where defined/called
        line_number: Line number
        category: Category (system, ble, peripheral, user)
        calls: List of functions this node calls
        called_by: List of functions that call this node
    """
    name: str
    is_sdk_api: bool = False
    is_user_func: bool = False
    file_path: str = ""
    line_number: int = 0
    category: str = "unknown"
    calls: List[str] = field(default_factory=list)
    called_by: List[str] = field(default_factory=list)

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "name": self.name,
            "is_sdk_api": self.is_sdk_api,
            "is_user_func": self.is_user_func,
            "file_path": self.file_path,
            "line_number": self.line_number,
            "category": self.category,
            "calls": self.calls,
            "called_by": self.called_by,
        }


@dataclass
class CrossFileCallChain:
    """
    Call chain that spans multiple files.

    Attributes:
        id: Unique identifier
        entry_point: Entry function name
        project_name: Source project name
        nodes: Dict of function name to CallChainNode
        edges: List of (caller, callee) tuples
        sdk_apis_used: Set of SDK APIs used
        user_funcs_used: Set of user functions used
        max_depth: Maximum call depth
        file_count: Number of source files analyzed
    """
    id: str
    entry_point: str = "main"
    project_name: str = ""
    nodes: Dict[str, CallChainNode] = field(default_factory=dict)
    edges: List[Tuple[str, str]] = field(default_factory=list)
    sdk_apis_used: Set[str] = field(default_factory=set)
    user_funcs_used: Set[str] = field(default_factory=set)
    max_depth: int = 0
    file_count: int = 0

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "id": self.id,
            "entry_point": self.entry_point,
            "project_name": self.project_name,
            "nodes": {k: v.to_dict() for k, v in self.nodes.items()},
            "edges": list(self.edges),
            "sdk_apis_used": list(self.sdk_apis_used),
            "user_funcs_used": list(self.user_funcs_used),
            "max_depth": self.max_depth,
            "file_count": self.file_count,
            "summary": self.get_summary(),
        }

    def get_summary(self) -> Dict[str, Any]:
        """Get summary statistics."""
        return {
            "total_nodes": len(self.nodes),
            "total_edges": len(self.edges),
            "sdk_api_count": len(self.sdk_apis_used),
            "user_func_count": len(self.user_funcs_used),
            "max_depth": self.max_depth,
        }

    def get_call_path(self, from_func: str, to_func: str) -> Optional[List[str]]:
        """
        Find call path between two functions.

        Args:
            from_func: Starting function name
            to_func: Target function name

        Returns:
            List of function names forming the path, or None
        """
        if from_func not in self.nodes or to_func not in self.nodes:
            return None

        # BFS to find shortest path
        visited = set()
        queue = [(from_func, [from_func])]

        while queue:
            current, path = queue.pop(0)

            if current == to_func:
                return path

            if current in visited:
                continue
            visited.add(current)

            node = self.nodes.get(current)
            if node:
                for callee in node.calls:
                    if callee not in visited:
                        queue.append((callee, path + [callee]))

        return None


@dataclass
class CallChainPattern:
    """
    A recognized call chain pattern (e.g., HID report sending).

    Attributes:
        name: Pattern name
        description: Pattern description
        entry_function: Entry function for this pattern
        sequence: Ordered list of function calls
        category: Category (hid, battery, advertising, etc.)
        source_project: Where this pattern was found
    """
    name: str
    description: str = ""
    entry_function: str = ""
    sequence: List[str] = field(default_factory=list)
    category: str = "unknown"
    source_project: str = ""

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "name": self.name,
            "description": self.description,
            "entry_function": self.entry_function,
            "sequence": self.sequence,
            "category": self.category,
            "source_project": self.source_project,
        }


# Known call chain patterns
KNOWN_PATTERNS = {
    'hid_report_send': CallChainPattern(
        name='HID Report Send',
        description='Send HID report to connected device',
        entry_function='user_procedure',
        sequence=['keybd_scan', 'keybd_report_send', 'hids_report_send', 'notification_send'],
        category='hid',
    ),
    'battery_update': CallChainPattern(
        name='Battery Level Update',
        description='Update battery level and notify',
        entry_function='batt_proc',
        sequence=['batt_adc_read', 'bass_bat_lvl_update', 'notification_send'],
        category='battery',
    ),
    'advertising_start': CallChainPattern(
        name='Advertising Start',
        description='Start BLE advertising',
        entry_function='app_adv_start',
        sequence=['gapm_create_advertising', 'gapm_set_adv_data', 'gapm_start_advertising'],
        category='advertising',
    ),
    'connection_handle': CallChainPattern(
        name='Connection Handle',
        description='Handle new BLE connection',
        entry_function='gapc_connection_req_ind',
        sequence=['gapc_connection_cfm', 'gapc_param_update_cmd'],
        category='connection',
    ),
}


class CallChainExtractor:
    """
    Extract call chains from source code.

    Uses Tree-sitter parser to analyze:
    - Initialization sequence (APIs called before main loop)
    - Main loop APIs (APIs called in while(1) loop)
    - Peripheral usage patterns
    - Call graph
    """

    def __init__(self, parser: Optional['TreeSitterCParser'] = None):
        """
        Initialize the call chain extractor.

        Args:
            parser: TreeSitterCParser instance (optional, will create if not provided)
        """
        self.parser = parser
        if not self.parser:
            self.parser = TreeSitterCParser()
            self.parser.initialize()

    def extract_init_sequence(
        self,
        source_file: str,
        known_sdk_apis: Optional[Set[str]] = None
    ) -> List[str]:
        """
        Extract initialization sequence from a source file.

        Looks for:
        - Clock configuration APIs
        - Peripheral initialization APIs
        - APIs called before main loop

        Args:
            source_file: Path to source file
            known_sdk_apis: Set of known SDK API names

        Returns:
            Ordered list of initialization API names
        """
        if known_sdk_apis is None:
            known_sdk_apis = set()

        result = self.parser.parse_example_file(
            source_file,
            known_sdk_apis,
            'project',
            extract_context=False
        )

        if not result:
            return []

        init_apis = []
        in_main_loop = False

        for call in result.function_calls:
            if not call.is_sdk_api:
                continue

            # Detect main loop start
            if call.calling_function == 'main':
                # Check if we're in a while(1) loop
                # (simplified: assume after certain point is main loop)
                if 'while' in call.context_snippet.lower() or call.line_number > 100:
                    in_main_loop = True
                    break

                # Add to init sequence if it's an init API
                if self._is_init_api(call.function_name):
                    init_apis.append(call.function_name)

        return init_apis

    def analyze_peripheral_usage(
        self,
        source_files: List[str],
        known_sdk_apis: Optional[Set[str]] = None
    ) -> Dict[str, int]:
        """
        Analyze peripheral API usage frequency.

        Args:
            source_files: List of source file paths
            known_sdk_apis: Set of known SDK API names

        Returns:
            Dictionary mapping peripheral names to usage count
        """
        if known_sdk_apis is None:
            known_sdk_apis = set()

        peripheral_counter = Counter()

        for source_file in source_files:
            result = self.parser.parse_example_file(
                source_file,
                known_sdk_apis,
                'project',
                extract_context=False
            )

            if not result:
                continue

            for call in result.function_calls:
                if call.is_sdk_api and call.peripheral:
                    peripheral_counter[call.peripheral] += 1

        return dict(peripheral_counter)

    def extract_call_chain(
        self,
        source_file: str,
        known_sdk_apis: Optional[Set[str]] = None
    ) -> CallChain:
        """
        Extract complete call chain from a source file.

        Args:
            source_file: Path to source file
            known_sdk_apis: Set of known SDK API names

        Returns:
            CallChain object
        """
        if known_sdk_apis is None:
            known_sdk_apis = set()

        result = self.parser.parse_example_file(
            source_file,
            known_sdk_apis,
            'project',
            extract_context=False
        )

        if not result:
            return CallChain(entry_point="unknown")

        call_chain = CallChain(entry_point="main")

        # Build call graph
        call_graph = defaultdict(set)
        for call in result.function_calls:
            call_graph[call.calling_function].add(call.function_name)

        # Count API calls
        api_counts = Counter()
        for call in result.function_calls:
            if call.is_sdk_api:
                api_counts[call.function_name] += 1

        # Extract init sequence (before main loop)
        init_apis = []
        main_loop_apis = []
        in_main_loop = False

        # Sort by line number
        sorted_calls = sorted(result.function_calls, key=lambda c: c.line_number)

        for call in sorted_calls:
            if not call.is_sdk_api:
                continue

            # Detect main loop (simplified heuristic)
            if call.calling_function == 'main' and not in_main_loop:
                # Check for loop indicators in context
                if 'while' in call.context_snippet.lower() or 'for' in call.context_snippet.lower():
                    in_main_loop = True

            if in_main_loop and call.calling_function == 'main':
                main_loop_apis.append(call.function_name)
            elif call.calling_function == 'main' and self._is_init_api(call.function_name):
                init_apis.append(call.function_name)

        # Group APIs by peripheral
        peripheral_apis = defaultdict(list)
        for call in result.function_calls:
            if call.is_sdk_api and call.peripheral:
                peripheral_apis[call.peripheral].append(call.function_name)

        call_chain.init_sequence = init_apis
        call_chain.main_loop_apis = main_loop_apis
        call_chain.peripheral_apis = dict(peripheral_apis)
        call_chain.call_graph = dict(call_graph)
        call_chain.api_call_counts = dict(api_counts)

        return call_chain

    def analyze_project(
        self,
        project_path: str,
        source_files: List[str],
        main_file: str,
        known_sdk_apis: Set[str]
    ) -> ProjectCallChain:
        """
        Analyze complete project call chains.

        Args:
            project_path: Path to project directory
            source_files: List of all source files
            main_file: Path to main.c file
            known_sdk_apis: Set of known SDK API names

        Returns:
            ProjectCallChain object
        """
        project_name = Path(project_path).name

        # Extract initialization sequence from main
        init_sequence = self.extract_init_sequence(main_file, known_sdk_apis)

        # Analyze peripheral usage across all files
        peripheral_usage = self.analyze_peripheral_usage(source_files, known_sdk_apis)

        # Collect all APIs used
        all_apis = set()
        for source_file in source_files:
            result = self.parser.parse_example_file(
                source_file,
                known_sdk_apis,
                'project',
                extract_context=False
            )
            if result:
                for call in result.function_calls:
                    if call.is_sdk_api:
                        all_apis.add(call.function_name)

        # Extract call chain from main
        call_chain = self.extract_call_chain(main_file, known_sdk_apis)

        # Calculate call depths
        call_depths = {}
        for func, called_funcs in call_chain.call_graph.items():
            if func != 'main':
                call_depths[func] = self._calculate_function_depth(
                    func, call_chain.call_graph
                )

        return ProjectCallChain(
            project_name=project_name,
            entry_point=call_chain.entry_point,
            init_sequence=init_sequence,
            main_apis_used=all_apis,
            peripheral_usage=peripheral_usage,
            call_depths=call_depths,
            library_dependencies=[]  # Would need to parse project config
        )

    def extract_from_file(
        self,
        source_file: str,
        known_sdk_apis: Optional[Set[str]] = None
    ) -> List[Any]:
        """
        Extract call chains from a source file.

        This is a convenience method that returns a list of extracted call chains.
        For a single file, returns a list with one CallChain object.

        Args:
            source_file: Path to source file
            known_sdk_apis: Set of known SDK API names

        Returns:
            List of CallChain objects (may be empty)
        """
        if known_sdk_apis is None:
            known_sdk_apis = set()

        try:
            chain = self.extract_call_chain(source_file, known_sdk_apis)
            if chain and chain.entry_point != "unknown":
                return [chain]
        except Exception as e:
            logger.warning(f"Failed to extract call chain from {source_file}: {e}")

        return []

    def _is_init_api(self, api_name: str) -> bool:
        """Check if an API is an initialization API."""
        init_keywords = [
            'Init', 'Config', 'Enable', 'Setup', 'Open',
            'Clock', 'RCC', 'Power', 'Reset'
        ]
        api_upper = api_name.upper()
        return any(kw.upper() in api_upper for kw in init_keywords)

    def _calculate_function_depth(
        self,
        func_name: str,
        call_graph: Dict[str, Set[str]],
        visited: Optional[Set[str]] = None
    ) -> int:
        """Calculate maximum call depth for a function."""
        if visited is None:
            visited = set()

        if func_name in visited:
            return 0  # Circular reference

        visited.add(func_name)

        max_depth = 0
        for called_func in call_graph.get(func_name, set()):
            if called_func.startswith('_'):  # Skip internal functions
                continue
            depth = self._calculate_function_depth(called_func, call_graph, visited)
            max_depth = max(max_depth, depth + 1)

        visited.remove(func_name)
        return max_depth

    def suggest_init_order(self, init_sequence: List[str]) -> List[str]:
        """
        Suggest proper initialization order based on dependencies.

        Args:
            init_sequence: Current initialization sequence

        Returns:
            Reordered init sequence
        """
        # Define dependency groups (higher number = later in sequence)
        priority_groups = {
            1: ['System', 'Core', 'CLOCK', 'RCC'],
            2: ['Power', 'LDO', 'Reset'],
            3: ['GPIO', 'Pin'],
            4: ['DMA', 'IRQ', 'NVIC'],
            5: ['UART', 'SPI', 'I2C', 'I2S'],
            6: ['ADC', 'DAC', 'Timer'],
            7: ['BLE', 'Radio'],
        }

        def get_priority(api: str):
            """Get priority for an API."""
            for priority, keywords in priority_groups.items():
                if any(kw in api for kw in keywords):
                    return priority
            return 4  # Default priority

        # Sort by priority
        return sorted(init_sequence, key=get_priority)

    # ========================================================================
    # Enhanced Methods (v0.2.0) - Cross-file Call Chain Tracking
    # ========================================================================

    def extract_cross_file_call_chain(
        self,
        project_path: str,
        known_sdk_apis: Optional[Set[str]] = None
    ) -> Optional[CrossFileCallChain]:
        """
        Extract call chain that spans multiple files.

        Args:
            project_path: Path to project directory
            known_sdk_apis: Set of known SDK API names

        Returns:
            CrossFileCallChain object or None
        """
        if known_sdk_apis is None:
            known_sdk_apis = set()

        project_path = Path(project_path)
        project_name = project_path.name

        # Find all source files
        source_files = list(project_path.rglob("*.c"))
        if not source_files:
            logger.warning(f"No source files found in {project_path}")
            return None

        # Create cross-file call chain
        chain_id = f"{project_name}_call_chain"
        call_chain = CrossFileCallChain(
            id=chain_id,
            entry_point="main",
            project_name=project_name,
            file_count=len(source_files)
        )

        # Parse each source file and build the call graph
        for source_file in source_files:
            self._parse_file_for_call_chain(
                str(source_file),
                known_sdk_apis,
                call_chain
            )

        # Calculate max depth
        call_chain.max_depth = self._calculate_max_depth(call_chain)

        # Categorize nodes
        self._categorize_nodes(call_chain, known_sdk_apis)

        logger.info(f"Extracted call chain from {project_name}: "
                   f"{len(call_chain.nodes)} nodes, {len(call_chain.edges)} edges, "
                   f"depth={call_chain.max_depth}")

        return call_chain

    def _parse_file_for_call_chain(
        self,
        source_file: str,
        known_sdk_apis: Set[str],
        call_chain: CrossFileCallChain
    ) -> None:
        """Parse a single file and add to call chain."""
        if not self.parser or not self.parser._initialized:
            return

        result = self.parser.parse_example_file(
            source_file,
            known_sdk_apis,
            'project',
            extract_context=False
        )

        if not result:
            return

        # Add functions defined in this file
        for func_name in result.functions_defined:
            if func_name not in call_chain.nodes:
                is_sdk = func_name in known_sdk_apis
                call_chain.nodes[func_name] = CallChainNode(
                    name=func_name,
                    is_sdk_api=is_sdk,
                    is_user_func=not is_sdk,
                    file_path=source_file,
                    category='sdk_api' if is_sdk else 'user_func'
                )

        # Process function calls
        for call in result.function_calls:
            caller = call.calling_function
            callee = call.function_name

            # Skip if no caller (global scope)
            if not caller:
                continue

            # Ensure caller node exists
            if caller not in call_chain.nodes:
                call_chain.nodes[caller] = CallChainNode(
                    name=caller,
                    is_user_func=True,
                    file_path=source_file,
                    category='user_func'
                )

            # Ensure callee node exists
            if callee not in call_chain.nodes:
                is_sdk = call.is_sdk_api or callee in known_sdk_apis
                call_chain.nodes[callee] = CallChainNode(
                    name=callee,
                    is_sdk_api=is_sdk,
                    is_user_func=not is_sdk,
                    file_path=source_file,
                    category=self._get_api_category(callee)
                )

            # Add edge
            edge = (caller, callee)
            if edge not in call_chain.edges:
                call_chain.edges.append(edge)

            # Update node relationships
            if callee not in call_chain.nodes[caller].calls:
                call_chain.nodes[caller].calls.append(callee)
            if caller not in call_chain.nodes[callee].called_by:
                call_chain.nodes[callee].called_by.append(caller)

            # Track SDK APIs
            if call.is_sdk_api or callee in known_sdk_apis:
                call_chain.sdk_apis_used.add(callee)
            else:
                call_chain.user_funcs_used.add(callee)

    def _calculate_max_depth(self, call_chain: CrossFileCallChain) -> int:
        """Calculate maximum call depth in chain."""
        if 'main' not in call_chain.nodes:
            return 0

        visited = set()
        memo = {}

        def depth(node_name: str) -> int:
            if node_name in memo:
                return memo[node_name]
            if node_name in visited:
                return 0  # Circular

            visited.add(node_name)
            node = call_chain.nodes.get(node_name)
            if not node or not node.calls:
                memo[node_name] = 0
                return 0

            max_child = max((depth(c) + 1 for c in node.calls), default=0)
            memo[node_name] = max_child
            return max_child

        return depth('main')

    def _categorize_nodes(
        self,
        call_chain: CrossFileCallChain,
        known_sdk_apis: Set[str]
    ) -> None:
        """Categorize all nodes in the call chain."""
        for name, node in call_chain.nodes.items():
            if node.category == 'unknown':
                node.category = self._get_api_category(name)
            node.is_sdk_api = name in known_sdk_apis or node.is_sdk_api
            node.is_user_func = not node.is_sdk_api

    def _get_api_category(self, api_name: str) -> str:
        """Determine category of an API."""
        api_lower = api_name.lower()

        # System/core
        if any(p in api_lower for p in ['iwdt', 'wdt', 'rcc', 'clk', 'sys', 'nvic', 'scb']):
            return 'system'

        # Peripherals
        if any(p in api_lower for p in ['uart', 'usart']):
            return 'uart'
        if any(p in api_lower for p in ['spi']):
            return 'spi'
        if any(p in api_lower for p in ['i2c']):
            return 'i2c'
        if any(p in api_lower for p in ['gpio']):
            return 'gpio'
        if any(p in api_lower for p in ['adc', 'sadc']):
            return 'adc'
        if any(p in api_lower for p in ['dma']):
            return 'dma'
        if any(p in api_lower for p in ['timer', 'tmr']):
            return 'timer'
        if any(p in api_lower for p in ['rtc']):
            return 'rtc'
        if any(p in api_lower for p in ['pwm']):
            return 'pwm'

        # BLE
        if any(p in api_lower for p in ['ble_', 'gap', 'gatt', 'att', 'l2cap', 'smp']):
            return 'ble'
        if any(p in api_lower for p in ['ke_', 'kernel']):
            return 'kernel'

        # Profiles
        if any(p in api_lower for p in ['hids', 'hid_']):
            return 'hid'
        if any(p in api_lower for p in ['bass', 'bat_']):
            return 'battery'
        if any(p in api_lower for p in ['diss', 'dev_info']):
            return 'device_info'
        if any(p in api_lower for p in ['otas', 'ota_']):
            return 'ota'

        return 'unknown'

    def extract_all_project_chains(
        self,
        projects_dir: str,
        known_sdk_apis: Optional[Set[str]] = None
    ) -> Dict[str, CrossFileCallChain]:
        """
        Extract call chains from all projects in directory.

        Args:
            projects_dir: Path to projects/ directory
            known_sdk_apis: Set of known SDK API names

        Returns:
            Dict mapping project name to CrossFileCallChain
        """
        if known_sdk_apis is None:
            known_sdk_apis = set()

        projects_path = Path(projects_dir)
        chains = {}

        for project_dir in sorted(projects_path.iterdir()):
            if project_dir.is_dir() and not project_dir.name.startswith('.'):
                chain = self.extract_cross_file_call_chain(
                    str(project_dir),
                    known_sdk_apis
                )
                if chain:
                    chains[project_dir.name] = chain

        logger.info(f"Extracted {len(chains)} project call chains from {projects_dir}")
        return chains

    def find_patterns_in_chain(
        self,
        call_chain: CrossFileCallChain
    ) -> List[CallChainPattern]:
        """
        Find known patterns in a call chain.

        Args:
            call_chain: CrossFileCallChain to analyze

        Returns:
            List of matched CallChainPattern objects
        """
        found_patterns = []

        for pattern_id, pattern in KNOWN_PATTERNS.items():
            # Check if entry function exists
            if pattern.entry_function not in call_chain.nodes:
                continue

            # Check if sequence can be found
            node = call_chain.nodes.get(pattern.entry_function)
            if not node:
                continue

            # Try to find the sequence through call graph
            if self._find_sequence_in_chain(call_chain, pattern.sequence):
                # Create a copy with source project set
                matched = CallChainPattern(
                    name=pattern.name,
                    description=pattern.description,
                    entry_function=pattern.entry_function,
                    sequence=pattern.sequence,
                    category=pattern.category,
                    source_project=call_chain.project_name
                )
                found_patterns.append(matched)

        return found_patterns

    def _find_sequence_in_chain(
        self,
        call_chain: CrossFileCallChain,
        sequence: List[str]
    ) -> bool:
        """Check if a sequence exists in the call chain."""
        if not sequence:
            return False

        # Get path from first to last element
        first = sequence[0]
        last = sequence[-1]

        # Check if all elements exist
        for func in sequence:
            if func not in call_chain.nodes:
                # Try partial match
                found = any(func in n or n in func for n in call_chain.nodes)
                if not found:
                    return False

        return True

    def export_call_chains_json(
        self,
        chains: Dict[str, CrossFileCallChain],
        output_file: str
    ) -> None:
        """
        Export call chains to JSON file.

        Args:
            chains: Dict of project name to CrossFileCallChain
            output_file: Output file path
        """
        data = {
            "metadata": {
                "total_projects": len(chains),
                "export_format": "call_chain_v2"
            },
            "chains": {name: chain.to_dict() for name, chain in chains.items()}
        }

        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)

        logger.info(f"Exported {len(chains)} call chains to {output_file}")


def extract_init_sequence(source_file: str, known_apis: Set[str]) -> List[str]:
    """Convenience function to extract init sequence."""
    extractor = CallChainExtractor()
    return extractor.extract_init_sequence(source_file, known_apis)


def extract_all_call_chains(projects_dir: str, known_apis: Optional[Set[str]] = None) -> Dict[str, CrossFileCallChain]:
    """Convenience function to extract all project call chains."""
    extractor = CallChainExtractor()
    return extractor.extract_all_projects(str(projects_dir), known_apis)


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python call_chain_extractor.py <source_path> [--cross-file]")
        print("  source_path: Path to main.c or project directory")
        print("  --cross-file: Extract cross-file call chains")
        sys.exit(1)

    extractor = CallChainExtractor()
    path = sys.argv[1]
    cross_file = '--cross-file' in sys.argv

    # Load known APIs if provided
    known_apis = set()
    if len(sys.argv) > 2 and not sys.argv[2].startswith('--'):
        with open(sys.argv[2], 'r') as f:
            known_apis = set(line.strip() for line in f if line.strip())

    if cross_file or Path(path).is_dir():
        # Extract cross-file call chain
        chain = extractor.extract_cross_file_call_chain(path, known_apis)
        if chain:
            print(f"\nCross-File Call Chain: {chain.project_name}")
            print(f"  Nodes: {len(chain.nodes)}")
            print(f"  Edges: {len(chain.edges)}")
            print(f"  SDK APIs: {len(chain.sdk_apis_used)}")
            print(f"  User Functions: {len(chain.user_funcs_used)}")
            print(f"  Max Depth: {chain.max_depth}")

            # Find patterns
            patterns = extractor.find_patterns_in_chain(chain)
            if patterns:
                print(f"\n  Found Patterns:")
                for p in patterns:
                    print(f"    - {p.name} ({p.category})")
    else:
        # Extract from single file
        init_seq = extractor.extract_init_sequence(path, known_apis)

        print(f"\nInitialization Sequence:")
        for i, api in enumerate(init_seq, 1):
            print(f"  {i}. {api}")
