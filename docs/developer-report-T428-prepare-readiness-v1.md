# T428 页面准备开发交付

2026-11-09，developer。按 T426 §3–5 实施 `T426_PREPARE_V1`，源码位于 `test/t426`，由生成器基于 T414 生成独立工程。原基线提交 `c82dad6`。完整生成与续测说明见 `test/t426/README.md`。

三个 TextInput 使用独立控制器，从创建开始接线编辑及焦点事件。准备调用 `stopEditing()` 后 clearFocus，动作区进入 PREPARING。SDK API 26 支持的 `defaultFocus(false)` 明确关闭默认焦点。页面从 EntryAbility 所属主窗口取得键盘四边避让初始快照，订阅避让、键盘高度、窗口活动和模式，保存原单位和身份。固定系统键盘配置必须由 tester preflight 显式确认，全屏手机模式由当前窗口与设备类型共同复核。

页面在实际 ArkUI `onFrame` 后的 `onIdle` 读取组件布局。两个不同帧点的完整几何、编辑/焦点、避让和原生 owner 观察一致后产生私有不可变 preparePermit。快照同时保留窗口 px、组件 window/screen px、浮点 vp box 和实际 vp2px 转换结果。预约唯一入口强制同步复核许可，再执行原 T414 校验。成功预约消费一次许可，释放后重新准备。3000ms 仅用于真实 elapsed 超时失败。

准备代际与导出 busy 独立。hide/disappear 解除本页具体 callback 引用、定时器和帧 owner。回配置、输入/焦点/窗口或原生 owner 换代撤销许可，OPEN 中新的遮挡或几何观察使 hold INVALID，保留首因。旧帧、旧定时器和旧异步文件失败均不能结束新准备或清除新导出标志。标准组件和窗口 API 异常保留原 code/name/message/runStatus，历史 55 的合成回执按原失败记录，不赋予 READY 含义。

独立冷文件为 `t426-prepare-<session>-<pageGeneration>-<prepareId>-<state>-<observationSerial>.json`，通过原 `DiagnosticColdFiles` 写入 SHA 收据和 WS_EVENT。状态变化分别落文件，含 API 原始记录、输入历史状态、有界事件序列、完整当前快照和 permit。超出诊断容量会拒绝，避免缺记录仍放行。状态文件与原 T413 hold/native/clock/PNG 档各自保持版本。

| 必要检查 | 结果 |
| --- | --- |
| 实际生成页/三个 TextInput 链/FrameCallback 与准备 helper | 16/0。冷启、编辑后准备、残留键盘、未知能力、迟到布局、旧 native geometry、位置/缩放、唯一预约门、OPEN 遮挡、连点、真实 elapsed 超时、hide/show/监听清理、旧异步失败和窗口重建均覆盖 |
| 历史 reject55 三组夹具 | 由真实页面观察分别得到隐藏后 READY、仍显示拒绝、配置未知拒绝。正式生成页没有 hideTextInput 调用 |
| 旧 T414 controller | 11/0。许可已通过的前置桩下验证 hold/ack/seal/释放与注错，页面许可本身由上一组实际接线检查 |
| T418 hold 状态差分执行器 | 8 个原场景输出符合原预期，含零副作用、旧 ack、seal 幂等、expiry 和 release 门 |
| T414 冷档工具及宿主 runner | 16/0 与 4/0，保留 PNG/取回/SHA/时钟包络拒判 |
| T427 全链截图负控 | 9/0，保持 1px、1码、缺列、旧 token、未知颜色及越 hold 拒判 |
| Hvigor 最终生成工程 | 30 tasks 全执行，BUILD SUCCESSFUL，零编译错误 |
| 最终 HAP 签名/验签 | PASS，详见 `test/reports/t257/T428-finalized.json` |

本卡未使用真机。页面逻辑在 ArkUI、窗口和原生状态桩下执行，生成工程完成 ArkTS/native 编译。真实 IME 事件、设备窗口能力和系统 PNG 可见性由 T408 使用新包采证。浮动键盘和未确认模式返回 `KEYBOARD_VISIBILITY_UNKNOWN`。T408 历史 attempt1 的原始 hilog 缺口继续保留，后续按 T426 §6 分别报告准备、采集链、标定与材质配对。

签包 `D:\hmprojects\HelloWorld\.codex\local-test\t257-sign\T428-finalized\T428-finalized-signed.hap`。

SHA256 `eb6ae00ba8a3cd00e35b891791442c07001013235549a5eecac2bcb64ad26a35`。

渲染保持 XComponent + shader。旧 T414 生成器、材质与仿真、hold/时间包络及颜色判据沿原合同执行。
