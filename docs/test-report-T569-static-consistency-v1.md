# T569 v1 · 目标 9 动态动员 v2 独立验证（离线可做域）

日期：2026-11-19，tester，不使用真机（按卡面 manager 接续：GPU 域待真机标定）。
依据：T573 DM v2 交付（test/t573/ 生成器+dm_solver_body.inc）对照 T556 独立
oracle（test/t556/independent_oracle.py）。
证据：`test/t606/t569-static-consistency.json` + 本报告对照表。

## 离线可做域完成：dmEval↔oracle 静态语义审计

T573 `dmEval`（GLSL）与 T556 `eval_q`（SS4.2 独立 oracle）**10 项逐条对照全部
一致**：f=F+q / u=RNE(p,f)（f=0→u=0）/ r=max(f,H_REG) / head=floor(kShear·u²/
(ONE·gn)) / yieldDepth=RNE(muStart·r/ONE) / excessHead / HsWide / H=min(B,max
(Hg,F+Hs)) / chiS=floor(excessHead·Q/r) / shearOff 域分支。

两处 GLSL 域扩展（非冲突）：head 负分子上取整防御修正（u²≥0 不可达）、
shearOff 配置分支。

## DM20/22/29/31：NOT_RUN（GPU 域待真机派工）

- DM20 强起动全管线 / DM22 replay 拒收 / DM29 动员后 CFL / DM31 冷预算——
  均需 Maleoon 真机 GLSL dispatch 实测，离线不可冒
- 离线准备已齐：solver body 静态审计通过；T556 oracle fixtures DM01/02/05/06/
  07/19/17 已 PASS（v1）；DM20–31 期望值推导沿同一 oracle 通道随时可扩

## 工具链阻断警告（关联 T541 v2）

任何携带 g9dm/g9spf kernel_assets（140KB+ R-string）的树重建会被 OHOS 26.0.0
clang/lld CPU 零挂死阻断——T573 solver 经主树 pipeline.comp 生成链交付，真机
验证须在工具链规避方案（T541 v2 回 architect）就位或使用既有已编产物包。
