# 优先级队列（priority）

> 维护人：manager。一轮聚焦最多 1~3 个高优先级问题。

## 当前优先级

| 优先级 | 问题 ID | 状态 | 路由 | 依赖 / 前置 | 备注 |
|---|---|---|---|---|---|
| P0 | P-003 | IN_PROGRESS（BLOCKED） | designer → architect → developer | **P-005 用例就位**；algorithm §3.3 修订 | B3，场景二三相模型；测试先行，未有用例前不进入 designer 修订 |
| P0 | P-005 | IN_PROGRESS | tester + effector 并行 | 无，可立即启动 | 场景二测试用例与判定脚本缺口；客观交 tester，主观交 effector；是 P-003 的前置 |
| P1 | P-001 | OPEN | designer → architect → developer | P-006 用例就位 | B1/M1’，场景一核心能力已实现但未闭环；验证前需补齐场景一用例 |
| P1 | P-006 | OPEN | tester + effector 并行 | 无，待本轮调度 | 场景一测试用例与判定脚本缺口；是 P-001 的验证前置 |
| P2 | P-002 | OPEN | designer → architect → developer | 相关客观/主观用例就位 | B2/M3，倒立渲染 want 路径已 PASS，面板路径与完整回归未闭环 |
| P2 | P-004 | OPEN | tester + effector 并行 | B3 与 B1/B2 完成后推进 | B4，全量回归与判定体系；依赖前置实现与用例齐备 |

状态说明：OPEN → IN_PROGRESS（designer 已开工）→ VALIDATING（测试中）→ CLOSED（双测试达标，designer 验证通过）

## 当前阻塞

**阻塞项**：`docs/algorithm-P003-P005.md` §3.3 的渗透公式 `hl[c] -= q; hs[c] += q` 会把轻沙转换为重沙，导致 `VH` 增加、`VL` 减少，只保持 `VH + VL` 不变，无法满足三相各自守恒。

**影响与路由**：

1. P-003 不能进入开发实现；developer 不得实现该公式。
2. P-005 不被该阻塞项卡住：tester/effector 可先补客观/主观用例，并把三相守恒写成硬断言，用于暴露此类公式错误。
3. P-003 保持 `IN_PROGRESS（BLOCKED）`；P-005 保持 `IN_PROGRESS`，按新路由分别交 tester 与 effector。
4. **测试先行**：P-003 在 `docs/test-cases.md` / `docs/effect-cases.md` 仍为占位的情况下，不得先交 designer 修订；必须等 P-005 的客观/主观用例就位后再进入实现闭环。

**下一步**：

1. P-005 → tester + effector：立即并行补客观/主观用例，不经过 designer/architect/developer；客观用例必须包含三相守恒断言。
2. P-003 → designer：待 P-005 用例就位后，修订 `docs/algorithm-P003-P005.md` §3.3，给出保相守恒的渗透/分选表达，或明确 v1 禁用该子机制并改由沉降顺序与表层滑移实现分选。
3. P-003 后续仍需 architect 更新工程方案；developer 待 designer 修订且 architect 确认后才可开工。

## 本轮选定

**问题 ID：P-003 + P-005（分路推进）**

### 选择理由

- 用户最新指令明确“场景二（水中两种重量的沙，堆积成形）优先，其次是场景一（两种不互溶液体油水分层）”，因此 B3/P-003 提升为当前最高优先级。
- P-003 是 G2 的最小可验证核心，直接覆盖三相守恒、重沙/轻沙分选、安息角、摇晃悬浮与再堆积等场景二关键目标。
- P-005 是 P-003 的验证前置：当前 `docs/test-cases.md`、`docs/effect-cases.md` 均为占位，仓库中也没有 WaterSandField/三相判定脚本；不补齐测试用例，P-003 无法客观/主观闭环。
- 当前 `test-report.md` 与 `effect-report.md` 均无有效数据，优先完成 P-003 + P-005 可先建立场景二的客观守恒、堆积/分选与主观观感基线，再反哺场景一的倒立与渲染验收。
- 修复成本相对可控：P-003 主要新增 WaterSandField 列标量三相模型、第四渲染模式与 Node 单测；P-005 可复用现有两相脚本框架扩展为三相与主观用例。

### 验收要求

1. **P-003 客观验收**：多轮旋转、倒立与摇晃角序列下，water、Σhs、Σhl 各自总量守恒，漂移 < 0.1%；susp ↔ 沙堆只做相内转移，不改变各相总量。
2. **P-003 分选与堆积**：静置后重沙在沙堆上 1/3 的占比 / 下 1/3 的占比 ≤ 0.5（初值）；沙堆轮廓平滑、坡度不超过安息角上限；长时静置零漂移。
3. **P-003 动态表现**：重沙沉降快于轻沙（vH > vL）；摇晃时沙堆表层按比例扬起为悬浮态，E 衰减后重新沉降并堆积；全过程连续平滑，无粒子瞬移，实测帧率 ≥ 100fps。
4. **P-005 客观测试用例**：产出可执行用例，覆盖三相守恒、分选比、沉降时序、安息角、水面水平/倾斜、倒立相序、摇晃→悬浮→再堆积、长时静置零漂移与 FPS；实现或扩展判定脚本以支持 VV-07/08/09 与三相像素/粒子分类。
5. **P-005 主观测试用例**：产出可复现的主观用例，覆盖水面观感、沙丘轮廓、重沙/轻沙分区、混合带、摇晃悬浮与再堆积自然度、卡顿/闪烁/瞬移等缺陷；评分标准需可量化。
6. **闭环条件**：P-003 与 P-005 均需 tester 的客观测试与 effector 的主观效果测试达标；任一不达标则保持 OPEN，不得仅凭实现完成或用例编写完成而关闭。

### 路由与交接说明

#### P-003 → designer

- **前置条件**：P-005 的客观/主观用例已就位，能验证三相守恒、分选、堆积、沉降、水面、倒立与主观观感。
- **输入文档**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四（B3）、`docs/algorithm-P003-P005.md` v1、`docs/architecture-P003-P005.md` §0.1、补齐后的 `docs/test-cases.md`、`docs/effect-cases.md`。
- **期望产出**：修订 `docs/algorithm-P003-P005.md` §3.3，保持 `VH = Σ(hs + suspH)` 与 `VL = Σ(hl + suspL)` 各自守恒；如 v1 无法给出保相渗透公式，应明确禁用该子机制并说明替代分选机制，同时在变更记录中写明原因。
- **必须包含**：不改变其他章节的修订说明；替换公式或明确的禁用策略；对测试钩子的影响；以及 developer 可据此实现的伪代码。
- **后续依赖**：designer 修订完成并经 architect 确认后，developer 才可按更新后的 `docs/architecture-P003-P005.md` 开工。

#### P-005 → tester + effector

- **tester 输入**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四/§五、`docs/priority.md`、`docs/test-cases.md`、`test/TC_TEST_CASES.md`、`test/vv_s75_analysis.py`。
- **tester 期望产出**：补齐 `docs/test-cases.md` 中 P-005 的客观用例与可执行判定脚本，覆盖三相守恒、分选比、沉降时序、安息角、水面、倒立、摇晃→悬浮→再堆积、长时静置与 FPS。
- **effector 输入**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四/§五、`docs/priority.md`、`docs/effect-cases.md`。
- **effector 期望产出**：补齐 `docs/effect-cases.md` 中 P-005 的主观场景与评分标准，覆盖水面观感、沙丘轮廓、重沙/轻沙分区、混合带、摇晃悬浮与再堆积自然度、卡顿/闪烁/瞬移等缺陷。
- **并行规则**：tester 与 effector 可并行；二者不修改 algorithm/architecture/developer 代码，不等待 P-003 修复。
- **测试先行**：P-005 是 P-003 的前置；用例未就位前，P-003 不进入 designer 修订，developer 更不得开工。

## 已闭环

（空）

## 变更记录

- 2026-10-13：初始创建。
- 2026-10-13：原因：启动第一轮调度。按 `docs/DESIGN_GAP_CLOSE.md` 的 B1→B4 顺序建立优先级；因两份最新报告均无有效数据且 P-001 是 G1 的前置核心，本轮仅选定 P-001 交由 designer 输出算法方案。
- 2026-10-13：原因：用户最新指令调整场景推进顺序。将 B3/P-003 提升为当前最高优先级并选定为本轮问题，B1/P-001 与 B2/P-002 降为次优先级，B4/P-004 保持最后；同步更新选择理由、验收要求与 Designer 交接说明。
- 2026-10-13：原因：用户补充测试用例也属于 gap 管理范围。新增 P-005/P-006 独立测试缺口，本轮选定 P-003 + P-005，并要求算法方案提供可测试钩子以支撑 tester/effector 后续落地用例。
- 2026-10-13：原因：architect 发现 algorithm v1 §3.3 渗透公式破坏分相守恒。登记当前阻塞项，将 P-003/P-005 标记为 `IN_PROGRESS（BLOCKED）`，并明确 developer 需等待 designer 修订与 architect 确认后开工。
- 2026-10-13：原因：用户更新 `.codex/PIPELINE.md` 问题路由规则。为每个问题标注路由：P-003 走 designer→architect→developer 完整闭环；P-005 由 tester+effector 并行直补，客观/主观用例不被算法修订阻塞。
- 2026-10-13：原因：用户新增“测试先行”原则。将 P-005 明确为 P-003 的前置依赖；在客观/主观用例就位前，P-003 不进入 designer 修订或开发实现。
