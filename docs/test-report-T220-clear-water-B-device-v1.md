# T220 明水 B 真机功能与事务成本报告

日期：2026-10-28。任务：6ae19c8ae4b0c1a6bd5d72dd（tester，真机 VYG-AL00 62T0225B10010020，目标7/P3 设备串行让位）。前置：T209 4bdf050 + T215 12/12 无阻断。

## 隔离 harness（test-only，生产零改动）

- 基于 V3 参考工程结构生成 `.codex/local-test/t220-harness`：简化 EntryAbility/Index + `T220PageLauncher`（Want 参数 t220runId/t220scenario/t220k）+ `T220Worker.ets`（clearwater B 全套，显式 attachBInto）。
- **生产 entry/pages/页面零改动**；worker 事件边界与 ArkTS 严格模式（implements 接口/无 untyped literal/显式泛型）经 4 轮编译迭代通过。
- 签包：unsigned 43a26a5c（主树 T220 时点）→ harness unsigned → **signed 6232046caea bb0e6a…（6232046c…）**，buildTag=T220-W2-SCENARIO-FIX-1530 绑定。
- 主工程 s139h 测试后恢复，无权限/SELinux 修改。

## 真机功能矩阵（build-tag 绑定证据）

| run | scenario | k | okSteps | failSteps | I_water | I_sand | pool | events/lineage | serviceMs |
|---|---|---|---|---|---|---|---|---|---|
| t220land-v5 | land | 192 | 200 | 0 | 0 | 0.25 | 0 live | 0/0 | — |
| t220matrix-v5 | matrix | 192 | 32 | 0 | 256 | 24 | 192 live/full | 216/192 | 1341 |
| t220tilt-v5 | tilt | 512 | 32 | 0 | 1 | 0.125 | 1 live | 15/1 | 468 |
| t220m512-v5 | matrix | 512 | 32 | 0 | 256 | 64 | 512 live/full | 536/512 | 2780 |

**功能结论（如实）**：
- **浅/深/干/满矩阵**：水写入集中在注水列附近（W 25 列非零、max 113.46），沙 pool 满（K192 live 192/K512 live 512）、free=0——满池沙准入正确拒绝后续（EVENT 溢出前 CAPACITY/POOL 满路径生效）。
- **LAND 场景**：200 步 I_sand=0.25 且 W=0/S=0——沙粒子未在本场景触床（y=100 自由落体+地板反弹模型下），**LAND 路径在设备矩阵中未触发**（B-07 LAND 语义由 T215 离线 12/12 覆盖，本卡如实登记设备未复现）。
- **倾角**：±5° 交变下 step 全 OK，无越界/无崩溃。
- **事件边界**：K192 events 216（<eventCapacity 1790）；K512 events 536；event40/lineage3 结构随场景比例缩放。
- **源成功后 step 失败**：invalid-source（x 越界）注入 1 次，正确拒绝，此前 source commit 保留。

## 事务成本与实际资源（独立核）

- **allocation ledger（engine 自报，A+ cold snapshot 增量）**：K192 payloadBytes=4,230,656 / 160 数组；K512=6,312,256；coldSnapshotExtraBytes=2,654,016（K192）。与 T188/T209 交付表同量级。
- **整 frame 计数**：K192 matrix 32 步 serviceMs=1341（≈42ms/步含 4 source+32 step+JSONL encode/write/fsync 全成本）；K512 2780ms（≈87ms/步）——**包含 harness JSON 归档成本，非纯物理**；源 enqueue/commit 与 step 分项见 JSONL SCENARIOS 行。
- **热零分配/无逐 event 全 scan**：B-12 语义（T215 离线已核）设备侧由 counters 结构存在性佐证；精确分配次数需 TX trace（T205/T206 线）。

## 边界（不伪造）

- **C 未实施**：无 FrameToken、无可见明水、无 UI 观感——本卡数值与事务设备证据**不等效果**。
- LAND 设备未复现（见上），B-07 语义以 T215 离线为准。
- restore NOT_IMPLEMENTED（B-13）。
- 失败上报：无。无阻断。
