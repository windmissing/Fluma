# P-011 v2.8 dual-input-v1 复盘记录

日期：2026-10-18。技术路线：`bottom-creep-v2.8-heavy-interface`。状态：设计交接，待补实现/双测事实与闭环结论。

用户明确要求测试模式由程序构造手机倾斜，体验模式通过传感器获取手机状态。此前人工手持不是本轮程序测试前置；该输入授权不修改物理预算、方向、主观 HG/≥6 分门槛或采集冻结闸门。

## 候选与选型

1. **选定：复用 canonical host schedule + wstilt，修复模式所有权与切换。** 已有确定性 schedule、Want parser 和真实 sensor 映射，最小代价即可满足需求；详见 algorithm-P011 §5.8。
2. **否决：内置新场景引擎与每帧 script 日志。** 重复已有 harness，并会破坏 fresh-Want-only 的 531 条命令身份协议。
3. **否决：只改按钮名。** callback、pending Want、gesture、forceInvert 与滤波缓存仍可跨模式污染。

## 已暴露的问题

测试中传感器仍更新滤波；体验中 PanGesture 暂时关闭传感器；多个入口分别维护不完整切换；消费端缺 mode gate。另有首命令顺序风险：EntryAbility 先写 tilt Want，再发布模式 Want，切换时清到最新 sequence 会吞掉同一个进入测试 Want 的首条合法命令。设计明确必须区分旧 pending 与当前转换 envelope，而不能全量丢弃。

## 本轮结论与下一步

当前是输入协议/实现缺口，尚无新证据证明物理方案失败。architect 应落实统一转换、传感器与模拟双向 gate、首命令保留及幂等同模式声明；tester 补行为 RED 基线，之后执行输入隔离与 canonical 冻结复测；effector 用授权的模拟姿态执行原主观标准，保留输入来源与逐帧证据。双报告后由 designer 追加结论，保持 P-011 未闭环。
