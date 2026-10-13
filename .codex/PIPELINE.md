# 流水线调度说明（PIPELINE）

> 本文档定义 7 角色多 Agent 流水线的运行规则。主会话（你，用户）是最高指挥，
> 通常只需对 manager 下达指令，由 manager 驱动其余角色。
> Codex 只在明确要求时拉起子 Agent——指令里写清"启动 xxx agent"。

## 角色与文档权责表

| 角色 | 职责 | 独占维护的文档 | 沙箱 |
|---|---|---|---|
| manager | 决策问题与优先级、调度、闭环判定（汇总） | goals.md / gap.md / priority.md（公用） | write |
| designer | 算法方案设计 + 最终效果验证（闭环判定人） | algorithm-P*.md（每问题一文件） | write |
| architect | 工程落地方案 | architecture-P*.md（每问题一文件） | write |
| developer | 编码实现 | 算法代码 | write |
| reviewer | 代码审核 + git 提交 | git 历史 | write |
| tester | 客观测试：用例+执行+报告 | test-cases.md（公用）/ 测试代码 / test-report-P*.md（每问题一文件） | write |
| effector | 主观效果测试：场景+demo+报告 | effect-cases.md（公用）/ demo / effect-report-P*.md（每问题一文件） | write |

文档均在项目根目录 `docs/` 下。

## 文档组织规则

| 资产 | 组织方式 | 说明 |
|---|---|---|
| 算法方案文档 | 每问题一个独立文件（`docs/algorithm-P*.md`） | 联合问题可用 `algorithm-P003-P005.md` 形式 |
| 工程设计文档 | 每问题一个独立文件（`docs/architecture-P*.md`） | 同上 |
| 方案复盘文档 | 每问题每轮迭代一个独立文件（`docs/retro-P*-v*.md`） | 记录 (2)→(7) 一轮的方案探索、取舍与结论 |
| 测试报告 | 每问题一个独立文件（`docs/test-report-P*.md` / `docs/effect-report-P*.md`） | 用例描述公用，报告按问题拆分 |
| manager 输出文档 | 所有问题公用一份（`goals.md` / `gap.md` / `priority.md`） | 目标、差距、优先级的全局账本 |
| 测试用例描述文档 | 所有问题公用一份（`test-cases.md` / `effect-cases.md`） | 按问题 ID 分节追加 |
| 工程代码 + 测试用例代码 | 全部纳入 git 管理 | reviewer 提交时必须包含产品代码与测试代码，不得遗漏或忽略 |

**重要**：本项目已有既有文档 `docs/PROJECT_GOALS.md`（流沙画 App 工程目标 v1.0）与
`docs/DESIGN_GAP_CLOSE.md`（验证体系 gap 关闭设计 v1.0）。manager 首次启用流水线时，
应以 PROJECT_GOALS.md 的场景一/场景二为 goals 的事实来源（可引用而不重复抄写），
DESIGN_GAP_CLOSE.md 的 B1~B4 批次可视为已排好优先级的 gap 条目，纳入 priority.md 管理。

## 闭环流程（一个问题的生命周期）

```
(1) manager   读 goals + 最新双测试报告 → 更新 gap.md / priority.md，选出本轮要解决的问题
              测试先行：先确认存在可验证该问题的客观/主观用例，缺失则先路由 tester/effector 补齐
              再按“问题路由规则”路由：
                算法/功能实现类   → (2) designer
                测试用例类（客观） → (6) tester
                测试用例类（主观） → (6) effector
(2) designer  针对问题设计算法方案 → algorithm*.md（含量化验收指标）
(3) architect 根据问题 + algorithm 方案设计开发方案 → architecture*.md
(4) developer 按 algorithm + architecture 开发 → 算法代码
(5) reviewer  审核代码一致性 → 通过则 git commit；不通过退回 (4)
(6) tester    设计用例并执行 → test-report-P*.md   ┐ 并行
    effector  设计场景并执行 → effect-report-P*.md  ┘
(7) designer  对照双报告验证验收指标：
      达成   → 标记 VALIDATED → 问题闭环，manager 在 priority.md 标 CLOSED
      未达成 → 先归因，再分流：
        算法方案问题 → 回到 (2) designer 重新设计算法（v2/v3），manager 同步调整优先级
        实现方案问题 → 算法与测试用例均无问题时 → 回到 (3) architect 修订实现方案
        用例问题 → designer 质疑测试用例正确性并上报 manager 决策：
                    manager 认为测试不合理 → 上报用户，由用户决策是否修改测试用例
                    用例修改通过决策后 → tester/effector 立即用修改后的用例重测，
                    并输出该问题的最新 test-report-P*.md / effect-report-P*.md
      红线     → 任何角色绝对不允许为了通过测试而私自修改测试用例或放宽判定标准
(8) manager   持续验证 gap、调整优先级，保证项目朝目标前进 → 回到 (1)
```

测试用例类问题从 (6) 进入闭环：tester/effector 产出用例与判定资产后，由 manager 对照 priority.md 的验收要求验收；不满足退回对应角色补充，满足则在 priority.md 标 CLOSED。

每轮从 (2) 设计算法方案到 (7) 判定达成/方案有问题，designer 必须沉淀一份独立复盘文档
`docs/retro-P*-v*.md`（如 `retro-P003-P005-v1.md`）：记录探索过的多种候选方案、选型理由、
被否方案的原因、实施与测试中暴露的问题、下一版改进方向。designer 应积极探索多种方案，不得只写单一方案。

## 问题路由规则（manager 决策后）

manager 在 `priority.md` 选出本轮要解决的问题后，按问题类型路由到下一角色。

**测试先行原则**：manager 决定要解决某个问题时，必须先保证存在能验证“该问题是否已解决”的主观或客观测试用例；若用例缺失，先把补齐用例作为前置事项路由给 tester/effector，用例就位后再进入实现，不得先实现后补测。

| 问题类型 | 下一角色 | 路径 |
|---|---|---|
| 算法/功能实现类（物理模型、渲染、交互、性能等） | designer | 走完整闭环 (2)→(8) |
| 测试用例与判定实现类——客观（用例、脚本、断言缺失） | tester | 直接补 `test-cases.md` 与测试代码，就位后进入 (6) |
| 测试用例与判定实现类——主观（场景、demo、评分标准缺失） | effector | 直接补 `effect-cases.md` 与 demo，就位后进入 (6) |

- 同轮同时存在实现类与测试类问题时，manager 应在 `priority.md` 为每个问题标注路由与依赖（如“P-003 → designer，P-005 → tester”），测试类问题不经过 designer/architect/developer。
- tester 与 effector 可并行（纪律 5）；它们与 designer/developer 等写同一批文件的角色必须串行。
- **基线测试**：测试用例补齐/修改后，tester/effector 必须立即用最新用例执行一次基线测试，并输出该问题的 test-report-P*.md / effect-report-P*.md；功能未实现时如实记录 FAIL/NOT_IMPLEMENTED（RED 基线），作为改动前证据，不得放宽判定或推迟执行。基线阶段因“问题尚未解决”产生的 FAIL/BLOCKED 属可预见结果，不需要 designer 分析，也不触发第 (7) 步的归因闭环；仅当基线暴露用例/脚本自身缺陷时才反馈修正。

## 调度指令示例

启动一轮完整流水线（新问题）：
> 启动 manager agent：读取 docs/goals.md 和最新的 test-report-P*.md / effect-report-P*.md，更新 gap 和优先级，选出本轮要解决的问题，然后驱动 designer 开始设计。

测试阶段并行（第 6 步）：
> 同时启动 tester 和 effector 两个 agent，分别对问题 P-001 的当前 commit 做客观测试和主观效果测试，等两边都完成后汇总。

闭环验证（第 7 步）：
> 启动 designer agent：对照该问题的 docs/test-report-P*.md 和 docs/effect-report-P*.md 最新报告，验证 P-001 方案 v2 的验收指标是否达成。

测试用例类问题路由：
> 启动 tester agent：针对 P-005 补充客观测试用例描述与判定脚本实现，覆盖 priority.md 验收要求，完成后输出 test-report 供 designer 验证。

单角色检查：
> 启动 reviewer agent：审查最近一次 developer 的改动是否与 algorithm.md v2 一致。

## 纪律

1. 每个子 Agent 只维护自己的文档，不越权改别人的账本
2. 文档变更必须带日期 + 原因，追加不覆盖（algorithm/test/effect 报告尤其如此）
3. 交接以文档为准：下游角色只认上游文档的最新版本，不依赖对话记忆
4. 冲突升级：任何角色发现上游文档有矛盾，报告 manager，不自行其是
5. 并行只用于 tester+effector 或多问题互不相关的场景；写同一批文件的任务必须串行
6. 测试先行：manager 选定要解决的问题必须先有可验证的客观/主观用例；用例缺失时先补用例再实现
7. 测试争议：测试未达成时 designer 可质疑测试用例正确性，异议上报 manager 决策；manager 认为测试不合理时上报用户决策。绝对不允许任何角色为了通过测试而私自修改测试用例或放宽标准

## 变更记录

- 2026-10-13：新增“问题路由规则”：算法实现类问题交 designer；测试用例补充/实现类问题按客观/主观分别交 tester 或 effector，不再统一走 designer。
- 2026-10-13：同步更新“闭环流程”：第 (1) 步加入路由分支，测试用例类问题直接从 (6) 进入闭环；文档引用改为按问题独立的 algorithm*.md / architecture*.md。
- 2026-10-13：新增“测试先行”原则：manager 选定问题须先确保存在可验证用例，缺失时先由 tester/effector 补齐再进入实现。
- 2026-10-13：新增测试争议机制：测试未达成时 designer 可质疑用例正确性，经 manager 复核、用户决策后方可修改；严禁为通过测试私自改用例。
- 2026-10-13：第 (7) 步测试失败改为归因分流：方案问题回 (3) 重设计；开发问题（实现与方案不符）退回 (4) developer 修复并经 reviewer 复审；用例问题走质疑→manager→用户决策链。
- 2026-10-13：修正第 (7) 步归因口径：算法方案问题回 (2) designer 重设计；算法与测试均无问题时属实现方案问题，回 (3) architect；用例问题走质疑→manager→用户决策链。
- 2026-10-13：新增文档组织规则：算法方案/工程设计/测试报告每问题一文件；manager 账本与测试用例描述公用一份；工程代码与测试代码全部纳入 git 管理。
- 2026-10-13：第 (7) 步用例问题分支补充：用例经决策修改后，tester/effector 必须立即用新用例重测并输出最新测试报告，再回到验证闭环。
- 2026-10-13：问题路由规则新增基线测试要求：用例补齐/修改后必须立即执行基线测试并输出报告，功能未实现时如实记录 RED 基线。
- 2026-10-13：基线测试规则补充：因问题尚未解决产生的基线失败属可预见结果，不由 designer 分析、不触发第 (7) 步归因闭环；仅用例/脚本自身缺陷需反馈修正。
- 2026-10-13：新增方案复盘规则：designer 须积极探索多种方案；每轮 (2)→(7) 沉淀独立复盘文档 docs/retro-P*-v*.md。
