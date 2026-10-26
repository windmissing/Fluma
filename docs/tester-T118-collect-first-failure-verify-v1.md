# T118：首错误诊断独立验证（三类99、分片身份与副作用对照）v1

日期：2026-10-26。角色：tester。任务卡 `6adf3c1ce4b0127832c15f8f`。被验对象：T117 `2c024c8`（P011_COLLECT_FIRST_FAILURE_V1）。执行器：`test/p118_t117_verify.cjs`（**6/6 PASS**，独立构造可达状态，不镜像开发 4/4）；raw：`test/reports/t118/p118_results.json`。T103 分片重组独立完成（脚本见报告§3，中间产物 `.codex/local-test/t118_c1c2.pkl`）。不使用真机；不改生产/阈值。

## 1. T117 三类99独立验证（每类用独立可达状态构造）

| case | 构造 | 断言 | 结果 |
|---|---|---|---|
| V1 类别1：真实 M2 入口 BUSY | 同 binding 二次 `beginCollect`（stepOpen=1 残留） | code=1（BUSY）；`collectFailure.phase='ENTRY'`；**entryStepOpen=1 ∧ entryActive=1 残留被记录**；rollback 后再次进入成功 | PASS |
| V2 类别2：BEGIN_HOT 原码折叠 | 预置 `work.pageBacklog=1` | code=3（STEP_FAILED 非 99）；`phase='BEGIN_HOT'`；Hot 原码非零；首谓词=`PAGE_BACKLOG`；exitActive=0 | PASS |
| V3 类别3：collect 成功后 audit drift 99 | `sourceInjectedHeavy+=4` 使 drift 超 DRIFT_LIMIT | `audit(true)`=UNKNOWN(99)；getDrift 三分项+limit 可读；beginCollect 从未失败——**99 与 beginCollect 失败可区分** | PASS |

**结论**：三类99在真实接口上均可独立触发并携带可区分证据，T117 的折叠修复有效。设备 COLLECT99 首因仍须设备侧 firstFailure.origin/rawCode 判定，本卡不归因。

## 2. 诊断开关副作用与首错误冻结

| case | 断言 | 结果 |
|---|---|---|
| V4 开关场等价 | 开/关诊断同输入 3 步：step 输出逐值 deepEqual、场快照逐值 deepEqual、seedCalls 相等、commit 单调、digest 8 位存在 | PASS |
| V5 首错误冻结 | 首次 `recordFirstFailure` 写定后，第二次失败路径不覆盖（JSON 逐字段一致） | PASS |
| V6 cleanup 记录 | trace 携带 cleanupBudgetEpoch/cleanupPageBacklog；步进后 binding 干净（active=0、pageBacklog=0） | PASS |

## 3. T103 C1/C2 分片身份重组（独立重解析，保留原报告）

从 `test/artifacts/t103-first-code5-capture-20261026/{C1,C2}-full.txt` 逐行解析 `WS_EVENT first_code5_trace` 信封并按 part 重组：

| 项 | C1 | C2 |
|---|---|---|
| 信封分片 | 3/3，唯一、连续 | 48/48，唯一、连续 |
| 原始 bytes / payload chars | 2,045,828 / **6,105** | 2,305,123 / **97,121** |
| runId | `p011-run-1793012705902-3`（≈19:05:05.902 起） | `p011-run-1793012755607-3`（≈19:05:55.607 起） |
| PID | 40925 | 41622 |
| JSON 解析 | OK（complete=true, dropped=0） | OK（complete=true, dropped=0） |
| traces | 仅 a675/substep675，REQUESTED=42，resultStatus=**blocked** | a563（REQUESTED42→clamped 成功，CLAMPED30 acceptedComparison=true）+ a564（REQUESTED42→**blocked**） |
| commit | before=after=682 | before=567 → after=568 |
| 每条 trace 内部 dt | 1/60 | 1/60（外层 tick dt=1/30，一次 tick 两子步） |
| blocked 候选 | callOrdinal1 REQUESTED θ=42 stage=COLLECT collectCode=99 collectColumn=-1 | 同左（a564） |
| beforePool | live=36/contact=0/airborne=36/**copyStatus=LAST_PRESENTED** | live=21/contact/airborne/copyStatus=LAST_PRESENTED |
| beforePoolProgress | run 同、attempt **a674**/substep 674、firstError=0、rollbackCount=0 | attempt **a562**/substep 562、firstError=0、rollbackCount=0 |

**分片身份结论**：
1. C1 是 6,105 字符小载荷，**不是 97KB**；C2 的两条 trace 同属**一个** run，"reset 产生第二个 run"无分片证据支持——与 T109 §1 一致，本轮独立复算确认。
2. C1/C2 是不同 run（两次独立触发）；C2 内部 a563 成功、a564 失败——**同 tick 双子步前成功后失败**，beforeCommit 567→after 568 的 +1 来自成功子步，不能标"整个 tick 逐值回滚"。
3. **LAST_PRESENTED 边界确认**：两臂 beforePool.copyStatus 均为 LAST_PRESENTED，且 poolProgress 的 attempt（a674/a562）落后于失败 trace（a675/a564）——beforePool 不是紧邻失败 attempt 的权威池快照，作归因输入时须标注该边界。
4. C2 a563 成功候选（REQUESTED42 与 CLAMPED30 均 PLANS_READY，feasible=true、thetaClamped≈30）证明同 run 前一子步计划链完整——COLLECT99 发生在 a564 的 REQUESTED 候选，与前次成功共享同一 42° 请求路径。
5. poolProgress firstError=0/rollbackCount=0 只是摘要层，不排除粒子/预算/审计分支失败（T109 §3 已定，本轮以原始分片佐证）。

## 4. 边界

- 未把诊断通过当业务修复；设备 code5 首因未裁定，T119 无本卡阻断。
- 未重复 T109 全文归因，仅独立重组分片身份与边界字段；原始 T103 报告与分片文件零改动。
- V3 断言未伪造 audit 路径的 firstFailure（audit 分支的 recordCollectFirstFailure 依赖 pending trace 场景，本卡如实记录 null），设备侧以实际导出为准。
