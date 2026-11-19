# T655：Windows 长链接命令与响应文件首次归因

2026-11-19，architect，不使用真机。结论 **BUILD_CACHE_FLAG_ACCUMULATION / TESTER_ISOLATED_BUILD_FIRST**；合并T590/T541同类构建问题，未证T629是根因，未排除历史编译器问题。正式表`architecture-T655-contract-v1.json`、保存证据`analysis-T655-evidence-v1.json`及共用只读脚本已交；本卡不执行构建、链接或产品测试。

## 决定性原件与因果边界

保留日志两条失败命令均明确由`cmd.exe /C`包裹clang++，目标libgrs_cold_slots.so。去ANSI后的实际字符数见证据，不直接照抄报告11876；每条含**62组**重复`--rtlib=compiler-rt … -Wl,-z,noexecstack`。当前cache/规则是稍后的独立快照：CMAKE_SHARED_LINKER_FLAGS已有**63组**、约10.5K字符。两者不能冒同一时刻完整argv，但都证明主要膨胀在链接标志，不在对象清单。

broker目标只有cold_slot_broker.cpp一个对象，现有Ninja已用`@CMakeFiles\grs_cold_slots.rsp`；rsp原字节仅该对象与-lm，未混入grs_foundation/G9/G11对象。当前rspfile_content只含`$in $LINK_PATH $LINK_LIBRARIES`，**LINK_FLAGS仍在shell外层**。因此“再开对象rsp”不能移走这批重复标志，不能将冷broker异常泛派为所有shader对象超长。

实际SDK toolchain在每次求值时以CACHE FORCE将`${OHOS_COMMON_LINKER_FLAGS} ${_DEVICEINFO_LIB}`前置到已有CMAKE_SHARED_LINKER_FLAGS；缓存反复配置可累积该完整块。T629的napi_provider_profile.cmake只加SDK头依赖/编译定义，并不设置链接flags。它可能促成一次重新配置，但时间先后不足证明它创造了累积或直接损坏rsp。真实触发次数/原始用户flags来源须构建收据说明，不能猜。

保留日志没有完整cmd/Win32原始错误正文、CreateProcess返回或失败的manual-rsp native argv；所以“cmd限制具体报错”仍属需要补齐的执行证据。外层超过报告8191且由cmd发起，使shell长度门为首候选；不能从hvigor的Configuration Error反推clang已启动，也不能把未保存的丢slash认作CMake生成器确实转义错误。rsp当前路径/内容本身有实物，正反斜杠须按消费者分层解释。

T541 a1还混两种**有意但未分名**的标本：SPF命令取t564-stage-tree及旧冻结header；DM命令取当前main-tree及其当前生成目录。304项manifest部分SHA仅16hex前缀；不能拿它宣整套冻结源/完整SHA全部一致。1–2s预处理/编译成功只证明这些标本本次未复现旧38/60分钟挂起，不能反证旧compiler内部问题不可能或把历史全部归ninja/cmd。修正输出-o等派生命令须同时保原始/派生argv，不声称逐字原argv。

## 原 T590/T541 tester 续办：先修调用与构建状态

选择**独立新输出/新CMake cache**，保留原污染cache、rules、rsp、命令、SDK和源SHA；不删除全局缓存、不换SDK、不删安全/FP门。两个标本要么各独立manifest，要么一次完整一致源树重建，所有-I、实现cpp、生成头和对象必须归同一声明闭包；不借旧header或另树对象省步骤。

原flags分用户显式基线、SDK策略块、target追加。新配置后先检查生成broker/实际待链接target的完整命令及各块次数，若同一cache继续配置发生累加，在该attempt收据明确记录。不要把“新输出”当产品数学修复。现成包构建/签名/部署/取证属tester，无需新增developer关闭门；G9rx已交独立正确包不因本主树污染自动blocked。

原生执行用结构化argv数组、明确cwd、`shell=False`或等价直接创建进程；响应token作为**一个参数**，例如`@D:/.../CMakeFiles/grs_cold_slots.rsp`，不用POSIX shlex解析Windows命令，不把JSON.stringify当shell转义，不将字符串经PowerShell→cmd→sh多次重解释。当前relative rsp须以实际Ninja build目录为cwd；改绝对路径保留原文件SHA，不重写内容冒原件。手工复放失败先保实际driver argv/退出码/stderr，不能把猜测的`CMakeFilesgrs...`当生成器事实。

## 持久工程边界与 target 级完整 rsp 合同

若干净调用已足够，当前不要求改业务CMake或SDK；若复用输出的配置生命周期需工程修复，交repo-owned构建wrapper/策略模块维护**已知用户基线+单份SDK块**，必须幂等。不能对任意linker tokens做全局REMOVE_DUPLICATES，库次序、start/end-group、用户覆盖选项等可能有语义；不要直接在安装SDK FORCE修补或覆盖整份cache flags。

若干净target真实合法flags仍超外层限制，developer的明确工程范围是`cold_slot_broker.cmake`及repo-owned `tools/build/link_from_manifest`一类**target限定**构建接缝：外层只放短launcher/manifest路径；manifest生成全部实际driver flags、对象、库及顺序，完整rsp覆盖LANGUAGE_COMPILE_FLAGS/ARCH_FLAGS/LINK_FLAGS/对象/搜索路径/库，不能再只搬对象。launcher直接以native argv运行clang++，不能又展开回cmd。其目标、输出SONAME/安装打包、八符号/DT_NEEDED、增量依赖/depfile/新输出必须与原SHARED目标一致；不得将broker.cpp静态混进每route。

rsp是driver参数格式：UTF8无BOM/LF、路径canonical forward slash；空格、引号、反斜杠按**该实际driver rsp解析策略**编码，Native argv转义与rsp内容转义分开。必须保原rsp字节SHA并用实际driver的诊断argv/编译链接结果核消费路径；不宣通用`replace`/shell转义适用于所有层。命令长度、有效flag列表/出处/重复数、cwd、工具SHA、对象/生成输入SHA与新ELF路径/BuildId/整文件SHA成套交付。只修target工程/编译，不引入设备门或要求tester修改产品数学。

原T590 probe按实际正确目标包/解码前置续测；T541按独立标本完成尚未做的链接/原精确验证。历史挂起只有新出现且有同attempt进程等待/栈证据才再分流，不延长旧空等。不得放宽-fno-fast-math/-ffp-contract=off/FP门、物理/GAP/容量或扩大CPU路线。

共同执行约束见`architecture-T655-T658-boundaries-v1.md`。本卡归因/合同已完成；现有构建失败和未测产品指标均未记PASS。
