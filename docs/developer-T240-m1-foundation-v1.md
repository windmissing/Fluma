# T240 M1 独立版本初始交付

2026-10-29，developer，不使用真机。

新增 `gpu_resident/m1`，从当前 T238/T259 manifest 生成 water16/header/source/event/completion/publication 全部 V2 word 索引、manifest SHA 与四档 K/Kw 资源配置。冷 profile 检查 revision2/feature3、范围/对齐/大块上限，M0 revision、M2/M3 feature 在任何库存写前拒绝。四档范围/版本/feature离线16断言通过。native/ArkTS/PackageHap BUILD SUCCESSFUL。

新增独立 ES310 `water_edge.comp` 的 head2 五界 proposal，含 P/J 受方余量、signed slope、宽整数 kappa*delta/240 与明确非法库存状态；私有输入是五行 SoA `[S,W,P,J,reserved0]`，不是 publicationColumn16，尚无产品调用方。仅输出 proposal，不直接更新库存或绕开 event/replay。glslang 语义编译通过，实际 shader 执行尚未做。证据 `test/reports/t240/{profile.txt,shader.json,package.log.gz}`。

**本卡部分交付，看评论，M1产品未实现。**冷 Plan 明确返回 `ProtocolReadyShaderNotImplemented`，不返回可提交handle；没有启用M1 APP_ROUTE。双kind source/W/J、T16分段/TURN/WAIT、真实水排序/ARRIVE、全相审计/事件重放/尾completion、water publication/scene仍待后续实现。独立WM规格通过与本提案shader编译均不能冒M1产品PASS。没有CPU-only效果或M0/Canvas回退。
