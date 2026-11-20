# T677 T700 回执独立复核

2026-11-20，在全新带空格目录 `T677 independent 4dc48593` 用仓库正式入口构建实际 broker 和 consumer。首次编译及链接、重配、无工作、只触碰 RSP 后双 ELF 重链、重链后再无工作，均有本次调用的 argv、返回码、原始 stdout/stderr 和前后闭包。调用 ID `e8101f2baed14ef18a88707b8f261b24`。

ELF 保持八个 ABI 导出，consumer 的 DT_NEEDED 包含 `libgrs_cold_slots.so`。静态模式只出 ARTIFACT_VERIFIED，observedSteps 为空。十项负控均被拒绝，包括缺清单、旧归档路径、错 ID、错根、错目标、缺步、串流互换、已有 ELF 的失败调用记录、截断和改字节。原 v1 的目标、缓存和 FP 拒绝原件继续作为对应历史证据。

正式回执和本次原始日志见 [receipt](../test/reports/t677/t700-independent/receipt/compile-receipt.json)，负控复现见 [negative.json](../test/reports/t677/t700-independent/negative.json)。本卡范围为离线构建与回执校验。
