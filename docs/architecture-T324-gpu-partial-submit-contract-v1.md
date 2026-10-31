# T324 · GPU 分片提交合同（partial-submit 状态机）v1

日期：2026-10-30。责任：architect。任务卡：滴答 `6ae45280e4b0e9cb68e5599a`。
状态：**ENGINEERING_CONTRACT_READY / FOR_T317_DEVELOPER_CONTINUATION**。
依据：`developer-T317-first-root-preparation-v1.md`（未完成合同请求原文）、`architecture-T313-gpu-first-root-window-v1.md`、实际源码 `run.cpp`/`m1/run.cpp`/`m1/full_transport.cpp`/`m1/quantum_candidate.cpp`/`app_bridge.cpp`。纯设计零代码。

## 0. 裁定总览

**合法异步/分片可在现合同内实现，不需要改变物理时钟/策略，不转 designer。** 关键依据是既有代码已包含三块可复用的异步先例：

1. `QuantumCandidate` 已有 `deferFence` 字段（quantum_candidate.h:35）+ 独立 `Submit(...)`/`Poll()` 生命周期（quantum_candidate.cpp:24-25）——Enqueue 时可不创建 fence、返回 OK 后由后续 Poll 补建/等待；
2. `ResidentRun::Pump` 已是「BUSY = 本轮无进展」的状态机（run.cpp:19-30：inflight 时 Poll，未 inflight 时 Submit）——continuation 只是把这个模式**下沉一层**到 transport 内部；
3. `FullM1Transport::Poll` 已处理「fence 未信号 → BUSY」（full_transport.cpp:44-46）——分片只是把 BUSY 的粒度从「整 root」细化到「root 内的 pass 批」。

T317 报告的三个死结逐一消解：

| T317 死结 | 合同消解 |
|---|---|
| 中间 pass 返回 BUSY → Run 下轮重试同 nonce 并重复 Begin | **nonce 不再在 Submit 入口消耗**：nonce 消耗移到 root 封尾（§2.4），BUSY 返回时 nonce 未动，无「重复 Begin」 |
| 返回 OK 则把未封尾中间态冒作 inflight | **新状态 `submitting` 与 `inflight` 分离**（§2.2）：submitting 不是 inflight，Poll 只认 inflight，无假完成 |
| 剩余工作转线程违反 context/owner 串行合同 | **不转线程**：所有 GL 调用仍在原 owner 线程，分片 = 把 27+ 个 dispatch 按批拆到多个 Pump 调用（§2.3），current context 每批重新确认（`Current()` 已有先例） |

## 1. 现状结构（源码事实）

M1 `FullM1Transport::SubmitQuantum`（full_transport.cpp:23-38）单次同步完成：
- 前置校验（nonce/frame/ordinal/params 十余项）→ `source.Retire()` → `PinCandidate()` → `owner.BeginQuantum`
- `quantum.Enqueue(...)`（quantum_candidate.cpp:12-20）：固定 27+ 个 `Pass()`/`Sort()` dispatch 序（Pass(0..27)，含 2 次 color 循环 ×3 pass、4 次 Sort）
- Enqueue 尾部：`glFenceSync` + `glFlush` → 返回 OK
- 然后 full_transport 才 `quantum.Set(...)`×7 + 再 Pass(21..28) + 第二个 fence + `active=true; quantumFlight=true`

M0 `SubmitRoot` 同构（owner.Begin 后遍历整份 plan）。

`ResidentRun::Pump`（run.cpp:26-30）：Submit 返回 OK 才 `nonce=next; inflight=true; receipt.submitted++`。**nonce 在 Submit 调用前 Add64 预算、OK 后提交**——这是「BUSY 重试同 nonce」问题的根源：现行 Run 层把 BUSY 当作可重试，但 transport 层 BUSY 语义是「整机忙」不是「分片未完」。

## 2. 分片提交状态机（工程合同）

### 2.1 新增 Transport 内部状态

```
enum class SubmitPhase : uint8_t {
  IDLE,           // 无进行中 root
  BATCH_ACTIVE,   // 本轮 Pump 已发出 ≥1 批 dispatch，批预算耗尽或批计数达上限，root 未封尾
  FENCE_PENDING,  // 全部批已发出，fence 已创建（或 deferFence 模式待补建），等 GPU 信号
  SEALED          // fence 信号 + owner.Poll 完成 → 转入现行 inflight 语义（对 Run 层即"提交完成"）
};
```

关键不变量：
- **INV-1**：`BATCH_ACTIVE`/`FENCE_PENDING` 期间 `active==false`（沿用现行 `if(active||frozen)return BUSY` 的对外语义），但新增 `submitting==true`。Run 层在 submitting 期间收到 BUSY **不重试同 nonce**（§2.4 nonce 合同）。
- **INV-2**：GL owner/context/thread 每批进入时用既有 `Current()` 断言（quantum_candidate.cpp:5 先例）；任一批 `Current()==false` → `STATE_LOST`（现行码已有），不跨线程。
- **INV-3**：dispatch 顺序 = 现 Enqueue 的固定 27+ Pass 序，分片只在 Pass 边界切割，**不重排、不跳过**；每批边界 = `glMemoryBarrier` 后（Pass 内部已含 barrier，天然可切割点）。
- **INV-4**：uniform 只在批首 `Set` 批内所需项（`uPass` 每批必须；`uColor` 等批内常量在批首重设——GL uniform 是 program 状态，跨批保持，但**每批重设首项**防止 driver 状态丢失，开销可忽略）。

### 2.2 Submit 签名与批预算

```cpp
// TransportCode FullM1Transport::SubmitQuantum(p, r) 内部改造（对外签名不变）：
//   首次调用（IDLE → BATCH_ACTIVE）：
//     全部前置校验 + Retire + PinCandidate + BeginQuantum（现行 :25-33 原样）
//     发出第 1 批（budgetNS 个 Pass）后返回 TransportCode::OK_SUBMITTING（新码）
//   续批调用（BATCH_ACTIVE，同 r.nonce）：
//     发出下一批，未完 → OK_SUBMITTING；全部批完 → 建 fence + glFlush → FENCE_PENDING → OK_SUBMITTING
//   Fence 后调用（FENCE_PENDING）→ 等价现行 Poll 逻辑（BUSY/OK/失败）
```

**批预算双限**（每批 CPU wall 上限，取小者）：
- `kBatchBudgetNs`：默认 **8,000,000ns（8ms）**——一个时钟窗的一半，保证单批内可完成「批发出 + 返回 + bridge 超窗检查」。可配置（见 §4 遥测），首版固定 8ms。
- `kBatchMaxPasses`：默认 **8 个 Pass**（27+ Pass 序 / 8 ≈ 4 批）——防单 Pass 超长（Pass(0,words) 大 dispatch）时预算形同虚设；单 Pass 超 8ms 时该 Pass 独占一批（无法再切，见 §5 边界）。

新 TransportCode 枚举值（追加，不改旧值）：
- `OK_SUBMITTING = 16`（示意值，实际以 transport 枚举表为准）：**本 root 已接受、分片进行中、未封尾**。Run 层据此推进 `receipt.submitted++` 一次并转 submitting 态（见 §2.4）。

### 2.3 Run 层 continuation（同 nonce 续批）

```cpp
// ResidentRun::Pump / m1::Run::Pump 改造（语义增量，其余原样）：
if (submitting) {                       // 新分支，在 inflight 分支之前
  auto code = transport.ContinueSubmit(); // 同 nonce 续批（内部按 §2.2 推进）
  if (code == TransportCode::BUSY) return code;      // 本轮无进展（不应发生，防御）
  if (code == TransportCode::OK_SUBMITTING) return TransportCode::OK; // 未完，下轮续
  // code == OK：全部批 + fence 完成 → 转入现行 inflight 语义
  inflight = true; /* 现行 :30 的 nonce/sourceFlight/receipt.submitted 逻辑在此执行 */
  return TransportCode::OK;
}
```

**nonce 消耗时点修改（本合同核心）**：
- 现行：`Add64(nonce,{1,0},next)` 在 Submit 前预算，OK 后 `nonce=next`——BUSY 时 nonce 未消耗，但 Run 下轮会**用同一 next 重试**，若 transport 已部分接受则重复 Begin。
- 新合同：`next` 的计算与校验留在首次 Submit（IDLE 态）——首次返回 OK_SUBMITTING 即 `nonce=next` **立即消耗**；后续 ContinueSubmit **不再带 nonce 参数**（transport 内部持有 pending root），无重复 Begin 可能。BUSY 只会在 IDLE 态（整机忙）返回，该语义与现行完全一致。
- **nonce 重试消除**：submitting 态下 Run 不再构造 root、不再 Add64——`root` 构造代码移入 `if(!submitting && !inflight)` 分支（现行 :26-28 位置不变）。

### 2.4 撤权 / 取消 / 封尾 / drain（cancel & seal-fence）

**Pause 期间的 cancel/revoke（未封尾 fence 的撤除）**：

```cpp
// 新增 TransportCode CancelSubmit()：
//   前置：submitting==true（BATCH_ACTIVE 或 FENCE_PENDING）
//   BATCH_ACTIVE：已发出的 dispatch 无法撤回（GPU 命令流不可回滚——GL 语义），
//     但 candidate/owner 侧可复位：source.owner.Stop()（现行 full_transport.cpp:33 失败路径先例）
//     + candidatePinned=false + failed=true → 状态回 IDLE，返回 PROTOCOL_REJECTED
//   FENCE_PENDING：glDeleteSync(fence)（现行 Poll :49 先例）+ 同上复位
//   语义：取消后的 root 永不进入 inflight/accepted；Run 层 Fail(PROTOCOL_REJECTED) 与现行 Pause 一致
```

**Pause/Stop 的 drain 序（app_bridge PauseBegin 先例）**：
1. `Run::Pause()`（现行 :32/:37）先 `clock.Pause()` + 清队列（原样）；
2. 若 `submitting`：先 `transport.CancelSubmit()`（§上文）再 `transport.Stop()`——**不能对未封尾 root 走 quarantine**（quarantine 语义 = 已建立 flight 等 fence，submitting 无 fence 可等；现行 `if(inflight){quarantined=true;...}` 分支保持只对 inflight）；
3. 若 `inflight`（封尾后）：现行 quarantine/Poll drain 原样不动。

**epoch/publication/lease 时序（不变式清单）**：
- publication slot 在首次 Submit 即锁定（`pending.publicationSlot`，现行 :36 已在 SubmitQuantum 内赋值）——分片期间 slot 占用但未发布，`source.owner.pubs[slot].available=false` 已在 SubmitSource 路径设置（full_transport.cpp:19），SubmitQuantum 路径经 `owner.BeginQuantum` 同样锁定；
- lease：`PinCandidate` 在首次 Submit 执行、`candidatePinned` 保持到 Poll 完成——分片期间 pin 保持，CancelSubmit 显式释放；
- epoch：bridge 撤权（`RevokeLocked`）只影响 App 层 epoch 计数，transport 不读 epoch——分片不引入新的 epoch 交互。

### 2.5 总墙钟合同（跨多批与 GAP 的关系）

**现行 GAP 合同不变**：`app_bridge.cpp` 的超窗检查（`Stamp()-last>33333333u` → CLOCK_GAP → 撤权 → Pause → 新 run）以 **AnchorSet→AnchorSet** 为窗。分片不改变该窗口定义——**跨批的总提交时间仍受该窗口约束**，这正是合同要求「不能钳 delta 隐藏长根」的体现：

- 分片的目标不是让长 root 逃脱 GAP，而是**让单次同步调用不超 33ms**（bridge 主循环每轮 Pump 必须返回，才能检查时钟、处理撤权）。分片后：每轮 Pump 发一批（≤8ms）→ 返回 → bridge 正常超窗检查 → 若已超窗则 Pause+CancelSubmit（§2.4）→ **长 root 被合法取消而非卡死主循环**；
- 若 root 未超窗但需多轮：每轮 Pump 之间 bridge 正常 Advance/Wait/Draw——submitting 状态下 bridge 的 Draw 用 LastGood token（`scene.LastGoodToken()`，现行 STALE 路径先例 :161），**不把未封尾 root 当 CURRENT**；
- **总墙钟披露义务**：新遥测字段 `submitTotalWallNs`（首次 SubmitEnter 到 fence 信号）逐批累计，超窗取消的 root 记 `submitCancelled=true` + 已发批数——GAP 判定（原值 CLOCK_GAP/撤权/新 run）与提交分账（cancelled vs completed）**分开记录**，不互相掩盖。

### 2.6 与 39 阶段字典的衔接

`T317_STAGE39_V2` 的 SubmitEnter/Exit、DispatchEnter/Exit、FenceEnter/Exit 语义在分片下扩展为：
- SubmitEnter/Exit：**每批一对**（不是每 root 一对）——`carry` 字段记批号（0,1,2...），首次批 carry=0 且额外 Emit 一个 `SubmitRootBegin`（新 stage 40，只首次）；
- DispatchEnter/Exit：每 Pass 一对（现行 quantum_candidate.cpp:9 已是每 Pass Emit，原样）；
- FenceEnter/Exit：只在末批 Emit（现行 full_transport.cpp:37 已有，位置移到末批）；
- 新 stage 41 `SubmitCancel`（CancelSubmit 成功时一对）。
- 字典版本 `T324_STAGE41_V1`，旧 1-39 数值不变（沿用 T317 保留旧值的先例）。

## 3. M0 与 M1 的差异处理

- **M1（FullM1Transport）**：本合同主体。Enqueue 的 27+ Pass 序是唯一切割对象；`SubmitSource`（full_transport.cpp:14-21）是单 dispatch 小 root，**不分片**（实测 M0 首 root 187ms / M1 78ms 都发生在 quantum/plan 路径；source 路径若也超窗可复用同机制，二期）。
- **M0（SubmitRoot 遍历整份 plan）**：plan 的 pass 序与 M1 同构，同一状态机适用；差异仅是 M0 无 `QuantumCandidate` 封装——需要在 M0 transport 内新增等价的 `passCursor`/`batchBudget` 字段。**首版先做 M1**（GPU 主线是 M1 quantum），M0 复用合同二期落地。

## 4. 遥测与验证

**新增字段（bridge status 追加，不删旧字段）**：
- `submitPhase`（IDLE/BATCH/FENCE/SEALED 字符串）
- `submitBatches`（本 root 已发批数）、`submitBatchPasses`（末批 Pass 数）
- `submitBatchWallNs`（末批 wall）、`submitTotalWallNs`（§2.5）
- `submitCancelled`（bool，CancelSubmit 计数）

**非机回归（沿 T317 test/t317/build.cmd 替身先例，新增用例）**：
1. 分片正常完成：27+ Pass 分 4 批，nonce 只消耗一次，fence 只建一次，receipt.submitted==1；
2. 批间 BUSY 语义：submitting 期间外部 Stop → CancelSubmit → 状态回 IDLE、无 quarantine、Fail(PROTOCOL_REJECTED)；
3. FENCE_PENDING 取消：末批后、信号前 Pause → glDeleteSync 被调（替身计数）→ 无泄漏 fence；
4. 超窗取消：脚本时钟注入 34ms 窗 → bridge CLOCK_GAP → CancelSubmit → submitCancelled=true、submitTotalWallNs 原值披露；
5. 每批 Current() 断言：模拟 context 切换（替身置换 current context）→ STATE_LOST；
6. 旧 39 阶段回归：不分片的小 root（source 路径）行为逐位不变（16/0 现行 suite 原样通过）。

**真机最小定点需求（T318 消费，非本卡门）**：用 41 阶段字典跑一次 C1 复现序列（T151 同款 swipe），确认设备上首 root 的 SubmitExit-SubmitEnter 逐批 ≤8ms 且总 wall 披露——**定位「单 dispatch 不可抢占」是否存在的唯一手段**（见 §5）。

## 5. 边界与已知限制（明示）

- **单 Pass 超预算不可切**：`Pass(0,words)`（words≈55K+，groups≈864）若设备上单 dispatch >8ms，该批超预算仍整批发出——GL 无 dispatch 内抢占。此时合同保证的是「主循环不被卡死（该批后返回）」而非「每批 ≤8ms」。真机 41 阶段数据若显示单 Pass 超窗，则须 designer 评估 words 拆分（shader 侧工作分区），**超出本卡工程范围**，作为 §4 真机需求的第一优先检查项。
- **批间 GL 状态假设**：分片假设 driver 在两次 Pump 之间保持 program 绑定与 SSBO 绑定（同线程同 context，GL 规范保证）；但**每批首 Pass 前重设 `glUseProgram`+绑定**（防御性，开销 5×glCall 级）列入实现细节。
- **不改动物理时钟/审计/守恒**：分片只动 transport/Run 的提交结构；quantum 的数值内容（uniform、pass 序、barrier）逐位不变——`p011PreviewCommitConsistent` 等物理校验路径零接触。
- **T317 的 79 项既有证据不重做**：本合同只补「未完成」部分（实际 root 提交窗口），已交付的冷准备/39 阶段/签包全部沿用。

## 6. 路由

- 本卡交付物 = 本文档（工程合同）；**T317 developer 按本合同继续**实现 partial-submit（原卡「未完成与须 architect 接手的具体合同」一节的直接回应）；
- 实现完成后 T318（tester 真机闭环）按 §4 真机需求消费 41 阶段数据；
- 若真机显示单 Pass 超窗（§5 第一项），回流 designer 评估 shader 工作分区——**现在不预派**。

## 7. 变更记录

- 2026-10-30 v1：初版。分片状态机（IDLE/BATCH_ACTIVE/FENCE_PENDING/SEALED）、nonce 消耗时点前移、CancelSubmit/drain 序、总墙钟与 GAP 关系（不钳不藏、cancelled 分账）、41 阶段字典、M1 先行 M0 二期、真机最小定点需求。
- 2026-10-31 **Amendment 1（T328 裁定，见 [architecture-T328-batch-gap-ratio-ruling-v1.md](architecture-T328-batch-gap-ratio-ruling-v1.md)）**：T318 真机证实逐批 ≤8ms 成立（最大 1.712ms，单 dispatch 691µs，§5 单 Pass 超窗不存在）；但「每 Pump 一批×8ms tick」使 130-op 表需 13 轮 Pump≈104-127ms，结构性超 33ms GAP 窗 3-4 倍，root 永不封尾。**§2.3 修订为 Pump 内多批循环**（kPumpBudgetNs=8ms 护栏，13 批压缩到单轮 Pump≈3.5ms）+ INV-5/6/7 增补 + §2.5 配比后目标（封尾总墙钟≤33ms）+ submitPumpWallNs 字段；GAP 判定不豁免不放宽。另：UINT32_MAX 首批 exit code 补显式 End(16)（SubmitQuantum lambda 尾与 SubmitSource 对称）。其余条款（nonce/逐批/取消/字典）经真机验证全不动。
- 2026-10-31 **Amendment 2（T335 归因，见 [architecture-T335-fence-stall-attribution-v1.md](architecture-T335-fence-stall-attribution-v1.md)）**：T330 真机证实 Amendment 1 循环生效（13 批单 Pump、循环本体 2.25/1.48ms），但**末批 glFenceSync 单调用停顿 37.65/41.28ms**（机理 (a)：驱动在 sync 创建时实现累计未 flush 命令缓冲，296/325µs/dispatch 线性指纹）击穿 8ms 预算与 33ms GAP 窗，J1/J3 FAIL。**§2.2 修订：每非末批 ExecuteBatch 尾加 glFlush**（fence 面前残余降至单批 ~10 dispatch，预期停顿 ~3ms、单 Pump ≈6.7ms）；INV-8（FENCE_PENDING×GAP 既设语义文档化：code=2 取消真机已验无泄漏）/INV-9（flush 不改批遥测，批 wall 自然含 flush 成本）增补；§2.5 重述为含 fence 口径（预算/窗均不动）。附：timeline 环容量 1024→4096（Amendment 1 使单 root 事件密度 50→376 行，7.5 倍重标定）。验证判据 K1-K5（fence 停顿 ≤5ms / submitPumpWallNs ≤8ms / accepted ≥1 / 六项无回归 / 环导出 COMPLETE）。**⚠️ 2026-10-31 T337b 真机证伪本修订的机理前提**（T341 §1）：停顿是 flush/fence 类调用的 ~33-41ms 固定税、与命令数无关（10 vs 130 dispatch 同税），线性摊销模型不成立——分批 flush 未消除停顿仅使其转移（首批 33.66ms），「逐批 ≤8ms」回归。Amendment 2 布局**保留**（T341 裁定：预热生效后 flush 零税，防御价值保留），机理表述以 [architecture-T341-flush-tax-reattribution-v1.md](architecture-T341-flush-tax-reattribution-v1.md) 为准。
- 2026-10-31 **Amendment 3（T341 归因，见 [architecture-T341-flush-tax-reattribution-v1.md](architecture-T341-flush-tax-reattribution-v1.md)）**：T337b 证伪 T335 摊销模型后二次归因——固定税机理 (a) 变体（GPU 空闲后首次真实提交的 DVFS 频率爬升+驱动前端恢复，count 无关；同 pid 每 run 复现排除进程级惰性初始化；零工作 glFinish 不触税）。**§2.2 增补：FullM1Transport::Create() 尾部预热真实 flush**（1 workgroup dispatch+glFlush，每 run 冷窗口一次，把税挪出热路径）；INV-10（warmupWallNs 原值披露+stage 42 SubmitWarmup；预热在冷窗口非计费期，不构成隐藏长根）；字典 T324_STAGE41_V1→T324_STAGE42_V1（旧 1-41 不变）。降级预案登记（预热真机失败才启用：回退分批 flush+接受末批单次税+GAP 窗重议）。验证判据 W1-W5（stage 42 每 run 恰一次/热路径首批 ≤8ms/flush 零税/accepted ≥1/六项+K5 保持）。T340 包裁定：T338b（7aafce3f）即刻执行 M1/M2/M4，fence 线独立。
