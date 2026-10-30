# T302 GPU M0/M1 真机时间线冷出口首次归因 · 工程修订 v1

2026-10-30，architect；原卡 `6ae41460e4b0f69bc56ef2c5`。消费 T296/T301 原卡与报告、`test/reports/t296/`/`t301/` 快照、T295/T281 交付及当前 `app_bridge.cpp`、`bridge_timeline.h`、两张产品页面。不使用真机、不重跑测试、不改业务代码。本稿接续 T291，限实验产品诊断通路；M0/M1 的 120Hz、F≤4、33,333,333ns、carry、根提交/完成和 sourceCommand32/completion256 均不改。

## 事实与归因

T295 的 `libgrsm0` 已注册 `timeline(cursor)`，同一 `app_bridge.cpp` 由 `GRS_APP_M1` 编成 `libgrsm1`，注册表也含此 NAPI 属性。`BridgeTimeline` 有 1024 条 first-preserve 行、单调 `steady_clock` ns、串行号、TID、epoch/run/surface/input 及 Wait/Wake/Anchor/Advance/Pump/Draw/Swap/Pause/Offer 等阶段；每页 4 行。T296 的 M0 状态中 `timelineSerial/count=77/77,dropped=0`，T301 M1 快照亦有 timeline 字段。实际 `GpuResidentM0.ets` 只每 100ms 调 `status()`，`GpuResidentM1.ets` 只经 `GpuM1DemoController.refresh()` 调 `status()`；两页均没有 `timeline()` 调用、冷导出动作或文件交付。**这是已证的页面到已存在 NAPI 的工程接线缺口**，不能再仅增加 native API 或要求 tester 从 status 三个标量还原行数据。T281 与 T295 是不同签包；同源注册不等于两包已获相同页面出口，新工程包须逐包核实际模块/页面身份。

T296 的 `fg_1` 与 `fg_40` 均为同 epoch/run 的 `clockDeltaNs=199246563`、`timelineSerial=77`、`PAUSED_CLOCK`；40 次采样是**一个末态平台**，不是 40 次独立超窗。`ret_15` 是新 epoch 的 180675469ns 末态；T301 的 104896094ns 同样只给最后超窗值和 `acceptedCount=0,lastOfferCode=NONE`，其原帧序列仍不可分账。READY 提交由权限状态迁移支持，READY 持续 `<0.5s/<0.8s` 及“人工亮屏前台 2s 必闭环”没有 worker 时刻证明；将 delta 称为挂起恢复的纯时长、将 Doze 定唯一因或将页面路由 PASS 升为整步 PASS 均越界。M0/M1 的 source terminal→成功 quantum/completion256→lease→同 token CURRENT 继续 NOT_RUN。

## developer 工程合同：真实可取的冷分页出口（不使用真机）

共用一个只在用户/测试显式触发时执行的 `GpuBridgeTimelineCold` ArkTS 收集器，由 M0、M1 各自当前页面调用其对应 `m0.timeline`/`m1.timeline`；在页面诊断入口显示/回报导出状态，文件写入当前 app 私有目录并 fsync，冷日志只输出短的文件名、包 SHA、字节数与摘要，避免 1MiB 热 hilog。收集器不能通过 `GpuM1DemoController.status()` 或页面 100ms 采样替代原 worker 行；不得在每帧自动遍历 256 页或阻塞 GL owner。冷调用每页最多 4 行，可分批让出 UI 事件循环，超时/取消/异常写显式 INCOMPLETE。

首读页固定 `revision=T295_STEADY_TRACE_V1`、capacity、rowBytes、count `C`、dropped `D`、epoch/run/包 SHA；随后 cursor 从 0 严格按 `next` 递增至 `C`，拒绝跳页、重复、非递增 serial、无效十进制 ns/U64、stage 未在版本字典、行身份与当前/历史 epoch 不一致或数据预算超限。first-preserve 行不会被覆盖，故导出开始时的前 `C` 行可作为稳定前缀；导出末再读 header/status，保存 `serial/count/dropped` 前后值。若写入期间增长、dropped>0、页的 `count<C` 或包身份改变，标明截取边界与 INCOMPLETE，不把前缀当整次会话完整。完整性清单写文件 SHA256、长度、页数、首末 serial、缺页/重复/溢出计数、模块名、签包 SHA、PID/run/epoch 与 M0/M1 route；原页/原行不因 JSON 转换丢 U64 精度（原十进制字符串原样保留）。`bridge_timeline.h` 的 stage enum 需发布固定映射和 revision；`completePage=true` 只表示单页序列化成功，不代表整次 trace 完整。

若期间同时发生新 run/epoch，先按行原身份分组，不能用页面当前 run 覆盖旧行；M1 重置/切源同理。出口读取不得重置环、清 pause、唤醒 worker、改源准入或状态；M0/M1 都保留 T282 READY 原子性、`PAUSED_CLOCK` 撤权和失败 FAULT/完整旧 token 显式 STALE。编译/非机检查必须覆盖两 module 实际导入、0/1/1024 行、多页、跨 epoch、导出中追加、dropped、页损坏、文件 fsync/摘要和中断；可部署签包及非机检查即 developer 交付，真机成功不加作开发门。

## tester 后续定点取证（新包，真机串行）

同包 SHA/PID 的 M0 与 M1 分别一次冷导出，保留文件原字节、校验清单和 status 原采样。按 `InitBegin→Ready→FirstTick→AnchorSet→WaitEnter/Wake→AdvanceBegin/End→Pump/RootSubmit/Poll→Draw/Swap→PauseBegin/End` 配对同 run/epoch/TID；计算 `last→WaitEnter` 的前轮工作、`WaitEnter→Wake` 的等待加调度、`Wake→下次 Stamp` 的后段准备，和初始化时长。`wait_for(8ms)` 不是调度保证，单凭等待段过长不能把具体毫秒全归 OS 挂起；需设备 scheduler/suspend 时间与线程 TID 对齐才可再分。若稳定前台的工作段长期超过 33ms，回 architect 判断工程热路径，再视不变量冲突转 designer；若只在设备调度缺口超窗，给可复现状态。不可钳 delta、补跑或把 READY 当 ACK。仅同一 K192 run 的真实 source terminal→quantum/completion256→lease→XComponent+shader 同 token 像素可升级产品最小链；完整 M1/收益另循真实前置。
