# T539 · GPU V4 保守 eta 门 oracle 更正与回归 v1

日期：2026-11-17，tester，不使用真机。
依据：T533 裁定（architecture-T533-conservative-eta-oracle-triage-v1.md）+ T497 §4.5。
范围：只修 T499 独立 oracle 漏的全列 eta0+eHi 容量谓词；产品 GPU 门正确，developer 不改此测试资产；版本化原输入/expected/来源 SHA。

## oracle 修正内容（V1→V2）

1. **全列保守门**（T533 §1）：`solve_lambda` 在 eHi 二分后、Whi/residual 成功返回前插入
   `any(eta0[c]+eHi > cap[c] for c in range(N))` → `ETA_TRUNCATION_RETRY`（detail 含
   violatingColumn/gate=CONSERVATIVE_ETA_TRUNCATION）。
2. **eHi==0 边界对齐产品**（本卡新发现）：V=0 且 F(0)>V 时产品 `m3TrialEta` 走 else 分支
   `if(eHi==0u) return 4u` 拒——oracle 原 residual 分支对 eHi=0 会负下标炸/漏拒。对齐为
   `eHi==0 && F(eHi)!=V → ETA_TRUNCATION_RETRY`（gate=PRODUCT_EHI0_SUM_MISMATCH）。
3. **fixture 未使用列 cap 语义修正**（T533 §2 授权路径）：M3A02/M3A03e 的 cap 补齐
   0→CQ（无沙无水列 cap 应 CQ；cap=0 是 fixture 补齐错误，会被全列门判全局触顶）。
   M3A02 期望按全列域语义独立复算重冻结：a=[Q,Q]→Ttrial=[11Q,11Q]、eHi=74899
   （F(e)=126e 于 e<Q 域）、V 奇 → residual d=37 升序 c2..c38 各+1；新增 M3A02b
   （小域 η 相等 + 门不误伤验证）。**期望先独立复算冻结，后跑 oracle**——未调用生产
   solver 生成期望。

## 回归结果

**26 PASS / 4 NOT_IMPLEMENTED / 0 FAIL**（v1 基线 17 PASS/4 NOT_IMPL；新增 T539e1..e7
8 项 + M3A02b，M3A02 重算后仍 PASS）。

### T533 §3 检查矩阵（8 项全过）

| 项 | 判定 |
|---|---|
| e1 指定反例（T533 §1 原样 N128/T0全0/W0=eta0=[5,5]/cap=[5,CQ]/r=[0,4]/L65536） | ETA_TRUNCATION_RETRY viol=0（v1 曾误 OK）✓ |
| e2 等号边界 eta0+eHi==cap | OK（== 不触发门）✓ |
| e3 F(eHi)==V 精确命中 | d=0、Wtrial[0]=50 ✓ |
| e4a r1=3（a=[0,3]）缩 λ | 仍 c0 触顶拒 ✓ |
| e4b λ=32768 | 仍拒 ✓（缩 λ 不自动解决，T533 §1 文字核实） |
| e5 WANT0 先行 | WANT0（不进 η/门）✓ |
| e6 eHi=0 且 F(0)>V | 产品语义拒（oracle 新对齐）✓ |
| e7 门查全 128 列（r=0 列 c5 触顶） | viol=5 ✓（不只查 r>0 列） |

### 产品同输入差分（T522/T523）

`truncation_contract` 场景（T522 solver_cases.py 原输入）经 V2 oracle 复算 =
**ETA_TRUNCATION_RETRY（viol=0）== GPU 实测 status=4**（T522/T523 保留负控）——
oracle/产品一致，分歧消解。未改 cap、未只查 r>0、未为凑通过放宽。

## T520 G920 勘误（卡面要求项）

T520 报告正文 "G920 supportImpulse=7 入环境账" 的 **7 是误值**（Bd=7 动摩擦参数被误当
反冲量）。按 T515 §5.8 正式式：REST 且 |8|≤Bs10 → p'=0（静摩擦分支不经 Bd）；差额入环境账
= p'−pTrial = **−8**。oracle 数值断言（pAfter=0/REST）不受影响——纯报告文字勘误，
fixture/oracle 无改动，原件留存 git 历史（commit 4e2647f）。勘误段已追加
（docs/test-report-T520-three-state-oracle-v1.md）。

## provenance（版本化）

- V1 冻结原件：`test/reports/t539/m3_source_oracle_v1-frozen.py`
  SHA256 `5f9b01e5859546b10ef096e53fe1220bb2842f3968323a42042311b5a4e446cc`
- V1 基线输出：`test/reports/t539/t499-v1-baseline.json`（17 PASS，历史）
- V2 provenance：`test/reports/t539/provenance-v2.json`（V2 SHA、变更清单、差分结论）
- 现行：`test/t499/m3_source_oracle.py`（revision=T499_M3_SOURCE_ORACLE_V2）

## 边界说明

- 未新增 CPU 仿真投入（只复用既有 oracle）；未跑 GPU（T522/T523 产品差分引用既测事实）。
- oracle V2 覆盖域单列，不把旧基线全绿冒新版本全绿（v1/v2 结果分档留档）。
- NOT_IMPLEMENTED 4 项（M3A01/04/09/10）维持 T499/T523 原判定不变。
