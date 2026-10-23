# P-011 correction 矩阵 conditioning 统计口径修订 v1

日期：2026-10-23。责任：designer。状态：**STATISTICAL_SCOPE_REVISED / SINGLE_WORKING_STATE / SAFETY_BOUNDARY_MONITOR**。供 tester 对 108-run 正式批直接引用；不修改采集 schedule、物理实现、硬安全阈值或历史 [预注册稿](preregistration-P011-correction-matrix-v1.md)。依据：[conditioning 验证回执](cnd-verify-receipt-P011-v1.md) §5/§7/§9/§10、[12-run pilot](corr-matrix-pilot-capture-receipt-P011-v1.md) §4、冻结计划 `calibration/p011-corr-matrix-formal-plan-v1.json`。本稿按用户已批准的“**27 个可交换 block、单一 working state**”降级口径解释正式数据；仅给方法裁决，不预判 D 有收益。

## 1. 统计实验单元与适用域

`p011BuildConditioningPrefix(kind)` 对 G1/G2/G0 发出相同的 `0→+15→0→−15→0` 前缀，`kind` 只写标签。Q1/Q2 成立：通道接通且正式初态不同于 reset；Q3 不成立：跨档 maxΔS 均值 `0.029149` 与同档重复 `0.025964` 接近、范围重叠（比 `1.123`）。故正式批只有**一个经该 65 s 前缀处理的 working-state 分布**；G1/G2/G0 是计划来源/执行批次标签，不是三种场景，也不是独立实验因素。`none` 验证 run、8 个 100 s conditioning 验证 run 和 12-run pilot 均不进正式效果统计。

108 个正式 run = **27 个完整 block × 4 变体**（原 G1 12、G2 12、G0 3）。每 block 按冻结 `blockId/seed/Williams` 序列保留 A/B/C/D 四 run 的配对；一条边、子步或单 run 均不能当独立 block。主比较仍是 **D 对 A 的完整九相窗口**：逐 block 算 `gain_b=1−J_E,D,b/J_E,A,b`，仅在 A 分母大于预注册分辨率且完整数据闭合时定义。B−A、C−B、D−C 维持机制解释用辅助对照，不能从中挑显著者改报主成功。

**与原首轮 12 block 的关系**：在“同一前缀/初态分布、同一 build/schedule/参数、block 完整且执行期漂移可接受”的条件下，27 block 是**同一主比较的样本扩容**，不是三场景各自复现，也不是从 12-run pilot 扩容。原 G1 的 12 个正式 block 是该池的一部分，后续 G2/G0 标签的 15 个正式 block 贡献新的 block/seed。由于计划按 G1→G2→G0 顺序执行，标签与采集时间混杂；不能仅凭“命令序列相同”宣称无批次漂移。tester 必须保留原标签/执行序/设备温度作描述性诊断，比较各批次的起点 S 摘要、accepted tilt/风险需求、J_E,A 与帧时/温度分布；诊断只能说明池化适用范围，**不得按结果删 block、重新分层或另挑有利子集给主 PASS**。若发现不可忽略的执行期漂移，报 `POOL_EXCHANGEABILITY_UNCERTAIN` 并提交 manager 复议；27-block 主结果可保留为有条件描述，不冒称多场景泛化。

正式主统计使用预注册的 `10,000` 次 **paired-block percentile bootstrap**（seed `20261022`）：从 27 个完整 block 有放回抽样，每次整块保留 A/B/C/D 配对，计算 27 个 `gain_b` 的中位数，双侧 95% CI 为 bootstrap 2.5/97.5 百分位。原工程门槛数值**不改**：中位 gain `≥10%` 且 CI 下界 `>0` 才可称单一 working state 下主收益有统计支持；20%/30% 仍是目标档/较强档描述，不是择优替代线。普通通道代价、输入可达、帧时、软项、逐笔下降界、硬安全与金额闭合继续用原 §3/§5 的定义和数值界，在同一 27-block 池内配对统计；非劣 CI 按原单侧对应端裁决。任何 `FACTOR_NOT_EXCITED`、零分母、缺关键 trace、未完成 block 或硬失败按原状态体系诚实报告，不补 epsilon、不把 108 run 的多边/多步当样本量。若整批未完成 27 block，不能直接用已采 block 按 27-block 方案判 PASS；需 manager 事前批准样本量/截尾修订。

## 2. 撤销的假设与允许的结论

| 原预注册 §4/§5 目标 | 本批裁决 | tester 报告写法 |
|---|---|---|
| G1 竞争场主效果 | **场景身份撤销**；D−A 主比较保留，改为“单一 conditioned working state” | 报 27-block 池化主结果与原 G1 12-block 历史标签诊断；不得称“竞争场已独立验证” |
| G2 邻边/热点稳健性及 `median≥0、CI下界≥−5%` | **假设撤销 / NOT_EVALUABLE**；无真正 G2 初态 | 不报 G2 稳健性 PASS/FAIL；G2 标签只作时间/批次诊断，不据其子集 CI 声称外推 |
| G0 均匀无正贡献控制、诚实停滞 | **控制身份撤销 / NOT_EVALUABLE**；无无贡献初态 | 不把 G0 标签的停滞当“控制如预期”，也不因其有正贡献而判 D 异常；真实 D≤0/guard/库存原因仍按逐笔 trace 如实描述 |
| C/D 权重与净贡献因子可辨识 | **独立待检**；conditioning Q3 失败并不自动证明算法因素被激发 | 查有竞争的候选数、`R_limit<Σu`、C/D 排序与实际 grant 差；若全授或只有单候选，报 `FACTOR_NOT_EXCITED`，D−A 即使改善也不能归因为权重 |

因此，原预注册 §5.2 中“**主收益 + G2 稳健性 + 代价/帧时/软项 + 硬门槛齐备才提交有用净收益建议**”在本批**不能完整满足**：G2 项没有可评估对象。本批最多形成“26/20 出厂推荐值、这一个经相同前缀处理的 working state 下，D−A 净收益/代价/安全证据”的建议；若要声称跨场景稳健或正式转正，须另做真正差异化 G2/G0 并重新预注册，由 manager 裁决。不能把标签池化 CI 当作 G2/G0 的替代证据。

## 3. tan30 贴边界的现阶段判定

硬约束是每提交后全边 `maxSandTopInvariantError=max_c |S[c+1]−S[c]−tan(effectiveTilt)| ≤ H+EPS`，`H=tan30°=0.5773502691896257`、引擎 `EPS=1e−9`；`audit.range` 是正式硬安全字段。pilot 无 conditioning 前缀的 run 峰值 `0.499–0.532`，对 H 留 `0.045–0.078` 余量。正式首两 block 的 A 峰值 `0.499728`、`0.501021`，余量约 `0.07762`、`0.07633`；B/C/D 峰值 `0.5773502691896xx`，与 H 差约 `10^−14`，至少比批准容差小四个数量级。正式已见 run 的 `range=False` 计数为 0；首个 D run 的贴边发生在 `effectiveTilt≈−18°`，**局部残差贴 H 不等同于命令角达到 30°**。

**裁决：当前按“契约内边界饱和”登记，不构成 tan30 形变上界不适用或硬安全 FAIL 的证据；不得因此放宽 H、EPS、guard 或预算。** B/C/D 相对 A 消耗了可用角度余量，这是事实；是否由 correction 笔本身、其他 material writer 的邻边级联，或 `updateTilt` 的可行域移动导致峰值，**仅凭 run 最大值不能归因**。贴边不证明 correction 有效，也不证明一定有问题；tester 应按 run/phase 统计 `invariantAtBoundary` 子步数及连续时长，并在首次贴边子步记录 `effectiveTilt`、边 c、r、提交前后 S、writer/grant、guard 截断、planner accepted/clamped 状态。守恒/预算/库存和 `range` 继续逐子步独立判；“正常饱和”仅是当前安全契约下的分类，不是物理机制审计通过。

正式脚本的 `invariantAtBoundary=|maxErr−H|≤EPS` 是**贴边诊断标志**，不是越界标志。旧哨兵零容差把 `H+2.1e−14` 误判停批；现与引擎一致用 `H+1e−9`。首 run 旧误判的 partial 记录保留作诊断，不进 27 block；不得把它与重采 run 合成一块，也不能为求绿进一步增大容差。

## 4. 后续若 `range=False` 的停批与归因预案

1. **立即停整个正式批**，保存失败 run 的原始/trimmed ledger、身份和哨兵输出及所在 block 的已采 run；标 `SAFETY_FAIL / RANGE_FALSE`。未完成 block 不进主收益估计；不得删失败轨迹、换 seed、只重跑失败臂或继续采到 108 个“全绿”run。
2. 找到**最早** `range=False` 的已提交子步，而不是用整 run 最大值定位。输出：runId、blockId、substepId、phase、accepted/effective/target tilt、越界边 c、`r_before/r_after`、`maxErr−H`、各材料 writer 的计划/guard admit/grant/预算回写，以及 `updateTilt` 两轮 planner 的可行区间与裁剪结果。特别检查修正中央边及 c±1 邻边的闭式联合预演、writer 顺序，以及纯 tilt 更新是否使原合法 S 相对新参考变违规。
3. 对照同 block 的 A 与 B/C/D，在**同一子步输入和状态**可重放时才做 writer 因果定位；先分辨 `audit.range` 真实为 false、采集/解析不一致，还是 `maxErr` 仅在 `H+EPS` 内贴边。若确为提交后 `>H+EPS`，按 guard/tilt 责任修复并补正式安全回归；原失败证据永久保留。不得以“视觉无害”“B/C/D 通常贴边”豁免。
4. 修复改变 build/hash、算法或采集方法后，由 manager 决定重新冻结批次及统计修订；旧失败批与新批不直接池化。仅在证据证明是哨兵实现错误、引擎 `audit.range` 始终为 true 且原始账可独立复算时，可像回执 §9 那样更正哨兵解释并保留完整追溯，不重写物理判据。

## 5. 可直接引用的报告句式

> Conditioning 通道能把 reset 态推进同一 working state，但 G1/G2/G0 共用完全相同的注入序列（`STRATUM_FACTOR_NOT_EXCITED`）。本正式批以 27 个完整 A/B/C/D 配对 block 为统计单元，合并估计单一 working state 下的 D−A 主效果；原 G2 邻边稳健性与 G0 无贡献控制均为 `NOT_EVALUABLE`，不作跨场景结论。B/C/D 的 tan30 贴边峰值与 H 差约 `10^−14`，当前 `audit.range=True`，按契约内边界饱和登记；机制来源未由 run 峰值判定。若任一已提交子步 `range=False`，立即停批并按最早越界子步追溯 writer/guard/planner，不放宽 `H+1e−9`。

历史预注册的 G1/G2/G0 设计意图及阈值保留；本 revision 只修订**本次已批准的 108-run 执行数据如何解释**。采集可与本设计裁决并行，最终统计和是否转正分别由 tester、manager 决定。
