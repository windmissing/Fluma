# P-011 peak 字段 revision v1

日期：2026-10-25（Asia/Shanghai）。责任：architect。任务：[6adcd3d3e4b0e6ce82a851f1](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adcd3d3e4b0e6ce82a851f1)。当前修订：rev2（T07 三项对齐收尾；rev1 正文来源保留）。

状态：**FIELD_DESIGN_READY / ALIGNED / PENDING_MANAGER_ACCEPTANCE / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_CALIBER_MANDATORY**。

本稿只登记独立 peak 字段与时序接口，不实现采样、checker 或流式峰值，不发布 tester 协议，不宣布 `peakProtocolEffective`。本卡记录 manager 23:55 已下达用户五项裁决：B、全部活粒子含 state2、E 分母、固定 on 后 3s 每成功提交采样、原 0.5 与分档迁移。designer 稿中 PROPOSED/NOT_APPROVED 是先前提案状态，本文不回改其历史，也不把当前口径批准等同协议生效。

## 1. 依据、当前源码及独立身份

唯一数学/场景依据为 [designer 迁移设计](design-P011-ws08-peak-nondeposited-migration-v1.md) §3–§5、§7–§8；复用依据为 [endpoint 字段 rev2](architecture-P011-endpoint-fields-revision-v1.md) §2–§6。既有 [endpoint 协议](endpoint-protocol-P011-terminal-nondeposited-v1.md) 与 [终点裁决](decision-P011-terminal-nondeposited-fraction-v1.md) 原文及版本均不改。

首次交付的历史核查：当时本地尚无 tester 独立 peak protocol，故登记待对齐。当前 rev2 已对照 [tester peak 协议](peak-protocol-P011-ws08-peak-nondeposited-v1.md)（commit `4591b46`）完成三项登记；唯一结论及逐字段对照见 §9。原核查与源码行号仅描述 rev1 时点，不声称当前实现仍未变化。`peakFieldAlignment=ALIGNED` 仅表示本字段稿与该协议内容对齐；tester 冻结稿 §0 的历史 `PENDING_ARCHITECT_REGISTRATION` 不回改，也不据此宣布协议生效。

源码核查（此刻事实，不代表新字段已实现）：

- `WaterSandField.ets:1383` 的 `setShakeInput` 验证 finite 后钳位并立即写 `shakeInput`，唤醒模型；目前没有记录实际接受事件 ID/同钟 elapsed/全序。计划 dispatch 时间不能替代这里的接受时间。
- `WaterSandField.ets:2133` 的 `recordEndpointCommit` 已提供成功提交 key/ordinal/stamp；`P011NaturalObservation.ets:700` 附近的 `stampEndpointObservation` 复制 post-commit elapsed、积分、源账本及当前 shakeInput。当前数值 shakeInput 仍不等于“本子步实际消费哪条输入”的证据。
- `WaterSandField.ets:5966` 的 `liftByShake` 先取旧 `shakeEnergy` 为 requestEnergy，再更新能量；collect/apply 两遍有不同阶段。不能以 begin context 的 shakeEnergy 或其最大值倒推接受输入。
- 卡片把全序列 `peakSusp` 的位置写为 `test/p011_endpoint_check.py`，实际 `evaluate_shake` / `peak=max(values)` 位于 `test/water_sand_check.py:476/483`；前者的 `windowPeakFraction` 是终点末窗量。此处登记源码定位差异供 tester/manager 核对，不修改两者或将 endpoint 末窗当作 peak。

| 身份字段（报告路径） | 类型与登记值 | 时机/责任、必需性与缺失处理 |
|---|---|---|
| `peakFieldRevision` | string=`p011-peak-fields-v1` | writer 声明实际支持的独立字段 revision；新采集必需，缺为 NOT_IMPLEMENTED |
| `peakMethodId` | string=`peakNonDepositedFraction` | checker 报告；新量必需，缺为 NOT_IMPLEMENTED |
| `peakMetricRevision` | string=`shake-peak-nondeposited-v1` | 采用设计稿拟议值；checker；缺为 NOT_IMPLEMENTED |
| `peakProtocolId` | string=`p011-ws08-peak-nondeposited` | 已核对 tester §0/§10；本稿不发布协议 |
| `peakProtocolRevision` | string=`ws08-peak-nondeposited-v1` | 已核对同上；身份吻合不等于正式生效 |
| `peakAuthority` | string=`APPROVED`，附本任务卡裁决来源 | 报告批准的口径状态；不继承 endpointAuthority，不代表实施通过 |
| `peakProtocolEffective` | boolean，当前保守登记 false | 仅按 manager 的独立适用范围决定写 true；缺省不得推断 true |
| `peakFieldAlignment` | string=`ALIGNED` | 本次登记已完成版本/hash及差异核对，见§9；不替运行时报告背书 |
| `peakProtocolSourceHash` | string 或 null | checker/证据包登记实际 tester 稿 SHA256；本次实际完整值见§9.1；旧证据缺来源仍不补造 hash |
| `evidenceMode` | string | 复用报告证据模式；离线补算为 RETROSPECTIVE_DIAGNOSTIC，旧稀疏值仅 OBSERVED_SAMPLED_MAX |

`ledgerSchemaVersion=7` 及可选扩展兼容旧文件，不代表任何协议生效。新增 context 字段均以 `peak*` 命名；既有 `endpoint*` 采样原语原名引用，不改名为 peak 的别名。旧数据缺新增字段不倒填零或假事件。

## 2. 数学、窗口与字段通则

```text
D_H=sum(stateAfter.bh+stateAfter.mh); D_L=sum(stateAfter.bl+stateAfter.ml)
U_H=sum(stateAfter.suspH); U_L=sum(stateAfter.suspL)
P_H=particles.massH; P_L=particles.massL             # pState∈{1,2}
E_H=VH0+context.sourceInjectedH; E_L=VL0+context.sourceInjectedL
D=D_H+D_L; U=U_H+U_L; P=P_H+P_L; E=E_H+E_L; M=U+P
W=[shakeOnEffectiveElapsedMs, shakeOnEffectiveElapsedMs+3000)
peakNonDepositedFraction=max_{eligible successful commits k in W}(M(k)/E(k))
```

只对**同一提交**先相加再求 max，相等保留全序最早提交。分母不得用 A=D+U+P 或历史 D+U 代替；P 包含水内/空气/接触待准入全部活粒子，state0、视觉 tracer 和累计 degraded 事件量不计。LAND 正余量继续计 P，MERGE 后计幸存存量一次，DEGRADE 后只计 U。所有质量单位沿用 material-volume unit，比例为未乘 100 的 number。

WS-08 仍是 default/TEST/tilt=0、沙堆已形成、rest3s→shake=1×3s→off 后 rest8s；整 run 无外源准入，故 IH=IL=0、E 恒定、sourceEpoch 不变。累计期望公式不是新有源变体授权。0.5 比较为未舍入的 `M>=0.5*E`，没有向下容差；≥0.90/0.75/0.50/0.35/0.20/<0.20 分别为 10/8/6/4/2/0，仅引用已批准迁移锚点，不重写测试判据。

字典通则：新增字段在旧 schema 文件中语法可选，在声明支持本 revision 的采集中按下表必需。表中“缺字段”指 writer 能力/必要字段未提供 → NOT_IMPLEMENTED；字段能力存在但该 run 漏事件/漏记录/关联不全 → INCOMPLETE；非有限、负质量、重复伪提交、矛盾关联或不闭合 → INVALID_LEDGER。初始库存/准入前身份不可证 → INVALID_BASELINE；经可信基线算得 E≤0 → INVALID_DENOMINATOR。多原因保留完整列表，最终执行顺序严格沿 tester §4.1，见§9.3，不能选择有利原因放行。

## 3. 实际接受边界、消费身份与同毫秒全序

### 3.1 新增原始输入事件流（待实现）

在 run 证据 manifest 下登记 `peakShakeInputEvents`（array，对应独立 NDJSON 文件亦可，以 path/hash 固定）。每次**实际接受**的 shake 写入在物理模型同步输入入口生成不可变事件；不是页面点击、发令或日志回调时间。所有写入来源（schedule、页面 tilt 派生、manual）都经过同一身份适配，不新增一条能改变动力学的输入通道。

事件最小字典（各字段均物理输入侧产出；接受时原子固定）：

| `peakShakeInputEvents[]` 内路径 | 类型/语义 | 必需性/缺失处理 |
|---|---|---|
| `eventId` | string，同 run 唯一接受事件身份 | 必需；无能力 NOT_IMPLEMENTED，某事件丢失 INCOMPLETE，重复/歧义 INVALID_LEDGER |
| `runId` | string，与原 ledger run 相同 | 必需；缺 INCOMPLETE，跨 run 关联 INVALID_LEDGER |
| `peakInputCommitOrder` | 非负安全整数，同 run 输入接受与成功 commit 共用全序 | 必需；能力缺 NOT_IMPLEMENTED，断档待事件类型/覆盖清单核对，矛盾/重复 INVALID_LEDGER |
| `effectiveElapsedMs` | finite number≥0，同 `endpointClockId` 的实际接受 elapsed，ms | 必需；能力缺 NOT_IMPLEMENTED，run 时钟无法关联 INCOMPLETE，逆行 INVALID_LEDGER |
| `endpointClockId` | string，复用既有同 run 时钟域 | 必需；缺/混域无映射 INCOMPLETE，不用墙钟替代 |
| `value` | finite number∈[0,1]，模型钳位后实际外部 shakeInput | 必需；缺 NOT_IMPLEMENTED，非法数 INVALID_LEDGER；场景要求 W 中为 1，不把 shakeEnergy 当 value |
| `requestedValue` | finite number，钳位前值 | 可选输入诊断；缺不阻断库存计算，不虚构值 |
| `kind` | `SHAKE_ON\|SHAKE_OFF\|SHAKE_UPDATE` | 必需；on=本 occurrence 正式开启1，off=正式停止0，其他写入为 update；缺 NOT_IMPLEMENTED |
| `shakeOnEventId` | string 或 null，所属激活段的 on 根事件 | on 自指；期间 update 指同根；off 指被终止根；无激活的零写入为 null。活动段缺关联 INCOMPLETE |
| `caseOccurrenceId` | string，WS-08 本次执行身份 | 必需；缺 INCOMPLETE，不拼接重复 case |
| `endpointPhaseOccurrenceId` | string，引用既有实际 schedule occurrence | 必需；缺 INCOMPLETE；不以 phaseD 字符串替代 |
| `commandSequence` | 非负安全整数，引用实际 schedule 命令 | 新正式 schedule run 必需；缺 INCOMPLETE |
| `sourceKind` | `SCHEDULE\|TILT_DERIVED\|MANUAL` | 必需；能力缺 NOT_IMPLEMENTED；不能用未登记来源覆盖事件掩盖场景不符 |

on/off 边界以事件指针登记在 manifest/report：

| 路径 | 类型/语义 | 时机、责任/必需性及缺失 |
|---|---|---|
| `shakeOnEventId` | string，唯一 on eventId | 输入侧接受时登记；必需，字段能力缺 NOT_IMPLEMENTED，具体事件不足 INCOMPLETE |
| `shakeOffEventId` | string，结束该激活段的 off eventId | 输入侧实际停止时登记；完整采集必需，漏 off/提前退出 INCOMPLETE |
| `shakeOnEffectiveElapsedMs` | number，严格引用 on.effectiveElapsedMs | 输入侧边界，checker 核对；同上，不取首个高 energy 时间 |
| `shakeOffEffectiveElapsedMs` | number，严格引用 off.effectiveElapsedMs | **复用既有同名实际 off 边界**；不给 endpoint 另造 off 时钟/别名；缺能力 NOT_IMPLEMENTED、漏边界 INCOMPLETE |
| `peakWindowStartElapsedMs` | number=`t_on` | checker 重算；必需，无法绑定 on 为 INCOMPLETE |
| `peakWindowEndElapsedMs` | number=`t_on+3000` | checker 重算；必需，不能改成 t_off 或数据最后时间 |

重复写1不能重新启动或延长窗口。若既有调度确有同值 refresh，仍各记接受 eventId，保持原 on 根身份、原固定窗口；是否属于同一合规 schedule 由实际 command/occurrence 证据与 tester 资格规则确认，不能把 manual/tilt 来源冒充 schedule。中途不同值/提前 off 记资格不符；晚 off 不延长 W，其漂移是否合规仍交 tester 已有/后续预注册门禁，不设新容差。

### 3.2 每成功提交的实际消费证据（待实现）

| ledger 精确路径 | 类型/语义 | 产出时机、责任/必需性及缺失 |
|---|---|---|
| `context.peakFieldRevision` | string=`p011-peak-fields-v1` | writer 封装原始 record；新 run 必需；缺 NOT_IMPLEMENTED |
| `context.peakInputCommitOrder` | 非负安全整数，成功提交在共享全序的位置 | 物理提交成功封装时分配一次；新 run 必需；缺 NOT_IMPLEMENTED，重复/顺序矛盾 INVALID_LEDGER |
| `context.peakConsumedShakeInputEventId` | string，实际驱动物理输入路径的最近接受事件 | 子步实际读取 shakeInput 时锁定，成功后复制；必需；缺能力 NOT_IMPLEMENTED，引用缺失 INCOMPLETE，错配 INVALID_LEDGER |
| `context.peakConsumedShakeOnEventId` | string 或 null，所消费事件的激活根 | 同上；W 内必需且等于 manifest.shakeOnEventId；off/rest 可 null；无可证关联 INCOMPLETE |
| `context.peakConsumedShakeInputValue` | finite number∈[0,1]，该消费输入的实际值 | 同上；必需；缺 NOT_IMPLEMENTED，和引用事件值矛盾 INVALID_LEDGER；值≠1为资格不符 |

消费身份要在本子步真实读取输入时固定，collect/preview/apply 共享同一个输入快照，post-commit 只复制该快照，不能在 finish 时回读“最新输入”替代。`liftByShake` 使用旧 energy 产生请求是原动力学，输入值为1而首步实际 LIFT=0仍是消费 on 的成功提交；不需等 energy 达峰才开启窗口。该登记不授权重排 collect/apply 或改变能量更新。

当前同步调用栈内输入接受与子步执行自然串行；若后续实现存在异步输入，则在原实际接受/消费边界线性化并保留证据，不能在运行中半个子步更换输入还只报一个身份。无法证明单一快照时报告 INCOMPLETE 并反馈 manager，不自行改变物理行为解冲突。失败 attempt 可保留消费诊断，但不得分配成功提交 order/ordinal 或参加 max；接受过的外部事件不因物理回滚而伪装为未发生。

**同一毫秒全序落点是事件与提交两处同名 `peakInputCommitOrder`。** 它由物理侧单一计数器串行分配，和 elapsed 一起保留，禁止 worker 重排或用 JSON 文件顺序猜测。同 ms，on 事件之后且 off 事件之前、确实消费该根输入的提交才可属于活动段；off 后即使 shakeEnergy>0也不入峰。事件顺序与消费根矛盾记 INVALID_LEDGER。不能用现有提交 ordinal 单独排序输入事件。

成员首先必须满足 `t_on<=context.endpointSampleElapsedMs<t_on+3000`，再检查消费身份及共享全序。即使 commit 与 on 同 ms，发生在 on 之前也排除；off 同 ms 之前的提交仍需满足固定半开窗口，恰等于 `t_on+3000` 一律排除。全序不把右端同 ms 的提交“挪回”窗口；若时钟分辨率无法证明边界归属，报告边界不确定/INCOMPLETE，按 tester §2.2/§4.1/§4.2 处理（见§9.3），不伪造小数时间或容差。

## 4. 复用的原始提交、时钟、库存与覆盖

以下是**已有契约的引用**，不是本稿新增/升级 endpoint 字段。当前支持情况以实际 writer/export 验证为准，本文不宣布完整实现。

| 精确路径/字段 | 类型与语义/取时 | 所有者、必需性与缺失 |
|---|---|---|
| `context.runId` | string，同 run | 物理侧原提交身份；必需，缺 INCOMPLETE |
| `context.sequence` | 安全整数，原 ledger 序号 | 物理/observation；必需，缺 NOT_IMPLEMENTED；重复伪记录 INVALID_LEDGER |
| `context.frameId` | 安全整数，原帧身份 | 同上，缺 NOT_IMPLEMENTED |
| `context.substepId` | 安全整数，原成功子步身份 | 同上，缺 NOT_IMPLEMENTED；同帧两子步分开 |
| `context.attemptId` / `context.commitStatus` | 原 string/null 与 committed/blocked/rolled_back/rejected | 原 attempt 审计；失败仍保留但不计入 W，身份不足 INCOMPLETE |
| `context.endpointSampleStage` | `POST_COMMIT`，失败 null | 物理侧成功后 stamp；必需，缺能力 NOT_IMPLEMENTED，声称成功却无 stamp INCOMPLETE |
| `context.endpointSampleElapsedMs` | finite number，成功提交后同 run 单调 ms | 原 sampler 唯一取时；必需，缺 NOT_IMPLEMENTED，混域/覆盖不足 INCOMPLETE，逆行 INVALID_LEDGER |
| `context.endpointCommittedOrdinal` | 安全整数，成功提交连续递增一次 | 原 sampler；必需；能力缺 NOT_IMPLEMENTED，run 内断档 INCOMPLETE，重复伪提交 INVALID_LEDGER |
| `context.endpointClockId` | string，同 on/off/schedule/commit 时钟 | 原 adapter；必需，不能证明同域为 INCOMPLETE |
| `context.endpointPhaseOccurrenceId` | string，原 schedule occurrence | 原 schedule→stamp；必需，缺 INCOMPLETE |
| `context.integratedPhysicsSeconds` | finite number≥0，成功 dt 之和，s | 原提交累计器，含本步一次；必需，缺 NOT_IMPLEMENTED；仅诊断，不能替换3s elapsed |
| `context.physicsDt` | finite number，真实积分 dt，s | 原物理侧；必需以核对积分，缺 NOT_IMPLEMENTED |
| `stateAfter.bh/bl/mh/ml/suspH/suspL` | 原 number[]，同一 post-commit 床/悬库存 | 物理 canonical；各数组必需，缺 NOT_IMPLEMENTED，非有限/负值等 INVALID_LEDGER |
| `particles.massH` / `particles.massL` | finite number≥0，state1/2分相存量 | canonical 粒子池成功后；各项必需，缺 NOT_IMPLEMENTED，不把 C 的 P 补0 |
| `particles.count` | 非负整数，活粒子数 | 原摘要；必需且闭合，缺 NOT_IMPLEMENTED，矛盾 INVALID_LEDGER |
| `particles.byState.FLYING.{count,massH,massL}` | 原状态1摘要 | 同步 post-commit；诊断可选，缺仅状态归因 NOT_OBSERVABLE，不阻断可信库存主量 |
| `particles.byState.CONTACT_WAIT.{count,massH,massL}` | 原状态2摘要，含 LAND 余量 | 同上，缺不补0，不宣称无滞留；若有则必须闭合 |
| 原逐粒 snapshot/`particleEvents` | 原 canonical 粒子数组/事件 | 按现有 v7 审计保留；非新峰值统计附加逐粒采集要求；存在时核对同提交，矛盾 INVALID_LEDGER |
| `context.sourceInjectedH` / `context.sourceInjectedL` | finite number≥0，同提交后累计实际准入 | 原物理源账本；各项必需，缺 NOT_IMPLEMENTED，不重复累加；不能证明准入前基线为 INVALID_BASELINE |
| `context.sourceEpoch` | 原安全整数，源 epoch | 原物理账本；必需，缺 NOT_IMPLEMENTED；WS-08变化为资格不符，绝不拼 epoch |
| `VH0` / `VL0` | finite number≥0，reset后任何准入前初始分相总量 | 原 audit 首条 heavyTotalBefore/lightTotalBefore 与 manifest key/hash证明；checker引用，证据不可证 INVALID_BASELINE，总 E≤0 INVALID_DENOMINATOR |
| `initialBaselineCommitKey` / `initialBaselineStateHash` | 原提交键/string hash | 原 manifest；必需，基线关联不可证 INVALID_BASELINE |

`peakCommitKey` 只投影原 `(runId,sequence,frameId,substepId)`，不另发一套提交身份；同时保留 ordinal、attemptId、case occurrence、phase occurrence。route/build/params/input/hash/clock/ledger 身份继承原证据包字段（buildCommit、sourceTreeHash、packageHash、routeId、particleRouteActive、scheduleId/hash、inputScriptHash、ledgerPath/hash、参数及 seed），不能跨 run/build/route 拼峰；关联不足记 INCOMPLETE。

stamp 顺序沿既有契约：全部物理事务/预算/guard/audit成功→锁定本次 key→同栈复制 stateAfter、P、源账本和已锁定消费身份→原累计积分/ordinal一次、原 elapsed stamp 一次→不可变 record 发布。新增 peak 全序仅附加证据，不使 endpoint sampler 再累计或再次读取“peak 时间”。worker/checker 只能读快照，不能异步回读 live 池。物理失败不参加 max，但不能从安全审计中删掉。

覆盖复用成功 ordinal 连续性、输入事件流、开始/固定结束边界证据和既有合法 `DORMANT_STATE_RECHECK`。只读保持证据不新增 commit/order/积分，不给 peakSampleCount 加1；tester §2.2允许带身份的合法休眠只读复核；无法证明保持状态或边界覆盖时记 INCOMPLETE。不能将 frozen/rollback/no-op/250ms日志空洞解释成合法休眠。

## 5. checker 独立报告字段字典

下表均为 **checker 在完整原始 v7 输入核验后重算/编排**，不把 writer 的在线 max 当正式权威。在线 reducer 可保持 O(1) 峰值状态，但必须输出原始证据供 checker 独立重算。字段定义不要求新扫描/复制完整物理场；质量扫描复用既有守恒扫描。

| 报告字段 | 类型/语义与时机 | 必需性/缺失或无效处理 |
|---|---|---|
| `peakNonDepositedFraction` | number或null，W内 max M/E，完整窗后定稿 | 主量必需；必要质量/身份字段缺 NOT_IMPLEMENTED；覆盖不足 INCOMPLETE，不造0 |
| `peakStockMass` | number或null，峰值提交 M | 必需，和主量同状态 |
| `peakReferenceMass` | number或null，峰值提交 E | 必需；可信 E≤0 INVALID_DENOMINATOR |
| `peakCommitKey` | 原四元提交键或null，相等 max 最早全序 | 必需；缺可核提交身份 INCOMPLETE，空窗不造 key |
| `peakSampleElapsedMs` | number或null，引用该 key 的 endpointSampleElapsedMs | 必需；不能用时刻查最近250ms样本替代 |
| `peakCommittedOrdinal` | integer或null，引用原 ordinal | 必需；和原 key/order矛盾 INVALID_LEDGER |
| `peakInputCommitOrder` | integer或null，引用峰值提交全序 | 必需；能力缺 NOT_IMPLEMENTED |
| `peakWindowSampleCount` | 非负整数，W内合格成功提交数，去除重复导出 | 必需；空窗/覆盖无法证明 INCOMPLETE，重复伪提交 INVALID_LEDGER |
| `peakWindowCoverageStatus` | string，COMPLETE/INCOMPLETE/NOT_IMPLEMENTED | 必需；只按实际全部提交及边界证据登记，不能由值达标推导完整 |
| `peakScore` | 0/2/4/6/8/10或null，未舍入原值分档 | 可评分时必需；无效项为null，不打0分凑平均 |
| `peakStatus` | PASS/FAIL/NOT_IMPLEMENTED/INCOMPLETE/INVALID_LEDGER/INVALID_BASELINE/INVALID_DENOMINATOR | 数值项状态必需；未生效时仅候选项，不能成为正式case放行 |
| `missingFields` | string[]，精确缺失路径 | 必需，完整时空数组；不隐去缺项 |
| `peakInputEligibilityStatus` | string，场景/时序资格状态 | 必需；无法确认为 INCOMPLETE，不用新容差求PASS |
| `peakInputEligibilityReasons` | string[]，提前off、非1输入、来源/phase不符等 | 必需；合规时空数组 |
| `peakBoundaryEvidencePaths` | string[]，on/off/固定窗/采集结束等原始证据 | 必需；漏边界 INCOMPLETE |
| `peakMissingCommittedOrdinals` | integer[]，成功提交断档 | 必需；非空为 INCOMPLETE，不能把失败attempt误算成漏提交 |
| `peakDuplicateCommitKeys` | 原键[]，重复导出/矛盾快照清单 | 必需；完全相同导出重试去重但披露，矛盾重复 INVALID_LEDGER |
| `peakFailedAttemptCount` | 非负整数，相关失败attempt数 | 必需安全诊断；沿既有hardGate处理，不静默删记录 |
| `peakIntegratedPhysicsSeconds` | number，峰值提交原累计积分 | 必需，缺 NOT_IMPLEMENTED；不取代实际 elapsed |
| `peakWindowIntegratedPhysicsSeconds` | number，W成功提交 physicsDt之和 | 必需诊断与设计必要界使用；缺 NOT_IMPLEMENTED，不直接假定3s |
| `peakMasses` | object，见下方逐字段 | 必需原始质量披露；缺相应字段按主量处理 |
| `peakNonDepositedHeavyFraction` | number或null，独立 max (U_H+P_H)/E_H | 分相诊断必需披露；E_H=0为N/A，不影响总量合法性，不造EPS比例 |
| `peakNonDepositedLightFraction` | number或null，独立 max (U_L+P_L)/E_L | 同上；无分相通过阈值，两相独立峰不可相加 |
| `legacyPeakSuspReported` | number或null，旧原字段原样本原窗口原结果 | 双报必需栏目；无实测为null/NOT_OBSERVABLE，保留历史身份，不伪造旧值 |
| `legacyPeakHistoricalReference` | number或null，同W逐子步 max U/(D+U) | 必需双算栏目；D+U=0记该栏目 INVALID_DENOMINATOR，不阻断E>0的新量 |
| `legacyPeakLiveFormula` | number或null，同W max U/max(A,EPS)或原字段实测 | 必需双算栏目；标明证据类型；无真实 metric 只称 LEDGER_RECOMPUTED_LIVE_FORMULA |
| `legacyHistoricalReferenceStatus` | string，上述历史分母合法性 | 必需；不得借旧分母无效把新量判无效 |
| `legacyPeakEvidence` | object，三legacy项各自 denominatorKind/sampleDensity/window/evidenceKind/status | 必需，string字段分别登记每项真实来源；缺来源 NOT_OBSERVABLE，不宣称实测/同窗可比 |
| `sameReferenceSuspOnlyPeak` | number或null，max_W U/E | 独立诊断必需栏目；不是legacy别名，也不和独立P峰相加 |
| `baselineNonDepositedFraction` | number或null，on前最近可证post-commit M0/E | 初态诊断必需栏目；无证据 NOT_OBSERVABLE，不宣称本轮新增扬起 |
| `baselineCommitKey` | 原提交键或null，对应M0 | 同上；区别于run初始库存基线，不新增on前≤5%门槛 |
| `peakNetIncreaseFraction` | number或null，max_W (M-M0)/E | 响应诊断，M0不可证为null/NOT_OBSERVABLE，不作为0.5替代量 |
| `cumulativeLiftGrantedMass` | number或null，W内原事件实际granted之和 | 响应诊断，事件不足 NOT_OBSERVABLE，不用requested/包数估算 |
| `peakLiftRejectionEvidence` | object或null，原guard拒绝与budget clipped来源 | 诊断可选；不足 NOT_OBSERVABLE，不改预算判据 |
| `peakParticleByState` | 原FLYING/CONTACT_WAIT快照或null，在总量峰值提交取 | 诊断可选，缺为NOT_OBSERVABLE；不以独立状态峰拼接 |
| `particleResidueDiagnosticStatus` | 原OBSERVED/NOT_OBSERVABLE | 必需栏目，复用原证据；无逐粒时长不能说已排除永久滞留 |
| `hardGateStatus` / `hardGateFailures` | 原状态string/原失败清单array | 独立硬闸门必需引用；失败时峰值仅诊断，不能单独放行 |
| `legacyPeakResult` | 原结果/分数/旧方法身份对象或null | 必需双报栏目，无证据保留未观测，不重评旧结果 |
| `endpointResultReference` | path/hash + 原endpoint身份、结果的引用对象或null | 独立结果栏目；未产出则NOT_OBSERVABLE，不由peak重算/替代 |
| `naturalnessEvidenceStatus` | string，现有效果链证据状态 | 必需独立栏目；未验证如实登记，不由峰值推导自然/流动 |

`peakMasses` 的各子字段均为 finite number，取**总量峰值同一提交**：`depositedMass`=D、`suspendedMass`=U、`particleMass`=P、`nondepositedMass`=M、`expectedMass`=E、`actualMass`=A；`depositedMassHeavy/Light`=D_H/D_L、`suspendedMassHeavy/Light`=U_H/U_L、`particleMassHeavy/Light`=P_H/P_L、`expectedMassHeavy/Light`=E_H/E_L。各项必需，缺必要源字段NOT_IMPLEMENTED，非有限/库存不闭合INVALID_LEDGER。`peakMasses.suspFraction`=U/E、`airborneFraction`=P/E、`depositedFraction`=D/E（number），沿批准显示口径；分母无效为null+INVALID_DENOMINATOR，不改原WS_METRIC公式。

报告必须区别数值子项、hard gates、endpoint、自然度/生命周期结果。尚未独立生效时即使内部 peakStatus=PASS，也标 `CANDIDATE_DIAGNOSTIC / FORMAL_RELEASE_NOT_AUTHORIZED`；不可报告整个WS-08已通过。旧250ms数据仅 OBSERVED_SAMPLED_MAX；完整W及真实P等具备的旧账本也只能 RETROSPECTIVE_DIAGNOSTIC，不追认旧验收。

## 6. 不复用的部分与实现交接

| 复用/不复用 | 约束与责任 |
|---|---|
| 复用commit/key/ordinal/audit | 同一提交身份、失败审计、post-commit快照与原子回滚规则；不创建第二套峰值提交 |
| 复用elapsed/integrated/clock | elapsed原值、同run时钟适配、原积分累计器；不新增peak wall clock，不修改physicsTime语义 |
| 复用particles/逐粒/源账本 | 原canonical P含state2、同提交逐粒/事件闭合、IH/IL及基线；不以计数/渲染估质量 |
| 不复用endpoint方法身份 | peakField/Method/Metric/Protocol独立；不得写成endpoint revision升级或字段别名 |
| 不复用endpoint报告字段组 | `peakNonDepositedFraction`不是`windowPeakFraction`，`peakWindow*`不是endpoint末1s窗；只引用结果 |
| 不复用生效权限 | `peakProtocolEffective`独立于`endpointProtocolEffective`；只能manager分别确认 |
| 新增输入证据 | 输入适配/WaterSandField登记接受事件、共享order、子步消费快照；observation/store/worker只复制序列化 |
| checker后续实现 | 从原始v7重算、固定半开W、全子步覆盖、相等最早key、双算/诊断分离；按tester协议实施 |

任何未来实现都先完成 tester 对齐→manager验收，再由 developer 按登记落地。本文不发出实现解锁或真机排期。在线峰值累计本身 O(1)/成功提交；状态摘要可复用原扫描，逐粒时长属于既有可选诊断，不以本稿新增寿命标准。

## 7. 首次交付的待对齐登记（历史记录，当前结论以§9为准）

检索范围为工作区 `docs` 文件名含 peak/ws08 与现有endpoint协议；目前仅发现 designer设计/复盘及本文，无独立tester peak协议。因此以下为**设计稿 §5.3 → 本字段稿 → 待tester复核**，不是已完成协议对齐。tester卡已确认需要 `peakMethodId=peakNonDepositedFraction`；卡片中的工程输入不充当已发布协议。

| designer/任务拟议项 | 本稿唯一落点 | 结论/待对齐原因 |
|---|---|---|
| peak*三项方法身份 | §1原建议ProtocolId/Revision/MetricRevision；补peakFieldRevision、peakMethodId | 保留设计稿建议；待tester正式稿版本/hash逐值核对 |
| peakAuthority=PROPOSED（历史） | peakAuthority=APPROVED，引用本卡用户五项裁决 | 只是批准状态随任务事实更新；非字段语义变更；不自动生效 |
| peakProtocolEffective=false | 同名独立字段，当前false | 一致，不从endpoint状态继承 |
| on/off实际边界 | shakeOn/OffEventId + 原同名EffectiveElapsedMs | elapsed保持设计名；新增真实接受证据，待tester核对事件结构 |
| 子步输入消费/同ms全序 | context.peakConsumedShakeInputEventId/ShakeOnEventId/InputValue + 两处peakInputCommitOrder | 设计要求的工程具体化；全序不放宽半开窗，待tester对齐唯一名称 |
| 固定3s W/每成功提交 | peakWindowStart/EndElapsedMs + 原endpointSampleElapsedMs/ordinal | 一致，off晚不延长，off早资格不符；不借用末1s |
| key/phase/integrated | 原context四元key、endpointPhaseOccurrenceId、integratedPhysicsSeconds | 原路径原名引用，不给endpoint改名 |
| 主量、峰值key、质量/score/status | §5保留所有拟议同名字段，附同提交peakMasses | 一致；分档与0.5无新增容差；待tester确认报告结构 |
| legacy三组与同分母U/E | legacyPeakSuspReported / HistoricalReference / LiveFormula / sameReferenceSuspOnlyPeak | 一致，保留证据类型与历史分母无效独立状态 |
| 初态/响应/滞留 | baselineNonDepositedFraction/key、peakNetIncreaseFraction、cumulativeLiftGrantedMass、byState及原诊断 | 一致，诊断不足不造因果结论/寿命标准 |
| NOT_IMPLEMENTED/INCOMPLETE等 | §2与各字典的原因映射，多原因保留 | 不新定最终优先级；待tester协议确认 |
| 原checker全序列max定位 | 实际为water_sand_check.py:483；endpoint checker是末窗 | 源码定位差异已明示，待tester/manager核对适配清单；本稿不修checker |

未决项明确为：**正式tester文件/版本/hash尚未交付**、新增输入事件/消费字段的精确名称与结构、同ms不确定边界/合法保持状态的协议接纳及最终状态优先级。均标 PENDING_TESTER_PROTOCOL_ALIGNMENT，不假装无差异；任务允许此状态交付。若后续发现与已批准数学/窗口或tester正文矛盾，停止相应定案并反馈manager，不修改设计求对齐。

## 8. 首次交付验证记录与红线逐项自检（历史记录）

本次仅进行文档/源码对照，不跑物理、采集或宣称新方法验证。原输入文件核查 SHA256：

| 输入 | SHA256 |
|---|---|
| designer设计 | `2E06F66EB2F72E5E7770DBCF9821B518A11034BA40EBABCC27B4478FB26F53E4` |
| endpoint字段rev2 | `1A92B95FBF434FBF16E5BFEAC8542018813FB9B250B01B0F8F3AA4336CFFA8E4` |
| endpoint协议 | `FFE337D73A4EE78B21A48543025468238CB3607D68B386DE7B2097FBA5148837` |
| 终点裁决 | `C8A6DA3F99160640D639A2DD62E2641E1E3FA73F928C009CE5A7B766663E67AC` |

| 红线 | 本次自检 |
|---|---|
| 纯设计零代码 | 只新增本文；不改entry/或test/，不代developer实现 |
| 不用真机 | 无连接、安装、采集、排队占用 |
| 不改endpoint正文/版本 | 原文件不编辑；末1s每提交、实际off+8s、5%与1e-9及双算均引用原契约 |
| 不改已批准终点裁决 | 原文件逐字保留，不把peak窗/阈值并入终点 |
| 不改DUALFIELD estimand | CONS/BUD/LIFE/SRC/GES保持原样，无新增通过线 |
| 不改冻结指标 | 30°±2°、target-based t90、R²、5%原样；0.5/分档只登记本卡已批准迁移 |
| 不追认历史 | 旧route/build原结果/分数保留，补算只诊断，不能补造v7字段 |
| 不宣布peakProtocolEffective | 当前false，最终独立适用范围仍由manager确认 |
| 不改测试判据 | 无测试资产修改；未定义新基线≤5%、最小LIFT、寿命或时间容差 |
| 矛盾反馈/对齐诚实 | 当前无已发布tester稿可对齐；待对齐明列，源码路径差异登记反馈，不自改设计 |

交付范围是字段设计可审查，不是实现或测量完成。下一步为 tester正式协议到位后复核字段差异并交manager验收；由manager决定后续developer实施与独立生效流程。


## 9. rev2：T07 三项对齐定案（2026-10-25）

本节为当前实施登记，取代 §7 的待交付判断，不覆盖 rev1 历史。只修订本字段稿，不改 peak/endpoint 协议、不改实现和测试。

### 9.1 正式身份、hash 来源与独立边界

| 项 | 唯一结论 |
|---|---|
| 正式协议文件 | `docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md` |
| 已入库来源 | `4591b46165155e8f5f2839b51adeb4d67d8300c8` 的上述路径 |
| `peakProtocolSourceHash` | `3f043d404224b8290248d04ea600e5a7ac08b763625f28385d3c3063755ebfe9`（SHA-256，原始字节） |
| 哈希核验方法 | Python subprocess 原始 bytes 读取 `git show 4591b46:<path>` 后 SHA-256；与工作区 `Path.read_bytes()` SHA-256 完全相等，`git diff HEAD -- <path>` 无差异；不经 PowerShell 文本转码 |
| 卡片旧前缀差异 | 卡写 `e9929058`，与该 commit 的真实 blob 不符；以已入库 blob 和同字节工作区两次核验值为准。属来源元数据纠正，无协议正文修改；回执反馈 manager |
| `protocolId` → `peakProtocolId` | `p011-ws08-peak-nondeposited`；协议 §0 通用身份表映射到 §10 peak 报告字段，不保留第二消费名 |
| `protocolRevision` → `peakProtocolRevision` | `ws08-peak-nondeposited-v1`；同上 |
| `peakMethodId` / `peakMetricRevision` | `peakNonDepositedFraction` / `shake-peak-nondeposited-v1`，与 §0、§10一致 |
| `peakAuthority` / `peakProtocolEffective` | `APPROVED` / `false`；批准的是方法，生效仍归 manager |
| `peakFieldRevision` / `peakFieldAlignment` | `p011-peak-fields-v1` / `ALIGNED`；rev2是本文编辑修订，不另升级字段方法身份 |

两份文件各自独立：协议为口径/判定权威，本稿为协议 §0/§10 所委托的工程字段登记。协议已引用独立字段登记的交接，本稿反向精确引用其文件/commit/hash形成可追溯关系，不为增加链接改写冻结协议。endpoint 的方法、报告字段、末1s窗、5%、实际 off+8s、生效权限均不变；`peakProtocolEffective` 与 `endpointProtocolEffective` 不互相推导。

### 9.2 事件与消费字段唯一名称

协议 §2.1/§2.2要求实际接受、实际消费和同ms全序，但未重复列出事件12字段及消费5字段的具体 schema；§0/§10明确委托 architect 登记。因此不是“协议已有另一套名字”，而是本稿 §3 对其抽象契约的唯一工程落实，**沿用原字典，不另造名、不改类型**。下表逐项确认，未在协议逐字出现的字段不得冒称已逐字匹配。

| 唯一精确路径 | 类型（沿§3） | 对应协议要求/对齐结论 |
|---|---|---|
| `peakShakeInputEvents[].eventId` | string | §2.1接受身份；唯一接受事件ID |
| `peakShakeInputEvents[].runId` | string | §2.2同run关联 |
| `peakShakeInputEvents[].peakInputCommitOrder` | nonnegative safe integer | §2.1输入事件/提交全序；共享计数器 |
| `peakShakeInputEvents[].effectiveElapsedMs` | finite number≥0 | §2.1实际接受同钟elapsed；不是dispatch时间 |
| `peakShakeInputEvents[].endpointClockId` | string | §2.2引用原时钟域，非新peak时钟 |
| `peakShakeInputEvents[].value` | number∈[0,1] | §2.1实际输入；W内须为1 |
| `peakShakeInputEvents[].requestedValue` | finite number，可选 | 诊断扩展，不取代钳位后的实际value |
| `peakShakeInputEvents[].kind` | SHAKE_ON/OFF/UPDATE | §2.1 on/off边界；取值用§3完整枚举 SHAKE_ON/SHAKE_OFF/SHAKE_UPDATE |
| `peakShakeInputEvents[].shakeOnEventId` | string或null | §2.1激活根及固定窗绑定，不随同值refresh重开窗 |
| `peakShakeInputEvents[].caseOccurrenceId` | string | §2.2 case occurrence |
| `peakShakeInputEvents[].endpointPhaseOccurrenceId` | string | §2.2原phase occurrence关联 |
| `peakShakeInputEvents[].commandSequence` | safe integer | §2.1调度与实际接受可追溯 |
| `peakShakeInputEvents[].sourceKind` | SCHEDULE/TILT_DERIVED/MANUAL | 来源诊断扩展，不授权变更WS-08输入 |
| `context.peakFieldRevision` | string | §0/§10独立字段登记版本 |
| `context.peakInputCommitOrder` | nonnegative safe integer | §2.1提交全序，与事件同名同域 |
| `context.peakConsumedShakeInputEventId` | string | §2.1本子步实际消费事件 |
| `context.peakConsumedShakeOnEventId` | string或null | §2.1消费属于本次on激活根 |
| `context.peakConsumedShakeInputValue` | number∈[0,1] | §2.1本子步实际消费值，不取energy替代 |

计数澄清：原卡称“12字段”，§3.1实际列有13项（含可选requestedValue）；本次逐项保留13行，不能为凑12删除诊断或来源字段。消费侧5项与原卡一致；该数量差异是卡片摘要计数，不构成第二schema。`shakeOnEventId/shakeOffEventId/shakeOnEffectiveElapsedMs/shakeOffEffectiveElapsedMs` 继续沿§3边界指针；off elapsed复用原值。此计数及hash来源差异一并反馈manager。

### 9.3 同ms边界、合法保持及判定顺序

唯一接纳规则：先按同钟 elapsed 检查 `t_on<=sample<t_on+3000`，再以事件/提交 `peakInputCommitOrder` 与消费根确定归属。on前同ms提交排除；off后提交排除；右端恰等于 `t_on+3000` 一律排除。共享全序仅解决同毫秒先后，不把右端样本挪回窗口。

| 场景 | 协议条款 | 最终处理 |
|---|---|---|
| 必要输入事件/消费/全序字段能力未提供 | §2.3、§4.1步骤4、§4.2 | NOT_IMPLEMENTED，列精确missingFields；不以energy替代 |
| 字段齐备且无矛盾，但时钟分辨率/边界证据无法证明归属 | §2.2完整采样证据、§4.1步骤3/6、§4.2 | INCOMPLETE；不伪造精度、不增时间容差 |
| 非单调elapsed、跨子步拼接、重复提交键、消费身份/全序矛盾 | §2.3、§4.1步骤2/5、§4.2 | INVALID_LEDGER；不能降成普通覆盖不足 |
| 提前off、窗内非1、断档/提前退出、sourceEpoch等变化 | §2.1/§2.2、§4.2 | INCOMPLETE；不延长固定W求及格 |
| 合法休眠、有身份只读保持值、完整边界 | §2.2 | 允许作为保持证据，不伪造成功子步、不增加sampleCount；冻结/回滚/漏记不能充当休眠 |

**最终顺序逐字沿协议§4.1的七阶段，不发明状态码排序：**

1. 路由与协议身份校验（particleRouteActive/particleRouteId；routeId归属；无外源准入；基线E>0）。
2. 硬闸门：守恒/库存/容量/guard/预算/有限值/账本对齐/边身份，既有safety checker原样执行；失败先报闸门，整体不合格，峰值只留候选诊断。
3. 窗口绑定与资格：on/off边界、消费与连续性、覆盖。
4. 字段完整性：必要字段缺失→NOT_IMPLEMENTED。
5. 账本/基线/分母：坏账本→INVALID_LEDGER；初值身份不可证→INVALID_BASELINE；E≤0→INVALID_DENOMINATOR。
6. 窗口资格不足→INCOMPLETE。
7. 逐提交f(k)→max/最早键/分档/及格比较。

步骤1/3先做校验和绑定，所发现的缺项/基线/分母/覆盖问题保留原因并按步骤4–6出码，不拿概括性的“身份失败”覆盖其具体状态；硬闸门实际失败始终先报。多原因全保留，不能把六个状态码简单全局排序改变协议流程。同ms不确定的唯一问题在字段齐备、无更早硬失败/无效量时最终为INCOMPLETE。

### 9.4 与协议§10的最终逐字段对照

| 协议§10字段/要求（逐项） | 本稿唯一路径 | 差异结论 |
|---|---|---|
| peakProtocolId | 同名，§1/§9.1 | 值一致 |
| peakProtocolRevision | 同名，§1/§9.1 | 值一致 |
| peakMetricRevision | 同名，§1/§9.1 | 值一致 |
| peakAuthority | 同名APPROVED | 一致，不复用endpoint批准 |
| peakProtocolEffective | 同名false | 一致，非manager不得置true |
| route/build/params/input/hash/clock身份 | §4原证据包身份 | 原路径引用，无第二套别名 |
| peakWindowStartElapsedMs | 同名=t_on | 一致 |
| peakWindowEndElapsedMs | 同名=t_on+3000 | 一致，不取off替换 |
| shakeOnEffectiveElapsedMs | 同名及on事件指针 | 一致；事件schema由§3落实 |
| shakeOffEffectiveElapsedMs | 同名原off边界 | 一致，不重取时钟 |
| peakWindowSampleCount | 同名 | 一致，仅成功提交 |
| peakWindowCoverageStatus | 同名 | 一致，保持证据不冒充提交 |
| missingFields | 同名精确路径数组 | 一致 |
| 输入消费/阶段occurrence/提交序列 | §3.2消费5项、§4原key/phase/ordinal | 协议抽象要求具体化，唯一命名见§9.2 |
| integratedPhysicsSeconds | context.integratedPhysicsSeconds | 同原名，不取代elapsed |
| peakNonDepositedFraction | 同名 | 一致，max_W((U+P)/E) |
| peakStockMass | 同名 | 一致，同峰值提交M |
| peakReferenceMass | 同名 | 一致，同提交E |
| peakCommitKey | 同名 | 一致，max并列取最早 |
| peakSampleElapsedMs | 同名引用原elapsed | 一致 |
| peakScore | 同名 | 一致，原分档且M≥0.5E |
| peakStatus | 同名 | 一致，按§9.3出码 |
| 峰值D/U/P/E | peakMasses.depositedMass/suspendedMass/particleMass/expectedMass | 协议未给具体嵌套路径，由§5唯一登记，不复制顶层别名 |
| suspFraction | peakMasses.suspFraction=U/E | 显示名原义，沿本稿§5路径 |
| airborneFraction | peakMasses.airborneFraction=P/E | 同上，不限空气中 |
| depositedFraction | peakMasses.depositedFraction=D/E | 同上 |
| peakNonDepositedHeavyFraction | 同名 | 一致，E_H=0则N/A |
| peakNonDepositedLightFraction | 同名 | 一致，E_L=0则N/A |
| 总峰值提交H/L分项 | peakMasses.*Heavy/*Light | 同一提交，独立相峰不能相加 |
| legacyPeakSuspReported | 同名 | 一致，原窗口原密度原实测 |
| legacyPeakHistoricalReference | 同名 | 一致，同W逐子步U/(D+U) |
| legacyHistoricalReferenceStatus | 同名 | 一致，旧分母无效不阻断新量 |
| legacyPeakLiveFormula | 同名 | 一致，U/max(A,EPS)，无实测明确为重算 |
| sameReferenceSuspOnlyPeak | 同名 | 一致，独立max(U/E) |
| OBSERVED_SAMPLED_MAX | evidenceMode/legacyPeakEvidence.*.evidenceKind | 这是证据类别而非新数值字段；保留密度与分母 |
| baselineNonDepositedFraction及提交键 | 同名+baselineCommitKey | 一致，不设on前5%门槛 |
| peakNetIncreaseFraction | 同名 | 一致，诊断不替代主量 |
| cumulativeLiftGrantedMass | 同名 | 一致，不用requested代grant |
| guard/budget拒绝 | peakLiftRejectionEvidence | 诊断的唯一具体路径 |
| 峰值FLYING/CONTACT_WAIT及缺byState | peakParticleByState、particleResidueDiagnosticStatus | 一致，NOT_OBSERVABLE不伪造零 |
| hardGateStatus | 同名及hardGateFailures | 一致，安全失败不可被峰值掩盖 |
| endpoint独立结果引用 | endpointResultReference | 一致，不重算终点 |
| 自然度/无永久悬浮证据 | naturalnessEvidenceStatus、particleResidueDiagnosticStatus及原证据 | 一致，不由峰值推导全链 |

§5额外字段（peakFieldRevision/hash、峰值ordinal/order、覆盖定位、质量细项、积分和legacy证据等）为上述要求的工程/诊断具体化，无同名不同义，也不改变协议通过门槛。事件字典未列于tester正文的事实已明示，不把“无冲突”写成“tester逐字登记了所有扩展”。本次三项均有唯一结论，无待实施方任选的命名/边界/状态项。

### 9.5 收尾自检与交接

- 只更新本文；entry/、test/、peak协议、endpoint协议与终点裁决均未编辑。
- 分子含state2、分母E、固定3s、每成功子步、0.5/原分档、末1s/5%、双算及DUALFIELD estimand均原样。
- 未使用真机、未重评历史、未宣布任何protocol effective；不把文档对齐当实现/验证完成。
- 已登记真实blob hash及卡片13/12摘要计数差异，提交回执供manager验收。T08沿既定范围继续，新增协议正文修订仍归tester管理，不由本文回改。

修订：2026-10-25，rev2，T07最小收尾：正式协议身份/hash、事件与消费唯一字段、同ms不确定边界/七阶段优先顺序及§10逐项对照。历史§7/§8保留；当前结论以本节为准。
