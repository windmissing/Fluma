# T367 developer 交付

`test/t367/generate.cjs` 在 T359 隔离 V2 工程接入 `T367_STOP_COHORT_V1`。按最近实际接受的 run、state epoch 和 admissionTransactionId 锁定最多三个新生离床 ID。导出保留独立停源、首个成功子步、末端及退役收据，加上 256 行最近切点。UP/CANCEL 等原因使用停源回调的 Date 毫秒时间。停源后物理时长来自成功子步累计。请求预测和 apply 按 PREVIEW/AUTHORITATIVE 区分，提交锚点带实际请求结果、床量差和最终角。无运动行和 ID 不在池中分别编码。

离线检查 9/9，含最后实际准入、拒绝交易、ON/OFF checkpoint 位型、停源锚点跨环覆盖、同 commit LAND、slot、reset 及四个额外停源原因。实测队列 ID 5/6，停源 commit 5、首成功 commit 6、LAND commit 43、末端 commit 53。独立锚点在环覆盖 604 行后保留。

Hvigor 隔离构建成功。HAP 签名和校验通过，SHA256 `e20d4dddfdc0215a127e4937dccd0b3743fd099a27342a4cc59c591435a66dad`。包和签名回执分别位于 `.codex/local-test/t257-sign/T367-delivery/T367-delivery-signed.hap`、`test/reports/t257/T367-delivery.json`。原停源冻结归因由 T369 同包真机续证。
