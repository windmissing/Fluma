# T333 · cw_commit 标签反序最小修复交付 v1

日期：2026-10-31。责任：developer。任务卡：滴答 `6ae5462fe4b031ea82f1f0f4`。
合同依据：`docs/architecture-T327-cw-commit-label-fix-contract-v1.md`（WRITER_SWAP_LOCALIZED / FIX_CONTRACT_READY）。

## 1. 实施内容（四件齐）

### 1.1 两行互换（§2 唯一改动点）

`test/t270/native_diag.h`（生成器源头；生成树的 `entry/src/main/cpp/p013renderer/cw_diag.h` 由 `generate.cjs :37` 复制自它）：

```cpp
// 修复前（V1，标签反序）：
r.sourceCommit=p.header[5];   // 错：header[5] 是 physicsCommit
r.physicsCommit=p.header[6];  // 错：header[6] 是 sourceCommit
// 修复后（V2，与 Engine 合同 P011ClearWaterEngine.ets :157 对齐）：
r.sourceCommit=p.header[6];
r.physicsCommit=p.header[5];
```

主工程 `entry/src/main/cpp/p013renderer/` 下不存在 cw_diag.h（该文件只存在于生成树，由 t270 生成器产出）——**源头即 test/t270/native_diag.h**，改它即改四在役树的共同祖先。已归档生成树（t270-v1..v4/t309-final/t321-final 等）一律不改（不可篡改红线）。

### 1.2 revision 升版 CW_C_DIAG_V1→V2

- `native_diag.h` exportCold 的 revision 字段：`CW_C_DIAG_V2`（合同原文指定）；
- `test/t270/ClearWaterDiagnostic.ets :8` ArkTS 冷导出包装层 revision 同步 V2——理由：nativeSamples JSON 嵌在该外层导出内，外层保持 V1 而内嵌 V2 样本会造成"同档异版"歧义，违背合同"新档立即可辨"意图；此为合同语义对位扩展（合同原文只点名 native 文件），在回执中注明；
- `test/t270/generate.cjs :46` t270-manifest.json revision 同步 V2；
- `test/t270/selftest.cjs :20` 断言同步 V2；
- `test/t270/package_check.py :7` 原生库内容断言 V1→V2。

### 1.3 旧档显式适配注释（防上下文丢失）

V2 导出同文件（native_diag.h）注释原文：

> V1 archives: "sourceCommit" key actually holds physicsCommit and vice versa. V2 writes the correct mapping. V1 archive readers MUST interpret keys as value-for-key-swapped; V1 archives are never rewritten (immutability).

旧档（T310 及之前全部 CW_C_DIAG_V1 归档）一律不改。

### 1.4 非机回归四判据

| 判据 | 验证载体 | 结果 |
|---|---|---|
| ①同 run native sourceCommit 与 critical TERMINAL sourceCommit（ledger 侧 l[16]）相等 | `test/t333/native_fix_test.cpp`（修复后 writer + Engine 合同值 586/2 + ledger l[16]=2） | **PASS** |
| ②native physicsCommit 与 header[5] 相等（=bridge physicsCommitLo 同槽） | 同上 | **PASS** |
| ③token 字段序 [4..11] 与样本两键不再镜像互换 | 同上（token[4]=h[5]=586=sample.physicsCommit、token[5]=h[6]=2=sample.sourceCommit；全带 [4..11] 按合同序） | **PASS** |
| ④V1 旧档适配规则读出与 header 一致（T310 档 3 帧任一层） | `test/t333/v1_archive_check.py`（900/901/902 三帧 layer 2；适配值对 header[5]/[6] 与 ledger l[16] 及 token[4]/[5] 四重一致） | **PASS** |

配套回归：t270 既有 native_test 9/9（无破坏）；t270 selftest 26/26（生成树全链，含 On/Off 权威对照）；生成项目 hvigor 构建 BuildNativeWithNinja 12s+CompileArkTS 24s+PackageHap 全绿；HAP 内 libp013renderer.so 含 `CW_C_DIAG_V2` 且不再含 `CW_C_DIAG_V1`。

## 2. 范围红线遵守

- 不改 header 合同（Engine :157 ArkTS 侧本就正确）、token 布局、ledger、Engine、shader、光学——零接触；
- 未冒 GPU M1（cw_diag 是诊断采样器）；
- FBO 水面窄证保留（body/swap/有效长按观感维持未证）；
- 四在役树归档未动（只改当前工作树源头）。

## 3. 与 T321 交接点（合同 §3）

cw_diag.h 的 sample/exportCold 只被冷诊断导出调用，与 T321 的 SourceController/FrameClock/receipt 无共享可变状态。**交接规则**：T321 若在 ClearWaterDemo 页面读取 nativeSamples 展示收据，必须按版本适配读——`revision==CW_C_DIAG_V1` 时"sourceCommit"键值实为 physicsCommit、反之；`V2` 直读。此规则已写入 V2 导出文件注释。

## 4. 包签名状态（如实披露）

本卡非机验证用 unsigned HAP（生成树 t333-fix）。**无手签包**——SignHap 00303116（已知密码长度校验问题）如实保留。T334 真机复核的包由执行 tester 按 T151 流程补签（T328 移交裁定 1 口径），交付三件套=手签包+sign/verify 原始输出+SHA256 归档。

## 5. 证据清单（test/reports/t333/）

- `native-fix-test.txt`：判据①②③ 10 断言 PASS（T333_CW_COMMIT_FIX_NATIVE_V1）
- `v1-archive-check.txt`：判据④ 3 帧全过（T333_V1_ARCHIVE_ADAPTATION_V1）
- `selftest-fixed-tree.txt`：t270 selftest 26/26（生成树含修复）
- `native-build.txt` / `hap-build.txt`：编译与打包日志（SignHap 00303116 已知失败保留）
- 生成树：`.codex/local-test/t333-fix/`（含修复的完整项目，可作 T334 打包基）

## 6. 变更记录

- 2026-10-31 v1：初版。两行互换+V2（native/ArkTS 包装/manifest/selftest/package_check 五处同步）+旧档适配注释+四判据非机回归全绿（10+3+26+9 断言）；归档不动；T334 真机复核就绪。
