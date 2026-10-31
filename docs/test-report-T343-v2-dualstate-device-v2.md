# T343 数值 V2 双态沙真机复测（DD12，T342 修复后）——D2/D3 FAIL（DD12 未达成，按约定 BLOCKED 先 architect）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T342 实施 commit `8de02c9`（exact70 V2 主链接入核验版）+ T338 观测 wiring（`1d95449`，仅观测）。

## 0. 包身份（T151 三件套）

- HEAD `1d95449`（= T342 `8de02c9` + T338 wiring）强制重编（CompileArkTS 8.9s + PackageHap）→ 重打包 → localSign + verify-app **PASS**。
- **T343-signed.hap SHA256 `c15d1a4cd6cbf1047aabb0f932ec0b87f906a4c76ed8977b715744466c23ae7b`**。

## 1. 执行口径澄清（重要）

- **V2 显式入口 = P051DerivedV2Demo 页**（`--ps p051v2 1`，S2 条款）。首测误在 EmptySandDemo（V1 生产默认，S1 条款明确"无 V2 facade"）观测到 present code=100——**该 code100 属 V1 链固有行为，不构成 D1 FAIL**（T342 修复对象是 V2 链）。
- 有效采集全部在 V2 页完成（两代进程 43905/48139，V2 隔离页注入 28 笔 `T218DBG inject ok=true adm>0 rej=none`，累计 adm ≈25 units）。

## 2. D1-D6 逐条判定

| 判据 | 结果 | 证据 |
|---|---|---|
| **D1 code100 消除** | **PASS** | V2 页两代进程 p013_scene 58+ 行 **全部 state=CURRENT、request==actual 逐行相等、code100=0、STALE=0**（hilog_full.txt 121 行 scene/T218DBG 证据）——renderer 严格拒绝在 V2 链零复现，T342 修复目标达成 |
| **D2 同 FrameToken 双态沙可见** | **FAIL** | live 粒子可见 ✓（截图 s1/s5 ~12-15 悬浮亮点）；**床面高度场沙不可见 ✗**——双态只有单态。且粒子位置跨 run/跨 2 分钟几乎不变（s1≈s4 逐字节近同）——粒子静止悬浮，重力未作用于粒子渲染位形 |
| **D3 床量非零 LAND 闭环** | **FAIL** | 三代截图（s1/s4/s5）均无床；注入持续成功但零沉积证据；writer hook 采集无 LAND 事件可写（T284锚点 FIRST_BIRTH 触发于帧 142 而床仍零——与 T218 BLOCKED 时同现象） |
| **D4 停止沉积与静置保持** | **NOT_APPLICABLE**（依赖 D3） | 无沉积可停；画面静止非目标语义 |
| **D5 材质稳定** | **部分** | code100=0 ✓、`T218DBG` seq/adm 逐笔记录完整（N192/512 成本通道在）；但库存守恒因床零无法闭环 |
| **D6 无 THREAD_BLOCK** | **PASS** | 两代进程长时间运行（>10min，commit 推进至 1157+）`THREAD_BLOCK_6S=0`、jscrash=0——T338 M1 基线（150ms@R148）一致 |

## 3. 核心发现（BLOCKED 移交 architect，按 T218 约定不直接修）

**T342 code100 修复有效（D1 达成）但 DD12 仍未达成**：V2 隔离页粒子出生并渲染（可见单态），但**粒子不沉降**——悬浮位置跨 run 静止、床量恒零。此现象与 T218 BLOCKED 时"床沙不可见"同位，**排除 code100 材质拒绝为其根因**（本包 renderer 零拒绝仍无沉降）。

新证据指向：V2 粒子链的**重力→粒子动力学→LAND→床写入**环节存在断链（注入的沙进 V2 链后既不沉降也不入床，admitted 量悬浮恒存）。候选归因（供 architect）：粒子步进未消费重力输入 / LAND writer 未被触发 / 床高度场 writer 写入与渲染投影断开。判别入口：`P051DerivedFacadeV2.step(dt)` 内粒子 settle 路径 + `P011DerivedProjectionV2` 投影。

## 4. 红线遵守

渲染仅 XComponent 未动；未改物理/守恒/判据；V1 生产默认未撤销（S1）；设备现象如实按代登记未宣普适。

证据：`test/reports/t343/`（hilog_full.txt 15729 行 + s1/s4/s5.jpeg + layout0-5.json + diag*.txt + scene1.txt）；签包 `.codex/local-test/t318-sign/T343-signed.hap`。
