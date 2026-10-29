# T278 AFTER 字典与原消费者开发交付

2026-10-29，developer，不使用真机。按已验收 T275 工程合同独立扩展 T250 隔离生成器，生产没有新增 fault hook，没有修改物理/dt/阈值/λ或事务算法。T250 的20 BEFORE范围独立，不合并本卡分母。

## 实现与证据

配方 `node test/t278/generate.cjs .codex/local-test/t278-v8`。AFTER v2 authority 为 `test/reports/t278/cut-inventory.json`（T250_CUT_SITES_V2），原v1原字节副本另存 cut-inventory-v1.json；不覆盖 T250 BEFORE inventory。20行均有 consumerFile/function/predicate、精确行号与源SHA、fixtureInput、原return、firstCodeOwner、rollbackBoundary/reason。

- NEXT 7个描述：cut1合法请求的原 anyWet→NO_WATER receipt；cut2合法有限1e9源请求的原λ/容量/η→CAPACITY_BLOCKED receipt；两者原txCode均0。cut4只在原collect调用前暂改active，原collect谓词315返回4（335传播）。cut6在原请求校验成功后暂改列号，原sink目标列检查返回2。cut7/8在成功prepare后暂改待消费事件计数，原capacity分支→failHot(9)。cut10在原publish和SINK_ROW审计成功后使用已有failAfterPublish→原failHot(13)。所有临时输入在finally撤销，所有码均来自原业务分支，无伪造throw或改成功返回值。
- cut7/8是同一个事件容量消费者，唯一计数 EVENT_CAPACITY_9；故**每臂6个唯一故障消费者**，不重复计前驱或T250 BEFORE。7个描述保留可关联上下文。
- OBS 11个：3/5/9/11/12/13/14/15/16/17/18。原返回后的冷快照连读两次，断言原字节相同。cut12在真实正ticket close成功后观测，不拿原close前trace.cut(12)当AFTER。cut15在finish wrapper336返回后观测，不拿内部FINISH audit当完整finish。cut9另测原index found/noPair/invalidInput，false+invalid=false明确为无配对，不造错误码。
- cut11/14已从v1空site补为原SINK_ROW/APPLY_TAIL调用，绑定原 `source==='APPLY_TAIL'?14:source==='FINISH'?15:11` 动态scope，不新增checkpoint；成功后归OBS。源提交后的19与终观察20明确NA，均不计故障PASS分母。

每个NEXT在 COMMON_FREEZE_ON/OFF 两臂保留 root/parent/scope/cut/ordinal、真实PID、生成源manifest与build-input SHA、实际offered/admitted/receipt。真实原restoreBackup后 authority、pool/budget原位型及events/lineage冷档和原backup **全字符串逐字节一致**。root2失败候选之后仍保留root1已准入19.2，并按原λ重试成功准入3.2；并未撤销成功前缀或把最终部分准入说成整个source拒绝。源码参数保持不变；OBS冷夹具额外合法输入SHAKE=1/TILT=20，单量子观测真实ticket close，单独标注T278_INPUT_SHAKE1_TILT20。

撤销注入的控制原source全准入、step成功；观察开关on/off的pool/source/local budget原字节一致。原fixture遇拒绝/部分准入后抛的TX_GPU_ADMISSION_FAILED是冷夹具“要求全准入”的终止提示，**不是新物理业务错误**；业务结论以归档原receipt/原consumer code为准。

## 验证、包与限制

`node test/t278/selftest.cjs .codex/local-test/t278-v8` **99/0**。`python test/t278/archive.py .codex/local-test/t278-v8` 产16份逐臂/逐切点冷档与SHA索引（summary.json）；OBS/NA/重复消费者不混入故障分母。独立AFTER证据环256行，逐例dropped=0。原4096 scope归档可能溢出并如实INCOMPLETE，不宣整流完整；关键AFTER原return/backup/restore/前缀档由独立证据环保留。Node使用实际隔离ETS源码转译，不声称OHOS ABI/设备通过。

T250既有source BEFORE **14/0**、field audit BEFORE **16/0**，见before-compatibility.json。hvigor PackageHap **BUILD SUCCESSFUL**（package-v8.log.gz）；签名、验签与签包AFTER接点存在性检查通过。

签包 `.codex/local-test/t257-sign/T278-v8/T278-v8-signed.hap`，SHA256 `205274a730768d68c0b29312791c2998fe37d9d87d60e514655f6872a123381e`。记录 test/reports/t257/T278-v8.json、test/reports/t278/handoff.json。所有背景前景仍XComponent+shader，原FAULT/显式STALE语义保持。未使用真机、未执行64root设备矩阵或外部峰值测试，未宣相关产品指标通过。
