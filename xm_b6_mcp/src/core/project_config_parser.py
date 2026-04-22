"""
Project Configuration Parser
=============================

Parse project configuration files to extract dependencies.

Supports:
- Keil .uvprojx (XML)
- Makefile
- CMakeLists.txt

Author: B6x MCP Server Team
Version: 0.1.0
"""

import re
import logging
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, field

logger = logging.getLogger(__name__)


@dataclass
class ProjectDependency:
    """
    Project dependency information.

    Attributes:
        project_name: Name of the project
        project_path: Path to project directory
        project_type: 'keil', 'makefile', 'cmake', 'unknown'
        linked_libraries: List of linked library files
        include_paths: Include directory paths
        source_files: Source file paths
        header_files: Header file paths
        defined_macros: Preprocessor macro definitions
        compiler_flags: Compiler options
        linker_flags: Linker options
        target_device: Target MCU/device name
    """
    project_name: str
    project_path: str
    project_type: str
    linked_libraries: List[str] = field(default_factory=list)
    include_paths: List[str] = field(default_factory=list)
    source_files: List[str] = field(default_factory=list)
    header_files: List[str] = field(default_factory=list)
    defined_macros: List[str] = field(default_factory=list)
    compiler_flags: List[str] = field(default_factory=list)
    linker_flags: List[str] = field(default_factory=list)
    target_device: str = ""


class ProjectConfigParser:
    """
    Parse project configuration files.

    Supports multiple build systems:
    - Keil µVision (.uvprojx)
    - Makefile
    - CMake (CMakeLists.txt)
    """

    def __init__(self):
        """Initialize the project config parser."""
        pass

    def parse_project(self, project_path: str) -> Optional[ProjectDependency]:
        """
        Auto-detect and parse project configuration.

        Args:
            project_path: Path to project directory or config file

        Returns:
            ProjectDependency or None
        """
        project_path = Path(project_path)

        # If it's a directory, look for config files
        if project_path.is_dir():
            # Check for Keil project
            uvprojx_files = list(project_path.rglob("*.uvprojx"))
            if uvprojx_files:
                return self.parse_keil_project(str(uvprojx_files[0]))

            # Check for Makefile
            makefile = project_path / "Makefile"
            if makefile.exists():
                return self.parse_makefile(str(makefile))

            # Check for CMake
            cmake_file = project_path / "CMakeLists.txt"
            if cmake_file.exists():
                return self.parse_cmake_project(str(cmake_file))

            logger.warning(f"No recognized project config in {project_path}")
            return None

        # If it's a file, parse based on extension
        elif project_path.is_file():
            if project_path.suffix == '.uvprojx':
                return self.parse_keil_project(str(project_path))
            elif project_path.name in ['Makefile', 'makefile']:
                return self.parse_makefile(str(project_path))
            elif project_path.name == 'CMakeLists.txt':
                return self.parse_cmake_project(str(project_path))

        return None

    def parse_keil_project(self, uvprojx_path: str) -> Optional[ProjectDependency]:
        """
        Parse Keil .uvprojx project file.

        Args:
            uvprojx_path: Path to .uvprojx file

        Returns:
            ProjectDependency or None
        """
        uvprojx_path = Path(uvprojx_path)
        if not uvprojx_path.exists():
            logger.warning(f"Keil project file not found: {uvprojx_path}")
            return None

        try:
            tree = ET.parse(uvprojx_path)
            root = tree.getroot()

            # Extract project name
            project_name = uvprojx_path.stem

            # Extract project path
            project_path = str(uvprojx_path.parent)

            # Extract linked libraries
            libs = []
            for lib_elem in root.findall(".//Library"):
                if lib_elem.text:
                    libs.append(lib_elem.text.strip())

            # Extract include paths
            includes = []
            for inc_elem in root.findall(".//IncludePath"):
                if inc_elem.text:
                    includes.extend([p.strip() for p in inc_elem.text.split(';') if p.strip()])

            # Extract source files
            sources = []
            for file_elem in root.findall(".//FilePath"):
                if file_elem.text:
                    sources.append(file_elem.text.strip())

            # Extract header files
            headers = [f for f in sources if f.endswith('.h')]
            sources = [f for f in sources if f.endswith('.c') or f.endswith('.s')]

            # Extract defines
            defines = []
            for def_elem in root.findall(".//Define"):
                if def_elem.text:
                    defines.append(def_elem.text.strip())

            # Extract target device
            target_device = ""
            device_elem = root.find(".//Device")
            if device_elem is not None and device_elem.text:
                target_device = device_elem.text.strip()

            return ProjectDependency(
                project_name=project_name,
                project_path=project_path,
                project_type='keil',
                linked_libraries=libs,
                include_paths=includes,
                source_files=sources,
                header_files=headers,
                defined_macros=defines,
                compiler_flags=[],
                linker_flags=[],
                target_device=target_device
            )

        except ET.ParseError as e:
            logger.error(f"Failed to parse Keil project {uvprojx_path}: {e}")
            return None
        except Exception as e:
            logger.error(f"Error parsing Keil project {uvprojx_path}: {e}")
            return None

    def parse_makefile(self, makefile_path: str) -> Optional[ProjectDependency]:
        """
        Parse Makefile.

        Args:
            makefile_path: Path to Makefile

        Returns:
            ProjectDependency or None
        """
        makefile_path = Path(makefile_path)
        if not makefile_path.exists():
            logger.warning(f"Makefile not found: {makefile_path}")
            return None

        try:
            with open(makefile_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            # Extract project name from directory
            project_name = makefile_path.parent.name
            project_path = str(makefile_path.parent)

            # Extract libraries (LIBS or LDFLAGS)
            libs = []
            lib_patterns = [
                r'LIBS\s*[\+:]?=\s*(.+)',
                r'LDLIBS\s*[\+:]?=\s*(.+)',
                r'LDFLAGS\s*[\+:]?=\s*(.+?)-l',
            ]
            for pattern in lib_patterns:
                matches = re.findall(pattern, content, re.MULTILINE)
                for match in matches:
                    # Extract -l<lib> patterns
                    lib_names = re.findall(r'-l(\w+)', match)
                    libs.extend([f"lib{name}.a" for name in lib_names])

            # Extract include paths (INCLUDES or CFLAGS)
            includes = []
            inc_patterns = [
                r'INCLUDES\s*[\+:]?=\s*(.+)',
                r'CFLAGS\s*[\+:]?=\s*(.+)',
            ]
            for pattern in inc_patterns:
                matches = re.findall(pattern, content, re.MULTILINE)
                for match in matches:
                    # Extract -I<path> patterns
                    inc_paths = re.findall(r'-I([^\s]+)', match)
                    includes.extend(inc_paths)

            # Extract source files (SOURCES)
            sources = []
            source_patterns = [
                r'SOURCES\s*[\+:]?=\s*(.+)',
                r'CSRCS\s*[\+:]?=\s*(.+)',  # C sources
                r'ASRCS\s*[\+:]?=\s*(.+)',  # Assembly sources
            ]
            for pattern in source_patterns:
                matches = re.findall(pattern, content, re.MULTILINE)
                for match in matches:
                    # Split on whitespace
                    srcs = match.split()
                    sources.extend([s for s in srcs if s.endswith('.c') or s.endswith('.s')])

            # Extract defines
            defines = []
            define_patterns = [
                r'DEFS\s*[\+:]?=\s*(.+)',
                r'CFLAGS\s*[\+:]?=\s*(.+)',
            ]
            for pattern in define_patterns:
                matches = re.findall(pattern, content, re.MULTILINE)
                for match in matches:
                    # Extract -D<macro> patterns
                    macros = re.findall(r'-D([^\s]+)', match)
                    defines.extend(macros)

            return ProjectDependency(
                project_name=project_name,
                project_path=project_path,
                project_type='makefile',
                linked_libraries=libs,
                include_paths=includes,
                source_files=sources,
                header_files=[],
                defined_macros=defines,
                compiler_flags=[],
                linker_flags=[],
                target_device=""
            )

        except Exception as e:
            logger.error(f"Error parsing Makefile {makefile_path}: {e}")
            return None

    def parse_cmake_project(self, cmake_path: str) -> Optional[ProjectDependency]:
        """
        Parse CMakeLists.txt.

        Args:
            cmake_path: Path to CMakeLists.txt

        Returns:
            ProjectDependency or None
        """
        cmake_path = Path(cmake_path)
        if not cmake_path.exists():
            logger.warning(f"CMakeLists.txt not found: {cmake_path}")
            return None

        try:
            with open(cmake_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            # Extract project name
            project_name = cmake_path.parent.name
            project_match = re.search(r'project\s*\(\s*(\w+)', content, re.IGNORECASE)
            if project_match:
                project_name = project_match.group(1)

            project_path = str(cmake_path.parent)

            # Extract include directories
            includes = []
            for match in re.finditer(r'include_directories\s*\((.+?)\)', content, re.DOTALL):
                inc_text = match.group(1)
                # Extract paths from ${} or quoted strings
                paths = re.findall(r'\$\{\w+\}|"([^"]+)"|(\S+)', inc_text)
                for p in paths:
                    inc_path = p[0] or p[1]
                    if inc_path and not inc_path.startswith('${'):
                        includes.append(inc_path)

            # Extract source files
            sources = []
            for match in re.finditer(r'(?:add_executable|add_library)\s*\(\s*\w+\s+(.+?)\)', content, re.DOTALL):
                src_text = match.group(1)
                srcs = re.findall(r'"([^"]+)"', src_text)
                sources.extend([s for s in srcs if s.endswith('.c') or s.endswith('.s')])

            # Extract defines
            defines = []
            for match in re.finditer(r'add_definitions\s*\((.+?)\)', content, re.DOTALL):
                def_text = match.group(1)
                defs = re.findall(r'-D([^\s"]+)', def_text)
                defines.extend(defs)

            # Extract target device (CMAKE_SYSTEM_PROCESSOR)
            target_device = ""
            device_match = re.search(r'CMAKE_SYSTEM_PROCESSOR\s*(?::=|=)\s*"?(\w+)"?', content)
            if device_match:
                target_device = device_match.group(1)

            return ProjectDependency(
                project_name=project_name,
                project_path=project_path,
                project_type='cmake',
                linked_libraries=[],  # Would need more complex parsing
                include_paths=includes,
                source_files=sources,
                header_files=[],
                defined_macros=defines,
                compiler_flags=[],
                linker_flags=[],
                target_device=target_device
            )

        except Exception as e:
            logger.error(f"Error parsing CMakeLists.txt {cmake_path}: {e}")
            return None

    def find_all_projects(self, sdk_path: str) -> List[ProjectDependency]:
        """
        Find all projects in SDK directory.

        Args:
            sdk_path: Path to SDK root

        Returns:
            List of ProjectDependency objects
        """
        sdk_path = Path(sdk_path)
        projects = []

        # Look for Keil projects in examples/ and projects/
        for search_dir in ['examples', 'projects']:
            search_path = sdk_path / search_dir
            if search_path.exists():
                for uvprojx in search_path.rglob("*.uvprojx"):
                    dep = self.parse_keil_project(str(uvprojx))
                    if dep:
                        projects.append(dep)

        logger.info(f"Found {len(projects)} projects in {sdk_path}")
        return projects


def parse_project(project_path: str) -> Optional[ProjectDependency]:
    """Convenience function to parse a project."""
    parser = ProjectConfigParser()
    return parser.parse_project(project_path)


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python project_config_parser.py <project_path_or_file>")
        sys.exit(1)

    parser = ProjectConfigParser()
    dep = parser.parse_project(sys.argv[1])

    if dep:
        print(f"\nProject: {dep.project_name}")
        print(f"Type: {dep.project_type}")
        print(f"Path: {dep.project_path}")
        print(f"Libraries: {len(dep.linked_libraries)}")
        print(f"Include paths: {len(dep.include_paths)}")
        print(f"Source files: {len(dep.source_files)}")
        print(f"Defines: {len(dep.defined_macros)}")

        if dep.target_device:
            print(f"Target device: {dep.target_device}")
    else:
        print("Failed to parse project")
