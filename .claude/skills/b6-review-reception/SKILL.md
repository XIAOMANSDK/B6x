---
name: b6-review-reception
description: |
  B6x 审查报告接收处理 — 验证每个审查发现后再实施修复。
  由 b6-code-review Step 6 自动加载，不独立触发。

  TRIGGER when: b6-code-review 完成后自动衔接。

  DO NOT TRIGGER when: 还没有审查报告、用户直接要求修改代码。
user-invocable: false
allowed-tools: Read, Grep, Glob, mcp__b6x-mcp-server__search_sdk, mcp__b6x-mcp-server__inspect_node
---

# B6 Review Reception

B6x 审查报告的接收处理协议。核心原则：**验证后再实施，技术正确优先。**

---

## 响应模式

对报告中的每个 issue（score ≥ 75），执行以下 6 步循环：

```
READ → UNDERSTAND → VERIFY → EVALUATE → RESPOND → IMPLEMENT
```

| 步骤 | 动作 | 说明 |
|------|------|------|
| READ | 完整阅读报告 | 不挑选，不跳过，全部读完再反应 |
| UNDERSTAND | 理解问题本质 | 用自己的话重述问题（说不清 → 问清楚） |
| VERIFY | 验证问题真实性 | 读代码、查 SDK API、对排除规则 |
| EVALUATE | 评估是否需要修复 | 真问题？影响范围？修复风险？ |
| RESPOND | 技术回应或 Push Back | 有证据就坚持，有错误就承认 |
| IMPLEMENT | 逐个修复并验证 | 一次一个问题，修复后验证无回归 |

---

## 审查态度 — 不信任原则

**不信任任何审查结果。** 必须独立验证：

- 不接受审查 agent 的判断，必须读取实际代码确认问题存在
- 不接受对 API 用法的断言，必须用 `search_sdk`/`inspect_node` 验证
- 对比误报排除规则，确认问题不在豁免范围内
- 警惕建议的修复引入新问题（过度修复）

---

## 验证协议

对每个 score ≥ 75 的 issue：

1. 读取报告引用的 `file:line`，**亲眼确认**问题存在
2. 用 `search_sdk` / `inspect_node` 验证 API 用法是否确实违反规范
3. 检查是否属于误报排除规则范围（`/* MISRA-IGNORE */`、SDK 内部约定等）
4. `grep` 相关调用方，确认修复不会破坏依赖链
5. 如果问题在未修改的代码行中 → 跳过（只审查变更范围）

**不明确的问题 → 先澄清，再动手。** 部分理解 = 错误修复。

### 禁止行为

- 不明确就猜测着修复 → 先问用户
- 审查说有问题就直接改 → 先读代码确认
- 批量修复所有问题 → 逐个验证、逐个修复

---

## Push Back 协议

当对审查结论有异议时，**用技术理由捍卫或修正**：

- 引用具体代码（file:line）作为证据
- 引用权威来源（SDK 文档、BLE 规范章节、MISRA 规则编号、芯片 datasheet）
- 如果反驳有技术依据，**立即撤回**该问题 — 不固执
- 如果反驳不成立，明确说明**为什么不成立**，给出替代方案

**原则：** 对事不对人。有证据就坚持，有错误就承认。

---

## 实施顺序

1. **澄清**：所有不明确的问题先搞清楚（不清不楚 → 不动手）
2. **按优先级修复**：

| 优先级 | 类别 | 示例 |
|--------|------|------|
| 1st | 🔴 MISRA 安全 | M06 禁止 malloc、M07 禁止递归 |
| 2nd | 🔴 安全性 | C01-C05 数组边界、空指针、溢出 |
| 3rd | 🔴 返回值检查 | M10 SDK 函数返回值必须检查 |
| 4th | 🟡 BLE 规范 | B01-B05 UUID、连接参数、事件处理 |
| 5th | 🟡 嵌入式缺陷 | E01-E08 ISR、volatile、位操作 |
| 6th | 🟡 代码风格 | S01-S07 命名、格式、注释 |
| 7th | 💡 Info | 仅在 score ≥ 90 时处理 |

3. **逐个修复**：每修复一个问题 → 验证无回归
4. **复审**：全部修复后 → `/b6-build` 编译通过 → `/b6-code-review` 复审

---

## 闭环

```
修复完成 → /b6-build 编译 → /b6-code-review 复审 → 直到 0 Critical, 0 Warning
```
