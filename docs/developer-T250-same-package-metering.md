# T250 旧事务三backend同包计量：部分交付

2026-10-29，developer，不使用真机。独立合包/两freeze臂/统一字典已实现和编译；完整20cut、planner λ及总资源计量未齐，SignHap失败，状态看评论。未改生产默认或 gpu_resident 主核。

`test/t250/generate.cjs` 在 T212 严格隔离工程上叠加，实际编入 TreeAdapter/core 和原 GPU_PAIR native bridge，同一 Worker 接三backend。`txSuite=true` 在一个 Worker/PID 顺序执行六组合（COMMON_FREEZE_OFF/ON × LEGACY_SCAN/TX_ADJ_AVL/GPU_PAIR_V1），每路独立物理owner/native configure→retire，归档 process.pid、相同包/source SHA、冷config与原 fixture。两臂只开关额外共同freeze；GPU协议必须的冻结从不关闭。额外成本与原native freeze分列，不把CPU0当GPU缺测。

每个source/step root独立原before、receipt/firstCode、after及回滚域原字节归档，synthetic满池入口保持T171 WET_192_64/WET_512_64，非现场重放。B记录原实际birth写完成，M记录原merge返回0及尝试数，A记录实际role边界，source λ记录原wet source retry loop。失败/回滚工作仍计工作量，accepted分母用原receipt，不从offered推birth。step planner λ、各访问/比较/动态分配未被这些计数覆盖，不能冒完整 B/M/A/λ 工程已齐。

64槽独立词典逐字段指定原单位/作用域/分母/inclusive关系；原native多项计时只适用于GPU请求，CPU缺测null/NaN，槽62/63等未定义槽不比较。不得逐槽相减推收益。原nativeRows可采owner/PID/live/peak；原resource仍含 UNBOUNDED_EXTERNAL/NOT_TOTALLED，JS/driver总峰值未测。新rootRows数量≤34但原字节字符串大小依输入，不据逻辑payload声称真实RSS。

20个cut注入面逐项 NOT_IMPLEMENTED；当前原始root前后/回执不是逐cut证据。也未实现20cut faultPlan注入/撤销、FIFO queue服务延迟与全量lambda scope。需要后续工程补齐后T251才能跑对应矩阵；现有局部正确性无需重验。生成manifest保留这些边界，未放宽任何物理判据。

离线12组（两N×两freeze×三backend）终态一致，GPU使用明标CPU double，仅验证闭包/边界。真正device GPU/PID两臂/收益 NOT_RUN。实际独立工程 `.codex/local-test/t250-handoff`，生成：`node test/t250/generate.cjs .codex/local-test/t250-NEW`；Want沿T212增加`txSuite=true`和`txFreezeArm`，其余txUnsignedSha/fixture/run/steps原字段保留。hvigor CompileArkTS+BuildNativeWithNinja+PackageHap通过；SignHap00303116密码字段长度错误，未改密码。字典、自测及源/ELF/HAP SHA见`test/reports/t250/`。
