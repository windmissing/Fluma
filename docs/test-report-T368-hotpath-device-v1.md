# T368 CPU 热路修订真机定点验证（T361 §6.3 首次设备互斥分解：hotApply 86%）（真机，完成——P2 FAIL 定量归因在案）

2026-11-06，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：**T366 交付签包 SHA256 `b7b71466283b5d32a0a18f1a69543bcc3f587db78e24643e2131167d4a2d37fd`**（离线 8/8；hotApply 互斥段入冷出口；64 词 substeps）。对照基线：T347（空场 12fps）/T352（触摸后 1.3fps）。

## 1. P1 空场/注入期帧率：**PASS（持平无回归）**

20s 桶曲线：**12.3-13.1fps** 稳定（bucket 0-120s），与 T347 空场 12fps 持平（略升 0.3-1.1fps 在采样噪声内，不宣改善）。

## 2. P2 停源高负载帧率：**FAIL（1.2fps）——但主导阶段首次定量锁定**

- 注入后坍塌至 **1.2fps**（bucket 140s 起全程 1.2），与 T352 的 1.3 同水平——**T366 修订无帧率收益**（对照 T352 判读一致）。
- **决定性新证据（T361 §6.3 交付）**：停源高负载 256 子步互斥分解（冷档 `v2-budget-t368.json` SHA256 `b1f4ff3aacb85fdde51d1e959c972705d90b41bfa5a5d3a155d5aad70d24f2e1`）：

| 阶段 | 耗时 | 占 wall |
|---|---|---|
| **hotApply** | **90526ms** | **86.0%** |
| collect | 4507ms | 4.3% |
| exact70 | 4219ms | 4.0% |
| field | 2296ms | 2.2% |
| preview/publicVerify/authorityCompare | 3337ms | 3.2% |
| 其余（step/structure/strictDomain/fillCopy/seal） | 339ms | 0.3% |

- **主导阶段 = hotApply（86.0%）**——T352 时代"阶段在别处不可定位"现在精确指认：帧预算耗尽不是 native/pack/verify，而是热路 apply 本体。下一步分流（hotApply 内部分解：audit 行数 vs 视图拷贝 vs alias 检查）交 architect。

## 3. P3 互斥闭合：**PASS（100.0%）**

stage sum = 105247ms == wall sum = 105247ms，**设备上零缺口零重叠**——T366 互斥计量修订在真机生效（T351 离线闭合设计的设备证实）。

## 4. P4 守恒与身份：**PASS**

- live=157 停源后恒定（无泄漏无漂移）；scene **CURRENT 317/317**、request==actual 逐行相等、零 code100/STALE/FAULT。
- 前后台往返 pid 24808 不变、零 jscrash/THREAD_BLOCK。

## 5. 结论与移交

- **P2 FAIL 先 architect（按卡）**：hotApply 86% 为精确分流输入——候选分解（a）hotApply 内 T311 逐行审计（T312 已定量 5.45ms/R 线性，R404-536 时 2.2-2.9s/帧 与 1.2fps 吻合）；（b）alias 快检 350-534 次/apply（T366 交付自述）。两者均指向 hotApply 内部，需下一张工程卡做 hotApply 内部互斥细分。
- T366 修订收益如实：计量闭合达成（P3），帧率未改善（P2）——不冒修复。
- 证据：`test/reports/t368/`（v2-budget-t368.json、hilog_full.txt 35657 行 317 scene 行、layout0-2.json）。
