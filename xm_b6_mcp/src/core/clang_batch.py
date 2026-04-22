"""
Clang Analysis Request Batching System
=======================================

Provides request deduplication, queue management, and backpressure
for clang_analyze tool to prevent server overload when LLM triggers
multiple rapid requests.

Author: B6x MCP Server Team
Version: 1.0.0
"""

import asyncio
import hashlib
import logging
import threading
import time
from dataclasses import dataclass, field
from enum import Enum
from typing import Dict, List, Optional, Any, Callable
from datetime import datetime

logger = logging.getLogger(__name__)


# ============================================================================
# Request State
# ============================================================================

class RequestState(Enum):
    """State of a pending request"""
    PENDING = "pending"
    PROCESSING = "processing"
    COMPLETED = "completed"
    FAILED = "failed"


# ============================================================================
# Data Classes
# ============================================================================

@dataclass
class PendingRequest:
    """Represents a pending analysis request"""
    request_id: str
    request_key: str  # SHA256 hash for deduplication
    code: str
    analysis_type: str
    files: List[str]
    sdk_context: Optional[Dict]
    brief: bool
    state: RequestState = RequestState.PENDING
    result: Optional[Dict[str, Any]] = None
    event: asyncio.Event = field(default_factory=asyncio.Event)
    waiters: List[str] = field(default_factory=list)
    created_at: float = field(default_factory=time.time)
    started_at: Optional[float] = None
    completed_at: Optional[float] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for logging/metrics"""
        return {
            "request_id": self.request_id,
            "request_key": self.request_key[:16] + "...",
            "analysis_type": self.analysis_type,
            "state": self.state.value,
            "waiter_count": len(self.waiters),
            "created_at": self.created_at,
            "wait_time_ms": round((self.started_at or time.time()) - self.created_at, 3) * 1000 if self.created_at else 0,
        }


@dataclass
class BatchConfig:
    """Configuration for request batching"""
    max_queue_size: int = 50           # Maximum queued requests
    max_waiters_per_request: int = 10  # Maximum waiters sharing one request
    batch_window_ms: int = 50          # Batch collection window (milliseconds)
    queue_timeout_ms: int = 30000      # Queue wait timeout (milliseconds)
    processing_timeout_ms: int = 10000 # Single request processing timeout
    worker_poll_interval_ms: int = 10  # Worker poll interval


# ============================================================================
# Error Responses
# ============================================================================

class BatchError:
    """Standard batch processing error responses"""

    @staticmethod
    def queue_overloaded(retry_after_ms: int = 1000) -> Dict[str, Any]:
        return {
            "success": False,
            "error": {
                "code": "QUEUE_OVERLOADED",
                "message": "Request queue is full.",
                "solution": "Wait and retry.",
                "retry_after_ms": retry_after_ms
            }
        }

    @staticmethod
    def queue_timeout(timeout_ms: int) -> Dict[str, Any]:
        return {
            "success": False,
            "error": {
                "code": "QUEUE_TIMEOUT",
                "message": f"Request timed out in queue (>{timeout_ms}ms)"
            }
        }

    @staticmethod
    def max_waiters_exceeded(max_waiters: int) -> Dict[str, Any]:
        return {
            "success": False,
            "error": {
                "code": "MAX_WAITERS_EXCEEDED",
                "message": f"Too many waiters for this request (>{max_waiters})",
                "solution": "Wait for existing request to complete and retry."
            }
        }

    @staticmethod
    def worker_not_running() -> Dict[str, Any]:
        return {
            "success": False,
            "error": {
                "code": "WORKER_NOT_RUNNING",
                "message": "Batch worker is not running.",
                "solution": "Check server logs for worker initialization errors."
            }
        }


# ============================================================================
# Request Batcher
# ============================================================================

class ClangRequestBatcher:
    """
    Manages request batching, deduplication, and queue control.

    This class provides:
    - Request deduplication via SHA256 hash of (code, type, files)
    - Queue size limits for backpressure
    - Waiter aggregation for duplicate requests
    - Metrics collection
    """

    _instance: Optional['ClangRequestBatcher'] = None
    _lock = threading.Lock()

    def __new__(cls, config: BatchConfig = None):
        """Singleton pattern for global batcher"""
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    cls._instance = super().__new__(cls)
                    cls._instance._initialized = False
        return cls._instance

    def __init__(self, config: BatchConfig = None):
        """Initialize the batcher (only once for singleton)"""
        if self._initialized:
            return

        self.config = config or BatchConfig()
        self._queue: asyncio.Queue[PendingRequest] = None  # Created in async context
        self._pending_requests: Dict[str, PendingRequest] = {}  # key -> request
        self._request_counter = 0
        self._lock = asyncio.Lock()
        self._metrics = {
            "total_requests": 0,
            "deduplicated": 0,
            "queue_overloads": 0,
            "timeouts": 0,
            "processed": 0,
            "failed": 0,
        }
        self._initialized = True
        self._loop: Optional[asyncio.AbstractEventLoop] = None

        logger.info(f"ClangRequestBatcher initialized with config: max_queue={self.config.max_queue_size}")

    def _ensure_queue(self):
        """Ensure queue exists in current event loop"""
        if self._queue is None:
            self._queue = asyncio.Queue(maxsize=self.config.max_queue_size)

    def _generate_request_key(self, code: str, analysis_type: str, files: List[str]) -> str:
        """Generate SHA256 hash for request deduplication"""
        content = f"{code}|{analysis_type}|{','.join(sorted(files))}"
        return hashlib.sha256(content.encode('utf-8')).hexdigest()

    def _generate_request_id(self) -> str:
        """Generate unique request ID"""
        self._request_counter += 1
        return f"clang-{int(time.time() * 1000)}-{self._request_counter}"

    async def submit_request(
        self,
        code: str,
        analysis_type: str,
        files: List[str],
        sdk_context: Optional[Dict] = None,
        brief: bool = True,
        waiter_id: str = None
    ) -> Dict[str, Any]:
        """
        Submit a request for batch processing.

        Args:
            code: Code snippet or symbol name
            analysis_type: Type of analysis (refs, type, check)
            files: Context files
            sdk_context: SDK context override
            brief: Return brief output
            waiter_id: Optional identifier for the waiter

        Returns:
            Analysis result dictionary
        """
        self._ensure_queue()
        self._loop = asyncio.get_running_loop()

        waiter_id = waiter_id or self._generate_request_id()
        request_key = self._generate_request_key(code, analysis_type, files)
        self._metrics["total_requests"] += 1

        async with self._lock:
            # Check for duplicate request
            if request_key in self._pending_requests:
                existing = self._pending_requests[request_key]

                # Check if we can add another waiter
                if len(existing.waiters) >= self.config.max_waiters_per_request:
                    self._metrics["queue_overloads"] += 1
                    return BatchError.max_waiters_exceeded(self.config.max_waiters_per_request)

                # Add as waiter
                existing.waiters.append(waiter_id)
                self._metrics["deduplicated"] += 1
                logger.debug(f"Request deduplicated: {request_key[:16]}... now has {len(existing.waiters)} waiters")

                # Wait for result outside the lock
                request = existing
            else:
                # Check queue capacity
                if self._queue.qsize() >= self.config.max_queue_size:
                    self._metrics["queue_overloads"] += 1
                    return BatchError.queue_overloaded()

                # Create new request
                request = PendingRequest(
                    request_id=self._generate_request_id(),
                    request_key=request_key,
                    code=code,
                    analysis_type=analysis_type,
                    files=files,
                    sdk_context=sdk_context,
                    brief=brief,
                    state=RequestState.PENDING,
                    waiters=[waiter_id],
                )

                self._pending_requests[request_key] = request

                try:
                    self._queue.put_nowait(request)
                    logger.debug(f"Request queued: {request.request_id}, queue_size={self._queue.qsize()}")
                except asyncio.QueueFull:
                    self._metrics["queue_overloads"] += 1
                    del self._pending_requests[request_key]
                    return BatchError.queue_overloaded()

        # Wait for result (outside lock)
        try:
            # Wait with timeout
            await asyncio.wait_for(
                request.event.wait(),
                timeout=self.config.queue_timeout_ms / 1000.0
            )
        except asyncio.TimeoutError:
            self._metrics["timeouts"] += 1
            logger.warning(f"Request timed out in queue: {request.request_id}")
            return BatchError.queue_timeout(self.config.queue_timeout_ms)

        # Return result
        if request.state == RequestState.FAILED:
            self._metrics["failed"] += 1
        else:
            self._metrics["processed"] += 1

        return request.result or BatchError.queue_timeout(self.config.queue_timeout_ms)

    async def get_next_request(self, timeout_ms: int = None) -> Optional[PendingRequest]:
        """
        Get next request from queue (for worker).

        Args:
            timeout_ms: Timeout for waiting

        Returns:
            PendingRequest or None if timeout
        """
        self._ensure_queue()
        timeout = (timeout_ms or self.config.worker_poll_interval_ms) / 1000.0

        try:
            request = await asyncio.wait_for(
                self._queue.get(),
                timeout=timeout
            )
            request.state = RequestState.PROCESSING
            request.started_at = time.time()
            return request
        except asyncio.TimeoutError:
            return None

    def complete_request(self, request: PendingRequest, result: Dict[str, Any]):
        """
        Mark request as completed and notify all waiters.

        Args:
            request: The completed request
            result: Analysis result
        """
        request.result = result
        request.state = RequestState.COMPLETED if result.get("success") else RequestState.FAILED
        request.completed_at = time.time()

        # Notify all waiters
        request.event.set()

        # Clean up from pending dict
        async def cleanup():
            async with self._lock:
                if request.request_key in self._pending_requests:
                    del self._pending_requests[request.request_key]

        # Schedule cleanup
        if self._loop and not self._loop.is_closed():
            self._loop.call_soon_threadsafe(
                lambda: asyncio.ensure_future(cleanup(), loop=self._loop)
            )

        logger.debug(f"Request completed: {request.request_id}, waiters_notified={len(request.waiters)}")

    def get_metrics(self) -> Dict[str, Any]:
        """Get batcher metrics"""
        queue_size = self._queue.qsize() if self._queue else 0
        return {
            **self._metrics,
            "queue_size": queue_size,
            "max_queue_size": self.config.max_queue_size,
            "pending_requests": len(self._pending_requests),
            "deduplication_rate": (
                self._metrics["deduplicated"] / self._metrics["total_requests"]
                if self._metrics["total_requests"] > 0 else 0
            ),
        }

    def clear(self):
        """Clear queue and pending requests (for testing)"""
        if self._queue:
            while not self._queue.empty():
                try:
                    self._queue.get_nowait()
                except asyncio.QueueEmpty:
                    break
        self._pending_requests.clear()


def get_batcher(config: BatchConfig = None) -> ClangRequestBatcher:
    """Get the global batcher instance"""
    return ClangRequestBatcher(config)


# ============================================================================
# Batch Worker
# ============================================================================

class ClangBatchWorker:
    """
    Background worker for processing queued clang analysis requests.

    The worker runs continuously, processing requests from the batcher queue.
    """

    def __init__(self, batcher: ClangRequestBatcher = None):
        self._batcher = batcher or get_batcher()
        self._running = False
        self._task: Optional[asyncio.Task] = None
        self._analyzer: Optional[Callable] = None

    async def start(self, analyzer: Callable = None):
        """
        Start the worker.

        Args:
            analyzer: Async callable that performs the actual analysis.
                     Signature: async fn(code, type, files, sdk_context, brief) -> dict
        """
        if self._running:
            logger.warning("Batch worker already running")
            return

        self._analyzer = analyzer
        self._running = True
        self._task = asyncio.create_task(self._worker_loop())
        logger.info("ClangBatchWorker started")

    async def stop(self):
        """Stop the worker gracefully"""
        if not self._running:
            return

        self._running = False

        if self._task:
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass
            self._task = None

        logger.info("ClangBatchWorker stopped")

    async def _worker_loop(self):
        """Main worker loop"""
        logger.debug("Batch worker loop started")

        while self._running:
            try:
                # Get next request with timeout
                request = await self._batcher.get_next_request(
                    timeout_ms=self._batcher.config.worker_poll_interval_ms
                )

                if request is None:
                    # Timeout, continue polling
                    continue

                # Process the request
                await self._process_request(request)

            except asyncio.CancelledError:
                logger.debug("Batch worker loop cancelled")
                break
            except Exception as e:
                logger.error(f"Batch worker error: {e}", exc_info=True)
                # Brief pause before continuing
                await asyncio.sleep(0.1)

        logger.debug("Batch worker loop ended")

    async def _process_request(self, request: PendingRequest):
        """Process a single request"""
        logger.debug(f"Processing request: {request.request_id}")

        try:
            # Use provided analyzer or default
            if self._analyzer:
                result = await self._analyzer(
                    code=request.code,
                    analysis_type=request.analysis_type,
                    files=request.files,
                    sdk_context=request.sdk_context,
                    brief=request.brief
                )
            else:
                # Default: use ClangParser directly
                from src.core.clang_parser import get_clang_parser
                parser = get_clang_parser()
                result = await parser.analyze(
                    code=request.code,
                    analysis_type=request.analysis_type,
                    files=request.files,
                    sdk_context_override=request.sdk_context,
                    brief=request.brief
                )

            self._batcher.complete_request(request, result)

        except Exception as e:
            logger.error(f"Request processing failed: {e}", exc_info=True)
            self._batcher.complete_request(request, {
                "success": False,
                "error": {
                    "code": "PROCESSING_ERROR",
                    "message": str(e)
                }
            })

    @property
    def is_running(self) -> bool:
        """Check if worker is running"""
        return self._running


# Global worker instance
_worker_instance: Optional[ClangBatchWorker] = None


async def start_batch_worker(analyzer: Callable = None, config: BatchConfig = None) -> ClangBatchWorker:
    """
    Start the global batch worker.

    Args:
        analyzer: Optional analyzer function
        config: Optional batch configuration

    Returns:
        The worker instance
    """
    global _worker_instance

    if _worker_instance is not None and _worker_instance.is_running:
        logger.warning("Batch worker already running")
        return _worker_instance

    # Initialize batcher with config
    batcher = get_batcher(config)
    _worker_instance = ClangBatchWorker(batcher)
    await _worker_instance.start(analyzer)

    return _worker_instance


async def stop_batch_worker():
    """Stop the global batch worker"""
    global _worker_instance

    if _worker_instance is not None:
        await _worker_instance.stop()
        _worker_instance = None


def get_worker() -> Optional[ClangBatchWorker]:
    """Get the global worker instance"""
    return _worker_instance


# ============================================================================
# Exports
# ============================================================================

__all__ = [
    "RequestState",
    "PendingRequest",
    "BatchConfig",
    "BatchError",
    "ClangRequestBatcher",
    "ClangBatchWorker",
    "get_batcher",
    "start_batch_worker",
    "stop_batch_worker",
    "get_worker",
]
