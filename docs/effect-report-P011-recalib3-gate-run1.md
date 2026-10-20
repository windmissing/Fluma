# P-011 观测③补采 gate run 停机报告（835e663 基线）

日期：2026-10-20；effector 执行单：冻结 schedule `0/+15/0/+45/0/-45/0`，前置状态按任务书全部满足后重启采集。
设备 62T0225B10010020；runId `p011-run-1792464814803-3`；candidate pair `hysteresis-calib-v1`；runRole=calibration。

## 被测基线（防旧包）

- commit `835e663`（[P-011] Reconcile bottom/total clipped ledger，含 4b1f635 v2.14 planning pass 全量）。
- HAP：`entry-default-p011-recon-fix-signed.hap`，SHA256 `dca1a7c7cff0aa08ae086cdd94e6f93a309f3a81df0dbd625e9793f960e71b74`，runner digest 闸门校验通过。
- 源码树 entry/ 干净（git status 空），WaterSandField.ets 含 A-1 对账修复（:925-937）。

## gate run 结果（checker=p011_slip_formal_check.py schemaVersion 2）

**阶段一 MUST-PASS 全部通过**（这是与 e2f02a1 基线的本质区别）：

| 闸门 | 结果 |
|---|---|
| hardConstraint（tan30，±45° 平台豁免 643 子步/81,661 边） | ✅ 164,084 边 **0 违约** |
| observationContract | ✅ 0 错误 |
| tiltLedgerAlignment | ✅ 0 缺失/0 不一致 |
| budget（A-1 对账恒等式） | ✅ 0 不匹配（835e663 修复生效） |
| phaseEndpoint 七相全部 | ✅ 坡度稳定、悬沙 ≤5% |
| rasterErrors | ✅ 0 |

checker overall=FAIL 的唯一驱动是 **SLIP-02 收敛包（30°±2°/R²≥0.90/t90）**，但该包已按 2026-10-20 阶段边界裁决 `DEFERRED / NOT VALIDATED`（docs/priority.md 第 4/6 条、test-cases.md §11.4），不阻断阶段一，本次仅作诊断记录。

**run-level 协议闸门（补采资格）**：边身份 52,888 grant **0 错位**；stop 373 条 **0 限流误标**；七相完整、字段齐全。按阶段一口径，**gate run 资格判定：合格**。

## 停机原因：±45° 平台在本基线下结构性不可达（新阻断，非采集问题）

复测发现与 e2f02a1 基线有本质差异：

1. **effectiveTilt 全程最高只到 29.906°**，从未到达 ±45°。positiveOvershoot 相内 progression：12.0→24.0→29.87°后钳死（targetTilt=45 不变）。
2. 根因：`835e663` 包含 `4b1f635` v2.14 planning pass——**运行时 tan30 可行域闸门生效**。向 45° 推进时，可行域交集上界 = atan(tan30°)≈30°（减去边距），planning pass 把 thetaAccepted 钳在 ~29.9°，BLOCKED/钳位语义按 §20.4 正常工作（这正是该闸门的设计行为：45° 参考线下材料不可能满足 tan30，于是**倾角本身被拒绝推进**）。
3. 后果链：冻结 schedule 的 ±45° 相在物理上永远停在 ~30°平台 → 协议要求的**稳定平台（effectiveTilt=±45°、rate=0）子步数为 0** → 每方向 start/stop 首转折 bracket 永远为 0 → `thetaStart/thetaStop` 在 ±45° 平台上**不可观测**。raster 佐证：overshoot 相末水面几乎水平（截图 `run-001/pos45-thumb.png`），收敛诊断 finalSlope 2.91°/0.12°（vs 旧基线同相有崩塌坡）。
4. checker 的 ±45° 豁免解决的是"到达 45° 后 tan30 硬门槛误杀 run"的问题；但 planning 闸门解决方式是**根本不让你到达 45°**——豁免口径与运行时限幅是两个层次，后者使补采协议的观测对象消失了。

## 判定与决定

- run-001 保留全部证据（`test/artifacts/p011-v212-recalib3-20261020/run-001/`：ledger 1,935 记录、七相 raster、manifest、schedule、summary），**不计入 fit/holdout**（无 ±45° 平台覆盖，不满足分层键"倾角方向覆盖"）。
- 按补采执行单停止条件：**"若 clean pilot 的稳定平台过滤后某一方向无 start……停止扩批并将该 schedule 的证据缺口提交 manager，不能现场改 schedule 或伪造 bracket"** → **停止扩批，上报 manager**。
- 有效合格 run：**0/34**；观测③维持 `BLOCKED（合格标定 run / 样本不足）`。

## 需 manager/designer 裁决的矛盾（三选一，各有代价）

当前存在**不可同时成立**的三个要求：
- A. 标定协议需要 ±45° 稳定平台（冻结 schedule 不可改）；
- B. v2.14 planning 闸门把运行时倾角钳在 ≈30°（tan30 可行域，为阶段一安全 MUST-PASS 而设，不可放宽——这是 4b1f635 的核心交付且经 architect §22 CONDITIONAL_PASS 验收）；
- C. 补采资格的 tan30 豁免只覆盖"到达 45° 后"的参考线判定。

候选路径：
1. **换标定平台角度**：协议平台角从 ±45° 改为 ±30°（planning 闸门允许的边界内），schedule 改版为新 protocol version 重新预注册——代价：与"冻结 schedule 不可改"冲突，且 30° 恰在阈值搜索窗 25°≤thetaStart<30° 上缘，平台角与被测阈值重叠，可辨识性存疑，需 designer 论证。
2. **观测窗口豁免 planning 闸门**：给标定 run 增加 capture-mode 开关（如 Want 参数 `p011capturemode`），在观测 run 内临时绕过 thetaAccepted 钳位（保留遥测记录钳位请求），代价：安全 MUST-PASS 在观测 run 内失守，需要 manager 明确"标定 run 豁免 ≠ 产品 run 豁免"的边界，并有 tester 侧配套闸门。
3. **接受 ±30° 平台现实并重定义观测③**：放弃 45° 平台标定，thetaStart/thetaStop 改在 ±15°/±30° 平台上标定（planning 闸门下唯一可达的平台），45° 行为留待阶段二 omega/限幅标定后——代价：与 designer 原方案的目标角度偏离，需 designer 重写可辨识性论证。

**effector 不预判选项，只交证据**：本报告提供 gate run 全套数据（ledger/遥测/raster）证明矛盾的存在与形态。未解除观测③ BLOCKED，未输出 thetaStart/thetaStop，未改物理、未改 schedule、未改用例。
