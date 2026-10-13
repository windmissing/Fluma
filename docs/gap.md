# Gap 清单（目标 vs 现状）

> 维护人：manager。数据来源：docs/test-report.md（客观）、docs/effect-report.md（主观）。
> 已闭环的问题移出本清单，历史记录见 git。

| 问题 ID | 描述 | 来源（哪份报告的哪项发现） | 影响的目标 | 状态 |
|---|---|---|---|---|
| P-003 | B3：WaterSandField 三相模型、第四渲染模式、面板与 Node 单测缺失；当前算法 v1 §3.3 存在分相守恒缺陷，developer 暂缓实现 | `docs/DESIGN_GAP_CLOSE.md` §四；`docs/architecture-P003-P005.md` §0.1；两份最新报告均无有效验收数据 | G2、G3 | IN_PROGRESS（BLOCKED） |
| P-005 | 场景二测试用例与判定脚本缺口：`docs/test-cases.md`、`docs/effect-cases.md` 为占位；仓库无 WaterSandField/三相用例；`test/vv_s75_analysis.py` 等仅支持两相；无 VV-07/08/09、三相守恒、分选比、沉降时序、安息角、水面与主观观感用例；客观/主观用例可直接由 tester/effector 并行补齐 | `docs/test-cases.md`、`docs/effect-cases.md`、`test/TC_TEST_CASES.md`、`test/vv_s75_analysis.py`、`entry/src/ohosTest/ets/test/SandGridTest.ets` | G2、G3 | IN_PROGRESS |
| P-001 | B1/M1‘：场景一 SandGrid 液体规则已实现（s79），但 `docs/test-report.md` 与 `docs/effect-report.md` 仍无有效数据，未完成客观/主观闭环 | `docs/DESIGN_GAP_CLOSE.md` §二 v1.1；`docs/test-report.md` 报告 #1 无数据；`docs/effect-report.md` 报告 #1 无数据 | G1、G3 | OPEN |
| P-006 | 场景一测试用例与判定脚本缺口：`test/TC_TEST_CASES.md` 已有 TC-11~13 草案，但未进入 `docs/test-cases.md`；`docs/effect-cases.md` 为占位；无双口径守恒、液滴归类、恢复时限与主观观感的可执行用例与报告 | `docs/test-cases.md`、`docs/effect-cases.md`、`test/TC_TEST_CASES.md`、`test/vv_s75_analysis.py` | G1、G3 | OPEN |
| P-002 | B2/M3：倒立渲染 `want` 直启路径已 PASS，面板路径与完整回归仍未闭环 | `test/reports/TC-09..10_s77r_independent_verify_report.md`；`docs/test-report.md`、`docs/effect-report.md` 均无有效数据 | G1、G3 | OPEN |
| P-004 | B4：三相判定脚本扩展、全场景动态回归矩阵、TC/SV/VV 报告缺失 | `docs/DESIGN_GAP_CLOSE.md` §五；两份最新报告均无有效验收数据 | G1、G2、G3 | OPEN |

> 现状说明：根据用户最新指令，当前推进顺序调整为 **B3 → B1 → B2 → B4**。场景二相关 gap（B3/P-003）优先，场景一相关 gap（B1/P-001、B2/P-002）次之，跨场景回归（B4/P-004）最后。测试用例缺口作为独立条目管理：P-005 对应场景二，P-006 对应场景一。当前客观与主观报告均为空，因此所有问题仍无法判定闭环。

> 2026-10-13 阻塞项：`docs/algorithm-P003-P005.md` §3.3 的 `hl[c] -= q; hs[c] += q` 会破坏重沙/轻沙分相守恒，违反 P-003 的最高优先级不变量。architect 已在 `docs/architecture-P003-P005.md` §0.1 标注，developer 不得实现该公式；需 designer 先修订算法，再由 architect 同步工程方案。P-005 不因此阻塞：tester/effector 可先编写用例，并把三相守恒作为硬断言，用于暴露该类公式错误。

## 变更记录

- 2026-10-13：初始创建。
- 2026-10-13：原因：启动 gap 管理。将 `docs/DESIGN_GAP_CLOSE.md` 的 B1~B4 纳入 P-001~P-004；结合 `test-report.md` 与 `effect-report.md` 当前无有效数据的事实，全部保持 OPEN。
- 2026-10-13：原因：用户最新指令调整场景推进顺序。将 gap 表与现状说明同步为“场景二（B3）优先，场景一（B1/B2）次之，B4 最后”。
- 2026-10-13：原因：用户补充测试用例也属于 gap。新增 P-005（场景二测试缺口）与 P-006（场景一测试缺口），并同步修正 P-001/P-002 描述以对齐 `DESIGN_GAP_CLOSE.md` v1.1 与 s77r 复核结果。
- 2026-10-13：原因：architect 发现 algorithm v1 §3.3 渗透公式破坏分相守恒。将 P-003/P-005 标记为 `IN_PROGRESS（BLOCKED）`，并记录 developer 暂缓实现，等待 designer 修订算法。
- 2026-10-13：原因：用户更新 `.codex/PIPELINE.md` 问题路由规则。P-003 继续走 designer→architect→developer 完整闭环；P-005 改为 tester（客观）+ effector（主观）并行直补，不再经过 designer/architect/developer，也不被 P-003 算法修订阻塞。
