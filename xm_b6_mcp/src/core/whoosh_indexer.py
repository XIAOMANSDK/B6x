"""
Whoosh Search Indexer
=====================

Full-text search engine for B6x SDK APIs using Whoosh.

Builds and searches index from Tree-sitter extracted data.

Author: B6x MCP Server Team
Version: 0.1.0
"""

import os
import json
import logging
from pathlib import Path
from typing import List, Dict, Optional, Any
from dataclasses import dataclass, asdict

from whoosh.index import Index, create_in, exists_in, open_dir
from whoosh.fields import Schema, ID, TEXT, KEYWORD, STORED
from whoosh.qparser import QueryParser, MultifieldParser, OrGroup
from whoosh.query import Query, And, Or, Term
from whoosh import scoring
from whoosh.analysis import StemmingAnalyzer

logger = logging.getLogger(__name__)


class B6xSchema:
    """
    Whoosh index schema for B6x SDK APIs.

    Unified schema supporting:
    - Functions (declarations + implementations)
    - Macros (register addresses, bitmasks, constants)
    - Enums (configuration options, status codes)

    Fields:
        entry_id: Unique identifier (e.g., "B6x_UART_Init", "UART0_BASE")
        entry_type: Type of entry ('function', 'macro', 'enum')
        name: Entry name (for exact matching)
        brief: Brief description from @brief
        detailed: Full description

        # Function fields
        parameters: Parameter descriptions
        return_type: Return type (e.g., "void", "int")
        implementation_file: Path to .c implementation file
        implementation_line: Line number in .c file
        has_implementation: Whether implementation exists

        # Macro fields
        macro_value: Macro value (expression)
        macro_type: Macro category ('register', 'bitmask', 'constant', 'other')

        # Enum fields
        enum_values: JSON array of enum values [{name, value, brief}]
        enum_underlying_type: Underlying type (int, uint32_t, etc.)

        # Common fields
        file_path: Source file path
        line_number: Line number in source
        peripheral: Related peripheral (e.g., "UART", "GPIO")
        module: Module name (e.g., "driver", "ble")
        is_exported: Whether entry is exported
        content: Combined text for full-text search
    """

    API_SCHEMA = Schema(
        # Common identification
        entry_id=ID(stored=True, unique=True),
        entry_type=KEYWORD(stored=True),  # 'function', 'macro', 'enum', 'register'
        name=TEXT(field_boost=2.0, stored=True),  # Boost for name matches
        brief=TEXT(stored=True),
        detailed=TEXT(stored=True),

        # Function fields
        parameters=TEXT(stored=True),
        return_type=KEYWORD(stored=True, commas=True),
        implementation_file=STORED,
        implementation_line=STORED,
        has_implementation=STORED,

        # Macro fields
        macro_value=STORED,
        macro_type=KEYWORD(stored=True),

        # Enum fields
        enum_values=STORED,  # JSON: [{name, value, brief}]
        enum_underlying_type=KEYWORD(stored=True),

        # ===== Register fields (SVD) =====
        register_base_address=STORED,        # e.g., "0x40000000"
        register_address_offset=STORED,      # e.g., "0x08"
        register_full_address=STORED,        # e.g., "0x40000008"
        register_access=KEYWORD(stored=True),  # 'read', 'write', 'read-write'
        register_reset_value=STORED,        # e.g., "0x00000000"
        register_fields=STORED,              # JSON: bit fields array
        register_size=STORED,                # Register size in bits

        # ===== Relationship fields (API <-> Register) =====
        related_registers=KEYWORD(stored=True, commas=True),  # function -> registers
        related_apis=KEYWORD(stored=True, commas=True),       # register -> functions
        related_peripheral=KEYWORD(stored=True),              # Peripheral reference

        # ===== Dependency fields (API call sequence) =====
        pre_requisites=KEYWORD(stored=True, commas=True),  # Required APIs before this one
        call_sequence=STORED,                               # Recommended call order (JSON)
        requires_clock=STORED,                              # bool: needs peripheral clock
        requires_gpio=STORED,                               # bool: needs GPIO config
        dependency_notes=TEXT(stored=True),                 # @note/@see/@requires content

        # ===== Example context fields (NEW) =====
        related_examples=KEYWORD(stored=True, commas=True),        # Example names list
        example_context_snippets=STORED,                          # JSON: [{file, line, context}]
        example_call_patterns=STORED,                             # JSON: common call patterns

        # ===== Document indexing fields (NEW - SDK documentation) =====
        doc_id=ID(stored=True, unique=True),                      # Document unique ID
        doc_type=KEYWORD(stored=True),                            # 'word', 'pdf', 'excel', 'code'
        doc_domain=KEYWORD(stored=True),                          # 'domain1', 'domain2', 'domain3', 'domain4', 'domain5'
        doc_source_path=STORED,                                   # Original file path
        doc_sections=KEYWORD(stored=True, commas=True),           # Section headers
        doc_title=TEXT(stored=True),                              # Document title
        doc_metadata=STORED,                                      # JSON: file metadata
        doc_parse_method=KEYWORD(stored=True),                    # 'markitdown', 'docx', 'pypdf2'

        # Common metadata
        file_path=STORED,
        line_number=STORED,
        peripheral=KEYWORD(stored=True, commas=True),
        module=KEYWORD(stored=True),
        is_exported=STORED,
        content=TEXT(analyzer=StemmingAnalyzer())  # Full-text search field
    )


@dataclass
class APIDocument:
    """API document for indexing (legacy - kept for compatibility)."""
    api_id: str
    name: str
    brief: str = ""
    detailed: str = ""
    parameters: str = ""
    return_type: str = ""
    file_path: str = ""
    line_number: int = 0
    peripheral: str = ""
    module: str = ""
    is_exported: bool = False

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for Whoosh indexing (legacy format)."""
        # Combine all fields for full-text search
        content_parts = [
            self.name,
            self.brief,
            self.detailed,
            self.parameters,
            self.peripheral,
        ]
        content = ' '.join([p for p in content_parts if p])

        return {
            'entry_id': self.api_id,  # Map api_id to entry_id for compatibility with schema
            'entry_type': 'function',  # APIDocument is for functions
            'name': self.name,
            'brief': self.brief,
            'detailed': self.detailed,
            'parameters': self.parameters,
            'return_type': self.return_type,
            'file_path': self.file_path,
            'line_number': self.line_number,
            'peripheral': self.peripheral,
            'module': self.module,
            'is_exported': self.is_exported,
            'content': content
        }


@dataclass
class IndexEntry:
    """
    Unified index entry supporting functions, macros, enums, and registers.

    This is the new unified format for all entry types.
    """
    entry_type: str  # 'function', 'macro', 'enum', 'register'
    name: str
    brief: str = ""
    detailed: str = ""
    file_path: str = ""
    line_number: int = 0
    peripheral: str = ""
    module: str = ""
    is_exported: bool = False

    # Function-specific fields
    parameters: str = ""
    return_type: str = ""
    implementation_file: str = ""
    implementation_line: int = 0
    has_implementation: bool = False

    # Macro-specific fields
    macro_value: str = ""
    macro_type: str = ""  # 'register', 'bitmask', 'constant', 'other'

    # Enum-specific fields
    enum_values: str = ""  # JSON: [{name, value, brief}]
    enum_underlying_type: str = ""

    # ===== Register-specific fields (SVD) =====
    register_base_address: str = ""
    register_address_offset: str = ""
    register_full_address: str = ""
    register_access: str = ""
    register_reset_value: str = ""
    register_fields: str = ""  # JSON: bit fields array
    register_size: int = 0

    # ===== Relationship fields =====
    related_registers: str = ""  # Comma-separated register names
    related_apis: str = ""  # Comma-separated API names
    related_peripheral: str = ""

    # ===== Dependency fields =====
    pre_requisites: str = ""  # Comma-separated prerequisite API names
    call_sequence: str = ""  # JSON: call sequence array
    requires_clock: bool = False
    requires_gpio: bool = False
    dependency_notes: str = ""

    # ===== Example context fields (NEW) =====
    related_examples: str = ""  # Comma-separated example names
    example_context_snippets: str = ""  # JSON: context snippets array
    example_call_patterns: str = ""  # JSON: common call patterns

    def get_id(self) -> str:
        """Generate unique entry ID."""
        return self.name

    def get_content(self) -> str:
        """Combine all fields for full-text search."""
        parts = [self.name, self.brief, self.detailed, self.peripheral]

        if self.entry_type == 'function':
            parts.extend([self.parameters, self.return_type])
        elif self.entry_type == 'macro':
            parts.extend([self.macro_value, self.macro_type])
        elif self.entry_type == 'enum':
            parts.extend([self.enum_underlying_type])
        elif self.entry_type == 'register':
            parts.extend([self.register_full_address, self.register_access])

        return ' '.join([p for p in parts if p])

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for Whoosh indexing."""
        doc = {
            'entry_id': self.get_id(),
            'entry_type': self.entry_type,
            'name': self.name,
            'brief': self.brief,
            'detailed': self.detailed,
            'file_path': self.file_path,
            'line_number': self.line_number,
            'peripheral': self.peripheral,
            'module': self.module,
            'is_exported': self.is_exported,
            'content': self.get_content()
        }

        # Add type-specific fields
        if self.entry_type == 'function':
            doc.update({
                'parameters': self.parameters,
                'return_type': self.return_type,
                'implementation_file': self.implementation_file,
                'implementation_line': self.implementation_line,
                'has_implementation': self.has_implementation,
                # Relationship and dependency fields for functions
                'related_registers': self.related_registers,
                'related_peripheral': self.related_peripheral,
                'pre_requisites': self.pre_requisites,
                'call_sequence': self.call_sequence,
                'requires_clock': self.requires_clock,
                'requires_gpio': self.requires_gpio,
                'dependency_notes': self.dependency_notes,
                # Example context fields for functions
                'related_examples': self.related_examples,
                'example_context_snippets': self.example_context_snippets,
                'example_call_patterns': self.example_call_patterns,
            })
        elif self.entry_type == 'macro':
            doc.update({
                'macro_value': self.macro_value,
                'macro_type': self.macro_type,
            })
        elif self.entry_type == 'enum':
            doc.update({
                'enum_values': self.enum_values,
                'enum_underlying_type': self.enum_underlying_type,
            })
        elif self.entry_type == 'register':
            doc.update({
                'register_base_address': self.register_base_address,
                'register_address_offset': self.register_address_offset,
                'register_full_address': self.register_full_address,
                'register_access': self.register_access,
                'register_reset_value': self.register_reset_value,
                'register_fields': self.register_fields,
                'register_size': self.register_size,
                'related_apis': self.related_apis,
                'related_peripheral': self.related_peripheral or self.peripheral,
            })

        return doc


@dataclass
class RegisterEntry:
    """
    Index entry for hardware registers from SVD files.

    Contains complete register definition including:
    - Address information (base, offset, full)
    - Access permissions
    - Reset value
    - Bit fields
    - Related APIs
    """
    name: str
    peripheral: str
    full_address: str
    base_address: str
    address_offset: str
    access: str
    reset_value: str
    size: int
    description: str = ""
    fields_json: str = ""  # JSON array of SVDField objects
    related_apis: str = ""  # Comma-separated API names

    def to_index_entry(self) -> IndexEntry:
        """Convert RegisterEntry to IndexEntry for Whoosh indexing."""
        return IndexEntry(
            entry_type='register',
            name=self.name,
            brief=self.description,
            detailed=self.description,
            peripheral=self.peripheral,
            module='driver',
            is_exported=True,
            register_base_address=self.base_address,
            register_address_offset=self.address_offset,
            register_full_address=self.full_address,
            register_access=self.access,
            register_reset_value=self.reset_value,
            register_fields=self.fields_json,
            register_size=self.size,
            related_apis=self.related_apis,
            related_peripheral=self.peripheral
        )


class WhooshIndexBuilder:
    """
    Whoosh index builder for B6x SDK APIs.

    Builds search index from Tree-sitter extracted function declarations.
    """

    def __init__(self, index_dir: str):
        """
        Initialize the index builder.

        Args:
            index_dir: Directory to store the index
        """
        self.index_dir = Path(index_dir)
        self.index: Optional[Index] = None

    def create_index(self) -> Index:
        """
        Create a new Whoosh index.

        Returns:
            Whoosh Index object
        """
        # Create directory if not exists
        self.index_dir.mkdir(parents=True, exist_ok=True)

        # Create index
        self.index = create_in(str(self.index_dir), B6xSchema.API_SCHEMA)
        logger.info(f"Created new index at {self.index_dir}")

        return self.index

    def open_index(self) -> Optional[Index]:
        """
        Open existing Whoosh index.

        Returns:
            Whoosh Index object or None if index doesn't exist
        """
        if not exists_in(str(self.index_dir)):
            logger.debug(f"Index not found at {self.index_dir} (will be created)")
            return None

        self.index = open_dir(str(self.index_dir))
        logger.info(f"Opened index at {self.index_dir}")

        return self.index

    def add_document(self, doc: APIDocument) -> bool:
        """
        Add a single API document to the index.

        Args:
            doc: APIDocument to add

        Returns:
            True if successful, False otherwise
        """
        if not self.index:
            logger.error("Index not initialized")
            return False

        try:
            writer = self.index.writer()
            writer.add_document(**doc.to_dict())
            writer.commit()

            logger.debug(f"Added document: {doc.api_id}")
            return True

        except Exception as e:
            logger.error(f"Failed to add document {doc.api_id}: {e}")
            return False

    def add_documents_batch(self, docs: List[APIDocument], batch_size: int = 100) -> int:
        """
        Add multiple API documents in batch for better performance.

        Args:
            docs: List of APIDocument objects
            batch_size: Number of documents per commit

        Returns:
            Number of successfully added documents
        """
        if not self.index:
            logger.error("Index not initialized")
            return 0

        try:
            writer = self.index.writer()
            added_count = 0

            for i, doc in enumerate(docs):
                try:
                    writer.add_document(**doc.to_dict())
                    added_count += 1

                    # Commit in batches
                    if (i + 1) % batch_size == 0:
                        writer.commit()
                        logger.info(f"Committed batch: {i + 1}/{len(docs)} documents")
                        writer = self.index.writer()  # New writer for next batch

                except Exception as e:
                    logger.error(f"Failed to add document {doc.api_id}: {e}")

            # Commit remaining documents
            writer.commit()

            logger.info(f"Added {added_count}/{len(docs)} documents to index")
            return added_count

        except Exception as e:
            logger.error(f"Failed to add documents batch: {e}")
            return 0

    def build_from_parse_results(
        self,
        parse_results: List[Any],  # List[HeaderParseResult]
        module: str = "driver",
        index_macros: bool = True,
        index_enums: bool = True
    ) -> int:
        """
        Build index from Tree-sitter parse results.

        Args:
            parse_results: List of HeaderParseResult from TreeSitterCParser
            module: Module name (e.g., "driver", "ble")
            index_macros: Whether to index macro definitions
            index_enums: Whether to index enum definitions

        Returns:
            Number of indexed entries (functions + macros + enums)
        """
        import json

        docs = []
        entries = []

        for result in parse_results:
            peripheral = self._extract_peripheral(result.file_path)

            # 1. Index functions
            for func in result.functions:
                params_text = '; '.join([
                    f"{p.type} {p.name}: {p.description}"
                    for p in func.parameters
                ])

                doc = APIDocument(
                    api_id=func.name,
                    name=func.name,
                    brief=func.brief_description,
                    detailed=func.detailed_description,
                    parameters=params_text,
                    return_type=func.return_type,
                    file_path=result.file_path,
                    line_number=func.line_number,
                    peripheral=peripheral,
                    module=module,
                    is_exported=func.is_exported
                )
                docs.append(doc)

            # 2. Index macros
            if index_macros:
                for macro in result.macros:
                    entry = IndexEntry(
                        entry_type='macro',
                        name=macro.name,
                        brief=macro.brief,
                        file_path=macro.file_path or result.file_path,
                        line_number=macro.line_number,
                        peripheral=peripheral,
                        module=module,
                        macro_value=macro.value,
                        macro_type=macro.macro_type
                    )
                    entries.append(entry)

            # 3. Index enums
            if index_enums:
                for enum in result.enums:
                    # Convert enum values to JSON for storage
                    enum_values_json = json.dumps([
                        {"name": v.name, "value": v.value, "brief": v.brief}
                        for v in enum.values
                    ])

                    entry = IndexEntry(
                        entry_type='enum',
                        name=enum.name,
                        brief=enum.brief,
                        file_path=enum.file_path or result.file_path,
                        line_number=enum.line_number,
                        peripheral=peripheral,
                        module=module,
                        enum_values=enum_values_json,
                        enum_underlying_type=enum.underlying_type
                    )
                    entries.append(entry)

        # Add to index
        function_count = self.add_documents_batch(docs)
        entry_count = self.add_entries_batch(entries)

        total_count = function_count + entry_count
        logger.info(f"Indexed {total_count} entries: {function_count} functions, {len(entries)} macros/enums")

        return total_count

    def add_entries_batch(
        self,
        entries: List[IndexEntry],
        batch_size: int = 100
    ) -> int:
        """
        Add multiple IndexEntry objects in batch.

        Args:
            entries: List of IndexEntry objects
            batch_size: Number of entries per commit

        Returns:
            Number of successfully added entries
        """
        if not self.index:
            logger.error("Index not initialized")
            return 0

        try:
            writer = self.index.writer()
            added_count = 0

            for i, entry in enumerate(entries):
                try:
                    writer.add_document(**entry.to_dict())
                    added_count += 1

                    # Commit in batches
                    if (i + 1) % batch_size == 0:
                        writer.commit()
                        logger.info(f"Committed batch: {i + 1}/{len(entries)} entries")
                        writer = self.index.writer()  # New writer for next batch

                except Exception as e:
                    logger.error(f"Failed to add entry {entry.get_id()}: {e}")

            # Commit remaining entries
            writer.commit()

            logger.info(f"Added {added_count}/{len(entries)} entries to index")
            return added_count

        except Exception as e:
            logger.error(f"Failed to add entries batch: {e}")
            return 0

    def _extract_peripheral(self, file_path: str) -> str:
        """
        Extract peripheral name from file path.

        Examples:
            "drivers/api/B6x_UART.h" -> "UART"
            "drivers/api/B6x_GPIO.h" -> "GPIO"
            "ble/api/ke_api.h" -> "BLE"

        Args:
            file_path: File path

        Returns:
            Peripheral name in uppercase
        """
        path = Path(file_path)

        # Extract from filename
        filename = path.stem.upper()  # e.g., "B6X_UART"

        # Try to extract peripheral from filename
        parts = filename.split('_')
        if len(parts) >= 2:
            # B6X_UART -> UART
            # B6X_UART_DMA -> UART_DMA
            peripheral = '_'.join(parts[1:])
            return peripheral

        return "UNKNOWN"

    def get_index_stats(self) -> Dict[str, Any]:
        """
        Get index statistics.

        Returns:
            Dictionary with index statistics
        """
        if not self.index:
            return {}

        try:
            searcher = self.index.searcher()
            doc_count = searcher.doc_count_all()

            # Sample a document to check fields
            sample_fields = []
            if doc_count > 0:
                # Get first document
                doc = searcher.document(0)
                if doc:
                    sample_fields = list(doc.keys())

            stats = {
                'document_count': doc_count,
                'index_dir': str(self.index_dir),
                'fields': sample_fields,
                'is_empty': doc_count == 0
            }

            return stats

        except Exception as e:
            logger.error(f"Failed to get index stats: {e}")
            return {}

    def search(
        self,
        query_str: str,
        limit: int = 10,
        fields: Optional[List[str]] = None,
        filter_peripheral: Optional[str] = None,
        filter_module: Optional[str] = None,
        filter_entry_type: Optional[str] = None
    ) -> List[Dict[str, Any]]:
        """
        Search the index for entries matching the query.

        Args:
            query_str: Search query string
            limit: Maximum number of results
            fields: Fields to search (default: ['name', 'brief', 'content'])
            filter_peripheral: Filter by peripheral name
            filter_module: Filter by module name
            filter_entry_type: Filter by entry type ('function', 'macro', 'enum', 'document')

        Returns:
            List of matching entries with scores
        """
        if not self.index:
            logger.error("Index not opened")
            return []

        with WhooshSearcher(self.index) as searcher:
            return searcher.search(
                query_str,
                limit=limit,
                fields=fields,
                filter_peripheral=filter_peripheral,
                filter_module=filter_module,
                filter_entry_type=filter_entry_type
            )


class WhooshSearcher:
    """
    Search interface for B6x SDK API index.
    """

    def __init__(self, index: Index):
        """
        Initialize the searcher.

        Args:
            index: Whoosh Index object
        """
        self.index = index
        self.searcher = None

    def __enter__(self):
        """Context manager entry."""
        self.searcher = self.index.searcher()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        if self.searcher:
            self.searcher.close()

    def search(
        self,
        query_str: str,
        limit: int = 10,
        fields: Optional[List[str]] = None,
        filter_peripheral: Optional[str] = None,
        filter_module: Optional[str] = None,
        filter_entry_type: Optional[str] = None
    ) -> List[Dict[str, Any]]:
        """
        Search the index for APIs matching the query.

        Args:
            query_str: Search query string
            limit: Maximum number of results
            fields: Fields to search (default: ['name', 'brief', 'content'])
            filter_peripheral: Filter by peripheral name
            filter_module: Filter by module name
            filter_entry_type: Filter by entry type ('function', 'macro', 'enum', 'document')

        Returns:
            List of matching API documents with scores
        """
        if not self.searcher:
            logger.error("Searcher not initialized")
            return []

        try:
            # Default fields to search
            if fields is None:
                fields = ['name', 'brief', 'content']

            # Create query parser
            parser = MultifieldParser(
                fields,
                schema=self.index.schema,
                fieldboosts={'name': 2.0, 'brief': 1.5, 'content': 1.0},
                group=OrGroup
            )

            query = parser.parse(query_str)

            # Apply filters
            if filter_peripheral:
                query = And([
                    query,
                    Term('peripheral', filter_peripheral.upper())
                ])

            if filter_module:
                query = And([
                    query,
                    Term('module', filter_module.lower())
                ])

            if filter_entry_type:
                query = And([
                    query,
                    Term('entry_type', filter_entry_type)
                ])

            # Execute search
            results = self.searcher.search(
                query,
                limit=limit,
                terms=True  # Include matched terms
            )

            # Convert results to list of dicts
            formatted_results = []
            for hit in results:
                result = {
                    'entry_id': hit.get('entry_id', ''),
                    'entry_type': hit.get('entry_type', 'unknown'),
                    'name': hit.get('name', ''),
                    'brief': hit.get('brief', ''),
                    'return_type': hit.get('return_type', ''),
                    'file_path': hit.get('file_path', ''),
                    'line_number': hit.get('line_number', 0),
                    'peripheral': hit.get('peripheral', ''),
                    'module': hit.get('module', ''),
                    'score': hit.score,
                    'matched_terms': [str(t) for t in hit.matched_terms()] if hasattr(hit, 'matched_terms') else []
                }

                # Add type-specific fields
                entry_type = result['entry_type']
                if entry_type == 'macro':
                    result['macro_value'] = hit.get('macro_value', '')
                    result['macro_type'] = hit.get('macro_type', '')
                elif entry_type == 'enum':
                    result['enum_values'] = hit.get('enum_values', '')
                    result['enum_underlying_type'] = hit.get('enum_underlying_type', '')
                elif entry_type == 'function':
                    result['has_implementation'] = hit.get('has_implementation', False)

                formatted_results.append(result)

            logger.info(f"Search '{query_str}': {len(formatted_results)} results")
            return formatted_results

        except Exception as e:
            logger.error(f"Search failed: {e}")
            return []

    def get_by_name(self, api_name: str) -> Optional[Dict[str, Any]]:
        """
        Get API by exact name.

        Args:
            api_name: Exact API name

        Returns:
            API document or None
        """
        if not self.searcher:
            return None

        try:
            from whoosh.query import Term

            # Try entry_id first (new schema), fall back to api_id (legacy)
            query = Term('entry_id', api_name) | Term('api_id', api_name)
            results = self.searcher.search(query, limit=1)

            if results:
                hit = results[0]
                entry_type = hit.get('entry_type', 'function')

                result = {
                    'entry_id': hit.get('entry_id', hit.get('api_id', '')),
                    'entry_type': entry_type,
                    'name': hit.get('name', ''),
                    'brief': hit.get('brief', ''),
                    'detailed': hit.get('detailed', ''),
                    'file_path': hit.get('file_path', ''),
                    'line_number': hit.get('line_number', 0),
                    'peripheral': hit.get('peripheral', ''),
                    'module': hit.get('module', ''),
                    'is_exported': hit.get('is_exported', False)
                }

                # Add type-specific fields
                if entry_type == 'function':
                    result.update({
                        'parameters': hit.get('parameters', ''),
                        'return_type': hit.get('return_type', ''),
                        'implementation_file': hit.get('implementation_file', ''),
                        'implementation_line': hit.get('implementation_line', 0),
                        'has_implementation': hit.get('has_implementation', False),
                    })
                elif entry_type == 'macro':
                    result.update({
                        'macro_value': hit.get('macro_value', ''),
                        'macro_type': hit.get('macro_type', ''),
                    })
                elif entry_type == 'enum':
                    result.update({
                        'enum_values': hit.get('enum_values', ''),
                        'enum_underlying_type': hit.get('enum_underlying_type', ''),
                    })

                return result

            return None

        except Exception as e:
            logger.error(f"Failed to get API by name: {e}")
            return None


class IndexEntryBuilder:
    """Builder for creating IndexEntry objects."""

    def _extract_peripheral(self, file_path: str) -> str:
        """Extract peripheral name from file path."""
        path = Path(file_path)
        filename = path.stem.upper()

        parts = filename.split('_')
        if len(parts) >= 2:
            peripheral = '_'.join(parts[1:])
            return peripheral

        return "UNKNOWN"

    def build_function_entry(
        self,
        func,
        module: str
    ) -> IndexEntry:
        """Build an IndexEntry from a FunctionDeclaration."""
        # Format parameters
        params_text = '; '.join([
            f"{p.type} {p.name}: {p.description}"
            for p in func.parameters
        ])

        return IndexEntry(
            entry_type='function',
            name=func.name,
            brief=func.brief_description,
            detailed=func.detailed_description,
            parameters=params_text,
            return_type=func.return_type,
            implementation_file=func.implementation_file,
            implementation_line=func.implementation_line,
            has_implementation=func.has_implementation,
            file_path=func.file_path,
            line_number=func.line_number,
            peripheral=self._extract_peripheral(func.file_path),
            module=module,
            is_exported=func.is_exported
        )

    def build_macro_entry(
        self,
        macro,
        module: str
    ) -> IndexEntry:
        """Build an IndexEntry from a MacroDefinition."""
        return IndexEntry(
            entry_type='macro',
            name=macro.name,
            brief=macro.brief,
            detailed="",
            macro_value=macro.value,
            macro_type=macro.macro_type,
            file_path=macro.file_path,
            line_number=macro.line_number,
            peripheral=self._extract_peripheral(macro.file_path),
            module=module,
            is_exported=True
        )

    def build_enum_entry(
        self,
        enum,
        module: str
    ) -> IndexEntry:
        """Build an IndexEntry from an EnumDefinition."""
        # Convert enum values to JSON
        import json
        enum_values_json = json.dumps([
            {"name": v.name, "value": v.value, "brief": v.brief}
            for v in enum.values
        ])

        return IndexEntry(
            entry_type='enum',
            name=enum.name,
            brief=enum.brief,
            detailed=f"Enum with {len(enum.values)} values",
            enum_values=enum_values_json,
            enum_underlying_type=enum.underlying_type,
            file_path=enum.file_path,
            line_number=enum.line_number,
            peripheral=self._extract_peripheral(enum.file_path),
            module=module,
            is_exported=True
        )


class RegisterEntryBuilder:
    """
    Builder for creating RegisterEntry and IndexEntry objects from SVD data.

    Converts SVDRegister objects to Whoosh index entries.
    """

    def build_register_entry(
        self,
        register,  # SVDRegister object
        module: str = "driver"
    ) -> IndexEntry:
        """
        Build an IndexEntry from an SVDRegister.

        Args:
            register: SVDRegister object from SVD parser
            module: Module name (e.g., "driver", "ble")

        Returns:
            IndexEntry with register information
        """
        # Convert fields to JSON
        fields_json = json.dumps([
            {
                "name": f.name,
                "bit_range": f.bit_range,
                "bit_offset": f.bit_offset,
                "bit_width": f.bit_width,
                "description": f.description,
                "access": f.access,
                "enumerated_values": f.enumerated_values
            }
            for f in register.fields
        ])

        return IndexEntry(
            entry_type='register',
            name=f"{register.peripheral}_{register.name}",  # e.g., "UART_CR"
            brief=register.description,
            detailed=register.description,
            peripheral=register.peripheral,
            module=module,
            is_exported=True,
            register_base_address=register.base_address,
            register_address_offset=register.address_offset,
            register_full_address=register.full_address,
            register_access=register.access,
            register_reset_value=register.reset_value,
            register_fields=fields_json,
            register_size=register.size,
            related_apis="",  # Will be populated by mapper
            related_peripheral=register.peripheral
        )

    def add_related_registers(
        self,
        function_entry: IndexEntry,
        register_names: List[str]
    ) -> IndexEntry:
        """
        Add related registers to a function entry.

        Args:
            function_entry: Function index entry
            register_names: List of register names

        Returns:
            Updated IndexEntry
        """
        function_entry.related_registers = ','.join(register_names)
        return function_entry

    def add_related_apis(
        self,
        register_entry: IndexEntry,
        api_names: List[str]
    ) -> IndexEntry:
        """
        Add related APIs to a register entry.

        Args:
            register_entry: Register index entry
            api_names: List of API function names

        Returns:
            Updated IndexEntry
        """
        register_entry.related_apis = ','.join(api_names)
        return register_entry

    def add_dependency_info(
        self,
        function_entry: IndexEntry,
        pre_requisites: List[str] = None,
        call_sequence: List[str] = None,
        requires_clock: bool = False,
        requires_gpio: bool = False,
        dependency_notes: str = ""
    ) -> IndexEntry:
        """
        Add dependency information to a function entry.

        Args:
            function_entry: Function index entry
            pre_requisites: List of prerequisite API names
            call_sequence: Recommended call sequence
            requires_clock: Whether peripheral clock is required
            requires_gpio: Whether GPIO configuration is required
            dependency_notes: Additional dependency notes

        Returns:
            Updated IndexEntry
        """
        function_entry.pre_requisites = ','.join(pre_requisites or [])
        function_entry.call_sequence = json.dumps(call_sequence or [])
        function_entry.requires_clock = requires_clock
        function_entry.requires_gpio = requires_gpio
        function_entry.dependency_notes = dependency_notes
        return function_entry


# Convenience functions

def create_index(index_dir: str) -> WhooshIndexBuilder:
    """
    Convenience function to create a new index.

    Args:
        index_dir: Index directory path

    Returns:
        WhooshIndexBuilder instance
    """
    builder = WhooshIndexBuilder(index_dir)
    builder.create_index()
    return builder


def open_index(index_dir: str) -> Optional[WhooshIndexBuilder]:
    """
    Convenience function to open existing index.

    Args:
        index_dir: Index directory path

    Returns:
        WhooshIndexBuilder instance or None
    """
    builder = WhooshIndexBuilder(index_dir)
    index = builder.open_index()

    if index:
        return builder

    return None


def search_apis(index_dir: str, query: str, limit: int = 10) -> List[Dict]:
    """
    Convenience function to search APIs.

    Args:
        index_dir: Index directory path
        query: Search query
        limit: Maximum results

    Returns:
        List of matching APIs
    """
    builder = open_index(index_dir)
    if not builder:
        return []

    with WhooshSearcher(builder.index) as searcher:
        return searcher.search(query, limit=limit)


if __name__ == "__main__":
    # Test the indexer
    import sys
    import tempfile

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    # Create test index
    with tempfile.TemporaryDirectory() as tmpdir:
        builder = WhooshIndexBuilder(tmpdir)
        builder.create_index()

        # Add test documents
        test_docs = [
            APIDocument(
                api_id="B6x_UART_Init",
                name="B6x_UART_Init",
                brief="Initialize UART peripheral",
                detailed="Full description of UART initialization...",
                parameters="UART_TypeDef *UARTx: UART pointer",
                return_type="void",
                peripheral="UART",
                module="driver"
            ),
            APIDocument(
                api_id="B6x_GPIO_Init",
                name="B6x_GPIO_Init",
                brief="Initialize GPIO pin",
                detailed="Initialize GPIO with given configuration...",
                parameters="GPIO_Pin pin: Pin number",
                return_type="void",
                peripheral="GPIO",
                module="driver"
            )
        ]

        builder.add_documents_batch(test_docs)

        # Print stats
        stats = builder.get_index_stats()
        print(f"\nIndex Stats: {json.dumps(stats, indent=2)}")

        # Test search
        with WhooshSearcher(builder.index) as searcher:
            results = searcher.search("UART")
            print(f"\nSearch 'UART': {len(results)} results")
            for r in results:
                print(f"  - {r['name']}: {r['brief']} (score: {r['score']:.2f})")
