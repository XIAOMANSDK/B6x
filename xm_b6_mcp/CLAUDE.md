# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

B6x MCP Server is a Model Context Protocol server that provides AI assistants with deep knowledge of the B6x BLE SoC SDK. It enables AI to navigate code, query hardware constraints, and validate embedded configurations.

## Commands

### Running the Server

```bash
python run.py
```

### Building Indices (SDK Updates)

```bash
python scripts/build_all_indices.py
```

### Testing

```bash
pytest tests/                    # Run all tests
pytest tests/ -v                 # Verbose output
pytest tests/test_layer1.py      # Run specific test file
```

## Architecture

### Tool Architecture (v3.5)

The server exposes 4 core tools + 1 utility:

| Layer | Tool | Purpose |
|-------|------|---------|
| 1 - Discovery | `search_sdk()` | Search APIs, docs, registers, examples |
| 2 - Detail | `inspect_node()` | Get detailed info about a specific node |
| 3 - Validation | `validate_config()` | Validate hardware configurations |
| Clang | `clang_analyze()` | Real-time semantic C code analysis |
| Utility | `get_server_info()` | Server statistics and version |

### Four-Dimensional Knowledge Graph

| Domain | Contents | Source |
|--------|----------|--------|
| Hardware | 368 registers, 65 pins, memory map | `core/B6x.SVD`, `doc/SW_Spec` |
| Drivers | APIs, structs, macros, dependencies | `drivers/api`, `drivers/src` |
| BLE | APIs, profiles, error codes, mesh | `ble/api`, `ble/prf`, `ble/lib` |
| Applications | Examples, call chains, configs | `examples`, `projects` |

### Key Source Files

```
src/
├── main.py                    # MCP server entry point, tool registration
├── layer1_discovery.py        # search_sdk() implementation
├── layer2_detail.py           # inspect_node() implementation
├── layer3_validation.py       # validate_config() implementation
├── core/
│   ├── knowledge_graph.py     # Knowledge graph loader and query
│   ├── whoosh_indexer.py      # Full-text search indexing
│   ├── tree_sitter_parser.py  # C code parsing
│   ├── svd_parser.py          # SVD register parsing
│   └── excel_parser.py        # Hardware constraint extraction
└── common/
    └── node_id.py             # Node ID format standardization
```

### Configuration Files

| File | Purpose |
|------|---------|
| `config/api_register_mapping.yaml` | Manual API-to-register mappings for conflict detection |
| `config/dependency_overrides.yaml` | API dependency relationships and call sequences |

### Data Output Structure

```
data/
├── knowledge_graph.json       # Unified knowledge graph (4MB)
├── whoosh_index/              # Full-text search index
├── constraints/               # Hardware constraints from Excel
│   ├── io_map.json           # Pin multiplexing
│   ├── power_consumption.json # Power data
│   └── memory_boundaries.json # Flash/SRAM limits
├── domain/                    # Domain-specific data
│   ├── hardware/             # Registers, pins, memory
│   ├── drivers/              # APIs, structs, macros
│   └── ble/                  # BLE APIs, profiles
└── relations/                 # Cross-domain relationships
```

## Node ID Format

Standard format: `type:identifier`

- API: `api:B6x_UART_Init`
- Register: `reg:UART1_CTRL`
- Document: `docs:uart_guide`

Legacy format (auto-converted): `B6x_UART_Init` → `api:B6x_UART_Init`

## Validation Chain

When `validate_config()` is called, it checks:

1. Pin conflicts (error if fails)
2. Clock tree validity (error if fails)
3. DMA allocation (error if fails)
4. Interrupt priority (warning)
5. Memory usage (warning)
6. API dependencies using `dependency_overrides.yaml`
7. Register conflicts using `api_register_mapping.yaml`

## SDK Path Configuration

Default SDK path: `../sdk6` (relative to xm_b6_mcp)

Custom path via environment:
```bash
export B6X_SDK_PATH="/path/to/sdk6"
```

## Related Documentation

- [README.md](README.md) - Project overview and usage examples
- [docs/architecture.md](docs/architecture.md) - Detailed system architecture
- [docs/build-guide.md](docs/build-guide.md) - Build process details
- [docs/config_directory_guide.md](docs/config_directory_guide.md) - Configuration file reference
