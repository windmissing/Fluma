# T523 · GPU M3 V4 产品路径与时间 gate 独立差分 v1

日期：2026-11-16（tester）
角色：tester（不使用真机）
被测：T522-delivery（developer-T522-v4-implementation.md）
HAP SHA256：`70d2742c6a76d11bc6aef5450a3e8e68df6c6a513ef7b9969cec54aadb49c3d7`（与卡面逐字核验一致）
前置确认：T522 已交签包（checks.json verifyApp=PASS），本卡解锁。

## 方法

独立复跑（不信任 developer 断言，全部断言由 harness 内 assert 承载、本卡独立执行 exe）：

1. **产品树 shader 一致性**：T522-delivery 的 m3a shader 与产品树 `entry/src/main/cpp/gpu_resident/m3a/` 逐文件 SHA256 对照——occupied_solvers / water_event / water_writer / source_seed / source_trial / inventory_audit 全部 **SAME**（离线 harness 与产品 shader 无漂移）。
2. **T499 oracle 输入冻结先于运行**：fixtures 由 `test/t522/solver_cases.py` 从 T499 oracle 生成（`.codex/local-test/t522-solver/*.bin` + expected.json），expected 先冻结、GPU 断言只对 frozen bin。

## 独立复跑结果

| 域 | 复跑 | 结果 |
|---|---|---|
| solver GPU（λ/η/water 11 项） | t522-solver.exe | **11/11 PASS**（uniform/floor/zero/bound/large_u64/truncation_contract + level_odd/pair/tilt/reverse/zero_rate） |
| SOURCE72 GPU 全场景 | t522-source.exe ×12 | **12/12 PASS**（bulk/retry/sand/water-direct/water-flight/corrupt-replay/zero-source/zero-max-physics/seed-resolution/root/root-offset/root-clamped/root-full） |
| full549 五角色 QUANTUM | root 4 档 | **4/4 PASS**（REQUESTED/offset7/CLAMPED/FULL_BRANCH：authority-latch/consistency/seal） |
| BQ01–BQ21 控制域 | t522-blocked.exe | **19/19 PASS 行**（含 BQ04 local-only 如实标注） |
| native production controller | t522-native-gl.exe | SOURCE72 接受 3192B（planned==observed==3192）；V4 冷快照 REQUESTED→TRACE_READY→page（无 INCOMPLETE）；QUANTUM **DEADLINE/GAP 拒**：fault=9、SOURCE 保留、quota[32]=0、无 Q 接受——软件后端保护行为，不冒性能 PASS |
| 四档 allocations | t522-allocations.exe | 192/64=61,257,552；192/128=65,229,392；512/64=129,767,504；512/128=133,739,344，padding=0 全 PASS |
| seed GPU | t522-seed.exe | 共享 origin/升序 seed/连续 ID：births=3、born=244667 PASS |

## 四项 NOT_IMPLEMENTED（T499）对应产品结果补证

| T499 项 | T522 产品结果（本卡复跑确认） | 判定 |
|---|---|---|
| M3A01 旧 reader/拒混用 | BQ20：既有 V3 cold reader 在解释前拒 V4 completion PASS；seed-resolution rejection/rollback PASS | **已补证（离线域）** |
| M3A04 seed 顺序/连续 ID | seed GPU：共享 origin、升序 seed、连续 ID births=3 born=244667 PASS；NO_PAIR/birth limit 走 m3SeedFailure | **已补证（离线域）** |
| M3A09 保前缀 | root 4 档全 PASS（成功前缀/seal）；corrupt-replay pass=8 篡改 trial 事件 error=31 在私有晋升前拒（SPARE 独立重放） | **已补证（离线域）** |
| M3A10 四档资源/真实设备期限 | 四档 allocations GL PASS（数字如上）；**真实设备期限 NOT_RUN**（本卡不使用真机） | **部分补证；设备域 NOT_RUN 回 architect** |

## λ 保守门分歧（T533 单列，本卡不复判）

`truncation_contract` 场景：产品 shader `m3TrialEta`（occupied_solvers.glsl:73-74）真实存在 T497 §4.5 门——`BASE_ETA[c]+eHi>CAP[c] → return 4u`，GPU 实测输出 status=4（ETA_TRUNCATION_RETRY）PASS。T499 oracle `solve_lambda` 无此门返回 OK/eHi=1。**分歧确认存在**，按卡面 T533 首次归因，本卡如实记录、不修改 tester 资产、不裁决。

## T487 保留 vx / 非零 RNE / 大乘积边界支持域检查

V4 merge 走 `m2MergeRows`（merge_primitives.glsl:57）：x..vy 全字段 `m2WeightedRNE`（一次有理 RNE）。S5/S6/S7 的 z 域=slot3（vx），本卡独立复算冻结期望：

| 例 | mA*zA+mB*zB / den | z 独立复算 | 冻结期望 |
|---|---|---|---|
| S5a | 122333/244666 | 0 | 0 ✓ |
| S5b | 366999/244666 | 2 | 2 ✓ |
| S5c | −366999/244666 | −2 | −2 ✓ |
| S5d | −611665/244666 | −2 | −2 ✓ |
| S6a | 0（精确抵消无负零） | 0 | 0 ✓ |
| S6b | 262708116988451/244666（<2^48 边界内） | 1073741824 | 1073741824 ✓ |
| S7 | 122333/131072（不等质量非整数中点） | 1 | 1 ✓ |

S6b 大乘积 262,708,116,988,451 < 2^48=281,474,976,710,656 边界成立。RNE 公式（round-half-even）与 m2WeightedRNE 语义逐点一致——**7/7 独立支持域检查 PASS**（数学域；vx 域 GPU 独立注入通道在 SOURCE72 输入 schema 中未见独立 vx patch 位，种子速度由 settle×sin/cos 派生，vx 独立注入仍无通道，维持 T487 RNE 项记录）。

## 时间 gate / 双守恒判定

- blocked 时间双守恒：BQ01–BQ21 全 PASS（blockedActive/Pending/ClearedCarry/Wait 分账 Balanced() 断言全链）。
- seed 与 blocked 时间：quota256 布局、GRS_M3_PROGRESS_CLOCK_V3 B/BC U128 时间账本、BLOCKED 再入/同锁退休/旧尾拒（native-guard/gap-observation 复跑：pulse=5 fault=7 与 developer 记录一致）。
- **软件后端 CLOCK_GAP/DEADLINE 不冒产品通过**（卡面明确）：native quantum result=6/fault=9 为保护行为如实记录，设备期限判定归 M3A10/真机域。

## NOT_RUN / 回 architect

- 真实设备期限（M3A10 后半）、真机 RSS/帧域——本卡不使用真机，缺设施精确回 architect。
- 设计 oracle 不当设备用：全部 PASS 限定"离线 ANGLE fixture + 真实产品 GLSL/production controller"域，不声称端到端/真机。

## 结论

**T523 v1：独立差分 PASS（离线域全绿 8 域 + 四项 NOT_IMPLEMENTED 三项补证一项部分）**。
分歧（λ 保守门 T499 oracle）如实单列 T533；vx 独立注入通道缺位维持 T487 记录；设备域 NOT_RUN 回 architect。

复跑原始日志：`.codex/local-test/t523-*.log`（solver 11 项/source 12 场景/blocked 19 行/native/cold/allocations/seed/contracts/control）。
