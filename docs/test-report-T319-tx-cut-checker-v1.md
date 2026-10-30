# T319 事务原档定向 checker（离线，结束待验收）

2026-10-30，tester 离线。按 architecture-T314 用现有 T304 归档（不重跑 20 矩阵、不改原件）作三补档独立对账 + T297 报告勘误范围确认。

## 一、beforeEvidence.hit 口径确认（T314 核心勘误）

三档 `result.faultPlan.hit=false` 均为旧 cut1/6/18 专用接点残留；**T250 BEFORE v10 正确口径 `beforeEvidence.hit=true` 三档全部成立**——T297/T304 报告若引用旧 `faultPlan.hit` 判"未命中"须按此勘误（报告勘误、原件不改）。

## 二、三 cut 原码/前缀/rollback 独立对账

### cut20 / root3 / ord1（ENDPOINT_FIXED_CLOCK，观察提交）

- `physicalStatus=FAULT`、`error=ENDPOINT_COMMIT_KEY_MISMATCH`、rootRows SOURCE1(19.2)→SOURCE2(6.4)→STEP3 `EXCEPTION:ENDPOINT_COMMIT_KEY_MISMATCH`
- **原字节对账**：root3 `snapshot` 30 词 **identical**（物理状态回滚）+ `totalUsed 0→5.0`、`sourceBudgetRaw/localBudgetRaw` 变化（**观察/endpoint 提交保留**）——T250 字典"已提交状态保留、非物理 rollback"语义与原字节吻合
- **缺证项（精确列出）**：`observationRows=[]`——T314 预期的 BEFORE_INPUT / INJECTED_READSET / ORIGINAL_CHECK_RETURN 三原行**未入归档**，观察注入读集与原检查返回值无法逐字节复核。需 worker 在 beforeEvidence 内补三行序列化（交 developer 小改），不重跑

### cut5B / root3 / ord2（SHAKE1_TILT20，物理失败）

- `firstCode=5`（父 step 码，consumer 原 4 不混写 ✓）、receipt `stepped=false, substeps=0`
- **逐字节 rollback 达成**：`snapshot identical` + `poolRawBytes identical`（物理状态与池原字节全等）
- SOURCE 成功前缀保留：root1 19.2 / root2 6.4

### cut9 / root2 / ord1 / ON（T171_ORIGINAL，admission 失败）

- `error=TX_GPU_ADMISSION_FAILED`（外层码）；原 consumer `selectPair` 抛 `TX_GPU_NOT_REUSABLE:STALE`、caller 映射 13（原码侧记录，不与外层码混写 ✓）
- root1 SOURCE 成功 19.2 → root2 SOURCE 3.2 → FAULT：**SOURCE1 成功前缀保留** ✓

## 三、Want 冷启合同与 T297 勘误边界

- 三档独立 PID、faultPlan PID 与各自 PID 一致、suite=false、steps=32（number）——T304 冷启合同遵守确认
- T297 报告勘误范围（已在 T304 报告登记）：旧四个 AVL/GPU 档"无档/未执行"（同 PID 二次 Want 不计执行），**不倒推旧唯一因**；本轮 checker 未发现与 T314 分流冲突的原码，无需再派工程修

## 四、结论

T319 **结束待验收**：三档 hit 口径/原码分层/逐字节 rollback/成功前缀全部独立对账通过；唯一缺证 = cut20 观察三行未入档（精确项已列，交 developer 补 beforeEvidence 序列化）；无双臂收益宣称（分母独立维持 NOT_PROVEN）。

证据：`test/reports/t319/checker_summary.json`（原档字段核对记录）。
