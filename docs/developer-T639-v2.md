# T639 · 双父 G11 完整产品接线开发交付 rev2

2026-11-19。消费 T634 W01–W12、T647/T648 私有双父 proof、T660 正式合同及完整 manifest/recipe rev2。当前约定的开发与编译范围已完成；这是 **DEVELOPMENT_COMPILE_COMPLETE**，不是产品、物理、像素或设备指标 PASS。不使用真机，不执行测试、产品、shader、签名或部署。此前 ede5891、b0143d23 的局部修改保留；v1 报告中的旧 seed/ghost 阻断已由 T660 解决。

## 实现落点

| 范围 | 当前实现 |
|---|---|
| W01 | `g11vis/controller.cpp`、`bridge.cpp` 为实际独立 owner；`GpuResidentG11.ets`、`GpuG11Port.ets`、`G11ResearchAbility.ets`、native 声明及独立 CMake/产品准备器实际消费。研究默认 OFF；R1/DM、K192/512、V2048/8192、ROOT/FULL 冷选。缺输入、正式输入大小/SHA 不符、生成非零退出、旧 generated contract 均阻止构建。 |
| W02–W05 | 完整 Tail256/1024B；rootKind、pass count、audit mask 独立。R1/DM Config、InputGate、ProgressClock、Permit 均用真实类型及 tagged variant，不把 R1 实例强转为 DM。T648 实际 ActiveProgramIdentity → Expected224 → NativeMappedObservation → 私有父 proof → CompositeGate；没有 CPU 补 bit18/19、默认 record cast 或改尾。 |
| W06–W08 | 真实材料 writer 的 sealed plan、四 map 与 marker 消费沿 b0143d23 接续；角色2/4独立更新。统一 barrier、四 map 正式顺序与 SOURCE replay BASE 保留。rev2 加入旧 P mass/slot/gen/id union、独立代表选择及全32word对照。默认 blend256；blend0仅显式独立配置对照。 |
| W09–W11 | 非零实际 context generation；联合 SceneLease pin 物理 bank、视觉 bank 和 glyph WORK，真实 consumer fence 退休前背压下一 writer。底材/背景 → glyph → 真 P/旧 P ghost → 前景在同一 FullFrameTargets candidate 完成，最后校验同 token/scope/freshness，真实成功 swap 才登记 presentedMapping。只用 XComponent+shader；失败保留并标明 stale 的上一有效新管线帧，无 Canvas/软件 GL 回退。 |
| W12 | 同 token 的真实父 ROOT/FULL 加七个 VIS 段，共11/17段，实际原 U32、单段及整档 SHA、共享进程 broker 两slot/单builder/单reader、60s不续期。完整 singlePhase witness、bootstrap raw reader、worker 流式文件归档；无整档 JS 聚合或 UI/GL 热路径文件写入。 |

## 首 claim 和撤权

真实程序与物理 seed 固定后，先预占并分配既有 singlePhase witness 的一份 GPU/native 容量，再执行冷初始化。随后四次原字节 copy：CURRENT header512、CURRENT marker128V、全512行 Pstyle8192、Config256。实际冷 fence signal 后，每次至多4096B map/read 到同一 native witness 块，核完整原件/单段 SHA。V2048/8192 前像分别271104/1057536B，SHA 不加 JSON/对齐/摘要 framing，不使用 recipe/物理 seed/零替代。

`g11BootstrapPage` 可在封存前导出原件。`g11BootstrapSeal` 内部封存收据并实际 End/free/refund；`VisualBootstrap::Ready()` 同时要求 state4、无 credit、无 native/GPU 块。只有此后可 BeginSession/SetActive、创建 ActiveProgramIdentity 和首次 claim。正常 GPU Root 不多读前像；Expected224/hash 只在正式 control61312..62240 上传928B。

每次冷配置使用独立 run/inputEpoch/pageSession/permission/config 身份。旧 scope 的 Stop 不撤销新配置；UI 后台/窗口撤权在排队前设置原子失效，晚 proof/present/raw reader 均不能跨代发布。Stop/到期只在实际 fence/map/reader 退休后释放信用；实际 EGL destroy 成功才可使用 context-retired 回执。loaded-module SHA、文件/内存 BuildId 的外部绑定复用只读 g9rx helper；不可读时具名拒绝配置，不伪造已绑定身份。

图形程序、反射和 FBO 在冷配置阶段准备，计算资格与 scene 结果分列；没有把首次图形编译塞进激活后的物理根。进度数学复用对应真实父 ProgressClock；组合私有 proof 通过后，在同一非GL提交锁中发布物理/视觉 bank、accepted ticks、clock/FIFO/token。普通根仍 SOURCE19、R1 Q2543、DM Q2923、4 compute SSBO、1尾 fence、1024B尾读回，250ms/GAP33333333ns/120Hz/30fps 不变。

## 冷字典与读取

`g11ColdBegin(token, ROOT|FULL)` 只有当前联合 publication 且 WORK/scene 可 pin 时捕获；上根物理 BASE/FINAL/selected/authority 与六个 FULL discovery 原件保留原布局。未进入 discovery 的段明确 executed=false/零占位，不称已经执行。七 VIS 段为 markerCURRENT/ACTUAL/REPLAY、glyph、materialPlan、control、fourMaps。CURRENT 在此指该 root 的冻结 BASE(previous CURRENT)，ACTUAL/REPLAY 是该 root 两个实际末态；元数据保存提交前真实物理映射，不误用下一根轮换后的 scratch 身份。

header32768B：word0..36 为版本/档位/身份/逻辑bank定位；256..511 Tail256；512..639 input128；640..767 config128；768..1023 quota256；1024..1407 六个 trial summary；1408..1535 controls128；1536..1663 初始 seed receipt128；2048起每段32word描述符。描述符0=id，1=trial，2=role，3=executed标志，4=J，5/6=event/path数，8..9=raw word offset，10..11=word数，12=行stride，23..30=单段SHA，31=seal；其它位置保持初始化零。payload从32768B开始按段顺序直接拼接。整档 SHA 是封存 header+payload 的原字节 SHA，没有后补可变元数据。

`ColdPage(id,segment,wordOffset)` 的 segment0读header，1..11/17读payload；每页≤1024U32且不拆 marker32/glyph8/plan8/cohort4 行，JSON≤16KiB，所有宽ID为十进制字符串。native分段读回和整档二次 hash 每次≤4096B，持同一 WORK pin；完成后 reader 只读不可变 native 块。manifest带正式 v2 字典路径/SHA、逐段定位/执行状态/SHA及整档 SHA。worker 按 LE U32 写 `.u32le`，逐页身份/范围及单段/整体 SHA 全部匹配才封存文件与外部 manifest；失败写 INCOMPLETE，不能当产品通过。

witness 预预约 `(nonce,micro,phase,role)`；SOURCE micro=-1 以U32_MAX传入，phase1..6，role2/4。目标 PLAN_BEGIN 后旧 marker 尚未更新时 copy old，目标 PLAN_APPLY 后 copy new/完整plan/maps以及当时 bank/plan header，复用该 root 的实际尾 fence。没有新增普通 root dispatch/fence；copy及对应 barrier 的实际计数独立报告。未到目标批次为 NOT_CAPTURED；失败私有资料不冒 CURRENT。witness header4096B保实际scope、参数/布局/pipeline/config/recipe/hash、初始seed receipt、目标batch headers及真实root outcome，其后为 old marker128V、new marker128V、plan2097152、maps524288B。容量仍3149824/4722688B各一份 GPU/native，bootstrap与phase互斥；两DM FULL388308992B和已知641920128B包络不增。

## 编译和宿主开销

最终证据：`test/reports/t639/compile-receipt-v2.json`、两个实际 hvigor 原始日志、八个 shader 静态编译日志和 generated-inputs.json。记录实际包内 ELF/BuildId/ABI、ABC、HAP SHA，以及与隔离编译源逐文件一致的原字节SHA/归一LF SHA。包内实际 R1/DM shader 字节与本次生成源相同；实际链接 Controller::Pump、注册入口、ABI及共享 cold broker。不是旧 whole-archive compile anchor。

实际AArch64：seed receipt512B；bootstrap helper568B（含该512B），phase helper248B；VisualTransport20168B；Controller34400B（含transport/cold/control/输入FIFO），Cold2552B，Scene38512B；owner App103512B（含controller/scene/32命令队列29184B/32回包槽1024B等）。这些包含关系不能重复相加，也不能全部声称属于512B预算。字符串/容器动态载荷、allocator、进程broker/缓存、driver/FBO/ArkTS/文件另计，未测实际运行峰值。

T648类型实编译 ProgramFacts408、ActiveProgramIdentity416、ExpectedContract56、NativeMappedObservation80；transport实际 optional active424/expected64已包含在自身sizeof。组合 accepted/blocked proof各1496B（含CompositePermit424及父proof），只在当前Pump临时消费；没有新增常驻Expected896B镜像。以上是类型尺寸，不是编译器栈峰值。

复现：

```text
node tools/g11/generate_contract.cjs --check
node tools/g11/prepare_build.cjs .codex/local-test/<全新目录>
在该目录使用当前 SDK/JBR 运行 hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon
python tools/g11/build_receipt.py <构建目录> <证据目录> <glslang.exe>
```

`prepare_compile.cjs` 旧命令名现在转到真实产品准备器；默认应用/父路线未被新研究页替换。正式运行、独立物理/视觉/负控/ghost跨SOURCE及slot复用、250ms可行性、设备属性、像素、实际归档与运行内存峰值仍由tester/effector接手。未把这些独立执行加成开发关闭门。

## 最终产物登记

全新隔离目录：`D:\hmprojects\HelloWorld\.codex\local-test\t639-delivery-v2`。完整构建 43 s 973 ms [39m，最终源码增量编译 14 s 829 ms [39m，均 BUILD SUCCESSFUL、零错误。8个shader静态编译通过；ArkTS系统能力/异常提示为编译警告，不伪记设备资格。

- unsigned HAP：`74ae28e15b01d4db170e6a7c8970327ed6365dc68c051b37c58af8e745577d77`
- 包内 libgrs_g11.so：`36b72b1ec66daa9ddda61ad8e89056f419ff7f3655068e0010911acb6f664f02`
- BuildId：`bbfe2dfc0148687bdbe22f60b7b161823e5c4eaa`
- ABC：`18490ca797a3a8baab9aee3031f2d3f9984d60beee3c1387db710fa37a60090e`
