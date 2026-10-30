# T282 READY 原子发布与队列修复

2026-10-30，developer；原卡 `6ae3e0eae4b0efba6938f71b`；消费 T269 §5.1。不使用真机。

`app_bridge.cpp` 共享权限迁入 `bridge_state.h`，仍由原 App mutex 唯一保护。EGL/transport 创建期间只持局部资源，完成后 `CommitReadyLocked` 同锁检查 epoch、active、held window/current window，再一次提交 identity/input/readyEpoch/ready/phase/status；失配仅退休局部资源。invalidate/setActive 同锁增代、撤权限、清 app 队列并发布 STATE_LOST；窗口引用失败亦撤权，不保留旧 READY。FAULT、CLOCK_GAP 同锁撤权。所有 worker 状态/完成写回再核当前 epoch/surface；旧初始化或完成不能覆盖新状态。

offer 要求 `permissionState=READY && readyEpoch==epoch && active && window`。queue 每项绑定原 packet、epoch/input；worker 调 ResidentRun::Offer 前再核当前权限与 GPU 实际 inputGeneration，旧项拒绝并计 cancelled/staleQueuedRejected，不改其32 word。呈现状态 CURRENT 与输入权限 READY 分列：CURRENT 仍只来自 shader/swap，权限 READY 不是 ACK/成功物理。状态 JSON 在同一 mutex 内序列化完整快照，新增 readyEpoch/permissionState/staleQueuedRejected。runQueueDepth 明确为 POST_PUMP_OR_POST_PAUSE；Pause 后刷新实际值，invalidate 后0，不把旧采样当待处理队列。

验证：生产 CommitReadyLocked 精确在创建完成/发布之前插入 invalidate、active=false、新run代和 surface replacement（含不增代的指针变化）；旧 READY 全部拒绝。并测正常发布、原 packet 位型、旧 epoch/输入代消费、队列满/取消、2000次 invalidate/发布与并发读快照。**18/0**。既有真实 Node N-API **17/0**；CLOCK_GAP/ResidentRun/取消 **14/0**。`Fixed120Clock`、120Hz、F≤4、33,333,333ns、carry和物理核未修改。全部 XComponent+shader，无 Canvas 新路径。

`default@PackageHap` BUILD SUCCESSFUL；签包及verify PASS。unsigned SHA256 `99d566b1c26264f29034edbe1b234b7381a4013265767dc1a48a859eda02442e`；signed SHA256 `21804317436c0fb0e212f24a6c7c098eed97e2336b602c04f540892348d2039e`。签包位置 `.codex/local-test/t257-sign/T282/T282-signed.hap`；签名证据 `test/reports/t257/T282.json`。未安装/启动/用机，不宣T283产品闭环PASS。

复跑 `test/t282/build.cmd`、`test/t272/build.cmd`、`test/t272/clock.cmd`；原始输出 `test/reports/t282/{state,napi,clock}.txt`，构建 `package.log.gz`。完整实际 EGL/window/N-API 在 OHOS 的生命周期续证由 T283 执行。
