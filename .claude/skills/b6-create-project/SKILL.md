---
name: b6-create-project
description: |
  创建 B6x SDK 新项目/工程/演示/例程/程序，引导式功能开发流程，生成 spec.md → plan.md → 项目文件。

  TRIGGER when: 用户要求创建新项目、新建工程、创建演示程序、写一个例程、实现某个功能、开发 BLE 应用、新建外设示例、基于 SDK 开发功能。即使没有明确说 "创建项目"，只要涉及从零开始构建 B6x 应用，也应使用此 skill。

  DO NOT TRIGGER when: 用户要求编译现有项目（用 /b6-build）、烧录调试（用 /b6-auto-debug）、代码审查（用 /b6-code-review）、验证硬件配置（用 /b6-validate-hardware）、修复现有代码问题。
user-invocable: true
allowed-tools: Read, Grep, Glob, Write, Edit, Agent, Skill, TaskCreate, TaskUpdate, TaskList, mcp__b6x-mcp-server__search_sdk, mcp__b6x-mcp-server__inspect_node, mcp__b6x-mcp-server__validate_config
tool-usage: |
  - MCP 工具（search_sdk, inspect_node）：用于所有 SDK 信息检索
  - Skill 工具：调用其他 skill（b6-library-recommend, b6-validate-hardware, b6-project-checklist）
  - Task 工具（TaskCreate, TaskUpdate, TaskList）：逐任务进度追踪
---

# B6 Create Project

创建 B6x SDK 项目，引导式功能开发，注重代码库理解和架构设计，6 阶段流程。**禁止跳过任何步骤。**

## Skill 类型：严格型（Rigid）

本 skill 必须严格按顺序执行，不可跳过或调换阶段：
- Phase 1→2→3→4→5→6 严格顺序
- Phase 5 内的 Task 按 plan.md 顺序逐个执行
- 每个 Task 验证通过后才能进入下一个

## 核心原则

1. **阶段完整**：必须完成所有 6 个阶段，每阶段用户确认后方可继续
2. **MCP 优先**：SDK 检索必须使用 MCP 工具，**禁止 Grep/Glob 搜索 SDK 源码**
3. **按需澄清**：识别到真实歧义时提问；需求已明确则列出假设确认后继续（Phase 3）
4. **先理解再行动**：阅读 Agent 标识的关键文件建立深度理解后再写代码
5. **简单优雅**：优先考虑可读性、可维护性和架构合理性
6. **逐任务执行**：每个 Task 独立执行、验证、完成后再进入下一个（详见 Phase 5）
7. **伴随验证**：每个 Task 完成后立即验证，而非全部完成后集中验证
8. **失败回退**：FAIL → 修复 → 重新验证 → 继续（回退范围限制在当前 Task）
9. **使用 Task 工具**：全程跟踪进度，逐任务更新状态
10. **YAGNI（不需要的不做）**：只实现当前明确要求的内容
11. **设计隔离性**：每个模块单一职责、可独立理解和测试

## 反模式："项目太简单不需要设计"

每个项目都要走完整流程，不管多简单。一个 GPIO 翻转示例、一个串口收发、一个配置变更——都不例外。
"简单"项目正是未审视的假设导致最多返工的地方。设计可以很短（外设示例几句话即可），
但必须呈现并获得批准。

## Red Flags（停止并检查）

| 想法 | 现实 |
|------|------|
| "项目简单，不用探索 SDK" | Phase 2 不可跳过 |
| "我记得 BLE 初始化流程" | 必须用 MCP 验证，不凭记忆 |
| "先写代码再补 spec" | spec 必须在 plan 之前 |
| "引脚分配看着没问题" | 必须用 validate_config 验证 |
| "BLE 配置和上次一样" | 每个项目独立验证库/角色匹配 |
| "这几个 Task 一起做更快" | 必须逐 Task 执行和验证 |
| "验证留到最后一起做" | 每个 Task 完成后立即验证 |

---

## Phase 1: 需求收集

**Goal**: 理解需要构建什么，确定项目类型

**Actions**:

1. 创建 Task 列表，追踪所有 6 个阶段进度
2. 采访用户，确认以下信息：
   - **项目名称和路径**（如 `projects/bleHid_Mouse` 或 `examples/uart_dma`）
   - **外设列表**（GPIO/UART/SPI/I2C/ADC/Timer 等）
   - **构建系统**（Keil/GCC/Both），**默认 Both**
3. **确定项目类型**（后续阶段用于条件执行）：
   - `TYPE_BLE`：含 BLE 协议栈的应用（`projects/` 目录）→ 额外采访 BLE 配置（连接数/Profile/Master-Slave 角色）
   - `TYPE_PERIPH`：纯外设示例（`examples/` 目录）
   - `TYPE_MIXED`：BLE + 外设（按 BLE 流程处理）
4. **范围检查**：如果请求描述了多个独立子系统（如"BLE HID + OTA + 多传感器采集"），
   立即标记并帮助用户分解为子项目。每个子项目走独立的 spec → plan → 实施周期。
   - 判断标准：子系统之间可独立运行、测试和部署
   - 分解后先完成核心子项目，再逐步扩展
5. 确认项目路径，检查目录是否已存在（存在则提醒用户）

### 构建系统选项

| 选项 | 说明 | 输出 |
|------|------|------|
| Keil | 仅 Keil MDK | `mdk/*.uvprojx` |
| GCC | 仅 GNU/CMake | `gnu/CMakeLists.txt` |
| **Both** (默认) | 双系统支持 | `mdk/` + `gnu/` |

---

## Phase 2: SDK 探索

**Goal**: 从多个视角理解 SDK 中的相关项目和模式

**Actions**:

1. 并行启动 2-3 个 `b6x-explorer` Agents，**按项目类型使用不同提示词**：

   **TYPE_BLE / TYPE_MIXED**:
   - **similar**: `在 projects/ 和 examples/ 中查找实现 {Profile类型，如 HID/SPP/OTA} 的 BLE 项目，返回 main.c、app.c、cfg.h 的完整路径，重点关注：BLE 初始化流程、Profile 注册方式、连接事件处理模式`
   - **arch**: `分析 ble/app/ 目录架构，追踪 app_init → ble_init → app_prf_create 完整调用链，返回关键函数签名和消息处理器注册方式`
   - **impl** (含外设时): `在 examples/ 中查找使用 {外设名} 的示例，返回驱动头文件路径、关键配置宏和初始化代码片段`

   **TYPE_PERIPH**:
   - **similar**: `在 examples/ 中查找使用 {外设列表} 的示例项目，返回项目路径、main.c 结构和关键初始化代码`
   - **impl**: `分析 drivers/api/{外设}.h 和 drivers/src/{外设}.c，返回完整 API 列表、配置结构体定义和典型使用模式`

2. 每个 Agent **必须返回 5-10 个关键文件列表**，主流程记录这些路径（Phase 5 使用）
3. **阅读所有 Agent 返回的关键文件**，建立深度理解
4. **仅 BLE/Mixed 项目**额外检查：
   - 调用 `/b6-library-recommend` 确认库选择
   - 验证库与角色匹配（参考 `reference.md` 第 1 节）
   - 确认连接数在库限制内（lite=1, 标准=3, 大型=6）
5. 整合发现，生成 `[项目路径]/docs/spec.md`（参考 `reference.md` 第 10 节模板）
6. **spec 自审**（完成后再进入 Phase 3）：
   - [ ] 无 TBD/TODO 占位符
   - [ ] 外设列表与引脚分配无冲突
   - [ ] BLE 库与角色/连接数匹配（仅 BLE 项目）
   - [ ] 功能需求可在一个 plan.md 中实现，否则需拆分子项目
   - [ ] **内部一致性**：spec 各节之间无矛盾（如 BLE 角色与 Profile 支持一致、引脚分配与外设列表一致）
   - [ ] **歧义检查**：每条需求只有一种解读，无"适当处理"/"按需配置"等模糊表述
7. 呈现综合摘要和发现的模式
8. **用户审核规格文件**：请用户审核 spec.md 文件本身，等待确认或修改请求后再继续
   > "spec.md 已生成至 `[项目路径]/docs/spec.md`。请审核并告知是否需要修改，确认后我们将进入需求澄清阶段。"

<HARD-GATE>
spec.md 未通过自审或未获得用户审核确认，不得进入 Phase 3。发现拆分需求时回到 Phase 1 重新界定范围。
</HARD-GATE>

---

## Phase 3: 需求澄清

**Goal**: 在设计前填补真实的歧义和空白

**触发原则**：只在识别到下列真实歧义时提问；若需求已明确，直接列出假设请用户确认后继续。

需关注的歧义点：
- 边界条件：连接数/MTU/广播间隔未指定
- 错误处理策略：断线重连/超时行为未说明
- 性能要求冲突：功耗 vs 响应速度
- 引脚分配存在潜在冲突

**Actions**:

1. 回顾 spec.md，列出所有歧义点，按优先级排序（影响架构 > 影响实现 > 锦上添花）
2. **存在歧义**：每次只问一个最关键的歧义，等用户回答后再问下一个
   - **优先使用选项式提问**：提供 2-4 个具体选项（含推荐），而非开放式提问
   - 示例：问"BLE 库选择哪个？"时提供 lite/标准/大型三个选项及各自适用场景
3. **无歧义**：列出关键假设，请用户一次性确认后继续

如果用户说"你觉得怎样最好"，提供具体建议并获得明确确认。

---

## Phase 4: 方案设计

**Goal**: 设计实现方案，复杂度匹配项目需求

**简单项目**（TYPE_PERIPH、单外设、功能单一）：直接设计单一方案，生成 `plan.md`，跳过多方案比较。

**中等及复杂项目**（TYPE_BLE/TYPE_MIXED、多外设协同、自定义 Profile）：

**Actions**:

1. 并行启动 2-3 个 `b6x-designer` Agents:
   - **minimal**: 基于模板，快速交付
   - **optimal**: 模块化，可维护性优先
   - **balanced** (仅复杂项目): 兼顾速度和质量
2. 每个 Agent 输出 `[项目路径]/docs/plan-[type].md`
3. **分节呈现方案**（按以下顺序逐节呈现，每节获用户确认后继续）：
   - **架构概览**：系统整体结构和模块划分
   - **文件职责映射**：每个文件的 Action 和 Responsibility
   - **关键设计决策**：BLE 库选择、引脚分配、数据流等
   - **方案比较**（多方案时）：复杂度/可维护性/开发时间的对比表
4. **询问用户偏好哪个方案**（多方案时）
5. 将选定方案合并为最终 `plan.md`（参考 `reference.md` 第 11 节模板）

---

## Phase 5: 项目构建 (Implementation)

<HARD-GATE>
必须等待用户明确说"开始"/"批准"/"执行"后才能启动构建。
不得自行推进到实现阶段。plan.md 未经用户确认也禁止开始构建。
</HARD-GATE>

**Goal**: 按 plan.md 逐任务构建项目文件

### Step 1: 加载并审查 plan.md

1. 读取 plan.md，审查每个 Task 的步骤完整性
2. 如有疑虑（缺依赖、步骤不清、文件路径冲突），先与用户确认再开始
3. 为 plan.md 中的每个 Task 创建对应的 TaskCreate 条目

### Step 2: 逐任务执行

对 plan.md 中的每个 Task，严格执行以下循环：

```
1. TaskUpdate → in_progress（标记当前任务开始）
2. 按步骤逐一执行（每个 Step 包含完整代码，禁止占位符）
3. 该 Task 所有步骤完成后 → 运行伴随验证（见下方验证矩阵）
4. 验证通过 → TaskUpdate → completed
5. 验证失败 → 修复 → 重新验证（回退范围限制在当前 Task）
```

**内联 Skill 调用**（Task 完成后即时触发）：

| Task 类型 | 完成后内联调用 |
|-----------|--------------|
| 项目脚手架（目录/基础文件） | Glob 检查文件结构与 plan.md File Structure 一致 |
| BLE 应用（cfg.h/app.c） | `/b6-validate-hardware` 验证引脚与时钟配置 |
| 构建系统（CMakeLists.txt/uvprojx） | `/b6-build` 编译验证（0 Error） |
| 自定义功能模块 | Read 检查接口完整性和集成正确性 |

### Step 3: 遇阻停止

遇到以下情况**立即停止执行**，向用户报告并等待指示：
- 缺少依赖（SDK 中找不到需要的 API 或库）
- 验证反复失败（修复 2 次仍未通过）
- 步骤指令不明确（无法确定唯一解读）
- 发现 plan.md 有遗漏（需要新增 Task 或步骤）

**不要猜测，不要跳过，不要强行推进。**

### 构建检查清单

以下清单在对应 Task 完成后立即检查，而非留到最后：

**通用**（项目脚手架 Task 后）:
- [ ] 目录结构正确（`src/`, `mdk/` 和/或 `gnu/`）
- [ ] 工程文件存在（`*.uvprojx` 和/或 `CMakeLists.txt`）
- [ ] 头文件路径配置正确

**仅 BLE/Mixed 项目** — `main.c`（BLE 应用 Task 后）:
- [ ] `rcc_ble_en()` 在 `sysInit()` 中
- [ ] `app_init(rsn)` 在 `devInit()` 中
- [ ] `ble_schedule()` 在主循环中

**仅 BLE/Mixed 项目** — `cfg.h`（BLE 应用 Task 后）:
- [ ] 正确的库选择宏（`BLE_LITELIB`/`BLE_LARGELIB`）
- [ ] 连接数配置（`BLE_NB_SLAVE`/`BLE_NB_MASTER`）
- [ ] 角色与库匹配（lite 库 `BLE_NB_MASTER` 必须为 0）

**仅 BLE/Mixed 项目** — `app.c`（BLE 应用 Task 后）:
- [ ] `app_prf_create()` 函数存在
- [ ] GAP Service 第一个注册
- [ ] Profile 注册顺序正确（GAP → GATT → 标准 → 自定义）

**构建系统 Task 后**:
- [ ] CMakeLists.txt 源文件列表与实际文件一致
- [ ] 头文件路径覆盖所有 #include
- [ ] 链接脚本类型匹配项目类型
- [ ] 编译通过（0 Error）

---

## Phase 6: 验证与总结 (Validation & Summary)

**Goal**: 全量验证项目完整性并记录完成的工作

**Actions**:

1. 调用 `/b6-project-checklist`（其阶段二自动执行 `/b6-code-review`、`/b6-sram-analyze`、`/b6-validate-hardware`）
2. 交叉验证：核对 plan.md 中所有 Task 和 Step 是否全部 completed
3. **仅 BLE/Mixed 项目**：额外检查：
   - 广播数据长度 ≤ 31 字节
   - `GAPC_DISCONNECT_IND` 处理包含重启广播
4. 结果处理：PASS → 继续；FAIL → 返回对应 Task 修复
5. 标记所有 Task 完成
6. 呈现项目总结：
   - 创建内容：项目名称及功能
   - 关键决策：BLE 库/架构/引脚分配
   - 生成文件：main.c, cfg.h, `*.uvprojx` 和/或 `CMakeLists.txt`

## 终态

Phase 6 完成后，唯一推荐后续动作为 `/b6-build [project]`。
不得在 create-project 流程内直接编译或烧录，保持 skill 职责单一。
