# T488 · GPU 对称业务脚本异步预约与 lease 修订（v1 阶段：E lease 全链 12/12 全过 + D1 输入等价/全账一致 + D2 RESERVED 达成；D1 SOURCE root 消费正控缺证交 architect）

2026-11-13，tester，离线不使用真机，commit `0bbc048` 基础上新增本卡文件。按 T478 v2 工程合同（`docs/architecture-T478-symmetric-driver-lease-v2.md`，355d632）在**新测试目录 `test/t488/`** 重建 T471 驱动器与 lease 用例；被测生成来源冻结为 `t475-handoff-final`（M1 路径，T467/T475/T482/T483 修复叠加；build.cmd 固定引用该生成源，hash 见 script manifest 与 run log）。T471 旧件按原 hash 保留未补造。

## 一、交付物

- `test/t488/driver_lease_diff.cpp`：D1 固定动作表差分 + D2 独立控制线程预约 + E 真实 transport lease 序列（单进程三阶段）。
- `test/t488/build.cmd`：MSVC 构建 + 运行（阶段 ENTER/EXIT 持久化到 `t488-stages.ndjson`，供外部 wall 守护读取最后阶段）。
- `test/t488/gl_wrappers.inc`：GL 符号转发包装（提取自 T475 gl_test，避免 main 重复定义）。
- 原件：`.codex/local-test/t488/`（script manifest、逐动作 JSONL 381+ 行、stages.ndjson、六轮运行日志）；汇总 `test/reports/t488/t488-summary.json`。

## 二、D1 固定脚本差分（T478 §2.1/§3）

- 不可变动作表 41 action（6 预热 pulse → SOURCE 沙 → 3 QUANTUM → RESERVE → 9 QUANTUM → SOURCE 水 → 18 QUANTUM → END），业务动作与 pulse 同拍（先业务后 pulse，修 T471 非业务拍占用间隔触发 CLOCK_GAP 缺陷）；执行前落盘 manifest（id/kind/scriptNs/seq/payloadHash）。
- **漏拍负控已拒**：try_lock 失败重试同一 action（不 continue 跳拍）；两臂送达 actionId 逐条一致（DELIVERED_ACTION_COUNT 40/40 PASS）。
- **输入等价达成**：generated 69/69、revoked 8/8、overload 61/61、carry 0/0、clearedCarry 120000000/120000000、pending 0/0、active 0/0、completionCount 0/0——**T441 §7 全部账两臂一致（NO_OBSERVATION_DISTURBANCE PASS）**。
- 修复 T471 三处判据缺陷：① `cmp(acceptedCount, generated)` 改为逐独立账目对拍；② 空-空 completion 不再记 PAIR_OK（本卡 completion 0/0 不计通过）；③ 记录性 `++passed` 全部删除（26 项 assert 均由 bool 结果计一次）。

## 三、D2 独立控制线程预约（T478 §2.2）

- **D2_BUSY_RESERVE PASS**：owner 在 PumpBegin 切点暂停（mutex 外），独立 control 线程调用生产 `ReserveEvidenceLocked` → `RESERVED` + `blocker=WAIT_ROOT` + 不直接 ARM（真实生产接线，非测试直写 evidence.armed）。
- **D2_ARM_AT_CHECKPOINT FAIL（缺证）**：owner 恢复后生产 `captureCheckpoint` 未达 ARM——与 D1 SOURCE root 未消费同根（见五）。

## 四、E 真实 transport 一次 Create 复用 + 正确 lease 序列（T478 §4）——12/12 全 PASS

| 断言 | 结果 | 说明 |
|---|---|---|
| E_CREATE_OK | PASS | 真实 `FullM1Transport::Create(192,64)` code=0（真实工厂三 program 路径） |
| E_SOURCE_OK | PASS | 真实 SOURCE 接受（drain OK） |
| E_ACQUIRE_OK | PASS | Acquire(S=1,G1)=OK，lease 身份一致 |
| E_ACQUIRE_DUP_BACKPRESSURE | PASS | 重复 Acquire 同代 =BACKPRESSURE(4) |
| E_RELEASE_OK | PASS | ReleasePublication+fence=OK |
| E_SIGNAL_REACQUIRE / E_SIGNAL_REACQUIRE_RETURN | PASS | **真实 fence 已 signal** 分支：同 G 再 Acquire OK 且归还（T471 把即时 signal 无条件判背压的缺陷已修） |
| E_SOURCE2_OK / E_SOURCE3_OK | PASS | slot 交替（1→0→1，产品合同：`publicationSlot==lastPublication` 且 AcceptedCount>0 → PROTOCOL_REJECTED） |
| **E_SAMESLOT_OLDGEN_STALE** | PASS | **同 slot 覆盖后旧代 STALE(5)**——T471 只在另一 slot 生成新 publication 不能证明旧代失效，本卡补第三 root 回同 slot 完成 T478 §4.4 判据 |
| E_SAMESLOT_NEWGEN_OK | PASS | 新代 G3 Acquire OK |
| E_TRANSPORT_RELEASE | PASS | Release code=0 |

悬空引用已修：E 守护不再 detach 引用局部变量（阶段 ENTER/EXIT 落盘 + build 层外部 timeout 承担守护）；`glFinish` 后期待旧 G STALE 的违反产品合同用例已改按真实 fence 状态分支判定。

## 五、缺证与移交（不冒 PASS）

**D1 SOURCE root 消费正控未达成**：两臂 SOURCE 均 `ENQUEUED_NOT_ACK`（READY 成立、边界/样本有效）但 `sourceCommit=0`、`accepted=0`、overload=61（active 计划积压丢弃）——root 从未被 worker 消费。连锁导致 CAPTURE_FROZEN/BEGIN_PREFIX_VALID/D2_ARM_AT_CHECKPOINT 三项 FAIL（无接受 root 即无可冻结档/无空闲 checkpoint）。

归因线索（交 architect）：
1. T459（同 App 构造、同队列消费路径）offer 在 **Wake hook 内（worker 线程）** 时 accepted=8 可行；本卡 D1 offer 在协调器线程，消费点 `FrontLocked` 的 progressV2 门（`!run->Tick().active` 时才消费）在 16ms 拍下未放行——`run->Tick().active` 的置位时序与外部线程 offer 的交互是首要归因点；
2. 或将该消费门在协调器 offer 下的行为与 T459 hook 内 offer 做差分定位（两臂已证输入等价，差异只剩线程归属）。

按 T478 §3 约束：相同送达后的分歧保留原数据（`t488_log.ndjson` runId `t488-702877288780900` 全量逐动作回执），不在测试里改 expected 追认。

## 六、边界

- 未改产品代码/线上接口/时间阈值；未强写 READY/ARM；漏拍负控能拒已证。
- T422 S1/S4 等价覆盖复用（不重复全套五场景）；T471 不重开。
- 六轮迭代记录（run1–run6）均在 `.codex/local-test/t488-run*.log`，判据修订轨迹可溯。
- E 段时长不冒设备期限结论（SwiftShader JIT；TRANSPORT_CREATE 实测 ~470s 墙钟已入 stages.ndjson，仅作 HOST_ONLY 记录）。
