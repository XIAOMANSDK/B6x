"""
Example Scanner
===============

Scans examples/ and projects/ directories to extract API usage information.

This module provides functionality to:
- Recursively scan example directories for .c files
- Parse example files to extract SDK API calls
- Build mappings between APIs and examples
- Generate usage statistics

Author: B6x MCP Server Team
Version: 0.1.0
"""

import logging
from pathlib import Path
from typing import List, Dict, Optional, Set
from dataclasses import dataclass, field

logger = logging.getLogger(__name__)


@dataclass
class ExampleMetadata:
    """Metadata about an example project."""
    name: str
    path: str
    example_type: str  # 'examples' or 'projects'
    source_files: List[str] = field(default_factory=list)
    header_files: List[str] = field(default_factory=list)
    total_api_calls: int = 0
    unique_apis_used: Set[str] = field(default_factory=set)


class ExampleScanner:
    """
    Scanner for extracting API usage from example code.

    Scans the examples/ and projects/ directories to find which
    SDK APIs are used in each example project.
    """

    def __init__(self, parser):
        """
        Initialize the example scanner.

        Args:
            parser: TreeSitterCParser instance for parsing files
        """
        self.parser = parser
        self._example_metadata: Dict[str, ExampleMetadata] = {}

    def scan_directory(
        self,
        directory: str,
        known_sdk_apis: Set[str],
        example_type: str = 'examples',
        extract_context: bool = False,
        context_lines: int = 5
    ) -> List:
        """
        Scan a directory for example files.

        Args:
            directory: Path to examples/ or projects/ directory
            known_sdk_apis: Set of known SDK API names
            example_type: 'examples' or 'projects'
            extract_context: Whether to extract extended context (default: False)
            context_lines: Number of context lines to extract (default: 5)

        Returns:
            List of ExampleParseResult objects
        """
        from .tree_sitter_parser import ExampleParseResult

        if not self.parser._initialized:
            logger.error("Parser not initialized")
            return []

        dir_path = Path(directory)
        if not dir_path.exists():
            logger.warning(f"Directory not found: {directory}")
            return []

        results = []
        source_files = list(dir_path.rglob("*.c"))
        total_files = len(source_files)

        logger.info(f"Scanning {total_files} .c files in {directory} (context extraction: {extract_context})")

        for i, source_file in enumerate(source_files):
            if (i + 1) % 50 == 0:
                logger.info(f"  Progress: {i + 1}/{total_files} files scanned")

            result = self.parser.parse_example_file(
                str(source_file),
                known_sdk_apis,
                example_type,
                extract_context=extract_context,
                context_lines=context_lines
            )

            if result and result.function_calls:
                results.append(result)
                self._update_metadata(result)

        logger.info(f"Found {len(results)} files with SDK API calls in {directory}")
        return results

    def scan_all(
        self,
        examples_dir: str,
        projects_dir: str,
        known_sdk_apis: Set[str]
    ) -> List:
        """
        Scan both examples/ and projects/ directories.

        Args:
            examples_dir: Path to examples/ directory
            projects_dir: Path to projects/ directory
            known_sdk_apis: Set of known SDK API names

        Returns:
            Combined list of ExampleParseResult objects
        """
        all_results = []

        # Scan examples
        if Path(examples_dir).exists():
            example_results = self.scan_directory(
                examples_dir,
                known_sdk_apis,
                'examples'
            )
            all_results.extend(example_results)

        # Scan projects
        if Path(projects_dir).exists():
            project_results = self.scan_directory(
                projects_dir,
                known_sdk_apis,
                'projects'
            )
            all_results.extend(project_results)

        return all_results

    def get_examples_for_peripheral(
        self,
        peripheral: str
    ) -> List[str]:
        """
        Get list of examples that use a specific peripheral.

        Args:
            peripheral: Peripheral name (e.g., 'GPIO', 'UART')

        Returns:
            List of example names
        """
        examples = []
        peripheral = peripheral.upper()

        for example_name, metadata in self._example_metadata.items():
            for api in metadata.unique_apis_used:
                if api.upper().startswith(peripheral):
                    examples.append(example_name)
                    break

        return sorted(examples)

    def get_examples_for_api(self, api_name: str) -> List[str]:
        """
        Get list of examples that use a specific API.

        Args:
            api_name: Name of the API function

        Returns:
            List of example names
        """
        examples = []

        for example_name, metadata in self._example_metadata.items():
            if api_name in metadata.unique_apis_used:
                examples.append(example_name)

        return sorted(examples)

    def search_examples(
        self,
        query: str,
        search_in: str = "both",
        limit: int = 20
    ) -> List[Dict[str, any]]:
        """
        Search examples by name, API usage, or path.

        Args:
            query: Search query (case-insensitive)
            search_in: Where to search - "name", "path", "api", or "both"
            limit: Maximum number of results

        Returns:
            List of dicts with example info:
            {
                "name": str,
                "path": str,
                "type": str,
                "total_api_calls": int,
                "unique_apis": int,
                "matched_apis": List[str]  # If search_in includes "api"
            }
        """
        results = []
        query_lower = query.lower()

        for example_name, metadata in self._example_metadata.items():
            match = False
            matched_apis = []

            # Search in example name
            if search_in in ["name", "both"]:
                if query_lower in example_name.lower():
                    match = True

            # Search in path
            if search_in in ["path", "both"]:
                if query_lower in metadata.path.lower():
                    match = True

            # Search in API names
            if search_in in ["api", "both"]:
                for api_name in metadata.unique_apis_used:
                    if query_lower in api_name.lower():
                        match = True
                        matched_apis.append(api_name)

            if match:
                results.append({
                    "name": example_name,
                    "path": metadata.path,
                    "type": metadata.example_type,
                    "total_api_calls": metadata.total_api_calls,
                    "unique_apis": len(metadata.unique_apis_used),
                    "matched_apis": matched_apis if search_in == "api" else []
                })

                if len(results) >= limit:
                    break

        return results

    def get_example_metadata(self, example_name: str) -> Optional[ExampleMetadata]:
        """
        Get metadata for a specific example.

        Args:
            example_name: Name of the example

        Returns:
            ExampleMetadata or None
        """
        return self._example_metadata.get(example_name)

    def get_all_metadata(self) -> Dict[str, ExampleMetadata]:
        """Get metadata for all scanned examples."""
        return self._example_metadata.copy()

    def print_statistics(self):
        """Print scanning statistics."""
        total_examples = len(self._example_metadata)
        total_files = sum(m.total_api_calls for m in self._example_metadata.values())

        print("\n" + "=" * 70)
        print("Example Scanner Statistics")
        print("=" * 70)
        print(f"  Total Examples:      {total_examples}")
        print(f"  Total API Calls:     {total_files}")

        # Top examples by API usage
        sorted_examples = sorted(
            self._example_metadata.items(),
            key=lambda x: x[1].total_api_calls,
            reverse=True
        )[:10]

        if sorted_examples:
            print("\n  Top Examples by API Usage:")
            for name, meta in sorted_examples:
                print(f"    {name:30s}: {meta.total_api_calls:4d} calls, "
                      f"{len(meta.unique_apis_used):3d} unique APIs")

        print("=" * 70)

    def _update_metadata(self, result):
        """Update metadata from a parse result."""
        example_name = result.example_name

        if example_name not in self._example_metadata:
            # Extract example type from path
            example_type = result.example_type
            path = str(Path(result.file_path).parent)

            self._example_metadata[example_name] = ExampleMetadata(
                name=example_name,
                path=path,
                example_type=example_type
            )

        metadata = self._example_metadata[example_name]

        # Add source file
        if result.file_path not in metadata.source_files:
            metadata.source_files.append(result.file_path)

        # Count API calls
        sdk_calls = [c for c in result.function_calls if c.is_sdk_api]
        metadata.total_api_calls += len(sdk_calls)

        # Collect unique APIs
        for call in sdk_calls:
            metadata.unique_apis_used.add(call.function_name)

    def reset(self):
        """Reset scanner state."""
        self._example_metadata.clear()
