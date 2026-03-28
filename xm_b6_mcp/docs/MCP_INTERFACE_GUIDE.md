# B6x MCP Server - MCP Interface Guide

> Version: 3.5.0 | MCP SDK: >=0.9.0

This document describes all MCP tool interfaces provided by the B6x MCP Server.

---

## Quick Reference

| Tool | Purpose | Layer |
|------|---------|-------|
| `search_sdk` | Discover SDK content | Layer 1 - Discovery |
| `inspect_node` | Get detailed node info | Layer 2 - Detail |
| `validate_config` | Validate hardware config | Layer 3 - Validation |
| `clang_analyze` | C code semantic analysis | Clang Integration |
| `get_server_info` | Server statistics | Utility |

---

## Tool 1: search_sdk

### Description
Search B6x SDK across multiple domains (APIs, docs, registers, examples).

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `query` | string | **Yes** | - | Search query (e.g., "UART initialization") |
| `scope` | string | No | "all" | Domain to search |
| `max_results` | integer | No | 10 | Max results per domain |
| `merge_mode` | string | No | "interleave" | How to merge results |

**Scope Options:**
- `"all"` - Search all domains
- `"api"` - API functions only
- `"docs"` - Documentation only
- `"macros"` - Macro definitions only
- `"registers"` - Hardware registers only
- `"examples"` - Example code only

**Merge Mode Options:**
- `"interleave"` - Interleave results by relevance
- `"group"` - Group results by domain
- `"rank"` - Global ranking with domain weights

### Register Search Patterns

When `scope="registers"`, the following search patterns are supported:

| Pattern | Example | Matches |
|---------|---------|---------|
| Full name | `"UART1_BRR"` | Exact match for UART1's BRR register |
| Short name | `"BRR"` | BRR register in all peripherals |
| Peripheral prefix | `"UART1"` | All UART1 registers |
| Fuzzy search | `"baud"` | Register names and descriptions |

### Response Format

```json
{
  "success": true,
  "status": "success",
  "query": "UART1_BRR",
  "scopes_searched": ["registers"],
  "total_results": 1,
  "results": [
    {
      "domain": "registers",
      "item_type": "register",
      "item_id": "BRR",
      "title": "UART1_BRR",
      "summary": "Baud rate register",
      "relevance_score": 1.0,
      "metadata": {
        "address": "0x40023008",
        "access": "read-write",
        "offset": "0x008"
      },
      "quick_actions": ["inspect_register", "inspect_fields"]
    }
  ],
  "pagination": {
    "has_more": false
  }
}
```

### Example Call

```python
# MCP Tool Call
{
  "name": "search_sdk",
  "arguments": {
    "query": "UART1_BRR",
    "scope": "registers",
    "max_results": 5
  }
}
```

---

## Tool 2: inspect_node

### Description
Inspect a specific SDK node (API, register, doc) with detailed view.

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `node_id` | string | **Yes** | - | Node identifier (e.g., "api:B6x_UART_Init") |
| `view_type` | string | No | "auto" | Type of view to return |
| `include_context` | boolean | No | true | Include related nodes |

**Node ID Format:** `type:identifier`

| Type | Prefix | Example |
|------|--------|---------|
| API | `api:` | `api:B6x_UART_Init` |
| Register | `reg:` | `reg:UART1_BRR` |
| Document | `docs:` | `docs:uart_guide` |

**View Types:**
- `"auto"` - Automatically select best view
- `"summary"` - Brief overview
- `"definition"` - Full definition (API signature, struct)
- `"implementation"` - Source code
- `"dependencies"` - Dependency graph
- `"register_info"` - Register fields and bit definitions

### Response Format

```json
{
  "success": true,
  "node_id": "api:B6x_UART_Init",
  "view_type": "summary",
  "data": {
    "name": "B6x_UART_Init",
    "signature": "void B6x_UART_Init(UART_TypeDef *UARTx, UART_Config_t *config)",
    "description": "Initialize UART peripheral",
    "parameters": [
      {"name": "UARTx", "type": "UART_TypeDef *", "description": "UART instance"},
      {"name": "config", "type": "UART_Config_t *", "description": "Configuration"}
    ],
    "header_file": "drivers/api/uart.h"
  },
  "related_nodes": [
    {"node_id": "api:UART_Config_t", "relation": "uses_type"},
    {"node_id": "reg:UART1_BRR", "relation": "configures"}
  ]
}
```

### Example Call

```python
# MCP Tool Call
{
  "name": "inspect_node",
  "arguments": {
    "node_id": "api:B6x_UART_Init",
    "view_type": "definition",
    "include_context": true
  }
}
```

---

## Tool 3: validate_config

### Description
Validate hardware configuration against chip constraints.

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `config_json` | string | **Yes** | - | Configuration as JSON string |
| `stop_on_first_error` | boolean | No | false | Stop at first error |
| `include_suggestions` | boolean | No | true | Include fix suggestions |

### Configuration JSON Structure

```json
{
  "pins": {
    "PA06": "CSC_UART1_TXD",
    "PA07": "CSC_UART1_RXD"
  },
  "clock": {
    "system_clock": 32,
    "enabled_peripherals": ["GPIO", "UART"]
  },
  "dma": [
    {"channel": 0, "peripheral": "UART1_TX"}
  ],
  "interrupts": [
    {"irq_name": "UART1_IRQn", "priority": 5}
  ],
  "memory": {
    "flash_used_kb": 128,
    "sram_used_kb": 16
  }
}
```

### Response Format

```json
{
  "is_valid": true,
  "errors": [],
  "warnings": [
    {
      "category": "memory",
      "message": "Flash usage is high (80%)",
      "suggestion": "Consider optimizing code size"
    }
  ],
  "suggestions": [
    {
      "category": "power",
      "message": "Consider using low-power mode for battery applications"
    }
  ],
  "validated_sections": ["pins", "clock", "dma", "interrupts", "memory"]
}
```

### Example Call

```python
# MCP Tool Call
{
  "name": "validate_config",
  "arguments": {
    "config_json": "{\"pins\": {\"PA06\": \"CSC_UART1_TXD\"}, \"clock\": {\"system_clock\": 32}}",
    "stop_on_first_error": false,
    "include_suggestions": true
  }
}
```

---

## Tool 4: clang_analyze

### Description
Real-time semantic C code analysis using Clang.

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `code` | string | **Yes** | - | C code snippet or symbol name |
| `type` | string | **Yes** | - | Analysis type |
| `files` | array | No | [] | Context files (max 3) |
| `sdk_context` | object | No | null | SDK context override |

**Analysis Types:**

| Type | Description | Use Case |
|------|-------------|----------|
| `refs` | Find all references | Where is this symbol used? |
| `type` | Deduce type | What is the type of this expression? |
| `check` | Diagnostics | Are there errors in this code? |

### Response Format

```json
{
  "success": true,
  "analysis_type": "refs",
  "code": "B6x_UART_Init",
  "results": [
    {
      "file": "drivers/src/uart.c",
      "line": 45,
      "column": 5,
      "context": "B6x_UART_Init(UART1, &config);"
    },
    {
      "file": "projects/demo/main.c",
      "line": 120,
      "column": 3,
      "context": "B6x_UART_Init(UART1_PORT, &uart_cfg);"
    }
  ],
  "total_references": 2
}
```

### Example Calls

```python
# Find all references
{
  "name": "clang_analyze",
  "arguments": {
    "code": "B6x_UART_Init",
    "type": "refs"
  }
}

# Check code for errors
{
  "name": "clang_analyze",
  "arguments": {
    "code": "uart_init(UART1, &config);",
    "type": "check",
    "files": ["projects/my_app/src/uart_config.c"]
  }
}

# Deduce type
{
  "name": "clang_analyze",
  "arguments": {
    "code": "cfg->baudrate",
    "type": "type"
  }
}
```

---

## Tool 5: get_server_info

### Description
Get B6x MCP server information and statistics.

### Parameters
None

### Response Format

```json
{
  "name": "b6x-mcp-server",
  "version": "3.5.0",
  "architecture": "Three-Layer v3.5 + Clang Integration",
  "total_tools": 5,
  "tool_list": [
    "search_sdk",
    "inspect_node",
    "validate_config",
    "clang_analyze",
    "get_server_info"
  ],
  "features": {
    "search": true,
    "inspect": true,
    "validate": true,
    "clang_analysis": true
  },
  "uptime_seconds": 3600.5,
  "stats": {
    "total_requests": 150,
    "successful_requests": 148,
    "failed_requests": 2,
    "tools_called": {
      "search_sdk": 80,
      "inspect_node": 45,
      "validate_config": 15,
      "clang_analyze": 8,
      "get_server_info": 2
    }
  }
}
```

### Example Call

```python
# MCP Tool Call
{
  "name": "get_server_info",
  "arguments": {}
}
```

---

## Typical Workflow

### 1. API Discovery Workflow

```
search_sdk("uart_init", scope="api")
    ↓
inspect_node("api:B6x_UART_Init", view_type="definition")
    ↓
inspect_node("api:UART_Config_t", view_type="definition")
    ↓
validate_config({...})
```

### 2. Register Debugging Workflow

```
search_sdk("UART1_BRR", scope="registers")
    ↓
inspect_node("reg:UART1_BRR", view_type="register_info")
    ↓
clang_analyze("UART1->BRR = 9600;", type="check")
```

### 3. Hardware Configuration Workflow

```
search_sdk("CSC_UART1", scope="macros")
    ↓
validate_config({
  "pins": {"PA06": "CSC_UART1_TXD"},
  "clock": {"system_clock": 32}
})
    ↓
Fix errors based on suggestions
```

---

## Error Handling

All tools return structured error responses:

```json
{
  "success": false,
  "error": "Tool execution failed",
  "tool": "search_sdk",
  "suggestion": "Check the logs for more details"
}
```

Common error patterns:

| Error | Tool | Cause | Solution |
|-------|------|-------|----------|
| `Invalid JSON` | validate_config | Malformed config_json | Validate JSON syntax |
| `Unknown tool` | call_tool | Invalid tool name | Check available tools |
| `Node not found` | inspect_node | Invalid node_id | Use search_sdk first |
| `Clang not available` | clang_analyze | libclang not loaded | Check Clang installation |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 3.5.0 | 2025-03 | Clang integration, register search patterns |
| 3.4.0 | 2025-02 | Fixed tool registration, token limits |
| 3.3.0 | 2025-01 | Three-layer architecture |
