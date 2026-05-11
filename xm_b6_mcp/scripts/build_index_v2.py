"""
Four-Dimensional Knowledge Graph Builder
========================================

Builds the unified knowledge graph from all SDK domains using
the new 4D architecture.

Domain Build Order (dependency-respect):
1. Hardware  (registers, pin mux, memory map, interrupts)
2. Drivers   (APIs, structs, dependencies)
3. BLE       (APIs, profiles, error codes)
4. Apps      (examples, call chains, configs)

After building all domains, constructs cross-domain relations.

Usage:
    python scripts/build_index_v2.py                          # Full build
    python scripts/build_index_v2.py --domain hardware         # Single domain
    python scripts/build_index_v2.py --stats                   # Show stats
    python scripts/build_index_v2.py --test                    # Test mode

Output:
    - data/knowledge_graph.json           (Unified graph)
    - data/domain/{domain}/               (Per-domain data)
    - data/relations/                     (Cross-domain relations)
"""

import sys
import logging
import json
from pathlib import Path
from typing import Dict, List, Optional
from datetime import datetime
from dataclasses import asdict

# Add src to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'src'))

from common.path_utils import make_relative, normalize_path
from domain_config import DOMAIN_REGISTRY, DomainType, get_all_domains
from core.knowledge_graph_schema import KnowledgeGraph, KnowledgeGraphEntry, EntryType, RelationType, CrossDomainRelation
from core.relation_mapper import CrossDomainRelationMapper

# Import parsers
from core.svd_parser import SVDParser
from core.excel_parser import ExcelHardwareParser
from core.linker_parser import LinkerScriptParser
from core.interrupt_parser import InterruptVectorParser
from core.tree_sitter_parser import TreeSitterCParser, parse_directory_full, parse_directory_source
from core.struct_extractor import StructExtractor
from core.dependency_extractor import DoxygenDependencyExtractor
from core.profile_parser import ProfileParser
from core.error_code_parser import BLEErrorCodeParser
from core.mesh_parser import MeshAPIParser, MeshErrorCodeParser, MeshModelParser, parse_mesh_module
from core.example_scanner import ExampleScanner
from core.call_chain_extractor import CallChainExtractor
from core.api_manifest_generator import APIUsageManifestBuilder
from core.hardware_constraint_indexer import HardwareConstraintIndexer
from core.dependency_graph_builder import DependencyGraphBuilder

# Domain 4 enhancements (v0.2.0)
from core.init_sequence_extractor import InitSequenceExtractor
from core.cfg_header_parser import CFGHeaderParser

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


# ============================================================================
# Configuration
# ============================================================================

# Path configuration: scripts/ → xm_b6_mcp/ → sdk6/
# build_index_v2.py is in: sdk6/xm_b6_mcp/scripts/
# SDK root is two levels up: sdk6/
SDK_PATH = Path(__file__).parent.parent.parent
# Output is in: sdk6/xm_b6_mcp/data/
OUTPUT_DIR = Path(__file__).parent.parent / "data"


# ============================================================================
# Knowledge Graph Builder
# ============================================================================

class FourDimensionalKnowledgeGraphBuilder:
    """Builder for the 4D knowledge graph."""

    def __init__(self, sdk_path: Path, output_dir: Path):
        self.sdk_path = sdk_path
        self.output_dir = output_dir
        self.knowledge_graph = KnowledgeGraph()

        # Output subdirectories
        self.domain_output_dir = output_dir / "domain"
        self.relations_output_dir = output_dir / "relations"

        # Ensure output directories exist
        self.domain_output_dir.mkdir(parents=True, exist_ok=True)
        self.relations_output_dir.mkdir(parents=True, exist_ok=True)

        # Per-domain data collectors
        self.domain_data: Dict[DomainType, Dict] = {
            "hardware": {},
            "drivers": {},
            "ble": {},
            "applications": {}
        }

        # Relation mapper
        self.relation_mapper = CrossDomainRelationMapper()

    def _convert_paths_to_relative(self, data):
        """
        Recursively convert absolute paths to relative paths in data structures.

        This ensures portability of generated data files across different environments.

        Args:
            data: Any data structure (dict, list, str, etc.)

        Returns:
            Data structure with paths converted to relative
        """
        import re

        if isinstance(data, dict):
            return {k: self._convert_paths_to_relative(v) for k, v in data.items()}
        elif isinstance(data, list):
            return [self._convert_paths_to_relative(item) for item in data]
        elif isinstance(data, str):
            # Check if this looks like a Windows absolute path
            if re.match(r'^[A-Z]:[/\\]', data):
                # Try to extract relative part from SDK path
                sdk_str = str(self.sdk_path)
                # Handle both forward and backslash paths
                data_normalized = data.replace('/', '\\')
                sdk_normalized = sdk_str.replace('/', '\\')

                if data_normalized.lower().startswith(sdk_normalized.lower()):
                    # Extract relative part
                    rel_part = data[len(sdk_str):].lstrip('/\\')
                    return rel_part.replace('\\', '/')  # Normalize to forward slashes

                # Try to find SDK root in path
                for sdk_part in ['sdk6', 'sdk', 'bxx_DragonC1']:
                    idx = data.lower().find(sdk_part.lower())
                    if idx >= 0:
                        remaining = data[idx + len(sdk_part):].lstrip('/\\')
                        if remaining:
                            return remaining.replace('\\', '/')

            return data
        else:
            return data

    def load_constraint_json(self, constraint_name: str) -> Optional[Dict]:
        """
        Load constraint JSON from data/constraints/.

        Args:
            constraint_name: Name of JSON file (e.g., "io_map.json")

        Returns:
            Parsed JSON dict or None if file not found
        """
        constraint_path = self.output_dir / "constraints" / constraint_name

        if not constraint_path.exists():
            logger.warning(f"  Constraint file not found: {constraint_path}")
            logger.warning(f"  Run 'python scripts/build_excel_constraints.py' first")
            return None

        try:
            with open(constraint_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            logger.info(f"  Loaded {constraint_name} (last_updated: {data.get('last_updated', 'unknown')})")
            return data
        except Exception as e:
            logger.error(f"  Failed to load {constraint_name}: {e}")
            return None

    # ========================================================================
    # Domain 1: Hardware & Registers
    # ========================================================================

    def build_hardware_domain(self) -> None:
        """Build Domain 1: Hardware & Registers."""
        logger.info("="*70)
        logger.info("Building Domain 1: Hardware & Registers")
        logger.info("="*70)

        domain_data = {}

        # -------------------------------------------------------------------
        # 1.1 SVD Register Definitions + BLE User Guide Descriptions
        # -------------------------------------------------------------------
        logger.info("\n[1.1] Parsing SVD file and BLE User Guide...")

        # SVD file is in parent directory of SDK
        svd_path = self.sdk_path.parent / "gen" / "SVD" / "B6x.svd"
        ble_guide_path = self.sdk_path / "doc" / "SW_Spec" / "B6x_BLE芯片使用指南_V1.0.1.docx"

        # Extract register descriptions from BLE User Guide
        register_descriptions = {}
        if ble_guide_path.exists():
            logger.info(f"  Loading BLE User Guide: {ble_guide_path}")
            try:
                from core.document_parser import DocumentParser
                doc_parser = DocumentParser()
                parsed_doc = doc_parser.parse(str(ble_guide_path))

                if parsed_doc and parsed_doc.markdown_text:
                    import re
                    lines = parsed_doc.markdown_text.split('\n')
                    for i, line in enumerate(lines):
                        # Find register patterns (e.g., "BLE_CTRL_REG", "UART1_CTRL", etc.)
                        register_matches = re.findall(r'[A-Z]{2,10}_?(?:CTRL|REG|CFG|CON|CSR|CR|SR)', line)
                        if register_matches:
                            for reg_name in register_matches:
                                if reg_name not in register_descriptions:
                                    # Get context (current line + next 2 lines)
                                    context = line.strip()
                                    for j in range(1, min(3, len(lines) - i)):
                                        context += ' ' + lines[i + j].strip()
                                    # Store full context (no truncation)
                                    register_descriptions[reg_name] = context
                    logger.info(f"  Extracted {len(register_descriptions)} register descriptions")
            except Exception as e:
                logger.warning(f"  Failed to parse BLE User Guide: {e}")
        else:
            logger.warning(f"  BLE User Guide not found: {ble_guide_path}")

        # Parse SVD file
        if svd_path.exists():
            logger.info(f"  Parsing SVD file: {svd_path}")
            svd_parser = SVDParser()
            peripherals = svd_parser.parse_svd_file(str(svd_path))
            registers = svd_parser.get_all_registers(peripherals)

            # Convert to dict and merge with BLE User Guide descriptions
            registers_dict = []
            for reg in registers:
                reg_dict = asdict(reg)
                reg_name = reg.name

                # Try exact match first
                if reg_name in register_descriptions:
                    reg_dict['user_guide_notes'] = register_descriptions[reg_name]
                else:
                    # Try partial match (peripheral name or keyword match)
                    matched = False
                    for key, desc in register_descriptions.items():
                        if reg.peripheral in key or key in reg_name:
                            reg_dict['user_guide_notes'] = desc
                            matched = True
                            break

                    if not matched:
                        reg_dict['user_guide_notes'] = ""

                registers_dict.append(reg_dict)

            domain_data["registers"] = registers_dict
            logger.info(f"  Parsed {len(registers)} registers from {len(peripherals)} peripherals")
        else:
            logger.warning(f"  SVD file not found: {svd_path}")
            domain_data["registers"] = []

        # -------------------------------------------------------------------
        # 1.2 Hardware Constraints from JSON (SSOT: Single Source of Truth)
        # -------------------------------------------------------------------
        logger.info("\n[1.2] Loading hardware constraints from JSON (SSOT)...")

        # Load pin mux constraints
        io_map_data = self.load_constraint_json("io_map.json")
        if io_map_data:
            domain_data["pin_mux"] = io_map_data["pins"]
            domain_data["pin_mux_metadata"] = {
                "constraint_type": io_map_data["constraint_type"],
                "version": io_map_data["version"],
                "last_updated": io_map_data["last_updated"],
                "total_pins": io_map_data["total_pins"]
            }
            logger.info(f"  Loaded {io_map_data['total_pins']} pins from JSON")
        else:
            domain_data["pin_mux"] = []
            logger.warning("  Using empty pin_mux data (run build_excel_constraints.py)")

        # Load power consumption constraints
        power_data = self.load_constraint_json("power_consumption.json")
        if power_data:
            domain_data["power_consumption"] = power_data["modes"]
            # Ensure each entry has a 'name' field for domain node resolution
            for entry in domain_data["power_consumption"]:
                if "name" not in entry and "mode" in entry:
                    entry["name"] = entry["mode"]
            domain_data["power_consumption_metadata"] = {
                "constraint_type": power_data["constraint_type"],
                "version": power_data["version"],
                "last_updated": power_data["last_updated"],
                "total_modes": power_data["total_modes"]
            }
            logger.info(f"  Loaded {power_data['total_modes']} power modes from JSON")
        else:
            domain_data["power_consumption"] = []
            logger.warning("  Using empty power_consumption data (run build_excel_constraints.py)")

        # Load flash map constraints
        flash_map_data = self.load_constraint_json("flash_map.json")
        if flash_map_data:
            domain_data["memory_regions"] = flash_map_data["regions"]
            domain_data["memory_regions_metadata"] = {
                "constraint_type": flash_map_data["constraint_type"],
                "domain": flash_map_data["domain"],
                "version": flash_map_data["version"],
                "last_updated": flash_map_data["last_updated"],
                "total_regions": flash_map_data["total_regions"]
            }
            logger.info(f"  Loaded {flash_map_data['total_regions']} flash regions from JSON")
        else:
            domain_data["memory_regions"] = []
            logger.warning("  Using empty memory_regions data (run build_excel_constraints.py)")

        # Load SRAM map constraints
        sram_map_data = self.load_constraint_json("sram_map.json")
        if sram_map_data:
            domain_data["sram_regions"] = sram_map_data["regions"]
            domain_data["sram_regions_metadata"] = {
                "constraint_type": sram_map_data["constraint_type"],
                "domain": sram_map_data["domain"],
                "version": sram_map_data["version"],
                "last_updated": sram_map_data["last_updated"],
                "total_regions": sram_map_data["total_regions"]
            }
            logger.info(f"  Loaded {sram_map_data['total_regions']} SRAM regions from JSON")
        else:
            domain_data["sram_regions"] = []
            logger.warning("  Using empty sram_regions data (run build_excel_constraints.py)")

        # Load memory boundaries constraints
        mem_boundaries_data = self.load_constraint_json("memory_boundaries.json")
        if mem_boundaries_data:
            domain_data["memory_boundaries"] = mem_boundaries_data["boundaries"]
            domain_data["memory_boundaries_metadata"] = {
                "constraint_type": mem_boundaries_data["constraint_type"],
                "domain": mem_boundaries_data["domain"],
                "version": mem_boundaries_data["version"],
                "last_updated": mem_boundaries_data["last_updated"],
                "total_boundaries": mem_boundaries_data["total_boundaries"]
            }
            logger.info(f"  Loaded {mem_boundaries_data['total_boundaries']} memory boundaries from JSON")
        else:
            domain_data["memory_boundaries"] = []
            logger.warning("  Using empty memory_boundaries data (run build_excel_constraints.py)")

        # -------------------------------------------------------------------
        # 1.3 Linker Script (Memory Layout)
        # -------------------------------------------------------------------
        logger.info("\n[1.3] Parsing linker scripts...")
        linker_parser = LinkerScriptParser()

        linker_files = list(self.sdk_path.glob("core/**/*.ld"))
        logger.info(f"  Found {len(linker_files)} linker scripts")

        if linker_files:
            # Parse ALL linker scripts and merge memory regions
            all_regions = []
            region_names = set()

            for ld_file in linker_files:
                try:
                    result = linker_parser.parse_file(str(ld_file))

                    # Deduplicate by region name
                    for region in result.memory_regions:
                        region_dict = region.__dict__
                        region_name = region_dict.get('name', '')

                        if region_name and region_name not in region_names:
                            region_names.add(region_name)
                            all_regions.append(region_dict)

                    logger.info(f"  Parsed {ld_file.name}: {len(result.memory_regions)} regions")
                except Exception as e:
                    logger.warning(f"  Failed to parse {ld_file.name}: {e}")

            domain_data["memory_regions"] = all_regions
            logger.info(f"  Total unique memory regions: {len(all_regions)}")
        else:
            logger.warning("  No linker scripts found")

        # -------------------------------------------------------------------
        # 1.4 Interrupt Vectors
        # -------------------------------------------------------------------
        logger.info("\n[1.4] Parsing interrupt vectors...")
        interrupt_parser = InterruptVectorParser()

        startup_files = list(self.sdk_path.glob("core/**/startup*.s"))
        cmsis_headers = list(self.sdk_path.glob("core/*.h"))
        logger.info(f"  Found {len(startup_files)} startup files")

        # Parse all startup files and merge interrupt vectors
        all_interrupts = {}
        interrupt_names = set()

        for startup_file in startup_files:
            try:
                table = interrupt_parser.parse_startup_file(str(startup_file))

                # Merge vectors, deduplicate by name
                for vector in table.vectors:
                    if vector.name and vector.name not in interrupt_names:
                        interrupt_names.add(vector.name)
                        all_interrupts[vector.name] = vector.__dict__

                logger.info(f"  Parsed {startup_file.name}: {len(table.vectors)} vectors")
            except Exception as e:
                logger.warning(f"  Failed to parse {startup_file.name}: {e}")

        domain_data["interrupts"] = list(all_interrupts.values())
        logger.info(f"  Total unique interrupt vectors: {len(all_interrupts)}")

        # Store domain data
        self.domain_data["hardware"] = domain_data

        # Export domain-specific JSON
        self._export_domain_data("hardware", domain_data)

    # ========================================================================
    # Domain 2: SOC Drivers
    # ========================================================================

    def build_drivers_domain(self) -> None:
        """Build Domain 2: SOC Drivers."""
        logger.info("="*70)
        logger.info("Building Domain 2: SOC Drivers")
        logger.info("="*70)

        domain_data = {}

        # -------------------------------------------------------------------
        # 2.1 Driver APIs (TreeSitter)
        # -------------------------------------------------------------------
        logger.info("\n[2.1] Parsing driver APIs...")
        try:
            # Use module import to avoid scoping issues
            import core.tree_sitter_parser as ts_parser
            parser = ts_parser.TreeSitterCParser()
            if parser.initialize():
                # Parse drivers/
                headers = ts_parser.parse_directory_full(str(self.sdk_path / "drivers" / "api"), "*.h")
                sources = ts_parser.parse_directory_source(str(self.sdk_path / "drivers" / "src"), "*.c")

                # Parse usb/
                usb_headers = ts_parser.parse_directory_full(str(self.sdk_path / "usb" / "api"), "*.h")
                usb_sources = ts_parser.parse_directory_source(str(self.sdk_path / "usb" / "src"), "*.c")

                # Combine all
                all_headers = headers + usb_headers
                all_sources = sources + usb_sources

                matched = parser.match_declarations_to_implementations(all_headers, all_sources)

                domain_data["apis"] = [
                    {
                        "name": f.name,
                        "brief": f.brief_description,
                        "detailed": f.detailed_description,
                        "return_type": f.return_type,
                        "parameters": [{"type": p.type, "name": p.name} for p in f.parameters],
                        "file_path": make_relative(f.file_path, self.sdk_path) if f.file_path else "",
                        "has_implementation": f.has_implementation
                    }
                    for f in matched
                ]

                # Also extract macros and enums
                domain_data["macros"] = []
                macro_seen = {}  # name -> index in list
                domain_data["enums"] = []
                for header in all_headers:
                    for macro in header.macros:
                        if macro.name in macro_seen:
                            existing = domain_data["macros"][macro_seen[macro.name]]
                            variants = existing.get("variants", [])
                            variants.append(macro.value)
                            existing["variants"] = variants
                        else:
                            macro_seen[macro.name] = len(domain_data["macros"])
                            domain_data["macros"].append({
                                "name": macro.name,
                                "value": macro.value,
                                "brief": macro.brief
                            })
                    for enum in header.enums:
                        domain_data["enums"].append({
                            "name": enum.name,
                            "brief": enum.brief,
                            "values": [{"name": v.name, "value": v.value} for v in enum.values]
                        })

                logger.info(f"  Parsed {len(matched)} functions (including USB)")
            else:
                logger.warning("  Failed to initialize TreeSitter parser")
                domain_data["apis"] = []
                domain_data["macros"] = []
                domain_data["enums"] = []
        except Exception as e:
            logger.warning(f"  TreeSitter parsing failed: {e}")
            import traceback
            logger.debug(f"  Traceback: {traceback.format_exc()}")
            domain_data["apis"] = []
            domain_data["macros"] = []
            domain_data["enums"] = []

        # -------------------------------------------------------------------
        # 2.2 Struct Definitions
        # -------------------------------------------------------------------
        logger.info("\n[2.2] Extracting struct definitions...")
        try:
            struct_extractor = StructExtractor()

            # Extract from drivers/api (returns Dict[str, List[StructDefinition]])
            drivers_structs_dict = struct_extractor.extract_from_directory(
                str(self.sdk_path / "drivers" / "api"),
                "*.h"
            )

            # Extract from usb/api (returns Dict[str, List[StructDefinition]])
            usb_structs_dict = struct_extractor.extract_from_directory(
                str(self.sdk_path / "usb" / "api"),
                "*.h"
            )

            # Flatten dicts to list
            structs = []
            for struct_list in drivers_structs_dict.values():
                structs.extend(struct_list)
            for struct_list in usb_structs_dict.values():
                structs.extend(struct_list)

            domain_data["structs"] = [
                self._convert_paths_to_relative(s.to_dict())
                for s in structs
            ]
            logger.info(f"  Extracted {len(structs)} struct definitions (including USB)")
        except Exception as e:
            logger.warning(f"  Struct extraction failed: {e}")
            domain_data["structs"] = []

        # -------------------------------------------------------------------
        # 2.3 Dependencies
        # -------------------------------------------------------------------
        logger.info("\n[2.3] Extracting API dependencies...")
        try:
            from core.tree_sitter_parser import TreeSitterCParser
            from core.dependency_extractor import DoxygenDependencyExtractor

            dep_extractor = DoxygenDependencyExtractor()
            parser = TreeSitterCParser()

            if not parser.initialize():
                logger.warning("  Failed to initialize Tree-sitter parser")
                domain_data["dependencies"] = []
                domain_data["call_graph"] = {}
                domain_data["dependency_tree"] = {}
            else:
                # Re-parse driver headers to get FunctionDeclaration objects with Doxygen
                driver_api_dir = self.sdk_path / "drivers" / "api"
                driver_src_dir = self.sdk_path / "drivers" / "src"

                if driver_api_dir.exists():
                    logger.info(f"  Parsing driver headers from: {driver_api_dir}")

                    # Use existing parse function
                    from core.tree_sitter_parser import parse_directory_full, parse_directory_source
                    parse_results = parse_directory_full(str(driver_api_dir), "*.h")

                    logger.info(f"  Parsed {len(parse_results)} header files")

                    # Parse source files for implementation matching
                    source_results = []
                    if driver_src_dir.exists():
                        logger.info(f"  Parsing driver source files from: {driver_src_dir}")
                        source_results = parse_directory_source(str(driver_src_dir), "*.c")
                        logger.info(f"  Parsed {len(source_results)} source files")

                    # Match declarations with implementations
                    matched_functions = parser.match_declarations_to_implementations(parse_results, source_results)
                    logger.info(f"  Matched {len(matched_functions)} functions ({sum(1 for f in matched_functions if f.has_implementation)} with implementations)")

                    all_dependencies = []
                    call_graph = {}

                    # Extract dependencies from ALL APIs (no limit)
                    for i, func_decl in enumerate(matched_functions):
                        if (i + 1) % 20 == 0:
                            logger.info(f"  Progress: {i+1}/{len(matched_functions)} functions processed")

                        # Extract Doxygen-based dependencies
                        try:
                            dep = dep_extractor.extract_dependencies(func_decl)
                            all_dependencies.append({
                                "api_name": dep.api_name,
                                "pre_requisites": dep.pre_requisites,
                                "requires_peripheral_clock": dep.requires_peripheral_clock,
                                "requires_gpio_config": dep.requires_gpio_config,
                                "requires_interrupt_config": dep.requires_interrupt_config,
                                "requires_dma_config": dep.requires_dma_config,
                                "notes": dep.notes,
                                "see_also": dep.see_also,
                                "dependency_source": dep.dependency_source
                            })

                            # Extract function calls from implementation if available
                            if func_decl.has_implementation and func_decl.implementation_file:
                                impl_calls = self._extract_function_calls_from_implementation(
                                    parser,
                                    str(self.sdk_path / func_decl.implementation_file),
                                    func_decl.name
                                )
                                call_graph[func_decl.name] = impl_calls

                        except Exception as e:
                            logger.debug(f"  Failed to extract dependencies for {func_decl.name}: {e}")

                    # Build dependency tree
                    dependency_tree = {}
                    for dep in all_dependencies:
                        api_name = dep["api_name"]
                        dependency_tree[api_name] = {
                            "pre_requisites": dep["pre_requisites"],
                            "requires_peripheral_clock": dep["requires_peripheral_clock"],
                            "requires_gpio_config": dep["requires_gpio_config"],
                            "calls_in_implementation": call_graph.get(api_name, [])
                        }

                    domain_data["dependencies"] = all_dependencies
                    domain_data["call_graph"] = call_graph
                    domain_data["dependency_tree"] = dependency_tree

                    logger.info(f"  Extracted {len(all_dependencies)} API dependencies")
                    logger.info(f"  Built call graph with {len(call_graph)} nodes")
                else:
                    logger.warning(f"  Driver API directory not found: {driver_api_dir}")
                    domain_data["dependencies"] = []
                    domain_data["call_graph"] = {}
                    domain_data["dependency_tree"] = {}

        except Exception as e:
            logger.error(f"  Dependency extraction failed: {e}")
            import traceback
            logger.debug(traceback.format_exc())
            domain_data["dependencies"] = []
            domain_data["call_graph"] = {}
            domain_data["dependency_tree"] = {}

        self.domain_data["drivers"] = domain_data
        self._export_domain_data("drivers", domain_data)

    # ========================================================================
    # Domain 3: BLE Protocol Stack
    # ========================================================================

    def build_ble_domain(self) -> None:
        """Build Domain 3: BLE Protocol Stack."""
        logger.info("="*70)
        logger.info("Building Domain 3: BLE Protocol Stack")
        logger.info("="*70)

        domain_data = {}

        # -------------------------------------------------------------------
        # 3.1 BLE APIs
        # -------------------------------------------------------------------
        logger.info("\n[3.1] Parsing BLE APIs...")
        try:
            parser = TreeSitterCParser()
            if parser.initialize():
                headers = parse_directory_full(str(self.sdk_path / "ble" / "api"), "*.h")
                sources = parse_directory_source(str(self.sdk_path / "ble" / "src"), "*.c")

                matched = parser.match_declarations_to_implementations(headers, sources)

                domain_data["apis"] = [
                    {
                        "name": f.name,
                        "brief": f.brief_description,
                        "file_path": make_relative(f.file_path, self.sdk_path) if f.file_path else ""
                    }
                    for f in matched
                ]

                logger.info(f"  Parsed {len(matched)} BLE APIs")
            else:
                domain_data["apis"] = []
        except Exception as e:
            logger.warning(f"  BLE API parsing failed: {e}")
            domain_data["apis"] = []

        # -------------------------------------------------------------------
        # 3.2 GATT Profiles
        # -------------------------------------------------------------------
        logger.info("\n[3.2] Parsing GATT profiles...")
        try:
            profile_parser = ProfileParser()
            profiles = profile_parser.parse_directory(str(self.sdk_path / "ble" / "prf"))

            domain_data["profiles"] = [
                self._convert_paths_to_relative(p.to_dict())
                for p in profiles
            ]
            logger.info(f"  Parsed {len(profiles)} GATT profiles")
        except Exception as e:
            logger.warning(f"  Profile parsing failed: {e}")
            domain_data["profiles"] = []

        # -------------------------------------------------------------------
        # 3.3 BLE Error Codes
        # -------------------------------------------------------------------
        logger.info("\n[3.3] Parsing BLE error codes...")
        try:
            le_err_path = self.sdk_path / "ble" / "api" / "le_err.h"
            if le_err_path.exists():
                error_parser = BLEErrorCodeParser(str(le_err_path))
                error_codes = error_parser.parse_error_codes()

                # Convert ErrorCode objects to dicts for JSON serialization
                domain_data["error_codes"] = [
                    {
                        "name": ec.name,
                        "value": ec.value,
                        "hex_value": f"0x{ec.value:04X}",
                        "category": ec.category,
                        "description": ec.description,
                        "probable_causes": ec.probable_causes,
                        "solutions": ec.solutions,
                        "related_apis": ec.related_apis
                    }
                    for ec in error_codes
                ]
                logger.info(f"  Parsed {len(error_codes)} error codes")
            else:
                domain_data["error_codes"] = []
        except Exception as e:
            logger.warning(f"  Error code parsing failed: {e}")
            domain_data["error_codes"] = []

        # -------------------------------------------------------------------
        # 3.4 Mesh APIs
        # -------------------------------------------------------------------
        logger.info("\n[3.4] Parsing Mesh APIs...")
        try:
            mesh_api_dir = self.sdk_path / "mesh" / "api"
            if mesh_api_dir.exists():
                mesh_api_parser = MeshAPIParser(str(mesh_api_dir))
                mesh_apis = mesh_api_parser.parse()

                domain_data["mesh_apis"] = [
                    {
                        "name": api.name,
                        "return_type": api.return_type,
                        "parameters": api.parameters,
                        "brief": api.brief,
                        "header_file": api.header_file
                    }
                    for api in mesh_apis
                ]
                logger.info(f"  Parsed {len(mesh_apis)} Mesh APIs")
            else:
                logger.warning(f"  Mesh API directory not found: {mesh_api_dir}")
                domain_data["mesh_apis"] = []
        except Exception as e:
            logger.warning(f"  Mesh API parsing failed: {e}")
            domain_data["mesh_apis"] = []

        # -------------------------------------------------------------------
        # 3.5 Mesh Error Codes
        # -------------------------------------------------------------------
        logger.info("\n[3.5] Parsing Mesh error codes...")
        try:
            mesh_api_dir = self.sdk_path / "mesh" / "api"
            if mesh_api_dir.exists():
                mesh_error_parser = MeshErrorCodeParser(str(mesh_api_dir))
                mesh_error_codes = mesh_error_parser.parse()

                domain_data["mesh_error_codes"] = [
                    {
                        "code": err.code,
                        "hex_code": err.hex_code,
                        "name": err.name,
                        "group": err.group,
                        "description": err.description
                    }
                    for err in mesh_error_codes.values()
                ]
                logger.info(f"  Parsed {len(mesh_error_codes)} Mesh error codes")
            else:
                domain_data["mesh_error_codes"] = []
        except Exception as e:
            logger.warning(f"  Mesh error code parsing failed: {e}")
            domain_data["mesh_error_codes"] = []

        # -------------------------------------------------------------------
        # 3.6 Mesh Models
        # -------------------------------------------------------------------
        logger.info("\n[3.6] Parsing Mesh models...")
        try:
            mesh_model_dir = self.sdk_path / "mesh" / "model" / "api"
            if mesh_model_dir.exists():
                mesh_model_parser = MeshModelParser(str(mesh_model_dir))
                mesh_models = mesh_model_parser.parse()

                domain_data["mesh_models"] = [
                    {
                        "name": model.name,
                        "model_id": model.model_id,
                        "type": model.type,
                        "header_file": model.header_file,
                        "states": model.states,
                        "description": model.description
                    }
                    for model in mesh_models
                ]
                logger.info(f"  Parsed {len(mesh_models)} Mesh models")
            else:
                logger.warning(f"  Mesh model directory not found: {mesh_model_dir}")
                domain_data["mesh_models"] = []
        except Exception as e:
            logger.warning(f"  Mesh model parsing failed: {e}")
            domain_data["mesh_models"] = []

        phone_compat_data = self.load_constraint_json("phone_compatibility_issues.json")
        if phone_compat_data:
            domain_data["phone_compatibility_issues"] = phone_compat_data.get("brands", {})
            domain_data["phone_compatibility_metadata"] = {
                "constraint_type": phone_compat_data["constraint_type"],
                "domain": phone_compat_data["domain"],
                "version": phone_compat_data["version"],
                "last_updated": phone_compat_data["last_updated"],
                "total_issues": phone_compat_data["total_issues"],
                "total_brands": phone_compat_data["total_brands"],
                "issues_by_severity": phone_compat_data["issues_by_severity"]
            }
            logger.info(f"  Loaded {phone_compat_data['total_issues']} phone compatibility issues")
        else:
            domain_data["phone_compatibility_issues"] = {}
            logger.warning("  Using empty phone_compatibility_issues data")

        self.domain_data["ble"] = domain_data
        self._export_domain_data("ble", domain_data)

    # ========================================================================
    # Domain 4: Applications & Context
    # ========================================================================

    def build_applications_domain(self) -> None:
        """Build Domain 4: Applications & Context."""
        logger.info("="*70)
        logger.info("Building Domain 4: Applications & Context")
        logger.info("="*70)

        domain_data = {}

        # -------------------------------------------------------------------
        # 4.1 Examples Scanner
        # -------------------------------------------------------------------
        logger.info("\n[4.1] Scanning examples...")
        try:
            parser = TreeSitterCParser()
            if parser.initialize():
                scanner = ExampleScanner(parser)

                # Collect SDK APIs
                all_apis = set()
                for api_data in self.domain_data.get("drivers", {}).get("apis", []):
                    all_apis.add(api_data.get("name", ""))
                for api_data in self.domain_data.get("ble", {}).get("apis", []):
                    all_apis.add(api_data.get("name", ""))

                # Scan directories
                examples = []
                for scan_dir in ["examples", "projects"]:
                    scan_path = self.sdk_path / scan_dir
                    if scan_path.exists():
                        results = scanner.scan_directory(
                            str(scan_path),
                            all_apis,
                            f"{scan_dir}_type"
                        )
                        examples.extend(results)

                # Process ALL examples (no limit)
                logger.info(f"  Processing {len(examples)} example files...")

                processed_examples = []
                for i, ex in enumerate(examples):
                    if (i + 1) % 50 == 0:
                        logger.info(f"  Progress: {i+1}/{len(examples)} examples processed")

                    processed_examples.append({
                        "name": ex.example_name,
                        "file_path": make_relative(ex.file_path, self.sdk_path) if ex.file_path else "",
                        "used_apis": [fc.function_name for fc in ex.function_calls],
                        "type": ex.example_type,
                        "functions_defined": ex.functions_defined,
                        "included_headers": ex.included_headers,
                        "total_api_calls": len(ex.function_calls)
                    })

                domain_data["examples"] = processed_examples
                logger.info(f"  Total examples processed: {len(processed_examples)}")
            else:
                domain_data["examples"] = []
        except Exception as e:
            logger.warning(f"  Example scanning failed: {e}")
            domain_data["examples"] = []

        # -------------------------------------------------------------------
        # 4.2 API Manifest
        # -------------------------------------------------------------------
        logger.info("\n[4.2] Building API manifest...")
        try:
            manifest_builder = APIUsageManifestBuilder()
            if examples:
                manifest = manifest_builder.build_manifest(
                    examples, str(self.sdk_path), all_apis
                )
                manifest_path = self.output_dir / "api_manifest.json"
                manifest_path.parent.mkdir(parents=True, exist_ok=True)
                manifest.to_json(str(manifest_path))
                logger.info(f"  API manifest exported to: {manifest_path}")
            else:
                logger.warning("  No examples scanned, API manifest skipped")
        except Exception as e:
            logger.warning(f"  API manifest build failed: {e}")
            import traceback
            logger.debug(f"  Traceback: {traceback.format_exc()}")

        # -------------------------------------------------------------------
        # 4.3 Init Sequence Extraction (NEW - Domain 4 Enhancement)
        # -------------------------------------------------------------------
        logger.info("\n[4.3] Extracting initialization sequences...")
        try:
            # Collect known SDK APIs
            all_sdk_apis = set()
            for api_data in self.domain_data.get("drivers", {}).get("apis", []):
                all_sdk_apis.add(api_data.get("name", ""))
            for api_data in self.domain_data.get("ble", {}).get("apis", []):
                all_sdk_apis.add(api_data.get("name", ""))

            init_extractor = InitSequenceExtractor()

            # Extract from projects/
            projects_path = self.sdk_path / "projects"
            init_sequences = []
            if projects_path.exists():
                init_sequences = init_extractor.extract_all_projects(
                    str(projects_path),
                    all_sdk_apis
                )

            # Convert to dict format
            domain_data["init_sequences"] = [
                self._convert_paths_to_relative(seq.to_dict())
                for seq in init_sequences
            ]
            logger.info(f"  Extracted {len(init_sequences)} init sequences")
        except Exception as e:
            logger.warning(f"  Init sequence extraction failed: {e}")
            import traceback
            logger.debug(f"  Traceback: {traceback.format_exc()}")
            domain_data["init_sequences"] = []

        # -------------------------------------------------------------------
        # 4.4 Call Chain Extraction (Enhanced - Domain 4 Enhancement)
        # -------------------------------------------------------------------
        logger.info("\n[4.4] Extracting call chains...")
        try:
            # Collect known SDK APIs
            all_sdk_apis = set()
            for api_data in self.domain_data.get("drivers", {}).get("apis", []):
                all_sdk_apis.add(api_data.get("name", ""))
            for api_data in self.domain_data.get("ble", {}).get("apis", []):
                all_sdk_apis.add(api_data.get("name", ""))

            chain_extractor = CallChainExtractor()

            # Extract cross-file call chains from projects/
            projects_path = self.sdk_path / "projects"
            call_chains = {}
            if projects_path.exists():
                call_chains = chain_extractor.extract_all_project_chains(
                    str(projects_path),
                    all_sdk_apis
                )

            # Convert to dict format
            domain_data["call_chains"] = {
                name: self._convert_paths_to_relative(chain.to_dict())
                for name, chain in call_chains.items()
            }

            # Find patterns in each chain
            all_patterns = []
            for chain in call_chains.values():
                patterns = chain_extractor.find_patterns_in_chain(chain)
                for p in patterns:
                    all_patterns.append(self._convert_paths_to_relative(p.to_dict()))

            domain_data["call_chain_patterns"] = all_patterns
            logger.info(f"  Extracted {len(call_chains)} project call chains")
            logger.info(f"  Found {len(all_patterns)} call chain patterns")
        except Exception as e:
            logger.warning(f"  Call chain extraction failed: {e}")
            import traceback
            logger.debug(f"  Traceback: {traceback.format_exc()}")
            domain_data["call_chains"] = {}
            domain_data["call_chain_patterns"] = []

        # -------------------------------------------------------------------
        # 4.5 Project Configuration Parsing (NEW - Domain 4 Enhancement)
        # -------------------------------------------------------------------
        logger.info("\n[4.5] Parsing project configurations...")
        try:
            cfg_parser = CFGHeaderParser()

            # Parse projects/
            projects_path = self.sdk_path / "projects"
            project_configs = {}
            if projects_path.exists():
                project_configs = cfg_parser.parse_all_projects(str(projects_path))

            # Convert to dict format
            domain_data["project_configs"] = {
                name: self._convert_paths_to_relative(config.to_dict())
                for name, config in project_configs.items()
            }

            # Generate summary
            config_summary = []
            for name, config in project_configs.items():
                summary = self._convert_paths_to_relative(config.get_summary())
                summary["project_name"] = name
                summary["enabled_profiles"] = config.get_enabled_profiles()
                config_summary.append(summary)

            domain_data["config_summary"] = config_summary
            logger.info(f"  Parsed {len(project_configs)} project configurations")
        except Exception as e:
            logger.warning(f"  Project configuration parsing failed: {e}")
            import traceback
            logger.debug(f"  Traceback: {traceback.format_exc()}")
            domain_data["project_configs"] = {}
            domain_data["config_summary"] = []

        # -------------------------------------------------------------------
        # 4.6 Known Init Sequences (from YAML)
        # -------------------------------------------------------------------
        logger.info("\n[4.6] Loading known init sequences...")
        try:
            import yaml
            yaml_path = Path(__file__).parent.parent / "config" / "known_init_sequences.yaml"
            if yaml_path.exists():
                with open(yaml_path, 'r', encoding='utf-8') as f:
                    known_sequences = yaml.safe_load(f)
                domain_data["known_init_sequences"] = known_sequences
                logger.info(f"  Loaded {len(known_sequences)} known init sequence patterns")
            else:
                logger.warning(f"  Known sequences file not found: {yaml_path}")
                domain_data["known_init_sequences"] = {}
        except ImportError:
            logger.warning("  PyYAML not available, skipping known sequences")
            domain_data["known_init_sequences"] = {}
        except Exception as e:
            logger.warning(f"  Failed to load known sequences: {e}")
            domain_data["known_init_sequences"] = {}

        self.domain_data["applications"] = domain_data
        self._export_domain_data("applications", domain_data)

    # ========================================================================
    # Supplementary Data Export (hardware_constraints, project_dependencies)
    # ========================================================================

    def export_supplementary_data(self) -> None:
        """Export supplementary data files required by sdk_reference.py consumers."""
        logger.info("="*70)
        logger.info("Exporting Supplementary Data")
        logger.info("="*70)

        # CC-12: hardware_constraints.json
        logger.info("\n[cc-12] Exporting hardware_constraints.json...")
        cc12_ok = False
        io_map_path = self.sdk_path / "doc" / "SW_Spec" / "B6x_IO_MAP.xlsx"
        if io_map_path.exists():
            try:
                indexer = HardwareConstraintIndexer(
                    io_map_path=str(io_map_path),
                    flash_map_path=None
                )
                output_path = self.output_dir / "hardware_constraints.json"
                indexer.export_to_json(str(output_path))
                logger.info(f"  Exported: {output_path}")
                cc12_ok = True
            except Exception as e:
                logger.error(f"  hardware_constraints.json export failed: {e}")
                import traceback
                logger.debug(f"  Traceback: {traceback.format_exc()}")
        else:
            logger.warning(f"  B6x_IO_MAP.xlsx not found: {io_map_path}, skipping Excel-based generation")

        # CC-13: project_dependencies.json
        logger.info("\n[cc-13] Exporting project_dependencies.json...")
        cc13_ok = False
        try:
            dep_builder = DependencyGraphBuilder()

            # Collect known SDK APIs
            all_sdk_apis = set()
            for api_data in self.domain_data.get("drivers", {}).get("apis", []):
                all_sdk_apis.add(api_data.get("name", ""))
            for api_data in self.domain_data.get("ble", {}).get("apis", []):
                all_sdk_apis.add(api_data.get("name", ""))

            # Extract call chains for all projects
            chain_extractor = CallChainExtractor()
            projects_path = self.sdk_path / "projects"

            if projects_path.exists():
                call_chains = chain_extractor.extract_all_project_chains(
                    str(projects_path),
                    all_sdk_apis
                )

                for proj_name, chain in call_chains.items():
                    dep_builder.add_project(call_chain=chain)

            total_projects = len(dep_builder.graph.projects_call_chains)
            if total_projects > 0:
                output_path = self.output_dir / "project_dependencies.json"
                dep_builder.export_to_json(str(output_path))
                logger.info(f"  Exported: {output_path} ({total_projects} projects)")
                cc13_ok = True
            else:
                logger.warning("  No project call chains extracted, skipping export")
        except Exception as e:
            logger.error(f"  project_dependencies.json export failed: {e}")
            import traceback
            logger.debug(f"  Traceback: {traceback.format_exc()}")

        # Fallback: generate from domain JSON if primary methods failed
        if not cc12_ok or not cc13_ok:
            logger.info("\n[cc-12/13 fallback] Generating from domain JSON...")
            try:
                import importlib.util
                _gen_path = Path(__file__).parent / "generate_missing_data.py"
                _spec = importlib.util.spec_from_file_location("generate_missing_data", _gen_path)
                _mod = importlib.util.module_from_spec(_spec)
                _spec.loader.exec_module(_mod)
                if not cc12_ok:
                    _mod.generate_hardware_constraints(self.output_dir)
                    logger.info("  hardware_constraints.json generated from domain data (fallback)")
                if not cc13_ok:
                    _mod.generate_project_dependencies(self.output_dir)
                    logger.info("  project_dependencies.json generated from domain data (fallback)")
            except Exception as e:
                logger.error(f"  Fallback generation also failed: {e}")
                import traceback
                logger.debug(f"  Traceback: {traceback.format_exc()}")

    # ========================================================================
    # Cross-Domain Relations
    # ========================================================================

    def build_cross_domain_relations(self) -> None:
        """Build cross-domain relationship graph."""
        logger.info("="*70)
        logger.info("Building Cross-Domain Relations")
        logger.info("="*70)

        # Load domain data into relation mapper
        for domain, data in self.domain_data.items():
            self.relation_mapper.load_domain_data(domain, data)

        # Build relations
        logger.info("\n[r1] Hardware ↔ Drivers")
        self.relation_mapper.map_hardware_to_drivers()
        self.relation_mapper.map_from_api_register_config()
        self.relation_mapper.map_structs_to_registers()

        logger.info("\n[r2] Drivers ↔ BLE")
        self.relation_mapper.map_ble_to_drivers()

        logger.info("\n[r2b] BLE API Group Dependencies")
        self.relation_mapper.map_ble_api_groups()

        logger.info("\n[r3] Applications → All")
        self.relation_mapper.map_examples_to_apis()
        self.relation_mapper.map_init_sequences()

        # ========================================================================
        # P1 Enhancements: Source File Mapping & Profile Dependencies
        # ========================================================================

        logger.info("\n[r4] P1: API Declaration → Implementation Mapping")
        self.relation_mapper.map_declarations_to_implementations(str(self.sdk_path))

        logger.info("\n[r5] P1: Profile → API Dependencies (AST-based)")
        self.relation_mapper.map_profiles_to_apis_with_ast(str(self.sdk_path))

        # ========================================================================
        # P2 Enhancement: Power Consumption Mapping
        # ========================================================================

        logger.info("\n[r6] P2: Power Consumption → API Mapping")
        self.relation_mapper.map_power_to_apis(str(self.sdk_path))

        # ========================================================================
        # Cross-reference completion: ensure all domain nodes are covered
        # ========================================================================

        logger.info("\n[r8] Driver Structs → APIs (config struct usage)")
        self.relation_mapper.map_structs_to_apis()

        logger.info("\n[r9] Driver Macros → APIs (macro-to-API grouping)")
        self.relation_mapper.map_macros_to_apis()

        logger.info("\n[r10] BLE Error Codes → APIs (error return mapping)")
        self.relation_mapper.map_error_codes_to_apis()

        logger.info("\n[r11] BLE Profiles → APIs (profile implementation)")
        self.relation_mapper.map_profiles_to_apis()

        logger.info("\n[r12] BLE Mesh APIs → BLE APIs (mesh dependencies)")
        self.relation_mapper.map_mesh_apis_to_ble()

        logger.info("\n[r13] BLE Mesh Models → Mesh APIs (model dependencies)")
        self.relation_mapper.map_mesh_models_to_mesh_apis()

        logger.info("\n[r14] Remaining Registers → Peripherals")
        self.relation_mapper.map_remaining_registers()

        logger.info("\n[r7] Inferring transitive relations")
        self.relation_mapper.infer_transitive_relations()

        # Export relations
        logger.info("\n[export] Exporting relations...")
        self.relation_mapper.export_to_json(str(self.output_dir / "relations.json"))

        # Export per-domain relations
        for domain in ["hardware", "drivers", "ble", "applications"]:
            self.relation_mapper.export_for_domain(
                domain,
                str(self.output_dir / "relations" / f"relations_{domain}.json")
            )

        logger.info(f"  Total relations: {len(self.relation_mapper.graph.all_relations)}")

    # ========================================================================
    # Unified Knowledge Graph Export
    # ========================================================================

    def export_unified_graph(self) -> None:
        """Export unified knowledge graph JSON."""
        logger.info("="*70)
        logger.info("Exporting Unified Knowledge Graph")
        logger.info("="*70)

        unified_graph = {
            "metadata": {
                "version": "2.2.1",  # Version 2.2.1 uses relative paths for portability
                "build_date": datetime.now().isoformat(),
                "sdk_path": "<resolved_at_runtime>",  # Portable marker - SDK root resolved at runtime
                "schema": "four_dimensional",
                "changes": [
                    "Domain 4 (Applications) enhancement with INIT_SEQUENCE, CALL_CHAIN, PROJECT_CONFIG",
                    "Added init_sequence_extractor.py for extracting initialization patterns",
                    "Added cfg_header_parser.py for project configuration parsing",
                    "Enhanced call_chain_extractor.py with cross-file tracking",
                    "Added known_init_sequences.yaml with predefined patterns",
                    "Removed all hardcoded limits (linker scripts, startup files, examples)",
                    "Implemented full API dependency extraction with Tree-sitter call graphs",
                    "Full register descriptions (no truncation)",
                    "v2.2.1: Changed to relative paths for portability across environments"
                ]
            },
            "domains": {},
            "relations": self._convert_paths_to_relative(
                self.relation_mapper.graph.to_dict()
            ),
            "statistics": self._compute_statistics()
        }

        # Add domain data (convert paths in all domain data)
        for domain in ["hardware", "drivers", "ble", "applications"]:
            unified_graph["domains"][domain] = self._convert_paths_to_relative(
                self.domain_data.get(domain, {})
            )

        # Export
        output_path = self.output_dir / "knowledge_graph.json"
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(unified_graph, f, indent=2, ensure_ascii=False)

        logger.info(f"\nExported unified graph to: {output_path}")

    # ========================================================================
    # Utility Methods
    # ========================================================================

    def _export_domain_data(self, domain: str, data: Dict) -> None:
        """Export domain-specific JSON files with relative paths."""
        domain_dir = self.domain_output_dir / domain
        domain_dir.mkdir(parents=True, exist_ok=True)

        for key, value in data.items():
            # Export both lists and non-empty dicts
            if (isinstance(value, list) and len(value) > 0) or (isinstance(value, dict) and len(value) > 0):
                output_file = domain_dir / f"{key}.json"
                # Convert paths before exporting
                relative_value = self._convert_paths_to_relative(value)
                with open(output_file, 'w', encoding='utf-8') as f:
                    json.dump(relative_value, f, indent=2, ensure_ascii=False)
                logger.info(f"  Exported: {output_file}")

    def _extract_function_calls_from_implementation(self, parser, impl_file: str, func_name: str) -> List[str]:
        """
        Extract function calls from implementation file using Tree-sitter.

        Args:
            parser: Initialized TreeSitterCParser instance
            impl_file: Path to .c implementation file
            func_name: Function name to find calls within

        Returns:
            List of function names called in the implementation
        """
        try:
            impl_path = Path(impl_file)
            if not impl_path.exists():
                return []

            # Read source code and keep a bytes version for Tree-sitter slicing
            with open(impl_path, 'r', encoding='utf-8', errors='ignore') as f:
                source_code = f.read()

            # Convert to bytes for Tree-sitter byte offset slicing
            source_bytes = source_code.encode('utf-8')

            # Parse with Tree-sitter
            tree = parser.parser.parse(source_bytes)

            # FIX: Locate the target function's AST node first
            target_func_node = self._find_function_definition_node(parser, tree.root_node, source_bytes, func_name)

            # Extract calls only from within the target function's body
            calls = []
            if target_func_node:
                # Get function body using field access
                body_node = target_func_node.child_by_field_name('body')
                if body_node:
                    # Only search within the function body
                    self._find_call_expressions_recursive(body_node, source_bytes, calls)
                else:
                    logger.debug(f"  Function {func_name} has no body in {impl_file}")
            else:
                logger.debug(f"  Could not locate function '{func_name}' in {impl_file}")

            return calls

        except Exception as e:
            logger.debug(f"  Failed to extract calls from {impl_file}: {e}")
            return []

    def _find_function_definition_node(self, parser, root_node, source_bytes: bytes, func_name: str):
        """
        Locate a function's AST node by traversing the AST.

        Args:
            parser: Initialized TreeSitterCParser instance (not used, kept for compatibility)
            root_node: Tree-sitter root node
            source_bytes: Source code as bytes
            func_name: Function name to find

        Returns:
            Tree-sitter node for the function definition, or None
        """
        def find_function_recursive(node):
            """Recursively search for function_definition with matching name."""
            if node.type == 'function_definition':
                # Get the declarator
                declarator = node.child_by_field_name('declarator')
                if declarator:
                    # Get the identifier from the declarator
                    for child in declarator.children:
                        if child.type == 'identifier':
                            name = source_bytes[child.start_byte:child.end_byte].decode('utf-8')
                            if name == func_name:
                                return node
                            break

            # Recurse into children
            for child in node.children:
                result = find_function_recursive(child)
                if result:
                    return result

            return None

        return find_function_recursive(root_node)

    def _find_call_expressions_recursive(self, node, source_bytes: bytes, calls: List[str]) -> None:
        """Recursively find all function call expressions in AST.

        Args:
            node: Tree-sitter AST node
            source_bytes: Source code as bytes (for byte-offset slicing)
            calls: List to accumulate found function names
        """
        if node.type == 'call_expression':
            # Extract function name from call expression
            for child in node.children:
                if child.type == 'identifier':
                    # Slice bytes using Tree-sitter's byte offsets, then decode to string
                    func_name = source_bytes[child.start_byte:child.end_byte].decode('utf-8')
                    calls.append(func_name)
                    break

        # Recurse into children
        for child in node.children:
            self._find_call_expressions_recursive(child, source_bytes, calls)

    def _compute_statistics(self) -> Dict:
        """Compute overall statistics."""
        stats = {
            "domains": {},
            "total_entries": 0,
            "build_time": datetime.now().isoformat()
        }

        for domain, data in self.domain_data.items():
            entry_count = sum(len(v) if isinstance(v, list) else 1 for v in data.values())
            stats["domains"][domain] = {
                "entries": entry_count,
                "types": list(data.keys())
            }
            stats["total_entries"] += entry_count

        if self.relation_mapper.graph:
            stats["relations"] = len(self.relation_mapper.graph.all_relations)

        return stats

    def print_summary(self) -> None:
        """Print build summary."""
        stats = self._compute_statistics()

        logger.info("\n" + "="*70)
        logger.info("Build Summary")
        logger.info("="*70)
        logger.info(f"Total Entries: {stats['total_entries']}")

        for domain, domain_stats in stats.get("domains", {}).items():
            logger.info(f"  {domain.capitalize():12s}: {domain_stats['entries']:5d} entries")

        if "relations" in stats:
            logger.info(f"Relations:     {stats['relations']:5d} total")

        logger.info("="*70)


def check_constraint_files(output_dir: Path) -> bool:
    """
    Check if required constraint JSON files exist.

    Args:
        output_dir: Output directory where constraints are stored

    Returns:
        True if all required files exist, False otherwise
    """
    required_files = [
        "io_map.json",
        "flash_map.json",
        "sram_map.json",
        "memory_boundaries.json",
        "power_consumption.json"
    ]

    constraints_dir = output_dir / "constraints"
    missing = []

    for filename in required_files:
        if not (constraints_dir / filename).exists():
            missing.append(filename)

    if missing:
        logger.error("=" * 70)
        logger.error("MISSING CONSTRAINT FILES")
        logger.error("=" * 70)
        logger.error("The following constraint JSON files are missing:")
        for f in missing:
            logger.error(f"  - {f}")
        logger.error("")
        logger.error("Please run: python scripts/build_excel_constraints.py")
        logger.error("")
        return False

    logger.info("All required constraint files found.")
    return True


# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    """Main entry point."""
    import argparse

    parser = argparse.ArgumentParser(description="Build 4D Knowledge Graph")
    parser.add_argument("--sdk-path", default=str(SDK_PATH), help="SDK root path")
    parser.add_argument("--output-dir", default=str(OUTPUT_DIR), help="Output directory")
    parser.add_argument("--domain", choices=["hardware", "drivers", "ble", "applications"],
                        help="Build only specific domain")
    parser.add_argument("--stats", action="store_true", help="Show statistics only")
    parser.add_argument("--test", action="store_true", help="Test mode (dry run)")

    args = parser.parse_args()

    sdk_path = Path(args.sdk_path)
    output_dir = Path(args.output_dir)

    if not sdk_path.exists():
        logger.error(f"SDK path not found: {sdk_path}")
        sys.exit(1)

    logger.info(f"SDK Path: {sdk_path}")
    logger.info(f"Output: {output_dir}")

    # Check if required constraint files exist (SSOT validation)
    if not check_constraint_files(output_dir):
        logger.warning("Continuing with partial data...")
        logger.warning("Some domains may have incomplete constraint information.")

    if args.stats:
        # Show existing stats
        logger.info("Statistics mode - showing existing data...")
        return

    if args.test:
        logger.info("Test mode - dry run")
        return

    # Create builder
    builder = FourDimensionalKnowledgeGraphBuilder(sdk_path, output_dir)

    try:
        # Build domains (or single domain)
        if args.domain:
            if args.domain == "hardware":
                builder.build_hardware_domain()
            elif args.domain == "drivers":
                builder.build_drivers_domain()
            elif args.domain == "ble":
                builder.build_ble_domain()
            elif args.domain == "applications":
                builder.build_applications_domain()
        else:
            # Build all in dependency order
            builder.build_hardware_domain()
            builder.build_drivers_domain()
            builder.build_ble_domain()
            builder.build_applications_domain()

            # Build cross-domain relations
            builder.build_cross_domain_relations()

            # Export supplementary data files
            builder.export_supplementary_data()

            # Export unified graph
            builder.export_unified_graph()

        # Print summary
        builder.print_summary()

        logger.info("\nBuild completed successfully!")

    except Exception as e:
        logger.exception(f"Build failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
