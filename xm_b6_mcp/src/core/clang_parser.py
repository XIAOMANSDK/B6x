"""
Clang-based Code Parser for B6x SDK
====================================

Real-time C code analysis using libclang for semantic understanding.
Provides symbol reference finding, type deduction, and compilation diagnostics.

This module uses python-clang bindings (clang.cindex) to load libclang
directly without running clangd daemon.

Author: B6x MCP Server Team
Version: 1.1.0
"""

import os
import sys
import time
import logging
import hashlib
import platform
import asyncio
import threading
import concurrent.futures
from pathlib import Path
from typing import List, Dict, Optional, Tuple, Any, Set
from dataclasses import dataclass, field
from enum import Enum
from ctypes.util import find_library
from functools import partial

logger = logging.getLogger(__name__)


# ============================================================================
# Configuration
# ============================================================================

class ClangParserConfig:
    """Clang parser configuration"""

    # Timeout control
    ANALYSIS_TIMEOUT = 5.0        # Single analysis timeout (seconds)

    # Concurrency control
    MAX_CONCURRENT = 1            # Max concurrent parsing tasks

    # Cache configuration
    CACHE_TTL = 300               # Result cache time (seconds)
    CACHE_MAX_SIZE = 100          # Max cache entries

    # TU Pool (Translation Unit Pool)
    TU_POOL_SIZE = 3              # Number of TUs to keep in memory
    TU_MAX_MEMORY_MB = 500        # Max memory per TU (MB)

    # Input limits
    MAX_CODE_LENGTH = 2000        # Max code snippet length (chars)
    MAX_FILES = 3                 # Max context files
    MAX_FILE_SIZE_KB = 50         # Max file size (KB)

    # libclang version
    MIN_LIBCLANG_VERSION = (16, 0, 0)


# ============================================================================
# Error Definitions
# ============================================================================

class ClangErrorCode(str, Enum):
    """Error codes for Clang analysis"""
    LIBCLANG_NOT_FOUND = "LIBCLANG_NOT_FOUND"
    LIBCLANG_VERSION = "LIBCLANG_VERSION"
    LIBCLANG_LOAD_FAILED = "LIBCLANG_LOAD_FAILED"
    ANALYSIS_TIMEOUT = "ANALYSIS_TIMEOUT"
    PARSE_ERROR = "PARSE_ERROR"
    FILE_NOT_FOUND = "FILE_NOT_FOUND"
    INVALID_TYPE = "INVALID_TYPE"
    INPUT_TOO_LARGE = "INPUT_TOO_LARGE"
    TOO_MANY_FILES = "TOO_MANY_FILES"
    FILE_TOO_LARGE = "FILE_TOO_LARGE"


@dataclass
class ClangError:
    """Structured error response"""
    code: ClangErrorCode
    message: str
    solution: Optional[str] = None
    details: Optional[Dict[str, Any]] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for JSON response"""
        result = {
            "code": self.code.value,
            "message": self.message,
        }
        if self.solution:
            result["solution"] = self.solution
        if self.details:
            result.update(self.details)
        return result


# ============================================================================
# Analysis Types
# ============================================================================

class AnalysisType(str, Enum):
    """Analysis type enumeration"""
    REFS = "refs"         # Find all references
    TYPE = "type"         # Type deduction
    CHECK = "check"       # Compilation diagnostics


@dataclass
class ReferenceResult:
    """Single reference result"""
    file: str
    line: int
    column: int
    code: str
    context: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        return {
            "file": self.file,
            "line": self.line,
            "column": self.column,
            "code": self.code,
            "context": self.context,
        }


@dataclass
class TypeResult:
    """Type deduction result"""
    expression: str
    type: str
    kind: str  # builtin, pointer, struct, etc.
    size: Optional[int] = None
    declaration: Optional[str] = None
    comment: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        result = {
            "expression": self.expression,
            "type": self.type,
            "kind": self.kind,
        }
        if self.size is not None:
            result["size"] = self.size
        if self.declaration:
            result["declaration"] = self.declaration
        if self.comment:
            result["comment"] = self.comment
        return result


@dataclass
class DiagnosticResult:
    """Compilation diagnostic"""
    severity: str  # error, warning, info
    message: str
    file: str
    line: int
    column: int
    suggestion: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        result = {
            "severity": self.severity,
            "message": self.message,
            "file": self.file,
            "line": self.line,
            "column": self.column,
        }
        if self.suggestion:
            result["suggestion"] = self.suggestion
        return result


# ============================================================================
# Result Cache
# ============================================================================

class ClangResultCache:
    """Cache for Clang analysis results (P1-1: thread-safe with Lock)"""

    def __init__(self, max_size: int = 100, ttl: int = 300):
        self.cache: Dict[str, Tuple[Dict, float]] = {}
        self.max_size = max_size
        self.ttl = ttl
        self._lock = threading.Lock()  # Thread safety lock

    def _hash_query(
        self,
        code: str,
        analysis_type: AnalysisType,
        files: List[str]
    ) -> str:
        """Generate query hash"""
        content = f"{code}|{analysis_type.value}|{'|'.join(sorted(files))}"
        return hashlib.md5(content.encode()).hexdigest()

    def get(
        self,
        code: str,
        analysis_type: AnalysisType,
        files: List[str]
    ) -> Optional[Dict]:
        """Get cached result (thread-safe)"""
        key = self._hash_query(code, analysis_type, files)
        with self._lock:
            if key in self.cache:
                result, timestamp = self.cache[key]
                if time.time() - timestamp < self.ttl:
                    logger.debug(f"Cache hit for {analysis_type.value} query")
                    return result
                else:
                    del self.cache[key]
        return None

    def set(
        self,
        code: str,
        analysis_type: AnalysisType,
        files: List[str],
        result: Dict
    ):
        """Set cache result (thread-safe)"""
        with self._lock:
            if len(self.cache) >= self.max_size:
                # Remove oldest entry
                oldest_key = min(self.cache.keys(), key=lambda k: self.cache[k][1])
                del self.cache[oldest_key]

            key = self._hash_query(code, analysis_type, files)
            self.cache[key] = (result, time.time())

    def clear(self):
        """Clear all cache (thread-safe)"""
        with self._lock:
            self.cache.clear()


# ============================================================================
# Input Validation
# ============================================================================

def validate_input(
    code: str,
    analysis_type: str,
    files: List[str]
) -> Tuple[bool, Optional[ClangError]]:
    """
    Validate input parameters.

    Returns:
        (is_valid, error) tuple. error is None if valid.
    """
    # Check code length
    if len(code) > ClangParserConfig.MAX_CODE_LENGTH:
        return False, ClangError(
            code=ClangErrorCode.INPUT_TOO_LARGE,
            message=f"Code snippet exceeds {ClangParserConfig.MAX_CODE_LENGTH} character limit",
            solution="Reduce code snippet size",
            details={"actual_length": len(code), "max_length": ClangParserConfig.MAX_CODE_LENGTH}
        )

    # Check analysis type
    try:
        AnalysisType(analysis_type)
    except ValueError:
        return False, ClangError(
            code=ClangErrorCode.INVALID_TYPE,
            message=f"Invalid analysis type: {analysis_type}",
            solution="Use one of: refs, type, check",
            details={"valid_types": [t.value for t in AnalysisType]}
        )

    # Check file count
    if len(files) > ClangParserConfig.MAX_FILES:
        return False, ClangError(
            code=ClangErrorCode.TOO_MANY_FILES,
            message=f"Too many context files: {len(files)}",
            solution=f"Limit to {ClangParserConfig.MAX_FILES} files or fewer",
            details={"file_count": len(files), "max_files": ClangParserConfig.MAX_FILES}
        )

    # Check file existence and size
    for file_path in files:
        path = Path(file_path)
        if not path.exists():
            return False, ClangError(
                code=ClangErrorCode.FILE_NOT_FOUND,
                message=f"File not found: {file_path}",
                solution="Check file path and ensure file exists"
            )

        size_kb = path.stat().st_size / 1024
        if size_kb > ClangParserConfig.MAX_FILE_SIZE_KB:
            return False, ClangError(
                code=ClangErrorCode.FILE_TOO_LARGE,
                message=f"File too large: {file_path} ({size_kb:.1f}KB)",
                solution=f"Use files smaller than {ClangParserConfig.MAX_FILE_SIZE_KB}KB"
            )

    return True, None


# ============================================================================
# Clang Parser Class
# ============================================================================

class ClangParser:
    """
    Clang-based C code parser.

    Provides semantic code analysis using libclang:
    - Symbol reference finding (refs)
    - Type deduction (type)
    - Compilation diagnostics (check)

    All Clang operations are executed in a thread pool to avoid
    blocking the async event loop.
    """

    def __init__(self):
        self.index = None
        self.libclang_path: Optional[str] = None
        self.libclang_version: Optional[Tuple[int, int, int]] = None
        self._initialized = False
        self._load_error: Optional[ClangError] = None  # Store specific load error

        # Result cache
        self.cache = ClangResultCache(
            max_size=ClangParserConfig.CACHE_MAX_SIZE,
            ttl=ClangParserConfig.CACHE_TTL
        )

        # SDK context (loaded on demand)
        self._sdk_context = None

        # Search composer for Whoosh index search (loaded on demand)
        self._search_composer = None

        # Thread pool for synchronous Clang operations
        self._executor = concurrent.futures.ThreadPoolExecutor(
            max_workers=ClangParserConfig.MAX_CONCURRENT,
            thread_name_prefix="clang-"
        )

        # Try to initialize
        self._init_libclang()

    def _init_libclang(self) -> bool:
        """
        Initialize libclang library.

        Tries to load libclang from:
        1. Local bundled library (xm_b6_mcp/lib/clang/)
        2. System library
        3. LLVM installation
        """
        if self._initialized:
            return self.index is not None

        # Try to find libclang
        libclang_path = self._find_libclang()

        if not libclang_path:
            logger.warning("libclang not found")
            self._initialized = True
            return False

        try:
            # Set library path before importing clang
            if platform.system() == "Windows":
                os.environ.setdefault("LIBCLANG_PATH", libclang_path)

            # Import clang bindings
            from clang import cindex

            # Set library file
            if not cindex.Config.loaded:
                cindex.Config.set_library_file(libclang_path)

            # Create index
            self.index = cindex.Index.create()
            self.libclang_path = libclang_path

            # Get and validate version
            try:
                version_str = cindex.conf.lib.clang_getClangVersion()
                self.libclang_version = self._parse_version(version_str)

                # Validate minimum version
                if self.libclang_version and self.libclang_version < ClangParserConfig.MIN_LIBCLANG_VERSION:
                    logger.error(
                        f"libclang version {self.libclang_version} < required {ClangParserConfig.MIN_LIBCLANG_VERSION}"
                    )
                    self._load_error = ClangError(
                        code=ClangErrorCode.LIBCLANG_VERSION,
                        message=f"libclang version {self._format_version(self.libclang_version)} is too old",
                        solution=f"Upgrade to libclang >= {self._format_version(ClangParserConfig.MIN_LIBCLANG_VERSION)}",
                        details={
                            "current_version": self._format_version(self.libclang_version),
                            "required_version": self._format_version(ClangParserConfig.MIN_LIBCLANG_VERSION),
                            "download_url": "https://github.com/llvm/llvm-project/releases",
                        }
                    )
                    self._initialized = True
                    return False

                logger.info(f"libclang loaded: {libclang_path}, version: {self.libclang_version or version_str}")
            except Exception as e:
                logger.warning(f"Could not determine libclang version: {e}")
                logger.info(f"libclang loaded: {libclang_path}")

            self._initialized = True
            return True

        except ImportError as e:
            logger.error(f"Failed to import clang module: {e}")
            logger.info("Install with: pip install clang")
            self._initialized = True
            return False

        except Exception as e:
            logger.error(f"Failed to load libclang: {e}")
            self._initialized = True
            return False

    def _format_version(self, version: Tuple[int, int, int]) -> str:
        """Format version tuple as string"""
        return f"{version[0]}.{version[1]}.{version[2]}"

    def _parse_version(self, version_str: str) -> Optional[Tuple[int, int, int]]:
        """
        Parse libclang version string.

        Args:
            version_str: Version string like "clang version 16.0.0"

        Returns:
            Tuple of (major, minor, patch) or None if parsing fails
        """
        import re
        match = re.search(r'(\d+)\.(\d+)\.(\d+)', version_str)
        if match:
            return tuple(int(x) for x in match.groups())
        return None

    def _find_libclang(self) -> Optional[str]:
        """
        Find libclang library file.

        Search order:
        1. Local bundled library
        2. System library
        3. Common LLVM installation paths
        """
        system = platform.system()
        machine = platform.machine().lower()

        # Determine platform directory
        if system == "Windows":
            platform_dir = "win-x64"
            lib_names = ["libclang.dll"]
        elif system == "Darwin":
            platform_dir = "darwin-x64" if machine in ["x86_64", "amd64"] else "darwin-arm64"
            lib_names = ["libclang.dylib"]
        else:  # Linux
            platform_dir = "linux-x64"
            lib_names = ["libclang.so.16", "libclang.so.17", "libclang.so.18", "libclang.so"]

        # Search paths
        search_paths = []

        # 1. Local bundled library
        local_lib_dir = Path(__file__).parent.parent.parent / "lib" / "clang" / platform_dir
        for lib_name in lib_names:
            search_paths.append(local_lib_dir / lib_name)

        # 2. clang Python package bundled library
        try:
            import clang
            clang_package_dir = Path(clang.__file__).parent
            bundled_lib = clang_package_dir / "native" / "libclang.dll"
            if bundled_lib.exists():
                search_paths.append(bundled_lib)
                logger.debug(f"Found clang package bundled libclang: {bundled_lib}")
        except ImportError:
            pass

        # 3. System library (via find_library)
        for lib_name in lib_names:
            system_lib = find_library(lib_name.replace(".so.16", "").replace(".so.17", "").replace(".so.18", ""))
            if system_lib:
                search_paths.append(Path(system_lib))

        # 3. Common LLVM installation paths
        if system == "Windows":
            llvm_paths = [
                Path("C:/Program Files/LLVM/bin"),
                Path("C:/LLVM/bin"),
            ]
        elif system == "Darwin":
            llvm_paths = [
                Path("/usr/local/opt/llvm/lib"),
                Path("/opt/homebrew/opt/llvm/lib"),
            ]
        else:
            llvm_paths = [
                Path("/usr/lib/llvm-16/lib"),
                Path("/usr/lib/llvm-17/lib"),
                Path("/usr/lib/llvm-18/lib"),
                Path("/usr/lib"),
            ]

        for llvm_path in llvm_paths:
            for lib_name in lib_names:
                search_paths.append(llvm_path / lib_name)

        # Try each path
        for path in search_paths:
            if path.exists():
                logger.debug(f"Found libclang at: {path}")
                return str(path)

        logger.warning(f"libclang not found. Searched: {[str(p) for p in search_paths[:5]]}")
        return None

    @property
    def sdk_context(self):
        """Get SDK context (lazy load)"""
        if self._sdk_context is None:
            from src.core.sdk_context import get_sdk_context
            self._sdk_context = get_sdk_context()
        return self._sdk_context

    @property
    def search_composer(self):
        """Get search composer for Whoosh index search (lazy load)"""
        if self._search_composer is None:
            try:
                from src.layer1_discovery import get_search_composer
                self._search_composer = get_search_composer()
            except ImportError:
                logger.debug("Search composer not available, will use text search fallback")
            except Exception as e:
                logger.debug(f"Failed to load search composer: {e}, will use text search fallback")
        return self._search_composer

    def is_available(self) -> bool:
        """Check if libclang is available"""
        return self.index is not None

    def get_load_error(self) -> Optional[ClangError]:
        """Get error if libclang failed to load"""
        if self.is_available():
            return None

        # Return specific error if available (e.g., version mismatch)
        if self._load_error:
            return self._load_error

        return ClangError(
            code=ClangErrorCode.LIBCLANG_NOT_FOUND,
            message="libclang library not found or failed to load",
            solution="Ensure libclang is installed. Download from: https://github.com/llvm/llvm-project/releases",
            details={
                "tried_paths": [
                    "xm_b6_mcp/lib/clang/{platform}/libclang.*",
                    "System library",
                    "LLVM installation",
                ],
                "required_version": ">= 16.0.0",
                "download_url": "https://github.com/llvm/llvm-project/releases",
                "solution": "Install LLVM/Clang 16.0 or later, add libclang to system PATH or LIBCLANG_PATH environment variable"
            }
        )

    # ========================================================================
    # Main Analysis Methods
    # ========================================================================

    async def analyze(
        self,
        code: str,
        analysis_type: str,
        files: List[str] = None,
        sdk_context_override: Dict = None,
        brief: bool = True
    ) -> Dict[str, Any]:
        """
        Perform code analysis with timeout control.

        All synchronous Clang operations are executed in a thread pool
        to avoid blocking the async event loop.

        Args:
            code: Code snippet or symbol name
            analysis_type: Type of analysis (refs, type, check)
            files: Optional context files
            sdk_context_override: Override SDK context
            brief: Return brief output

        Returns:
            Analysis result dictionary
        """
        files = files or []

        # Check if libclang is available
        if not self.is_available():
            error = self.get_load_error()
            return {
                "success": False,
                "error": error.to_dict(),
            }

        # Validate input
        is_valid, error = validate_input(code, analysis_type, files)
        if not is_valid:
            return {
                "success": False,
                "error": error.to_dict(),
            }

        # Check cache
        atype = AnalysisType(analysis_type)
        cached = self.cache.get(code, atype, files)
        if cached:
            return cached

        # Perform analysis with timeout
        start_time = time.time()

        try:
            # Run analysis in thread pool with timeout (P0-3: use get_running_loop for Python 3.10+)
            loop = asyncio.get_running_loop()

            if atype == AnalysisType.REFS:
                analysis_func = partial(
                    self._analyze_refs_sync, code, files, sdk_context_override
                )
            elif atype == AnalysisType.TYPE:
                analysis_func = partial(
                    self._analyze_type_sync, code, files, sdk_context_override
                )
            elif atype == AnalysisType.CHECK:
                analysis_func = partial(
                    self._analyze_check_sync, code, files, sdk_context_override
                )
            else:
                return {
                    "success": False,
                    "error": {"code": "INVALID_TYPE", "message": f"Unknown type: {analysis_type}"}
                }

            # Execute with timeout using thread pool
            result = await asyncio.wait_for(
                loop.run_in_executor(self._executor, analysis_func),
                timeout=ClangParserConfig.ANALYSIS_TIMEOUT
            )

        except asyncio.TimeoutError:
            return {
                "success": False,
                "error": {
                    "code": ClangErrorCode.ANALYSIS_TIMEOUT.value,
                    "message": f"Analysis timed out (>{ClangParserConfig.ANALYSIS_TIMEOUT}s)",
                    "solution": "Simplify code snippet or reduce context files",
                }
            }

        except Exception as e:
            logger.error(f"Analysis failed: {e}", exc_info=True)
            return {
                "success": False,
                "error": {
                    "code": ClangErrorCode.PARSE_ERROR.value,
                    "message": str(e),
                }
            }

        # Add metadata
        parse_time_ms = round((time.time() - start_time) * 1000, 2)
        result["metadata"] = {
            "parse_time_ms": parse_time_ms,
            "files_analyzed": len(files),
            "sdk_context_used": sdk_context_override is None,
        }

        # Cache successful results
        if result.get("success"):
            self.cache.set(code, atype, files, result)

        return result

    async def _analyze_direct(
        self,
        code: str,
        analysis_type: str,
        files: List[str] = None,
        sdk_context_override: Dict = None,
        brief: bool = True
    ) -> Dict[str, Any]:
        """
        Perform code analysis directly, bypassing cache.

        This method is designed to be called by the ClangBatchWorker.
        It skips cache checks as the batcher handles deduplication.

        Args:
            code: Code snippet or symbol name
            analysis_type: Type of analysis (refs, type, check)
            files: Optional context files
            sdk_context_override: Override SDK context
            brief: Return brief output

        Returns:
            Analysis result dictionary
        """
        files = files or []

        # Check if libclang is available
        if not self.is_available():
            error = self.get_load_error()
            return {
                "success": False,
                "error": error.to_dict(),
            }

        # Validate input
        is_valid, error = validate_input(code, analysis_type, files)
        if not is_valid:
            return {
                "success": False,
                "error": error.to_dict(),
            }

        # Perform analysis with timeout (no cache check)
        start_time = time.time()
        atype = AnalysisType(analysis_type)

        try:
            loop = asyncio.get_running_loop()

            if atype == AnalysisType.REFS:
                analysis_func = partial(
                    self._analyze_refs_sync, code, files, sdk_context_override
                )
            elif atype == AnalysisType.TYPE:
                analysis_func = partial(
                    self._analyze_type_sync, code, files, sdk_context_override
                )
            elif atype == AnalysisType.CHECK:
                analysis_func = partial(
                    self._analyze_check_sync, code, files, sdk_context_override
                )
            else:
                return {
                    "success": False,
                    "error": {"code": "INVALID_TYPE", "message": f"Unknown type: {analysis_type}"}
                }

            # Execute with timeout using thread pool
            result = await asyncio.wait_for(
                loop.run_in_executor(self._executor, analysis_func),
                timeout=ClangParserConfig.ANALYSIS_TIMEOUT
            )

        except asyncio.TimeoutError:
            return {
                "success": False,
                "error": {
                    "code": ClangErrorCode.ANALYSIS_TIMEOUT.value,
                    "message": f"Analysis timed out (>{ClangParserConfig.ANALYSIS_TIMEOUT}s)",
                    "solution": "Simplify code snippet or reduce context files",
                }
            }

        except Exception as e:
            logger.error(f"Direct analysis failed: {e}", exc_info=True)
            return {
                "success": False,
                "error": {
                    "code": ClangErrorCode.PARSE_ERROR.value,
                    "message": str(e),
                }
            }

        # Add metadata
        parse_time_ms = round((time.time() - start_time) * 1000, 2)
        result["metadata"] = {
            "parse_time_ms": parse_time_ms,
            "files_analyzed": len(files),
            "sdk_context_used": sdk_context_override is None,
            "batch_processed": True,
        }

        return result

    # ========================================================================
    # Synchronous Analysis Methods (run in thread pool)
    # ========================================================================

    def _analyze_refs_sync(
        self,
        code: str,
        files: List[str],
        sdk_context_override: Dict = None
    ) -> Dict[str, Any]:
        """
        Synchronous reference finding - runs in thread pool.
        Implements cross-file reference search.
        """
        from clang import cindex

        # Build compile arguments
        args = self._get_compile_args(sdk_context_override)

        # Extract symbol name from code
        symbol_name = self._extract_symbol_name(code)

        # Get files to search (explicit files + SDK-wide search)
        files_to_search = self._get_files_for_symbol(symbol_name, files)

        all_refs: List[ReferenceResult] = []

        for file_path in files_to_search:
            if not Path(file_path).exists():
                continue

            try:
                # Parse file with detailed processing for macro/type definitions
                tu = self.index.parse(
                    file_path,
                    args=args,
                    options=(
                        cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES |
                        cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
                    )
                )

                # Find cursor for symbol
                for cursor in tu.cursor.walk_preorder():
                    if cursor.spelling == symbol_name:
                        # Found definition/declaration, now find references in this TU
                        refs = self._find_references_in_tu(cursor, file_path)
                        all_refs.extend(refs)

            except Exception as e:
                logger.warning(f"Failed to parse {file_path}: {e}")
                continue

        # Deduplicate results
        seen = set()
        unique_refs = []
        for ref in all_refs:
            key = (ref.file, ref.line, ref.column)
            if key not in seen:
                seen.add(key)
                unique_refs.append(ref)

        return {
            "success": True,
            "type": "refs",
            "symbol": symbol_name,
            "results": [r.to_dict() for r in unique_refs[:50]],  # Limit results
            "count": len(unique_refs),
            "truncated": len(unique_refs) > 50,
            "files_searched": len(files_to_search),
        }

    def _get_files_for_symbol(self, symbol_name: str, explicit_files: List[str]) -> List[str]:
        """
        Get list of files to search for a symbol.

        Combines:
        1. Explicitly provided files
        2. Inferred files from symbol name (e.g., B6x_UART_Init -> uart.c, uart.h)
        3. SDK-wide search for matching files

        Returns:
            List of file paths to search
        """
        files = list(explicit_files)  # Start with explicit files

        # Infer files from symbol name
        inferred = self._infer_files_from_symbol(symbol_name)
        for f in inferred:
            if f not in files and Path(f).exists():
                files.append(f)

        # SDK-wide search: find all C/H files that might contain the symbol
        sdk_wide = self._search_sdk_for_symbol(symbol_name)
        for f in sdk_wide:
            if f not in files:
                files.append(f)

        # Limit total files to prevent performance issues
        max_files = 20
        if len(files) > max_files:
            logger.info(f"Limiting search to {max_files} files (found {len(files)})")
            files = files[:max_files]

        return files

    def _infer_files_from_symbol(self, symbol_name: str) -> List[str]:
        """Infer likely source files from symbol name"""
        files = []

        # Map prefixes to driver files
        prefix_map = {
            "UART": ["uart"],
            "GPIO": ["gpio"],
            "SPI": ["spi", "spim"],
            "I2C": ["i2c"],
            "DMA": ["dma"],
            "TIMER": ["timer", "ctmr"],
            "PWM": ["pwm"],
            "ADC": ["sadc", "adc"],
            "RTC": ["rtc"],
            "EXTI": ["exti"],
            "RCC": ["rcc", "clock"],
            "FLASH": ["flash"],
            "CRC": ["crc"],
            "WDT": ["wdt", "watchdog"],
        }

        # BLE-specific patterns
        ble_patterns = [
            ("BLE_", ["ble_api", "ble_lib", "ble_app"]),
            ("ATT_", ["att", "gatt"]),
            ("GATT_", ["gatt", "att"]),
            ("L2CAP_", ["l2cap"]),
            ("SM_", ["sm", "security"]),
            ("HOGP_", ["hogp", "hid"]),
            ("BAS_", ["bas", "battery"]),
            ("DIS_", ["dis", "device_info"]),
        ]

        symbol_upper = symbol_name.upper()

        # Check driver prefixes
        for prefix, file_names in prefix_map.items():
            if prefix in symbol_upper:
                for name in file_names:
                    # Add header file
                    h_path = self.sdk_context.paths.drivers_api / f"{name}.h"
                    if h_path.exists():
                        files.append(str(h_path))
                    # Add source file
                    c_path = self.sdk_context.paths.drivers_src / f"{name}.c"
                    if c_path.exists():
                        files.append(str(c_path))
                break

        # Check BLE patterns
        for pattern, file_names in ble_patterns:
            if pattern in symbol_upper:
                for name in file_names:
                    for base_path in [self.sdk_context.paths.ble_api, self.sdk_context.paths.ble_lib]:
                        h_path = base_path / f"{name}.h"
                        if h_path.exists():
                            files.append(str(h_path))
                        c_path = base_path / f"{name}.c"
                        if c_path.exists():
                            files.append(str(c_path))
                break

        return files

    def _search_sdk_for_symbol(self, symbol_name: str) -> List[str]:
        """
        Search SDK for files that might contain the symbol.
        Uses Whoosh index search first, falls back to text search if unavailable.
        """
        files = []

        # Try using Whoosh index search first (faster)
        if self.search_composer:
            try:
                import asyncio
                # Run async search in sync context
                loop = asyncio.new_event_loop()
                try:
                    result = loop.run_until_complete(
                        self.search_composer.search(
                            query=symbol_name,
                            scope="all",
                            max_results=10,
                            merge_mode="rank"
                        )
                    )

                    if result.get("success"):
                        for item in result.get("results", []):
                            # Extract file path from search result
                            file_path = item.get("metadata", {}).get("file_path")
                            if file_path and Path(file_path).exists():
                                files.append(file_path)
                            elif item.get("item_id"):
                                # Try to get file from item_id pattern
                                item_id = item["item_id"]
                                if ":" in item_id:
                                    node_type, name = item_id.split(":", 1)
                                    # Infer file from API name pattern
                                    inferred = self._infer_files_from_symbol(name)
                                    files.extend([f for f in inferred if f not in files])

                    logger.debug(f"Whoosh index search found {len(files)} files for '{symbol_name}'")
                finally:
                    loop.close()

                if files:
                    return files[:10]  # Limit results

            except Exception as e:
                logger.debug(f"Whoosh index search failed: {e}, falling back to text search")

        # Fallback: Directories to search with text search
        # Priority: core_reg first for register/macro definitions, then API headers
        search_dirs = [
            self.sdk_context.paths.core_reg,  # Register definitions (UART1_BASE, UART_TypeDef, etc.)
            self.sdk_context.paths.drivers_api,
            self.sdk_context.paths.core,
            self.sdk_context.paths.drivers_src,
            self.sdk_context.paths.ble_api,
            self.sdk_context.paths.ble_lib,
            self.sdk_context.paths.ble_prf,
        ]

        # Quick text search for symbol in files
        for search_dir in search_dirs:
            if not search_dir or not search_dir.exists():
                continue

            try:
                for ext in ["*.c", "*.h"]:
                    for file_path in search_dir.rglob(ext):
                        try:
                            # Quick check: read first 10KB and search for symbol
                            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                                content = f.read(10240)
                                if symbol_name in content:
                                    files.append(str(file_path))
                                    if len(files) >= 10:  # Limit search
                                        return files
                        except Exception:
                            continue
            except Exception as e:
                logger.debug(f"Error searching {search_dir}: {e}")
                continue

        return files

    def _find_references_in_tu(
        self,
        cursor,
        source_file: str
    ) -> List[ReferenceResult]:
        """Find all references to a cursor in its translation unit"""
        from clang import cindex

        refs = []
        tu = cursor.translation_unit

        # Walk all cursors in TU
        for c in tu.cursor.walk_preorder():
            # Match by spelling
            if c.spelling == cursor.spelling:
                if c.location.file:
                    refs.append(ReferenceResult(
                        file=str(c.location.file),
                        line=c.location.line,
                        column=c.location.column,
                        code=self._get_line_of_code(c),
                        context=self._get_context(c),
                    ))

        return refs

    def _analyze_type_sync(
        self,
        code: str,
        files: List[str],
        sdk_context_override: Dict = None
    ) -> Dict[str, Any]:
        """
        Synchronous type analysis - runs in thread pool.

        Strategy:
        1. First try to find symbol definition in SDK files
        2. Parse the actual SDK file to get accurate type information
        3. Fall back to temporary file approach if SDK file not found
        """
        from clang import cindex

        # Extract symbol name from code
        symbol_name = self._extract_symbol_name(code)
        args = self._get_compile_args(sdk_context_override)

        # Strategy 1: Find symbol definition in SDK files
        sdk_files = self._search_sdk_for_symbol(symbol_name)
        if sdk_files:
            # Try parsing each SDK file to find type information
            for sdk_file in sdk_files[:3]:  # Limit to first 3 files
                try:
                    if not Path(sdk_file).exists():
                        continue

                    tu = self.index.parse(
                        sdk_file,
                        args=args,
                        options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES |
                                cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
                    )

                    # Walk AST to find the symbol and extract its type
                    for cursor in tu.cursor.walk_preorder():
                        if cursor.spelling == symbol_name:
                            # Check if this is a macro definition
                            if cursor.kind == cindex.CursorKind.MACRO_DEFINITION:
                                # For macros, try to extract type from the expansion
                                tokens = list(cursor.get_tokens())
                                if tokens:
                                    # Try to infer type from macro body
                                    macro_body = ''.join(t.spelling for t in tokens[1:])  # Skip macro name
                                    macro_type = self._infer_macro_type(macro_body, tu, args)
                                    return {
                                        "success": True,
                                        "type": "type",
                                        "result": {
                                            "expression": code,
                                            "type": macro_type.get("type", "macro"),
                                            "kind": "macro",
                                            "macro_body": macro_body.strip(),
                                            "source_file": str(cursor.location.file) if cursor.location.file else sdk_file,
                                            "source_line": cursor.location.line if cursor.location.file else 0,
                                        }
                                    }

                            # Found the symbol - extract type information
                            type_info = cursor.type
                            if type_info:
                                canonical_type = type_info.get_canonical()
                                return {
                                    "success": True,
                                    "type": "type",
                                    "result": {
                                        "expression": code,
                                        "type": canonical_type.spelling if canonical_type else type_info.spelling,
                                        "kind": type_info.kind.name.lower(),
                                        "source_file": str(cursor.location.file) if cursor.location.file else sdk_file,
                                        "source_line": cursor.location.line if cursor.location.file else 0,
                                    }
                                }

                except Exception as e:
                    logger.debug(f"Failed to parse SDK file {sdk_file}: {e}")
                    continue

        # Strategy 2: Fall back to temporary file approach with inline type definitions
        temp_code = f"""
// Type analysis wrapper - portable across compilers
{self._get_includes()}

/* Use __typeof__ for better ARM compiler compatibility
 * __typeof__ is the GCC/Clang extension with better portability
 */
#ifndef __typeof__
#define __typeof__ typeof
#endif

void __analysis_context__(void) {{
    /* Portable type deduction: evaluate expression and deduce type */
    __typeof__(({code})) __type_holder__ = ({code});
}}
"""

        try:
            tu = self.index.parse(
                "temp.c",
                unsaved_files=[("temp.c", temp_code)],
                args=args,
            )

            # Find the expression in the AST (P0-2 fix: direct type extraction)
            for cursor in tu.cursor.walk_preorder():
                if cursor.spelling == "__type_holder__":
                    # Directly get type from the variable declaration
                    # cursor.type gives us the type of __type_holder__ which is typeof((expr))
                    type_info = cursor.type
                    if type_info:
                        # Get the underlying type (remove typeof wrapper if present)
                        underlying_type = type_info.get_canonical()
                        return {
                            "success": True,
                            "type": "type",
                            "result": {
                                "expression": code,
                                "type": underlying_type.spelling if underlying_type else type_info.spelling,
                                "kind": type_info.kind.name.lower(),
                            }
                        }

            # Fallback: try to extract from diagnostics
            return {
                "success": True,
                "type": "type",
                "result": {
                    "expression": code,
                    "type": "unknown",
                    "kind": "unknown",
                    "note": "Could not determine exact type",
                }
            }

        except Exception as e:
            return {
                "success": False,
                "error": {
                    "code": ClangErrorCode.PARSE_ERROR.value,
                    "message": f"Type analysis failed: {str(e)}",
                }
            }

    def _analyze_check_sync(
        self,
        code: str,
        files: List[str],
        sdk_context_override: Dict = None
    ) -> Dict[str, Any]:
        """Synchronous code check - runs in thread pool"""
        # Create temporary file for parsing
        temp_code = f"""
{self._get_includes()}

// Code to check
{code}
"""

        args = self._get_compile_args(sdk_context_override)

        try:
            from clang import cindex

            tu = self.index.parse(
                "temp.c",
                unsaved_files=[("temp.c", temp_code)],
                args=args,
            )

            diagnostics = []
            for diag in tu.diagnostics:
                severity = "info"
                if diag.severity >= cindex.Diagnostic.Error:
                    severity = "error"
                elif diag.severity >= cindex.Diagnostic.Warning:
                    severity = "warning"

                diagnostics.append(DiagnosticResult(
                    severity=severity,
                    message=diag.spelling,
                    file=str(diag.location.file) if diag.location.file else "temp.c",
                    line=diag.location.line if diag.location.file else 0,
                    column=diag.location.column if diag.location.file else 0,
                    suggestion=self._get_diagnostic_suggestion(diag),
                ))

            return {
                "success": True,
                "type": "check",
                "diagnostics": [d.to_dict() for d in diagnostics],
                "summary": {
                    "errors": sum(1 for d in diagnostics if d.severity == "error"),
                    "warnings": sum(1 for d in diagnostics if d.severity == "warning"),
                    "infos": sum(1 for d in diagnostics if d.severity == "info"),
                }
            }

        except Exception as e:
            return {
                "success": False,
                "error": {
                    "code": ClangErrorCode.PARSE_ERROR.value,
                    "message": f"Code check failed: {str(e)}",
                }
            }

    # ========================================================================
    # Helper Methods
    # ========================================================================

    def _get_compile_args(self, sdk_context_override: Dict = None) -> List[str]:
        """Get compile arguments from SDK context"""
        if sdk_context_override:
            # Use override context
            args = []
            for path in sdk_context_override.get("include_paths", []):
                args.extend(["-I", path])
            for name, value in sdk_context_override.get("defines", {}).items():
                args.append(f"-D{name}={value}")
            return args

        # Use SDK context
        return self.sdk_context.get_compile_args()

    def _get_includes(self) -> str:
        """Get common includes for code analysis.

        Note: For ARM embedded target, we can't use system headers.
        Instead, we define standard types inline and include SDK headers.
        We use include guards to prevent stdint.h inclusion errors.
        """
        # Build include statements for SDK headers
        sdk_includes = ""
        include_paths = self.sdk_context.get_include_paths()
        for path in include_paths[:3]:  # Include first 3 SDK paths
            sdk_includes += f"/* SDK path: {path} */\n"

        return f"""
/* Prevent stdint.h inclusion - we define types inline */
#define _STDINT_H
#define __STDINT_H
#define _STDINT_H_

/* Standard integer types for embedded analysis */
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

/* Additional ARM types */
typedef unsigned char uint_fast8_t;
typedef signed char int_fast8_t;
typedef unsigned short uint_fast16_t;
typedef signed short int_fast16_t;
typedef unsigned int uint_fast32_t;
typedef signed int int_fast32_t;
typedef unsigned long long uint_fast64_t;
typedef signed long long int_fast64_t;
typedef unsigned int uintptr_t;
typedef signed int intptr_t;
typedef unsigned int uint_least8_t;
typedef signed int int_least8_t;
typedef unsigned short uint_least16_t;
typedef signed short int_least16_t;
typedef unsigned int uint_least32_t;
typedef signed int int_least32_t;
typedef unsigned long long uint_least64_t;
typedef signed long long int_least64_t;
typedef long long intmax_t;
typedef unsigned long long uintmax_t;

/* Standard bool type */
typedef _Bool bool;
#define true 1
#define false 0

/* Standard size types */
typedef unsigned int size_t;
typedef signed int ptrdiff_t;
#define NULL ((void*)0)

/* SDK common includes - provides access to SDK types and APIs */
#include "B6x.h"
#include "gpio.h"
#include "uart.h"
#include "spi.h"
#include "i2c.h"
#include "timer.h"
#include "pwm.h"
#include "sadc.h"
#include "dma.h"
#include "rtc.h"

{sdk_includes}
"""

    def _infer_macro_type(self, macro_body: str, tu, args: List[str]) -> Optional[Dict]:
        """
        Infer type from macro definition body.

        For macros like:
        - #define UART1 ((UART_TypeDef *)UART1_BASE)
        - #define UART1_BASE ((uint32_t)0x40023000)

        Returns dict with 'type' and 'confidence' keys.
        """
        import re

        # Remove leading/trailing whitespace
        macro_body = macro_body.strip()

        # Pattern 1: Double paren cast - ((TYPE...)...)  handles spaces like (( UART_TypeDef * ))
        # Match: ((TYPE *) or ((TYPE*)
        double_cast_match = re.match(
            r'\(\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*(\*?)\s*\)',
            macro_body
        )
        if double_cast_match:
            type_name = double_cast_match.group(1).strip()
            pointer = double_cast_match.group(2)
            inferred_type = f"{type_name} {pointer}".strip() if pointer else type_name
            return {
                "type": inferred_type,
                "confidence": "high",
                "method": "double_paren_cast"
            }

        # Pattern 2: Simple cast - (TYPE)...
        simple_cast_match = re.match(r'\(\s*([A-Za-z_][A-Za-z0-9_]*\s*\*?)\s*\)', macro_body)
        if simple_cast_match:
            inferred_type = simple_cast_match.group(1).strip()
            inferred_type = re.sub(r'\s+', ' ', inferred_type)
            return {
                "type": inferred_type,
                "confidence": "medium",
                "method": "simple_cast"
            }

        # Pattern 3: Number literal - infer numeric type
        if re.match(r'^0x[0-9A-Fa-f]+$', macro_body):
            return {
                "type": "int",
                "confidence": "medium",
                "method": "hex_literal"
            }
        if re.match(r'^\d+$', macro_body):
            return {
                "type": "int",
                "confidence": "medium",
                "method": "decimal_literal"
            }
        if re.match(r'^\d+\.\d+', macro_body):
            return {
                "type": "double",
                "confidence": "medium",
                "method": "float_literal"
            }

        # Pattern 4: String literal
        if macro_body.startswith('"'):
            return {
                "type": "const char *",
                "confidence": "high",
                "method": "string_literal"
            }

        # Pattern 5: Character literal
        if macro_body.startswith("'"):
            return {
                "type": "char",
                "confidence": "high",
                "method": "char_literal"
            }

        # Could not infer type
        return {
            "type": "unknown",
            "confidence": "none",
            "method": "fallback"
        }

    def _extract_symbol_name(self, code: str) -> str:
        """Extract symbol name from code snippet"""
        # Simple extraction: take first identifier
        import re
        match = re.search(r'\b([A-Za-z_][A-Za-z0-9_]*)\b', code)
        return match.group(1) if match else code.strip()

    def _get_line_of_code(self, cursor) -> str:
        """Get the line of code at cursor location"""
        try:
            if cursor.location.file:
                with open(cursor.location.file, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                    if 0 < cursor.location.line <= len(lines):
                        return lines[cursor.location.line - 1].strip()
        except Exception:
            pass
        return cursor.spelling

    def _get_context(self, cursor) -> Optional[str]:
        """Get enclosing context (function/class name)"""
        parent = cursor.semantic_parent
        if parent:
            return f"{parent.kind.name} {parent.spelling}"
        return None

    def _get_diagnostic_suggestion(self, diagnostic) -> Optional[str]:
        """Get suggestion for diagnostic"""
        msg = diagnostic.spelling.lower()

        if "unused variable" in msg:
            return "Remove unused variable or use it in the code"
        elif "undeclared" in msg:
            return "Check spelling or add missing include"
        elif "incompatible" in msg:
            return "Check type compatibility"
        elif "expected" in msg:
            return "Fix syntax error"

        return None

    def shutdown(self):
        """Shutdown the parser and cleanup resources"""
        if self._executor:
            self._executor.shutdown(wait=False)
            self._executor = None
        logger.info("ClangParser shutdown complete")


# ============================================================================
# Global Instance
# ============================================================================

_parser_instance: Optional[ClangParser] = None


def get_clang_parser() -> ClangParser:
    """Get or create Clang parser singleton"""
    global _parser_instance
    if _parser_instance is None:
        _parser_instance = ClangParser()
    return _parser_instance


# ============================================================================
# Exports
# ============================================================================

__all__ = [
    "ClangParser",
    "ClangParserConfig",
    "ClangError",
    "ClangErrorCode",
    "AnalysisType",
    "validate_input",
    "get_clang_parser",
]
