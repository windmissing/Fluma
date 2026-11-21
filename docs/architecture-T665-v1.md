# T665 · RX 设备 shader 重复符号与产物偏差归因

> 2026-11-21 T791 修订。RX 生成和只读校验均须从 T641 已核验的 T637 引用检查动态设计的 UTF-8 JSON、schema 和批准原字节 SHA。现有产物依赖摘要用于一致性核对。错误在冻结输入阶段终止，保 T781 正常重建及生产 shader 原字。 [T791 构建合同](architecture-T791-v1.md)

2026-11-19；architect；不使用真机，不运行生成器、编译器、shader或产品。读取T645全4份UI/三份hilog、真实signed HAP、T644开发收据/旧ELF、prod/harness生成物及冻结父源/生成器/调用链。原件 `analysis-T665-evidence-v1.json`；正式工程修订 `architecture-T665-contract-v1.json`。

结论 **RX_TARGET_COMPILER_REJECTED_CAPTURE_LOCAL_SYMBOL_REUSE**。两native ELF确实不同，但嵌入的完整生产compute shader完全相同；不能把本次失败仅归旧包/错误shader，也不能从静态glslang“通过”断言设备驱动缺陷。先最小RX工程兼容修订+完整源身份闭包，再原T645独立设备续证；不改DM物理数学/接受门，不转designer。

## 1. 源—产物—加载的已证与未证

| 对象 | 原始身份 | 结论 |
|---|---|---|
| T644开发ELF | a04296f2fbc29241972ca0bb418a3f738f52105d19e4d6c1b021614cca52ac9e；5540744B；BuildId a0488247ca999cf3931dc8897ba9b88f03621fbe | 与开发机器收据一致 |
| T645实签包中ELF | b050b96e3cda592841f33fe47322db4e23654e44089145e9fc02552fbc74fd63；1399592B；BuildId e0e6203459b9f4691bef6d7ec89ea92de0022b4b | 与UI两次LOADED_FILE_OBSERVED的file SHA/BuildId相符，实签HAP 0bdf3304…与签名收据相符 |
| 两ELF嵌入生产shader | **145268B / 2144行 / SHA 74c820ecabfa8025361c42e10646642bf22d1f9b49cdb912349e2b856b6c1d25** | 从`#version 310 es`到首NUL前原字节相等，非按声明字符串猜相等 |
| T645/T644生产pipeline.comp磁盘 | 147412B / SHA c6074fbe… | CRLF比运行字符串LF多2144字节；按C++原始字面量换行处理后与ELF完全相符。磁盘原SHA必须保留，不能把归一化SHA谎称磁盘原SHA |
| saved native harness | LF SHA b1f3bcf1…，不是74c820ec… | 还缺部分最终参数/inputGeneration检查，非仅末尾harness注释；不能拿它的静态成功替生产源码资格 |

两构建树的RX generator/frozen/kernel_assets/transport/CMake字节一致；共享program_factory的Receipt新增route5观察钩子等native差别已记录。native BuildId不同不能仅凭库大小称strip差别，更不能倒推出shader变化；本次实际抽取已排除这两个ELF的shader原件偏差。当前主树create_diagnostics又有T663在途修改，按独立工作保留，不混成本卡原始目标或回退他人修改。

memoryBuildIdMatched只核实际内存PT_NOTE BuildId与被打开文件的一致性，**不等于每个映射字节全量比对，也不是该attempt的glShaderSource原串回读**。原链为kernel_assets::kRxPipeline→transport→programcache::Build→create468::GlApi::Source，后者`glShaderSource(shader,1,&src,nullptr)`；检查的T645快照没有宏注入或#line，shader本身也无#define/#line。包内源、原调用链与设备报错行形成强一致证据，但本次无同attempt raw-source/hash/program receipt文件，不把这个缺项写成已直接捕获。

## 2. 精确报错接缝

rx-hilog.txt与rx-hilog3.txt分别有 `18:01:54.736` / `18:04:53.902`、pid31025/tid31147 的 `0:1728: S0022: Symbol 'slot' redeclared`；l2/l4均CONFIGURE_FAILED_8。native时间窗口/attempt票据未完整保存，两次墙钟日志保持独立，不捏造其各自程序handle。

完整生产源第1728行是：

```glsl
if(best==uK)break;uint slot=best,c=g9Packet(slot,5u),mass=...;
```

它位于`g9Capture`的ordinal循环体，上一行嵌套扫描`for(uint slot=0u;slot<uK;++slot)`，函数前面还有清contacts的slot循环。这个名字在相邻循环及选中包局部间复用，正好对应目标编译器拒绝；在冻结父pipeline中已存在，并非RX本次插入DM函数时额外声明了第二个slot。生成器替换/拼接保留了它。

这里不宣某个语言版本规范或驱动一定错误：T644报告只说静态GLSL通过，保存的development/build/inputs没有可核的**该次glslang版本、命令、宏/目标与完整静态日志**。本轮没有重跑编译来补造历史。目标设备与静态编译器差异是需要工程兼容处理的事实，精确编译器规范归责未证；无需把它升级成新的物理算法争议。

## 3. 最小完整工程修订，交developer

所有业务改动限**新RX生成路径/其产物身份**，冻结父源receipt和默认g9dm/g9spf/G11不改，T647接受门与Config64/DM数学不改。不能只修改`.codex/local-test/t645-tree/...pipeline.comp`，也不能全局字符串replace所有slot掩盖插入路径。

1. 在 `tools/g9rx/generate_assets.py` 对已SHA核定的父变体完成数学注入之后、最终源hash与所有输出之前，添加具名 `RX_CAPTURE_SCOPE_HYGIENE_V1` 转换。严格定位唯一`bool g9Capture()`函数及完整花括号范围，核原legacy片段恰一次、原函数SHA与版本相符；不匹配/多匹配fatal，不默默跳过或复用旧资产。
2. 只在这个函数将三种局部用途拆开：清contacts循环`clearSlot`、候选扫描循环`scanSlot`、选中包`selectedSlot`。扫描loop使用显式 `{...}`，更新best与比较器参数只替换该loop绑定；选中包后续引用只绑定selectedSlot。禁止改函数参数/事件字段/输出索引、排序/比较/循环次数、continue/break及任何质量/动量公式。规范变换记录包含函数前后原字节SHA、精确binding范围、转换次数=1；后置拒绝legacy重复片段，不能靠全局改名通过。
3. 先形成唯一**LF UTF8、无BOM/内嵌NUL、无额外#line**的 canonical source bytes；pipeline.comp用write_bytes写同一串，pipeline_source.h字面量同一串，shaderSHA取同一串不含C终止NUL。C++源码容器自己的SHA与shader原串SHA分别记。所有生产/测试hook资产各自产生独立身份，保存harness与prod差分，不能复用旧harness文件冒新生成。
4. `pipeline_identity.json`/inputs/source manifest/implementationSHA/bundleSHA/cache ABI/reader身份必须包含hygieneRevision、实际生成器及冻结源、转换记录、canonical shader、kernel_assets/transport、实际GL source wrapper/program_factory头等全调用链输入。header/comp/ELF内actual source hash若不同立即fatal；缺依赖/stale同样fatal。CMake/prepare_build/build_receipt必须把新合同/转换输入与生成检查包含在独立输出依赖中，保存全编译命令与实际产物清单。
5. RX configure/build入口在交给共享工厂前用**已有CPU字节hash**核`strlen(PipelineSource())`、canonical SHA与生成header/该program contract一致，失败返回具名SOURCE_IDENTITY_MISMATCH并保firstFailure；不增加任何GL query/glGetShaderSource/同步/热IO。该native计算是身份检查，不是CPU仿真投入。共享factory/cache仍使用实际source bytes的key，新的源码及ABI salt使旧program不可冒新产物；不在map后补GPU PASS或改门。
6. developer按真实生产新路径生成、静态编译最终同一份canonical shader、全native/ArkTS/unsigned包编译，并从实际链接ELF抽取原串核SHA/字节数/行号映射。静态日志记录实际工具路径/版本/flags/宏/语言目标/退出码；失败保原错，不以只过旧harness编译作通过。开发完成+编译交付即可，签名部署/设备回归属于tester，不加设备PASS或manager批准门。

这是一份明确边界的兼容/身份工程修订，不要求本architect先写或运行新shader。不能承诺改名后设备必过；后续若仍拒，按新源原串/精确firstFailure重新归因，而不连续盲换变量名。

## 4. 原T645后续精确采证

已有test设施保留，修复后的生产包/native/源身份齐即可由原T645 tester签名部署，固定一个配置attempt，保存实际loaded file SHA/BuildId+scope/clock/window/context/ownerTid、program角色与handle、实际sourceSHA/长度、编译器原日志、compile/link ENTER/EXIT和生成行映射。优先复用现有program Artifact receipt的actual shaderSHA/route/shaderLogBytesHex/原始错误与source-manifest；它只能证明自己的实际来源，不能用默认中间量角色名误认成M2程序。若现有RX冷/状态出口无法取到该receipt，保精确调用/原错后交architect定义**该缺字段**的有界只读seam，不泛加日志线程或新的GL查询。

READY、pulse/触摸、T642 N01–N11、FIFO/真实DM数学/审核/replay、CURRENT-BASE冷档/reader、成本/同token新管线帧均沿原卡完整未完，不能由LOAD身份或编译成功冒PASS。两次CONFIGURE_FAILED_8是创建工程失败；未到后段记录NOT_REACHED，不误称后段产品FAIL或已完成验证。

主树cache64次重复flags去重后的ninja rc0仅保为那次构建结果；幂等修复独立在途T666，不是本shader归因新前置，也不从此宣所有卡/全工具链永久已过。

执行prompt：developer不使用真机；tester/effector维护测试脚本、签名部署与真机串行采证。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态有限例外），不得更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。GPU优先、CPU停止、10暂停、水13→14→12、旧湿M1暂停；背景前景仅XComponent+shader，FAULT或明确STALE，无Canvas/软件GL产品回退。无manager新增验收门；本卡工程归因完成不冒产品PASS。

## 2026-11-20 T691 续归因

T674 实际 canonical 通过原静态编译，真机新拒绝定位到第1063行 coherent C元素直接传 g9AddMass 的标量实参。新增有限的值复制兼容变换计划，保 coherent、屏障和作用域修复。T594 RX实测与V5入口须分开，详见 [T691](architecture-T691-v1.md)。
