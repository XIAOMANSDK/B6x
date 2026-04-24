---
name: using-b6x-skills
description: B6x 开发会话启动时加载 - 建立 B6x skill 发现与调用规则，要求在任何响应前先调用 Skill 工具
---

<B6X_SKILLS>

你拥有 B6x 开发专用 skill。

**以下是你使用 B6x skill 的指南。所有 skill 均通过 `Skill` 工具调用。**

## 指令优先级

1. **CLAUDE.md 用户指令** — 最高优先级（如 CLAUDE.md 明确要求某种做法，遵从用户）
2. **B6x Skills** — 覆盖默认系统行为
3. **默认系统提示** — 最低优先级

## 核心规则

**在任何响应或操作之前，先调用相关的 B6x skill。** 即使只有 1% 的可能性与某个 skill 相关，也应该先调用查看。如果调用后发现不适用，可以忽略并正常继续。

## 决策流程

```
收到用户消息
  → "是否涉及 B6x 开发？"
    → 是 → 检查是否有匹配 skill → 调用 Skill 工具 → 遵循 skill 指令
    → 否 → 正常响应
  → "即将声称完成/修复/通过？"
    → 是 → 必须先调用 b6_verification-before-completion
```

## B6x Skill 速查表

| Skill | 触发关键词 |
|-------|-----------|
| `/b6-build` | 编译、构建、Keil、GCC、CMake、错误、警告 |
| `/b6-auto-debug` | 烧录、调试、下载、DAPLink、J-Link、RTT、监控 |
| `/b6-create-project` | 新项目、创建、BLE 应用、实现功能 |
| `/b6-code-review` | 代码审查、审查代码、MISRA、安全性 |
| `/b6-validate-hardware` | 引脚配置、时钟、DMA、中断、硬件验证 |
| `/b6-library-recommend` | BLE 库、用哪个库、连接数、RA |
| `/b6-project-checklist` | 项目检查、完整性、结构验证 |
| `/b6-sram-analyze` | SRAM 用量、内存分析、RAM |
| `/b6-power-analyze` | 功耗、低功耗、电流 |
| `/b6-translate-error` | 错误码、BLE 错误、翻译错误码 |
| `/b6-review-reception` | 审查反馈、审查响应（由 b6-code-review 自动加载） |
| `/b6_verification-before-completion` | 即将声称完成、修复、通过 |

## Skill 优先级

多个 skill 同时适用时，按以下顺序调用：

1. **流程类优先**：`b6_verification-before-completion`、`b6-code-review`
2. **工作流类次之**：`b6-create-project`、`b6-auto-debug`
3. **工具类最后**：`b6-build`、`b6-validate-hardware`、`b6-library-recommend`

## Skill 类型

| 类型 | Skills | 说明 |
|------|--------|------|
| **严格型（Rigid）** | `b6-build`、`b6-auto-debug`、`b6_verification-before-completion` | 必须严格遵循每个步骤 |
| **灵活型（Flexible）** | `b6-create-project`、`b6-library-recommend` | 原则性指导，可根据上下文调整 |

## 红旗词 — 看到这些想法时必须停下来调用 skill

| 你的想法 | 现实 | 应对动作 |
|---------|------|---------|
| "编译一下就行" | 编译有专门流程 | 先调用 `/b6-build` |
| "这个简单，直接改" | 简单任务也会变复杂 | 检查是否有 skill 覆盖 |
| "修复看起来正确" | 未经验证 = 未完成 | 调用 `b6_verification-before-completion` |
| "我记得这个 skill" | Skill 会更新 | 必须重新读取当前版本 |
| "不需要正式流程" | 任何时候都应该用 skill | 适用就用 |
| "引脚应该配成..." | 硬件配置有约束 | 先调用 `/b6-validate-hardware` |
| "先做这一步再说" | 检查必须在行动之前 | 先调用相关 skill |
| "我来烧录固件" | 烧录有完整流程 | 先调用 `/b6-auto-debug` |
| "差不多完成了" | 没有验证证据就不算完成 | 调用 `b6_verification-before-completion` |

## 使用方法

- 使用 `Skill` 工具并传入 skill 名称（如 `skill: "b6-build"`）
- Skill 会展开为完整指令 — 直接遵循执行
- 如果调用后发现不适用，正常继续即可

</B6X_SKILLS>
