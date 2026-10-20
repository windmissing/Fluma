# P-011 观测③ v2 采集制品冻结记录（effector）

日期：2026-10-20；责任：effector。本文落实 `docs/observation-protocol-P011-v2-preregistration-v1.md` §3 冻结清单中 effector 侧的待绑定制品。**本文只冻结制品与身份摘要，不构成 pilot 启动授权**——按预注册稿 §1/§8，启动前仍须绑定 manager 的 pilot 批准记录。

## 1. 已冻结制品与摘要

| 制品 | 路径 | SHA-256 | 状态 |
|---|---|---|---|
| 规范 schedule CSV | `calibration/schedule-obs3v2-preregistration-v1.csv` | `685be2bcbfdc14e261d86e3a105fc9a383a386709c02b96613f48f46504fe5a4` | ✅ 与 designer 预注册稿 §2 注册的 `scheduleSha256` **逐字节一致**（复算核对通过） |
| 采集 runner | `tools/run_p011_obs3v2_ninephase.ps1` | `893a992865bf8e81b6ad02d70237edbbc1da32fa7de0b9fc9b1f5573b0fea64f` | ✅ 已固化；启动时内置 schedule/HAP 双摘要闸门，任一不匹配即拒绝运行 |
| 候选 manifest（pilot pair） | `calibration/candidate-manifest-obs3v2-preregistration-v1.json` | `fbe0b488bf52e32c7afd65b1c185ad5a4dbcddd7fbf5c4287b9d2db9c04c68ac` | ✅ 含 candidateId `cand-28-22911`（pair `pair-28-22911`，thetaStart=28 / thetaStop=22.911 占位候选，搜索窗 `[25,30)` / `[15,25]` 不变）、设备/build/协议/runner 摘要、randomization `NOT_STARTED` |
| 被测构建 HAP | `entry-default-p011-obs3v2-891370d-signed.hap` | `c9867dac82cd9689bdbb97187b3f9ae195ef3f3a57c9e80eedbf3cbaadf62d6f` | ✅ 从干净 worktree `D:/hmprojects/hw-obs3v2-891370d`（detached HEAD = `891370d731eef32bf813f52910fa5a82439df2ed`）构建并签名，含 §19.5 逐 writer 材料 guard（B·续，architect §23.1 `PASS_WITH_DEFECT`） |
| checker（safety） | `test/p011_slip_formal_check.py`（schemaVersion 2） | 由 tester 维护版本 |tester 已就位（test-cases.md §11.4/11.5/11.7 已同步） |
| checker（calibration） | `test/p011_calibration_protocol_check.py`（schemaVersion 1） | 由 tester 维护版本 | ✅ 已存在；期望九相 targets、`scheduleSha256`、`platformToleranceDeg` 均从 manifest 读取，与本 manifest 字段对齐 |

## 2. runner 与规范 schedule 的映射声明

runner 内嵌九相序列 `0/+15/0/+30/0/-15/0/-30/0`、每相 `holdMs=5100`，与规范 CSV 逐行等价；输入 sequence = phaseIndex+1。schedule.csv 采集制品记录 `sequence,phaseIndex,phase,target,holdMs,commandTimeMs`。runner 不读 CSV 文件而按冻结序列硬编码，但启动闸门先验证 CSV 文件摘要 = 注册摘要，保证"规范摘要 ↔ 执行行为"绑定；runner 自身摘要单独登记（上表），两者任一变更均为新 revision。

## 3. 平台识别参数来源

预注册稿 §4 已由 designer 给出：平台角容差 `0.20°`、速率容差 `1e-9°/s`、窗口 `≥200ms` 且连续 ≥3 子步、planner 排除规则。这些数值已写入候选 manifest（`platformToleranceDeg: 0.20`），calibration checker 从 manifest 读取（缺失时报 `PROTOCOL_METADATA_MISSING`），满足"必须由 designer 给出、checker 审核"的流转要求。

## 4. pilot 启动前置核对单（逐项待勾）

- [x] 规范 schedule 文件 + 摘要固化（= designer 注册值）
- [x] runner 文件 + 字节摘要固化
- [x] pilot 候选 manifest + 摘要固化（含 build/schedule/runner 身份）
- [x] 被测构建从干净 891370d worktree 构建签名 + 摘要固化
- [x] tester checker（safety + calibration）就位、test-cases §11.7 同步
- [x] **manager pilot 批准记录绑定**：priority.md 2026-10-20「P-011 观测③平台可达性矛盾裁决」第 2/4 条（"允许进入新 protocol 的单 run clean pilot，以测实际平台/bracket yield"），适用构建 `891370d731eef32bf813f52910fa5a82439df2ed` / HAP `c9867dac…2d6f` / protocolId `p011-observation3-v2` / revision `preregistration-v1`；用户于 2026-10-20 13:06 当面确认批准生效
- [x] 设备状态确认（62T0225B10010020 在线；pilot 启动时执行屏幕常亮/存储检查）

## 5. 纪律重申

- 本记录与 runner 均不含"开始采集"的执行；在 manager 批准记录绑定前运行 runner 会被视为越权，effector 不执行。
- pilot 结果无论正负都按预注册稿 §8 输出（`thresholdVerdict=NOT_EVALUATED`，删失/未观测照实标注），manager 审阅后才决定扩批。
- 任何制品变更（schedule/runner/manifest/构建）= 新 protocol revision，重新冻结，不现场修补。
