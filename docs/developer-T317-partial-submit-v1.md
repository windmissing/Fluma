# T317 · T324 分片提交合同实现交付（partial-submit 有界真实 Submit）

2026-10-30，developer；依据 `architecture-T324-gpu-partial-submit-contract-v1.md`（T324 §6 路由：本卡按合同继续 T317 剩余项）。不使用真机。

## 已实现（对应 T324 合同逐条）

- **§2.1/2.2 分片状态机**：`FullM1Transport` 新增 `SubmitPhase{IDLE,BATCH_ACTIVE,FENCE_PENDING}` 与 `TransportCode::OK_SUBMITTING=16`（追加枚举）。`SubmitQuantum` 保留全部原校验/Retire/PinCandidate/BeginQuantum（一次性），staging 后发首批并返回 OK_SUBMITTING；`ContinueSubmit` 同 nonce 续批；末批（表耗尽）在原位置建 fence+glFlush+stats 后转 FENCE_PENDING。批预算双限：`kBatchBudgetNs=8ms`（合同值）+ `kBatchMaxOps=10` 顶层 op（38 顶层 op 序按合同"~4 批一窗"算术；见下 INV-3 说明）。单 op 超预算独占一批（§5 边界）。
- **pass 表（INV-3 逐位）**：`QuantumCandidate::BuildPassTable` 把原 Enqueue 内联 pass 序（0..20 含 2×color3、4×Sort 展开）**加**原 SubmitQuantum 尾（21..28 含 bind3 三切换/traceOutputBytes 通道/uUploaded@27/TestHook 20/25/27 边界）编码为固定 `QuantumPassOp` 表；`RunPassRange` 按 op 执行所有原副作用（uColor/uSortK·J/binding3/输出字节/上传字节）。192×64 展开为 113 pass、512×128 为 136 pass；**trace stage 序与单提交版逐位一致已断言**。
- **§2.3 Run continuation**：`m1::Run::Pump` 新增 submitting 分支（inflight 之前）：ContinueSubmit，OK_SUBMITTING 返回 OK 下轮续；OK 转 inflight（现行 Poll 语义）；root 构造与 Add64 移入非 submitting 分支——**nonce 在 OK_SUBMITTING 即消耗，BUSY 只在 IDLE 整机忙时出现，重复 Begin 结构性消除**。`HasFlight()`/`SourceInFlight()` 覆盖 submitting（pause 窗口 drain 可见）。
- **§2.4 CancelSubmit/drain**：新 `CancelSubmit()`——BATCH_ACTIVE：owner.Stop+unpin+failed+IDLE（dispatch 不可回滚，root 永不 inflight/accepted）；FENCE_PENDING：glDeleteSync(fence)+同复位。`Run::Pause` 在 submitting 时先 CancelSubmit 再 Stop，**不进 quarantine**（quarantine 保持只对 inflight）。active/quantumFlight 在 cancel/fail 时清除（Release 不再被未复位 active 卡死）。
- **§2.5 总墙钟**：`submitFirstNs/submitTotalWallNs/lastBatchWallNs/batchesIssued/lastBatchPasses/submitCancelled` 全程原始值披露；GAP 判定/撤权/新 run 原值不动，不钳不藏。
- **§2.6 字典**：`SubmitStage` 追加 `SubmitRootBegin=40`（每 root 首批一次）、`SubmitCancel=41`；导出 `stageRevision=T324_STAGE41_V1`；`GpuBridgeTimelineCold.ets` 接受 23/39/41 三版本（旧档证据可读），41 时 stages 数组含两个新名。
- **M0（合同 §3 二期）**：`ResidentRun`/`SourceGpuTransport` 路径零改动——M0 bridge 10 模式回归在新字典下 11/11 通过。

## 关键语义修正（实现期发现，合同细化）

合同 §2.2 写"Fence 后调用 → 等价 Poll"，实现为：**FENCE_PENDING 的 ContinueSubmit 返回 OK**（root 移交 Run 作 inflight，由现行 Poll 路径独占接受）——若返回 OK_SUBMITTING 会让 Run 永久滞留 submitting（app 全链测试实证：cont 计数 2490 不收敛）。partial/cold/app 三套测试已按此语义断言。

## 非机证据（设备 NOT_RUN 如实登记）

| 套件 | 结果 | 覆盖 |
|---|---|---|
| `test/t324/partial_host.cpp`（真实 GL PC ANGLE pbuffer）×{192×64,512×128} | **19/19 ×2** | 分片完成（13/15 批、113/136 pass、trace 逐位、零 GL 错、audit=1023 同单提交版）；submitting 期间二次 Submit 拒 BUSY；FENCE_PENDING Continue→OK；FENCE_PENDING 取消（accepted 不变、无 fence 泄漏、Release OK）；BATCH_ACTIVE 取消（IDLE、后续 Submit PROTOCOL_REJECTED） |
| `test/t324/run_contract.cpp` | **9/9** | nonce 一次消耗、单 Begin、continuation 调用序、seal→inflight→Poll、Pause cancel-before-stop、无 quarantine |
| `test/t324/app_gl.cpp`（原 t317 app 循环适配：脚本时钟保持原节奏，轮上限 2500） | **6/6** | 全链：source+quantum 双 root 接受（accepted=2/sourceCommit=1/physicsCommit=2）、CURRENT、requested==rendered token、rawGL/EGL=0；quantum 经 14 轮 continuation 完成 |
| `test/t324/bridge_test.cpp`（t317 十模式 + 41 字典） | **11/11** | 8ms 等待/40ms/187ms 初始化/旧 epoch 撤权/78ms Submit/40ms Poll/BUSY 重试/Pause drain 全保留 |
| `node test/t324/cold_units.cjs` | **23/23** | T306 原 20 项 + 41 字典接受/42 拒绝/分页防切换 |

构建：生产 hvigor BuildNativeWithNinja（5.9s 实跑）+ CompileArkTS（10.6s 实跑）全绿；SignHap 00303116 已知签名配置失败如实保留（未手签不冒签包）。宿主编译 MSVC /O2 零错误。

## 修改文件

| 文件 | 用途 |
|---|---|
| `entry/src/main/cpp/gpu_resident/source_transport.h` | TransportCode::OK_SUBMITTING=16（追加） |
| `entry/src/main/cpp/gpu_resident/submit_observer.h` | SubmitStage 40/41（SubmitRootBegin/SubmitCancel） |
| `entry/src/main/cpp/gpu_resident/m1/quantum_candidate.h/.cpp` | pass 表（BuildPassTable/RunPassRange/bindTargets/pubWords/uploadedStage33）；Enqueue 转 staging |
| `entry/src/main/cpp/gpu_resident/m1/full_transport.h/.cpp` | SubmitPhase 状态机、ExecuteBatch、ContinueSubmit/CancelSubmit/FailSubmit、遥测字段、Poll 门 FENCE_PENDING |
| `entry/src/main/cpp/gpu_resident/m1/run.h/.cpp` | submitting 分支、nonce 消耗时点、Pause drain 序、HasFlight 覆盖 |
| `entry/src/main/cpp/gpu_resident/bridge_timeline.h` | stageRevision→T324_STAGE41_V1 |
| `entry/src/main/ets/physics/GpuBridgeTimelineCold.ets` | 41 字典接受（23/39/41 三版本） |
| `test/t324/{partial_host,run_contract,app_gl,bridge_test}.cpp, gl_extra.cpp, cold_units.cjs` | 五套非机验证 |

## 边界与未做

- 单 Pass 超 8ms 不可切（GL 无 dispatch 内抢占，合同 §5）——真机 41 阶段数据若显示单 Pass 超窗，回流 designer 评估 shader 工作分区；**设备逐批 ≤8ms 验证归 T318 真机**。
- M0 SubmitRoot 分片为二期（合同 §3）；M0 路径行为逐位不变已由 bridge 回归保证。
- app_gl 集成测试的脚本时钟节奏沿用 t317 原版（accepted 跳 8.4ms+1ns 步进），轮上限调至 2500（分片 root 需 ~14 轮 continuation；原 180000 上限对应单轮提交节奏）。
- 不改物理/审核/守恒/时钟阈值；GAP/CLOCK_GAP/撤权/新 run 原值原语义。
