# T148 向外除法与纯核缺口分流 V1

2026-10-27；architect；原卡`6adfe799e4b03273ad8c38b6`。不使用真机，不重跑测试，不改业务或判据。消费T147报告/执行器/真实JSON、T133算术与证书实现、T127/T110合同、T135部分代码及中断回执。精确整数核算见`analysis-T148-division-sign-v1.json`：仅解析归档样本并核算有理数，不调用产品模块或重跑tester。

**分流完成：F1是tester除法oracle/解释的符号错误，当前divInto负号正确，禁止按报告建议删除。** T135缺口主体是已有工程待实现，可继续隔离开发；其阶段二尚未完成，完整strict生产保持禁止。T147历史9/10与原始FAIL保留，不篡改成10/10；修订oracle后的独立执行另由tester交付。

## 1. 精确商方向裁定

令v=RN64(a/b)、b≠0，所有比较针对binary64输入的精确有理数值：

`q−v=a/b−v=(a−v*b)/b=−(v*b−a)/b`。

`compareAffineBound(v,b,[-a],1)`返回s=sign(v*b−a)；`directed(v,t)`需要t=sign(q−v)。故正确关系为`t=−s*sign(b)`，与现实现`-sign*(b>0?1:-1)`一致。b<0需同时翻转除数极性；s=0时端点[v,v]。这不是修改数值设计，也不是需要developer返修的生产缺陷。

T147正文把`exact−v=(v*b−a)/b`写成反号，D2b执行器同样计算`sRaw*sign(b)`后交expectEndpoints；真实实现compareAffineBound的乘积与标量符号本身正确。add/sub/mul oracle不因此撤销；其独立结果仍按原覆盖消费。intervalDiv依赖当前正确div，不能据F1推其有同源缺陷；非跨零区间除法尚须独立定点覆盖，不能从区间乘法PASS外推。

报告“决定性反例”的精确结果：

- a=181.74490928649902，b=0.000032622665372628035，v=5571123.8554711。
- `v*b−a=503444847/37778931862957161709568 >0`。
- `a/b−v=−503444847/1232449452300566528 <0`。
- 因而正确端点是[nextDown(v),v]，恰为报告记载的生产返回；[v,nextUp(v)]不含精确商。

对`d2b_f1_evidence.json`实际归档的**5条**数据，独立BigInt整数在2^-1074网格上交叉乘比较，5条production区间全部包含精确商、5条reported oracle区间均不包含；正负分母都有。文件不是75条全量样本，不能把“75样本归档”照抄。执行器口径也允许把overflow拒绝计入matched，故仅凭matched5不能认定全部为精确零残差；循环75轮不等75个有效非零分母/有限商。

## 2. 先修工程语义，再交tester定点修订

T127除法段补明s与t两个语义，本文与原工程一致；生产`P011OutwardArithmetic.ets`不改。tester不使用真机，拥有`test/p147_outward_independent.cjs`及其版本化报告/新结果目录：

1. D2b预期改为`expectEndpoints(v,-sRaw*sign(b))`，保留原sign(v*b−a)的独立精确计算；不能把所有op oracle统一翻号。报告反例改正为“现实现正确、原oracle错误”，保留原V1结果。
2. 删除decisive里混合`=== false`与`||`的含混布尔表达式；明确按位比较期望端点，另用有理数交叉乘验证lo≤q≤hi。固定上述反例和±1/3、±1/10、正负分母、精确商、下溢到±0/次正规、overflow/零除拒绝；记录输入及端点原bits，不依赖JSON保留-0。
3. 非跨零intervalDiv检查四角精确商并形成全区间，含全负分母；跨零/端点零仍拒绝。共享result/corner调用后立即复制标量；不得引用被下次调用覆盖的对象。
4. 将attempted/skippedZero/rejectedOverflow/acceptedExact/acceptedInexact/mismatch分列；逐条归档本轮有效样本而非只前5条。seed初值和最终值分开，不能将最终LCG状态标为重放初始seed。Python fractions可作独立复核，但本轮T148不冒充执行过该tester/Python套件。

这是**测试oracle解释/实现修订**，非新增产品验收门，也不认定原D2b为预期未实现RED。若修正后出现端点不包精确商的真反例，携带原bits/精确分数回architect，届时再判产品缺陷；不预先安排删负号返修。

## 3. T135：现有产物与实施缺口

当前`P011WriterKernels`的node()只增加计数，没有往envelope填逐node ideal/error/η；preview会调用LOCAL材料证书，commit只写材料列和部分poolMass/state。`materialOnly=true`不可能替代完整事务验证；registry中SUPPORTED仅表示隔离草稿有该分支，不是已认证strict可运行授权。7/7开发自测保留范围，不能签T134实施后独立验证或完整支持。

| 缺项 | 当前裁定与接力 |
| --- | --- |
| RN node/δ0/A/b/η | T110与T127§2–3已定，developer继续实现，非等新设计。填写每个节点输入/原RN值/向外ideal与误差区间及分支域；CanonicalS三次RN的δ0不漏，settle A≠1、top MIX系数与后分支b≠0保留 |
| INWARD | settle、TOP/MIX、已CONTACT_WAIT partial LAND已有保守内域路线，按T127做系数正/负/0、闭/开端点、向内可表示q及精确affine复核；RAW/INWARD共冻结入口bound。不能当前RAW失败一律ZERO冒本范围完成 |
| TOP后两分支 | MIX_LIGHT/MIX_LIGHT_HEAVY仅RAW+完整审计或NoWrite ZERO；INWARD V1不支持。若要扩大其逆像支持，才交designer新设计，不阻止既定RAW/ZERO |
| LAND/原子pool | 只支持已CONTACT_WAIT单phase合法request；full释放一次、partial RN(m−q)<m，保留ID/source/位置/速度；补free-list/nextId/lineage、每列在途cache、provenance与dry waterTop到同一patch |
| ticket/预算/审计 | actualDebit与qGranted/qControl/相和/真实库存Δ分列；只退原debit−qControl一次，最终publish前shadow结算；audit失败恢复所有参与对象，不在验证前写材料试算 |
| 身份/commit | 必须绑定request/branch/node版本、owner/serial/config/原before所有位型、pool及ticket；共享shadow下次preview会被覆盖，旧token失效，不可重复消费。先验证全部before/证书/资源，再一次原子提交；失败所有参与对象不变或按既定真实rollback |
| 真正设计缺项 | 首触AIRBORNE退零时CONTACT_HOLD完整运动/事件关系、top后两段RN逆像、其它writer/publication/invert/外源闭合，仍交designer逐项补篇。不得混入本版已支持纯核 |

NoWrite ZERO不执行零量split、±0写、水/cache派生重算或pool motion/state变化；不创建成功LAND事件或新成功ticket。若原路径需要CONTACT_HOLD，返回UNSUPPORTED，不能假无写。INWARD最大性只针对已证保守内域，不声称最大真实物理可行量；包络与实际patch矛盾报ENVELOPE_CERTIFICATE_MISMATCH，不能减q/ZERO藏证书问题。

原子shadow写集必须显式列材料、水、来源、预算ticket/refund、pool slot/ID/state/phase/source/x/y/vx/vy/mass、free-list/nextId/lineage、列cache及事件草稿，不以poolDigest或材料证书代替。shadow只保存实际changed槽/列及必要未变邻居，定容并先检查容量；任何before不符、audit或容量失败不得留下部分写。LOCAL只验证邻边，FINAL无入口豁免且全局审计后才可能未来publication；本卡仍不接生产strict路由。

## 4. T134预测试与可执行接力

T134 RED-01“模块均不存在”属于旧时点NOT_IMPLEMENTED探针。模块出现后该断言过时，自然退出缺实现基线；它的FAIL不是产品回归，也不能改为“所有实现已通过”。tester保留旧报告/fixtures与其它逐node/分支/身份oracle，增加明确IMPLEMENTATION_PRESENT并消费真实模块输出；必要更新适配，不删数值/原子事务检验。

**T135现在即可继续隔离实现**，不等待并不存在的div产品返修，也不等待CONTACT_HOLD全设计。最小顺序：①依既定RN图实现逐node/δ0/affine与域证明；②支持核INWARD+后两top分支RAW/ZERO；③补完整atomic shadow与请求/证书消费身份；④离线编译/自测交付，tester以T134 fixtures和修订后T147除法oracle独立验证。开发卡关闭不以真机为前置；缺哪阶段如实报部分，不能阶段中断冒完成。签名配置问题与compile/PackageHap范围分开，不改密码冒新包。

后续通用prompt：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本行状态有限例外），不改滴答同名清单、不回退他人修改。所有渲染含背景前景仅新XComponent+shader，禁止Canvas上屏/失败回退；失败FAULT或显式STALE上一有效新管线帧。失败先architect，工程先文档再developer、设计designer、用例争议manager；未实现预期RED登记NOT_IMPLEMENTED。manager登记不验收，不启完整strict生产、不签设备/step73/code5修复通过。
