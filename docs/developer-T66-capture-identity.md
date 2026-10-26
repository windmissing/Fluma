# T66 身份与采集收尾部分交付

日期：2026-10-26。developer；不使用真机。消费 T64/T65/T68；开发部分交付，T44 可见性未宣称修复通过。

物理 attempt 入口统一分配 run 内单调安全整数 sequence；getP011Metric/hook 数量不再改变它，新增 P011_COMMIT_IDENTITY_V2。账本/endpoint/metric 共用 attempt 身份；V2 stamp 必须匹配 sequence/frame/substep/attempt，旧报告与原质量/peak公式保留。没有去重、删除失败记录或改变准入累加顺序。

store 提供 run/storeGeneration/requestId 身份校验与幂等 finish；进度分别维护 sent/written 两流，written 必须连续，CLOSED 必须与两流 written/sent 严格相等。fence 含两路径、末条 attempt/sourceEpoch、末成功 commitKey；缺少真实 endpoint stamp 时不伪造 commitKey。正式入口为前台“结束采集”或 new Want 三字段 p011finish/p011finishrun/p011finishgeneration；混用业务参数拒绝。结束在 UI attempt 间边界停止后续 tick/schedule/触摸准入，拆 sinks，再排 close；不会重置 run/切模式/伪造停摇。重复同请求复用 promise，异请求/旧代拒绝。reset/销毁尽力走幂等收尾。普通 Home/Show 不主动关闭。

metadata 冻结启动时真实 route，不再固定冒称旧路线。异步启动前队列有界256，溢出显式失败。`tools/p011_capture_manifest.py ledger admissions fence.json output.json` 在受控拉取后生成两文件 bytes/SHA256/raw行数及最终身份；只有严格 CLOSED、计数/末条身份一致才 COMPLETE，不去重、不把前缀变完整。

T68 版本变更：页面移除 Canvas 上屏及背景/前景回退，也不运行初始化隐藏 Canvas 背景。XComponent 唯一场景入口保留。T56 尚未提供完整 ScenePacket/FBO/全部层，当前显式 NEWPIPE_FAULT，**不提交基础床/粒子局部帧冒充全场景**；旧场景模式同样明确 unsupported。保留历史渲染函数与旧测试/报告作为数学和证据出处，不把其旧成功更名为新管线成功。

验证：identity 3+2/0，store5/0，既有 endpoint9/0，manifest1/0，共20项；`test/reports/t66/`。比较 hook/no-hook 及重复 getter 的真实场数组不变。ArkTS CompileArkTS 与 BuildNativeWithNinja 成功（公共构建证据 `test/reports/t75/build.txt`）。worker 文件 IO/真实 fsync、设备 Want/UI 生命周期、完整场景同帧呈现仍交后续 tester/effector；host桩不替代它们。完整渲染部分等待 T56，不能关闭 T44 的 FAIL/BLOCKED 产品结论。
