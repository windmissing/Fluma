# T477 复跑问题修复

2026-11-11，developer，不使用真机。消费 tester 的 v2 报告、T482-final 实际生成链与两轮原始 manifest。

导出错误已定位到生产者与消费者的阶段字典不一致。T467/T475 为记录 driver flush enter/exit 使用 TailStage(141/142)，同时把该事件直接送入 BridgeTimeline；该页仍声明 T404_STAGE65_V1，ArkTS reader 严格拒绝 stage>65。第 120 行之后恰遇首个 flush 事件即可复现 INVALID_ROW。BridgeTimeline 实际保留固定前缀，满后增加 dropped，没有循环覆盖旧行；已有原件不足以把本错误归为并发覆盖。

修复将这两个事件正式登记为 BatchFlushEnter=66、BatchFlushExit=67，生产 native、T467/T475 叠加生成器和冷 reader 同步使用 T477_STAGE67_V1。保留旧字典对应的严格上限。flush enter 的 probeEnd=0 表示调用未返回，离线 reader 只在新字典该事件接受这一编码。cold reader 另保存有界被拒页的原始字节和 SHA，放在 rejectedPage 具名字段中，不计入有效行；发生丢行、增长或非法字段仍输出 INCOMPLETE。

运行问题的原始数据与“首 root 没有 Draw”的推断不同。两轮 manifest.statusBefore 都记录 newPhysicalFrames=2446，epoch3/session1，随后 PAUSED_CLOCK/CLOCK_GAP，clockDeltaNs=34,329,218，高于原门限 33,333,333。pageNativeGapMaxNs 是最后接受 pulse 的统计，不能据其较小就否定 supervisor 之后触发的 GAP。失败导出停在 120 行也不能证明后续不存在 Draw。该计数属于 native 呈现账，设备可见像素仍需独立验证。

工程修复让当前 frame 在同代 FAULTED_REVOKED 后保留原始撤权原因，停止发送已关闭 token；若 state 读取与 pulse 之间发生撤权，重新读取同代原因，避免 UI 长期用 STALE_PAGE_SESSION 掩盖 CLOCK_GAP 或 GPU_PROGRESS_TIMEOUT。真实 GAP 继续撤权，恢复通过显式新空场、新 frame、新 session。worker 的 runnable 还要求 AllowedLocked，撤权后采用原等待与 drain 路径，避免把残留 active tick 当作可继续的计算。

生成链显式覆盖原先冻结的 timeline reader/阶段定义，保留 T456 的 BootCapability/Allocation 与 log 接线。没有更改物理 kernel、缓存 key、GAP、root 绝对期限、quota、守恒或渲染管线。T477 已取得的 Maleoon 三 program 冷暖主链 PASS 保留。

回归脚本与原始日志位于 test/t477_fix 和 test/reports/t477_fix。新增检查使用实际 native Export 生成 124 行跨过原失败点，覆盖旧字典拒绝及原字节保留、未知事件拒绝、dropped 仍拒、撤权首因、新 frame 显式重绑与旧 callback 隔离。另复用 T306/T404 冷导出、T467 页面及恢复事务断言。生成 M2 App 的 GLES 夹具在沙水准入、沉降后等待原 30Hz 节拍上的当前物理帧，断言 native 呈现账、有效 token 和渲染目标，再验证显式新空场；沉降完成的那一拍不直接等同于呈现完成。最终签包与验证结果记录在 delivery.json。

tester 使用新签包按原卡复跑冷导出与显式新空场恢复。超过原 GAP 的设备停顿仍是有效故障；若再次发生，导出完整 boot/recovery 首错及新 timeline 原件，分别记录 UI callback、native receive 与 job 域。稳定帧率和实际系统上屏结论由设备测试给出。

本轮结果为新增 9/0、页面 12/0、T306/T404 20/0 与 22/0、恢复事务 31/31。实际生成 M2 App 验证沙水准入、沉降、当前物理帧与有效 token/Draw/Swap，并完成显式新空场恢复，记录 native 当前物理帧 1。Hvigor 30 任务零错误，签包验签 PASS，签包 SHA256 为 b27eb2114579ae486d30e48c8155aa4a5448ac9e248c755aeb836923cbbac13a，收据 test/reports/t257/T477-fix.json。
