# T335 · glFenceSync 停顿首次归因与 Amendment 2 v1

日期：2026-10-31。责任：architect。任务卡：滴答 `6ae55382e4b031ea82f3f856`。
状态：**MECHANISM_ATTRIBUTED / SINGLE_FIX_RECOMMENDED / AMENDMENT_2_READY**。
依据：`test-report-T330-pump-loop-device-v1.md`、`test/reports/t330/t330_device_J_accounting.json`（三 run 原始分账）、T329 实施（e66f1c2）、`architecture-T328-batch-gap-ratio-ruling-v1.md`（Amendment 1）、当前源码 `m1/full_transport.cpp`/`bridge_timeline.h`。纯设计零代码。

## 0. 裁定总览

| 项 | 结论 |
|---|---|
| 停顿机理 | **(a) 驱动在 sync 对象创建时强制实现（realize）累计未 flush 命令缓冲**——停顿与未 flush dispatch 数线性相关（296/325µs/dispatch，两 run），非 (b) 等待完成（规范语义不等待）非 (c) 其它串行化（无线索） |
| 修复方向 | **唯一推荐 (i) 末批前分批 glFlush**（每非末批 ExecuteBatch 尾加 glFlush）——fence 处残余量降到单批 ~10 dispatch，预期停顿 37-41ms→~3ms，单 Pump 总墙钟 ≈6.7ms < 8ms 预算 |
| (ii) fence 独立 Pump | **拒绝**——停顿原值搬家：该 Pump 自身仍超 8ms 预算，且 FENCE_PENDING 停留期照样超 33ms GAP 窗（T330 实测 46-49ms），root 仍被取消 |
| (iii) 预算重算/批上限 | **拒绝**——fence 停顿不是预算算术问题 |
| FENCE_PENDING×GAP | **既设行为，无需合同变更**——code=2 取消路径真机已验（无 fence 泄漏无崩溃）；修复 (i) 后 root 在窗内封尾+Poll，交互变为罕见路径；INV-6 不动，补 INV-8 文档注记 |
| 环扩容 | **1024→4096**（Amendment 1 使单 root 事件密度 50→376 行，7.5 倍；4096 覆盖 ~10 root；+330KB 诊断构建内存；fail-closed/分页不变） |

## 1. 停顿机理归因（机理假设分层裁定）

### 1.1 T330 定量分解（原始数据复核）

| 量 | run1 | run2 |
|---|---:|---:|
| 12 个非末批 wall 合计（dispatch 提交本体） | 3.74ms | — |
| 末批 wall（含 fence） | 37.72ms | 41.34ms |
| **fence 单调用停顿**（FenceEnter→FenceExit 原值） | **37.65ms** | **41.28ms** |
| dispatch 总数 | 127 | 127 |
| **停顿 / dispatch** | **296µs** | **325µs** |
| Pump 总段 | 39.04ms | 42.54ms |
| anchor→pause（GAP 触发原值） | 46.12ms | 49.15ms |

**关键事实**：fence 停顿几乎独占末批 wall（37.65/37.72 = 99.8%）；循环本体 12 批合计仅 3.74ms——**瓶颈唯一且已定位到 glFenceSync 单调用**。

### 1.2 机理分层裁定

**(a) 驱动在 sync 创建时实现累计未 flush 缓冲——采纳为最可能机理**：
- **代码事实**：现行 ExecuteBatch 的尾序是 `glFenceSync(...); glFlush();`（full_transport.cpp:104）——**fence 创建在 flush 之前**，12 个非末批循环内无任何 flush（逐批只 RunPassRange+glMemoryBarrier，无 glFlush）。fence 创建时驱动面前是 **127 个未 flush 的 dispatch**。
- **数量关系**：37.65ms/127 ≈ 296µs/dispatch、41.28ms/127 ≈ 325µs/dispatch——两 run 独立测量给出高度一致的 per-dispatch 归一化值，且该值落在 T318 实测单 dispatch GPU 执行成本区间（最大 691µs、典型 100-300µs）内。**停顿量追踪累计命令数而非瞬时 GPU 负载**——这是「deferred realization 一次性结账」的特征指纹。
- **GL 规范依据**：glFenceSync 向命令流插入 sync 对象；在 tile-based mobile GPU 驱动（Mali/Adreno 类）上，sync 对象创建需要与 GPU 前端建立同步点——驱动此时把攒在 CPU 侧命令缓冲的 pending dispatch **提交/实现**到 GPU，该实现成本在创建调用内同步发生。ES 3.x 规范不禁止驱动在 fence 创建时做隐式提交（规范只约束客户端语义：fence 创建「不应等待完成」——见 (b)）。
- **与既有数据不矛盾**：T317 前的整体式 SubmitQuantum 同样是 fence-在-flush-前 + 整 root 无中间 flush——M1 Pump 内 78.07ms 中就含同型停顿（当时无 41 阶段无法分离）；T318 时代分批未循环、root 在 BATCH_ACTIVE 即被取消、fence 从未创建（FenceEnter=0）——无 fence 样本；PC SwiftShader 是软件渲染器（无 tile-based 提交模型），T329 N1 的 13ms 首批特性是编译/资源 lazy init，机理不同。三组历史数据与本机理**零矛盾**。

**(b) fence 创建等待 GPU 完成——拒绝**：GL 规范明确 fence 创建不等完成（glClientWaitSync 才等）；若机理是等待，停顿应随 GPU 瞬时负载波动而非线性追踪 dispatch 数。两 run 递增停顿（37.65→41.28ms）与 dispatch 数恒定（127）的偏离（+9%）可用 GPU 频率/热状态解释，不推翻线性模型。

**(c) 驱动/context 级其它串行化——排除**：无线索支持（无证据指向 context 切换/锁竞争；单 worker TID 恒定 4161/36438）。

**结论：(a) 为主机理，置信度高（数量指纹 + 规范允许 + 三组历史数据互证）。不判驱动 bug**——这是 tile-based 驱动在「sync 创建于大量未 flush 命令后」这一使用模式下的合理行为，修复应在调用侧（分批 flush），不在驱动侧。

## 2. 修复方向裁定（唯一推荐 (i)）

### 2.1 三方向对照推算

| 方向 | fence 停顿（推算） | 单 Pump 总墙钟 | vs 8ms 预算 | vs 33ms GAP 窗 | 语义代价 | 判定 |
|---|---|---|---|---|---|---|
| **(i) 每非末批尾 glFlush** | **~3ms**（fence 只面对末批残余 ~10 dispatch × ~300µs） | **≈6.7ms**（3.74 工作 + ~3 fence） | ✓ | ✓（anchor→pause 预期 ~7ms） | 零——flush 位置在批边界，事务/dispatch 序/遥测全不动 | **✅ 唯一推荐** |
| (ii) fence 拆独立 Pump | 37-41ms（原值） | fence-Pump 39-42ms | ✗ 超 5 倍 | ✗ FENCE_PENDING 停留期 46-49ms 照样超窗 | 还引入新风险：fence-Pump 期间 GAP 取消（T330 code=2 正是此路径） | ❌ 停顿搬家不消停顿 |
| (iii) 预算重算/批数上限 | 37-41ms（原值） | 39-42ms | ✗ | ✗ | 无 | ❌ 不触及停顿本身 |

### 2.2 方向 (i) 的论证细节

- **GL 语义相容**：glFlush 可在任意命令间调用（ES 3.x 规范：「flush 告诉驱动尽快执行 accumulated 命令」）；批边界（glMemoryBarrier 后）是 T324 合同已认定的天然切割点，flush 置于此处与 INV-3（dispatch 序不变）相容。
- **逐批 ≤8ms 相容性**：glFlush 本身按规范「尽快返回」（异步请求，不等 GPU 完成）——12 次批尾 flush 各自的 CPU 侧成本预期在 µs 级（真机实测待 T331 验证）；即便驱动对 flush 也做部分同步实现，每次只面对单批 ~10 dispatch（~3ms GPU 侧量），**最坏情形该批 wall 增至 ~3ms 仍 ≤8ms 预算**。
- **事务语义零接触**：flush 不改变命令序、不产生可见副作用、不影响 SSBO 状态；T324 的提交原子性由 fence+owner.Poll 保障（flush 与否不参与原子性）。41 字典无新增 stage（flush 是 ExecuteBatch 内部实现细节，不是合同事件；如需观测可在批遥测 code 位扩展，见 §3 遥测）。
- **与 Amendment 1 六项已验证条款兼容性**：nonce 一次消耗（不动）/carry 单调（不动）/取消无崩溃（不动）/submitting≠inflight（不动）/41 字典（不动）/逐批 ≤8ms（12/13 批实测 ≤0.36ms，加 flush 后最坏 ~3ms 仍过）。**J2/J4/J5 已 PASS 项无回归路径**：封尾路径（J2）因停顿下降更稳定到达；code=16（J4）与 GAP 原值（J5）与 flush 无交互。
- **尾部顺序修正（附带）**：末批的 `glFenceSync; glFlush;` 顺序维持现状即可（分批 flush 后 fence 面前只剩末批残余）；无需调整为 flush-先-fence-后——两者在分批 flush 下等价（fence 语义只要求 fence 之后的命令不先于 fence 执行，flush 位置不影响 fence 的完成条件）。

## 3. Amendment 2（T324 合同第二次修订）

对 `architecture-T324-gpu-partial-submit-contract-v1.md`（经 Amendment 1）的增量修订：

### §2.2 修订（Amendment 2）：批尾 flush

```
ExecuteBatch() 尾部增补：非末批（passCursor < total 返回 OK_SUBMITTING 的路径）
在 lastBatchWallNs 采样后追加 glFlush()；
末批（封尾路径）维持现行 fence→flush 序不变。
```

**新不变式**：
- **INV-8（FENCE_PENDING×GAP 既设语义文档化）**：CancelSubmit 在 FENCE_PENDING（code=2）删 fence + owner.Stop 是完整清理（GPU 侧已提交工作随新 run 作废——**anchor 窗统辖 run 存活，这是既设行为**）；T330 真机已验无泄漏无崩溃。修复 (i) 落地后该路径成为罕见路径（root 在窗内封尾+Poll），**不删除该路径**——GAP 语义（窗判定不豁免不放宽）保持完整。
- **INV-9（flush 不改批遥测）**：批尾 flush 不产生 timeline 事件（无新 stage）；批 wall（lastBatchWallNs）自然含 flush 成本——若 flush 使某批 wall 显著增长（>1ms），该批 wall 原值披露即暴露，不隐藏。

### §2.5 修订（配比后目标更新）

Amendment 1 的「单轮 Pump ≤8ms 且封尾总墙钟 ≤33ms」目标在真机的**新瓶颈口径**下重述：单轮 Pump ≤8ms **含 fence 创建**（即末批 wall 含 fence 停顿须 ≤8ms——分批 flush 后预期 ~3ms）；J1 判据的 submitPumpWallNs 语义不变（自然含 fence）。**8ms 预算与 33ms GAP 窗都不动**——本修订不构成放宽。

### 实施/验证规格

**developer 实施卡（建议 T331）**：
1. ExecuteBatch 非末批尾加 `glFlush();`（一行）；
2. **环扩容**（§4 裁定）：bridge_timeline.h `capacity=1024`→`4096`（一处常数）；
3. 非机回归：T329 的 N1-N3 全量（行为断言不含 flush 时序——flush 不改返回码/状态）+ 新用例：替身计数 flush 调用=非末批批数（12/root）。

**tester 真机卡（T331 后续或并入）**：
- K1：末批 fence 停顿 ≤5ms（分批 flush 生效的直接判据；T330 基线 37-41ms）；
- K2：submitPumpWallNs ≤8ms（J1 在含 fence 口径下转 PASS）；
- K3：anchor→pause < 33ms、root inflight→Poll→accepted ≥1（J3 转_PASS 前提）；
- K4：六项无回归项复测（nonce/carry/取消/code=16/GAP 原值/逐批 ≤8ms）；
- K5：环 4096 下三 run 导出 COMPLETE（NATIVE_DROPPED 消失）。

## 4. 环扩容裁定（T330 移交）

- **事实**：Amendment 1 循环使单 root 事件密度 50→376 行（7.5 倍）；环 1024 在多 run 会话（T330 三 run）必然溢出，三次导出全 INCOMPLETE（fail-closed 语义正确，判据数据在溢出前已落盘）。
- **裁定：1024→4096**。依据：密度涨 7.5 倍是 Amendment 1 的结构性后果（非异常），环容量按新密度重标定（4096≈10.8 root 余量）；内存 110KB→442KB（+330KB，BridgeTimeRow ~108B×4096，诊断构建常驻）；分页导出（4 行/页）与 fail-closed 语义不变。**不做**自动导出清环（改变冷导出语义，侵入大）；**不维持** 1024（多 run 诊断持续残废）。
- 归属：工具链常数修订，并入 T331 实施卡（一处常数 + 回归断言），不单独立卡。

## 5. 路由

- 本卡归因+Amendment 2 → **developer 实施卡（建议 T331）**：flush 一行 + 环常数一处 + 非机回归；
- → **tester 真机卡**：K1-K5（J1/J3 预期转 PASS，双源同 token 链第二次获得封尾前提）；
- 与 T328 裁定/T329 实施无矛盾：T328 方向 (a)（Pump 内循环）真机已证生效（13 批单 Pump 2.25ms 本体）——本卡是把同一循环结构下的**下一个**瓶颈（fence 结账）消除，方向连续；
- 若 K1 后 fence 停顿仍 >5ms（分批 flush 未充分摊销），回流本卡重估机理 (a) 的摊销模型——预期概率低（摊销逻辑直接）。

## 6. 变更记录

- 2026-10-31 v1：初版。机理 (a) 采纳（296/325µs/dispatch 数量指纹+规范允许+三组历史数据互证，不判驱动 bug）；修复唯一推荐 (i) 分批 flush（fence 残余 ~3ms、Pump ≈6.7ms；(ii) 停顿搬家拒绝、(iii) 不触及拒绝）；Amendment 2（批尾 flush+INV-8/9+§2.5 含 fence 口径重述）；FENCE_PENDING×GAP 核清为既设行为（code=2 真机已验，无合同变更）；环 1024→4096（密度 7.5 倍重标定）；K1-K5 验证判据。
