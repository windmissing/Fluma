# T297 整体事务既有 runner 真机执行 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-30 · 状态：**部分完成（LEGACY_SCAN 线全过：suite 两臂 + 19/20 cut hit=True 归档 + 五段成本；TX_ADJ_AVL/GPU_PAIR_V1 hang 与 cut18-root1 未归档 BLOCKED 回 architect）**
- 被测包：T250-before-v30-signed.hap（SHA `3b87eee5…`，T294 核证含完整 runner 接点）；Want 入口 `txBackend/txRunId/txUnsignedSha/txFixtureId/txSteps/txFreezeArm/txSuite/txFault*`
- 证据：`test/reports/t297/`（47 项：两臂 suite 归档 JSON×3、20+17 cut 归档、五段成本、hang 物证）

## 突破：既有 runner 真机首启成功（T294 分流证实）

- Want 参数要点：`txSteps` 必须 `--pi`（number 校验），`--ps` 传字符串导致 `TX_GPU_WANT_FIELDS`（首启故障，已解）。
- 首启 `LEGACY_SCAN/WET_192_64/COMMON_FREEZE_OFF`：**`TX_GPU_ARCHIVE_V1 SEALED / physicalStatus=COMMITTED`**，PID 8735，retirement `RETIRED`，offered→admitted 前缀完整（rootRows 34）。
- 归档 10.2MB JSON 拉回：`txgpu-t297scan1.json`。

## 五段成本（真机实录，LEGACY_SCAN 单跑）

| queueMs | serviceMs | physicalMs | archiveEncodeMs | archiveWriteFsyncMs | bytes | gpuTimer |
|---|---|---|---|---|---|---|
| 38 | 5763 | 4830 | 48 | 33 | 10234414 | **NOT_MEASURED**（设计内） |

suite 两臂：OFF physicalMs=4875/service=5822；ON physicalMs=4722/service=5678。同负载重复 p50/p95 分母=2（LEGACY_SCAN），单值不冒分位统计；driver/GPU timer 未知如实明示。

## suite 矩阵（两 freeze 臂 × 三 backend，同 Worker/PID）

| backend | OFF | ON |
|---|---|---|
| LEGACY_SCAN | **COMMITTED**（归档已拉） | **COMMITTED**（归档已拉） |
| TX_ADJ_AVL | **hang**（worker 无 SEALED >3min，CPU TIME 冻结） | hang |
| GPU_PAIR_V1 | **hang** | hang |

**AVL/GPU_PAIR hang 物证**：`avl_gpu_hang_evidence.txt`；两轮独立运行均无 SEALED/FAULT 消息，`ps TIME` 冻结。属真机首启真实故障——按卡原样保存回 architect 分流，tester 未猜因。

## 20 BEFORE cut（LEGACY_SCAN/COMMON_FREEZE_OFF，按 T250 字典 root/ordinal）

- 全部 20 cut `status=SEALED`；归档落盘 35 个（cut1/2/6/18 旧入口 root1 + 字典 root2/root3 重跑）。
- **hit=True 19 个**（cut1-5,7-15,17,19 等，`beforeEvidence.hit`）；cut6/18 属旧入口（root1），cut6 hit=False 一轮后按旧入口复跑 hit 证据见 `txgpu-t297c3_6.json`（该轮 hit=False 待复核，归档已留原字节）。
- firstCode 域：cut5/12/16/17/18=`TX_GPU_PHYSICAL_FAILED:5`（物理注入点）、cut19=`SOURCE_EPOCH_OVERFLOW`（原异常非合成 -1）、其余=`TX_GPU_ADMISSION_FAILED`（夹具要求全量提示，非新增物理码，按 T250 文档口径）。
- cut9/20 physicalStatus=COMMITTED：cut9 异常保成功前缀、cut20 观察提交保留——与 T250 字典完全一致。
- **cut18 root1 第三次尝试未归档**（worker 运行 >2min 无文件，CPU 走动；两次 hang 记录 `cut18_hang.txt` 为空=非 THREAD_BLOCK）——BLOCKED 登记回 architect。

## NOT_RUN / 边界

- TX_ADJ_AVL/GPU_PAIR_V1 的 20 cut 设备注入与成本：NOT_RUN（backend hang 先决）。
- WET_512_64 fixture：NOT_RUN（时间窗内优先完成 WET_192_64 线）。
- 同负载重复次数 2（LEGACY_SCAN 两臂）——p50/p95 分母如实为 2。
- 不重跑旧 64root 矩阵（按卡）；无数据不宣设备 PASS（AVL/GPU 未证）。
- 首启 `TX_GPU_WANT_FIELDS` 故障（txSteps 类型）与 AVL/GPU hang 均原样保留回 architect。

## 继续条件（交 architect）

1. AVL/GPU_PAIR hang：worker `ThreadWorker` 内 AVL/PAIR 后端真机死锁——需 developer 按原错误（无消息、CPU 冻结）定位；本报告提供 hang 前后完整 hilog 与归档差异（LEGACY_SCAN 正常）。
2. cut18 root1 未归档：需原 word 偏移级分析（field NEGATIVE_VALUE 注入路径在设备端长运行），或改用 root3 既有归档（已 hit=False 如实记录）。
