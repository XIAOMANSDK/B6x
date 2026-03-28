"""
Markdown Table Parser - Stage 2 of Excel Parsing Pipeline
==========================================================

Parse structured data from Markdown tables created by Excel conversion.

This module provides:
- Flash Map table parsing
- SRAM allocation sparse table parsing
- BLE compatibility list parsing
- Chinese column name handling

Usage:
    parser = MarkdownTableParser()
    regions = parser.parse_flash_map_md(md_path)
    regions, boundaries = parser.parse_sram_allocation_md(md_path)

Author: B6x MCP Server Team
Version: 1.0.0
"""

import re
import logging
from pathlib import Path
from typing import List, Dict, Any, Tuple, Optional

logger = logging.getLogger(__name__)


class MarkdownTableParser:
    """
    Parse structured data from Markdown tables.

    Handles:
    - Standard Markdown tables (| col1 | col2 |)
    - Sparse tables (empty cells, merged cells)
    - Chinese column names
    - Multiple sheets in one file
    """

    def __init__(self):
        """Initialize the parser."""
        # Table row pattern: | cell1 | cell2 | cell3 |
        self.table_row_pattern = re.compile(r'^\|(.+)\|$', re.MULTILINE)
        self.heading_pattern = re.compile(r'^#{1,6}\s+(.+)$', re.MULTILINE)

    # ========================================================================
    # Flash Map Parser
    # ========================================================================

    def parse_flash_map_md(self, md_path: Path) -> List[Dict[str, Any]]:
        """
        Parse Flash Map from Markdown tables.

        Expected format:
        ## Sheet: Flash 256KB
        | Size: 256KB | ... |
        | Range: 0x08000000 - 0x0803FFFF | ... |

        Args:
            md_path: Path to Markdown file

        Returns:
            List of memory region dictionaries
        """
        md_path = Path(md_path)
        if not md_path.exists():
            logger.error(f"Markdown file not found: {md_path}")
            return []

        try:
            with open(md_path, 'r', encoding='utf-8') as f:
                md_content = f.read()

        except Exception as e:
            logger.error(f"Failed to read {md_path}: {e}")
            return []

        regions = []

        # Split by sheet headings
        sheets = self._split_by_sheets(md_content)

        for sheet_name, sheet_content in sheets:
            # Check if this is a Flash sheet
            if 'flash' in sheet_name.lower() or 'data' in sheet_name.lower():
                sheet_regions = self._parse_flash_sheet(sheet_content, sheet_name)
                regions.extend(sheet_regions)

        logger.info(f"Parsed {len(regions)} Flash regions from Markdown")
        return regions

    def _parse_flash_sheet(self, sheet_content: str, sheet_name: str) -> List[Dict[str, Any]]:
        """Parse a single Flash sheet content."""
        regions = []
        lines = sheet_content.split('\n')

        # Scan through lines to find key patterns
        main_flash_size_kb = None
        main_flash_range = None
        data_flash_base = None
        data_flash_size_kb = None
        code_flash_size_kb = None

        for i, line in enumerate(lines):
            cells = self._extract_table_cells(line)
            if not cells:
                continue

            # Check each cell for patterns
            for j, cell in enumerate(cells):
                cell = str(cell).strip()
                if not cell or cell == 'NaN':
                    continue

                # Pattern: "Size: 2Mb(256KB)" - main Flash size
                if 'size:' in cell.lower() and 'mb(' in cell.lower():
                    size_match = re.search(r'Size:\s*\d+Mb\((\d+)KB\)', cell, re.IGNORECASE)
                    if size_match and not main_flash_size_kb:
                        main_flash_size_kb = int(size_match.group(1))

                # Pattern: "Range: 000000H - 03FFFFH" - main Flash range
                elif 'range:' in cell.lower():
                    # Remove the "Range:" prefix and clean up the string
                    range_str = cell.lower().replace('range:', '').strip()
                    # Remove backslashes (Markdown escape)
                    range_str = range_str.replace('\\', '')

                    # Split by common delimiters
                    parts = re.split(r'[\s\-–~]+', range_str)
                    # Filter out empty strings and keep hex numbers
                    hex_parts = [p.rstrip('h') for p in parts if p.strip() and re.match(r'[0-9A-Fa-f]+H?', p.strip())]

                    if len(hex_parts) >= 2:
                        try:
                            start_addr = int(hex_parts[0], 16)
                            end_addr = int(hex_parts[1], 16)
                            if not main_flash_range:
                                main_flash_range = (start_addr, end_addr)
                        except ValueError:
                            pass

                # Check for DATA Region marker
                if 'data' in cell.lower() and 'region' in cell.lower():
                    # Look for Size:XXKB in nearby cells
                    for k in range(max(0, j-2), min(len(cells), j+5)):
                        nearby = str(cells[k]).strip()
                        size_match = re.search(r'Size:(\d+)KB', nearby, re.IGNORECASE)
                        if size_match:
                            data_flash_size_kb = int(size_match.group(1))
                            break

                    # Look for base address in first column of this row
                    first_col = str(cells[0]).strip()
                    if first_col.startswith('0x'):
                        try:
                            data_flash_base = int(first_col, 16)
                        except ValueError:
                            pass

                # Check for CODE Region marker
                if 'code' in cell.lower() and 'region' in cell.lower():
                    # Look for Size:XXKB in nearby cells
                    for k in range(max(0, j-2), min(len(cells), j+5)):
                        nearby = str(cells[k]).strip()
                        size_match = re.search(r'Size:(\d+)KB', nearby, re.IGNORECASE)
                        if size_match:
                            code_flash_size_kb = int(size_match.group(1))
                            break

        # Create main Flash region - use MemoryRegion dataclass format
        if main_flash_size_kb and main_flash_range:
            start_addr, end_addr = main_flash_range

            # Determine region name from sheet name
            if '256KB' in sheet_name or '256kb' in sheet_name.lower():
                region_name = 'FLASH'
            elif '128KB' in sheet_name or '128kb' in sheet_name.lower():
                region_name = 'FLASH_128KB'
            else:
                region_name = sheet_name.replace(' ', '_')

            region = {
                'region_name': region_name,
                'base_address': f"0x{start_addr:08X}",
                'size': main_flash_size_kb * 1024,
                'size_kb': main_flash_size_kb,
                'is_xip': True,
                'access_type': 'READ_ONLY',
                'description': f"{sheet_name} Code Flash (XIP supported)"
            }
            regions.append(region)

        # Create DATA Flash region - use MemoryRegion dataclass format
        if data_flash_base and data_flash_size_kb:
            data_region = {
                'region_name': 'DATA_FLASH',
                'base_address': f"0x{data_flash_base:08X}",
                'size': data_flash_size_kb * 1024,
                'size_kb': data_flash_size_kb,
                'is_xip': False,
                'access_type': 'READ_WRITE',
                'description': 'Data Flash for configuration and OTA'
            }
            regions.append(data_region)

        # Fallback: create default regions if no data parsed
        if not regions:
            default = self._create_default_flash_region(sheet_name)
            # Convert default to dataclass format
            regions.append({
                'region_name': default['name'],
                'base_address': default['base_address'],
                'size': default['size_bytes'],
                'size_kb': default['size_kb'],
                'is_xip': default['is_xip'],
                'access_type': default['access_type'],
                'description': default['description']
            })

        return regions

    def _create_default_flash_region(self, sheet_name: str) -> Dict[str, Any]:
        """Create default Flash region from sheet name."""
        if '256KB' in sheet_name or '256kb' in sheet_name.lower():
            return {
                'name': 'FLASH',
                'base_address': '0x08000000',
                'end_address': '0x0803FFFF',
                'size_bytes': 262144,
                'size_kb': 256,
                'size_human': '256KB',
                'is_xip': True,
                'access_type': 'READ_ONLY',
                'description': 'Code Flash (XIP supported)'
            }
        elif '128KB' in sheet_name or '128kb' in sheet_name.lower():
            return {
                'name': 'FLASH_128KB',
                'base_address': '0x08000000',
                'end_address': '0x0001FFFF',
                'size_bytes': 131072,
                'size_kb': 128,
                'size_human': '128KB',
                'is_xip': True,
                'access_type': 'READ_ONLY',
                'description': 'Code Flash 128KB variant'
            }
        elif 'data' in sheet_name.lower():
            return {
                'name': 'DATA_FLASH',
                'base_address': '0x18000000',
                'end_address': '0x18003FFF',
                'size_bytes': 16384,
                'size_kb': 16,
                'size_human': '16KB',
                'is_xip': False,
                'access_type': 'READ_WRITE',
                'description': 'Data Flash for configuration and OTA'
            }
        else:
            # Unknown sheet, return generic Flash region
            return {
                'name': sheet_name.replace(' ', '_'),
                'base_address': '0x08000000',
                'end_address': '0x08000000',
                'size_bytes': 0,
                'size_kb': 0,
                'size_human': 'Unknown',
                'is_xip': True,
                'access_type': 'READ_ONLY',
                'description': f'{sheet_name} (size unknown)'
            }

    # ========================================================================
    # SRAM Allocation Parser
    # ========================================================================

    def parse_sram_allocation_md(
        self,
        md_path: Path
    ) -> Tuple[List[Dict[str, Any]], List[Dict[str, Any]]]:
        """
        Parse SRAM allocation from Markdown sparse tables.

        Expected format:
        ## Sheet: SRAM空间分配
        | SRAM | Address | 主从共3个连接 | ... |
        |------|---------|---------------|-----|
        | SRAM1| 0x20000000 | ... | ... |

        Args:
            md_path: Path to Markdown file

        Returns:
            Tuple of (SRAM regions list, memory boundaries list)
        """
        md_path = Path(md_path)
        if not md_path.exists():
            logger.error(f"Markdown file not found: {md_path}")
            return [], []

        try:
            with open(md_path, 'r', encoding='utf-8') as f:
                md_content = f.read()

        except Exception as e:
            logger.error(f"Failed to read {md_path}: {e}")
            return [], []

        regions = []
        boundaries = []

        # Find SRAM sheet
        sheets = self._split_by_sheets(md_content)

        for sheet_name, sheet_content in sheets:
            if 'sram' in sheet_name.lower() or '空间分配' in sheet_name:
                regions, boundaries = self._parse_sram_sheet(sheet_content)
                break

        logger.info(f"Parsed {len(regions)} SRAM regions and {len(boundaries)} boundaries")
        return regions, boundaries

    def _parse_sram_sheet(
        self,
        sheet_content: str
    ) -> Tuple[List[Dict[str, Any]], List[Dict[str, Any]]]:
        """Parse SRAM sheet content."""
        regions = []
        boundaries = []

        lines = sheet_content.split('\n')

        # Find header row
        header_row = None
        header_idx = None

        for idx, line in enumerate(lines):
            cells = self._extract_table_cells(line)
            if cells and any('连接' in str(c) for c in cells):
                header_row = cells
                header_idx = idx
                break

        if not header_row:
            logger.warning("No SRAM header row found")
            return [], []

        # Extract library types from header
        library_types = []
        for i, cell in enumerate(header_row):
            if '连接' in str(cell):
                # Map Chinese to library names
                cell_str = str(cell)
                if '3个' in cell_str or '主从' in cell_str:
                    library_types.append(('ble6', i))
                elif '1个' in cell_str and ('lite' in cell_str.lower() or '单从' in cell_str):
                    library_types.append(('ble6_lite', i))
                elif '6个' in cell_str:
                    library_types.append(('ble6_8act_6con', i))

        logger.debug(f"Found library types: {[lt[0] for lt in library_types]}")

        # Parse data rows
        current_sram = None
        current_address = None

        for line in lines[header_idx + 1:]:
            cells = self._extract_table_cells(line)
            if not cells:
                continue

            first_cell = cells[0].strip() if cells else ""

            # Check for SRAM region marker
            if first_cell.startswith('SRAM') or first_cell.startswith('SRAM:'):
                current_sram = first_cell.replace('SRAM:', '').strip()
                continue

            # Check for address
            if first_cell.startswith('0x') or first_cell.startswith('0X'):
                try:
                    current_address = int(first_cell, 16)
                except ValueError:
                    pass

            # Extract region data for each library
            for lib_name, col_idx in library_types:
                if col_idx < len(cells) and cells[col_idx]:
                    cell_value = str(cells[col_idx]).strip()

                    # Extract size (hex format)
                    size_match = re.search(r'0x([0-9A-Fa-f]+)', cell_value)
                    if size_match and current_address:
                        size = int(size_match.group(1), 16)

                        # Extract region name
                        region_name = cell_value.split('\n')[0].strip()
                        if not region_name or region_name.startswith('0x'):
                            region_name = f"{current_sram}_REGION"

                        # Create region - use SRAMRegion dataclass format
                        region = {
                            'region_name': region_name,
                            'start_address': f"0x{current_address:X}",
                            'end_address': f"0x{current_address + size:X}",
                            'size_bytes': size,
                            'size_kb': size // 1024,
                            'description': f"{region_name} ({lib_name})",
                            'library_type': lib_name
                        }
                        regions.append(region)

                        # Create boundary
                        boundary = {
                            'region_name': region_name,
                            'start_address': f"0x{current_address:X}",
                            'end_address': f"0x{current_address + size:X}",
                            'size_bytes': size,
                            'size_kb': size // 1024,
                            'size_human': f"{size // 1024}KB" if size >= 1024 else f"{size}B",
                            'boundary_type': 'allocated',
                            'library_type': lib_name,
                            'description': f"{region_name} boundary for {lib_name}",
                            'access_restrictions': ['read_write'],
                            'alignment_requirement': 4,
                            'sram_region': current_sram
                        }
                        boundaries.append(boundary)

        return regions, boundaries

    # ========================================================================
    # BLE Compatibility Parser
    # ========================================================================

    def parse_ble_compatibility_md(self, md_path: Path) -> List[Dict[str, Any]]:
        """
        Parse BLE compatibility list from Markdown tables.

        Expected format:
        ## Sheet: BLE Compatibility
        | Feature | Supported | Notes |
        |---------|-----------|-------|
        | BLE 5.0 | Yes | ... |

        Args:
            md_path: Path to Markdown file

        Returns:
            List of compatibility feature entries
        """
        md_path = Path(md_path)
        if not md_path.exists():
            logger.error(f"Markdown file not found: {md_path}")
            return []

        try:
            with open(md_path, 'r', encoding='utf-8') as f:
                md_content = f.read()

        except Exception as e:
            logger.error(f"Failed to read {md_path}: {e}")
            return []

        features = []

        # Split by sheets
        sheets = self._split_by_sheets(md_content)

        for sheet_name, sheet_content in sheets:
            if 'compatibility' in sheet_name.lower() or '兼容性' in sheet_name:
                sheet_features = self._parse_compatibility_sheet(sheet_content)
                features.extend(sheet_features)

        logger.info(f"Parsed {len(features)} BLE compatibility features")
        return features

    def _parse_compatibility_sheet(self, sheet_content: str) -> List[Dict[str, Any]]:
        """Parse compatibility sheet content."""
        features = []

        lines = sheet_content.split('\n')

        # Find header row
        header_row = None
        header_idx = None

        for idx, line in enumerate(lines):
            cells = self._extract_table_cells(line)
            if cells:
                # Look for feature/function column
                if any('feature' in str(c).lower() or '功能' in str(c) or '特性' in str(c)
                       for c in cells):
                    header_row = [str(c).lower() for c in cells]
                    header_idx = idx
                    break

        if not header_row:
            logger.warning("No compatibility header row found")
            return []

        # Map column indices
        col_map = {}
        for i, col_name in enumerate(header_row):
            if 'feature' in col_name or '功能' in col_name or '特性' in col_name:
                col_map['feature'] = i
            elif 'support' in col_name or '支持' in col_name:
                col_map['support'] = i
            elif 'library' in col_name or '库' in col_name:
                col_map['library'] = i
            elif 'note' in col_name or '备注' in col_name or '说明' in col_name:
                col_map['notes'] = i

        # Parse data rows
        for line in lines[header_idx + 1:]:
            cells = self._extract_table_cells(line)
            if not cells or len(cells) < 2:
                continue

            # Extract feature name
            feature_name = str(cells[col_map.get('feature', 0)]).strip()
            if not feature_name or feature_name.lower() in ['', 'nan', '-']:
                continue

            # Determine supported libraries
            supported_libs = []
            support_cell = str(cells[col_map.get('support', 1)]).lower() if col_map.get('support') < len(cells) else ""
            library_cell = str(cells[col_map.get('library', 1)]) if col_map.get('library') < len(cells) else ""

            if 'yes' in support_cell or '是' in support_cell or 'all' in support_cell:
                supported_libs = ['ble6_lite', 'ble6', 'ble6_8act_6con']
            elif 'lite' in support_cell:
                supported_libs = ['ble6_lite']
            elif 'standard' in support_cell or 'ble6' in library_cell:
                supported_libs = ['ble6', 'ble6_8act_6con']
            elif 'full' in support_cell:
                supported_libs = ['ble6_8act_6con']

            # Extract notes
            notes = str(cells[col_map.get('notes', len(cells) - 1)]) if col_map.get('notes') < len(cells) else ""

            feature = {
                'feature_name': feature_name,
                'supported_libs': supported_libs,
                'min_sdk_version': '',
                'max_sdk_version': '',
                'notes': notes,
                'constraints': []
            }

            features.append(feature)

        return features

    # ========================================================================
    # Utility Methods
    # ========================================================================

    def _split_by_sheets(self, md_content: str) -> List[Tuple[str, str]]:
        """
        Split Markdown content by sheet headings.

        Args:
            md_content: Full Markdown content

        Returns:
            List of (sheet_name, sheet_content) tuples
        """
        sheets = []
        lines = md_content.split('\n')

        current_sheet = "Default"
        current_content = []

        for line in lines:
            # Check for sheet heading (## followed by text, with or without "Sheet:")
            if line.strip().startswith('##'):
                # Save previous sheet if we have content
                if current_content:
                    sheets.append((current_sheet, '\n'.join(current_content)))

                # Extract sheet name
                heading_text = line.strip()[2:].strip()  # Remove ##
                if ':' in heading_text:
                    sheet_name = heading_text.split(':', 1)[1].strip()
                else:
                    sheet_name = heading_text
                current_sheet = sheet_name
                current_content = []
            else:
                current_content.append(line)

        # Save last sheet
        if current_content:
            sheets.append((current_sheet, '\n'.join(current_content)))

        return sheets

    def _extract_table_cells(self, line: str) -> List[str]:
        """
        Extract table cells from a Markdown table row.

        Args:
            line: Markdown table row (e.g., "| cell1 | cell2 |")

        Returns:
            List of cell values (stripped)
        """
        match = self.table_row_pattern.match(line.strip())
        if not match:
            return []

        # Split by pipe and clean up
        cells = match.group(1).split('|')
        cells = [cell.strip() for cell in cells]

        # Filter out separator rows (---)
        if cells and all(re.match(r'^-+$', str(c)) for c in cells if c):
            return []

        return cells


# ============================================================================
# Convenience Functions
# ============================================================================

def parse_flash_map(md_path: Path) -> List[Dict[str, Any]]:
    """Convenience function to parse Flash Map from Markdown."""
    parser = MarkdownTableParser()
    return parser.parse_flash_map_md(md_path)


def parse_sram_allocation(md_path: Path) -> Tuple[List[Dict[str, Any]], List[Dict[str, Any]]]:
    """Convenience function to parse SRAM allocation from Markdown."""
    parser = MarkdownTableParser()
    return parser.parse_sram_allocation_md(md_path)


def parse_ble_compatibility(md_path: Path) -> List[Dict[str, Any]]:
    """Convenience function to parse BLE compatibility from Markdown."""
    parser = MarkdownTableParser()
    return parser.parse_ble_compatibility_md(md_path)
