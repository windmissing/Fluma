# T344 · 冷采 captureMs 超预算归因与修复交付 v1

日期：2026-10-31。责任：developer。任务卡：滴答 `6ae5915de4b031ea82fbd6cc`。
依据：`docs/test-report-T340-cold-capture-device-v1.md`（P1 FAIL：三锚点 18.45/20.63/34.23ms 超 4ms 预算 4.6-8.6 倍；rawBytes=0 零 payload 却耗 18-34ms）。

## 1. 窗口内耗时构成定位（归因）

**结论：18-34ms 全部来自被窗口覆盖的调用方业务工作；T284Cold 自身在窗口内的动作（一次 enqueue 引用+算术记账）<0.01ms。窗口起表点定义过宽（类别 b：窗口定义过宽，非 T284Cold 可优化）。**

窗口结构（设备树 t340-v1 `P051DerivedV2Demo.ets :29-31` 实测代码路径）：

```
t284Choose → T284Cold.select()          ← 起表 captureBegin（锚点判定时刻）
  ├─ t248AuthorityCold()                 ← 模型侧权威捕获
  ├─ t248.capture(exportPayloadCold())   ← 发布快照捕获
  ├─ drawNewPipeline(...)                ← 完整渲染管线（pack+GL+swap）
  ├─ JSON.stringify(大对象)              ← T266_FRAME 日志构造
  │    （含 runClock JSON.parse+summary、nativeStages、receipt 全量、
  │     boundedTrace、landWriter、pixel——多项 O(N) 序列化）
  └─ t276Cold → T284Cold.enqueue()       ← 唯一的 T284Cold 自身动作
t284Finish → captureMs 定格              ← 计入以上全部
```

**窗口内工作清单与耗时构成**（按 T340 三锚点数据+T343 同页 hilog 结构推算）：

| 段 | 归属 | 估算 |
|---|---|---|
| select() 本身（锚点判定+索引登记） | T284Cold | <0.01ms |
| t248AuthorityCold + t248.capture | 调用方（模型侧） | ~2-4ms |
| drawNewPipeline（渲染管线全含） | 调用方（渲染） | ~8-15ms |
| T266_FRAME JSON 构造（多段 O(N) 序列化） | 调用方（诊断日志） | ~5-12ms |
| enqueue ×1（引用+记账） | T284Cold | <0.01ms |

**rawBytes=0 的机制解释**：页面代码中第一次（也是唯一一次）enqueue 发生在 drawNewPipeline 与 JSON 构造**之后**——起表后 18-34ms 的调用方工作先行耗尽 4ms 预算，enqueue 入口的预算检查（`now()-captureBegin>maxCaptureMs`）直接拒绝，**payload 从未入队**，故 rawBytes=0/parts=0/hashes=[] 与 18-34ms 同时出现。这与 T340 全部样本「零分片产出却超预算」逐字段吻合。

**T323「memcpy 级 ≤2ms」预期与源码现实的矛盾登记**：T323 分流稿设想「锚点触发只做原子 memcpy」，但实施结构（t284/apply.cjs 缝）把 select()（起表）放在了页面帧回调的锚点判定处，其后到 finish 之间天然包含该帧的渲染与日志工作——起表点定义本就含业务工作。预期需按语义对位修正为「captureMs 只计量捕获交接动作本身」，这正是本修复。

## 2. 修复方向唯一推荐与定量推算

**推荐 (a) 窗口收窄**——captureBegin 从 select() 移到**第一个 enqueue() 调用前**（惰性起表），captureMs 只计量 T284Cold 自身捕获动作。定量推算：

- 三候选评估：
  - (b) 调用方移出窗口：渲染管线与锚点帧日志**必须发生在该帧回调内**（渲染时序合同），物理上不可移出——否决；
  - (c) 预算重估：T284Cold 的 4ms 预算语义（T284 原始合同）就是「捕获动作」预算而非「整帧」预算——预算定义无错，否决（且红线禁自行放宽）；
  - (a) 窗口收窄：与 T323 §3「锚点只记 {anchorId, bufferValid, captureMs}」的原始语义一致；T284Cold 自身动作 <0.01ms，修复后 captureMs 预期 **≤2ms 有 200× 余量**。
- T340 三锚点对照：修复前 18.45/20.63/34.23ms（全部 INCOMPLETE 零产出）→ 修复后窗口只含 enqueue 记账，与三锚点各自的前置业务量无关——**统一回到亚毫秒**。

## 3. 最小修复实施（test/t284/T284Cold.ets 三处）

1. `select()`：`captureBegin=-1`（不起表）；
2. `enqueue()`：首次调用时惰性起表 `if(this.captureBegin<0)this.captureBegin=this.now()`（附归因注释）；
3. `finish()`：未起表（无捕获动作）→ captureMs=0 不误报超预算；起表后超预算照旧 `BUDGET_CAPTURE_MS`。

**INCOMPLETE 语义红线**：BUDGET_CAPTURE_MS 触发条件、reject 路径、零分片产出、NOT_CAPTURED 判定全部原样——修复只改起表点，不改任何预算判定。

## 4. 非机回归

| 套件 | 结果 |
|---|---|
| t284 selftest（T331 基线 15/15） | **15/15**（'capture wall budget explicit' 用例按收窄语义适配：预捕获业务不计费断言+窗口内延迟仍触发断言，双向覆盖） |
| t284 reassembly | 4/4 |
| **T344 窗口合同 W1-W4** | **4/4**：W1 零业务窗口 captureMs=0.01≤2ms；W2 设备形态（20ms 调用方业务后 enqueue）不计费不触发（T340 故障形态消除）；W3 窗口内 5ms 延迟仍 BUDGET_CAPTURE_MS（fail-closed 不变）；W4 无捕获动作 captureMs=0+NOT_CAPTURED |
| 生成树 hvigor | BuildNativeWithNinja+CompileArkTS+PackageHap 绿（SignHap 00303116 已知如实保留） |

## 5. 红线遵守

- 4ms 预算数值未改；INCOMPLETE/BUDGET_CAPTURE_MS 规则未改（W3 实证）；
- T331 增量证书/sort 消除主链零接触；
- T323 预期矛盾如实登记（§1）并按语义对位修正；
- 物理零接触。

## 6. 交付后路径

设备复测（captureMs 实测回 ≤2ms）归 T343 换包或后续真机卡；本修复在 t284 生成树链（generate.cjs→apply.cjs 复制 T284Cold.ets），复测包自动携带。

## 7. 包签名状态

非机验证，无手签包（SignHap 00303116 已知如实保留）；复测包由执行 tester 按 T151 补签。

## 8. 变更记录

- 2026-10-31 v1：初版。归因（窗口起表点过宽，18-34ms=渲染管线+权威捕获+大 JSON 构造；rawBytes=0 机制=enqueue 前预算已耗尽）+唯一推荐 (a) 定量推算+最小修复（惰性起表三处）+窗口合同 W1-W4+selftest 15/15+reassembly 4/4+构建绿。
