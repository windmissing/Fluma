# T334 cw_commit 标签修复真机复核（K1-K4）（真机，完成）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T333 实施 commit `0147095`（生成树 `.codex/local-test/t333-fix`）。合同依据：`docs/architecture-T327-cw-commit-label-fix-contract-v1.md`。

## 0. 实际包身份（T328 移交裁定 1 口径，三件套）

- T333-fix 生成树 unsigned HAP → Python 重打包（绕 sign-tool 11017001）→ hap-sign-tool localSign + verify-app **PASS**。
- **T334c-signed.hap SHA256 `c5338fa3c62e3029606cb45fc987475c9ea5bb4a8a84434852a4b4a278ca396d`**，Want `--ps cwdiagSha` 外部输入核对一致，导出 manifest `packageSha` 回读同值。
- **取证工装参数变更（如实登记）**：T333-fix 生成树 `ClearWaterDemo.ets:21` 诊断环容量硬编码 256，环满即丢（`dropped` 递增、不覆盖），TICK 高频 ~2s 填满 → 注水事件全部被丢弃（run1-run5 四轮 INCOMPLETE/无 DOWN 实证）。tester 将容量改为 **4096（ClearWaterDiagnostic 合同上限）**后重建重签——此改动只影响取证容量、不触碰 cw_diag.h 修复本身（native V2 两行互换零接触）、不改 header/token/ledger 合同；T310 时代 V1 容量即 16384。初版误设 16384 触发 `CW_DIAG_CAPACITY` 崩溃（>4096 上限，fail-closed 正确，jscrash 在案）后改 4096。

## 1. K1-K4 逐条判定（run9：count=918、dropped=0、frames=500-502 捕获窗、5 笔 ENQUEUE/TERMINAL 全 ACCEPTED）

| 判据 | 结果 | 证据（`diag_run9.json`） |
|---|---|---|
| **K1 同 run native sourceCommit == critical TERMINAL sourceCommit（ledger 侧）** | **PASS** | 捕获帧 500-502 native sample `sourceCommit=4`（9/9 样本）；ledger 侧 TERMINAL 序列 srcCommit 1→5，其中 **TERMINAL#4（row497）srcCommit=4 且 globalWaterAfter=0.066773754 == 捕获帧 ledger11/nativeWater**——同 run 三层（native sample / engine ledger / critical TERMINAL）sourceCommit 同值 4 闭环 |
| **K2 native physicsCommit == header[5] == bridge physicsCommitLo** | **PASS** | native `physicsCommit=266`（9/9）；frameData `h5=266`（3 帧）。V1 反序将写 src=266/phy=4——实测 src=4/phy=266，映射已修复 |
| **K3 token 字段序 [4..11] 与样本两键不再镜像互换** | **PASS** | frameToken[4]=**266**(physics)/[5]=**4**(source)，与 header[5]=266(phy)/[6]=4(src) 语义对位；T322 勘误的 V1 镜像（source 键存物理值 586）在 V2 真机样本中不存在 |
| **K4 T310 V1 旧档适配规则读出与 header 一致** | **PASS** | 适配规则（V1 sourceCommit 键值实为 physicsCommit）由双侧互证：T310 V1 档（src 键=586=phy，phy 键=2=src）+ 本轮 V2 直读（src=4=源序号、phy=266=物理计数）语义一致；非机 `v1_archive_check` 3 帧已过（T333 交付），真机侧 V2 直读与 V1 适配读出的语义映射逐位对齐 |

**附带确认**：
- `revision=CW_C_DIAG_V2` 在导出 manifest 与 nativeSamples 内层均生效（0 个 DISABLED、0 个 V1 残留）。
- 同 run 源账闭环（T310 同款）：DOWN→ENQUEUE×5→TERMINAL×5 全 ACCEPTED，W 累积 0.0111→0.0334→0.0445→0.0668→0.0779；nativeWater==ledgerWater==TERMINAL#4 W（0.066773754）。
- 5 笔 TERMINAL 的 `phyCommit`(159/161/215/217/269) 与 `srcCommit`(1-5) 分离清晰——两键语义独立正确。

## 2. 结论

- **K1/K2/K3/K4 全 PASS**：T333 两行互换修复真机生效，cw_commit 标签反序缺陷（T322 勘误）在 V2 真机导出中闭环。
- V2 revision 可辨、V1 适配规则可用、旧档零改动红线遵守。
- 取证容量 256→4096 工装变更如实登记（不影响被测修复语义）；建议后续生成树将默认容量回调或参数化，交 manager/developer 酌情。
- FBO 水面窄证保留（本卡未触碰 body/swap/观感）。

证据：`test/reports/t334/`（diag_run9.json + hilog_run*.txt + diag_run2/3/4/5/8.json 过程留证 + layout*.json）；签包 `.codex/local-test/t318-sign/T334c-signed.hap`。
