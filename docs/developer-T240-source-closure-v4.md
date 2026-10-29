# T240 收窄范围：GPU source 到 native 接受与 publication lease

2026-10-29，developer，不使用真机。按原卡最新收窄范围，本轮完成 **source GPU core → sourceEvent16/completion256 → 独立M1 source transport/owner → accepted bank/publication lease**。此前 c9f524e/v3 为部分交付；本稿取代其对source关闭范围的未完成结论。T16/水运动/ARRIVE/完整quantum/水scene仍未实现，继续独立任务，不宣完整M1支持、不启生产默认路由。

## 实现

`m1/source_audit.glsl` 增加真实source-only审计，不直接放宽接受mask：库存/U64守恒/容量与spare逐word replay后，再核ID非零/唯一/代际/耗尽、空槽清理、双相来源与累计终态、sourceEvent16逐字段、live包与原terminal的质量/位置/来源一致、零运动预算/床guard的逐字节不变身份。历史source累计序号/总账、settled/flight、append去重前缀对应。该类run从空态开始且不能提交quantum，所以运动预算为0、床仍0是可核的source限定身份，不冒已实现整步预算/湿沙guard。

publication重新密集打包有效沙/水前缀，单独dispatch逐字段核sand16/water20/column16/header256、未用/保留位及完整身份/版本/SHA；读写分pass。全部通过才输出auditMask=1023及255 stage mask。source-only共8 dispatch/9 barriers，completion准确带eventArena和实际上传字节/容量最大值；错误仍fatal，accepted bank不切。

新增 `m1/SourceRootOwner` 使用revision2的SOURCE专用接受门；拒M0/QUANTUM，不复用M0仅沙的尾包语义。核256word长度、magic/revision/seal、run/owner/context/surface/geometry/config全64位、nonce/control/frame、全部版本/三SHA、bank与pub代际、stage/audit/replay、原GL、prefix和receipt、双相mass/I/settled/flight、ID及余量。成功/元数据拒绝/重复按其真实含义接受：replay可带原admitted但本次不加I/commit；受理bank/summary/receipt/pub lease在一次无回调owner操作中更新。临时等待/late/tombstone不接受。

新增 `m1/SourceCoreTransport`：同native GL owner/current context、独立新空source-only run、两state bank+workspace spare、双event/pub/tail；显式查询能力/对象range大小、冷完整初始化、配置失败退休。每pass四SSBO；尾fence异步0超时poll，普通路径只map **1024B completion**，不读pool、CPU不积分/补算库存。accepted pub按完整generation/token lease，已lease/releasePending槽背压，真实release fence退休后复用。显式Stop后需新run；late signaled fence不map、不更新bank；context lost为STATE_LOST。freeze/capture才允许冷整state导出。trace为有界256条，溢出明确Dropped；无伪装无限完整记录。

这是隔离source模块，不是完整configureM1：独立source workspace有其真实大小/生命周期，不拿完整M1资源模板的payload声称设备分配/驻留PASS。完整M1 `Plan`仍返回ProtocolReadyShaderNotImplemented；默认M0/页面不迁移。CMake编译新增owner/transport；没有改旧M0物理/renderer，背景前景仍XComponent+shader，FAULT/完整旧token STALE规则保持。source publication lease不等同完整scene swap/CURRENT。

## 非机验证

`prepare_t240_source.py` glslang ES310语义编译通过。`build_t240_source.cmd` 真实PC SwiftShader GPU **20/0**：原双相/重复/冲突/零源/ID末值/重放篡改，新增7个GPU候选/终态/guard预算/保留位/pub篡改反例，真实fatal15且base逐word不变。

`build_t240_transport.cmd` 与同exe容量参数在 **K192/512 × Kw64/128** 四组合，各 **562/0**（总2248断言）：真实8-pass GPU→fence→尾map→native owner接受；双相/DIRECT_W/FLIGHT_J、重复/正常拒绝、不合格式/身份/版本/SHA/审计/保留位等尾包篡改、长度/GL拒绝、pub真实完整身份冷读、持有lease槽背压、release fence后复用、非owner context拒绝、Stop的late fence无map/无bank变更。普通map量每被消费尾包1024B，Stop晚尾0B。宿主只在显式冷证据处整state/pub读回；这不冒真机零读回成绩。证据 `test/reports/t240/source-closure-v4.json` 含源SHA及四组合原日志索引。

fullM1 profile gate **16/0**，旧M0基础control **87/0**。hvigor root PackageHap **BUILD SUCCESSFUL**（source-transport-package.log.gz）。同一工作区编译包签名/验签成功：`.codex/local-test/t257-sign/T240-source-v4/T240-source-v4-signed.hap`，SHA256 `763d7aeba1a74cdbaea0e7cb6732341d105153330bfa4e77a4c42c1a2119df82`，签名记录 `test/reports/t257/T240-source-v4.json`。生产路由关闭，包不声称已能从页面运行完整M1；真实执行证据来自链接相同C++/GLSL实现的PC host。

未用真机，无性能/设备/观感或完整水scene产品PASS结论。开发交付满足卡片已收窄source闭环，不新增后续整步效果为本卡关闭前置。
