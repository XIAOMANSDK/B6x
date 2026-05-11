"""
API Manifest Generator
======================

Generates api_manifest.json - a human-readable manifest of API usage
across examples and projects.

This module provides functionality to:
- Build API-to-Examples and Example-to-APIs mappings
- Collect call site details (file, line, context)
- Compute usage statistics
- Export to JSON format

Author: B6x MCP Server Team
Version: 0.1.0
"""

import logging
import json
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional, Set
from dataclasses import dataclass, field, asdict
from collections import defaultdict

logger = logging.getLogger(__name__)


@dataclass
class APIUsageManifest:
    """API usage manifest for the entire SDK."""
    manifest_version: str = "1.0.0"
    generation_timestamp: str = ""
    sdk_path: str = ""

    # Scan scope
    scan_scope: Dict[str, int] = field(default_factory=dict)  # examples/projects count

    # Statistics
    total_examples: int = 0
    total_files: int = 0
    total_api_calls: int = 0
    unique_sdk_apis_used: int = 0

    # Mappings
    api_to_examples: Dict[str, List[str]] = field(default_factory=dict)
    example_to_apis: Dict[str, List[str]] = field(default_factory=dict)
    api_call_sites: Dict[str, List[Dict]] = field(default_factory=dict)
    api_statistics: Dict[str, Dict] = field(default_factory=dict)
    example_metadata: Dict[str, Dict] = field(default_factory=dict)

    def to_json(self, file_path: str, indent: int = 2):
        """Export manifest to JSON file."""
        # Convert sets to lists
        manifest_dict = asdict(self)

        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(manifest_dict, f, indent=indent, ensure_ascii=False)

        logger.info(f"Manifest written to: {file_path}")

    @classmethod
    def from_json(cls, file_path: str) -> 'APIUsageManifest':
        """Load manifest from JSON file."""
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        return cls(**data)


class APIUsageManifestBuilder:
    """
    Builder for API usage manifest.

    Collects example scan results and builds a complete manifest
    with mappings, statistics, and call site details.
    """

    def __init__(self):
        """Initialize the builder."""
        self._api_to_examples: Dict[str, Set[str]] = defaultdict(set)
        self._example_to_apis: Dict[str, Set[str]] = defaultdict(set)
        self._api_call_sites: Dict[str, List[Dict]] = defaultdict(list)
        self._api_calls_count: Dict[str, int] = defaultdict(int)
        self._example_files: Dict[str, Set[str]] = defaultdict(set)

    def build_manifest(
        self,
        example_results: List,
        sdk_path: str,
        known_sdk_apis: Set[str]
    ) -> APIUsageManifest:
        """
        Build complete manifest from example scan results.

        Args:
            example_results: List of ExampleParseResult objects
            sdk_path: Path to SDK root
            known_sdk_apis: Set of all known SDK API names

        Returns:
            APIUsageManifest object
        """
        # Build mappings
        for result in example_results:
            self._process_example_result(result)

        # Build statistics
        api_statistics = self._build_api_statistics(known_sdk_apis)
        example_metadata = self._build_example_metadata()

        # Create manifest
        manifest = APIUsageManifest(
            generation_timestamp=datetime.now().isoformat(),
            sdk_path=str(sdk_path),
            scan_scope=self._compute_scan_scope(),
            total_examples=len(self._example_to_apis),
            total_files=sum(len(files) for files in self._example_files.values()),
            total_api_calls=sum(self._api_calls_count.values()),
            unique_sdk_apis_used=len(self._api_to_examples),
            api_to_examples=self._convert_sets_to_lists(self._api_to_examples),
            example_to_apis=self._convert_sets_to_lists(self._example_to_apis),
            api_call_sites=dict(self._api_call_sites),
            api_statistics=api_statistics,
            example_metadata=example_metadata
        )

        return manifest

    def _process_example_result(self, result):
        """Process a single example parse result."""
        example_key = f"{result.example_type}/{result.example_name}"

        # Track file
        self._example_files[example_key].add(result.file_path)

        # Process each function call
        for call in result.function_calls:
            if call.is_sdk_api:
                # API -> Examples mapping
                self._api_to_examples[call.function_name].add(example_key)

                # Example -> APIs mapping
                self._example_to_apis[example_key].add(call.function_name)

                # Count API calls
                self._api_calls_count[call.function_name] += 1

                # Collect call site details
                self._api_call_sites[call.function_name].append({
                    "file": result.file_path,
                    "example": result.example_name,
                    "line": call.line_number,
                    "column": call.column_number,
                    "function": call.calling_function,
                    "args": call.arguments[:5],  # Limit to 5 args
                    "context": call.context_snippet
                })

    def _build_api_statistics(self, known_sdk_apis: Set[str]) -> Dict[str, Dict]:
        """Build statistics for each API."""
        statistics = {}

        for api_name in self._api_to_examples.keys():
            examples = self._api_to_examples[api_name]
            call_count = self._api_calls_count[api_name]
            call_sites = self._api_call_sites.get(api_name, [])

            # Determine peripheral and module from first call site
            peripheral = ""
            module = ""
            if call_sites:
                # Extract from context or classify
                peripheral = self._extract_peripheral_from_name(api_name)
                module = self._extract_module_from_name(api_name)

            statistics[api_name] = {
                "usage_count": call_count,
                "example_count": len(examples),
                "peripheral": peripheral,
                "module": module,
                "files": list(set(site["file"] for site in call_sites))
            }

        return statistics

    def _build_example_metadata(self) -> Dict[str, Dict]:
        """Build metadata for each example."""
        metadata = {}

        for example_key, apis in self._example_to_apis.items():
            example_type, example_name = example_key.split('/', 1)

            # Classify APIs by peripheral
            peripheral_breakdown = defaultdict(int)
            for api in apis:
                peripheral = self._extract_peripheral_from_name(api)
                if peripheral:
                    peripheral_breakdown[peripheral] += 1

            metadata[example_key] = {
                "name": example_name,
                "type": example_type,
                "api_count": len(apis),
                "file_count": len(self._example_files[example_key]),
                "peripheral_breakdown": dict(peripheral_breakdown)
            }

        return metadata

    def _compute_scan_scope(self) -> Dict[str, int]:
        """Compute scan scope counts."""
        scope = {}

        for example_key in self._example_to_apis.keys():
            example_type = example_key.split('/')[0]
            scope[example_type] = scope.get(example_type, 0) + 1

        return scope

    def _extract_peripheral_from_name(self, api_name: str) -> str:
        """Extract peripheral name from API name."""
        peripherals = [
            'GPIO', 'UART', 'SPI', 'I2C', 'ADC', 'DAC', 'TIMER', 'PWM',
            'DMA', 'EXTI', 'RTC', 'WDT', 'FLASH', 'USB', 'BLE', 'IWDG',
            'LDO', 'RCO', 'PWC', 'SADC', 'STEPPER', 'IR'
        ]

        for p in peripherals:
            if api_name.startswith(p):
                return p

        return ""

    def _extract_module_from_name(self, api_name: str) -> str:
        """Extract module name from API name."""
        if api_name.startswith(('ble_', 'BLE', 'ke_', 'KE')):
            return 'ble'
        elif api_name.startswith(('B6x_', 'b6x_')):
            return 'driver'
        else:
            return 'driver'  # Default to driver

    def _convert_sets_to_lists(self, data: Dict[str, Set[str]]) -> Dict[str, List[str]]:
        """Convert sets to sorted lists for JSON serialization."""
        return {
            k: sorted(list(v)) for k, v in data.items()
        }

    def print_statistics(self, manifest: APIUsageManifest):
        """Print manifest statistics."""
        print("\n" + "=" * 70)
        print("API Manifest Statistics")
        print("=" * 70)
        print(f"  SDK Path:            {manifest.sdk_path}")
        print(f"  Manifest Version:    {manifest.manifest_version}")
        print(f"  Generated:           {manifest.generation_timestamp[:19]}")
        print()
        print(f"  Scan Scope:")
        for scope_type, count in manifest.scan_scope.items():
            print(f"    {scope_type:15s}: {count:4d}")
        print()
        print(f"  Total Examples:      {manifest.total_examples}")
        print(f"  Total Files:         {manifest.total_files}")
        print(f"  Total API Calls:     {manifest.total_api_calls}")
        print(f"  Unique SDK APIs:     {manifest.unique_sdk_apis_used}")

        # Most used APIs
        sorted_apis = sorted(
            manifest.api_statistics.items(),
            key=lambda x: x[1]['usage_count'],
            reverse=True
        )[:10]

        if sorted_apis:
            print("\n  Most Used APIs:")
            for api, stats in sorted_apis:
                print(f"    {api:30s}: {stats['usage_count']:4d} calls in "
                      f"{stats['example_count']:2d} examples")

        print("=" * 70)

    def reset(self):
        """Reset builder state."""
        self._api_to_examples.clear()
        self._example_to_apis.clear()
        self._api_call_sites.clear()
        self._api_calls_count.clear()
        self._example_files.clear()
