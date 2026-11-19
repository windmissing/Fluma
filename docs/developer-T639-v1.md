# T639 当前交付索引（v1 历史报告保留）

2026-11-19当前结论：T660前置已齐，本卡完整开发/编译交付见 [developer-T639-v2.md](developer-T639-v2.md) 和 `test/reports/t639/compile-receipt-v2.json`。以下仅为旧版局部交付/阻断历史，不能继续作为当前未实现结论；产品/真机/独立验证未执行、不冒PASS。

---

# T639 接续：真实双父适配与候选 GPU 材料实现；新增契约阻断

2026-11-19，消费 T648 `2a670f2`。**本卡整体未完成，状态“看评论”；本次提交不是 G11 产品闭环。** 原先 DM 2404/2024、时间/审核和私有父 proof 接口阻断已经 T648 解决，下面历史报告只作留档，不再将其列为当前前置。本次未使用真机，未执行测试、shader、签名或部署。

## 本次可独立提交的修改

- `parent_adapter.h/.cpp` 使用真实 `variant<R1 Config,DM Config>` 与真实父 Permit；明确转换资源描述，不转换父 Config/Permit，不降组合 count/mask。R1 使用既有 synthetic research 参数，DM 解码真实正式 Config64；物理 seed 由冻结 T552 输入构造与真实 DM 模板生成，写实际 scope/参数后哈希。输入仅声明 SYNTHETIC_TEST_NATIVE_CONTROL，不冒 REAL_TOUCH。
- `composite_gate.h/.cpp` 消费 T648 的真实私有 R1/DM composite proof，核组合身份、真实 18/19、ExpectedHash 后才返回本域私有证明；没有任意 count/mask 或测试 boolean 授权入口。它目前是可编译模块，**未声称已接入实际 native producer/联合提交**。
- `material_plan.glsl` 与组合生成器在真实 recruit、Hamilton final share、deposit、sealed source/launch/capture event writer 接缝写 pending 计划；touch rollback 清对应 pending。核完整来源键、范围、nonce/封前缀，物理批结束后再封计划；不用最终差或预测通量反推。核新床 cohort、S→F→P 全退出来源等守恒覆盖。默认两个父源码和共享 proof 只读。
- `marker_map.glsl` 加入精确 F rank/m 左先右后迁移、真实 S cut/launch/deposit、固定锚 phase、accepted tick 256、已有 F capture 混合及 blend=0 的 P capture；并行 rebase/requant 计数用原子操作。**blend=256 的 P capture 明确拒绝**，未私占 reserved 字段保存旧 P 质量。
- `visual_stage.glsl` 加入一次候选冷初始化、独立角色 counter/ID、真实暴露/事件顺序的 INACTIVE 补样、marker 对比、Pstyle 真实 id/gen 与最小 marker slot 代表、无样本真实 P 不透明。Expected/VisualConfig 采用真实 LE SHA 约束，扩展从 GPU 的冻结区写入，不在 native map 后补尾。这些是**候选 producer 源码**，没有运行或完整审核/呈现 PASS 证据；column index、完整 ghost/glyph 审核仍未闭合。
- `generate_contract.cjs` 从正式资源表生成八档数据，修正 DM V8192 两档旧表少算 896B（7406208 / 7408768），没有增加正式预算。冻结输入 seed header 可复现生成；正常编译先 `--check`，stale/missing fatal。CMake 跟踪新增 GLSL/seed/工具依赖，R1/DM 组合保留 SOURCE19、Q2543/2923。
- 原 R1-only `VisualTransport::Create` 不能产生新受信种子身份，现明确返回 `CALIBRATION_CONFIG_REQUIRED`，不再报告 READY。保留的旧 Permit 改名 LegacyCompositePermit，避免与新的 tagged CompositePermit 发生跨 TU ODR 冲突。原 raw Commit 仍拒绝，未建立 owner/page 的空壳成功路径。

## 当前确实无法自行裁定的两项契约

1. **实际视觉 seed SHA 的精确前像及冷取交接。** T647 要求第一 root claim 前 Expected224[144..151] 绑定实际视觉种子；T648 `acceptance_t647/contract.cpp:23` 对 G11 强制非零。T563 manifest `identity.actualVisualSeedSHA` 只规定 GPU 初始化并 raw cold retrieval 后可得，没有规定 CURRENT marker payload / bankHeader / Pstyle 的确切跨度、顺序、header 身份是否纳入，以及首 claim 前读取与 singlePhase witness 信用交接。已核两版工程、机器表和真实工厂，不能用 recipeSHA、物理 seedSHA 或零替代。**原卡问题评论 `6afeba20e4b0e5384a544ca3`。** 需要 architect 冻结具体字节域及初始化→hash→首次 claim/reader 切点，developer 再实施 native 捕获。
2. **P→F 旧 ghost mass 持久落点。** marker32[10] 为当前 F massSnapshot，glyph8[6] 要旧 P mass；捕获后 packet slot 可以换代，phase 跨 SOURCE 不推进时间，而当前 plan/event 会被下一批覆盖。marker[30]/余区正式保留零，Pstyle4 无 mass。共用 glyph 缓存不在 ACTUAL/REPLAY marker 独立比较内；用新 F 总质量会冒充旧包质量。已核字典/slot 生命周期与 span256，blend=0 可实现，默认 span256 无合法既定持久字段。**原卡问题评论 `6afebd90e4b0dc337eac0d2e`。** 需要 architect 明确现有字段合法 union 或修订字典/预算；developer 不擅改协议。

这不是设备等待、不是默认 DM 设备 PASS 前置，也没有新增 manager 验收门。两项均已原卡反馈，尚无回复；继续写一个随意 hash/字段约定会破坏冻结身份与独立 replay，不能自行放行。已完成上述独立材料/适配/生成范围并编译，保留候选源码，未把部分产出报整卡完成。

## 未完成范围与接续入口

| 项 | 当前实际状态 |
|---|---|
| W01 | 生成/全输入依赖可编译；独立 G11 owner/NAPI/page、真实 session/clock 未接 |
| W02–W03 | 保留 1024B 尾安全；两父类型化 adapter/gate 模块已编译；native ActiveProgramIdentity/Expected224 冻结上传及真实 mapped proof 未接 |
| W04/W06–W08 | 候选真实 writer/plan/marker/补样/Pstyle/角色对比落地；完整 bit18/19 生产审核及默认 ghost、column index 未闭合，未运行 |
| W05 | 真 Config/seed 构造模块已编译；视觉 seed 原件/hash 阻断实际 Create/首次 claim |
| W09/W11 | 私有 candidate 保留，联合物理/VIS bank 提交锁、SceneLease/实际 fence/token/Swap 未接，不发布 |
| W10 | 已有新管线 scene 消费保留；旧 P ghost 需要上述字段契约，未宣完整真实呈现 |
| W12 | 完整 ROOT/FULL+7VIS raw cold、singlePhase witness/broker/reader 未完成；不将旧 metadata Read 冒原件 |

先由 architect 回到原卡补充这两个精确契约；developer 续实际种子捕获→可信 native identity/Expected→owner/联合提交与 lease→完整冷档/reader。旧父默认/新 RX/共享 proof 源码未改。无 Canvas 上屏或回退。

## 编译证据

`node tools/g11/prepare_compile.cjs .codex/local-test/t639-receipt-build` 生成新的隔离编译工程，g11vis 六个 TU whole-archive 并链接真实 R1/DM gates 与 T647 核心。native、ArkTS、unsigned HAP 全构建 **39.869s**；最终 legacy guard/类型修订后增量 **9.841s**，均零错误。两套 compute GLSL 静态编译 exit0，未运行 shader。

这仍是**编译脚手架包，不是可运行 G11 产品包，不交 T600/T601 做产品复测**。正式 receipt、输入/产物 SHA 和原始日志：`test/reports/t639/continuation-compile.json`、`continuation-full-build.log`、`continuation-final-build.log`。HAP SHA `3a59830b360842fe0863c07a0758c8986e410a49f4b29ffacc9cbe333d1ee3c6`；打包 libgrs_g11_compile.so SHA `98758d23ca7b1008341fb41e6c3a334af7a95f3a703f5c571a7cc62b9d173924`。GPU 正确性、ON/OFF、开销/期限、非空完整帧均 NOT_MEASURED，不冒 PASS。

---

以下为 `ede5891` 历史回执。其父接口阻断已由 T648 解决；仅保留当时原始结论，不作为本轮状态。

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
