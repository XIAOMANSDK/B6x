"""
API to Error Code Mapper
========================

Map BLE APIs to potential error codes they can return.

Extracted from:
- API documentation (@return tags)
- Error code names (semantic matching)
- Common patterns

Author: B6x MCP Server Team
Version: 0.1.0
"""

import re
import logging
from typing import Dict, List, Set, Optional, Any
from dataclasses import dataclass, asdict
from collections import defaultdict

from .error_code_parser import BLEErrorCodeParser, ErrorCode

logger = logging.getLogger(__name__)


@dataclass
class APIErrorMapping:
    """
    Mapping between an API and possible error codes.

    Attributes:
        api_name: Name of the API function
        error_codes: List of error codes that might be returned
        error_categories: Categories of errors (e.g., ['GATT', 'GAP'])
        common_errors: Most frequently returned errors
        error_conditions: Map of error -> condition that triggers it
    """
    api_name: str
    error_codes: List[str]  # Error code names
    error_categories: List[str]
    common_errors: List[str]
    error_conditions: Dict[str, str]  # error_code -> condition

    def __post_init__(self):
        if self.error_codes is None:
            self.error_codes = []
        if self.error_categories is None:
            self.error_categories = []
        if self.common_errors is None:
            self.common_errors = []
        if self.error_conditions is None:
            self.error_conditions = {}


@dataclass
class ErrorOccurrence:
    """Record of an error code occurrence in documentation."""
    error_code: str
    context: str  # Where this was found
    probability: str  # 'high', 'medium', 'low'


class APIErrorMapper:
    """
    Build mappings between APIs and error codes.

    Methods:
    - Parse API documentation for @return tags
    - Semantic matching based on API/error names
    - Pattern-based inference
    """

    def __init__(self, error_parser: Optional[BLEErrorCodeParser] = None):
        """
        Initialize the API error mapper.

        Args:
            error_parser: BLEErrorCodeParser instance (optional)
        """
        self.error_parser = error_parser
        self._api_error_map: Dict[str, APIErrorMapping] = {}
        self._error_api_map: Dict[str, Set[str]] = defaultdict(set)

        # Common error patterns
        self._error_patterns = {
            'GATT': [
                (r'GATT.*Write', ['ERR_INVALID_HANDLE', 'ERR_REJECT', 'ERR_INVALID_OFFSET']),
                (r'GATT.*Read', ['ERR_INVALID_HANDLE', 'ERR_NOT_ALLOWED']),
                (r'GATT.*Notify', ['ERR_INVALID_HANDLE', 'ERR_NOT_ALLOWED']),
                (r'GATT.*Indicate', ['ERR_INVALID_HANDLE', 'ERR_NOT_ALLOWED']),
            ],
            'GAP': [
                (r'GAP.*Disconnect', ['ERR_REMOTE_USER_TERM_CONN', 'ERR_AUTH_FAILURE']),
                (r'GAP.*Connect', ['ERR_DIRECTED_ADV_TIMEOUT', '_ERR_CONN_ACCEPT_TIMEOUT']),
                (r'GAP.*Discover', ['_ERR_BUSY', 'ERR_TIMEOUT']),
            ],
            'L2CAP': [
                (r'L2CAP.*Connect', ['ERR_NO_RESOURCE', 'ERR_INVALID_PARAMETER']),
                (r'L2CAP.*Disconnect', ['_ERR_NOT_CONNECTED']),
            ],
        }

    def build_mappings_from_documentation(
        self,
        api_docs: List[Any],
        error_parser: Optional[BLEErrorCodeParser] = None
    ) -> Dict[str, APIErrorMapping]:
        """
        Build API→error mappings from API documentation.

        Args:
            api_docs: List of API documentation objects (FunctionDeclaration)
            error_parser: BLEErrorCodeParser instance

        Returns:
            Dictionary mapping API names to APIErrorMapping objects
        """
        if error_parser:
            self.error_parser = error_parser

        # Get all error codes
        all_errors = []
        if self.error_parser:
            all_errors = self.error_parser._error_codes

        for api in api_docs:
            mapping = self._extract_errors_from_api_doc(api, all_errors)
            if mapping.error_codes:
                self._api_error_map[api.name] = mapping
                for error_code in mapping.error_codes:
                    self._error_api_map[error_code].add(api.name)

        logger.info(f"Built {len(self._api_error_map)} API→error mappings")
        return self._api_error_map

    def _extract_errors_from_api_doc(self, api, all_errors: List[ErrorCode]) -> APIErrorMapping:
        """Extract error codes from API documentation."""
        error_codes = set()
        error_conditions = {}
        common_errors = []

        # Extract from @return tags
        return_errors = self._extract_return_errors(api.detailed_description)
        for error_name in return_errors:
            error_codes.add(error_name)

        # Semantic matching based on API name
        semantic_errors = self._match_errors_semantically(api.name, all_errors)
        for error_name in semantic_errors:
            error_codes.add(error_name)

        # Pattern-based inference
        pattern_errors = self._infer_from_patterns(api.name)
        for error_name in pattern_errors:
            error_codes.add(error_name)

        # Determine common errors
        common_errors = self._determine_common_errors(api.name, list(error_codes))

        # Determine error categories
        categories = self._get_error_categories(list(error_codes), all_errors)

        # Build error conditions
        for error_code in error_codes:
            error_conditions[error_code] = self._infer_error_condition(api.name, error_code)

        return APIErrorMapping(
            api_name=api.name,
            error_codes=list(error_codes),
            error_categories=categories,
            common_errors=common_errors,
            error_conditions=error_conditions
        )

    def _extract_return_errors(self, doc_string: str) -> Set[str]:
        """Extract error codes from @return tags."""
        errors = set()

        # Pattern for @return ERR_XXXX
        pattern = r'@return\s+(ERR_\w+)'
        matches = re.findall(pattern, doc_string)
        errors.update(matches)

        # Pattern for "Returns: ERR_XXXX"
        pattern = r'(?:Returns|return)\s*:\s*(ERR_\w+)'
        matches = re.findall(pattern, doc_string, re.IGNORECASE)
        errors.update(matches)

        return errors

    def _match_errors_semantically(self, api_name: str, all_errors: List[ErrorCode]) -> Set[str]:
        """Match error codes based on semantic similarity."""
        errors = set()

        api_lower = api_name.lower()
        api_words = set(re.findall(r'\w+', api_lower))

        for error_code in all_errors:
            error_lower = error_code.name.lower()
            error_words = set(re.findall(r'\w+', error_lower))

            # Check for word overlap
            overlap = api_words & error_words

            # If API and error share significant words, they might be related
            if len(overlap) >= 2 or (len(overlap) >= 1 and any(w in ['connect', 'disconnect', 'read', 'write', 'discover'] for w in overlap)):
                errors.add(error_code.name)

        return errors

    def _infer_from_patterns(self, api_name: str) -> Set[str]:
        """Infer errors from API name patterns."""
        errors = set()

        for category, patterns in self._error_patterns.items():
            for pattern, pattern_errors in patterns:
                if re.search(pattern, api_name, re.IGNORECASE):
                    errors.update(pattern_errors)

        return errors

    def _determine_common_errors(self, api_name: str, error_codes: List[str]) -> List[str]:
        """Determine which errors are most commonly returned."""
        common = []

        # Common errors that apply to many APIs
        universal_errors = ['ERR_BUSY', 'ERR_INVALID_STATE', 'ERR_NO_RESOURCE']

        for error in universal_errors:
            if error in error_codes:
                common.append(error)

        # API-specific common errors
        api_lower = api_name.lower()
        if 'write' in api_lower:
            common.extend([e for e in error_codes if 'INVALID_HANDLE' in e or 'REJECT' in e])
        elif 'read' in api_lower:
            common.extend([e for e in error_codes if 'INVALID_HANDLE' in e or 'NOT_ALLOWED' in e])
        elif 'connect' in api_lower:
            common.extend([e for e in error_codes if 'TIMEOUT' in e or 'NO_RESOURCE' in e])

        return list(set(common))

    def _get_error_categories(self, error_codes: List[str], all_errors: List[ErrorCode]) -> List[str]:
        """Get categories for a list of error codes."""
        categories = set()

        error_map = {e.name: e for e in all_errors}

        for error_code in error_codes:
            if error_code in error_map:
                categories.add(error_map[error_code].category)

        return sorted(categories)

    def _infer_error_condition(self, api_name: str, error_code: str) -> str:
        """Infer the condition that triggers an error."""
        conditions = {
            'ERR_INVALID_HANDLE': 'Handle is invalid or has been invalidated',
            'ERR_NOT_ALLOWED': 'Operation not allowed in current state',
            'ERR_BUSY': 'Resource is currently busy',
            'ERR_NO_RESOURCE': 'Insufficient resources available',
            'ERR_TIMEOUT': 'Operation timed out',
            'ERR_INVALID_STATE': 'Device or service in invalid state',
            'ERR_REJECT': 'Request rejected by remote device',
            'ERR_AUTH_FAILURE': 'Authentication or pairing failed',
        }

        # Specific conditions based on API name
        api_lower = api_name.lower()
        if 'connect' in api_lower:
            if 'TIMEOUT' in error_code:
                return 'Connection attempt timed out'
            elif 'NO_RESOURCE' in error_code:
                return 'No available connection slots'
        elif 'write' in api_lower:
            if 'INVALID_HANDLE' in error_code:
                return 'Characteristic or service handle is invalid'
            elif 'REJECT' in error_code:
                return 'Write rejected by remote device (not authorized)'

        return conditions.get(error_code, 'Unknown condition')

    def build_semantic_mappings(self):
        """Build mappings based on semantic analysis of API and error names."""
        if not self.error_parser:
            logger.warning("No error parser available for semantic mapping")
            return

        all_errors = self.error_parser._error_codes

        # Group errors by category
        category_errors = defaultdict(list)
        for error in all_errors:
            category_errors[error.category].append(error.name)

        # Build mappings for each API based on its category
        for api_name, mapping in self._api_error_map.items():
            for category in mapping.error_categories:
                # Add all errors from the category
                for error_name in category_errors.get(category, []):
                    if error_name not in mapping.error_codes:
                        mapping.error_codes.append(error_name)

    def get_possible_errors(self, api_name: str) -> List[Dict[str, Any]]:
        """
        Get possible error codes for an API.

        Args:
            api_name: API function name

        Returns:
            List of error information dictionaries
        """
        if api_name not in self._api_error_map:
            return []

        mapping = self._api_error_map[api_name]

        results = []
        for error_code in mapping.error_codes:
            results.append({
                'error_code': error_code,
                'probability': 'high' if error_code in mapping.common_errors else 'medium',
                'condition': mapping.error_conditions.get(error_code, ''),
                'category': self._get_error_category(error_code)
            })

        return sorted(results, key=lambda x: x['probability'], reverse=True)

    def _get_error_category(self, error_code: str) -> str:
        """Get category for an error code."""
        if self.error_parser:
            error = self.error_parser.get_error_by_name(error_code)
            if error:
                return error.category
        return 'UNKNOWN'

    def get_apis_for_error(self, error_code: str) -> List[str]:
        """
        Get APIs that might return a specific error code.

        Args:
            error_code: Error code name

        Returns:
            List of API names
        """
        return sorted(self._error_api_map.get(error_code, set()))

    def export_to_json(self, output_path: str):
        """
        Export API→error mappings to JSON.

        Args:
            output_path: Path to output JSON file
        """
        import json
        from pathlib import Path

        data = {
            "version": "1.0.0",
            "total_mappings": len(self._api_error_map),
            "api_to_errors": {k: asdict(v) for k, v in self._api_error_map.items()},
            "error_to_apis": {k: list(v) for k, v in self._error_api_map.items()}
        }

        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)

        logger.info(f"API→error mappings exported to: {output_path}")


def map_api_errors(api_docs: List[Any], error_parser: BLEErrorCodeParser) -> APIErrorMapper:
    """Convenience function to build API error mappings."""
    mapper = APIErrorMapper(error_parser)
    mapper.build_mappings_from_documentation(api_docs)
    mapper.build_semantic_mappings()
    return mapper


if __name__ == "__main__":
    import sys

    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s'
    )

    # Test with mock data
    print("API Error Mapper - Test Mode")
    print("This module is typically used via the build_index.py script")
