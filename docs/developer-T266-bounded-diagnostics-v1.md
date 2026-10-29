# T266 有界诊断、run时钟与合法启动交付

2026-10-29，developer，不使用真机。按T265恢复的T243 v2；隔离工程 `.codex/local-test/t266-v9`，生产源码/默认路由未改。

`T266Clock` 绑定真实run/stateEpoch，变化时清零物理秒/首次出生锚点/请求秒/准入质量/拒绝计数与源/step记录。只累计原`result.substeps * substepDt`，不以root的ok/stepped推算全部成功；256行step记录保留requestedDt、committed子步、commit前后、原首码与时钟前后。256行source记录保留sequence、请求秒/真实准入、commit前后与原拒因。两个记录区满都累计dropped，不填零冒完整数据。源率当量明确仅mass/8；首次正量干沙成功准入绑定真实birth commit，物理秒另算。

沿原WaterSandField 206个函数作用域接预分配4096行TxWorkTrace，保留root/parent/原code与inclusive/exclusive；上下文为substep/commit/capacity/live/stateEpoch，阶段编号词典在manifest。模型用caller monotonic ms，配wall对齐点；trace满显式INCOMPLETE，UI阻塞期间不能导出最后阶段时NOT_CAPTURED。原物理函数、dt、准入、事务/rollback不变。

正常帧每60帧输出固定阶段摘要，完整authority/publication/packed仅首次非零床量及首出生后成功物理秒≥8锚点和随后3帧、或显式`t266heavy=true`重日志臂采。记录编码/日志上一采样成本（明确previous，不冒当前成本）。原每帧大JSON/pack Array.from和native glReadPixels已退出正常路径。native定容256行记录目标准备、独立UPLOAD_EXCLUSIVE、分层draw、present、swap与token哈希；NAPI冷drain保留dropped/GPU timer NOT_MEASURED。观察器同一run clock供publication快照使用，不再创建零时长新观察器。

隔离EntryAbility唯一默认分支命中数量与已知SandCanvas默认断言后，冷启动固定到白名单内`pages/P051DerivedV2Demo`；未知版本fail closed。只有V2页面，无Canvas回退。`t266heavy=true`仍可显式选择重日志对照；两臂身份随manifest/包保留。原T248逐帧像素探针不作为重日志臂恢复；像素需tester另作显式设备capture。

离线24/0：run/epoch重置、失败/未执行不累计、真实root第一子步提交第二子步规划阻断时仍累计1/60秒；65步观测开关authority池/床/receipt/原首码/commit逐项相等，覆盖3次非空LAND事件页；定容满和合法路由检查通过。两独立run的runId只作各自身份，不强求字符串相等。native/ArkTS/PackageHap BUILD SUCCESSFUL，HAP核唯一V2路由、ABC与真实native阶段代码PASS。证据 `test/reports/t266/{offline,manifest,delivery}.json`、`v9-package.log.gz`。

签包verify-app/profile PASS，`test/reports/t257/T266-v9.json`。signed SHA `9e0bd6b2d28f56b450cd2bb1cd0a9bee7a9b4f9b9e002e3c3eecb6482493895d`；包路径 `D:\hmprojects\HelloWorld\.codex\local-test\t257-sign\T266-v9\T266-v9-signed.hap`。T267用此包冷启动，正常/重日志臂分别新run，目标≥8成功物理秒，沿同token锚点取床/packed/像素证据。

开发交付完成；未做设备冷启动、8秒锚点、性能收益或像素验收。环满/日志系统截断必须按INCOMPLETE/NOT_CAPTURED处理，不能以本卡编译、原WM规格或离线LAND代产品PASS。失败/阻塞新证据仍先architect归因。
