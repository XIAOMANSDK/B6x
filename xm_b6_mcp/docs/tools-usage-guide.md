# MCP Server Tools - Usage Guide

This document describes the maintenance and development tools available in the `scripts/` directory.

## Table of Contents

1. [Build Scripts](#build-scripts)
2. [Deployment Tools](#deployment-tools)
3. [Utility Scripts](#utility-scripts)
4. [clang_analyze MCP Tool](#clang_analyze-mcp-tool)
5. [When to Use These Tools](#when-to-use-these-tools)

---

## Build Scripts

### build_all_indices.py

**Purpose:** Build all search indices (main entry point for index building).

**Location:** `scripts/build_all_indices.py`

**Usage:**
```bash
cd /path/to/xm_b6_mcp
python scripts/build_all_indices.py              # Full build
python scripts/build_all_indices.py --incremental # Incremental build
```

**What It Does:**
- Orchestrates all build scripts
- Builds Whoosh full-text index
- Parses Excel hardware constraints
- Generates knowledge graph
- Supports incremental builds for faster updates

---

### build_index_v2.py

**Purpose:** Build the four-dimensional knowledge graph.

**Location:** `scripts/build_index_v2.py`

**Usage:**
```bash
python scripts/build_index_v2.py                  # Full build
python scripts/build_index_v2.py --domain hardware # Single domain
python scripts/build_index_v2.py --stats          # Show stats
python scripts/build_index_v2.py --test           # Test mode
```

**Domain Build Order:**
1. Hardware (registers, pin mux, memory map, interrupts)
2. Drivers (APIs, structs, dependencies)
3. BLE (APIs, profiles, error codes)
4. Apps (examples, call chains, configs)

**Output:**
- `data/knowledge_graph.json` - Unified knowledge graph
- `data/domain/{domain}/` - Per-domain data
- `data/relations/` - Cross-domain relations

---

### build_document_index.py

**Purpose:** Build full-text search index for SDK documentation.

**Location:** `scripts/build_document_index.py`

**Usage:**
```bash
python scripts/build_document_index.py
```

**What It Does:**
- Parses markdown and text documentation
- Builds Whoosh full-text index
- Enables fast document search via `search_sdk()`

---

### build_excel_constraints.py

**Purpose:** Parse Excel hardware specification files.

**Location:** `scripts/build_excel_constraints.py`

**Usage:**
```bash
python scripts/build_excel_constraints.py
```

**What It Does:**
- Parses hardware constraint Excel files
- Extracts pin multiplexing rules
- Generates `data/constraints/` files
- Validates hardware configurations

---

## Deployment Tools

### deploy_libclang.py

**Purpose:** Download and deploy libclang for Clang integration.

**Location:** `scripts/deploy_libclang.py`

**Usage:**
```bash
python scripts/deploy_libclang.py              # Download and deploy
python scripts/deploy_libclang.py --check      # Check current status
python scripts/deploy_libclang.py --local "C:/path/to/libclang.dll"  # Copy from local
```

**What It Does:**
- Downloads LLVM/libclang from official releases
- Deploys to `lib/clang/{platform}/` directory
- Supports Windows, Linux, macOS
- Minimum version: libclang >= 16.0.0

---

## Utility Scripts

### build_logger.py

**Purpose:** Enhanced logging utility for build scripts.

**Location:** `scripts/build_logger.py`

**Features:**
- Colored console output (Windows/Linux/macOS compatible)
- UTF-8 encoded file logging
- File processing progress tracking
- Build summary generation

**Usage in Scripts:**
```python
from build_logger import BuildLogger
logger = BuildLogger("my_script")
logger.file_start("uart.h")
logger.file_success("uart.h", "15 APIs indexed")
logger.summary()
```

---

### generate_data_quality_report.py

**Purpose:** Analyze and report data quality of generated indices.

**Location:** `scripts/generate_data_quality_report.py`

**Usage:**
```bash
python scripts/generate_data_quality_report.py
```

**What It Does:**
- Analyzes constraint file data sources (Excel parsing vs default)
- Calculates domain data coverage
- Generates quality score (EXCELLENT/GOOD/FAIR/POOR)
- Outputs JSON and console report

**Output:** `data/data_quality_report.json`

---

## clang_analyze MCP Tool

**Purpose:** Real-time C code semantic analysis using Clang.

**Location:** `src/clang_tools.py` (MCP tool), `src/core/clang_parser.py` (parser)

**Usage via MCP:**
```python
# Find all references to a symbol
await clang_analyze(code="B6x_UART_Init", type="refs")

# Deduce type of an expression
await clang_analyze(code="cfg->baudrate", type="type")

# Check code for diagnostics
await clang_analyze(code="B6x_UART_Init(UART1, &cfg);", type="check")

# With context files
await clang_analyze(
    code="uart_init(&config)",
    type="refs",
    files=["projects/my_app/src/uart_config.c"]
)
```

**Analysis Types:**

| Type | Description | Use Case |
|------|-------------|----------|
| `refs` | Find all references | Find where a function/variable is used |
| `type` | Type deduction | Understand expression types |
| `check` | Compiler diagnostics | Detect syntax and semantic errors |

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `code` | string | Yes | C code snippet or symbol name |
| `type` | string | Yes | Analysis type: `refs`, `type`, `check` |
| `files` | array | No | Context files (max 3) |
| `sdk_context` | object | No | Custom SDK context (include paths, defines) |
| `brief` | boolean | No | Return brief output (default: true) |

**SDK Context Auto-injection:**

The tool automatically injects SDK-specific context:
- Include paths: `drivers/api`, `core`, `ble/api`
- Defines: `__B6X__=1`, `ARM_MATH_CM0_PLUS=1`
- Target: `arm-none-eabi`

**When to Use:**

| Scenario | Recommended Tool |
|----------|-----------------|
| Search SDK content | `search_sdk()` (faster, indexed) |
| View node details | `inspect_node()` |
| Validate configuration | `validate_config()` |
| **Real-time code analysis** | `clang_analyze()` |

**Requirements:**

- libclang >= 16.0.0
- Deploy via: `python scripts/deploy_libclang.py`

See [clang-integration.md](clang-integration.md) for detailed documentation.

---

## When to Use These Tools

### Development Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Initial Setup / SDK Updates                              │
│    ↓                                                       │
│    Run: deploy_libclang.py --check                         │
│    Run: build_all_indices.py --incremental                 │
│    → Ensures libclang is available, indices are up-to-date │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ 2. Build Search Indices                                    │
│    ↓                                                       │
│    Run: build_all_indices.py                               │
│    → Rebuilds Whoosh index, docs, constraints              │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ 3. Verify Data Quality                                      │
│    ↓                                                       │
│    Run: generate_data_quality_report.py                    │
│    → Reports index quality and coverage                     │
└─────────────────────────────────────────────────────────────┘
```

### Maintenance Schedule

| Task | Tool | Frequency |
|------|------|-----------|
| Deploy libclang | `deploy_libclang.py` | Once / After LLVM updates |
| Rebuild indices | `build_all_indices.py` | Weekly or as needed |
| Incremental rebuild | `build_all_indices.py --incremental` | After SDK updates |
| Quality check | `generate_data_quality_report.py` | After major changes |

---

## Troubleshooting

### Build Script Issues

**Issue:** "Module not found" errors
```
Solution: Make sure you're running from xm_b6_mcp root directory
cd /path/to/xm_b6_mcp
pip install -r requirements.txt
```

**Issue:** "SDK path not found"
```
Solution: Verify SDK_PATH or run from correct directory
# The scripts expect: ../sdk6 relative to xm_b6_mcp/
```

### libclang Issues

**Issue:** "libclang not found"
```
Solution: Deploy libclang first
python scripts/deploy_libclang.py
# Or check status:
python scripts/deploy_libclang.py --check
```

**Issue:** "libclang version too old"
```
Solution: Update to libclang >= 16.0.0
python scripts/deploy_libclang.py  # Downloads latest compatible version
```

---

## Best Practices

1. **Version Control**
   - Commit generated files in `data/` and `config/`
   - Document what changed in commit message

2. **Testing**
   - Run `build_all_indices.py --test` to verify build
   - Use `generate_data_quality_report.py` to validate data

3. **Backup**
   - Backup `data/` directory before rebuilding indices
   - Keep previous versions for rollback

4. **Documentation**
   - Update this document when adding new tools
   - Document any custom modifications to scripts

---

## Related Documentation

- [Architecture Overview](architecture.md) - MCP Server architecture
- [Build Guide](build-guide.md) - Build and setup instructions
- [Config Directory Guide](config_directory_guide.md) - Configuration file reference

---

**Last Updated:** 2026-03-17
**Maintained By:** B6x MCP Server Team
