# T299 事务 backend 设备受阻首次归因 · 工程与证据修订 v1

2026-10-30，architect，原卡 `6ae40da4e4b0f69bc56e4d51`。消费 T297 原卡评论/正式报告、`test/reports/t297/` 36 份 JSON 与 11 份 txt、T250 BEFORE v30 字典及签包源树、T294/T245/T256。只做离线归因；不使用真机、不重跑测试、不改业务代码。该旧 pair 的问题不阻断 GPU 驻留 M1 或 T228。

## 已有原档的精确界线

T297 的 `txgpu-t297scan1.json` 是 `LEGACY_SCAN/WET_192_64/COMMON_FREEZE_OFF`，PID 8735、`suite=false`、34 root、`COMMITTED`；hilog 记录同 run `SEALED`。`txgpu-t297L_COMMON_FREEZE_OFF/ON.json` 分别 PID 12581/13208，均 `suite=false`、各只跑一个 LEGACY backend、34 root、`COMMITTED`。现存 **36 份 JSON 各有不同 PID，全部 `suite=false`、`suiteRows=[]`、backend 均为 LEGACY_SCAN**。因此报告/原卡所称“两臂同 Worker/PID suite 六路”无原档支持；两臂是两个进程的单独运行，既没有 AVL/GPU 的 suite 行，也不构成同 PID 三 backend 成本比较。OFF/ON 的 4875/4722ms 和 5822/5678ms 分属不同 freeze 臂、不同 PID，不能合并为同负载 p50/p95 分母 2。首启 `TX_GPU_WANT_FIELDS` 经 `txSteps --pi` 修正后 LEGACY 可运行，仅说明 Want 类型问题已解决。

`avl_gpu_hang_evidence.txt` 仅有首次 Want 错误与 LEGACY 的 `SEALED` 行；六个 `hilog_t297s_{TX_ADJ_AVL,GPU_PAIR_V1,LEGACY_SCAN}_{OFF,ON}.txt` 中四个 AVL/GPU 文件为 **0 字节**，两个 LEGACY 文件各重复同一 537B LEGACY 首启行。`cut18_hang.txt` 为 0 字节。没有 AVL/GPU 对应的 Want 原字节、PID/TID、worker 已收命令、阶段/最后进度、CPU TIME 连续采样、线程栈、native GL/fence 返回或导出错误；“无 SEALED/FAULT”与单独 `ps TIME` 不变均不能推出 worker 死锁，甚至不能证明其进入相应 backend。cut18 root1 的“>2min 无文件”也没有留存配对原时序。这里应登记 **设备证据不足，原因未定**，不派未经定位的算法/死锁修复。

## 20 BEFORE cut 口径校正

`cut-inventory.json` v10 的 root：cut1/2→1，cut3/4/6–11/13–15/19→2，cut5/12/16–18/20→3，cut5 另有 A/B ordinal 1/2。真机 `txgpu-t297c2_6.json` 为 root2/cut6，`faultPlan.hit=true`、`AFTER_ORIGINAL_CHECK.firstCode=4`；`beforeEvidence.hit=false` 是因为 cut1/6/18 走旧 `TxCutFault`，不能当未命中。`txgpu-t297c2_18.json` 已归档 root3/cut18，`faultPlan.hit=true`、原 `firstCode=1`、父 step receipt `errorCode=5`，因此“cut18 未归档”仅适用于无原档的 root1 尝试，**字典要求的 root3 已有设备档**。`txgpu-t297c2_9.json` root2/cut9 的 `beforeEvidence` 有原 `TX_GPU_NOT_REUSABLE:STALE` 异常及原 caller 映射 13；其 `TX_GPU_ADMISSION_FAILED` 是夹具要求全量的终止提示，不能充原物理码。`txgpu-t297cut9.json` root1 `COMMITTED/hit=false` 与字典 root2 无矛盾。

按现存文件，cut1–19 在各自字典 root/ordinal1 有一个 hit=true 的设备档（1/6/18 读 `faultPlan.hit`，其余读 `beforeEvidence.hit`），**这只证明命中范围**；不能用 `SEALED` 或最终 `TX_GPU_ADMISSION_FAILED` 一键推断 19 点原码、完整 rollback 与事件 lineage 全 PASS。cut20 仅有 root1 的 `txgpu-t297cut20.json`，`COMMITTED` 且两个 hit 均 false；字典要求 root3，观察提交保留的故障证据 **NOT_RUN/缺档**。cut5 ordinal2 以及两 freeze 臂各 20 cut 的设备分母也未形成；当前 cut 档全部 `COMMON_FREEZE_OFF`。物理源/池/事件原字节和成功前缀应逐点用冷档 checker 校验，不能凭 `rollback=ORIGINAL_STATE_BYTES_ARCHIVED` 字符串判逐字节通过。

## 分流与接力

**tester 先补精确设备事实（真机，串行）：**用原签包 SHA `3b87eee5…` 和唯一 runId，先对 AVL、GPU 各单 backend、小步数、分别 OFF/ON 启动，留原 Want（`txSteps` number）、App/worker PID/TID、启动/进程存活和连续 CPU TIME/线程状态、全窗口 hilog、目标 JSON 是否存在/大小及拉取错误；每次明确 `suite=false`，不要将单独新进程混写成同 Worker suite。至少给同一进程下命令已 post、worker 是否收命令的证据；若没有阶段证据，明确标不可区分 worker/Native/GPU/导出。无响应时采有界线程栈或系统调度/GL 锚点，区分阻塞、长计算、被挂起或进程退出。对已有 cut 档先用正式字典的 checker 重算每点 root/ordinal、hit 所属对象、原 consumer code、rollback/成功前缀和 archive SHA；报告勘误两臂 PID、cut6/18、cut20 与分位数。随后仅补字典 root3/cut20、cut5 B 及未覆盖 freeze 臂，避免无条件重复 1–19 已有命中。测试失败/缺证回 architect，测试用例分母或口径争议由 manager 裁。

**确定存在的工程观测缺口，先由 developer 修订后的本合同实现（不使用真机）：**`TxGpuPageLauncher` 只在 `onmessage` 打终态，不记录 worker `onerror/onexit`；`TxGpuWorker` 在 `runTxGpuFixture` 返回、`suiteRows` 跑完且 archive fsync 后才发唯一 `SEALED/FAULT`。故无终态时无法定位停在 Want/worker 启动、fixture/backend、Native configure/select/fence、JSON 编码或 fsync。隔离 runner 增固定容量、低开销阶段锚点与冷导出：包 SHA、runId、PID/TID、backend/freeze/fixture/steps/suite/faultPlan 身份，`WANT_VALIDATED/WORKER_CREATED/COMMAND_POSTED/COMMAND_RECEIVED/NATIVE_CONFIG_ENTER+EXIT/ROOT_ENTER+EXIT/ARCHIVE_ENCODE_ENTER+EXIT/FSYNC_ENTER+EXIT/TERMINAL`，单调时间与序号、lastCompletedRoot、firstError、dropped/incomplete；launcher 记录 `onerror/onexit` 原错误。根/阶段锚点不能泄露大 JSON、不能逐帧无界日志。确保失败归档保留原错误与已完成前缀，单 Worker owner 与真实 native EGL/GL 线程约束不变；不改事务/故障码、源算法或生产默认路由。先编译、非机注入卡在 backend/Native/编码/fsync 各段核阶段归属，再签包交 tester；设备稳定或完整 T251 不是开发关闭门。

若补证指向具体工程断点，再按原码/栈修工程并转 developer；若是设计不变量冲突转 designer；若只是时窗/设备调度，保留设备受阻并要求可复现场景。不能用本报告证明 AVL/GPU 真机 FAIL 的唯一机制，也不能宣三 backend、20 cut 双臂、p50/p95 或外部 driver 总峰值 PASS。任何呈现仍限新 XComponent+shader 前景/背景，故障 FAULT 或显式完整旧 token STALE，禁 Canvas 上屏或回退。
