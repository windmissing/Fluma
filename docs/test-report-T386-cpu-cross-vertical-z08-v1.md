# T386 CPU 零获准纵向推进独立真机验证（Z08）— 判据达成（同 token 项部分，如实）

- 卡面：按 T378 Z08 与 test/reports/t381/developer.md，用 T367 cohort 装置核同 run UP→零获准→源列下降/触床→原列 T3，记录全额或受限余量、预算退款、守恒/回滚及同 token，保原始文件和身份。
- 包：T381-handoff-signed.hap SHA256 `2189bf62e7ed33bd08d5521fae56cc53b95fb20e46437176f8642dc0ae1c4c3f`（卡面给签包，已装）。
- 路由：`p051v2=1` P051DerivedV2Demo（`P011_CPU_DRY_CROSS_VERTICAL_V1` 隔离启用入口），pid 55523，run `p011-run-1793958125594-3`。证据：`test/reports/t386/`。

## 方法

真实触摸 12 轮出生（T367 同款手法）→ UP → 「导出跨列收据」两时点采档：**基线**（substep 842 / commit 860，注入后早期）与**沉降后**（substep 1444 / commit 1462，+602 成功子步）。收据为 `P011_CPU_DRY_CROSS_VERTICAL_RECEIPT_V1`：240 行尾行（finalF64Hex 240×4 F64 大端 + integers 8×U32 + values 20 字段），独立失败回执两时点各一。

## Z08 判据逐条

| 判据 | 结果 | 证据 |
|---|---|---|
| 同签包、同 run、实际角、成功物理秒 | **PASS** | 两时点同 run `p011-run-1793958125594-3`、owner=1、无 epoch 跳变；+602 子步成功物理秒（PAUSED 未发生，tick 连续） |
| UP 后首拒绝→零获准 | **PASS** | 基线 **240/240 行 grant=0**（零获准真实 T4 保留源列，`blockedYFree` 路径） |
| 源列下降/接触 | **PASS** | 基线 240/240 final y < y0（全部下降中，y 12.49–78.43）；settled **240/240 y=0.0**（触床/移交 T3）——非以 vy/帧计数推断，直接位型读数 |
| T3 全额或可解释余量 | **PASS** | **remainder==requested 240/240**（全额余量）；settled 尾行 y=0 由下一子步 T3 处理沉积（契约：尾部只改位置/速度/state） |
| 预算退款 | **PASS** | reserved 总 5.0 → refunded 总 5.0 **精确闭合**（240/240 行 refunded>0） |
| 零跨列 | **PASS** | final x ≈ 源列 240/240（基线与 settled 双时点）；无粒子进入 targetPlusOne 列 |
| 守恒 | **PASS** | mass 总和 23.08518 基线=settled（**delta 0.0**） |
| 回滚 | **PASS** | 独立失败回执两时点均 `failed=false, rolledBack=false`（零回滚事件） |
| 同 token | **PARTIAL（如实）** | 跨列收据按 T381 设计无 token 字段；同 run/same owner/零回滚身份链完整保留，token 相等需 publication 收据通道（判别入口已注明），本收据不可直接断言 |

## 结论

T381「CPU 干态跨列受阻时继续原列纵向运动」在真机 Z08 独立续证**达成**：零获准→源列纵向下降→触床移交 T3→全额余量+预算退款精确闭环→质量守恒零漂移→零回滚。沉降观感与 T369 慢速沉降机制一致（602 子步完成全程）。

## 边界

未改业务/判据；同签包同 run 未中断；收据原始文件（finalF64Hex/integers/values 原始数组）原样归档未重排；同 token 部分达成如实登记（收据设计无该字段，非取证缺陷）。
