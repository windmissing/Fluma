# T688 · T587 fixture 回执与 lab 矩阵覆盖

2026-11-20，architect。不使用真机。T587 已按原回执登记完成，其中可保留的是**设备 UI fixture 的正常完成、排空及 Poll 记录落盘**。其余独立 driver 的真机矩阵由后续 effector 执行，原卡无需重开。

原件 job1/generation1、pid65246、route m2、合成 scope。30条原始 record 对象字节按 native 封存规则连拼SHA为 `8e19f2d2bf66495aaa8ec24bdc6cd5a106315c25ace51ad31da6fa5800efa515`。FINISHED/COMPLETED_FIXTURE，pendingResources=NONE，terminalNs 327416765099377 < pollSampleNs 327427769007502，cancelRequestedNs/cancelObservedNs 均0。原件无 BARRIER_ENTER/CANCEL 时点，UI 的释放按钮操作不能证明命中真实 checkpoint。输出 `lab-export.json` 是 Poll record JSON，文件路径及完整records SHA支持该落盘。真正 FilesV2 还需 Begin/Index/FilePage/End 原回包、每个fileId/offset与 full SHA。

**产物须按实际 SHA 区分。** 原报告 b1866e03 对应初版 `T668-signed.hap` 签收一致（lab .so BuildId f8e3a238…），与最终 handoff `T668-final` b102e174…/lab BuildId a7ba8f18…不同。当前设备没有同进程 loaded module 原件，不替最终 HAP 背书。独立 NDK `t668_driver` 的 SHA6243cb78…/BuildId7be66db1…另核，DT_NEEDED 为 libc++_shared.so 与 libc.so，执行不依赖上述 HAP/Ability。

| 域 | 本轮覆盖裁定 | 后续要保的原始结果 |
|---|---|---|
| 正常 UI 合成完成/排空/终态时序/Poll落盘 | 设备已证，原范围保留 | 不重复同条件UI四步 |
| 原生真实 checkpoint 取消、重复取消、终态后取消 | 真机未测，v4两取消时钟为0；历史V2标签不是本轮证据 | `barrier_cancel DIR`，先实际BARRIER_ENTER，第一次REQUESTED、重试ALREADY_REQUESTED（若已封则ALREADY_TERMINAL分档），取消observe与清理、DRAINING/FINISHED实际状态，Cancel后再Poll逐字段核terminalNs/outcome/firstFailure/recordsSHA不变，重复和错job原回 |
| Store满128、pop后fence、等位和插入后取消、5s不pop | T668 host已有以上分支，设备未测 | `full_queue_pop`、`full_queue_cancel`、`fence_cancel`、`deadline` 各新进程，保第129项CACHE_QUEUE_FULL、queuedAtEntry128、真实wait/pop/insert/fence seq/enter/return、128任务全部越过fence，超时独立记录原生 deadline与OS唤醒偏差，结束真实 Retire/join |
| FilesV2 完整分页+BUSY、错误exportId、错页/offset/fileId、结束后拒 | V4设备未证，离线parser正反控另计 | `files DIR`，初次Begin后第二Begin拒绝，错id End不关闭既有reader，完整index和4096B分页base64原件逐页核offset/chunk/full SHA与总长；正确End后旧ID拒绝；每种负控新export，记录原错误、pin/reader恢复 |
| TTL不续、原文件变化 | 设备未测 | `files_ttl DIR`：真实clock，至少一次到期前读，在原Begin起60s后拒且读不续期；`files_mutation DIR`：自有隔离目录Begin后同尺寸改一字节，FILE_CHANGED且关闭；合同中的截断/替换/删除由tester按同一原fixture变体覆盖，保原/变更SHA、分段返回及清理 |
| 注入时钟到边界/故障和 catalog 竞争 | 缺原始用例执行证据，适用独立合成试具，缺模式即 tester 补设施 | 沿T531/T635已列边界测试，明确标 INJECTED_CLOCK/INJECTED_FAULT。除已行使项外逐条列 NOT_RUN，不把host或注入计为可见设备行为 |
| 产品渲染/物理/帧截止；强制生产 BUSY、强制中断文件I/O | 合成 test 的非适用域；后两项非合同能力，不派产品实施 | 用真实独立产品矩阵，保持GPU与新渲染合同 |

保留 T587 v3 既有 UI 完成后结果未被改写的窄证及其独立包身份。T688要补的是原生 Cancel 回执与同job前后完整不可变证明。若要证明 Release 路也实际命中 checkpoint，先用原 `CHECKPOINT_BARRIER` 记录 `BARRIER_ENTER` 再行使 `ReleaseBarrier`，保存原调用、退出和 FINISHED；可复用现有端点由 tester 补一个 fixture case，不重跑 v4 的普通完成路径。

## 接力与完成判据

tester复用已交 ARM64 driver/同源Store和 files.py；当前 driver 以30ms sleep发取消或释放、只有一次FINISHED后Cancel，不能由打印 `EXPECTED` 推定阶段真实命中或终态不变。补测试夹具的有界阶段证据与精确断言：同 mutex 发布已进入等位或已插 fence 后再发取消，未进入时保准确超时；在释放/finalize不改生产锁序。用后置Poll做完整不可变联核；TTL正控需带到期前读，记录本次Begin和原生过期，现有一次睡61s只覆盖到期样本；files.py只校原文件流，负控状态、错ID保reader、资源回收需另看driver原件。已有设备原件若可取直接复用。先核缺项是测试模式或产物，实际生产接口错或缺才由 architect 定工程范围。

effector串行部署实际 ELF及对应libc++，独立可写新目录与进程。先检查签名/文件哈希、真实 loader/argv；总界普通≤30s、TTL≤80s，普通超时用真实退出/wait/stdout/stderr/MODE记录INCOMPLETE，不用外层kill冒内部5s成功。FilesV2上限32文件、单文件2MiB、总64MiB、4096B每页、单JSON16KiB、原计账与RAII/reader/校验约束不动。预算内完成剩余分支，并按设备/host/注入和未测分别保存原件；首次真实失败带原错、场景、模式、job/export 身份与证据回 architect。设备窗口只排队，不新增manager验收。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（本人状态有限例外）。GPU驻留、CPU停投，前景背景仅XComponent+shader，FAULT或明确STALE。
