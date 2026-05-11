"""
Document Indexer for Whoosh
============================

Index parsed documents (Word, PDF, Excel) into Whoosh search engine.

Author: B6x MCP Server Team
Version: 0.1.0
"""

import logging
from pathlib import Path
from typing import List, Dict, Optional, Any
from datetime import datetime

from whoosh.index import Index
from whoosh.fields import Schema

from core.document_parser import ParsedDocument, DocumentParser
from core.whoosh_indexer import WhooshIndexBuilder

logger = logging.getLogger(__name__)


class DocumentIndexer:
    """
    Index parsed documents into Whoosh.

    Supports:
    - Word documents (.docx)
    - PDF documents (.pdf)
    - Excel documents (.xlsx)
    - Markdown documents (.md)
    """

    def __init__(self, index_builder: WhooshIndexBuilder):
        """
        Initialize the document indexer.

        Args:
            index_builder: WhooshIndexBuilder instance
        """
        self.index_builder = index_builder
        self.index = index_builder.index
        self.parser = DocumentParser()

    def index_document(self, doc: ParsedDocument) -> bool:
        """
        Index a single parsed document.

        Args:
            doc: ParsedDocument to index

        Returns:
            True if successful, False otherwise
        """
        if not self.index:
            logger.error("Index not initialized")
            return False

        try:
            # Prepare document dict for Whoosh
            doc_dict = self._document_to_dict(doc)

            # Add to index
            writer = self.index.writer()
            writer.update_document(**doc_dict)  # Use update_document to handle duplicates
            writer.commit()

            logger.info(f"Indexed document: {doc.doc_id} ({doc.doc_type})")
            return True

        except Exception as e:
            logger.error(f"Failed to index document {doc.doc_id}: {e}")
            return False

    def index_documents_batch(
        self,
        docs: List[ParsedDocument],
        batch_size: int = 10
    ) -> int:
        """
        Index multiple documents in batch.

        Args:
            docs: List of ParsedDocument objects
            batch_size: Number of documents per commit

        Returns:
            Number of successfully indexed documents
        """
        if not self.index:
            logger.error("Index not initialized")
            return 0

        try:
            writer = self.index.writer()
            added_count = 0

            for i, doc in enumerate(docs):
                try:
                    # Prepare document dict
                    doc_dict = self._document_to_dict(doc)

                    # Add to index
                    writer.update_document(**doc_dict)
                    added_count += 1

                    # Commit in batches
                    if (i + 1) % batch_size == 0:
                        writer.commit()
                        logger.info(f"Committed batch: {i + 1}/{len(docs)} documents")
                        writer = self.index.writer()  # New writer for next batch

                except Exception as e:
                    logger.error(f"Failed to index document {doc.doc_id}: {e}")

            # Commit remaining documents
            writer.commit()

            logger.info(f"Indexed {added_count}/{len(docs)} documents")
            return added_count

        except Exception as e:
            logger.error(f"Failed to index documents batch: {e}")
            return 0

    def _document_to_dict(self, doc: ParsedDocument) -> Dict[str, Any]:
        """
        Convert ParsedDocument to Whoosh document dict.

        Args:
            doc: ParsedDocument

        Returns:
            Dictionary for Whoosh indexing
        """
        # Import json for metadata serialization
        import json

        # Combine text for full-text search
        content_parts = [
            doc.title,
            doc.markdown_text[:2000],  # First 2000 chars for preview
            ' '.join(doc.sections)  # Section headers
        ]
        content = ' '.join([p for p in content_parts if p])

        return {
            # Document identification
            'doc_id': doc.doc_id,
            'doc_type': doc.doc_type,
            'doc_domain': doc.domain,
            'doc_title': doc.title,
            'doc_source_path': doc.file_path,
            'doc_sections': ','.join(doc.sections),
            'doc_metadata': json.dumps(doc.metadata, ensure_ascii=False),
            'doc_parse_method': doc.parse_method,

            # For backward compatibility with API schema
            'entry_id': doc.doc_id,
            'entry_type': 'document',
            'name': doc.title,
            'brief': doc.markdown_text[:500] if doc.markdown_text else "",
            'detailed': doc.markdown_text,
            'file_path': doc.file_path,
            'line_number': 0,
            'peripheral': 'DOCUMENT',
            'module': 'documentation',
            'is_exported': True,
            'content': content
        }

    def index_directory(
        self,
        directory: str,
        domain_filter: Optional[str] = None,
        extensions: Optional[List[str]] = None
    ) -> Dict[str, int]:
        """
        Parse and index all documents in a directory.

        Args:
            directory: Directory path
            domain_filter: Only index documents in this domain (None = all)
            extensions: File extensions to include (default: .docx, .pdf, .xlsx, .md)

        Returns:
            Dictionary with indexing statistics
        """
        # Parse documents
        logger.info(f"Parsing documents in {directory}")
        parsed_docs = self.parser.parse_directory(directory, extensions)

        # Filter by domain if specified
        if domain_filter:
            filtered_docs = [d for d in parsed_docs if d.domain == domain_filter]
            logger.info(f"Filtered to {len(filtered_docs)}/{len(parsed_docs)} documents in {domain_filter}")
            parsed_docs = filtered_docs

        # Index documents
        stats = {
            'total': len(parsed_docs),
            'indexed': 0,
            'failed': 0,
            'by_domain': {},
            'by_type': {}
        }

        for doc in parsed_docs:
            # Count by domain
            if doc.domain not in stats['by_domain']:
                stats['by_domain'][doc.domain] = 0
            stats['by_domain'][doc.domain] += 1

            # Count by type
            if doc.doc_type not in stats['by_type']:
                stats['by_type'][doc.doc_type] = 0
            stats['by_type'][doc.doc_type] += 1

            # Index document
            if self.index_document(doc):
                stats['indexed'] += 1
            else:
                stats['failed'] += 1

        return stats

    def index_sdk_documents(
        self,
        sdk_doc_dir: str,
        domains: Optional[List[str]] = None
    ) -> Dict[str, Any]:
        """
        Index all SDK documentation into 5 domains.

        Args:
            sdk_doc_dir: SDK documentation directory
            domains: List of domains to index (None = all)

        Returns:
            Comprehensive indexing report
        """
        doc_path = Path(sdk_doc_dir)
        if not doc_path.exists():
            logger.error(f"SDK doc directory not found: {sdk_doc_dir}")
            return {}

        # Default to all domains
        if domains is None:
            domains = ['domain1', 'domain2', 'domain3', 'domain4', 'domain5']

        logger.info(f"Indexing SDK documents from: {sdk_doc_dir}")
        logger.info(f"Target domains: {domains}")

        # Document type mapping by domain (based on user-provided table)
        domain_patterns = {
            'domain1': {
                'dirs': ['DataSheet', 'HW_Spec', 'SW_Spec'],
                'patterns': [
                    '*DataSheet*',           # B6x_DataSheet_v3.4.docx
                    '*Layout设计错误*',        # 典型Layout设计错误(必看).docx
                    '*硬件设计指导*',          # 硬件设计指导.docx
                    '*开发板使用说明*',         # 开发板使用说明.docx
                    '*B6x_BLE芯片使用指南*'    # B6x_BLE芯片使用指南_V1.0.1.docx
                ]
            },
            'domain2': {
                'dirs': ['SW_Spec'],
                'patterns': ['*B6x_api*', '*设计指导*']  # B6x_api(部分)设计指导.docx
            },
            'domain3': {
                'dirs': ['SW_Spec'],
                'patterns': [
                    '*B6x_BLE-Sram空间分配*',   # B6x_BLE-Sram空间分配.xlsx
                    '*B6x_BLE-功耗参考*',        # B6x_BLE-功耗参考.xlsx
                    '*B6x BLE兼容性列表*'       # B6x BLE兼容性列表.xlsx
                ]
            },
            'domain4': {
                'dirs': ['SW_Spec'],
                'patterns': [
                    '*B6x常见应用解答*',        # B6x常见应用解答.docx
                    '*JFlash配置说明*'          # JFlash配置说明.docx
                ]
            },
            'domain5': {
                'dirs': [
                    'MP_Spec/量产软件/ISP Tool',
                    'B6x无线认证资料/认证指导手册',
                    '.'
                ],
                'patterns': [
                    '*ISP Tool量产说明文档*',    # ISP Tool量产说明文档.docx
                    '*无线认证指导*',           # B6x无线认证指导.docx
                    '*认证指导手册*',           # B6x无线认证指导.docx
                    '*bxISP_Programmer*'        # bxISP_Programmer_User_Guide_V1.0_CN.docx
                ]
            }
        }

        total_stats = {
            'start_time': datetime.now().isoformat(),
            'sdk_doc_dir': str(sdk_doc_dir),
            'domains': {},
            'total_documents': 0,
            'total_indexed': 0,
            'total_failed': 0
        }

        # Process each domain
        for domain in domains:
            if domain not in domain_patterns:
                logger.warning(f"Unknown domain: {domain}")
                continue

            logger.info(f"\n{'='*60}")
            logger.info(f"Processing {domain.upper()}")
            logger.info(f"{'='*60}")

            config = domain_patterns[domain]
            domain_docs = []

            # Find documents matching domain patterns
            for dir_name in config['dirs']:
                search_dir = doc_path / dir_name
                if not search_dir.exists():
                    logger.debug(f"Directory not found: {search_dir}")
                    continue

                # Find matching files
                for pattern in config['patterns']:
                    matches = list(search_dir.rglob(pattern))
                    for match in matches:
                        if match.is_file() and match.suffix.lower() in ['.docx', '.doc', '.pdf', '.xlsx', '.md']:
                            logger.info(f"Found: {match.name}")
                            domain_docs.append(str(match))

            # Parse and index documents
            domain_indexed = 0
            domain_failed = 0

            for doc_path in domain_docs:
                try:
                    # Parse document
                    parsed_doc = self.parser.parse(doc_path)

                    if parsed_doc:
                        # Override domain classification
                        parsed_doc.domain = domain

                        # Index document
                        if self.index_document(parsed_doc):
                            domain_indexed += 1
                        else:
                            domain_failed += 1
                    else:
                        domain_failed += 1

                except Exception as e:
                    logger.error(f"Failed to process {doc_path}: {e}")
                    domain_failed += 1

            # Record domain stats
            total_stats['domains'][domain] = {
                'documents_found': len(domain_docs),
                'indexed': domain_indexed,
                'failed': domain_failed
            }

            total_stats['total_documents'] += len(domain_docs)
            total_stats['total_indexed'] += domain_indexed
            total_stats['total_failed'] += domain_failed

            logger.info(f"{domain}: {domain_indexed}/{len(domain_docs)} indexed")

        total_stats['end_time'] = datetime.now().isoformat()

        return total_stats


def index_documents(
    index_dir: str,
    doc_dir: str,
    domains: Optional[List[str]] = None
) -> Dict[str, Any]:
    """
    Convenience function to index SDK documents.

    Args:
        index_dir: Whoosh index directory
        doc_dir: Documentation directory
        domains: Domains to index (None = all)

    Returns:
        Indexing statistics
    """
    # Open or create index
    builder = WhooshIndexBuilder(index_dir)

    if not builder.open_index():
        logger.info("Creating new index")
        builder.create_index()

    # Create indexer
    indexer = DocumentIndexer(builder)

    # Index documents
    return indexer.index_sdk_documents(doc_dir, domains)


if __name__ == "__main__":
    import sys
    import tempfile

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    if len(sys.argv) < 2:
        print("Usage: python document_indexer.py <doc_directory>")
        sys.exit(1)

    doc_dir = sys.argv[1]

    # Create temporary index
    with tempfile.TemporaryDirectory() as tmpdir:
        stats = index_documents(tmpdir, doc_dir)

        print("\n" + "="*60)
        print("Indexing Summary")
        print("="*60)
        print(f"Total Documents: {stats.get('total_documents', 0)}")
        print(f"Indexed: {stats.get('total_indexed', 0)}")
        print(f"Failed: {stats.get('total_failed', 0)}")
        print()
        print("By Domain:")
        for domain, domain_stats in stats.get('domains', {}).items():
            print(f"  {domain}: {domain_stats['indexed']}/{domain_stats['documents_found']}")
