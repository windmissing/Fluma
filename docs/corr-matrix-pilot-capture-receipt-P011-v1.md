# P-011 correction 矩阵 12-run pilot 采集回执 v1（effector 真机）

- **日期**：2026-10-22；责任：effector。对应任务【correction·矩阵pilot】（manager 14:20 三裁决后执行）。
- **性质**：pilot（预注册 §4/§6）——只核验数据闭合、因素身份、安全与激励；**永不进正式统计**，不据收益择样，不证明统计成功。
- **被测构建**：`entry-default-s132-signed.hap`（main HEAD 9827739，含 ABCD 变体开关 c054021 + schedule 路径接线 ee12705/5d4935e + F1 消融档；SHA256 `c861e45f…a5983` 硬校验）。
- **真机**：VYG-AL00（62T0225B10010020）。设备全程独占，无外部接管。
- **产物根**：`test/artifacts/p011-corr-matrix-pilot-20261022/`（12 run 目录 × {hilog.txt, ledger.ndjson, ledger-trimmed.ndjson, identity-sentinel.json} + collect-manifest.json + hard-safety-audit.json）。

## 1. 采集执行摘要

| 项 | 值 |
|---|---|
| 计划 | G1/G2/G0 各 1 block × A/B/C/D；Williams ABDC/BCAD/CDBA（seed=20261022；DACB 为 pilot 未用第 4 序列）；strata 执行序 G1→G2→G0 |
| 变体绑定 | B/C/D：`p011cvar=<V>` + `p011route=relax-correction-weighted-v1`（schedule 通道，接线修复后）；A：cvar=A、route 不绑（原 relax） |
| 公共 | 26/20 factory-recommended（thresholdCalibrationId=user-configured）、同 build/seed=20261016/wspreset=thick/wssnapshot=true/testmode、exec 身份三元组 |
| G1/G2/G0 初态 | **同初态降级执行**（manager 裁决②；p011conditioning 未设，G 标签语义登记于 manifest） |
| 输入 | 九相模板 A=18.7°/0.5°/s（manager 裁决③主档）；每 run 204.3s + 冷重置 |
| 实际用时 | ~61 min（12 run，含缓冲；首次批因哨兵实现缺陷中止 2 run 后重跑） |

## 2. 一次中止记录（红线执行实例）

首批 runner 的身份哨兵误从 hilog 正则抓 variant（ledger 才是载体）→ G1-B run02 误报 IDENTITY FAILURE 中止（红线要求的行为正确触发，判定源错误）。诊断确认 ledger 上 `context.correctionVariant='B'` + route 绑定 + D-1b 闭合全部正常。处置：哨兵改为**解析 ledger context.correctionVariant + D-1b 闭合校验**，中止批次隔离改名 `p011-corr-matrix-pilot-20261022-aborted1/`（2 run 留档），全 12 run 重跑。重跑批次为新目录，无混批。

## 3. 身份回执（12/12 全过）

每 run ledger 全量统计 `context.correctionVariant` 分布 + D-1b 四字段闭合：

| run | variant 分布（子步数） | D-1b requested/granted/rejected/stalled | 闭合 |
|---|---|---|---|
| G1-A-run01 | A:4783 | 0/0/0/0（route 不绑，NOT_APPLICABLE 语义 ✅） | ✅ |
| G1-B-run02 | B:4733 | 10529/10444/85/53 | ✅ |
| G1-D-run03 | D:4706 | 10310/10222/88/56 | ✅ |
| G1-C-run04 | C:4732 | 10106/10002/104/71 | ✅ |
| G2-B-run05 | B:4719 | 10572/10484/88/50 | ✅ |
| G2-C-run06 | C:4738 | 10201/10097/104/67 | ✅ |
| G2-A-run07 | A:4783 | 0/0/0/0 | ✅ |
| G2-D-run08 | D:4752 | 9988/9893/95/54 | ✅ |
| G0-C-run09 | C:4733 | 9910/9789/121/66 | ✅ |
| G0-D-run10 | D:4713 | 10643/10560/83/52 | ✅ |
| G0-B-run11 | B:4734 | 10136/10069/67/46 | ✅ |
| G0-A-run12 | A:4773 | 0/0/0/0 | ✅ |

- 每 run variant 分布**单一**（4733±50 子步全部同 variant），无跨 variant 污染。
- D-1b：requested = granted + rejected 逐 run 成立；A 组四字段全零落地（预注册 §2「A 的 correction 计数 NOT_APPLICABLE」）。
- B/C/D 的 granted/requested ≈ 97–99%（拒绝率 0.6–1.2%），stalled 占 requested 0.45–0.70%——**激励充分，非零产出**（预注册「筛明显零产出」通过）。

## 4. 硬安全快扫（12/12 全过；正式判定移交 tester/checker）

逐 run ledger 全量 `audit.range/conservation/finite/stock/capacity` 布尔扫描：

| 项 | 结果 |
|---|---|
| range=False（tan30 越界） | **0 次 ×12 run**（`maxSandTopInvariantError` 峰值 0.499–0.532 < tan30=0.5774；发生于 |tilt|≈8–15° ramp 段，A=18.7 主档下无结构性越界——与 2026-10-20 定性的 45° 结构性限制不冲突，本模板峰值 18.7°） |
| conservation=False | 0 次 ×12 |
| finite=False | 0 次 ×12 |
| stock=False | 0 次 ×12 |
| capacity=False | 0 次 ×12 |
| totalBudgetExcess 峰值 | ≤5.3e-14（浮点残差级） |

**硬安全失败 0 次**——未触发「立即停批」红线。

## 5. 观察登记（非判定）

1. **G1 内部 D vs A 的 D-1b 对照**：B/C/D requested ~10.0–10.6k/子步口径远大于 A 的 0——符合机制预期（correction 候选笔数仅 route 绑定 run 记录）。
2. **B 与 C/D 的 granted 量级接近**（10444 vs 10222/10002 in G1）：pilot 输入下预留分配接近满授，权重差异（B 全授序 vs C/D e² 加权）体现在 order 而非总量——与预注册 §1「权重可辨识性检查」提示一致，正式 run 的 J_E 比较仍待离线分析。
3. **conditioning 未设**：G1/G2/G0 三 strata 实际同输入同初态——manager 裁决②明确 pilot 接受；正式 run 前初态注入通道按 developer 任务补。
4. 首批 2 run（G1-A/G1-B，aborted1 批次）与重跑批同配置同 seed——已隔离不混入，不用于任何统计。

## 6. 红线遵守

- 硬安全失败 0 → 未触发停批；身份失败 1 次（首批，判定源缺陷）→ 按红线停止并隔离，未重试到全绿后才继续（修正哨兵后全量重跑）。
- pilot 数据不进 fit/holdout，不据收益择样。
- 无效果结论、无 VALIDATED 判定；J_E/ΔE_relax 等正式指标提取属 tester 离线分析职责。

## 附：产物

- `collect-manifest.json`（含 manager 裁决引用、Williams 分配、variant 绑定语义）
- `corr-pilot-{G1,G2,G0}-{A,B,C,D}-runNN/`：hilog.txt、ledger.ndjson（原始）、ledger-trimmed.ndjson、identity-sentinel.json
- `hard-safety-audit.json` + `safety_audit.py`（复跑脚本）
- 中止批次：`../p011-corr-matrix-pilot-20261022-aborted1/`
- runner：`tools/run_p011_corr_matrix_pilot.ps1`
