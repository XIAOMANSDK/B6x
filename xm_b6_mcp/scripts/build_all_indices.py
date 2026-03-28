#!/usr/bin/env python3
"""
Build All Indices - Complete Rebuild
====================================

Master script to rebuild all indices from scratch.

Usage:
    python scripts/build_all_indices.py

This script will:
1. Clean all existing indices (Whoosh index, JSON constraints, parsed docs)
2. Build Excel constraints (io_map.json, power_consumption.json, etc.)
3. Parse all SDK documents (Word/PDF/Excel)
4. Classify documents into 5 domains
5. Index documents to Whoosh
6. Parse C code with Tree-sitter and index APIs to Whoosh
7. Build 4D knowledge graph from all SDK domains (NEW)

Author: B6x MCP Server Team
Version: 1.2.0 - Added knowledge graph build step
"""

import sys
import json
import shutil
import logging
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)


def get_paths():
    """Get all relevant paths."""
    mcp_root = Path(__file__).parent.parent
    sdk_root = Path(__file__).parent.parent.parent

    return {
        "mcp_root": mcp_root,
        "sdk_root": sdk_root,
        "index_dir": mcp_root / "data" / "whoosh_index",
        "constraints_dir": mcp_root / "data" / "constraints",
        "parsed_docs_file": mcp_root / "data" / "parsed_documents.json",
        "index_report": mcp_root / "data" / "document_index_report.json",
        "excel_script": mcp_root / "scripts" / "build_excel_constraints.py",
        "document_script": mcp_root / "scripts" / "build_document_index.py",
        "kg_script": mcp_root / "scripts" / "build_index_v2.py"
    }


def clean_directory(path: Path, name: str) -> bool:
    """Clean a directory by removing and recreating it."""
    if path.exists():
        try:
            logger.info(f"Removing {name}: {path}")
            shutil.rmtree(path)
        except Exception as e:
            logger.error(f"Failed to remove {name}: {e}")
            return False

    try:
        logger.info(f"Creating {name}: {path}")
        path.mkdir(parents=True, exist_ok=True)
        return True
    except Exception as e:
        logger.error(f"Failed to create {name}: {e}")
        return False


def clean_file(path: Path, name: str) -> bool:
    """Clean a file by removing it."""
    if not path.exists():
        logger.info(f"File not found (skipping): {name}")
        return True

    try:
        logger.info(f"Removing {name}: {path}")
        path.unlink()
        return True
    except Exception as e:
        logger.error(f"Failed to remove {name}: {e}")
        return False


def clean_indices(paths: dict, clean_targets: list = None) -> bool:
    """
    Clean indices with granular control.

    Args:
        paths: Path dictionary
        clean_targets: List of targets to clean.
            Options: ["all", "whoosh", "constraints", "documents",
                     "cache", "domain:hardware", "domain:drivers",
                     "domain:ble", "domain:applications"]
            If None, cleans nothing (incremental build mode).

    Returns:
        True if successful
    """
    if clean_targets is None:
        logger.info("No clean targets specified (incremental build)")
        return True

    logger.info("=" * 70)
    logger.info("CLEANUP: Removing Specified Targets")
    logger.info("=" * 70)
    logger.info(f"Targets: {', '.join(clean_targets)}")
    logger.info("")

    results = []

    for target in clean_targets:
        if target == "all":
            # Full clean (legacy behavior)
            logger.info("Cleaning ALL targets...")

            # Clean Whoosh index directory
            results.append(clean_directory(
                paths["index_dir"],
                "Whoosh index directory"
            ))

            # Clean constraints directory (but keep the directory)
            if paths["constraints_dir"].exists():
                for json_file in paths["constraints_dir"].glob("*.json"):
                    results.append(clean_file(json_file, f"constraint file: {json_file.name}"))

            # Clean parsed documents
            results.append(clean_file(
                paths["parsed_docs_file"],
                "parsed documents file"
            ))

            # Clean index report
            results.append(clean_file(
                paths["index_report"],
                "index report file"
            ))

            # Clean build cache
            cache_file = paths["mcp_root"] / "data" / "file_hashes.json"
            results.append(clean_file(cache_file, "build cache"))

        elif target == "whoosh":
            logger.info("Cleaning Whoosh index...")
            results.append(clean_directory(
                paths["index_dir"],
                "Whoosh index directory"
            ))

        elif target == "constraints":
            logger.info("Cleaning constraints...")
            if paths["constraints_dir"].exists():
                for json_file in paths["constraints_dir"].glob("*.json"):
                    results.append(clean_file(json_file, f"constraint file: {json_file.name}"))

        elif target == "documents":
            logger.info("Cleaning documents...")
            results.append(clean_file(
                paths["parsed_docs_file"],
                "parsed documents file"
            ))
            results.append(clean_file(
                paths["index_report"],
                "index report file"
            ))

        elif target == "cache":
            logger.info("Cleaning build cache...")
            cache_file = paths["mcp_root"] / "data" / "file_hashes.json"
            results.append(clean_file(cache_file, "build cache"))

        elif target.startswith("domain:"):
            # Clean specific domain
            domain = target.split(":")[1]
            logger.info(f"Cleaning domain: {domain}")
            domain_dir = paths["mcp_root"] / "data" / "domain" / domain
            if domain_dir.exists():
                for json_file in domain_dir.glob("*.json"):
                    results.append(clean_file(json_file, f"{domain} domain file"))
            else:
                logger.info(f"  Domain directory not found: {domain_dir}")

    success = all(results)

    if success:
        logger.info("[OK] Clean operations completed successfully")
    else:
        logger.error("[FAIL] Some clean operations failed")

    logger.info("")
    return success


def run_script(script_path: Path, name: str) -> bool:
    """Run a Python script as a subprocess."""
    if not script_path.exists():
        logger.error(f"Script not found: {script_path}")
        return False

    logger.info("=" * 70)
    logger.info(f"Running: {name}")
    logger.info("=" * 70)

    try:
        # Set PYTHONIOENCODING to UTF-8 for Windows compatibility
        import os
        env = os.environ.copy()
        env['PYTHONIOENCODING'] = 'utf-8'

        result = subprocess.run(
            [sys.executable, str(script_path)],
            cwd=str(script_path.parent.parent),
            capture_output=False,
            text=True,
            env=env
        )

        if result.returncode == 0:
            logger.info(f"[OK] {name} completed successfully")
            logger.info("")
            return True
        else:
            logger.error(f"[FAIL] {name} failed with return code {result.returncode}")
            logger.error("")
            return False

    except Exception as e:
        logger.error(f"[FAIL] Error running {name}: {e}")
        logger.error("")
        return False


def build_excel_constraints(paths: dict) -> bool:
    """Build Excel constraint JSON files."""
    return run_script(
        paths["excel_script"],
        "Excel Constraints Builder"
    )


def build_document_index(paths: dict) -> bool:
    """Build document index (parse, classify, index to Whoosh)."""
    return run_script(
        paths["document_script"],
        "Document Index Builder"
    )


def _count_cached_apis(cache_manager, category: str, module: str) -> int:
    """
    Estimate number of cached APIs for a module.

    Args:
        cache_manager: BuildCacheManager instance
        category: File category (e.g., "c_headers")
        module: Module name (e.g., "driver", "ble")

    Returns:
        Estimated count of cached APIs
    """
    cached_files = cache_manager.get_cached_files(category)
    count = 0

    for rel_path, file_info in cached_files.items():
        # Simple heuristic: estimate based on file path
        if module == "driver" and "driver" in rel_path.lower():
            count += 50  # Average APIs per driver file
        elif module == "ble" and "ble" in rel_path.lower():
            count += 30  # Average APIs per BLE file

    return count


def classify_peripheral(file_path: str) -> str:
    """
    Classify peripheral type from file path.

    Args:
        file_path: Header file path

    Returns:
        Peripheral name (e.g., 'UART', 'GPIO', 'BLE', 'OTHER')
    """
    from pathlib import Path
    file_name = Path(file_path).stem.upper()

    if 'UART' in file_name:
        return 'UART'
    elif 'I2C' in file_name:
        return 'I2C'
    elif 'SPI' in file_name:
        return 'SPI'
    elif 'GPIO' in file_name or 'IOPAD' in file_name:
        return 'GPIO'
    elif 'DMA' in file_name:
        return 'DMA'
    elif 'TIMER' in file_name or 'CTMR' in file_name or 'ATMR' in file_name or 'BTMR' in file_name:
        return 'TIMER'
    elif 'BLE' in file_name or 'GATT' in file_name or 'L2C' in file_name or 'GAP' in file_name or 'ATT' in file_name:
        return 'BLE'
    elif 'RCC' in file_name or 'RCO' in file_name:
        return 'RCC'
    elif 'FLASH' in file_name:
        return 'FLASH'
    else:
        return 'OTHER'


def build_api_index(paths: dict, cache_manager=None, incremental=False) -> dict:
    """
    Build API index from C source code using Tree-sitter.

    This step connects the Tree-sitter parser output to Whoosh index,
    enabling API search functionality in search_sdk().

    Process:
        1. Parse C header files with Tree-sitter (incremental if cache_manager provided)
        2. Build Whoosh index from parse results
        3. Index driver APIs, BLE APIs, and other SDK APIs

    Args:
        paths: Dictionary of paths
        cache_manager: Optional BuildCacheManager for incremental builds
        incremental: If True, use cache_manager for incremental build

    Returns:
        dict: {
            'success': bool,
            'stats': {
                'total_indexed': int,
                'driver_apis': int,
                'ble_apis': int,
                'by_peripheral': dict,
                'by_type': dict,
                'incremental': bool,
                'files_processed': int,
                'files_cached': int
            }
        }
    """
    logger.info("=" * 70)
    logger.info("Building API Index from C Source Code")
    if incremental and cache_manager:
        logger.info("Mode: INCREMENTAL (using cache)")
    else:
        logger.info("Mode: FULL BUILD")
    logger.info("=" * 70)

    try:
        # Import Tree-sitter parser and Whoosh indexer
        from core.tree_sitter_parser import parse_directory_full
        from core.whoosh_indexer import WhooshIndexBuilder
        import tempfile
        import shutil

        # Paths to API directories
        driver_api_dir = paths["sdk_root"] / "drivers" / "api"
        ble_api_dir = paths["sdk_root"] / "ble" / "api"
        index_dir = paths["index_dir"]

        # Ensure Whoosh index exists
        index_dir.mkdir(parents=True, exist_ok=True)

        # Initialize Whoosh index builder
        builder = WhooshIndexBuilder(str(index_dir))
        if not builder.open_index():
            logger.error("Failed to open Whoosh index")
            return {
                "success": False,
                "stats": {
                    "total_indexed": 0,
                    "by_peripheral": {},
                    "by_type": {"functions": 0, "macros": 0, "enums": 0}
                }
            }

        total_indexed = 0
        total_files_processed = 0
        total_files_cached = 0

        # 1. Parse and index Driver APIs
        if driver_api_dir.exists():
            logger.info(f"Checking driver APIs in: {driver_api_dir}")

            try:
                # Build file list
                driver_files = [
                    (str(f.relative_to(paths["sdk_root"])), f)
                    for f in driver_api_dir.glob("*.h")
                ]
                logger.info(f"Found {len(driver_files)} driver header files")

                # Incremental or full build?
                if incremental and cache_manager:
                    # Get changed files only
                    changed_files = cache_manager.get_changed_files("c_headers", driver_files)
                    total_files_cached = len(driver_files) - len(changed_files)

                    if changed_files:
                        logger.info(f"Found {len(changed_files)} changed driver API files")
                        total_files_processed += len(changed_files)

                        # Create temporary directory for changed files
                        with tempfile.TemporaryDirectory() as temp_dir:
                            temp_path = Path(temp_dir)

                            # Copy changed files preserving structure
                            for rel_path, abs_path in changed_files:
                                dest = temp_path / rel_path
                                dest.parent.mkdir(parents=True, exist_ok=True)
                                shutil.copy2(abs_path, dest)

                            # Parse only changed files
                            driver_parse_results = parse_directory_full(str(temp_path), "*.h")
                            logger.info(f"Parsed {len(driver_parse_results)} changed header files")

                            # Build Whoosh index from parse results
                            count = builder.build_from_parse_results(driver_parse_results, module="driver")
                            total_indexed += count
                            logger.info(f"Indexed {count} driver API entries to Whoosh")

                            # Update cache
                            for rel_path, abs_path in changed_files:
                                cache_manager.update_file_cache(
                                    "c_headers", abs_path, rel_path,
                                    outputs=["whoosh_index"]
                                )
                    else:
                        logger.info("No driver API files changed, using cache")
                        # Count cached APIs
                        cached_count = _count_cached_apis(cache_manager, "c_headers", "driver")
                        total_indexed += cached_count
                else:
                    # Full build
                    logger.info("Full build: parsing all driver API files")
                    total_files_processed += len(driver_files)

                    driver_parse_results = parse_directory_full(str(driver_api_dir), "*.h")
                    logger.info(f"Tree-sitter parsed {len(driver_parse_results)} driver header files")

                    # Build Whoosh index from parse results
                    count = builder.build_from_parse_results(driver_parse_results, module="driver")
                    total_indexed += count
                    logger.info(f"Indexed {count} driver API entries to Whoosh")

                    # Update cache if provided
                    if cache_manager:
                        for rel_path, abs_path in driver_files:
                            cache_manager.update_file_cache(
                                "c_headers", abs_path, rel_path,
                                outputs=["whoosh_index"]
                            )

            except Exception as e:
                logger.warning(f"Failed to parse driver APIs: {e}")
        else:
            logger.warning(f"Driver API directory not found: {driver_api_dir}")

        # 2. Parse and index BLE APIs
        if ble_api_dir.exists():
            logger.info(f"Checking BLE APIs in: {ble_api_dir}")

            try:
                # Build file list
                ble_files = [
                    (str(f.relative_to(paths["sdk_root"])), f)
                    for f in ble_api_dir.glob("*.h")
                ]
                logger.info(f"Found {len(ble_files)} BLE header files")

                # Incremental or full build?
                if incremental and cache_manager:
                    # Get changed files only
                    changed_files = cache_manager.get_changed_files("c_headers", ble_files)
                    total_files_cached += len(ble_files) - len(changed_files)

                    if changed_files:
                        logger.info(f"Found {len(changed_files)} changed BLE API files")
                        total_files_processed += len(changed_files)

                        # Create temporary directory for changed files
                        with tempfile.TemporaryDirectory() as temp_dir:
                            temp_path = Path(temp_dir)

                            # Copy changed files preserving structure
                            for rel_path, abs_path in changed_files:
                                dest = temp_path / rel_path
                                dest.parent.mkdir(parents=True, exist_ok=True)
                                shutil.copy2(abs_path, dest)

                            # Parse only changed files
                            ble_parse_results = parse_directory_full(str(temp_path), "*.h")
                            logger.info(f"Parsed {len(ble_parse_results)} changed header files")

                            # Build Whoosh index from parse results
                            count = builder.build_from_parse_results(ble_parse_results, module="ble")
                            total_indexed += count
                            logger.info(f"Indexed {count} BLE API entries to Whoosh")

                            # Update cache
                            for rel_path, abs_path in changed_files:
                                cache_manager.update_file_cache(
                                    "c_headers", abs_path, rel_path,
                                    outputs=["whoosh_index"]
                                )
                    else:
                        logger.info("No BLE API files changed, using cache")
                        # Count cached APIs
                        cached_count = _count_cached_apis(cache_manager, "c_headers", "ble")
                        total_indexed += cached_count
                else:
                    # Full build
                    logger.info("Full build: parsing all BLE API files")
                    total_files_processed += len(ble_files)

                    ble_parse_results = parse_directory_full(str(ble_api_dir), "*.h")
                    logger.info(f"Tree-sitter parsed {len(ble_parse_results)} BLE header files")

                    # Build Whoosh index from parse results
                    count = builder.build_from_parse_results(ble_parse_results, module="ble")
                    total_indexed += count
                    logger.info(f"Indexed {count} BLE API entries to Whoosh")

                    # Update cache if provided
                    if cache_manager:
                        for rel_path, abs_path in ble_files:
                            cache_manager.update_file_cache(
                                "c_headers", abs_path, rel_path,
                                outputs=["whoosh_index"]
                            )

            except Exception as e:
                logger.warning(f"Failed to parse BLE APIs: {e}")
        else:
            logger.warning(f"BLE API directory not found: {ble_api_dir}")

        # Collect detailed statistics
        api_stats = {
            "total_indexed": total_indexed,
            "by_peripheral": {},
            "by_type": {
                "functions": total_indexed,
                "macros": 0,
                "enums": 0
            },
            "incremental": incremental and cache_manager is not None,
            "files_processed": total_files_processed,
            "files_cached": total_files_cached
        }

        # Classify by peripheral (only if we have parse results)
        all_results = []
        if 'driver_parse_results' in locals():
            all_results.extend(driver_parse_results)
        if 'ble_parse_results' in locals():
            all_results.extend(ble_parse_results)

        for result in all_results:
            peripheral = classify_peripheral(result.file_path)
            api_stats["by_peripheral"][peripheral] = \
                api_stats["by_peripheral"].get(peripheral, 0) + len(result.functions)

        # Summary
        logger.info("")
        logger.info(f"API Index Statistics:")
        logger.info(f"  Total APIs: {api_stats['total_indexed']}")
        logger.info(f"  Functions: {api_stats['by_type']['functions']}")

        if api_stats["incremental"]:
            logger.info(f"  Build Mode: INCREMENTAL")
            logger.info(f"  Files Processed: {api_stats['files_processed']}")
            logger.info(f"  Files Cached: {api_stats['files_cached']}")
        else:
            logger.info(f"  Build Mode: FULL")

        if api_stats["by_peripheral"]:
            logger.info(f"  By Peripheral:")
            for periph, count in sorted(api_stats["by_peripheral"].items(),
                                     key=lambda x: x[1], reverse=True):
                logger.info(f"    {periph}: {count}")

        logger.info("")
        logger.info(f"[OK] API Index Build Complete - {total_indexed} entries indexed")
        logger.info("")

        return {
            "success": total_indexed > 0,
            "stats": api_stats
        }

    except Exception as e:
        logger.error(f"[FAIL] API Index Build failed: {e}")
        logger.error("")
        return {
            "success": False,
            "stats": {
                "total_indexed": 0,
                "by_peripheral": {},
                "by_type": {"functions": 0, "macros": 0, "enums": 0},
                "incremental": False,
                "files_processed": 0,
                "files_cached": 0
            }
        }


def build_knowledge_graph(paths: dict) -> bool:
    """
    Build 4D knowledge graph from all SDK domains.

    This step builds the unified knowledge graph that connects
    hardware constraints, driver APIs, BLE protocols, and examples.

    Returns:
        bool: True if successful, False otherwise
    """
    return run_script(
        paths["kg_script"],
        "4D Knowledge Graph Builder"
    )


def build_error_api_mapping(paths: dict) -> dict:
    """
    Build error-to-API mapping by analyzing SDK source code and merging with config.

    This step:
    1. Analyzes SDK C source files for error return statements
    2. Merges with manual configuration from config/error_api_mapping.yaml
    3. Exports the final mapping to data/relations/error_api_mapping.json

    Args:
        paths: Dictionary of paths

    Returns:
        dict: {
            'success': bool,
            'stats': {
                'errors_with_apis': int,
                'total_mappings': int,
                'sdk_analyzed': bool,
                'config_merged': bool
            }
        }
    """
    logger.info("=" * 70)
    logger.info("Building Error-to-API Mapping")
    logger.info("=" * 70)

    try:
        from core.error_return_analyzer import ErrorReturnAnalyzer

        sdk_path = paths["sdk_root"]
        config_path = paths["mcp_root"] / "config" / "error_api_mapping.yaml"
        output_path = paths["mcp_root"] / "data" / "relations" / "error_api_mapping.json"

        # Ensure output directory exists
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Initialize analyzer
        analyzer = ErrorReturnAnalyzer(str(sdk_path))

        # Analyze SDK source code
        logger.info("Analyzing SDK source code for error returns...")
        sdk_result = analyzer.analyze_all()
        sdk_analyzed = len(sdk_result) > 0

        # Merge with config file if it exists
        config_merged = False
        if config_path.exists():
            logger.info(f"Merging with config file: {config_path}")
            final_result = analyzer.merge_with_config(str(config_path))
            config_merged = True
        else:
            logger.info("No config file found, using SDK analysis only")
            final_result = sdk_result

        # Export to JSON (write the merged result directly, not via analyzer)
        if final_result:
            with open(output_path, 'w', encoding='utf-8') as f:
                json.dump(final_result, f, indent=2, ensure_ascii=False)
            logger.info(f"Exported mapping to {output_path}")

        # Calculate statistics
        total_mappings = sum(len(apis) for apis in final_result.values())
        errors_with_apis = len([e for e, apis in final_result.items() if apis])

        stats = {
            'errors_with_apis': errors_with_apis,
            'total_mappings': total_mappings,
            'sdk_analyzed': sdk_analyzed,
            'config_merged': config_merged
        }

        logger.info("")
        logger.info(f"Error-to-API Mapping Statistics:")
        logger.info(f"  Errors with API mappings: {errors_with_apis}")
        logger.info(f"  Total API mappings: {total_mappings}")
        logger.info(f"  SDK analyzed: {sdk_analyzed}")
        logger.info(f"  Config merged: {config_merged}")
        logger.info("")
        logger.info(f"[OK] Error-to-API Mapping Complete")
        logger.info("")

        return {
            "success": True,
            "stats": stats
        }

    except Exception as e:
        logger.error(f"[FAIL] Error-to-API Mapping failed: {e}")
        logger.error("")
        return {
            "success": False,
            "stats": {
                'errors_with_apis': 0,
                'total_mappings': 0,
                'sdk_analyzed': False,
                'config_merged': False,
                'error': str(e)
            }
        }


def generate_final_report(paths: dict, results: dict) -> dict:
    """Generate final build report."""
    report = {
        "build_date": datetime.now().isoformat(),
        "build_version": "1.2.0",
        "build_type": "full_rebuild",
        "steps": results,
        "summary": {
            "cleanup_success": results.get("cleanup", False),
            "excel_constraints_success": results.get("excel_constraints", False),
            "document_index_success": results.get("document_index", False),
            "api_index_success": (
                results.get("api_index", {}).get("success", False)
                if isinstance(results.get("api_index"), dict)
                else results.get("api_index", False)
            ),
            "knowledge_graph_success": results.get("knowledge_graph", False),
            "error_api_mapping_success": (
                results.get("error_api_mapping", {}).get("success", False)
                if isinstance(results.get("error_api_mapping"), dict)
                else results.get("error_api_mapping", False)
            ),
            "overall_success": all(
                v.get("success", True) if isinstance(v, dict) else v
                for v in results.values()
                if v is not None  # Skip None values (treat skipped steps as successful)
            )
        }
    }

    # Load detailed reports if available
    if paths["index_report"].exists():
        try:
            with open(paths["index_report"], 'r', encoding='utf-8') as f:
                doc_report = json.load(f)
                report["document_index_details"] = doc_report
        except Exception as e:
            logger.warning(f"Could not load document index report: {e}")

    # Count constraint files
    if paths["constraints_dir"].exists():
        constraint_files = list(paths["constraints_dir"].glob("*.json"))
        # Exclude build_report.json from count
        constraint_files = [f for f in constraint_files if f.name != "build_report.json"]
        report["summary"]["constraint_files_generated"] = len(constraint_files)
        report["summary"]["constraint_files"] = [f.name for f in constraint_files]

    # Add document index details
    if "document_index_details" in report:
        doc_details = report["document_index_details"]
        if isinstance(doc_details, dict):
            # Extract document statistics
            doc_summary = doc_details.get("summary", {})
            report["summary"]["documents_parsed"] = doc_summary.get("documents_parsed", 0)
            report["summary"]["documents_indexed"] = doc_summary.get("documents_indexed", 0)
            report["summary"]["excel_constraints"] = doc_summary.get("excel_constraints", 0)

    # Add API index details
    if results.get("api_index") and isinstance(results["api_index"], dict):
        api_stats = results["api_index"].get("stats", {})
        report["summary"]["total_apis_indexed"] = \
            api_stats.get("total_indexed", 0)
        report["api_index_details"] = api_stats

    # Add error-to-API mapping details
    if results.get("error_api_mapping") and isinstance(results["error_api_mapping"], dict):
        mapping_stats = results["error_api_mapping"].get("stats", {})
        report["summary"]["errors_with_apis"] = mapping_stats.get("errors_with_apis", 0)
        report["summary"]["total_api_mappings"] = mapping_stats.get("total_mappings", 0)
        report["error_api_mapping_details"] = mapping_stats

    return report


def save_report(report: dict, report_path: Path):
    """Save final build report."""
    try:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        with open(report_path, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        logger.info(f"Report saved to: {report_path}")
    except Exception as e:
        logger.error(f"Failed to save report: {e}")


def main():
    """Main build process."""
    import argparse

    parser = argparse.ArgumentParser(
        description="B6x SDK Index Builder - Build All Indices",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Full rebuild (default behavior)
  python scripts/build_all_indices.py

  # Incremental build (use cache)
  python scripts/build_all_indices.py --incremental

  # Clean only Whoosh index and rebuild
  python scripts/build_all_indices.py --clean whoosh

  # Clean specific domain and rebuild
  python scripts/build_all_indices.py --clean domain:drivers

  # Force full rebuild (ignore cache)
  python scripts/build_all_indices.py --force

  # Clean all and force rebuild
  python scripts/build_all_indices.py --clean all --force

Clean Targets:
  all              - Clean everything (Whoosh, constraints, documents, cache)
  whoosh           - Clean only Whoosh index
  constraints      - Clean only Excel constraint JSON files
  documents        - Clean only parsed documents
  cache            - Clean only build cache
  domain:hardware  - Clean only hardware domain
  domain:drivers   - Clean only drivers domain
  domain:ble       - Clean only BLE domain
  domain:applications - Clean only applications domain
        """
    )

    parser.add_argument(
        "--clean",
        nargs="+",
        choices=["all", "whoosh", "constraints", "documents",
                "cache", "domain:hardware", "domain:drivers",
                "domain:ble", "domain:applications"],
        help="Clean specified targets before building"
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force full rebuild (ignore cache)"
    )
    parser.add_argument(
        "--incremental",
        action="store_true",
        help="Enable incremental build (default: False for backward compatibility)"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Show detailed build progress"
    )

    args = parser.parse_args()

    # Set logging level
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    logger.info("")
    logger.info("=" * 70)
    logger.info("B6x SDK Index Builder")
    if args.incremental and not args.force:
        logger.info("Mode: INCREMENTAL BUILD (using cache)")
    elif args.force:
        logger.info("Mode: FORCE REBUILD (ignoring cache)")
    else:
        logger.info("Mode: FULL REBUILD")
    logger.info("=" * 70)
    logger.info(f"Start Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    logger.info("")

    # Get paths
    paths = get_paths()

    logger.info("Paths:")
    for key, value in paths.items():
        logger.info(f"  {key}: {value}")
    logger.info("")

    # Initialize cache manager
    cache_manager = None
    incremental_mode = False

    if not args.force:
        try:
            from core.build_cache import BuildCacheManager

            cache_file = paths["mcp_root"] / "data" / "file_hashes.json"
            cache_manager = BuildCacheManager(cache_file, paths["sdk_root"])

            if args.incremental:
                incremental_mode = True
                cache_manager.print_cache_stats()
        except Exception as e:
            logger.warning(f"Failed to initialize cache manager: {e}")
            logger.info("Falling back to full build")
    else:
        logger.info("Force rebuild requested, ignoring cache")

    # Track results
    results = {}

    # Clean phase
    if args.clean:
        results["cleanup"] = clean_indices(paths, args.clean)

        # If cleaning all, also clear cache
        if "all" in args.clean and cache_manager:
            cache_manager.clear_all()

        # Check if clean succeeded
        if not results["cleanup"]:
            logger.error("Cleanup failed. Aborting build.")
            return 1
    else:
        results["cleanup"] = None  # No clean requested
        logger.info("No clean targets specified (incremental mode)")

    # Force full rebuild?
    if args.force and cache_manager:
        logger.info("Force rebuild requested, clearing cache")
        cache_manager.clear_all()

    # Build steps with cache manager
    # Step 1: Build Excel constraints (includes SRAM from YAML, flash from YAML)
    logger.info("")
    results["excel_constraints"] = build_excel_constraints(paths)

    # Step 2: Build document index (with cache)
    # Note: build_document_index now accepts cache_manager parameter
    # We'll call it with subprocess for now, but could integrate later
    results["document_index"] = build_document_index(paths)

    # Step 3: Build API index (with cache)
    results["api_index"] = build_api_index(
        paths,
        cache_manager=cache_manager,
        incremental=incremental_mode
    )

    # Step 4: Build knowledge graph (4D architecture)
    results["knowledge_graph"] = build_knowledge_graph(paths)

    # Step 5: Build error-to-API mapping (SDK analysis + config)
    results["error_api_mapping"] = build_error_api_mapping(paths)

    # Step 6: Generate final report
    logger.info("=" * 70)
    logger.info("FINAL REPORT")
    logger.info("=" * 70)

    report = generate_final_report(paths, results)
    report["build_type"] = "incremental" if incremental_mode else "full_rebuild"
    report["incremental_mode"] = incremental_mode

    if cache_manager:
        report["cache_stats"] = cache_manager.get_cache_stats()

    report_path = paths["mcp_root"] / "data" / "build_report_all.json"
    save_report(report, report_path)

    # Print summary
    if results["cleanup"] is not None:
        logger.info(f"Cleanup: {'[OK]' if results['cleanup'] else '[FAIL]'}")
    else:
        logger.info("Cleanup: SKIPPED")

    logger.info(f"Excel Constraints: {'[OK]' if results['excel_constraints'] else '[FAIL]'}")
    logger.info(f"Document Index: {'[OK]' if results['document_index'] else '[FAIL]'}")
    logger.info(f"Knowledge Graph: {'[OK]' if results.get('knowledge_graph') else '[FAIL]'}")

    # Handle API index result
    api_success = False
    if isinstance(results.get("api_index"), dict):
        api_success = results["api_index"].get("success", False)
        api_stats = results["api_index"].get("stats", {})
        total_apis = api_stats.get("total_indexed", 0)

        mode_str = "INCREMENTAL" if api_stats.get("incremental") else "FULL"
        logger.info(f"API Index: {'[OK]' if api_success else '[FAIL]'} ({total_apis} APIs indexed, {mode_str})")

        # Display incremental stats
        if api_stats.get("incremental"):
            logger.info(f"  Files Processed: {api_stats.get('files_processed', 0)}")
            logger.info(f"  Files Cached: {api_stats.get('files_cached', 0)}")

        # Display peripheral breakdown
        if api_stats.get("by_peripheral"):
            top_periphs = sorted(
                api_stats["by_peripheral"].items(),
                key=lambda x: x[1],
                reverse=True
            )[:5]
            logger.info(f"  Top Peripherals:")
            for periph, count in top_periphs:
                logger.info(f"    {periph}: {count}")
    else:
        logger.info(f"API Index: {'[OK]' if results.get('api_index', False) else '[FAIL]'}")

    constraint_count = report["summary"].get("constraint_files_generated", 0)
    logger.info(f"Constraint Files: {constraint_count}")

    # Error-to-API mapping result
    error_mapping = results.get("error_api_mapping", {})
    if isinstance(error_mapping, dict):
        mapping_success = error_mapping.get("success", False)
        mapping_stats = error_mapping.get("stats", {})
        errors_with_apis = mapping_stats.get("errors_with_apis", 0)
        total_mappings = mapping_stats.get("total_mappings", 0)
        logger.info(f"Error-to-API Mapping: {'[OK]' if mapping_success else '[FAIL]'} ({errors_with_apis} errors, {total_mappings} APIs)")
    else:
        logger.info(f"Error-to-API Mapping: {'[OK]' if error_mapping else '[FAIL]'}")

    if report["summary"]["overall_success"]:
        logger.info("")
        logger.info("=" * 70)
        logger.info("[OK] BUILD COMPLETED SUCCESSFULLY!")
        logger.info("=" * 70)
        logger.info(f"End Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        logger.info("")

        # Print cache stats if incremental
        if incremental_mode and cache_manager:
            cache_manager.print_cache_stats()

        return 0
    else:
        logger.error("")
        logger.error("=" * 70)
        logger.error("[FAIL] BUILD COMPLETED WITH ERRORS")
        logger.error("=" * 70)
        logger.error(f"End Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        logger.error("")
        return 1


if __name__ == "__main__":
    sys.exit(main())
