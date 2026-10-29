# T250 收窄终点交付：20 BEFORE 与逐点原档

2026-10-29，developer，不使用真机。按原卡最新范围，仅关闭20个BEFORE故障面及原始冷档/非机自测；AFTER由T278另卡，设备及外部driver峰值不属于本卡关闭门。历史 bfc52bc/v9 为3/20部分交付，本轮补齐其余17点，没有停在单次切片。

## 入口、产物与实际边界

生成 `node test/t250/generate_before.cjs .codex/local-test/t250-before-v30`，复用原三backend同包工程，新增BEFORE专用生成层，不覆盖原T250/T278生成器。生产源码未增加fault hook；物理阈值/λ/准入/预算/事务数学未改。`TxFaultPlan`仍核fixture/build SHA/backend/freeze/PID/root/cut/BEFORE/ordinal。旧cut1/6/18入口保持；新增各点只修改尚待原检查消费的隔离输入，finally撤销，再由原caller执行其真实重试/rollback。

正式字典 `test/reports/t250/before-v30/cut-inventory.json`：20行逐项原函数/谓词/源SHA/行号、可控输入、原return或exception、firstCode领域、根/ordinal和rollback/成功前缀边界。历史V1和T278 AFTER字典保留；不将AFTER、OBS/NA或同一消费者重复充入BEFORE分母。

| cut | 实际原返回（数值不重编码） |
| --- | --- |
| 1 / 2 | 原source precheck null/txCode0；原λ容量拒绝receipt CAPACITY_BLOCKED/txCode0 |
| 3 / 4 / 5 | 原begin未配置1；原collect4；A与B collect各自4（ordinal1/2独立） |
| 6 / 7 / 8 | 原request identity4；原sink active前置4；原prepare return0+workStatus PATCH_BUSY6 |
| 9 / 10 | 原selector抛TX_GPU_NOT_REUSABLE:STALE，原merge caller映射13；原publish lease校验4 |
| 11 / 12 / 13 / 14 / 15 | 原SINK_ROW audit1；原ticket close lookup4；原plan重复消费15；原APPLY_TAIL audit1；原finish active校验4 |
| 16 / 17 / 18 | 原preview null；原root receipt errorCode5；原field NEGATIVE_VALUE1（父root5） |
| 19 | 原发布前safe-integer检查抛SOURCE_EPOCH_OVERFLOW，未发布新源 |
| 20 | 现有opt-in probe的原key检查抛ENDPOINT_COMMIT_KEY_MISMATCH，属于观察封存异常 |

cut9/19/20保留原异常消息/归属，trace实现中的catch -1不冒新的物理业务码。cut20并非给终观察void接口新造拒绝：启用现有probe和固定端点clock，在 `finishNaturalObservation` 内、原 `stampEndpointObservation` 消费stamp之前，暂改key.runId，由已存在的 `P011NaturalObservationProbe` 身份检查真实拒绝。物理子步此时已提交，测试断言commit+1保留、源账不变、key撤销且观察故障前后物理原档相同；不伪造物理失败或回滚已提交子步。这是BEFORE消费者验证，和T275裁定“终边界无AFTER下一消费者”相容。

源root2各失败候选沿原λ重试后可准入3.2，root1已成功19.2始终保留；不把夹具“要求全量”终止提示TX_GPU_ADMISSION_FAILED当新增物理码。cut2的原夹具offered在finally恢复，另有INJECTED_READSET明确实际消费1e9，不用恢复后的19.2冒实际故障输入。cut5/12/16/17/18自动选合法SHAKE1/TILT20冷输入；cut20自动选现有端点probe/clock，worker按既有faultPlan即可触达，不依赖Node私有参数。字典给出每点rootId，错误root未命中不能宣PASS。

## 原档与恢复判据

42个故障case（20×两freeze臂，另加cut5 B各臂）及6个相同输入模式的撤销控制，各自独立gzip和SHA，索引在 `test/reports/t250/before-v30/summary.json`。无单一巨JSON依赖。每例含实际PID、fixture/build-input/source SHA、root/parent/scope/ordinal、注入前/实际/撤销读集、原返回、root receipt/authority、事件lineage、真实backup/restore与原成功前缀。

新增raw hot读集含planA/B、frozen/patchBefore/Scratch/After四池的13个原buffer、预算/票据adapter原buffer及元数据，events/patchActions的40个buffer按有效event/lineage/guard前缀记录原位型；保留完整backing长度/容量/前缀数，不冒未使用尾区已采。字段顺序冻结到同源SHA的 `coldBuffers()`，字典列eventBufferOrder。全局scope4096溢出仍INCOMPLETE；独立BEFORE witness512行各例dropped=0。

真实source rollback的物理/池/事件lineage回到原backup；step/preview最终material/pool/source/commit与root前逐值/位型一致。新子步的budget初始化/尝试标记在原档中保留，不把它们误当源库存回滚失败，也不声称整个预算blob总是和子步初始化前相同。cut17负water输入在原中间normalizer里可被重整，保留这个中间差异；最终撤销输入后的完整物理authority与root前一致，不把每个中间restore都伪称全字节相等。cut20则按真实提交边界核保留，不要求rollback。

正常scope回归曾发现冷observer在LEAVE写出前重入导致父子行顺序错位；本生成层已改为原emit finally里先封本行再冷读。T278同类问题另以9cbcc4a补正，不改生产trace/物理。

## 验证与包

- `before_all.cjs` **300/0**：40个基本BEFORE＋A/B额外2例，两freeze臂，6个同输入撤销控制；包含worker默认参数自动路由、原拒绝/原位型/恢复/成功前缀/观察提交保留断言。
- 原三backend两freeze、N192/512正常差分 **12/0**。GPU_PAIR这一非机测试使用明确CPU接口double，不冒真实GPU成绩；未重跑64root设备矩阵。
- scope开关物理位精确与父子顺序 **12/0**；容量4096如实INCOMPLETE。
- hvigor PackageHap **BUILD SUCCESSFUL**；签名及验签PASS，检查签包ABC实际包含BEFORE、INJECTED_READSET及raw hot读集接点。

生成manifest/source SHA与T159/原生CMake身份一起更新，不把旧parent source SHA充本次源码身份。签包 `.codex/local-test/t257-sign/T250-before-v30/T250-before-v30-signed.hap`，SHA256 `3b87eee5e12c2429fdc930d4b5af58338cf5fb96d2d36c537b19f72975a85cfd`。签名记录 `test/reports/t257/T250-before-v30.json`；正式交接 `test/reports/t250/before-v30/handoff.json`。测试/归档命令在test/t250/before_all.cjs、archive_before.py。

未用真机；没有设备吞吐、外部资源峰值或完整T205产品PASS结论。历史resourceEvidence原有可见面不冒新增冷档的总内存峰值。全部背景前景仍仅XComponent+shader，保留FAULT/完整旧token STALE，未改公共渲染或默认路由。本卡收窄20 BEFORE范围已完成。
