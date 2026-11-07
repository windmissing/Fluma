# P-011 双端退款接口差异裁定（manager）

日期：2026-06-24 修订：2026-10-24。裁定人：manager（项目 `6ac8a7e3e9adb60000000419`）。

依据：developer 在 D1 B 段（卡 `6adc52e4…2a85`）按纪律停手反馈的接口差异，manager 已逐行核对源码确认属实。

---

## 1. 事实认定（manager 已复核源码）

| # | 事实 | 出处 | 性质 |
|---|---|---|---|
| F1 | `clipSubstepMotion(source, target, requested)` 对两端 local **各扣 `allowed`**，`total` 扣 `allowed` | `WaterSandField.ets:3548-3566`（:3560 source、:3562 target、:3564 total via `clipTotalMotion`） | 既有实现 |
| F2 | `materialGuardRefund(rejected, localColumns, bottomChannel)` 调用 `P011GuardLedgerRefund.refundLocal(localColumns, rejected)`，并单独 `totalMotionBudgetRemaining += rejected` | `WaterSandField.ets:4092-4106` | 既有实现 |
| F3 | `refundLocal` 先算 `share = amount / columns.length`，再对每列退 `min(share, localUsed[c])`；unrefundable 部分**不再分配**（保守丢弃） | `P011MaterialGuard.ets:214-236`（:221 `share`、:225 `min(share, localUsed[c])`） | 既有实现 |
| F4 | 由此：跨列 partial reserve 被拒时，若按 rev1 语义整笔退回，调用 F2/F3 只会让两端各退 `rejected/2` → **两端 local 各留一半净扣**，total 却已全退 | 由 F1+F2+F3 推出 | **接口语义不兼容** |
| F5 | 架构稿 rev1 §13.2 要求「guard 部分退款按各端实际扣额退、total 只退一次」 | `architecture-P011-dual-field-particle-v1.md` §13.2 | 现行契约 |

**定性**：这是**旧退款接口与 rev1 契约不兼容**，不是物理失恒、不是阈值问题、不是 D1 实现错误。D1 桩按 rev1 实现（F5）是对的，错的是「直接复用旧 `materialGuardRefund` 就能满足 rev1」这个假设。

## 2. 裁定

**rev1 契约不变，F5 继续有效**。退款接法按下列规则落地：

### 2.1 粒子票据退款（新增 adapter，D2 接法）

- **local**：按各端**实际已扣额**逐端退还（票据记录每端 reserved，`refundLocal` 的平分语义不适用于粒子票据）。
- **total**：整笔只退一次（与 F1 的 total 扣款口径对称：total 扣一次就退一次）。
- **bottom**：粒子通道 bottom 恒 0，不退不扣。
- **不得**直接以双端列表原样调用旧 `materialGuardRefund`；**不得**对 total 双退。
- 每笔退款须可核：`requested` / 每端 `reserved` / 每端 `refunded` / `total refunded` / `netLocalDelta` 逐值相等（`netLocalDelta` 须为 0）。

### 2.2 旧通道行为保持不变

- 旧 `materialGuardRefund` 与 `P011GuardLedgerRefund.refundLocal` **不得修改**（其平分+保守丢弃语义被既有 P-011 通道的 planner 影子镜像依赖）。
- 粒子票据 adapter 为**新增**窄接口，与旧退款路径并存：粒子走 adapter，旧通道走原函数。
- 唯一允许的旧函数改动是**新增一个重载/入口**接受「各端实际扣额」数组；若实现上必须改 `refundLocal` 内部，则旧调用方需保持逐值不变并有回归守卫。

### 2.3 D1 与 D2 的边界

| 项 | 归属 | 说明 |
|---|---|---|
| 粒子票据 adapter 的**契约**（接口签名、退款语义、逐值断言） | **architect**（本裁定即契约，rev1 §13.2 补一条 R9 澄清） | D2 按此实现 |
| 粒子票据 adapter 的**实现** | **developer**（D2 真实接法） | D1 保持桩形态，不越界写 WaterSandField |
| D1 单测「桩与真实 clip 逐值相等」 | **保留该断言**，但其对照基准改为「rev1 语义桩 vs 粒子票据 adapter 桩」 | 不得删除或放宽；对照旧 `materialGuardRefund` 的用例改为**明确标注为已知不兼容**的负对照（记录差异数值），不算失败 |

## 3. 对 D1 B 段完成条件的影响

原完成条件「§13.6 各组验证全绿」现调整为：

1. **可豁免项（唯一一项）**：「桩与真实 clip 逐值相等」中**涉及旧 `materialGuardRefund` 平分语义的那一条对照** —— 因 F1+F3 决定该对照在数学上不可能成立（除非改旧函数，而旧函数不可改），改为按 §2.3 记录差异数值作为负对照证据。
2. **不可豁免项**：其余全部 §13.6 验证组必须全绿；粒子票据 adapter 桩的逐值断言（§2.1 末条）必须绿。
3. **不可豁免项**：生产热路径零分配收口（原稿 §4.4）—— 需继续收口复用缓冲，或由 manager 另行裁定降级标准。

D1 B 段**任务不关闭**，剩余范围：

| # | 剩余项 | 角色 |
|---|---|---|
| R1 | 按 §2.1 实现粒子票据 adapter 桩 + §2.3 的逐值断言 | developer |
| R2 | 把 §2.1 契约写进架构稿 rev1 §13.2（新增 R9 条目） | architect |
| R3 | 生产热路径零分配收口（或提出降级方案交 manager 裁定） | developer |
| R4 | 旧 `materialGuardRefund` 不变的回归守卫 | developer |

## 4. 不得触碰的项

- 冻结阈值（300/s、128 列、守恒/库存/容量/有限值、26/20、tan30、30°±2°/t90/R²/5%）**一律不动**。
- 旧 `P011GuardLedgerRefund.refundLocal` 的平分+保守丢弃语义**不得**为了让粒子路径通过而改变。
- 预注册 `DUALFIELD-*` 16 例 estimand **原样**。
- 本裁定不改变 rev1 的落床余量口径（deposit 只扣 granted、余量留原粒 state=2、槽释放=质量归零）。

## 5. 后续

- architect：按 §2.1/§3-R2 补 rev1 §13.2 的 R9 条目（退款 adapter 契约）→ 解锁 developer 的 adapter 桩与 D2 接线。
- developer：按 §3-R1/R3/R4 完成 D1 B 段剩余三项 → 复测交 manager 验收 → 解锁 D2。
- manager：R3 若提出降级方案，由 manager 裁定是否接受（不接受则继续收口）。
