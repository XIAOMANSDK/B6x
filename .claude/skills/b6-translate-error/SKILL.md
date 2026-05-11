---
name: b6-translate-error
description: |
  翻译 BLE 错误码，提供排查建议。

  TRIGGER when: 用户询问 BLE 错误码含义、错误码翻译、0x 开头的错误代码、ATT_ERR/GAP_ERR/SMP_ERR
  等错误常量、配对失败原因、连接错误、GATT 错误、BLE 返回码、协议栈错误。即使没有明确说 "翻译错误码"，
  只要出现 BLE 相关的数字错误码或错误常量名，也应使用此 skill。

  DO NOT TRIGGER when: 用户要求创建新项目（用 /b6-create-project）、编译构建（用 /b6-build）、
  烧录调试（用 /b6-auto-debug）、代码审查（用 /b6-code-review）、硬件配置验证（用 /b6-validate-hardware）。
user-invocable: true
disable-model-invocation: true
allowed-tools: Read, Grep, Glob, mcp__b6x-mcp-server__search_sdk
---

# B6 Translate Error

翻译 BLE 错误码，提供排查建议。

**命令**: `/b6-translate-error <error_code>`

**支持格式**: `0x85` / `133` / `ATT_ERR_INVALID_HANDLE`

---

## 错误分层

| 范围 | 层 | 前缀 | 说明 |
|------|-----|------|------|
| 0x00-0x0D | GAP | `GAP_ERR_` | 通用访问 profile |
| 0x80-0x90 | ATT | `ATT_ERR_` | 属性协议 |
| 0xA0-0xAF | L2C | `L2C_ERR_` | L2CAP |
| 0xB0-0xB6 | GATT | `GATT_ERR_` | GATT profile |
| 0xC0-0xDC | SMP | `SMP_ERR_` | 安全管理 |
| 0xE0-0xF2 | PRF | `PRF_ERR_` | Profile 错误 |
| 0x100+ | LL | `LL_ERR_` | 链路层 |

---

## MCP 工具

| 工具 | 用途 |
|------|------|
| `search_sdk("error 0x85", "docs")` | 搜索错误码文档 |

> 详见 `.claude/rules/mcp-usage.md`

---

## 执行步骤

1. 解析错误码（十六进制/十进制/常量名）
2. 根据范围确定协议层
3. 用 `search_sdk()` 搜索或读 `ble/api/le_err.h`
4. 输出翻译和排查建议

---

## 输出格式

```
错误: 0x85 (133)
层: ATT (Attribute Protocol)
名称: ATT_ERR_INVALID_HANDLE

描述: 属性句柄无效

原因:
1. 访问不存在的特征值
2. 服务发现未完成就访问
3. 句柄被服务变更通知失效

排查:
1. 确认服务发现完成后再读写
2. 检查 GATT 表中是否存在该特征
3. 重新执行服务发现
```

---

## 常见错误速查

### GAP 错误

| 码 | 名称 | 含义 | 解决 |
|----|------|------|------|
| 0x01 | GAP_ERR_TIMEOUT | 操作超时 | 检查连接状态，重试 |
| 0x07 | GAP_ERR_NO_LINK | 无连接 | 先建立连接 |
| 0x09 | GAP_ERR_AUTH_FAIL | 认证失败 | 重新配对 |
| 0x0B | GAP_ERR_REQ_CANCEL | 请求取消 | 检查应用逻辑 |

### ATT 错误

| 码 | 名称 | 含义 | 解决 |
|----|------|------|------|
| 0x80 | ATT_ERR_INVALID_HANDLE | 句柄无效 | 检查特征句柄 |
| 0x81 | ATT_ERR_READ_NOT_PERMITTED | 禁止读取 | 检查权限设置 |
| 0x82 | ATT_ERR_WRITE_NOT_PERMITTED | 禁止写入 | 检查权限设置 |
| 0x8A | ATT_ERR_NOT_FOUND | 属性未找到 | 检查 UUID |
| 0x8C | ATT_ERR_REQUEST_NOT_SUP | 不支持请求 | 检查协议版本 |

### SMP 错误

| 码 | 名称 | 含义 | 解决 |
|----|------|------|------|
| 0xC5 | SMP_ERR_PAIRING_FAILED | 配对失败 | 重新配对 |
| 0xC6 | SMP_ERR_DHKEY_CHK_FAIL | 密钥校验失败 | 重新配对 |
| 0xC8 | SMP_ERR_UNSUPP_REMOTE_FEAT | 不支持远程特性 | 检查设备兼容性 |

---

## 错误处理

| 错误 | 处理 |
|------|------|
| 错误码格式无效 | 提示正确格式 |
| 未找到错误定义 | 搜索 SDK 源码 |
| 多个匹配 | 列出所有可能 |

---

