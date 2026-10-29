# T240 GPU 双kind source 核

2026-10-29，developer；不使用真机。接续595239f，新增独立M1 `source_core.comp`，4 binding、4 dispatch。GPU执行base→candidate/spare复制、双kind source、库存审核/terminal32、event64旧新值回放及逐word candidate/spare比对。三次源结果分别为沙按质量包出生、DIRECT_W只增W、FLIGHT_J真实水pool出生并只增J；W+J总量/I_water、沙总量/I_sand和source/physics commit分账。正量化/容量/slot/ID/代次/完整key重放/冲突与命令域均在GPU检查。

water16、terminal32、event64按T238 revision2；真实MAX水ID可用一次、随后0耗尽，DIRECT_W不需要ID。full32命令/terminal32在GPU工作区保留；base accepted prefix决定可见表项，失败candidate不影响已接受base。事件篡改导致REPLAY_MISMATCH=16，不接受该candidate。

实际PC SwiftShader执行12/0：沙出生、J出生、DIRECT_W、同key重复零副作用、payload冲突、非法量/范围/kind、事件质量篡改拒绝并保base、MAX水ID/后续拒绝以及耗尽后仍可DIRECT_W。日志`test/reports/t240/source-host.txt`，展开shader SHA/语义编译`source-shader.json`。native/ArkTS/PackageHap BUILD SUCCESSFUL，`source-package.log.gz`。源核已作为独立native函数编入基础库，M0shader/reader未改。

**部分交付，看评论；非完整M1 source transport。**当前source核尚未封存sourceEvent16/完整completion256、bank接受owner及publication；头部root/frame等全身份提交也要由完整transport统一完成，不把terminal32直接冒产品ACK。水T16/TURN/WAIT、ARRIVE/侧扩散应用、湿沙/全quantum审核及水scene/APP_ROUTE未接。本核只由隔离测试driver调用；对外Plan仍明确NOT_IMPLEMENTED，不开放半成品M1功能。PC软件GL不是设备WM或硬件收益证据。
