#!/usr/bin/env python3
"""
Build Cache Management - Incremental Build Support
===================================================

Manages file hash tracking for incremental builds to avoid unnecessary
full rebuilds when only a few files have changed.

Cache Strategy:
- Hybrid mtime + MD5 change detection
- Fast path: mtime/size check (no file read)
- Slow path: MD5 hash (only when mtime/size changed)
- Conservative: Cache Tree-sitter and document parsing only

Author: B6x MCP Server Team
Version: 1.0.0
"""

import json
import hashlib
import logging
from pathlib import Path
from datetime import datetime
from typing import Dict, Optional, List, Tuple

logger = logging.getLogger(__name__)


class BuildCacheManager:
    """
    Manages file hash tracking for incremental builds.

    This class provides:
    - File change detection using hybrid mtime + MD5 strategy
    - Cache storage and retrieval
    - Granular cache invalidation by category
    - Cache consistency validation
    """

    def __init__(self, cache_file: Path, sdk_root: Path):
        """
        Initialize BuildCacheManager.

        Args:
            cache_file: Path to cache JSON file (typically data/file_hashes.json)
            sdk_root: Root directory of SDK (for relative path calculations)
        """
        self.cache_file = cache_file
        self.sdk_root = sdk_root
        self.cache_data = self._load_cache()

    def _load_cache(self) -> Dict:
        """
        Load existing cache or create new structure.

        Returns:
            Cache dictionary with file_hashes, metadata
        """
        if self.cache_file.exists():
            try:
                with open(self.cache_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                # Validate cache structure
                if self._validate_cache_structure(data):
                    logger.info(f"Loaded build cache from: {self.cache_file}")
                    return data
                else:
                    logger.warning("Cache structure invalid, creating new cache")

            except json.JSONDecodeError as e:
                logger.warning(f"Cache file corrupted: {e}, creating new cache")
            except Exception as e:
                logger.warning(f"Failed to load cache: {e}, creating new cache")

        # Create new cache structure
        logger.info("Creating new build cache")
        return {
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "sdk_root": str(self.sdk_root),
            "file_hashes": {
                "excel_constraints": {},
                "documents": {},
                "c_headers": {},
                "c_sources": {}
            }
        }

    def _validate_cache_structure(self, data: Dict) -> bool:
        """
        Validate cache structure.

        Args:
            data: Cache dictionary to validate

        Returns:
            True if valid, False otherwise
        """
        required_keys = ["version", "file_hashes"]
        if not all(key in data for key in required_keys):
            return False

        if not isinstance(data["file_hashes"], dict):
            return False

        return True

    def _save_cache(self):
        """Save cache to disk."""
        try:
            self.cache_data["last_updated"] = datetime.now().isoformat()
            self.cache_file.parent.mkdir(parents=True, exist_ok=True)

            with open(self.cache_file, 'w', encoding='utf-8') as f:
                json.dump(self.cache_data, f, indent=2, ensure_ascii=False)

            logger.debug(f"Saved cache to: {self.cache_file}")

        except Exception as e:
            logger.error(f"Failed to save cache: {e}")

    def _compute_md5(self, file_path: Path) -> Optional[str]:
        """
        Compute MD5 hash of a file.

        Args:
            file_path: Path to file

        Returns:
            MD5 hash string, or None if error
        """
        try:
            with open(file_path, 'rb') as f:
                return hashlib.md5(f.read()).hexdigest()
        except Exception as e:
            logger.error(f"Failed to compute MD5 for {file_path}: {e}")
            return None

    def is_file_changed(self, file_category: str, file_path: Path,
                       relative_path: str) -> bool:
        """
        Check if file has changed using hybrid mtime + MD5 strategy.

        Strategy:
        1. Check if file exists in cache
        2. Compare mtime and size (fast check)
        3. If mtime/size changed, compute MD5 (slow but accurate)
        4. Return True if file changed, False if cached version is valid

        Args:
            file_category: Category key (e.g., "c_headers", "documents")
            file_path: Absolute path to file
            relative_path: Relative path from sdk_root (for cache key)

        Returns:
            True if file changed, False if cached version is valid
        """
        if not file_path.exists():
            logger.warning(f"File no longer exists: {relative_path}")
            return True

        # Get current file info
        stat = file_path.stat()
        current_mtime = stat.st_mtime
        current_size = stat.st_size

        # Check cache
        category_cache = self.cache_data["file_hashes"].get(file_category, {})
        cached_info = category_cache.get(relative_path)

        if not cached_info:
            logger.debug(f"New file (not in cache): {relative_path}")
            return True

        # Fast check: mtime and size
        if (cached_info.get("mtime") == current_mtime and
            cached_info.get("size_bytes") == current_size):
            logger.debug(f"Cache hit (mtime): {relative_path}")
            return False

        # Slow check: MD5 if mtime or size changed
        logger.debug(f"Cache miss (mtime), checking MD5: {relative_path}")
        current_md5 = self._compute_md5(file_path)

        if current_md5 is None:
            # Error computing MD5, assume changed
            return True

        cached_md5 = cached_info.get("md5")
        if cached_md5 == current_md5:
            # File content same but mtime different (e.g., touch, copy)
            # Update mtime in cache
            cached_info["mtime"] = current_mtime
            self._save_cache()
            logger.debug(f"Cache hit (MD5, mtime updated): {relative_path}")
            return False

        logger.info(f"File changed: {relative_path}")
        return True

    def update_file_cache(self, file_category: str, file_path: Path,
                         relative_path: str, outputs: List[str] = None):
        """
        Update cache entry for a file after processing.

        Args:
            file_category: Category key (e.g., "c_headers", "documents")
            file_path: Absolute path to file
            relative_path: Relative path from sdk_root (for cache key)
            outputs: List of output files generated from this file
        """
        if not file_path.exists():
            logger.warning(f"Cannot update cache for non-existent file: {relative_path}")
            return

        current_md5 = self._compute_md5(file_path)
        if current_md5 is None:
            logger.warning(f"Failed to compute MD5 for cache update: {relative_path}")
            return

        stat = file_path.stat()

        # Ensure category exists
        if file_category not in self.cache_data["file_hashes"]:
            self.cache_data["file_hashes"][file_category] = {}

        # Update cache entry
        self.cache_data["file_hashes"][file_category][relative_path] = {
            "path": str(file_path),
            "md5": current_md5,
            "size_bytes": stat.st_size,
            "mtime": stat.st_mtime,
            "last_built": datetime.now().isoformat(),
            "outputs": outputs or []
        }

        self._save_cache()
        logger.debug(f"Updated cache for: {relative_path}")

    def get_changed_files(self, file_category: str,
                         file_list: List[Tuple[str, Path]]) -> List[Tuple[str, Path]]:
        """
        Get list of changed files from a file list.

        Args:
            file_category: Category key (e.g., "c_headers", "documents")
            file_list: List of (relative_path, absolute_path) tuples

        Returns:
            List of (relative_path, absolute_path) for changed files
        """
        changed = []

        for rel_path, abs_path in file_list:
            if self.is_file_changed(file_category, abs_path, rel_path):
                changed.append((rel_path, abs_path))

        return changed

    def get_cached_files(self, file_category: str) -> Dict[str, Dict]:
        """
        Get all cached files for a category.

        Args:
            file_category: Category key

        Returns:
            Dictionary of cached file info
        """
        return self.cache_data["file_hashes"].get(file_category, {})

    def clear_category(self, file_category: str):
        """
        Clear all cache entries for a category.

        Args:
            file_category: Category key to clear
        """
        if file_category in self.cache_data["file_hashes"]:
            count = len(self.cache_data["file_hashes"][file_category])
            self.cache_data["file_hashes"][file_category] = {}
            self._save_cache()
            logger.info(f"Cleared cache for category '{file_category}': {count} files")
        else:
            logger.warning(f"Category not found in cache: {file_category}")

    def clear_all(self):
        """Clear entire cache."""
        total_files = sum(
            len(files)
            for files in self.cache_data["file_hashes"].values()
        )

        self.cache_data = {
            "version": "1.0",
            "last_updated": datetime.now().isoformat(),
            "sdk_root": str(self.sdk_root),
            "file_hashes": {
                "excel_constraints": {},
                "documents": {},
                "c_headers": {},
                "c_sources": {}
            }
        }
        self._save_cache()
        logger.info(f"Cleared all build cache: {total_files} files")

    def get_cache_stats(self) -> Dict:
        """
        Get cache statistics.

        Returns:
            Dictionary with cache statistics
        """
        stats = {
            "version": self.cache_data["version"],
            "last_updated": self.cache_data["last_updated"],
            "sdk_root": self.cache_data["sdk_root"],
            "total_files": 0,
            "by_category": {}
        }

        for category, files in self.cache_data["file_hashes"].items():
            count = len(files)
            stats["total_files"] += count
            stats["by_category"][category] = count

        return stats

    def print_cache_stats(self):
        """Print cache statistics to log."""
        stats = self.get_cache_stats()

        logger.info("=" * 70)
        logger.info("Build Cache Statistics")
        logger.info("=" * 70)
        logger.info(f"Version: {stats['version']}")
        logger.info(f"Last Updated: {stats['last_updated']}")
        logger.info(f"Total Cached Files: {stats['total_files']}")
        logger.info("By Category:")

        for category, count in stats["by_category"].items():
            if count > 0:
                logger.info(f"  {category}: {count} files")

        logger.info("=" * 70)
