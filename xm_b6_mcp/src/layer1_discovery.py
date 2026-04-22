"""
Layer 1: Discovery Layer - Search Composer
===========================================

解决"信息在哪里"的问题

This layer provides unified search across multiple SDK domains:
- API functions
- Documentation
- Macro definitions
- Hardware registers
- Example code

Pattern: Composer Pattern (并行聚合多个搜索引擎)

Author: B6x MCP Server Team
Version: 1.0.0
"""

import json
import logging
import asyncio
from abc import ABC, abstractmethod
from typing import Dict, List, Any, Optional, Union, Literal
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path

# Import existing parsers
try:
    from src.core.whoosh_indexer import WhooshSearcher, WhooshIndexBuilder, open_index
    from src.core.svd_parser import SVDParser
    from src.core.example_scanner import ExampleScanner
except ImportError as e:
    logging.warning(f"Layer 1: Import failed: {e}")

# Import common modules for NodeId standardization
try:
    from src.common.node_id import NodeId, NodeType, create_node_id
except ImportError:
    # Fallback if common module not available
    NodeId = None
    NodeType = None
    create_node_id = None

# Import result types for error status
try:
    from src.common.result_types import success_result, error_result, partial_result
    RESULT_TYPES_AVAILABLE = True
except ImportError:
    RESULT_TYPES_AVAILABLE = False


# ============================================================================
# Local Configuration
# ============================================================================

def _get_base_path():
    """Get base path (SDK root)"""
    return Path(__file__).parent.parent.parent


def _get_data_path(*parts):
    """Get path in data directory"""
    return _get_base_path() / "xm_b6_mcp" / "data" / Path(*parts)


logger = logging.getLogger(__name__)


# ============================================================================
# Enums and Data Classes
# ============================================================================

class SearchScope(Enum):
    """Search domain enumeration"""
    API = "api"
    DOCS = "docs"
    MACROS = "macros"
    REGISTERS = "registers"
    EXAMPLES = "examples"
    ALL = "all"


@dataclass
class SearchResult:
    """Unified search result format"""
    domain: str
    item_type: str
    item_id: str
    title: str
    summary: str
    relevance_score: float
    metadata: Dict[str, Any]
    quick_actions: List[str]
    # New fields for token tracking
    estimated_tokens: int = field(default=0)


# ============================================================================
# Token Estimator
# ============================================================================

class TokenEstimator:
    """
    Estimate token count for search results.

    Provides token estimation to prevent result explosion
    that could exceed context window limits.

    Uses a simple character-based estimation: tokens ≈ chars / 4
    This is accurate enough for JSON structures (within ~10%).
    """

    # Default token limits
    DEFAULT_HARD_LIMIT = 6000  # Leave room for system prompt
    DEFAULT_SOFT_LIMIT = 4000  # Target for normal searches

    def __init__(self, model: str = "gpt-4"):
        """
        Initialize token estimator.

        Args:
            model: Model name (for reference, estimation is model-agnostic)
        """
        self.model = model

    def estimate_result_tokens(self, result: SearchResult) -> int:
        """
        Estimate tokens for a single search result.

        Args:
            result: SearchResult to estimate

        Returns:
            Estimated token count
        """
        # Build JSON representation of result
        result_dict = {
            "domain": result.domain,
            "item_type": result.item_type,
            "item_id": result.item_id,
            "title": result.title,
            "summary": result.summary,
            "relevance_score": result.relevance_score,
            "metadata": result.metadata,
            "quick_actions": result.quick_actions,
        }

        # Rough estimate: JSON string length / 4
        return len(json.dumps(result_dict, separators=(',', ':'))) // 4

    def estimate_total_tokens(self, results: List[SearchResult]) -> int:
        """
        Estimate total tokens for all results.

        Args:
            results: List of SearchResults

        Returns:
            Total estimated token count
        """
        return sum(self.estimate_result_tokens(r) for r in results)

    def estimate_response_tokens(self, results: List[SearchResult], metadata: Dict) -> int:
        """
        Estimate total tokens for complete search response.

        Args:
            results: List of SearchResults
            metadata: Response metadata

        Returns:
            Total estimated token count including metadata
        """
        # Estimate results
        results_tokens = self.estimate_total_tokens(results)

        # Estimate metadata and wrapper
        metadata_tokens = len(json.dumps(metadata, separators=(',', ':'))) // 4
        wrapper_tokens = 200  # Fixed overhead for response structure

        return results_tokens + metadata_tokens + wrapper_tokens


# ============================================================================
# Base Search Engine
# ============================================================================

class SearchEngine(ABC):
    """
    Base class for all search engines.

    Each engine is responsible for searching a specific domain
    and returning standardized SearchResult objects.
    """

    def __init__(self):
        self.name = self.__class__.__name__
        self.base_path = _get_base_path()
        self.data_path = _get_data_path()

    @abstractmethod
    async def search(self, query: str, max_results: int) -> List[SearchResult]:
        """
        Execute search in this domain.

        Args:
            query: Search query string
            max_results: Maximum number of results to return

        Returns:
            List of SearchResult objects sorted by relevance
        """
        pass

    def _create_result(
        self,
        item_type: str,
        item_id: str,
        title: str,
        summary: str,
        score: float,
        metadata: Dict[str, Any],
        quick_actions: List[str] = None
    ) -> SearchResult:
        """Helper to create SearchResult with standardized item_id format."""
        # Standardize item_id to "type:identifier" format
        if create_node_id is not None and item_id and ":" not in item_id:
            try:
                node_id_obj = create_node_id(item_type, item_id)
                item_id = node_id_obj.to_string()
            except Exception:
                pass  # Keep original item_id on error

        return SearchResult(
            domain=self.name.replace("Engine", "").lower(),
            item_type=item_type,
            item_id=item_id,
            title=title,
            summary=summary,
            relevance_score=score,
            metadata=metadata,
            quick_actions=["inspect_node"]
        )


# ============================================================================
# API Search Engine
# ============================================================================

class APIEngine(SearchEngine):
    """
    Search engine for API functions using Whoosh index.

    Searches through:
    - Function names
    - Parameter types
    - Return types
    - Documentation comments
    """

    def __init__(self):
        super().__init__()
        self.whoosh: Optional[WhooshSearcher] = None
        self._init_index()

    def _init_index(self):
        """Initialize Whoosh index"""
        try:
            whoosh_index_dir = _get_data_path("whoosh_index")
            if whoosh_index_dir and whoosh_index_dir.exists():
                self.whoosh = open_index(str(whoosh_index_dir))
                logger.info("APIEngine: Whoosh index loaded")
            else:
                logger.warning(f"APIEngine: Whoosh index not found at {whoosh_index_dir}")
        except Exception as e:
            logger.error(f"APIEngine: Failed to load Whoosh index: {e}")

    async def search(self, query: str, max_results: int) -> List[SearchResult]:
        """Search API functions"""
        if not self.whoosh:
            logger.warning("APIEngine: Whoosh index not available")
            return []

        try:
            # Use Whoosh to search API index
            results = self.whoosh.search(query, limit=max_results)

            formatted_results = []
            for result in results:
                formatted_results.append(self._create_result(
                    item_type="function",
                    item_id=result.get("name", "unknown"),
                    title=result.get("name", "unknown"),
                    summary=self._extract_summary(result),
                    score=result.get("score", 0.5),
                    metadata={
                        "header_file": result.get("file_path", ""),
                        "parameters": result.get("parameters", [])[:3],
                        "return_type": result.get("return_type", "void"),
                    },
                    quick_actions=["inspect_definition", "inspect_implementation"]
                ))

            return formatted_results

        except Exception as e:
            logger.error(f"APIEngine: Search failed: {e}")
            # Return empty list but let composer handle error status
            return []

    def _extract_summary(self, result: Dict) -> str:
        """Extract summary from API result"""
        parts = []
        if result.get("brief"):
            parts.append(result["brief"])
        if result.get("parameters"):
            params = result["parameters"][:2]
            param_str = ", ".join([p.get("name", "") for p in params])
            parts.append(f"Params: {param_str}")
        return ". ".join(parts) if parts else "API function"


# ============================================================================
# Register Search Engine
# ============================================================================

class RegisterEngine(SearchEngine):
    """
    Search engine for hardware registers using SVD parser.

    Searches through:
    - Register names
    - Peripheral names
    - Bit field names
    - Register descriptions
    """

    def __init__(self):
        super().__init__()
        self.svd: Optional[SVDParser] = None
        self._init_svd()

    def _init_svd(self):
        """Initialize SVD parser"""
        try:
            # Try to find SVD file (new path: sdk6/core/B6x.svd)
            base_path = _get_base_path()
            svd_path = base_path / "core" / "B6x.svd"

            if not svd_path.exists():
                # Fallback to previous path
                svd_path = base_path / "xm_b6_mcp" / "SVD" / "DragonC.svd"

            if not svd_path.exists():
                # Fallback to legacy path
                svd_path = base_path / "doc" / "DataSheet" / "B6x.svd"

            if svd_path.exists():
                self.svd = SVDParser(str(svd_path))
                logger.info(f"RegisterEngine: SVD file loaded from {svd_path}")
            else:
                logger.warning(f"RegisterEngine: SVD file not found at {svd_path}")
        except Exception as e:
            logger.error(f"RegisterEngine: Failed to load SVD: {e}")

    async def search(self, query: str, max_results: int) -> List[SearchResult]:
        """Search hardware registers"""
        if not self.svd:
            logger.warning("RegisterEngine: SVD parser not available")
            # Return mock results for testing
            return self._mock_results(query, max_results)

        try:
            # Search SVD for registers matching query
            results = self.svd.search_registers(query=query, limit=max_results)

            formatted_results = []
            for result in results:
                formatted_results.append(self._create_result(
                    item_type="register",
                    item_id=result.get("name", "unknown"),
                    title=f"{result.get('peripheral', '')}_{result.get('name', '')}",
                    summary=result.get("description", "Hardware register"),
                    score=result.get("relevance", 0.5),
                    metadata={
                        "address": result.get("address", ""),
                        "access": result.get("access", "read-write"),
                        "offset": result.get("offset", ""),
                    },
                    quick_actions=["inspect_register", "inspect_fields"]
                ))

            return formatted_results

        except Exception as e:
            logger.error(f"RegisterEngine: Search failed: {e}")
            return []

    def _mock_results(self, query: str, max_results: int) -> List[SearchResult]:
        """Generate mock results for testing when SVD not available"""
        mock_data = {
            "uart": [
                {
                    "name": "UART1_CTRL",
                    "peripheral": "UART1",
                    "description": "UART Control Register",
                    "address": "0x40011000",
                    "access": "read-write"
                },
                {
                    "name": "UART1_BRR",
                    "peripheral": "UART1",
                    "description": "UART Baudrate Register",
                    "address": "0x40011004",
                    "access": "read-write"
                }
            ],
            "gpio": [
                {
                    "name": "GPIOA_MODER",
                    "peripheral": "GPIOA",
                    "description": "GPIO Mode Register",
                    "address": "0x40000000",
                    "access": "read-write"
                }
            ]
        }

        results = []
        query_lower = query.lower()

        for key, registers in mock_data.items():
            if key in query_lower:
                for reg in registers[:max_results]:
                    results.append(self._create_result(
                        item_type="register",
                        item_id=reg["name"],
                        title=f"{reg['peripheral']}_{reg['name']}",
                        summary=reg["description"],
                        score=0.7,
                        metadata={
                            "address": reg["address"],
                            "access": reg["access"]
                        },
                        quick_actions=["inspect_register"]
                    ))

        return results


# ============================================================================
# Documentation Search Engine
# ============================================================================

class DocEngine(SearchEngine):
    """
    Search engine for documentation using Whoosh index.

    Searches through:
    - SDK documents (PDF, Word, Markdown)
    - API references
    - User guides
    - Application notes
    """

    def __init__(self):
        super().__init__()
        self.whoosh: Optional[WhooshIndexBuilder] = None
        self._init_index()

    def _init_index(self):
        """Initialize Whoosh index"""
        try:
            whoosh_index_dir = _get_data_path("whoosh_index")
            if whoosh_index_dir and whoosh_index_dir.exists():
                self.whoosh = open_index(str(whoosh_index_dir))
                logger.info("DocEngine: Whoosh index loaded")
            else:
                logger.warning(f"DocEngine: Whoosh index not found at {whoosh_index_dir}")
        except Exception as e:
            logger.error(f"DocEngine: Failed to load Whoosh index: {e}")

    async def search(self, query: str, max_results: int) -> List[SearchResult]:
        """Search documentation"""
        if not self.whoosh:
            logger.warning("DocEngine: Whoosh index not available")
            return []

        try:
            # Use Whoosh to search document index
            # Documents are stored with entry_type='document'
            results = self.whoosh.search(
                query,
                limit=max_results,
                fields=['title', 'content', 'brief', 'name'],
                filter_entry_type='document'
            )

            formatted_results = []
            for result in results:
                formatted_results.append(self._create_result(
                    item_type="documentation",
                    item_id=result.get("entry_id", "unknown"),
                    title=result.get("title", result.get("name", "Document")),
                    summary=self._extract_doc_summary(result),
                    score=result.get("score", 0.5),
                    metadata={
                        "file_path": result.get("file_path", ""),
                        "file_type": result.get("file_type", ""),
                        "domain": result.get("domain", ""),
                    },
                    quick_actions=["open_document"]
                ))

            return formatted_results

        except Exception as e:
            logger.error(f"DocEngine: Search failed: {e}")
            return []

    def _extract_doc_summary(self, result: Dict) -> str:
        """Extract summary from document result"""
        # Try brief, then content, then title
        if result.get("brief"):
            return result["brief"]
        if result.get("content"):
            content = result["content"]
            # Truncate long content
            if len(content) > 200:
                return content[:200] + "..."
            return content
        return result.get("title", result.get("name", "Document"))


# ============================================================================
# Examples Search Engine
# ============================================================================

class ExampleEngine(SearchEngine):
    """
    Search engine for example code.

    Searches through:
    - Example project names
    - Peripheral examples
    - Code snippets
    """

    def __init__(self):
        super().__init__()
        self.scanner: Optional[ExampleScanner] = None
        self._init_scanner()

    def _init_scanner(self):
        """Initialize example scanner from pre-built JSON data"""
        try:
            # Load pre-built examples from JSON (faster and more reliable)
            examples_path = _get_data_path("domain", "applications", "examples.json")
            if examples_path.exists():
                with open(examples_path, 'r', encoding='utf-8') as f:
                    self._examples_data = json.load(f)
                logger.info(f"ExampleEngine: Loaded {len(self._examples_data)} examples from JSON")
                self.scanner = True  # Mark as initialized
            else:
                logger.warning(f"ExampleEngine: Examples JSON not found at {examples_path}")
                self._examples_data = []
        except Exception as e:
            logger.error(f"ExampleEngine: Failed to load examples: {e}")
            self._examples_data = []

    async def search(self, query: str, max_results: int) -> List[SearchResult]:
        """Search example code from pre-built JSON data"""
        if not self._examples_data:
            return self._mock_examples(query, max_results)

        try:
            query_lower = query.lower()
            formatted_results = []

            for example in self._examples_data:
                # Search in name, path, and used APIs
                name = example.get("name", "").lower()
                path = example.get("file_path", "").lower()
                apis = example.get("used_apis", [])

                # Match if query is in name, path, or any used API
                if (query_lower in name or
                    query_lower in path or
                    any(query_lower in api.lower() for api in apis)):

                    # Extract example name from path
                    path_parts = example.get("file_path", "").split("/")
                    example_name = path_parts[1] if len(path_parts) > 1 else example.get("name", "Example")

                    formatted_results.append(self._create_result(
                        item_type="example",
                        item_id=example.get("file_path", "unknown"),
                        title=example_name,
                        summary=f"Uses {len(apis)} APIs: {', '.join(apis[:5])}{'...' if len(apis) > 5 else ''}",
                        score=0.7,
                        metadata={
                            "path": example.get("file_path", ""),
                            "type": example.get("type", ""),
                            "total_api_calls": example.get("total_api_calls", 0),
                            "unique_apis": len(apis),
                        },
                        quick_actions=["view_example", "copy_code"]
                    ))

                    if len(formatted_results) >= max_results:
                        break

            return formatted_results

        except Exception as e:
            logger.error(f"ExampleEngine: Search failed: {e}")
            return []

    def _mock_examples(self, query: str, max_results: int) -> List[SearchResult]:
        """Generate mock example results"""
        mock_examples = [
            {
                "id": "uart_basic",
                "name": "UART Basic Example",
                "description": "Simple UART send/receive example",
                "path": "examples/peripheral/uart/uart_basic"
            },
            {
                "id": "ble_hids",
                "name": "BLE HID Keyboard",
                "description": "BLE HID device example (keyboard)",
                "path": "examples/ble/ble_hids_keyboard"
            }
        ]

        results = []
        query_lower = query.lower()

        for ex in mock_examples:
            if any(keyword in ex["id"].lower() or keyword in ex["name"].lower()
                   for keyword in query_lower.split()):
                results.append(self._create_result(
                    item_type="example",
                    item_id=ex["id"],
                    title=ex["name"],
                    summary=ex["description"],
                    score=0.7,
                    metadata={"path": ex["path"]},
                    quick_actions=["view_example"]
                ))

        return results[:max_results]


# ============================================================================
# Macro Search Engine
# ============================================================================

class MacroEngine(SearchEngine):
    """
    Search engine for macro definitions using Whoosh index.

    Searches through:
    - #define macros
    - Enum constants
    - Configuration macros
    """

    def __init__(self):
        super().__init__()
        self.whoosh: Optional[WhooshIndexBuilder] = None
        self._init_index()

    def _init_index(self):
        """Initialize Whoosh index"""
        try:
            whoosh_index_dir = _get_data_path("whoosh_index")
            if whoosh_index_dir and whoosh_index_dir.exists():
                self.whoosh = open_index(str(whoosh_index_dir))
                logger.info("MacroEngine: Whoosh index loaded")
            else:
                logger.warning(f"MacroEngine: Whoosh index not found at {whoosh_index_dir}")
        except Exception as e:
            logger.error(f"MacroEngine: Failed to load Whoosh index: {e}")

    async def search(self, query: str, max_results: int) -> List[SearchResult]:
        """Search macro definitions"""
        if not self.whoosh:
            logger.warning("MacroEngine: Whoosh index not available")
            return []

        try:
            # Use Whoosh to search macro index
            # Macros are stored with entry_type='macro'
            # Note: Macro names like CFG_PMU_DFLT_CNTL are indexed as single tokens
            # Use wildcard query for better matching
            import whoosh.query as wq
            from whoosh.qparser import QueryParser, OrGroup

            # Build a query that matches prefix, wildcard and fuzzy
            query_lower = query.lower()

            # Create multiple query types for better matching
            # 1. Prefix: "cfg" matches "cfg_pmu..."
            # 2. Wildcard: "wkup" matches "*wkup*" -> finds "cfg_wkup_..."
            # 3. Fuzzy: allows minor typos
            name_queries = [
                wq.Prefix('name', query_lower),
                wq.Wildcard('name', f'*{query_lower}*'),
                wq.FuzzyTerm('name', query_lower),
            ]

            combined_query = wq.Or(name_queries)

            # Add entry_type filter
            final_query = wq.And([
                combined_query,
                wq.Term('entry_type', 'macro')
            ])

            # Execute search directly
            with self.whoosh.index.searcher() as searcher:
                results = searcher.search(final_query, limit=max_results)

                formatted_results = []
                for hit in results:
                    result_dict = dict(hit)
                    formatted_results.append(self._create_result(
                        item_type="macro",
                        item_id=result_dict.get("name", "unknown"),
                        title=result_dict.get("name", "Macro"),
                        summary=self._extract_macro_summary(result_dict),
                        score=hit.score if hasattr(hit, 'score') else 0.5,
                        metadata={
                            "value": result_dict.get("macro_value", ""),
                            "macro_type": result_dict.get("macro_type", ""),
                            "header_file": result_dict.get("file_path", ""),
                            "peripheral": result_dict.get("peripheral", ""),
                        },
                        quick_actions=["inspect_definition"]
                    ))

                return formatted_results

        except Exception as e:
            logger.error(f"MacroEngine: Search failed: {e}")
            return []

    def _extract_macro_summary(self, result: Dict) -> str:
        """Extract summary from macro result"""
        parts = []
        if result.get("brief"):
            parts.append(result["brief"])
        if result.get("macro_value"):
            parts.append(f"Value: {result['macro_value']}")
        if result.get("macro_type"):
            parts.append(f"Type: {result['macro_type']}")
        return ". ".join(parts) if parts else "Macro definition"


# ============================================================================
# Main Search Composer
# ============================================================================

class SDKSearchComposer:
    """
    Unified search composer using Composer Pattern.

    Aggregates multiple search engines and merges their results
    based on the specified merge strategy.
    """

    # Domain weights for global ranking
    DOMAIN_WEIGHTS = {
        "api": 1.0,
        "docs": 0.9,
        "examples": 0.85,
        "registers": 0.7,
        "macros": 0.6
    }

    def __init__(self):
        self.engines: Dict[str, SearchEngine] = {}
        self.base_path = _get_base_path()
        self.data_path = _get_data_path()
        self._init_engines()

    def _init_engines(self):
        """Initialize all search engines"""
        self.engines = {
            SearchScope.API.value: APIEngine(),
            SearchScope.DOCS.value: DocEngine(),
            SearchScope.MACROS.value: MacroEngine(),
            SearchScope.REGISTERS.value: RegisterEngine(),
            SearchScope.EXAMPLES.value: ExampleEngine(),
        }
        logger.info(f"SDKSearchComposer: Initialized {len(self.engines)} search engines")

    async def search(
        self,
        query: str,
        scope: Union[str, List[str]] = "all",
        max_results: int = 10,
        merge_mode: Literal["interleave", "group", "rank"] = "interleave",
        hard_token_limit: int = TokenEstimator.DEFAULT_HARD_LIMIT
    ) -> Dict[str, Any]:
        """
        Execute cross-domain search with token limit enforcement.

        Args:
            query: Search query string
            scope: Domains to search ("all", single domain, or list of domains)
            max_results: Maximum results per domain (soft limit)
            merge_mode: How to merge results from multiple domains
            hard_token_limit: Hard token limit for response (default: 6000)

        Returns:
            Unified search results with pagination metadata and error status
        """
        import time
        start_time = time.time()

        # Parse scope
        scopes = self._parse_scope(scope)

        # Parallel search
        results_by_domain = await self._parallel_search(query, scopes, max_results)

        # Track which domains had errors (returned empty but had no data)
        domains_with_errors = []
        domains_with_results = []
        for domain, results in results_by_domain.items():
            if results:
                domains_with_results.append(domain)
            else:
                # Check if this was due to an error (no index available)
                engine = self.engines.get(domain)
                if engine and not getattr(engine, '_index_available', True):
                    domains_with_errors.append(domain)

        # Reserve buffer for response metadata overhead (query, scopes, JSON structure)
        metadata_buffer = 200  # tokens reserved for metadata wrapper
        merge_token_limit = max(hard_token_limit - metadata_buffer, 1)

        # Merge results with token limit
        merged_results = self._merge_results(
            results_by_domain,
            mode=merge_mode,
            hard_token_limit=merge_token_limit
        )

        # Estimate final token count
        estimator = TokenEstimator()
        estimated_tokens = estimator.estimate_response_tokens(
            [self._deserialize_result(r) for r in merged_results],
            {"query": query, "scopes_searched": scopes}
        )

        # Check for pagination marker
        has_more = False
        domains_with_more = []
        pagination_info = None

        if merged_results:
            # Check last result for pagination marker
            last_result = merged_results[-1]
            if "_metadata" in last_result and "_pagination" in last_result.get("_metadata", {}):
                pagination_info = last_result["_metadata"]["_pagination"]
                has_more = pagination_info.get("has_more", False)
                domains_with_more = pagination_info.get("domains_with_more", [])

            # Also check for _pagination at top level
            if "_pagination" in last_result:
                pagination_info = last_result["_pagination"]
                has_more = pagination_info.get("has_more", False)

        # Determine overall status
        if not merged_results and domains_with_errors:
            # All searched domains had errors
            status = "failure"
            status_message = f"Search failed - indexes not available for: {', '.join(domains_with_errors)}"
            suggestion = "Run build scripts to generate Whoosh index"
        elif domains_with_errors and domains_with_results:
            # Partial success
            status = "partial"
            status_message = f"Partial results - some domains unavailable: {', '.join(domains_with_errors)}"
            suggestion = None
        else:
            status = "success"
            status_message = None
            suggestion = None

        # Build response with pagination info and error status
        response = {
            "success": status != "failure",
            "status": status,
            "query": query,
            "scopes_searched": scopes,
            "total_results": len(merged_results),
            "results": merged_results,
            "metadata": {
                "merge_mode": merge_mode,
                "max_results_per_domain": max_results,
                "search_time_ms": round((time.time() - start_time) * 1000, 2),
                "domains_with_results": domains_with_results,
                "estimated_tokens": estimated_tokens,
                "token_limit": hard_token_limit,
            }
        }

        # Add error info if applicable
        if status_message:
            response["error_info"] = {
                "message": status_message,
                "domains_with_errors": domains_with_errors,
            }
            if suggestion:
                response["error_info"]["suggestion"] = suggestion

        # Add pagination info if applicable
        if has_more:
            response["pagination"] = {
                "has_more": True,
                "domains_with_more": domains_with_more,
                "suggestion": pagination_info.get("suggestion", "Use search_scope='api' to narrow results")
            }
        else:
            response["pagination"] = {
                "has_more": False
            }

        return response

    def _deserialize_result(self, result_dict: Dict) -> SearchResult:
        """Convert dict back to SearchResult for token estimation"""
        return SearchResult(
            domain=result_dict.get("domain", ""),
            item_type=result_dict.get("item_type", ""),
            item_id=result_dict.get("item_id", ""),
            title=result_dict.get("title", ""),
            summary=result_dict.get("summary", ""),
            relevance_score=result_dict.get("relevance_score", 0),
            metadata=result_dict.get("metadata", {}),
            quick_actions=result_dict.get("quick_actions", []),
        )

    def _parse_scope(self, scope: Union[str, List[str]]) -> List[str]:
        """Parse search scope parameter"""
        if isinstance(scope, str):
            if scope == "all":
                return [s.value for s in SearchScope if s != SearchScope.ALL]
            return [scope]
        return scope

    async def _parallel_search(
        self,
        query: str,
        scopes: List[str],
        max_results: int
    ) -> Dict[str, List[SearchResult]]:
        """Execute parallel search across multiple domains"""
        tasks = []

        for scope in scopes:
            if scope in self.engines:
                task = self.engines[scope].search(query, max_results)
                tasks.append((scope, task))

        results = {}
        completed_tasks = await asyncio.gather(*[task for _, task in tasks], return_exceptions=True)

        for (scope, _), result in zip(tasks, completed_tasks):
            if isinstance(result, Exception):
                logger.warning(f"Search engine {scope} failed: {result}")
                results[scope] = []
            else:
                results[scope] = result

        return results

    def _merge_results(
        self,
        results_by_domain: Dict[str, List[SearchResult]],
        mode: str = "interleave",
        hard_token_limit: int = TokenEstimator.DEFAULT_HARD_LIMIT
    ) -> List[Dict]:
        """
        Merge results from multiple domains with token limit enforcement.

        Args:
            results_by_domain: Results from each domain
            mode: Merge mode (interleave, group, rank)
            hard_token_limit: Maximum tokens to return

        Returns:
            Merged and truncated results
        """
        # Filter out low-relevance results and deduplicate within each domain
        MIN_RELEVANCE_SCORE = 0.5
        for domain in results_by_domain:
            filtered = [r for r in results_by_domain[domain] if r.relevance_score >= MIN_RELEVANCE_SCORE]
            results_by_domain[domain] = self._deduplicate_domain(filtered)

        if mode == "group":
            # Group results by domain
            return self._merge_group(results_by_domain, hard_token_limit)

        elif mode == "interleave":
            # Interleave by relevance
            return self._merge_interleave(results_by_domain, hard_token_limit)

        elif mode == "rank":
            # Global ranking with domain weights
            return self._merge_rank(results_by_domain, hard_token_limit)

        return []

    def _deduplicate_domain(
        self,
        results: List[SearchResult]
    ) -> List[SearchResult]:
        """
        Remove duplicate results within a single domain.

        Dedup key: (item_id.lower(), item_type)
        Keep strategy: retain the result with the highest relevance_score.
        On tie, keep the first occurrence.

        Args:
            results: Search results from one domain

        Returns:
            Deduplicated list preserving order of first occurrence
        """
        if not results:
            return results

        seen: Dict[tuple, int] = {}  # dedup_key -> index in deduped list
        deduped: List[SearchResult] = []

        for result in results:
            dedup_key = (result.item_id.lower(), result.item_type)

            if dedup_key in seen:
                existing_idx = seen[dedup_key]
                existing_score = deduped[existing_idx].relevance_score
                if result.relevance_score > existing_score:
                    deduped[existing_idx] = result
            else:
                seen[dedup_key] = len(deduped)
                deduped.append(result)

        return deduped

    def _merge_group(
        self,
        results_by_domain: Dict[str, List[SearchResult]],
        hard_token_limit: int
    ) -> List[Dict]:
        """Merge results grouped by domain"""
        estimator = TokenEstimator()
        total_tokens = 0
        merged = []

        for domain, results in results_by_domain.items():
            if not results:
                continue

            # Estimate tokens for this domain
            domain_items = [self._serialize_result(r) for r in results]
            domain_tokens = estimator.estimate_total_tokens(results)

            # Check if adding this domain would exceed limit
            if total_tokens + domain_tokens > hard_token_limit:
                # Add pagination marker
                merged.append({
                    "domain": domain,
                    "count": len(results),
                    "items": [],
                    "_truncated": True,
                    "_reason": "Token limit exceeded"
                })
                break

            # Add full domain results
            merged.append({
                "domain": domain,
                "count": len(results),
                "items": domain_items
            })
            total_tokens += domain_tokens

        return merged

    def _merge_interleave(
        self,
        results_by_domain: Dict[str, List[SearchResult]],
        hard_token_limit: int
    ) -> List[Dict]:
        """
        Merge results with interleaving and hard token limit.

        Interleaves results from different domains while respecting
        the token limit. When limit is reached, marks remaining domains
        as having more results.
        """
        merged = []
        estimator = TokenEstimator()
        total_tokens = 0

        # Create priority queues for each domain
        domain_queues = {}
        for domain, results in results_by_domain.items():
            if results:
                # Sort by relevance and create iterator
                sorted_results = sorted(results, key=lambda r: r.relevance_score, reverse=True)
                domain_queues[domain] = iter(sorted_results)

        # Interleave until queues empty or token limit reached
        iteration = 0
        max_iterations = 1000  # Prevent infinite loops

        while domain_queues and iteration < max_iterations:
            iteration += 1
            added_this_round = False

            for domain in list(domain_queues.keys()):
                try:
                    result = next(domain_queues[domain])

                    # Estimate tokens for this result
                    result_tokens = estimator.estimate_result_tokens(result)

                    # Check token limit BEFORE adding
                    if total_tokens + result_tokens > hard_token_limit:
                        # Token limit reached - mark remaining domains as having more
                        self._mark_pagination_for_remaining_domains(domain_queues, merged)
                        return merged

                    # Add result
                    serialized = self._serialize_result(result)
                    serialized["domain"] = domain
                    merged.append(serialized)

                    total_tokens += result_tokens
                    added_this_round = True

                except StopIteration:
                    # Domain exhausted
                    del domain_queues[domain]

            # Safety check: if nothing added and queues not empty, break
            if not added_this_round and domain_queues:
                break

        return merged

    def _merge_rank(
        self,
        results_by_domain: Dict[str, List[SearchResult]],
        hard_token_limit: int
    ) -> List[Dict]:
        """Merge results with global ranking and token limit"""
        estimator = TokenEstimator()
        total_tokens = 0
        all_results = []

        # Collect all results with adjusted scores
        for domain, results in results_by_domain.items():
            weight = self.DOMAIN_WEIGHTS.get(domain, 0.5)
            for result in results:
                adjusted_score = result.relevance_score * weight
                all_results.append({
                    "result": result,
                    "domain": domain,
                    "adjusted_score": adjusted_score  # Keep raw for sorting
                })

        # Find max score for normalization
        max_score = max((r["adjusted_score"] for r in all_results), default=1.0)
        if max_score <= 0:
            max_score = 1.0  # Prevent division by zero

        # Normalize scores to 0-1 range and round
        for item in all_results:
            item["adjusted_score"] = round(item["adjusted_score"] / max_score, 3)

        # Sort by normalized adjusted score
        all_results.sort(key=lambda x: x["adjusted_score"], reverse=True)

        # Add results until token limit
        merged = []
        for item in all_results:
            result = item["result"]
            result_tokens = estimator.estimate_result_tokens(result)

            if total_tokens + result_tokens > hard_token_limit:
                # Mark has_more and stop
                if merged:
                    last_result = merged[-1]
                    last_result["_pagination"] = {
                        "has_more": True,
                        "truncated_count": len(all_results) - len(merged)
                    }
                break

            serialized = self._serialize_result(result)
            serialized["domain"] = item["domain"]
            serialized["adjusted_score"] = item["adjusted_score"]
            merged.append(serialized)
            total_tokens += result_tokens

        return merged

    def _mark_pagination_for_remaining_domains(
        self,
        domain_queues: Dict[str, Any],
        current_results: List[Dict]
    ):
        """
        Mark which domains have more results.

        IMPORTANT: This method converts iterators to lists to safely check
        remaining elements WITHOUT consuming them (fix for pagination bug).
        """
        domains_with_more = []
        remaining_counts = {}

        for domain, queue in domain_queues.items():
            # Convert iterator to list to safely check remaining elements
            # without consuming them (fixes bug where next() consumed first element)
            remaining = list(queue)
            if remaining:
                domains_with_more.append(domain)
                remaining_counts[domain] = len(remaining)

        if domains_with_more and current_results:
            # Add pagination metadata to last result
            last_result = current_results[-1]
            if "_metadata" not in last_result:
                last_result["_metadata"] = {}
            last_result["_metadata"]["_pagination"] = {
                "has_more": True,
                "domains_with_more": domains_with_more,
                "remaining_counts": remaining_counts,
                "suggestion": "Narrow search_scope to specific domain or increase max_results"
            }

    def _serialize_result(self, result: SearchResult) -> Dict:
        """Serialize SearchResult to dict"""
        return {
            "domain": result.domain,
            "item_type": result.item_type,
            "item_id": result.item_id,
            "title": result.title,
            "summary": result.summary,
            "relevance_score": round(result.relevance_score, 3),
            "metadata": result.metadata,
            "quick_actions": result.quick_actions
        }


# ============================================================================
# Global Instance and Tool Registration
# ============================================================================

_composer_instance: Optional[SDKSearchComposer] = None


def get_search_composer() -> SDKSearchComposer:
    """Get or create search composer singleton"""
    global _composer_instance
    if _composer_instance is None:
        try:
            _composer_instance = SDKSearchComposer()
        except Exception as e:
            logger.error(f"Failed to create SDKSearchComposer: {e}")
            raise
    return _composer_instance


# Export for use in main.py
__all__ = [
    "SDKSearchComposer",
    "SearchEngine",
    "SearchScope",
    "SearchResult",
    "get_search_composer"
]
