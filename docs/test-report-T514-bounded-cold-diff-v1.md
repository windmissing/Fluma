# T514 · GPU 有界冷档协议生产逻辑独立差分 tester 报告 v1

- 卡：`6af8605de4b0711ec478f88a`（tester，P5，不使用真机）
- 日期：2026-11-16
- 被测：T507（db22352 有界冷协议/两 slot/UTF8 writer/collector 接线）实际交付档
- 方法：从原件/源码独立复算（判据先立，不信任 developer 断言）

## 独立差分 10/10 PASS

| ID | 判据 | 独立复算 |
|---|---|---|
| E1 | writer 恰好 limit | boundary-complete `state=COMPLETE`，limitUtf8Bytes=1,847,406 |
| E2 | limit−1 语义 | boundary-minus-one `INCOMPLETE/DIAGNOSTIC_BUDGET`，limit 差**恰好 1 字节** |
| E3 | 首因保留 | 4 groups 的 firstCause/faultRoot 在 COMPLETE 与 INCOMPLETE 间 **逐字段相同**（4/4 对照，含 scope 原始字节） |
| E4 | 分配失败 fallback | fallback-worst `INCOMPLETE/ALLOCATION_FAILED`、13,218B ≤ 16KiB 小 envelope（不冒完整） |
| E5 | 容量常量源码==T503 manifest | traceCapacityBytes=55,410,688、2 slot、1 builder、1 reader、diagnostic 2MiB、page 16KiB——napi.inc/page_units.cjs 常量与 manifest.maxCumulativeCapacityBytes 逐字对照 |
| E6 | native helper 28/0 | units log 尾 JSON 行 `passed:28, failed:0, captureBytes:1,404,072, fallbackBytes:13,218` 独立读 |
| E7 | collector 17/0 | 全部 finally 释放路径（bad/io/cancel/unknown/manifest/busy/budget/stale/oversize/throw + 两页实际迁移）PASS |
| E8 | contract 机检 | 基线增强 delivery hash、native 常量==T503 manifest、旧在线升级拒绝——1/1 |
| E9 | 版本兼容 | 旧在线 requestM2Trace/readM2Trace → UNSUPPORTED_COLD_REVISION（collect.py 断言在源）；旧离线 V3 档案/reader 保留 |
| E10 | 签包 | M1 `a1baa577…` / M2 `b702032f…` verifyApp PASS（signing 档） |

## 判定要点

- **writer 边界语义**：恰好 limit 完整 COMPLETE、少 1 字节即 INCOMPLETE/DIAGNOSTIC_BUDGET 且
  首因/faultRoot 四组逐字段保留——"不将未读完整记 PASS"在此落实（INCOMPLETE 显式标注非 COMPLETE）。
- **容量分账**：trace 55,410,688 累计与 timeline 4MiB/diagnostic 2MiB 独立预算（manifest
  wholeArchiveJsonLimitFormula 独立于 timeline 预算、stream pages）——源码常量与机器 manifest 一致。
- **未读完整**：本轮差分覆盖的是 T507 交付的离线原件与源码合同；**设备端冷档/真实 sensor 域
  NOT_MEASURED**（与设备独立，卡面明确）。旧档兼容仅核拒绝语义，未逐字节重放旧 V3 档案（复用 T476 26/26 域）。
