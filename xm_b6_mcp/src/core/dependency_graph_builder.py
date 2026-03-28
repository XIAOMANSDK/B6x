"""
Dependency Graph Builder
========================

Build dependency graph between projects.

Creates:
- Project -> Projects (which projects depend on which)
- API -> Projects (which API is used in which projects)
- Library -> Projects (which library is used by which projects)
- Similar projects (based on peripheral usage)

Author: B6x MCP Server Team
Version: 0.1.0
"""

import logging
from pathlib import Path
from typing import Dict, List, Set, Optional, Any
from collections import defaultdict
from dataclasses import dataclass, field, asdict

try:
    from .project_config_parser import ProjectDependency
    from .call_chain_extractor import ProjectCallChain
except ImportError:
    ProjectDependency = None
    ProjectCallChain = None

logger = logging.getLogger(__name__)


@dataclass
class SimilarProject:
    """Information about a similar project."""
    project: str
    similarity: float  # 0.0 to 1.0
    common_peripherals: List[str]
    common_apis: Set[str] = field(default_factory=set)


@dataclass
class DependencyGraph:
    """
    Complete dependency graph for all projects.

    Attributes:
        projects: All project dependencies
        projects_call_chains: All project call chains
        library_to_projects: Mapping of library to projects that use it
        api_to_projects: Mapping of API to projects that use it
        peripheral_to_projects: Mapping of peripheral to projects that use it
    """
    projects: Dict[str, 'ProjectDependency'] = field(default_factory=dict)
    projects_call_chains: Dict[str, 'ProjectCallChain'] = field(default_factory=dict)
    library_to_projects: Dict[str, Set[str]] = field(default_factory=dict)
    api_to_projects: Dict[str, Set[str]] = field(default_factory=dict)
    peripheral_to_projects: Dict[str, Set[str]] = field(default_factory=dict)


class DependencyGraphBuilder:
    """
    Build dependency relationship graph between projects.

    Analyzes:
    - Which libraries each project uses
    - Which APIs each project uses
    - Peripheral usage patterns
    - Project similarities
    """

    def __init__(self):
        """Initialize the dependency graph builder."""
        self.graph = DependencyGraph()

    def add_project(
        self,
        dependency: Optional['ProjectDependency'] = None,
        call_chain: Optional['ProjectCallChain'] = None
    ):
        """
        Add a project to the dependency graph.

        Args:
            dependency: ProjectDependency object
            call_chain: ProjectCallChain object
        """
        if dependency:
            self.graph.projects[dependency.project_name] = dependency

            # Build library -> projects mapping
            for lib in dependency.linked_libraries:
                self.graph.library_to_projects[lib].add(dependency.project_name)

        if call_chain:
            self.graph.projects_call_chains[call_chain.project_name] = call_chain

            # Build API -> projects mapping
            for api in call_chain.main_apis_used:
                self.graph.api_to_projects[api].add(call_chain.project_name)

            # Build peripheral -> projects mapping
            for peripheral, count in call_chain.peripheral_usage.items():
                self.graph.peripheral_to_projects[peripheral].add(call_chain.project_name)

    def build_library_to_projects_map(self) -> Dict[str, List[str]]:
        """
        Build library to projects mapping.

        Returns:
            Dictionary mapping library names to lists of project names
        """
        return {
            lib: sorted(projects)
            for lib, projects in self.graph.library_to_projects.items()
        }

    def build_api_to_projects_map(self) -> Dict[str, List[str]]:
        """
        Build API to projects mapping.

        Returns:
            Dictionary mapping API names to lists of project names
        """
        return {
            api: sorted(projects)
            for api, projects in self.graph.api_to_projects.items()
        }

    def build_peripheral_to_projects_map(self) -> Dict[str, List[str]]:
        """
        Build peripheral to projects mapping.

        Returns:
            Dictionary mapping peripheral names to lists of project names
        """
        return {
            peripheral: sorted(projects)
            for peripheral, projects in self.graph.peripheral_to_projects.items()
        }

    def find_similar_projects(
        self,
        target_project: str,
        min_similarity: float = 0.3
    ) -> List[SimilarProject]:
        """
        Find projects similar to the target project.

        Similarity based on:
        - Peripheral usage (Jaccard coefficient)
        - Common APIs

        Args:
            target_project: Name of the target project
            min_similarity: Minimum similarity threshold (0-1)

        Returns:
            List of SimilarProject objects, sorted by similarity
        """
        target = self.graph.projects_call_chains.get(target_project)
        if not target:
            logger.warning(f"Project {target_project} not found in call chains")
            return []

        similar = []

        for proj_name, chain in self.graph.projects_call_chains.items():
            if proj_name == target_project:
                continue

            # Calculate peripheral usage similarity
            periph_sim = self._calculate_peripheral_similarity(
                target.peripheral_usage,
                chain.peripheral_usage
            )

            # Calculate API similarity
            api_sim = self._calculate_api_similarity(
                target.main_apis_used,
                chain.main_apis_used
            )

            # Combined similarity (weighted average)
            combined_sim = 0.7 * periph_sim + 0.3 * api_sim

            if combined_sim >= min_similarity:
                common_periphs = self._get_common_peripherals(
                    target.peripheral_usage,
                    chain.peripheral_usage
                )
                common_apis = target.main_apis_used & chain.main_apis_used

                similar.append(SimilarProject(
                    project=proj_name,
                    similarity=combined_sim,
                    common_peripherals=sorted(common_periphs),
                    common_apis=common_apis
                ))

        return sorted(similar, key=lambda x: x.similarity, reverse=True)

    def _calculate_peripheral_similarity(
        self,
        usage1: Dict[str, int],
        usage2: Dict[str, int]
    ) -> float:
        """
        Calculate peripheral usage similarity (Jaccard coefficient).

        Args:
            usage1: Peripheral usage dict for project 1
            usage2: Peripheral usage dict for project 2

        Returns:
            Similarity score (0.0 to 1.0)
        """
        set1 = set(usage1.keys())
        set2 = set(usage2.keys())

        intersection = len(set1 & set2)
        union = len(set1 | set2)

        return intersection / union if union > 0 else 0.0

    def _calculate_api_similarity(
        self,
        apis1: Set[str],
        apis2: Set[str]
    ) -> float:
        """
        Calculate API usage similarity (Jaccard coefficient).

        Args:
            apis1: Set of APIs used by project 1
            apis2: Set of APIs used by project 2

        Returns:
            Similarity score (0.0 to 1.0)
        """
        intersection = len(apis1 & apis2)
        union = len(apis1 | apis2)

        return intersection / union if union > 0 else 0.0

    def _get_common_peripherals(
        self,
        usage1: Dict[str, int],
        usage2: Dict[str, int]
    ) -> Set[str]:
        """Get common peripherals between two projects."""
        return set(usage1.keys()) & set(usage2.keys())

    def get_project_dependencies(self, project_name: str) -> Dict[str, Any]:
        """
        Get complete dependency information for a project.

        Args:
            project_name: Name of the project

        Returns:
            Dictionary with dependency information
        """
        dep = self.graph.projects.get(project_name)
        chain = self.graph.projects_call_chains.get(project_name)

        if not dep and not chain:
            return {}

        result = {
            'project': project_name,
        }

        if dep:
            result['linked_libraries'] = dep.linked_libraries
            result['include_paths'] = dep.include_paths
            result['source_files'] = dep.source_files
            result['defines'] = dep.defined_macros
            result['target_device'] = dep.target_device

        if chain:
            result['init_sequence'] = chain.init_sequence
            result['api_usage'] = {
                'total_apis': len(chain.main_apis_used),
                'peripherals': dict(chain.peripheral_usage)
            }
            result['entry_point'] = chain.entry_point

        return result

    def export_to_json(self, output_path: str):
        """
        Export dependency graph to JSON file.

        Args:
            output_path: Path to output JSON file
        """
        import json

        data = {
            "version": "1.0.0",
            "total_projects": len(self.graph.projects),
            "library_to_projects": self._convert_sets_to_lists(
                self.build_library_to_projects_map()
            ),
            "api_to_projects": self._convert_sets_to_lists(
                self.build_api_to_projects_map()
            ),
            "peripheral_to_projects": self._convert_sets_to_lists(
                self.build_peripheral_to_projects_map()
            ),
            "projects": {}
        }

        # Add project details
        for proj_name, dep in self.graph.projects.items():
            data["projects"][proj_name] = {
                "type": dep.project_type,
                "libraries": dep.linked_libraries,
                "includes": dep.include_paths,
                "sources": len(dep.source_files),
                "device": dep.target_device
            }

        # Add call chain details
        for proj_name, chain in self.graph.projects_call_chains.items():
            if proj_name not in data["projects"]:
                data["projects"][proj_name] = {}

            data["projects"][proj_name]["call_chain"] = {
                "entry_point": chain.entry_point,
                "init_sequence": chain.init_sequence,
                "total_apis": len(chain.main_apis_used),
                "peripheral_usage": chain.peripheral_usage
            }

        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)

        logger.info(f"Dependency graph exported to: {output_path}")

    def _convert_sets_to_lists(self, data: Dict[str, Set[str]]) -> Dict[str, List[str]]:
        """Convert sets to lists for JSON serialization."""
        return {k: sorted(v) for k, v in data.items()}

    def load_from_json(self, json_path: str):
        """
        Load dependency graph from JSON file.

        Args:
            json_path: Path to JSON file
        """
        import json

        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Restore library_to_projects
        for lib, projects in data.get("library_to_projects", {}).items():
            self.graph.library_to_projects[lib] = set(projects)

        # Restore api_to_projects
        for api, projects in data.get("api_to_projects", {}).items():
            self.graph.api_to_projects[api] = set(projects)

        # Restore peripheral_to_projects
        for periph, projects in data.get("peripheral_to_projects", {}).items():
            self.graph.peripheral_to_projects[periph] = set(projects)

        logger.info(f"Dependency graph loaded from: {json_path}")


def build_dependency_graph(
    projects: List['ProjectDependency'],
    call_chains: List['ProjectCallChain']
) -> DependencyGraphBuilder:
    """
    Convenience function to build a dependency graph.

    Args:
        projects: List of ProjectDependency objects
        call_chains: List of ProjectCallChain objects

    Returns:
        DependencyGraphBuilder instance
    """
    builder = DependencyGraphBuilder()

    for project in projects:
        builder.add_project(dependency=project)

    for chain in call_chains:
        builder.add_project(call_chain=chain)

    return builder


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    print("Dependency Graph Builder - Test Mode")
    print("This module is typically used via the build_index.py script")
