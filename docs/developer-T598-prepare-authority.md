# T598 单对象准备事务接线

2026-11-18。开发编译，不执行测试，不用真机。

T574旧旁对象已移除；prepareGate本身为PrepareReadinessV4，继承并复用原严格predicate/frame/permit实现，只有一个许可状态对象。删除可直接ready旁路；show/begin重置并按真实prepare/page/branch/windowId/windowGeneration/controller/session/surface/context/run绑定owner。noteChange/实际snapshot sample/checkForReserve/consumeTicket均在该对象，所有ticket逐项匹配prepare/page/input/observation/owner；consume只在原reserve返回RESERVED后，同UI栈执行，无中途await或布局修改。原3000ms仅PREPARING，无新READY TTL。

prepareUpdate同次更新@State reserveEnabled与文本；按钮真实依赖READY及configuring/exporting/evidence/capture可配置，不强制启用。fresh snapshot/check ticket在reserve内再读，不靠按钮作为授权。

handler wrapper所有出口保存first/last各Uint32Array64=256B核心，原件含prepare/page/input/observation/ownerSerial、ROI、busy、RESERVED/consume、handler前后状态和实际文件IO阶段（0未请求/1写中/2完成/3失败）；尾48个UTF16 code unit是有界reason片段，并非无界完整错误串。外部JSON<=4096B，按id只返回匹配first/last。写盘失败保内存见证，异常不逃出onClick。原t413冷文件/firstCause与strict geometry/PNG保持。未声称记录完整raster第二副本。

test/t598/generate.cjs沿T540实际链，补调用已有G11/G12仅源码生成器，解决其旧CMake相对路径遗漏的生成头；没有运行这些管线。最终t598-final3-build.txt，native/ArkTS/HAP零错误（17.682s）。没有运行READY/预约/HOLD/PNG测试，交T592独立验证。
