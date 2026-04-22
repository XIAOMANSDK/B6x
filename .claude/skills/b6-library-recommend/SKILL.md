---
name: b6-library-recommend
description: |
  根据应用需求推荐 BLE 库并生成 cfg.h 配置。

  TRIGGER when: 用户询问 BLE 库选择、应该用哪个 BLE 库、库配置、连接数配置、RAM 配置、
  BLE 角色配置（Master/Slave）、BLE_LITELIB/BLE_LARGELIB 设置、ANCS Client 支持、
  低功耗 BLE 配置、多连接配置、Mesh 配置。即使没有明确说 "推荐库"，只要涉及 BLE 库/连接数/角色的选择，也应使用此 skill。

  DO NOT TRIGGER when: 用户要求创建新项目（用 /b6-create-project）、编译构建（用 /b6-build）、
  烧录调试（用 /b6-auto-debug）、代码审查（用 /b6-code-review）、硬件配置验证（用 /b6-validate-hardware）。
user-invocable: true
allowed-tools: Read, Grep, Glob, mcp__b6x-mcp-server__search_sdk
---

# B6 Library Recommend

根据应用需求推荐 BLE 库并生成 cfg.h 配置。

**命令**: `/b6-library-recommend [描述]`

## 库选择规则

| 条件 | 库 | RAM | 角色 |
|------|-----|-----|------|
| **仅从设备** 1 个连接 / HID / 传感器 / 低功耗 | `ble6_lite.lib` | ~6KB | Slave Only |
| 主设备 + 从设备共 2~3 个连接 / 通用应用 | `ble6.lib` | ~14.6KB | Master + Slave |
| 主设备 + 从设备共 4-6 连接 / 网关 | `ble6_8act_6con.lib` | ~16.2KB | Master + Slave |
| Mesh | `ble6_mesh.lib` | ~22KB | Mesh Server or Mesh Client |

> ⚠️ **重要**: `ble6_lite.lib` **只能作为从设备（Slave）**，不支持主设备（Master）角色！
> - 需要作为主设备连接其他 BLE 设备时（如 ANCS Client、BLE Central），必须使用 `ble6.lib` 或 `ble6_8act_6con.lib`

## 执行步骤

1. **需求澄清**：若用户未明确说明以下信息，先提问后再推荐：
   - 连接数（同时连接几个设备？）
   - 角色（作为从设备被连接、还是主动连接其他设备？）
   - 特殊需求（低功耗优先？Apple FindMy？Mesh？）
2. 根据需求选择库，输出推荐和 cfg.h 配置
3. 若用户提供了现有 cfg.h，对比推荐结果并说明差异；否则跳过此步
4. 若需查询特定 Profile 或不常见场景的 SDK 支持情况，按需调用：
   `mcp__b6x-mcp-server__search_sdk("<具体查询词>")`

## cfg.h 配置模板

```c
// ble6_lite.lib (1 从设备连接) - 仅支持 Slave 角色
#define BLE_LITELIB     (1)
#define BLE_LARGELIB    (0)
#define BLE_NB_SLAVE    (1)
#define BLE_NB_MASTER   (0)   // lite 库不支持 Master

// ble6.lib (主从共 3 连接) - 支持 Master + Slave
#define BLE_LITELIB     (0)
#define BLE_LARGELIB    (0)
#define BLE_NB_SLAVE    (1)   // 从设备连接数
#define BLE_NB_MASTER   (2)   // 主设备连接数 (ANCS Client 等需要设置)

// ble6_8act_6con.lib (主从共 6 连接) - 支持 Master + Slave
#define BLE_LITELIB     (0)
#define BLE_LARGELIB    (1)
#define BLE_NB_SLAVE    (3)
#define BLE_NB_MASTER   (3)
```

## 输出格式

```
推荐: ble6_lite.lib
理由: 单连接 HID 应用

cfg.h:
#define BLE_LITELIB  (1)
#define BLE_LARGELIB (0)
#define BLE_NB_SLAVE (1)

资源: RAM ~6KB, 连接数 1
```
