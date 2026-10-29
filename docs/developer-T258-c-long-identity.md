# T258 C V3 完整长身份读取修复

2026-10-29，developer。不使用真机。按T254，仅C V3 frameToken/configDigest改用clear_water_identity.h：先查询实际UTF8字节数，≤8192/4096、checked+1后一次临时分配，再读并核真实复制长度/NUL/完整字节。空/内嵌NUL/错误类型/超限拒绝，无hash-only/截断。超限阶段CW_V3_PARSE_IDENTITY_LENGTH，其它身份错误CW_V3_PARSE_IDENTITY，原INVALID_ARGUMENT状态不变。临时读取内存随调用释放；失败清空请求身份，不把上次长token作为本次requested返回。通用256B String实现逐字节未改，其它路由不扩容。

23/0离线测试，使用实际Node N-API ABI调用产品读取函数及产品ClearWaterAtomic/ValidateClearWater，不是复制读取模型。真实clearWaterCConfig()+clearWaterCBConfig()和ClearWaterNativeFrame.bind()产生996B token、854B configDigest，ArkTS源经TypeScript开发执行→Node native addon全字节一致，合法完整packet返回同token；重复长token由实际atomic拒绝，GPU边界调用0。覆盖255/256、8192/8193、4096/4097 UTF8字节，非ASCII按字节边界、空/NUL/type错误均在GPU前拒绝。GPU接口明标synthetic，不是Harmony运行时/真机六层绘制通过。

真实hvigorw PackageHap/native/ArkTS BUILD SUCCESSFUL、零错误。test/reports/t258/含JSON、原构建gzip、包与ELF SHA；test/t258/build.cmd可编译Node addon并执行真实ETS配置接缝，依赖已有本机p013-host SDK头及MSVC。无Canvas回退/物理改动/阈值变化。设备定点复测s144由独立tester执行；签包按T257后续，不作为本开发完成门。
