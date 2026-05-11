#!/usr/bin/env python3
"""
Enhanced Logging Utility for Build Scripts
============================================

Provides detailed logging with diagnostics for build scripts.
Features:
- Colored console output
- Detailed error reporting
- Progress tracking
- Diagnostic information for troubleshooting

Usage:
    from build_logger import BuildLogger
    logger = BuildLogger("script_name")
    logger.info("Processing file...")
    logger.error("Failed to parse", details={"file": "test.xlsx", "error": "format error"})

Author: B6x MCP Server Team
Version: 1.0.0
"""

import sys
import logging
from pathlib import Path
from typing import Dict, Any, Optional
from datetime import datetime


class BuildLogger:
    """Enhanced logger for build scripts with diagnostic capabilities."""

    # ANSI color codes
    COLORS = {
        'RESET': '\033[0m',
        'RED': '\033[91m',
        'GREEN': '\033[92m',
        'YELLOW': '\033[93m',
        'BLUE': '\033[94m',
        'MAGENTA': '\033[95m',
        'CYAN': '\033[96m',
        'BOLD': '\033[1m',
    }

    def __init__(self, script_name: str, log_dir: Optional[Path] = None):
        """Initialize build logger.

        Args:
            script_name: Name of the script for identification
            log_dir: Optional directory for log files (default: data/logs/)
        """
        self.script_name = script_name
        self.use_colors = self._supports_colors()

        # Setup Python logging
        self.logger = logging.getLogger(script_name)
        self.logger.setLevel(logging.DEBUG)

        # Console handler with colored output
        # Force UTF-8 encoding for Windows console to handle special characters
        if sys.platform == 'win32':
            try:
                sys.stdout.reconfigure(encoding='utf-8')
            except (AttributeError, OSError):
                pass  # Fall back to default encoding

        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setLevel(logging.INFO)
        console_formatter = logging.Formatter('%(message)s')
        console_handler.setFormatter(console_formatter)
        self.logger.addHandler(console_handler)

        # File handler for detailed logs
        if log_dir is None:
            # Default to data/logs/
            script_dir = Path(__file__).parent
            mcp_root = script_dir.parent
            log_dir = mcp_root / "data" / "logs"

        log_dir.mkdir(parents=True, exist_ok=True)
        log_file = log_dir / f"{script_name}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"

        file_handler = logging.FileHandler(log_file, encoding='utf-8')
        file_handler.setLevel(logging.DEBUG)
        file_formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        file_handler.setFormatter(file_formatter)
        self.logger.addHandler(file_handler)

        self.log_file = log_file

        # Statistics tracking
        self.stats = {
            'info': 0,
            'warning': 0,
            'error': 0,
            'success': 0,
            'files_processed': 0,
            'files_failed': 0
        }

    def _supports_colors(self) -> bool:
        """Check if terminal supports ANSI colors."""
        # Windows doesn't support ANSI colors by default
        if sys.platform == 'win32':
            try:
                import ctypes
                kernel32 = ctypes.windll.kernel32
                kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
                return True
            except:
                return False
        return True

    def _colorize(self, text: str, color: str) -> str:
        """Add ANSI color codes to text if supported."""
        if self.use_colors and color in self.COLORS:
            return f"{self.COLORS[color]}{text}{self.COLORS['RESET']}"
        return text

    def info(self, message: str):
        """Log info message."""
        self.logger.info(message)
        self.stats['info'] += 1

    def warning(self, message: str):
        """Log warning message."""
        colored_msg = self._colorize(f"[!] {message}", 'YELLOW')
        self.logger.warning(colored_msg)
        self.stats['warning'] += 1

    def error(self, message: str, details: Optional[Dict[str, Any]] = None):
        """Log error message with optional diagnostic details."""
        colored_msg = self._colorize(f"[ERROR] {message}", 'RED')
        self.logger.error(colored_msg)
        self.stats['error'] += 1

        if details:
            self.logger.debug(f"Error details: {details}")
            self.logger.debug(f"Detailed diagnostic information:")
            for key, value in details.items():
                self.logger.debug(f"  {key}: {value}")

    def success(self, message: str):
        """Log success message."""
        colored_msg = self._colorize(f"[OK] {message}", 'GREEN')
        self.logger.info(colored_msg)
        self.stats['success'] += 1

    def debug(self, message: str):
        """Log debug message."""
        self.logger.debug(message)

    def header(self, title: str, width: int = 70):
        """Log a section header."""
        separator = "=" * width
        self.logger.info("")
        self.logger.info(self._colorize(separator, 'BOLD'))
        self.logger.info(self._colorize(title, 'BOLD'))
        self.logger.info(self._colorize(separator, 'BOLD'))
        self.logger.info("")

    def sub_header(self, title: str):
        """Log a subsection header."""
        self.logger.info(self._colorize(title, 'BOLD'))

    def file_start(self, file_path: str):
        """Log start of file processing."""
        self.info(f"Processing: {file_path}")
        self.stats['files_processed'] += 1

    def file_success(self, file_path: str, details: Optional[str] = None):
        """Log successful file processing."""
        # Use ASCII-safe checkmark to avoid encoding issues on Windows
        msg = f"[OK] {file_path}"
        if details:
            msg += f" - {details}"
        self.success(msg)

    def file_error(self, file_path: str, error: Exception, context: Optional[Dict[str, Any]] = None):
        """Log file processing error with diagnostics."""
        details = {
            'file': str(file_path),
            'error_type': type(error).__name__,
            'error_message': str(error)
        }
        if context:
            details.update(context)

        self.error(f"Failed to process {file_path}: {error}", details=details)
        self.stats['files_failed'] += 1

    def file_skipped(self, file_path: str, reason: str):
        """Log skipped file."""
        msg = self._colorize(f"[SKIPPED] {file_path} - {reason}", 'CYAN')
        self.logger.info(msg)

    def summary(self):
        """Log build summary."""
        self.logger.info("")
        self.logger.info(self._colorize("=" * 70, 'BOLD'))
        self.logger.info(self._colorize("Build Summary", 'BOLD'))
        self.logger.info(self._colorize("=" * 70, 'BOLD'))

        total_files = self.stats['files_processed']
        failed_files = self.stats['files_failed']

        if total_files > 0:
            success_rate = ((total_files - failed_files) / total_files) * 100
            self.logger.info(f"Files Processed:   {total_files}")
            self.logger.info(f"Files Succeeded:   {total_files - failed_files}")
            self.logger.info(f"Files Failed:      {failed_files}")
            self.logger.info(f"Success Rate:      {success_rate:.1f}%")

        self.logger.info(f"Warnings:          {self.stats['warning']}")
        self.logger.info(f"Errors:            {self.stats['error']}")

        if self.stats['error'] > 0:
            self.logger.info("")
            self.logger.info(self._colorize("Build completed with errors", 'RED'))
        elif self.stats['warning'] > 0:
            self.logger.info("")
            self.logger.info(self._colorize("Build completed with warnings", 'YELLOW'))
        else:
            self.logger.info("")
            self.logger.info(self._colorize("Build completed successfully", 'GREEN'))

        self.logger.info("")
        self.logger.info(f"Log file: {self.log_file}")

    def log_exception(self, exc: Exception, context: Optional[Dict[str, Any]] = None):
        """Log exception with full traceback."""
        import traceback

        details = {
            'exception_type': type(exc).__name__,
            'exception_message': str(exc),
            'traceback': traceback.format_exc()
        }

        if context:
            details.update(context)

        self.error(f"Exception: {exc}", details=details)

    def get_stats(self) -> Dict[str, int]:
        """Get logging statistics."""
        return self.stats.copy()

    def print_diagnostics(self):
        """Print diagnostic information for troubleshooting."""
        self.logger.debug("")
        self.logger.debug("Diagnostic Information:")
        self.logger.debug(f"  Script: {self.script_name}")
        self.logger.debug(f"  Platform: {sys.platform}")
        self.logger.debug(f"  Python Version: {sys.version}")
        self.logger.debug(f"  Colors Supported: {self.use_colors}")
        self.logger.debug(f"  Log File: {self.log_file}")


def create_logger(script_name: str, log_dir: Optional[Path] = None) -> BuildLogger:
    """Factory function to create a BuildLogger.

    Args:
        script_name: Name of the script for identification
        log_dir: Optional directory for log files

    Returns:
        BuildLogger instance
    """
    return BuildLogger(script_name, log_dir)
