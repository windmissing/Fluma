# T423 开发交付

2026-11-09。按 T420 §5–6 交付同一隔离包内的 `LEGACY_FULL` 和 `LOCAL_UNDO_V1`。页面默认旧路，切换时新建 run，正常页面及非隔离调用继续沿既有入口。候选在同一个 owner 的私有候选 bank 原位写入，保留原 STEP/source 事务、公共发布与失败回滚边界。

稀疏干态 apply 中，最多 128 行时可按已闭包的单行选择 local。支持单粒 T3/T4/T5，以及不触发合并且最多四个空槽的 T1/T2。稠密计划、合并、湿态、未知形状或容量不足走完整 bank。候选直接把权威池读到 scratch，以旧位型日志保留 before，减少一条每行整池复制。原 `scratch→after→authority` 保留。需要完整 before 的冷失败、外显 restore 或尾切点会原位回放旧字，物化并单独计费。原事件、票据、exact70、K16、尾部重验、公共全验和第二步失败的已提交前缀保持。

| Writer / cut | 登记与恢复 |
| --- | --- |
| `P011ParticleHot.editPoolHot` T1/T2 | `scratch.pool` 原 SoA 1/4/8B lanes，最多 4 个已核空槽。旧 `liveCount/freeCount/nextId` 随 mark 保留。原写顺序、seed、guard 和 birth 数式照常执行。 |
| T3/T4/T5 单粒 | 同一 ID 所在槽；旧/目标列。T3 全额或 T5 的 free stack 写入预存旧 word。T381/Z08 在行边界只消费对应旧槽和完整 after。 |
| `DerivedViewV2.finishActual` | 每触列的双相 70×U16 limbs、两计数、两 cache、dirty，加触槽 generation/ID 与 view revision。保留原算术和 verifier。固定 339 槽，日志六组 U32 合计 8136B。DataView 对象与原有 bank 另计。 |
| 事务外沿 | `P011ParticleM2Sink` 的 field/ticket/event 顺序与原 checkpoint 仍执行。`WaterSandField` 保存 preview、authority 和失败前缀。`P011DerivedCommitV2` 仍执行 binding、structure、exact70、strict、authority bytes，再 seal。未知外部 writer 不取代原全验。 |
| 失败原件 | `DerivedCoordinatorV2.fail` 在冻结前物化当前逻辑 before，并用旧 word 复原。有效 pending 行不传播到 publication。原 root rollback 继续保护本次根之前的提交。 |

同输入两路执行 13 个场景，覆盖 N192/N512、Q0/1/2、空场、逐笔注入后停源、稀疏和稠密接触。原始 `test/reports/t423/units.json` 保存每帧 Q/A/R/L、互斥 leaf 时间、字节账、T366 role/audit 记录和 p50/p95/max。复制计数是逻辑 payload bytes，不能直接换算带宽。Node 的墙时含宿主开销，两臂串行执行，仅作复跑观测。

| 负载 | 每帧最大 R | local / fallback 行 | 净减少整池 payload | 旧路 / 候选 step p95 |
| --- | ---: | ---: | ---: | ---: |
| N192、L12 阻挡跨列 | 24 | 48 / 1 | 529536 B | 11.98 / 9.52 ms |
| N512、L12 阻挡跨列 | 24 | 48 / 1 | 1268736 B | 4.52 / 4.49 ms |
| N512、L101 接触 | 404 | 1212 / 1 | 36249600 B | 40.84 / 36.95 ms |
| N512、L157 接触 | 628 | 0 / 1885 | 0 B | 57.80 / 56.03 ms |
| N512、L237 接触 | 948 | 0 / 2846 | 0 B | 84.84 / 85.12 ms |

每命中行删除 `55N+16C` B，旧路径四条 pool payload 边变为三条。扣除物化后，单行 admission/停源小样本的净收益为 0。高接触 R628/948 与历史 R534–606/942 接近但未同条件匹配。以上不包含设备呈现和 UI 调度，不能判定目标帧率。K16/full/inc 计数在样本两路相同，例如 N512/L157 的代表 role 记录为 44 full、588 inc。

T381 候选路回归通过九个注错切点、受阻原列下落、终床变化、容量/预算/同向反向及成功前缀。T400/T403 候选路正反控保持 LEVEL0 `[0.1,0.1,0.09999999999999998]` 和 TILT25 `[0.1,0.032121583881089655,0]`。实际 publish 后注错对照 213 个冻结 buffer、首因和原始边界。两个独立 run 中的全局 ticket/patch handle 只按各自等价关系规范化，非身份位型直接对拍。第二步 prepare 故障保留首步 commit4。诊断开关关时再对拍通过。

剩余成本仍含公共全量证明、运动 collect/投影、主 bank 发布及真实画面消费。减少公共扫描仍需 T425 证明 ordered RN64 fold 和 exact70 每 prefix 首错、slot/free/ID 变化、dirty 传播及 rollback 下的逐切点等价。保留同步、每成功版本原发布；异步背压与新 ACK 没有已批准语义，当前切换仅发生在新 run。T424 可在同包选旧路/候选对拍。

复跑：`node test/t423/generate.cjs .codex/local-test/<新目录>`，再运行 `node test/t423/units.cjs <隔离工程>` 及 `node test/t423/regression.cjs <隔离工程> t381`、`t400`。生产签包 `T423-release` 的 Hvigor 全量 30 tasks 成功，签名和验签通过。未使用真机。渲染由 XComponent + shader 执行。

签包 `.codex/local-test/t257-sign/T423-release/T423-release-signed.hap`。SHA256 `4f8f845a437606ab1c9291faa33e4c6f8eb0cd7b0f3c3e210f1904483fa929ce`。
