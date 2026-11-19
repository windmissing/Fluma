# T639 developer：独立安全修复与真实父前置阻断

2026-11-19。**本卡未完成，BLOCKED；不是产品交付/PASS。** 按T634/T563继续到实际父typed接受链发现冲突后，已完成不依赖接受链的安全/静态修复。本人行应为看评论；原卡已有问题评论6afe69b8e4b0680b8cd552aa。未使用真机、未执行测试。

## 具体阻断、证据与已尝试办法

1. `g9dm/transport.cpp` Common/Pump实际使用SOURCE12/QUANTUM2404，`g9dm/schema_generated.h` 的kQuantumPasses有2404项，正式T554也要求2404。真实`g9dm/completion_gate.cpp` Common仍硬判12/2024，同时要求tail[39]/[40]和NativeEvidence.dispatches都等2024。真实2404尾必被拒；这不是视觉适配器可用R1强转解决的类型差异。
2. 同一DM `pipeline.comp` 的g9EndAcceptedMetadata仍只有`uRequiredPasses==2024u`才推进acceptedQuanta；实际DM uniform为2404。其PACK_SCENE_METADATA最终把g9Ctl(17)写65535，tail[42]原样取该值；正式DM Q mask需要物理bit16/17，而G11 DM Q正式要求0xfffff（含视觉18/19）。当前父时间和最终审核汇总与正式DM合同均未对齐。
3. 两父CompletionGate API只有固定Common/Accepted/Blocked，尚无T634引用的可接受组合count/mask/新profile身份的ExpectedContract入口。R1真实gate接受2024，DM真实gate也仍2024；组合必须分别2543/2923、SOURCE19。不能修改实际evidence、抹掉审核位、归一化tail谎称原父已接受。确需明确修订两个父的typed接受接缝及新组合tail身份/接受证明，而不是绕过真实父gate。

已核实际源码/正式T554/T563/T634、真实native和GPU切点及已生成两表；已尝试在**仅组合生成路径**显式rootKind化、严格保持组合计数和mask，消除计数猜kind。这能修组合shader表达，却不能使原DM自身typed接受/accepted time和正式双父合同一致，也不能授予联合提交证明。真实父默认修复会改变既有physics/时间/审核语义；本角色指令要求算法/工程冲突先反馈，不能静默扩展到默认DM修复或把另写宽松validator冒复用父接受链。

需要architect：按T563 §9“父R1/DM实际实现未齐时具名真实前置”，明确DM原2404/accepted time/bit16–17真实审核的修复接缝与源身份；明确R1/DM CompositeExpectedContract及新tail revision/profile/返回validated proof的正式接缝，保原默认门与组合门分域。manager据该修订交developer续作。无需设备输入、无需manager新增验收门。原始片段/行号/SHA见`test/reports/t639/blocker-evidence.json`。

## 当前源码范围（非完整闭环）

| 项 | 已完成安全/静态部分 | 未完成/受影响范围 |
|---|---|---|
| W01 | 正确repoRoot、独立build assets、正式输入原字节SHA/大小、missing/nonzero fatal、全输入configure依赖；输出两独立GLSL与输入收据 | 产品owner/bridge/研究OFF页未接；不建可见但不可接受的空壳 |
| W02 | words256U32、static_assert 1024B，memcpy不越界 | 完整typed接受待上述前置 |
| W03 | 显式uRootKind，真实pass数与requiredAudits独立；所有12/2024/2404猜kind点仅组合路径替换并静态拒残留 | 新tail的完整身份/typed验证待修订 |
| W04 | 删除CPU补18/19；只读GPU实际tail位，缺required mask不能通过；无typed proof一律不切bank | GPU真实材料/视觉审核bit producer和完整接受链仍未完成，不将静态相等/空数组冒审核PASS |
| W05 | DM不再经R1 Config入口伪创建；visualGeneration改U64并拒超限 | 真R1/DM tagged adapter及父完整初始化/seed/header待正式接缝，不伪称已落地 |
| W06 | 修描述符aligned/range检查与非法ordinal取模别名 | 实际writer sealed计划→marker/初始GPU seed/role独立ID/phase witness未落地；不能用最终差或digest替代 |
| W07 | 修VIS分支误插入g9Flight（原生成器匹配第一个lane声明）、统一RootCopy屏障、RootAudit循环作用域/提前返回；VIS行走实际pass counter及统一role/micro门 | 真材料计划和独立审核仍随W06 |
| W08 | 四map顺序OLD_BED/NEW_BED/OLD_FLOW/NEW_FLOW及SOURCE replay旧图读B | 真实BASE冻结/联合租约和全部writer生命周期随parent adapter闭合，未宣运行通过 |
| W09 | PreparePrograms与FBO取实际context generation，移除固定0 | 联合publication token未产生；拒尺寸字符串冒token |
| W10 | glyph8字段对应、两锚圆盘/真实S/F轮廓、accepted tick Q16相位；真实P attrs0..5/id/gen/Pstyle反射与消费；缺样本不隐藏真实P | 真实材料/marker/Pstyle producer与独立身份审核尚未闭合，不宣真实通量效果 |
| W11 | FullFrameTargets候选FBO、完整底材/背景、真实P消费与blit源码接缝 | 缺validated publication/scene lease/WORK fence，不呈现或swap，不把私有候选冒完整有效帧 |
| W12 | 未把现metadata Read伪装成raw捕获 | 真实父ROOT/FULL+视觉U32/SHA共享broker/联合pin依赖真实双提交身份，尚未实施 |

本轮不接通产品页，组合Commit和Draw明确拒绝；旧父默认源、阈值、生产数学、已交T629/T632/T638不修改。不是以NotImplemented壳宣完整交付。

## 编译与复现

`node tools/g11/prepare_compile.cjs <全新.codex/local-test目录>`只准备**编译脚手架**：沿G13研究入口编ArkTS，同时将g11vis四个TU以whole-archive链接进独立libgrs_g11_compile.so。此HAP不是可运行G11产品包/组合owner交付，不可拿它做T600/T601产品复测。missing/错误输入fail fatal，不吃旧header。生成器冻结两表19/2543、19/2923；2个compute和6个图形shader通过Khronos glslang16.6.0静态编译；未运行shader。

最终编译结果和原件SHA写`test/reports/t639/development.json`。此前增量目录遇到旧CMake依赖路径../归一化重复，已去重并改全新隔离目录验证，不掩盖失败日志。运行资格、物理、像素、资源期限及效果均NOT_MEASURED；本卡后续按真实工程前置继续，不以设备为开发门。

最终全新目录`.codex/local-test/t639-delivery`：hvigor **BUILD SUCCESSFUL 45.466s**，34/34执行；全archive G11编译库/ArkTS/unsigned HAP通过。报告已冻结实际源码和产物SHA；这不是完整G11产品交付。
