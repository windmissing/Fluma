# T533 V4保守触顶门与独立oracle分歧裁定 v1

2026-11-16，architect，P5，不使用真机。正式合同T497 §4步骤5（原卡称§4.5）明确：任一 `eta0[c]+eHi>cap[c]` 即ETA_TRUNCATION_RETRY。实际T522产品GLSL已实现，T523保留status4原始负控。**责任为独立测试oracle遗漏正式谓词，不是GPU物理实现缺陷，也无设计歧义需要改阈值。** 不改tester资产、不重跑GPU或新增CPU仿真器；只对指定反例作有限整数复算，原件/SHA见 `analysis-T533-evidence-v1.json`。

## 1. 指定反例完整重算

N128，T0全0，W0=eta0=[5,5,0…]，cap=[5,CQ,CQ…]，r=[0,4,0…]，L65536。a=[0,4,0…]，want4，bound=CQ−5，Ttrial=[0,4,0…]。V10；F(0)=6，F(1)=133，故eHi1、Fhi>V，若只按clamp/residual算法，Wlo=[5,1,0…]、d4，按增量合格列升序得Wtrial=[5,2,1,1,1,0…]，精确和10。这个候选具有水容量解，**但不满足保守门**：c0的5+1>cap0=5。

T499 `solve_lambda` 在eHi后直接分 Fhi==V / residual，无上述全列谓词，因此返回OK。实际GLSL `m3TrialEta` 在eHi后、写水候选前核全128列并return4，符合合同；不能因oracle叫OK就删除GPU门。T497已明确λ为有限启发式准入，不宣最大可行λ，数学存在其它分配解不推翻该政策。

同反例将r1改成1/2/3或lambda32768，仍正want，F(0)<V、eHi1，c0仍违规；缩λ不会自动解决。零want应先走WANT0，无需η；eHi0且eta0==cap是等号允许。既有λ下降/8attempt/LMIN停止保持，不从本例推导为always可准入或篡改请求量。

## 2. 分流及工程约束

manager将**测试工具实现修正**交tester，冻结T499 oracle新revision/输入SHA与旧输出历史；tester独立在完成lower_bound后加全列`>`拒绝，早于Whi/residual成功返回。保原GPU `truncation_contract` 负控、现status4原件和源成功前缀/失败trial回滚检查。developer不改独立oracle，不把正式保守门改成只检查r>0列或只检查最终水量；designer本轮无须改算法。

这不是用例不合理需放宽标准的裁决：指定反例合法且有区分力，继续保留。若修正oracle后历史fixture用cap0补齐“未使用列”而导致全局触顶门拒绝，tester逐项核真实输入语义：无J空列cap应CQ；若确有J净空cap0就是有意义的全局拒绝，不能默改成CQ凑原expected。只有拟更改场景/适用域/验收标准的争议才交manager裁决；算法本身不由tester选。

## 3. 精确检查与回执

tester必要检查：指定Fhi>V反例、Fhi==V但另一列保守触顶、等号边界允许、eHi0、WANT0先行、J cap、request0但触顶列、U64累加、全8trial/reason/λ/accepted全有全无；更新fixture provenance后再由独立GPU harness消费冻结原件。期望字典先生成、产品程序后执行；不得调用生产solver生成自己的期望。原T523已测离线事实保留，新oracle版本覆盖域单列，不能把旧基线全绿冒新版本全绿。

本卡只复算现成数学定义及指定oracle单次调用，不修改/执行完整测试套件，不新增CPU业务投入。工程裁定已完成，产品无需按此分歧返修，tester接修oracle；实现/真机/效果通过结论未扩张。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，XComponent+shader，禁止Canvas/软件GL产品回退，GAP/250ms/120Hz quota/30fps不改。
