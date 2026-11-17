# T536 八族特化与完整冷预备开发交付

2026-11-17。按 T530 §4，独立覆盖 T507 最终 M1/M2 生成路线。用户限定开发、编译、提交；未执行测试、物理 dispatch、真机或端到端验证。运行期冷预备是否落在500ms、物理全等和实际收益留待独立执行，本卡不称性能通过。

`test/t536/generate.cjs <fresh .codex/local-test/path> m1|m2` 在 T507（含 T492/T501/T502）真实增强来源上接线，默认模式和原数学保持。`overlay.py` 复制规范 native 实现，分别生成 route 的 shader/ABI/manifest；不以 m3a 代码替代旧路线。

`specialize.py` 从实际完整 Q shader 提取29个原 stage body，按 T530 八族重建入口；保 copy0/rollback20/completion27 原先位于错误门之前的顺序。构造函数依赖闭包后删除不可达定义，交给驱动的是已裁掉其它族入口和函数的源码。原 M1 组装字节与 CMake 实际 `full_m1_kernels.h` 的完整源 SHA 相等；M2 从真实完整 literal 提取，不套 M1 数学。

原固定 pass 表、sort轮数、bindings、barriers、审核/replay/publication与根时限不减、不重排。八族所有权为 COPY{0,20,22,28}、PREFLIGHT{1,24}、WATER{2,3,4}、INDEX_SORT{6,7,8,9,16,17}、TRANSPORT{5,10,11,15,18}、BED{12,13,14}、AUDIT_REPLAY{19,21,23}、PUBLICATION{25,26,27}。

规范 native 实现在 `entry/src/main/cpp/gpu_resident/stage_specialization/`。Programs 定容持有八 program、每组反射表及具类型的 uniform shadow；切换组时仅恢复需要且变更的值，找不到的优化字段不强制active。原 wire uploadedBytes 保持原逻辑上传口径；额外真实 GL uniform bytes/program switches 独立累计，不拿逻辑字节冒实际提交减少。协议 staging 故障走 PROTOCOL_REJECTED，不伪造 raw GL 错误。group 源/route/ABI/环境各自参与 cache key，角色具名 Q_COPY 等，淘汰/32条/128MiB维持原机制；不用原大 Q program 假称同 key 命中。冷 receipt 和资源 ledger 记录新增固定 host 存储/临时GPU对象；program 对象走既有真实GL ledger。

READY 前 `PrepareAllStages` 使用同 owner/current 撤权和原500ms政策：

- 私有 event、SOURCE work、publication、completion四对象；Q candidate/work原本即私有。真实 bank 仅绑定只读 B，stage28 目标为私有缓冲，不覆盖 SourceRootOwner bank。
- 完整 SOURCE M1八阶段/M2十三阶段，以合法正量模型 fixture 覆盖真实 decode/分配/replay/审计域，检查私有 completion。无 owner.Begin/Accept、Run quota、业务 source commit 或 Swap。
- Q 执行原全表，覆盖当前档最大排序规模、所有八族、完整 audit/replay/metadata/publication/seal；尾输出私有，检查GPU首错。BASE前后分块只读FNV摘要必须一致。
- fence/Flush/冷 map/清理全保 owner与期限检查。失败明确 `T536_COLD_PREPARE_V1/COLD_PREPARE_FAILED`，记录cut/原GL/TransportCode/实际issued/临时峰值/真实uniform提交；不放宽时限或伪报预备成功。无论成功失败都退休自己的临时对象，不改变业务资格。

普通每活跃route SOURCE+8Q+3scene 为12角色；Programs固定 host 大小由sizeof登记。临时冷GPU峰值为 `eventCapacity*256 + source.workWords*4 + source.pubWords*4 + 1024`，独立于普通驻留，四对象不与已持有业务state混为一份。实际冷墙钟/driver驻留/根吞吐没有测量结论。原批10op/8ms检查、250ms根、双GAP、单热尾fence/1024B map不变；不可抢占GL调用超时后仍按旧门拒绝。

M1/M2最终工程 `.codex/local-test/t536-delivery-m1` 与 `...-m2` 均完整34项构建通过；最终协议错误分流收口后各33项增量构建通过，零编译错误。两路线各八 shader 编译/链接通过，调用编译器未 dispatch/draw。最终签包/verify通过：

- M1 `d267a1cc447cd9751eeb196ce87e00bac568aa4ad9d3d08de5557dc4c83b578e`
- M2 `8137594febb4bbd970667f3461677976611a31ef108e29b4bc5d841417163ed0`

最终而非早期delivery收据为 `test/reports/t257/T536-m1-final.json` / `T536-m2-final.json`。逐族源/函数闭包/ABI、完整原源SHA、生成 native SHA、构建日志与签包交接在 `test/reports/t536/delivery.json`。签名材料、根工程profile未改，不安装设备；剩余独立数学、冷热对照与真机成本/期限由tester接力。
