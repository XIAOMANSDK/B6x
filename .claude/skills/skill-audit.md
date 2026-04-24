# Skill 审计指南 (Skill Audit Guide)

> B6x SDK Skills 长期合规审查工具
> 基于 skill-creator 官方指南 + writing-skills 最佳实践
> 初始创建: 2026-04-01

## 使用说明

两个工具分工明确，按顺序使用：

1. **`/skill-creator`** — 创建或改进 skill（动态行为验证）
   - 捕获意图 → 编写草稿 → 测试用例 → 子代理跑测试 → 量化评估 → 迭代改进
   - 验证 skill 是否正确触发、输出是否达标

2. **本文档 (skill-audit.md)** — 静态合规审查（格式/结构/规范性）
   - 对照官方指南检查 frontmatter、body、token 效率、说服力等
   - 确保 description 用中文、保留 TRIGGER/DO NOT TRIGGER 格式

**流程：`/skill-creator` 完成 skill 创建/改进 → `skill-audit.md` 做合规审查**

> 如果 skill 尚未创建，先用 `/skill-creator`；如果 skill 已存在只需审查优化，直接用本文档。

## 项目约定

1. **description 使用中文**
2. **保留 TRIGGER when / DO NOT TRIGGER when 格式**
3. **参考官方 writing-skills 指南优化其他方面**
4. **保留项目特有字段**: `user-invocable`, `allowed-tools`, `disable-model-invocation`, `tool-usage`, `compatibility`

## 官方参考文档

Step 1 审查前，**必须先读取并总结**以下 4 个文件：

| # | 路径 | 内容 |
|---|------|------|
| 1 | `~/.claude/plugins/cache/claude-plugins-official/superpowers/5.0.6/skills/writing-skills/SKILL.md` | Skill 文件格式规范 (frontmatter/body 结构) |
| 2 | `~/.claude/plugins/cache/claude-plugins-official/superpowers/5.0.6/skills/writing-skills/anthropic-best-practices.md` | Token 效率、简洁原则、渐进式展开 |
| 3 | `~/.claude/plugins/cache/claude-plugins-official/superpowers/5.0.6/skills/writing-skills/testing-skills-with-subagents.md` | 红旗列表、反合理化条款、测试方法 |
| 4 | `~/.claude/plugins/cache/claude-plugins-official/superpowers/5.0.6/skills/writing-skills/persuasion-principles.md` | 权威语言、说服力强化 |

---

## 工作流程 (Workflow)

每个 Skill 严格遵循以下 **5 步**流程：

```
┌─────────────────────────────────────────────────────┐
│  Step 1: 审查 (Review)                              │
│  ├─ 读取 4 个官方参考文档 (见上方「官方参考文档」)    │
│  ├─ 读取当前 SKILL.md (+ reference.md 如有) 全文     │
│  ├─ 对照 Step 5 合规检查表逐项检查                   │
│  └─ 输出《审查报告》：列出具体问题和改进点           │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  Step 2: 提案 (Propose)                             │
│  ├─ 针对每个问题提出具体修改建议                     │
│  ├─ 展示 Before/After 对比                          │
│  └─ 等待用户逐条确认/否决/调整                      │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  Step 3: 方案评审 (Review Plan)                     │
│  ├─ 汇总用户确认的所有修改                           │
│  ├─ 输出完整修改方案（diff 预览）                    │
│  └─ 用户最终确认 "执行" 或 "再调整"                 │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  Step 4: 执行 (Execute)                             │
│  ├─ 应用所有确认的修改到 SKILL.md                    │
│  └─ 如需新建 reference.md 则同步创建                 │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  Step 5: 官方指南复审 (Final Audit)                  │
│  ├─ 对照下方《官方指南合规检查表》逐项打勾           │
│  └─ 展示检查结果给客户                               │
└─────────────────────┬───────────────────────────────┘
                      │
               ┌──────▼──────┐
               │ 客户是否接受？│
               └──┬───────┬──┘
                  │       │
            接受  │       │ 不接受
                  ▼       ▼
          ┌──────────┐  ┌──────────────────────┐
          │ ✅ 完成   │  │ 回到 Step 2 继续修改  │
          │ 更新进度  │  │ 针对客户反馈调整      │
          │ 下一个    │  │ → Step 3 → Step 4    │
          │ Skill    │  │ → Step 5 再次复审     │
          └──────────┘  └──────────┬───────────┘
                        └──────────┬───────────┘
                                   │
                              (循环直到接受)
```

---

## Step 5: 合规检查表

> 每个修改完的 Skill 须通过以下检查，结果展示给客户最终裁定。
> 来源: `writing-skills/SKILL.md` + `anthropic-best-practices.md` + `testing-skills-with-subagents.md` + `persuasion-principles.md`

### A. Frontmatter 合规

| # | 检查项 | 来源 | ✅/❌ |
|---|--------|------|-------|
| A1 | `name` 使用小写字母+数字+连字符，≤64字符 | SKILL.md | |
| A2 | `description` 总长 ≤ 1024 字符 | SKILL.md | |
| A3 | `description` 第三人称，不含 "我"/"你" | best-practices | |
| A4 | `description` 包含具体触发条件和关键词 | SKILL.md CSO | |
| A5 | `description` 包含错误消息/症状/工具名等搜索词 | SKILL.md 关键词覆盖 | |
| A6 | 项目特有字段保留完整 (`user-invocable`, `allowed-tools` 等) | 项目约定 | |

### B. Body 内容合规

| # | 检查项 | 来源 | ✅/❌ |
|---|--------|------|-------|
| B1 | body < 500 行 | best-practices | |
| B2 | 概述 1-2 句核心原则，不含常识解释 | best-practices 简洁原则 | |
| B3 | 代码示例: 一个优秀示例 > 多个平庸示例 | SKILL.md | |
| B4 | 无叙事性内容 ("在会话中我们发现...") | SKILL.md 反模式 | |
| B5 | 无时间敏感信息 | best-practices | |
| B6 | 术语一致 (同一概念始终用同一术语) | best-practices | |
| B7 | 流程图仅用于不明显的决策点 | SKILL.md | |
| B8 | 路径使用正斜杠 `/` 而非反斜杠 `\` | best-practices 反模式 | |

### C. Token 效率

| # | 检查项 | 来源 | ✅/❌ |
|---|--------|------|-------|
| C1 | 重型参考 (>100行) 已拆到独立文件 | SKILL.md 渐进式展开 | |
| C2 | 引用层级仅一层深 (SKILL.md → file) | best-practices | |
| C3 | 独立文件 >100 行时顶部有目录 | best-practices | |
| C4 | 无重复内容 (不重复 CLAUDE.md / rules 中的信息) | SKILL.md 交叉引用 | |
| C5 | "Claude 已经知道的" 不解释 | best-practices 简洁原则 | |

### D. 说服力与合规性 (仅纪律型 Skill)

| # | 检查项 | 来源 | ✅/❌ |
|---|--------|------|-------|
| D1 | 关键规则使用权威语言 ("必须"/"禁止"/"无例外") | persuasion-principles | |
| D2 | 合理化借口表 (如有) | SKILL.md | |
| D3 | 红旗列表 — "看到这个想法 → 停止" | testing-skills | |
| D4 | 反合理化条款 (明确禁止常见绕过方式) | testing-skills | |

### E. 功能完整性

| # | 检查项 | 来源 | ✅/❌ |
|---|--------|------|-------|
| E1 | 功能未缩减 — 修改前后功能等价 | 项目约定 | |
| E2 | 触发条件未丢失 — TRIGGER/DO NOT TRIGGER 保留 | 用户需求 | |
| E3 | 中文内容保留 | 用户需求 | |

### F. 合规判定

复审结果展示给客户，由客户决定：

- **客户说"接受"** → ✅ 该 Skill 完成，更新进度，进入下一个 Skill
- **客户说"不接受"或提出修改意见** → ❌ 记录客户反馈，回到 Step 2 针对性调整 → Step 3 → Step 4 → Step 5 再次复审 → **循环直到客户说"接受"**
- **D 项不适用** → 标记 N/A（仅参考类 Skill，无需纪律强化）

---

### 规则

1. **每次只改一个 skill**，严格从 Step 1 开始完整执行 5 步工作流（审查→提案→方案评审→执行→复审），不得跳步或复用上一轮结果
2. **Step 2 中每条建议独立确认**，不批量通过
3. **Step 3 用户明确说"执行"后才动手改文件**
4. **保持功能完整性**: 不删除实际有用的内容，只优化格式和效率
5. **保留项目特有字段**: `user-invocable`, `allowed-tools`, `disable-model-invocation`, `tool-usage`, `compatibility`
6. **description 使用中文**
7. **保留 TRIGGER when / DO NOT TRIGGER when 格式**

## 进度追踪

| # | Skill | 状态 | 修改内容 |
|---|-------|------|----------|
| 1 | b6-build | ✅ 已完成 | 路径正斜杠+消除重复 |
| 2 | b6-auto-debug | ✅ 已完成 | 删除备选方案冗余+添加红旗列表 |
| 3 | b6-create-project | ✅ 已完成 | reference.md添加TOC+Phase7去重 |
| 4 | b6-code-review | ✅ 已完成 | 精简输出模板+添加红旗列表 |
| 5 | b6-validate-hardware | ✅ 已完成 | 删除重复示例章节 |
| 6 | b6-cmake-init | ✅ 无需修改 | 模板与reference重复属设计取舍 |
| 7 | b6-library-recommend | ✅ 无需修改 | 精简合规 |
| 8 | b6-project-checklist | ✅ 已完成 | 精简输出模板(68→20行) |
| 9 | b6-translate-error | ✅ 无需修改 | 精简合规 |
| 10 | b6-power-analyze | ✅ 已完成 | CSO修正+删除6处"为什么"+精简输出模板(278→208行) |
| 11 | b6-sram-analyze | ✅ 已完成 | 合并重复阈值表+精简输出模板+CSO修正(253→195行) |
| 12 | extract-doc | ✅ 已完成 | 消除3处重复+精简代码/删除基础步骤(253→153行) |
