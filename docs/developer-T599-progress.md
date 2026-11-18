# T599 开发交付

2026-11-18。仅开发、编译、链接检查和提交，未运行测试、reader、模拟或真机。软件接线已交付；真实平台时钟资格 UNKNOWN，不声明真实手势产品 PASS。

研究页显式 REAL_TOUCH_R2 入口使用正式 Config64、parameter revision 3 与修订 seed R2。同一上传数组核模板 SHA 后写实际身份、计算 actual SHA；额外冷初始化核验 dispatch 不进入 SOURCE/Q root 计数，不改物理配方/阈值。

DOWN/MOVE/UP/CANCEL 使用同一 changedTouches 点的原 BaseEvent.timestamp、local/window/display 坐标和同 UIContext.vp2px 比例。native 自采 receipt，转换前拒绝非有限值、非安全整数和精度超限。MOVE 零力、仅累计有界摘要；UP 一次建立 INPUT128 V2，接原队列/Offer/Claim/回执。QUEUED_NOT_CONSUMED 不冒已消费。G9 receipt/claim/freeze/transport deadline 使用显式 CLOCK_MONOTONIC，250ms/GAP/120Hz/30fps 不变。

immutable PresentedMapping 来自同一次成功 Swap 的 lease，含真实 commit/generation、完整 scope、geometry/viewport/angle、参数/actual seed/token SHA。查询不创建身份/计数或授时钟；UP 要与 DOWN 同映射。scope 生命周期失效撤 stroke/proof，旧映射不能授权。

ClockProducer 提供 native-only 读取/安装/撤销。可选 native provider 必须核实际 OS/build、事件 producer、转换链、本 native 时钟源码 SHA 与完整 scope；编译生成的源码 SHA 也须匹配。无 JS verified=true、Date.now 或时间差拟合。目前无具备实际 vendor 证明的 linked provider，默认 INPUT_CLOCK_UNVERIFIED。

平台接手所需：实际 OS/vendor build 身份/SHA；本设备 BaseEvent.timestamp 采样 API→producer→重采样/转换→CLOCK_MONOTONIC 完整源链、单位/epoch；实际部署二进制与源码对应关系。平台实现者据证据实现 LoadQualifiedPlatformClockProof，native 安装获得新 proof generation；tester/effector 独立验证设备资格与效果。SDK “since boot”或上游源码本身不足。当前不将 UNKNOWN 宣成 REAL_TOUCH 资格。

cold writer 实写 CURRENT/BASE 两份256U32于3072/3328、Config64/SEED_META64于3584/3648，完整字典含原坐标/时序/model/配方/scope/来源/frozenInput SHA。仅接受提交后晋升 lastConsumed，BLOCKED/FAULT/重复不晋升。保持16队列、活动stroke、freeze/base/lastConsumed原信用；sizeof断言约束Mapping≤256B、活动adapter≤1024B。不额外保存整个prefix。

V1合成身份保留，V2 capability/receipt/page/逐页collector/reader完整接线。外部manifest存原prefix/双record SHA、存在性、输入域及clock/platform来源；明确要求独立平台证明，archive完整不冒时钟资格。reader补版本/零位/摘要/映射/INPUT/时长/age/整数配方/seed绑定代码，本轮未执行。

完整新生成工程 .codex/local-test/t599-delivery-final 编译成功；同步最后scene时钟和manifest后 native/ArkTS/HAP再次成功，日志 .codex/local-test/t599-delivery-sealed-build.txt（15.807s）。无签名部署。详见 test/reports/t599/development.json。共享cold broker由本轮T596依赖提交提供。渲染仅XComponent+shader。
