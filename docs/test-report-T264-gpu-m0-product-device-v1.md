# T264 GPU M0 产品整步真机验证 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-29 · 状态：**FAIL（产品桥接层阻塞，交 architect 首次归因）**
- 被测包：**T236-app-v1-signed.hap**（SHA `18e4895bd46de6974242df588b36789cb9cc2cb2d5ad7483a37111755248b1cc`，T257 授权签）
- 页面：`pages/GpuResidentM0`（`--ps grsm0 true`；注意 want 参数值须为字符串 `true`，页面判 `=== 'true'`）
- 归档：`test/reports/t264/`（t264a–h.json 状态序列 dump、t264m/n.jpeg、t264_grs.txt、t264_suspend.txt）

## 结果：APP_ROUTE 页面加载成功，产品管线在 **ADVANCE 层每会话失败**，整步闭环（source→step→completion→lease→同 token swap/像素）**未能建立**

| 项 | 证据 |
|---|---|
| 页面/路由 | `SetUIContentInner: pages/GpuResidentM0` ✅；XComponent `grs-m0`（SURFACE）native 库 `libgrsm0.so` 已加载（/proc/maps 5 处）✅；App 专用线程在跑 ✅ |
| 状态迁移（会话1，pid 12348） | `STATE_LOST`（invalidate）→（点 Spawn sand）→ **`{"state":"FAULT","stage":"ADVANCE"}`**，此后永久停 |
| 状态迁移（会话2/3，pid 16078，两次冷启动） | 同样：`STATE_LOST` → 点 New empty run / Spawn sand → **`FAULT/ADVANCE`** 复现，永不停恢复 |
| **缺陷 A：GRS_SOURCE INVALID_COMMAND** | 两次会话各复现：`GRS_SOURCE INVALID_COMMAND`（pid 12348 @15:54:00、pid 16078 @16:03:55）。页面构造 `ArrayBuffer(128)`+`Uint32Array(32)` 合法；`app_bridge.cpp:70 offer()` 的 napi 视图检查（`type!=napi_uint32_array || length!=32`）在真机 ArkTS 运行时拒绝——**ArkTS→napi typedarray 传递接缝失效**，source 命令从未入队 |
| **缺陷 B：FAULT/ADVANCE 永久失败** | `Fixed120Clock::Plan`（control.cpp:20）`deltaNs>33333333u` 即 `Pause()+INVALID_INPUT`；`app_bridge loop` 用真实墙钟差调 `Advance(delta,…)` 且**任何一次超 33.3ms 即 `failed=true` 永久熔断**。真机存在 Doze/调度挂起（SUSPEND_MANAGER `DOZE_BY_STOP_LIFECYCLE_DETECT_FOR_APP_FOREGROUND`），会话内长间隔后首个 delta 必然越限。**桥接层未钳制 delta、也未把时钟上限失败与"可恢复重同步"区分** |
| 源入队→completion→lease→swap | **NOT_RUN**：被缺陷 A/B 双重阻断，completion256/lease/像素/收益均无法观测 |

## 与 T252 同类的接缝属性

两缺陷都在 **ArkTS↔native 桥接/接线层**，产品 shader/transport/owner 核心未被执行到（无一条产品 completion 产生）——与 T252 `CW_V3_PARSE`（napi String 255B）同类：**离线 host 与真实 ArkTS 数据接缝的行为差异**。tester 不修产品，按卡交 architect。

## architect 归因输入（供裁决，tester 不实施）

1. **缺陷 A**：`offer()` 应接受 CommonJS 侧真实传入的视图类型（ArkTS Uint32Array 在真机 napi 下 type 报告值需核实），或页面改传 `ArrayBuffer`+桥内自行建视图；须与 T258 `clear_water_identity.h` 的"显式身份"合同一致，不得静默降级。
2. **缺陷 B**：`Advance` 调用方需钳 `deltaNs`（如 `min(delta, 33_333_333)`）或把 `Plan` 的 `INVALID_INPUT`（超时）降级为**重同步事件**（丢弃间隔、carry 重置、publish RESYNC），不得永久 `failed=true`——否则真机任何一次 Doze 挂起即永久停摆。二者择一或组合由 architect 裁决，涉及产品时序合同。
3. 页面 want 参数值合同：`--ps grsm0 true`（字符串 "true"），任务卡中 `grsm0=true` 的 `1` 写法不生效，建议开发交接文档统一。

## NOT_RUN / NOT_MEASURED（如实）

- K192 source→step→completion→lease→同 token swap/像素闭环：NOT_RUN（被阻断）。
- K512、末 ID 耗尽（0xFFFFFFFF/nextSandId=0）、重复 receipt、late/cancel/resize/background/contextloss 旧代拒绝：NOT_RUN（K192 闭环未建立前无对象）。
- GPU timer/disjoint、CPU/worker/fence/map p50/p95、同负载 CPU 对照收益：NOT_MEASURED（无整步运行）。
- 显示呈现（`displayPresentVerified`）：字段在 status JSON 中恒 false，无帧可验证。

## 证据

- `t264a.json`：首启 `STATE_LOST`；`t264b/c.json`：Spawn 后 `FAULT/ADVANCE`
- `t264d/e.json`：会话 2 冷启动 `STATE_LOST`（T+3s/T+8s）；`t264f1-4.json`：New empty run 后仍 `STATE_LOST`；`t264g/h.json`：Spawn → `FAULT/ADVANCE`（两次会话复现）
- `t264_grs.txt`：`GRS_SOURCE INVALID_COMMAND`（两 pid 各一条）
- `t264_suspend.txt`：SUSPEND_MANAGER/Doze 日志（佐证会话内长挂起存在）
- `t264m/n.jpeg`：页面截屏（状态文字 FAULT/ADVANCE 可见）
- 代码定位：`app_bridge.cpp:70`（offer 视图检查）、`control.cpp:20`（Plan 33.3ms 上限）、`app_bridge.cpp loop`（delta 熔断）

## 失败上报

**FAIL → architect**：缺陷 A（napi typedarray 接缝）+ 缺陷 B（delta 越限永久熔断）。复现：装 T236-app-v1 → `aa start --ps grsm0 true` → 点 Spawn sand → 状态 `{"state":"FAULT","stage":"ADVANCE"}`；hilog `GRS_SOURCE INVALID_COMMAND`。
