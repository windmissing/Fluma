# T439 · GPU 体验能力与输入独立基线报告

2026-11-09，tester，P5，不使用真机。依 T433 §8 B-T 冻结显式 GPU 入口的独立契约 fixture 与可复跑判读器。T438 产品入口未实现：依赖项按 NOT_IMPLEMENTED 登记，不把模型自测冒产品通过。

## 交付

- `test/t439/capability_baseline.cjs` — 独立契约基线判读器（CLI 无参，exit 0/1，verdict PASS，32 entries）
- `test/reports/t439/t439_baseline.json` — 判定原件

## 能力契约（C1–C4，与 T433 analysis JSON + app_bridge.cpp 源码逐位核对）

| 项 | 冻结结果 |
|---|---|
| 常规 profile | `GRS_APP_M1=true`、无实验宏 → `setProgressV2` 返回 `PROGRESS_V2_UNSUPPORTED`（app_bridge.cpp:931 `#ifndef` 分支源码核对），页面显示明确能力结果，不伪选中 V2 |
| 实验 profile（T421-release） | 实验宏=true、defaultProgressV2=false，V2 需冷选显式新 run；V1/V2 不同 run 切换；120Hz 物理与 30Hz 显示不同频 |
| configureM1 | 只接受 k∈{192,512}×kw∈{64,128}×feature=3×revision=2，否则 `UNSUPPORTED_M1_CONFIG` |

## 输入契约矩阵 25 项（拒因独立冻结，源自 controller 实际语义）

覆盖：down 五类拒因（PAGE_INACTIVE/POINTER_ACTIVE/NOT_READY/INVALID_POINTER/OUTSIDE_BOX）、UP 停源而已成功 J 继续落水、tick 时钟门（CLOCK_GAP_NO_CATCHUP >33,333,333ns/回拨、INVALID_CLOCK、REQUEST_ID_EXHAUSTED）、传感器（INVALID_SENSOR、与手动互斥、|deg|>30 拒、切换需重新 down）、前后台（BACKGROUND/RESUMING、恢复需 down）、重置/换档（RESET_NEW_EMPTY_RUN/PROFILE_NEW_EMPTY_RUN 后 snapshot=null 必须重新观察）、旧 token STALE 拒绝。

## NOT_IMPLEMENTED 登记（5 项，等 T438）

I20 常规 profile V2 拒绝的实际页面呈现、I21 实验 V2 冷选新 run、I22 不导入 CPU/M0 snapshot 的显式入口、I23 内存不足明确 unsupported/fault、I25 UP 后 J 继续落水的实际入队观测。这些依赖 T438 显式入口接线，本卡只冻结预期输入与库存 token 序列。

## 复用与边界

- 复用 T411（归档完整性）/T412（整数配额 oracle）/T416（token 链判读）做库存 token 序列核对，不重写物理
- 30fps 只约束显示目标，不放宽 33,333,333ns 真实控制门；模拟秒/墙钟秒/新内容帧分列报告（T433 §6）
- 本卡只交独立输入/能力预期；产品差分（T438 到件后）另卡，不卡 T408 设备或开发关卡
