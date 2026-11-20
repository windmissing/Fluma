# T713 v2 · RX 1097 修订驱动复测——1097 已消，新错 readonly@1987

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T713-rx-signed（T709 修订包，inputSha 828bc4e8→signed 3c271cac，
sign=PASS；libgrs_g9rx.so 1406480B/SHA ad5a371b/BuildId af1bf65b）。
证据：`test/reports/t491/t713b/`（l1 布局、h713.txt driver 原文、
v2-rx1097.json）。

## 执行链

grsm1 直达 RX 研究页 → 启用研究：
- **LOADED_FILE_OBSERVED：memoryBuildIdMatched=true**（T709 修订 so 正确加载）
- **CONFIGURE_FAILED_8**：driver `0:1987: S0001: Function call discards
  'readonly' access qualifier`

## 判定

- **1097 行 coherent 拒因已消失**（T709 修复生效）
- 修复暴露**下一层限定符丢弃**：新行 1987，`readonly`（与 coherent 同
  S0001 族，非新缺陷类）
- 修复链进度：S0022（slot）FIXED → S0001 coherent@1097 FIXED →
  **S0001 readonly@1987 OPEN**

## 回执

回 architect/developer：1987 行 readonly 与 T691/T709 同型变换（调用前
局部拷贝）可解；每轮修复推进一层。Create/READY/首 Swap 仍被 configure
门前阻塞，如实 NOT_REACHED。
