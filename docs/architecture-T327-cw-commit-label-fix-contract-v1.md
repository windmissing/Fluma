# T327 · 明水 commit 标签工程修订（writer 映射与版本兼容）v1

日期：2026-10-30。责任：architect。任务卡：滴答 `6ae45280e4b0e9cb68e5599f`。
状态：**WRITER_SWAP_LOCALIZED / FIX_CONTRACT_READY / FOR_DEVELOPER_MINIMAL**。
依据：`test-report-T322-cw-ledger-erratum-v1.md`、`test/reports/t322/erratum_summary.json`、`test/reports/t310/cw-critical-1793341741239-1-frames.json`（nativeSamples 九样本原值）、`architecture-T315-clear-water-longpress-attribution-v1.md`、Engine 合同（`P011ClearWaterEngine.ets :157`）、writer 源码（`.codex/local-test/t321-final/entry/src/main/cpp/p013renderer/cw_diag.h :14`）。纯设计零代码。

## 0. 反序位置实锤（源码行级）

**Engine 合同**（`P011ClearWaterEngine.ets :157`，publication header 构造）：

```ets
h[0]=this.owner; h[1]=this.generation; h[2]=boundary.publicationSerial; h[3]=this.generation;
h[4]=this.config.configRev;
h[5]=c.physicsCommit;    // ← header[5] = physicsCommit
h[6]=c.sourceCommit;     // ← header[6] = sourceCommit
```

**Native writer**（`p013renderer/cw_diag.h :14`，sample 填充）：

```cpp
r.ledgerWater=p.ledger[11];
r.sourceCommit=p.header[5];   // ← 错：header[5] 是 physicsCommit
r.physicsCommit=p.header[6];  // ← 错：header[6] 是 sourceCommit
```

两行赋值互换。**导出端**（cw_diag.h :19 `exportCold`）以 `\"sourceCommit\":"<<r.sourceCommit<<",\"physicsCommit\":"<<r.physicsCommit` 序列化——结果 T310 九样本（3 帧×3 层）全部 `sourceCommit:586, physicsCommit:2`，而 header 语义是 `header[5]=586=physicsCommit`、`header[6]=2=sourceCommit`。**值正确、标签反**——与 T322 勘误判定一致。

同树文件核对（`t309-final`/`t321-final`/`t270-v3`/`t270-v4` 四树 cw_diag.h :14 逐位相同）：**该缺陷存在于所有在役 native 树**，不是单包回归。

## 1. 影响面（哪些校验/旧档受影响）

| 面 | 影响 | 处置 |
|---|---|---|
| T310 归档九样本 | `sourceCommit/physicsCommit` 键下值互换 | **不改档**（原档不可篡改红线）；报告层勘误已在 T322 完成，消费方读档时按「值对键反」解释 |
| header/critical/token 链 | **不受影响**——header[5]/[6] 本身正确；token 字段 [4..11] 与 header 同序正确；critical TERMINAL `sourceCommit=2` 来自 ledger 侧（l[2]=sourceCommit，:159）与 header 无关 | 无 |
| `app_bridge.cpp :161/:191/:204` | **不受影响**——bridge 从 `gpu->Owner().Header()` 按 header256::sourceCommitLo(:14)/physicsCommitLo(:16) 读，枚举正确 | 无 |
| 旧档兼容 | T310 之前的 CW_C_DIAG_V1 归档（T270-T309 各包）同样键反 | **统一按「值对键反」解释**，见 §2 版本兼容 |
| 关联校验 | `cw-critical` 帧的 header[5]/[6] 判等逻辑（如 token 比对、physicsCommit===b[3] 判定 :157 h[7]）都在 ArkTS 侧用正确索引，无 native 样本参与 | 无 |

**结论：影响面收敛于「native 样本导出的两个 JSON 键」**，物理链/账本/token/帧校验零接触。

## 2. 修复合同（developer 最小改动）

**唯一改动点**：`entry/src/main/cpp/p013renderer/cw_diag.h :14` 两行互换：

```cpp
// 修复后（与 Engine 合同 :157 对齐）：
r.sourceCommit=p.header[6];
r.physicsCommit=p.header[5];
```

**版本兼容三规则**：
1. **revision 升版**：`CW_C_DIAG_V1` → `CW_C_DIAG_V2`（cw_diag.h :19 exportCold 的 revision 字段）——新档立即可辨；
2. **旧档显式适配，不篡改**：消费方（checker/tester 报告）读档规则——`revision==CW_C_DIAG_V1` 时按「sourceCommit 键值实为 physicsCommit、physicsCommit 键值实为 sourceCommit」解释；`revision==CW_C_DIAG_V2` 直读。**把该解释规则写进 V2 导出的同一行注释**（cw_diag.h 内），防未来读者丢失上下文；
3. **回归判据**（developer 自测 + tester 复核共用）：
   - 同一 run 的 native 样本 `sourceCommit` 与 critical TERMINAL 行 `sourceCommit`（ledger 侧）**相等**（修复前为 physics 值不等）；
   - 同一 run 的 native 样本 `physicsCommit` 与 header[5]/bridge `physicsCommitLo` 相等；
   - token 字段序 [4..11] 与样本两键**不再镜像互换**（token 中 `physicsCommit` 在 [7]，样本应同值）；
   - V1 旧档样本用适配规则读出的两值与 header 一致（从 T310 档抽样 3 帧任一层验证）。

**范围红线**：
- **不改** header 合同（:157 ArkTS 侧正确）、token 布局、ledger、Engine、shader、光学；
- **不动** T321 在途 GAP 反馈开发（卡面「T321 在途独立，不回退/抢 hunk」——cw_diag.h 与 T321 的 ClearWaterSourceController.ets/停源反馈文件不相交，无 hunk 冲突面；若 T321 分支恰好也改 cw_diag.h（可能性低），developer 按共享树规则 rebase 保留双方）；
- **不冒 GPU M1**——cw_diag 是诊断采样器（cold export 专用），非渲染链组件；
- **FBO 水面窄证保留**（T322 §三）：body/swap/有效长按观感维持未证，本修复不改变能证边界。

## 3. 接口交接（与 T321 的边界）

cw_diag.h 的 sample/exportCold 只被冷诊断导出（1Hz 冷 timer / explicit cold export）调用；T321 交付的「停源反馈/有界原时钟收据」在 ArkTS 侧（SourceController/FrameClock/receipt）。**无共享可变状态**。唯一交接点：T321 若在 ClearWaterDemo 页面新增收据展示，可能**读取** nativeSamples——届时按 §2 规则 2 的版本适配读。此句写入两卡回执，developer/tester 各自留意。

## 4. 路由

- **developer 最小修复卡**：§2 两行互换 + revision V2 + 注释 + 回归判据（预计单文件单 hunk）；
- **tester 复核**：修复包复采一次 CW run（沿 T310 采集流程，不需要重采 ROI/光学），核 §2 规则 3 四判据；
- 旧档解释规则（§2 规则 2）同步登记到 T322 勘误附录（tester 报告勘误时引用本卡 §2）。

## 5. 变更记录

- 2026-10-30 v1：初版。反序实锤于 cw_diag.h :14（两行互换，四在役树同缺）；影响面收敛于 native 样本两 JSON 键；修复合同（两行互换 + CW_C_DIAG_V2 + 旧档值对键反适配规则 + 四回归判据）；与 T321 无 hunk 冲突面、只读交接点登记；不冒 GPU M1、FBO 窄证保留。
