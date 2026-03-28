#!/usr/bin/env python3
"""
Build Document Index - SDK Documentation Indexer
==============================================

Parse and index all 25 SDK documents into 5-domain Whoosh index.

NOTE: Excel constraints must be built BEFORE running this script.
Run build_excel_constraints.py or build_all_indices.py first.

Workflow:
1. Parse Word/PDF/Markdown documents
2. Classify documents to 5 domains
3. Index to Whoosh search engine
4. Generate statistics report

Usage:
    python scripts/build_document_index.py

Author: B6x MCP Server Team
Version: 0.2.0
"""

import sys
import json
import logging
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)


# ============================================================================
# Paths
# ============================================================================

def find_sdk_root() -> Path:
    """Find SDK root directory."""
    current = Path(__file__).parent.parent
    while current.parent != current:
        if (current / "ble").exists() and (current / "drivers").exists():
            return current
        current = current.parent
    return Path(__file__).parent.parent.parent


def get_paths() -> Dict[str, Path]:
    """Get all relevant paths."""
    mcp_root = Path(__file__).parent.parent
    sdk_root = find_sdk_root()

    return {
        'mcp_root': mcp_root,
        'sdk_root': sdk_root,
        'doc_dir': sdk_root / "doc",
        'data_dir': mcp_root / "data",
        'constraints_dir': mcp_root / "data" / "constraints",
        'index_dir': mcp_root / "data" / "whoosh_index",
        'scripts_dir': mcp_root / "scripts"
    }


# ============================================================================
# Build Steps
# ============================================================================

def step_1_parse_documents(paths: Dict[str, Path], cache_manager=None, incremental=False) -> Dict[str, Any]:
    """
    Step 1: Parse Word/PDF/Excel documents to Markdown/JSON.

    Args:
        paths: Dictionary of paths
        cache_manager: Optional BuildCacheManager for incremental builds
        incremental: If True, use cache_manager for incremental build

    Returns:
        Statistics dictionary
    """
    print("\n" + "="*70)
    print("[1/4] Parsing Word/PDF/Excel Documents")
    if incremental and cache_manager:
        print("Mode: INCREMENTAL (using cache)")
    else:
        print("Mode: FULL BUILD")
    print("="*70)

    from core.document_parser import DocumentParser

    parser = DocumentParser()
    doc_dir = paths['doc_dir']

    # Document file patterns to search (include Excel for full-text search)
    search_patterns = [
        '**/*.docx',
        '**/*.pdf',
        '**/*.xlsx',  # Added: Excel files for indexing
        '**/*.md'
    ]

    # Build file list
    all_docs_list = []
    for pattern in search_patterns:
        for file_path in doc_dir.glob(pattern):
            if file_path.is_file():
                all_docs_list.append(file_path)

    print(f"  Found {len(all_docs_list)} documents total")

    all_docs = []
    errors = []
    total_files_processed = 0
    total_files_cached = 0

    # Incremental or full build?
    if incremental and cache_manager:
        # Build file list with relative paths
        file_list = [
            (str(f.relative_to(paths['sdk_root'])), f)
            for f in all_docs_list
        ]

        # Get changed files only
        changed_files = cache_manager.get_changed_files("documents", file_list)
        total_files_cached = len(file_list) - len(changed_files)
        total_files_processed = len(changed_files)

        print(f"  Found {len(changed_files)} changed documents")
        print(f"  Using cache for {total_files_cached} unchanged documents")

        # Load existing parsed documents
        parsed_docs = {}
        docs_file = paths['data_dir'] / "parsed_documents.json"

        if docs_file.exists() and total_files_cached > 0:
            print(f"  Loading cached documents from: {docs_file}")
            try:
                with open(docs_file, 'r', encoding='utf-8') as f:
                    existing_data = json.load(f)
                    for doc in existing_data:
                        doc_key = doc["file_path"]
                        parsed_docs[doc_key] = doc
                print(f"    Loaded {len(parsed_docs)} cached documents")
            except Exception as e:
                print(f"    [WARN] Failed to load cache: {e}")
                parsed_docs = {}

        # Parse only changed documents
        for rel_path, doc_path in changed_files:
            print(f"  Parsing: {rel_path}")

            try:
                parsed_doc = parser.parse(str(doc_path))
                if parsed_doc:
                    # Convert to dict
                    doc_dict = parsed_doc.to_dict()

                    # Add metadata
                    doc_dict["size_bytes"] = doc_path.stat().st_size
                    doc_dict["parse_time"] = datetime.now().isoformat()

                    all_docs.append(parsed_doc)
                    parsed_docs[rel_path] = doc_dict

                    print(f"    [OK] {parsed_doc.doc_type}  {parsed_doc.domain}")

                    # Update cache
                    cache_manager.update_file_cache(
                        "documents", doc_path, rel_path,
                        outputs=[str(docs_file)]
                    )
                else:
                    errors.append(f"{doc_path.name}: Parse returned None")
            except Exception as e:
                print(f"    [FAIL] Error: {e}")
                errors.append(f"{doc_path.name}: {e}")

        # Merge cached and newly parsed documents
        merged_docs = list(parsed_docs.values())

        # Save merged documents
        docs_data = merged_docs
        docs_file = paths['data_dir'] / "parsed_documents.json"

        with open(docs_file, 'w', encoding='utf-8') as f:
            json.dump(docs_data, f, indent=2, ensure_ascii=False, default=str)

        print(f"  Saved {len(docs_data)} documents to: {docs_file}")

        # Update all_docs for statistics
        for doc_dict in docs_data:
            try:
                parsed_doc = DocumentParser.ParsedDocument(**doc_dict)
                all_docs.append(parsed_doc)
            except Exception as e:
                print(f"  [WARN] Failed to reconstruct document: {e}")

    else:
        # Full build
        print(f"  Full build: parsing all {len(all_docs_list)} documents")
        total_files_processed = len(all_docs_list)

        for file_path in all_docs_list:
            rel_path = str(file_path.relative_to(paths['sdk_root']))
            print(f"  Parsing: {rel_path}")

            try:
                parsed_doc = parser.parse(str(file_path))
                if parsed_doc:
                    all_docs.append(parsed_doc)
                    print(f"    [OK] {parsed_doc.doc_type}  {parsed_doc.domain}")

                    # Update cache if provided
                    if cache_manager:
                        cache_manager.update_file_cache(
                            "documents", file_path, rel_path,
                            outputs=[str(paths['data_dir'] / "parsed_documents.json")]
                        )
                else:
                    errors.append(f"{file_path.name}: Parse returned None")
            except Exception as e:
                print(f"    [FAIL] Error: {e}")
                errors.append(f"{file_path.name}: {e}")

        # Save parsed documents for next step
        # Convert to serializable format
        docs_data = [doc.to_dict() for doc in all_docs]
        docs_file = paths['data_dir'] / "parsed_documents.json"

        with open(docs_file, 'w', encoding='utf-8') as f:
            json.dump(docs_data, f, indent=2, ensure_ascii=False, default=str)

        print(f"  Saved parsed documents to: {docs_file}")

    stats = {
        'status': 'success',
        'total_documents': len(all_docs),
        'parsed_by_type': {},
        'parsed_by_domain': {},
        'errors': errors,
        'incremental': incremental and cache_manager is not None,
        'files_processed': total_files_processed,
        'files_cached': total_files_cached
    }

    # Count by type and domain
    for doc in all_docs:
        stats['parsed_by_type'][doc.doc_type] = stats['parsed_by_type'].get(doc.doc_type, 0) + 1
        stats['parsed_by_domain'][doc.domain] = stats['parsed_by_domain'].get(doc.domain, 0) + 1

    print(f"\n  [OK] Parsed {len(all_docs)} documents")
    print(f"     By type: {stats['parsed_by_type']}")
    print(f"     By domain: {stats['parsed_by_domain']}")

    if stats['incremental']:
        print(f"     Files processed: {stats['files_processed']}")
        print(f"     Files cached: {stats['files_cached']}")

    if errors:
        print(f"     Errors: {len(errors)}")

    return stats


def step_2_classify_domains(paths: Dict[str, Path]) -> Dict[str, Any]:
    """
    Step 2: Classify documents into 5 domains.
    """
    print("\n" + "="*70)
    print("[2/4] Classifying Documents into 5 Domains")
    print("="*70)

    docs_file = paths['data_dir'] / "parsed_documents.json"

    if not docs_file.exists():
        print("  [!] Parsed documents file not found, skipping")
        return {'status': 'skipped', 'reason': 'No parsed documents'}

    with open(docs_file, 'r', encoding='utf-8') as f:
        docs_data = json.load(f)

    # Domain descriptions
    domain_descriptions = {
        'domain1': 'Hardware & Registers',
        'domain2': 'SOC Drivers',
        'domain3': 'BLE Stack',
        'domain4': 'Applications',
        'domain5': 'Production & Toolchain'
    }

    # Group by domain
    by_domain = {f'domain{i}': [] for i in range(1, 6)}

    for doc_data in docs_data:
        domain = doc_data.get('domain', 'domain4')
        by_domain[domain].append(doc_data)

    # Print classification
    for domain, docs in by_domain.items():
        if docs:
            print(f"\n  {domain.upper()} - {domain_descriptions[domain]}:")
            for doc in docs:
                print(f"    - {doc['file_name']} ({doc['doc_type']})")

    stats = {
        'status': 'success',
        'domains': {
            domain: {
                'name': domain_descriptions.get(domain, domain),
                'count': len(docs),
                'documents': [d['file_name'] for d in docs]
            }
            for domain, docs in by_domain.items()
        },
        'total_classified': len(docs_data)
    }

    print(f"\n  [OK] Classified {len(docs_data)} documents into 5 domains")

    return stats


def step_3_index_whoosh(paths: Dict[str, Path]) -> Dict[str, Any]:
    """
    Step 3: Index all documents to Whoosh.
    """
    print("\n" + "="*70)
    print("[3/4] Indexing Documents to Whoosh")
    print("="*70)

    from core.whoosh_indexer import WhooshIndexBuilder
    from core.document_parser import ParsedDocument

    index_dir = paths['index_dir']
    docs_file = paths['data_dir'] / "parsed_documents.json"

    if not docs_file.exists():
        print("  [!] Parsed documents file not found, skipping")
        return {'status': 'skipped', 'reason': 'No parsed documents'}

    # Load parsed documents
    with open(docs_file, 'r', encoding='utf-8') as f:
        docs_data = json.load(f)

    # Create or open index
    builder = WhooshIndexBuilder(str(index_dir))

    if not builder.open_index():
        print("  Creating new Whoosh index...")
        builder.create_index()

    # Import document indexer
    from core.document_indexer import DocumentIndexer

    indexer = DocumentIndexer(builder)

    # Reconstruct ParsedDocument objects
    parsed_docs = []
    for doc_data in docs_data:
        try:
            doc = ParsedDocument(**doc_data)
            parsed_docs.append(doc)
        except Exception as e:
            print(f"  [!] Failed to reconstruct document: {e}")

    # Index in batches
    batch_size = 5
    indexed_count = indexer.index_documents_batch(parsed_docs, batch_size=batch_size)

    stats = {
        'status': 'success',
        'index_dir': str(index_dir),
        'total_documents': len(parsed_docs),
        'indexed_count': indexed_count,
        'failed_count': len(parsed_docs) - indexed_count
    }

    print(f"\n  [OK] Indexed {indexed_count}/{len(parsed_docs)} documents")
    print(f"     Index location: {index_dir}")

    return stats


def step_4_generate_report(paths: Dict[str, Path], all_steps: Dict[str, Any]) -> Dict[str, Any]:
    """
    Step 4: Generate comprehensive report.
    """
    print("\n" + "="*70)
    print("[4/4] Generating Index Report")
    print("="*70)

    # Count Excel constraint files
    excel_constraints_count = 0
    if paths['constraints_dir'].exists():
        excel_constraints_count = len(list(paths['constraints_dir'].glob("*.json")))

    # Compile report
    report = {
        'build_date': datetime.now().isoformat(),
        'build_version': '0.2.0',
        'steps': all_steps,
        'summary': {
            'excel_constraints': excel_constraints_count,
            'documents_parsed': all_steps.get('step1', {}).get('total_documents', 0),
            'documents_classified': all_steps.get('step2', {}).get('total_classified', 0),
            'documents_indexed': all_steps.get('step3', {}).get('indexed_count', 0),
            'total_errors': sum(
                len(step.get('errors', []))
                for step in all_steps.values()
                if isinstance(step.get('errors'), list)
            )
        },
        'paths': {
            'sdk_root': str(paths['sdk_root']),
            'mcp_root': str(paths['mcp_root']),
            'doc_dir': str(paths['doc_dir']),
            'index_dir': str(paths['index_dir']),
            'constraints_dir': str(paths['constraints_dir'])
        }
    }

    # Save report
    report_path = paths['data_dir'] / "document_index_report.json"
    with open(report_path, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False, default=str)

    print(f"\n  [OK] Report saved to: {report_path}")

    # Print summary
    print("\n" + "="*70)
    print("BUILD SUMMARY")
    print("="*70)
    print(f"Excel Constraints: {report['summary']['excel_constraints']} files")
    print(f"Documents Parsed:   {report['summary']['documents_parsed']} files")
    print(f"Documents Indexed:  {report['summary']['documents_indexed']} files")
    print(f"Total Errors:       {report['summary']['total_errors']}")

    # Print domain breakdown
    if 'step3' in all_steps and 'domains' in all_steps['step3']:
        print("\nDomain Breakdown:")
        for domain_id, domain_info in all_steps['step3']['domains'].items():
            if domain_info['count'] > 0:
                print(f"  {domain_id.upper()} ({domain_info['name']}): {domain_info['count']} documents")

    print(f"\nIndex Location:     {paths['index_dir']}")
    print(f"Report Location:    {report_path}")

    return report


# ============================================================================
# Main Build Function
# ============================================================================

def main():
    """Main build function."""
    import argparse

    parser = argparse.ArgumentParser(
        description="B6x SDK Document Index Builder"
    )
    parser.add_argument(
        "--incremental",
        action="store_true",
        help="Enable incremental build (use cache)"
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force full rebuild (ignore cache)"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Show detailed build progress"
    )

    args = parser.parse_args()

    print("="*70)
    print("B6x SDK Document Index Builder")
    print("="*70)
    print(f"Build Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    # Get paths
    paths = get_paths()

    print(f"\nPaths:")
    print(f"  SDK Root:       {paths['sdk_root']}")
    print(f"  Doc Directory:  {paths['doc_dir']}")
    print(f"  MCP Root:       {paths['mcp_root']}")
    print(f"  Index Directory:{paths['index_dir']}")

    # Create directories
    paths['data_dir'].mkdir(parents=True, exist_ok=True)
    paths['constraints_dir'].mkdir(parents=True, exist_ok=True)
    paths['index_dir'].mkdir(parents=True, exist_ok=True)

    # Initialize cache manager
    cache_manager = None
    incremental_mode = False

    if not args.force:
        try:
            from core.build_cache import BuildCacheManager

            cache_file = paths['data_dir'] / "file_hashes.json"
            cache_manager = BuildCacheManager(cache_file, paths['sdk_root'])

            if args.incremental:
                incremental_mode = True
                cache_manager.print_cache_stats()
        except Exception as e:
            print(f"  [WARN] Failed to initialize cache manager: {e}")
            print(f"  [INFO] Falling back to full build")
    else:
        print("  [INFO] Force rebuild requested, ignoring cache")

    # Run build steps
    all_steps = {}

    try:
        # Step 1: Parse documents
        all_steps['step1'] = step_1_parse_documents(
            paths,
            cache_manager=cache_manager,
            incremental=incremental_mode
        )

        # Step 2: Classify domains
        all_steps['step2'] = step_2_classify_domains(paths)

        # Step 3: Index to Whoosh
        all_steps['step3'] = step_3_index_whoosh(paths)

        # Step 4: Generate report
        report = step_4_generate_report(paths, all_steps)

        # Final status
        total_errors = report['summary']['total_errors']
        if total_errors == 0:
            print("\n" + "="*70)
            print("[OK] BUILD COMPLETED SUCCESSFULLY!")
            print("="*70)
            return 0
        else:
            print(f"\n[!] Build completed with {total_errors} errors")
            return 1

    except KeyboardInterrupt:
        print("\n\n[FAIL] Build interrupted by user")
        return 1
    except Exception as e:
        print(f"\n\n[FAIL] Build failed with error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
