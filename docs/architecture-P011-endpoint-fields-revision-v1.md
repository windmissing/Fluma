# P-011 endpoint 字段 revision v1

日期：2026-10-24。责任：architect。任务：6adc64ff…cb44（终点指标字段，仿真线·阶段二，manager 登记 P4）。

状态：**FIELD_DESIGN_READY / ALIGNED / PENDING_MANAGER_REVISION_ACCEPTANCE / NOT_IMPLEMENTED / NOT_VALIDATED**。当前修订：rev2（2026-10-24，卡6adc85fc…81cb）；字段名已依manager裁定完成R1–R3对齐，协议正式生效条件不变。

本稿只登记工程字段与采样接口，不修改产品代码、checker、tester协议/用例或冻结阈值。用户已批准口径，**不等于协议已生效或实现验收完成**。developer 卡6adc6513…db0的前置仍是tester protocol revision与本字段稿均就位；本稿不代manager解锁。

## 1. 权威输入、边界与命名状态

- [用户批准记录](decision-P011-terminal-nondeposited-fraction-v1.md)（15:56）：终点分子U+P、各用例历史分母、5%、末1.0s每已提交子步质量窗，以及新旧双算/旧报告不追认。
- [designer定稿](design-P011-terminal-nondeposited-fraction-v1.md) §7.2–7.4为数学与场景语义权威；§1–§6历史提案有不同版本名/摇晃资格，不能混用。
- [路线C架构rev1](architecture-P011-dual-field-particle-v1.md) §5.6/§13：canonical粒子库存、部分LAND余量、post-commit账本、route门、原子回滚。
- [DUALFIELD预注册](preregistration-P011-dual-field-particle-v1.md)：16例estimand原样，本字段扩展不改CONS/BUD/LIFE/SRC/GES。
- [tester endpoint protocol](endpoint-protocol-P011-terminal-nondeposited-v1.md) §0/§3.2/§5.3/§6/§11（首次交付commit `69e27b9`）及[manager字段对齐裁定](decision-P011-endpoint-field-alignment-v1.md) §1–§4：本次R1–R3与tester T1–T3的唯一依据。
- `priority.md` 15:56/16:12：tester卡6adc64e7…bc1先发布endpoint protocol revision，architect对齐字段，developer实现后才出双报告。

**历史核查记录（保留原诚实声明，R2）**：本稿16:24首次交付时，工作区尚未发现tester endpoint protocol revision落盘，因此当时将protocolId/revision留null、endpointFieldAlignment记PENDING_TESTER_PROTOCOL，未声称ALIGNED。tester稿随后于16:26落盘（首次commit `69e27b9`）；manager18:05出具字段对齐裁定。上述“尚未落盘/不得登记ALIGNED”仅描述16:24时点，不再是当前前置状态。

**当前对齐（R1–R3）**：已核对tester稿§0身份与T1–T3修订记录：`endpointProtocolId=p011-endpoint-terminal-nondeposited`、`endpointProtocolRevision=endpoint-terminal-nondeposited-v1`、`endpointFieldAlignment=ALIGNED`。三处唯一落点为 `context.endpointPhaseOccurrenceId`、`legacyEndpointFraction`、`legacyMetricSuspensionFraction`。证据=[manager裁定](decision-P011-endpoint-field-alignment-v1.md)+tester首版commit `69e27b9`+当前tester稿§11/§13改名记录。ALIGNED仅指字段契约对齐，不能凭其签发正式终点PASS；后续命名变更仍须版本化登记，不由developer选名。

本次对齐读取文件SHA256：tester当前稿 `FFE337D73A4EE78B21A48543025468238CB3607D68B386DE7B2097FBA5148837`；manager字段裁定 `07B87A352C805EBF20DF05A9B1DA267DF510460D354BEF909A19C3093EB0ED16`。`69e27b9`只标识首次交付；本hash另覆盖后续T1–T3改名工作区内容，不将首版commit冒充已提交的修订版本。后续正式证据包须重新记录实际文件hash与提交状态。

版本相互独立：

| 身份字段 | 本稿登记值/规则 | 不可推导的结论 |
|---|---|---|
| `ledgerSchemaVersion` | 延续路线C schema v7，字段作可选扩展，原v6历史可解析 | v7不表示endpoint协议生效 |
| `endpointFieldRevision` | `p011-endpoint-fields-v1` | 不代表实现或测量通过 |
| `endpointMetricRevision` | `terminal-nondeposited-v1.1`，对应designer §7定稿及用户批准 | 不使用历史§4的v1字符串混名 |
| `endpointProtocolId` | `p011-endpoint-terminal-nondeposited`（tester §0） | 不以任务ID/本文版本替代 |
| `endpointProtocolRevision` | `endpoint-terminal-nondeposited-v1`（tester §0） | 不等于物理实现revision |
| `endpointAuthority` | `APPROVED`（用户口径裁决） | 不等于正式启用 |
| `endpointProtocolEffective` | false，只有manager确认适用范围后按其记录登记true | 不能从schema或代码上线自动推导 |
| `endpointFieldAlignment` | `ALIGNED`；证据=manager对齐裁定+tester首版commit `69e27b9`+当前稿改名记录及导出hash | 不等于已实现、已验证或正式放行 |

## 2. post-commit采样身份与数据路径

### 2.1 账本扩展字段的唯一路径

沿用 `record.context/stateAfter/particles/particleEvents/audit/budgets`。新增字段放在以下位置，不在每个工具里各起别名：

| 精确路径 | 类型/单位 | 写入时点与语义 |
|---|---|---|
| `context.endpointFieldRevision` | string | 当前writer实际支持的字段revision |
| `context.endpointSampleStage` | `POST_COMMIT`或null | 仅成功物理提交为POST_COMMIT；blocked/rolled_back/rejected为null |
| `context.endpointSampleElapsedMs` | number或null，ms | 成功提交后同run起点的单调实际elapsed，见§3 |
| `context.integratedPhysicsSeconds` | number，s | 同run所有成功提交物理子步physicsDt之和；本子步只加一次 |
| `context.endpointClockId` | string | 与run manifest一致的时钟域标识 |
| `context.endpointPhaseOccurrenceId` | string或null | 同名相位每次出现的唯一schedule身份，不能用phaseD字符串充当 |
| `context.endpointCommittedOrdinal` | 非负安全整数或null | 仅成功提交递增1；拒绝/回滚没有新ordinal；用于漏样核查 |
| `context.sourceInjectedH/sourceInjectedL/sourceEpoch` | 原名、原单位 | endpoint采样携带的是**同一提交后的**累计准入与epoch，不拼begin时旧值 |
| `particles.byState.FLYING.{count,massH,massL}` | 整数/unit | 当前pState=1存量，成功提交后统计 |
| `particles.byState.CONTACT_WAIT.{count,massH,massL}` | 整数/unit | 当前pState=2存量，含部分LAND余量 |

原 `(context.runId,sequence,frameId,substepId)` 为提交关联键，另保留已有attemptId/commitStatus/errorCode与commandSequence/commandPhaseId/legId/segmentId。`endpointPhaseOccurrenceId`由schedule manifest显式提供并关联commandSequence/segmentId，不从图形变化事后切段；手动/无可核相位身份时为null并记INCOMPLETE。不得把不同run、重复同名phase或不同sourceEpoch拼成末窗。

新增context字段可选是为兼容历史文件；在新revision的C正式采样中除允许null的状态场景外均必需。字段缺失不能默认0；失败记录上的null不是“零时刻/零粒子”。writer不能把原begin-time的 `physicsTime` 改成post-commit时钟，也不要求context/metric各自physicsTime相等。

### 2.2 原子采样接口（待developer实现）

```ts
interface P011EndpointClock {
  clockId: string;
  nowElapsedMs(): number; // 同run、同schedule、单调实际elapsed
}
interface P011EndpointCommitKey {
  runId: string; sequence: number; frameId: number; substepId: number;
  attemptId: string | null; committedOrdinal: number;
}
interface P011EndpointPhaseIdentity {
  occurrenceId: string; commandSequence: number;
  commandPhaseId: number; segmentId: string | null;
}
interface P011EndpointPostCommitStamp {
  key: P011EndpointCommitKey;
  endpointSampleElapsedMs: number;
  integratedPhysicsSeconds: number;
  endpointClockId: string;
  phase: P011EndpointPhaseIdentity;
}
// WaterSandField/observation组装层内部接口，不新增物理输入通道。
stampEndpointPostCommit(key: P011EndpointCommitKey, physicsDt: number,
  phase: P011EndpointPhaseIdentity, clock: P011EndpointClock): P011EndpointPostCommitStamp;
```

实际提交顺序：全部事务/预算/guard完成→物理audit通过→同一同步栈锁定本次commit key→复制stateAfter、粒子摘要/byState、post-commit source账本→integrated累计一次→读取elapsed一次→封装最终record并发布。所有快照在下一物理事务/下一子步开始前取完，不能延到异步worker/250ms metric回调再读live池。发布后worker只接不可变值/独立数组，禁止引用下一子步会复写的缓冲。

当前 `finishNaturalObservation(null)` 位于step成功分支、早于updateMetrics/旧p011SubstepId最终递增；developer必须在此成功封装点绑定“本次即将提交”的key并只增新ordinal一次，不把旧substepId和下一帧particles串接。失败分支调用finishNaturalObservation(error)记录attempt后才restoreBackup：这类record**不能**带POST_COMMIT stamp进入endpoint窗。失败attempt仍用于硬闸门与完整性审计，不能被静默过滤后报告PASS。

采样源只认真实canonical state：D来自stateAfter的bh/bl/mh/ml，U来自同record的suspH/suspL，P来自同record粒子池。M1池只提供只读snapshot/摘要；M2 WaterSandField负责确定成功提交与ledger身份；页面提供schedule身份和时钟适配，不能根据renderer点数/像素估质量。T1/T2/LAND/MERGE/DEGRADE均遵守rev1原子边界，不能将粒子提交前的缓存与提交后的床层组合。

## 3. elapsed、积分时长与固定边界

### 3.1 运行时钟

run manifest记录 `endpointClockId/endpointClockApi/endpointClockUnit/endpointClockResolutionMs/endpointClockOrigin/runStartClockMs`。`endpointClockId=runId+":startup-elapsed-v1"`、`endpointClockUnit="ms"`、`endpointClockOrigin="DEVICE_STARTUP_MINUS_RUN_START_V1"`，runStartClockMs为该设备同一次启动的实际uptime原始读数；resolution为所用API实际可证明的分辨率，不能自行填高精度。run起点只设一次；phase切换不重置elapsed。设备重启、时钟域变化或run reset必须开新runId，不能强行+1补成单调序列。

本地SDK `26.0.0/ets/api/@ohos.systemDateTime.d.ts` 已核实 `systemDateTime.getUptime(TimeType.STARTUP,false):number` 为同步ms，包含深睡。设计采用此API的实际读数减runStartClockMs；同一run的schedule起点、命令接受、生效时间、phase边界和commit采样均经同一adapter取数。与现有Date.now驱动的旧schedule路径隔离，**仅新endpoint revision的采集run启用**；不改既有历史时间戳定义或以强行max(last+1,Date.now)冒充真实elapsed。

如schedule仍以另一时钟发令，必须提供经tester协议认可的显式映射及校验；没有映射不能直接拿设备uptime减主机time.monotonic或墙钟，判INCOMPLETE。时钟API不可用返回采样不可用，不静默退回Date.now。深睡/后台造成的空窗仍要检查覆盖与输入连续性，STARTUP继续走时不表示采集有效。

同ms内多次提交允许elapsed相等（非递减），顺序由committedOrdinal与commit key决定，禁止人造递增时间。实际时钟分辨率如不足以区分固定边界两侧的状态，保留原始读数与顺序并报边界不确定；由tester协议明确接纳口径，不自行给时间加容差。不得将既有 `physicsTime/physicsTimeMs/physicsDt` 改名为endpoint elapsed：physicsTimeMs当前还是计算耗时字段。

### 3.2 integratedPhysicsSeconds

`integratedPhysicsSeconds(k)=Σ physicsDt(j)`，只求本run成功提交的真实物理子步，含本次j=k。单位秒，逐子步计算；一次frame两substep要加两次，clipped dt只加实际积分量，不能把原输入frameDelta算进去。blocked/rolled_back/rejected、dormant只读复核、日志重试/重复导出都不加。回滚必须连累计器一起回滚，reset开新run归0。

该字段仅诊断积分推进与实际elapsed的差异；不能拿它选1s/8s边界，也不能凭累计1秒证明真实观察1秒。report同时披露elapsed跨度、积分跨度、成功substep数、失败attempt数和缺失序号。不要按60Hz假定“60条=1s”。

### 3.3 边界清单与采集覆盖

run schedule manifest为每个occurrence登记 `phaseStartElapsedMs/phaseEndElapsedMs/endpointBoundaryElapsedMs/windowStartElapsedMs/windowEndElapsedMs`，末窗固定为 `[t_e-1000,t_e]`。不能以最后一条日志时间反推t_e。

- SLIP-03：t_e为预注册phase occurrence固定结束边界，endpoint样本是该边界及之前最后一个成功提交；R_SLIP整run冻结。保留原tilt/shake耦合，**不加shake=0条件**。
- WS-08：记录实际输入生效的 `shakeOffEffectiveElapsedMs`，固定t_e=该值+8000；外部shake输入末窗为0，shakeEnergy允许按原动力学衰减，不要求浮点0。计划dispatch时点与实际生效时点分别记，不能混用。
- WS-09：保留原10s/末5s schedule，是否适用新末1s和正式判定以tester revision的case清单为准；本文不把它改成8s，不扩适用范围。
- DUALFIELD-GES：不新增通用5%门槛；只可在另有预注册stop/观察窗时附诊断。

需要一条覆盖windowStart之前或恰好该时刻的已提交状态作为left anchor，以及同钟的开始/固定结束边界证据；窗口内每笔提交必须连续可核。anchor不冒充新提交或改变闭区间成员。若anchor状态延续进窗口，其是否作为边界保持状态参与比较由tester协议明确；未定时报告INCOMPLETE，不能采样稀疏后自行插值为PASS。

合法dormant没有新提交时，允许另发**只读覆盖证据**，不伪造substep：`kind=DORMANT_STATE_RECHECK`、`runId,endpointClockId,observedElapsedMs,heldCommitKey,heldCommittedOrdinal,heldStateHash,sourceEpoch,phaseOccurrenceId,modelDormant,modelFrozen,lastErrorCode,inputSequence,routeId`。hash覆盖床/susp/粒子及源账本。它只证明某成功状态在该实际时点仍保持；不能增加ordinal/integratedPhysicsSeconds，不能把emitP011NoopTelemetry人为递增的旧substepId当作物理提交。冻结/失败/活动源/输入身份不清不构成合法休眠覆盖。checker按tester协议把保持区间与提交记录连起来，不以250ms WS_METRIC替代全子步账本。

## 4. particles.byState 与滞留诊断

```ts
interface P011ParticleStateMass {
  count: number; massH: number; massL: number;
}
interface P011ParticleByState {
  FLYING: P011ParticleStateMass;
  CONTACT_WAIT: P011ParticleStateMass;
}
// 挂在已存在的particles摘要下：particles.byState?: P011ParticleByState
```

FLYING精确对应pState=1，包含水内/空气中任何速度的飞行粒；CONTACT_WAIT精确对应pState=2，不因贴床/速度0/超时/池满而排除。state=0空槽不计。统计按pMass当前值分相，不用massPackage×count估计；MERGE后只计幸存包；LAND后的正余量仍计CONTACT_WAIT；DEGRADE后质量只在U中，不把degraded累计值加进byState/P。

每笔成功提交检查：两状态count之和=particles.count，两状态分相质量之和=particles.massH/massL，并与逐粒求和/列缓存闭合（使用既有冻结质量容差1e-9，不另定宽松线）；bySourceKind是同一批粒子的另一分组，不可与byState再相加。负质量、非有限、未知活状态或不闭合记INVALID_LEDGER。

byState是诊断扩展，**不改变P的估计对象**。若已有可信particles总质量而byState缺失，质量窗可独立计算候选数值，`particleResidueDiagnosticStatus=NOT_OBSERVABLE`，不能声称长时滞留已排除；本字段writer若声称实现该revision却漏字段，应明确缺失清单，不补零。若连C的particles摘要也缺失则终点NOT_IMPLEMENTED，不能假定P=0。

byState聚合量不能证明“同一个粒子永久不落”。可选分片诊断文件使用精确字段 `runId,committedOrdinal,particleId,sourceId,sourceKind,state,stateEnteredElapsedMs,firstObservedElapsedMs,lastObservedElapsedMs,mass,phase,landRequested,landGranted,guardRejected,budgetClipped`；`stateEnteredElapsedMs`必须在真实状态转换时登记，历史未知为null，不由首次截图倒填。MERGE保留mergedIds/cohort lineage，不能把新幸存id当成旧粒已落床。

报告保留 `particleResidueDiagnosticStatus=OBSERVED|NOT_OBSERVABLE`、`particleResidueDiagnosticCodes[]`（观测到同粒持续滞留时包含 `PERSISTENT_PARTICLE_RESIDUE`）、`particleResidueEvidencePaths[]`，并披露实际观察区间/最长可证持续时长。没有新批准的寿命阈值；末1s存在CONTACT_WAIT只足以说“该窗有待准入质量”，不能直接断言数学上的永久。无论数值是否≤5%，持续滞留证据交manager独立审查，不能以质量PASS替代生命周期出口。

## 5. 三分项定名、正式原始质量与双算

| 输出/计算字段 | 定义 | 来源与用途 |
|---|---|---|
| `depositedMass`（D） | Σ(bh+bl+mh+ml) | 同record.stateAfter，真实床层 |
| `suspendedMass`（U） | Σ(suspH+suspL) | 同record.stateAfter |
| `particleMass`（P） | particles.massH+massL | 当前所有state1/2库存 |
| `nondepositedMass`（M） | U+P | 正式候选分子，不含累计事件 |
| `expectedMass`（E） | VH0+VL0+IH+IL | 原始reset基线+同提交累计准入一次 |
| `actualMass`（A） | D+U+P | 校验与旧总量公式，不作新展示分母替代 |
| `suspFraction` | U/E | 新展示分项1，比例未乘100 |
| `airborneFraction` | P/E | 新展示分项2，不是“在空气中”的子集 |
| `depositedFraction` | D/E | 新展示分项3，不用D/A偷换 |
| 旧 `WS_METRIC.suspensionFraction` | 原 `suspended/max(totals.heavy+totals.light,EPS)`，C计入canonical P后为U/max(A,EPS) | 原字段原公式保留；不是suspFraction的别名 |

新三分项可加到 `WaterSandMetrics/WS_METRIC`，同时携带sourceEpoch、metric commit key与endpointFieldRevision，用于展示；正式checker仍从v7原始质量重算。不要用250ms metric值选择endpoint，也不要用显示舍入百分比比较阈值。E≤0时新三分项为null+INVALID_DENOMINATOR，不借用旧公式的EPS得到0/PASS。

VH0/VL0从reset后任何准入前的首条 `audit.heavyTotalBefore/lightTotalBefore` 取，manifest记录 `initialBaselineCommitKey/initialBaselineStateHash`。R_SLIP从run首子步stateBefore的Σ(bh+bl)取，另记baseline key/hash；不得加mh/ml，不以E代替，不在每phase重冻。采集起点已在注入后且无法证明原基线则INVALID_BASELINE。

新公式按case选择 `endpointReferenceKind=SLIP_INITIAL_BH_BL|EXPECTED_TOTAL`、`endpointReferenceMass=R`：SLIP为R_SLIP，WS为每提交E；`endpointFraction=M(endpoint)/R(endpoint)`，`windowPeakFraction=max M(k)/R(k)`。真正质量闸门逐提交比较 **M(k)≤0.05R(k)+1e-9 unit**，而不是先将比例四舍五入。即使报告fraction略大于0.05，仍须保留质量绝对容差的实际比较结果和gap，不能另增比例ε。

**同run旧/新双算至少两组，不能混成一个legacy字段：**

1. **历史分母语义复算（manager采纳tester §5.3）**：`legacyEndpointFraction=U/R_legacy`；SLIP的 `R_legacy=R_SLIP`，WS的 `R_legacy=A_hist=D+U`。它保留各case历史分母含义，不把新口径WS的E偷换进legacy。报告 `legacyEndpointReferenceKind=SLIP_INITIAL_BH_BL|HISTORICAL_DEPOSIT_PLUS_SUSP`、`legacyEndpointReferenceMass=R_legacy`、legacy原PASS/FAIL与`legacy-susp-only`身份。旧终点数值比较仍按历史方法，不给历史PASS追加新末1s要求。
2. **现行代码路径实测**：`legacyMetricSuspensionFraction=U/max(A,EPS)`，A=D+U+P；记录实际metric值与同提交重算值及差异，缺真实metric证据不能伪称实测。它不等于WS历史U/(D+U)，也不等于SLIP的U/R_SLIP。三分项和约为1只作守恒诊断，不用M/E替代SLIP正式闸门。

**双算差值的适用分母**：原v1将 `endpointFractionDelta=P/R` 无条件接在legacyEndpointFraction后，在WS历史R_legacy=D+U而新R=E时不成立。现精确定义 `endpointFractionDelta=endpointFraction-legacyEndpointFraction`；仅当新旧分母同值时才等于P/R（SLIP满足，WS须另核分母）。新分子同分母恒等式仍可另报 `sameReferenceSuspOnlyFraction=U/R_new`、`sameReferenceParticleContribution=P/R_new`，不把它们冒名legacy。此澄清落实manager已批准的分母分工，不改新判式M/R、5%或末窗。

manager采纳的“C run中两个legacy量因第三桶而不同”按实际数值条件披露，不是双算失败：WS在U>0、P>0时U/(D+U)与U/(D+U+P)严格不同；U=0或P=0可能相同，SLIP另受其历史R_SLIP影响。数值偶合不允许互为别名，两个字段始终分列、不得任一单独放行。保留tester §5.3的核心结论及其边界说明，不借改名改变估计对象。

报告 `legacyEndpointResult`、`revisedEndpointResult`与 `endpointWindowStatus` 分列；同run双算不是重写旧文件/结论。协议尚未正式生效时只能报告获批口径的计算与诊断，不以任何单一口径放行C。历史补算标 `evidenceMode=RETROSPECTIVE_DIAGNOSTIC`，需要真实v7粒子/时钟身份；旧OFF/B数学对照的P=0明确标 `particleEvidenceKind=STRUCTURAL_ZERO_OFF_OR_B`，C必须 `MEASURED_CANONICAL`，不得向旧文件补造摘要或byState。

本次不改WS-08 peakSusp≥0.5，不修沉降t90实现（另卡6adc6412…beac），不更改OLS/target-based t90/R²/SLIP-02阶段适用性，不对GES另设5% PASS。整项WS-08结果不能仅凭残余子项PASS推出。

## 6. 报告字段清单（developer与checker交接）

以下字段为每run/每case occurrence的结构化报告契约；不存在的证据显式null并列missingFields，不用空串/0掩盖。

| 组 | 必须字段名 |
|---|---|
| 文档/启用身份 | `endpointFieldRevision,endpointMetricRevision,endpointProtocolId,endpointProtocolRevision,endpointAuthority,endpointProtocolEffective,endpointFieldAlignment,protocolSourceHash,evidenceMode` |
| 采集身份 | `runId,caseId,routeId,particleRouteActive,particleEvidenceKind,buildCommit,sourceTreeHash,packageHash,ledgerPath,ledgerHash,scheduleId,scheduleHash,renderSeed,inputScriptHash` |
| 阶段/源身份 | `endpointPhaseOccurrenceId,commandPhaseId,segmentId,sourceEpoch,sourceInjectedH,sourceInjectedL,resetCount,invertCount,routeChangeCount,modeChangeCount,externalAdmissionCount`（各事件同时保留time/key的证据列表） |
| 时钟/固定边界 | `endpointClockId,endpointClockApi,endpointClockResolutionMs,runStartClockMs,phaseStartElapsedMs,phaseEndElapsedMs,shakeOffEffectiveElapsedMs,endpointBoundaryElapsedMs,windowStartElapsedMs,windowEndElapsedMs` |
| 实际末样本 | `endpointCommitKey,endpointCommittedOrdinal,endpointSampleElapsedMs,integratedPhysicsSeconds,endpointSampleStage,endpointBoundaryLagMs`（lag=t_e-实际末样本elapsed，不自定PASS容差） |
| 初始基线 | `initialBaselineCommitKey,initialBaselineStateHash,VH0,VL0,R_SLIP,endpointReferenceKind,endpointReferenceMass` |
| 原始质量 | `depositedMass,suspendedMass,particleMass,nondepositedMass,expectedMass,actualMass`；分相精确命名为 `depositedMassHeavy,depositedMassLight,suspendedMassHeavy,suspendedMassLight,particleMassHeavy,particleMassLight,nondepositedMassHeavy,nondepositedMassLight,expectedMassHeavy,expectedMassLight,actualMassHeavy,actualMassLight,conservationErrorHeavy,conservationErrorLight` |
| 展示与双算（R3唯一落点） | `suspFraction,airborneFraction,depositedFraction,legacyEndpointFraction`（SLIP U/R_SLIP、WS U/(D+U)）、`legacyMetricSuspensionFraction`（U/max(A,EPS)，现行路径实测）、`legacyEndpointReferenceKind,legacyEndpointReferenceMass,legacyEndpointResult,legacyReportIdentity,endpointFraction,endpointFractionDelta,windowPeakFraction`；两legacy字段禁止互为别名或单独放行 |
| 质量闸门 | `endpointThresholdFraction=0.05,endpointEpsMass=1e-9,endpointMassLimit,endpointMassGap,windowWorstCommitKey,windowWorstMassGap,legacyEndpointResult,revisedEndpointResult,endpointStatus,endpointWindowStatus,hardGateStatus,hardGateFailures` |
| 窗口覆盖/资格 | `windowCommittedCount,windowFirstCommitKey,windowLastCommitKey,windowCommittedOrdinals,missingCommittedOrdinals,duplicateCommitKeys,failedAttemptCount,coverageStartElapsedMs,coverageEndElapsedMs,boundaryEvidencePaths,dormantRecheckPaths,inputEligibilityStatus,inputEligibilityReasons` |
| byState/滞留 | `particles.byState.FLYING/CONTACT_WAIT.{count,massH,massL},particleResidueDiagnosticStatus,particleResidueDiagnosticCodes,particleResidueEvidencePaths,residueObservedDurationMs` |
| 适用范围 | `otherCaseGatesStatus,legacyReportIdentity,managerApplicabilityDecision,effectReportPath,objectiveReportPath,missingFields` |

`endpointMassGap=M-0.05R-1e-9`，正数超限；`windowWorstMassGap=max_k(M(k)-0.05R(k)-1e-9)`，其key不一定等于最大fraction的key（分母可能随允许场景变化），因此两项都报。质量恒等式用未舍入原值；不用negative gap替代其它硬闸门。原始v7 ledger保存最高精度数值，CSV/Markdown是报告视图。

状态名沿批准语义：`endpointStatus/endpointWindowStatus`使用 `PASS|FAIL|NOT_IMPLEMENTED|INVALID_LEDGER|INVALID_BASELINE|INCOMPLETE|INVALID_DENOMINATOR`；BY_STATE诊断另用OBSERVED/NOT_OBSERVABLE，不能挤进质量PASS字段。缺必要质量字段NOT_IMPLEMENTED，非有限/不守恒/重复伪提交INVALID_LEDGER，初值身份无效INVALID_BASELINE，时钟/覆盖/资格不足INCOMPLETE，R非正INVALID_DENOMINATOR；都不得PASS。多问题并存保存 `hardGateFailures/missingFields/inputEligibilityReasons` 全清单，状态优先级及最终联合case判定以tester protocol为准，architect不自行裁掉失败原因。

`PERSISTENT_PARTICLE_RESIDUE`为诊断码，不是新寿命阈值；数值PASS与该码可同时出现，overall出口仍需manager独立审查。`endpointProtocolEffective=false`时，即便内部数值revisedEndpointResult=PASS，报告须标 `FORMAL_RELEASE_NOT_AUTHORIZED` 于适用范围说明，不把它当正式case PASS。

## 7. 实现面、风险、验证与对齐表

| 责任面 | 待实施内容 | 边界 |
|---|---|---|
| M1 `P011ParticleField.ets` | 只读byState聚合/必要状态转换诊断、snapshot含pState | 不改粒子质量/预算/路由，以诊断名义过滤state2禁止 |
| M2 `WaterSandField.ets` | 成功提交stamp、源账本同子步、积分累计器及回滚、三分项输出 | 不动旧physicsTime或旧suspensionFraction公式 |
| observation `P011NaturalObservation.ets`/store/worker | context可选扩展、particles.byState、commit事件序列/不可变序列化 | worker不异步回读live物理状态；失败attempt仍落账 |
| 页面/schedule/采集适配 | 同钟run/命令生效/固定边界身份、只读dormant复核 | 只新revision采集run接入，旧schedule不追溯改语义 |
| checker/报告（developer按tester契约） | 原始v7重算、历史R与E分开、质量窗、双算、状态与missingFields | 不改tester estimand或peakSusp；不以summary metric充当账本 |

关键风险：时钟混域或伪递增会污染1s/8s窗；finishNaturalObservation早于旧substepId递增会串样；begin的sourceEpoch与after质量错配会重复IH/IL；250ms下采样会漏掉窗内峰值；byState总量不能证明同粒永久滞留；只读no-op不能冒充新提交。各风险均由本稿的clockId/commit key/post-commit/ordinal/原始ledger/逐粒诊断字段暴露，不能靠事后换窗口掩盖。

单次byState统计O(Nmax)，可与已有粒子摘要扫描合并；endpoint采样每成功子步固定标量与现有snapshot一起发布，不复制额外完整场、不为诊断引入物理循环。逐粒cohort导出仅诊断run分片执行，帧时统计保留诊断开关身份，不拿开启日志的设备耗时冒充模型耗时。Node时钟由测试夹具注入可控单调实际elapsed；只能验证采样/窗口逻辑，不能宣称真机时序或长稳已验证。

实现验收准备清单（未执行）：同frame双substep不串样；失败回滚积分/ordinal不增长；相同ms按commit序；elapsed8s但积分不足不偷换；重复phase不拼窗；SLIP的R与E不同反例；部分LAND余量计P；MERGE/DEGRADE不改M；state2四个百分点与数值PASS并列诊断；缺摘要/缺byState/缺末端/clock混域分别分类；合法dormant与冻结/缺日志区分；旧字段公式与OFF/B历史数据保真。用例及期望值仍由tester protocol冻结。

字段对齐表：

| 批准/上游原名 | 本稿唯一落点 | 当前核查 |
|---|---|---|
| endpointSampleElapsedMs | context.endpointSampleElapsedMs→report同名 | 与decision §3/designer §7一致 |
| integratedPhysicsSeconds | context.integratedPhysicsSeconds→report同名 | 与decision §3一致，诊断不作时间窗 |
| particles.byState.FLYING/CONTACT_WAIT | record.particles.byState同名 | 与decision §3/designer §7一致 |
| suspFraction/airborneFraction/depositedFraction | 新metrics及report同名 | 与decision附加规定一致，共E |
| endpointReferenceKind/endpointReferenceMass/endpointFraction/windowPeakFraction | checker/report同名 | 与designer §7.4一致 |
| context.endpointPhaseOccurrenceId | context.endpointPhaseOccurrenceId | ALIGNED；manager §1 A1，tester §3.2/§6.2/§11 |
| legacyEndpointFraction | checker/report同名；SLIP U/R_SLIP、WS U/(D+U) | ALIGNED；manager §1 A2/§2，tester §5.3/§6.1 |
| legacyMetricSuspensionFraction | checker/report同名；U/max(A,EPS)，A=D+U+P | ALIGNED；manager §1 A3，tester §5.3/§6.1 |
| tester endpoint protocol revision | endpointProtocolId=`p011-endpoint-terminal-nondeposited`；endpointProtocolRevision=`endpoint-terminal-nondeposited-v1` | **ALIGNED**；manager裁定+tester首次commit `69e27b9`+当前稿§11/§13；case/状态/覆盖按tester冻结正文，不再自定 |

## 8. 变更记录

- 2026-10-24，rev2（6adc85fc…81cb），依manager字段对齐裁定执行R1–R3：§1/§7补tester protocolId/revision与ALIGNED证据；16:24未落盘声明改为历史陈述并保留16:26/18:05事实链；§6明确两个legacy唯一名字、公式、分母与历史身份。同步修正§5将WS历史复算误写成新E分母以及无条件P/R差值的说明，采用已裁定的tester §5.3分工。字段标识`p011-endpoint-fields-v1`不变（本次文书修正，不是新估计对象），源码/checker/用例零改动，正式协议生效条件不变。

- 2026-10-24，v1：按endpoint字段卡与已批准终点口径，登记post-commit关联键/采样点、elapsed与积分诊断、byState与滞留证据、三分项和双算、完整报告清单；给出本地SDK时钟接口依据与新revision隔离接入。tester endpoint protocol尚未落盘，明确字段对齐前置；纯设计零代码，不改任何冻结估计对象、不授权实施或正式放行。


## 9. O-2 外源准入计数定义与协议一致性提案（T04，2026-10-25）

责任：architect。状态：**ARCHITECT_ANALYSIS_COMPLETE / MANAGER_DECISION_REQUIRED / O2_NOT_IMPLEMENTED**。
这是原T04的口径分析交付，不是整卡O-2代码完成；D-4由a5e197f已完成，不重做；D-2/D-6由93a9922另卡完成，不混入。本节为唯一推荐，待manager作口径裁决后developer实施；不回改已发布endpoint/peak协议、既往双报告或冻结判据。

### 9.1 当前源码与权威边界

2026-10-25只读事实：

- `WaterSandField.injectGestureSand`只在最终audit成功分支把实际准入量加到sourceInjectedHeavy/Light，并将sourceEpoch加1；一个请求可含多个列/两相，仍只加1。lambda重试在其之前，不是准入次数。当前唯一`sourceEpoch += 1`即该分支；场reset同时将IH/IL/epoch归0并建立新run。
- `P011NaturalObservation.ets`明确注释externalAdmissionCount是成功外源准入事务次数；但`finishNaturalObservation`传给stamp的第7/8实参都来自this.sourceEpoch。没有独立count状态或成功receipt重算来源，不能由重复字段证明完整性。
- 当前checker要求externalAdmissionCount非负整数（排除bool），缺失记missingFields，非法值记INVALID_EXTERNAL_ADMISSION_COUNT，与sourceEpoch不等记EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH硬失败；D-4报告已从selected.context回填。
- [正式endpoint协议](endpoint-protocol-P011-terminal-nondeposited-v1.md)用sourceEpoch恒定识别末窗无源；本字段稿§6要求externalAdmissionCount随time/key证据，但此前未给它新的独立量纲。协议没有授权把其改为质量/按列计数/拒绝请求次数。补充报告观察O-2记录复制来源问题，不能据此推导“合法count必须不同”。

原卡同时要求“合法有外源run里count与epoch不等”和“二者不一致即硬失败”，这两项不能在相同定义域同时作为正常运行要求。**独立取证来源不等于不同估计对象，也不等于健康数据必须数值不同。** manager已有评论要求不得制造不一致；本节把冲突转成明确裁决项，不由architect改checker测试判据。

### 9.2 唯一推荐计数对象（沿当前count整数语义）

```text
externalAdmissionCount(run, resetEpoch, t)
  = 到t为止、该run/reset范围内已成功原子提交且admittedH+admittedL>0
    的外源准入事务数量（每事务恰计1）
```

| 项 | 精确规则 |
|---|---|
| 单位/类型 | transaction，非负安全整数；不是unit质量、列数、相数、粒子数或触摸帧数 |
| 正式输入唯一路径 | `context.externalAdmissionCount`，报告`externalAdmissionCount`由selected.context复制；不从occurrence占位值或sourceEpoch回填 |
| 何时加1 | 最终审计/全部池与场/源账本成功的同一原子准入提交；两相/多列一次事务仅一次 |
| 何时不加 | preview/collect、lambda试算/失败重试、admitted=0、NO_WATER/拒绝/rollback、源内部搬运LIFT/LAND/DEGRADE/MERGE、渲染/receipt重复读取、回调重发 |
| 计数身份 | 成功提交的admissionTransactionId；复用既有runId/sourceId/request.sequence并绑定唯一成功提交序号。若原request sequence不保证一次调用唯一，新增物理侧成功准入序号，不以回调顺序猜身份 |
| sourceInjectedH/L | 同一批成功事务的admittedH/L之和，单位material-volume；小数质量仍算一次事务，不能相加后取整当count |
| 计数reset | 与IH/IL、sourceEpoch、run基线同时reset为0；记录新run/reset身份，跨run计数不可拼接 |
| invert/模式/route/前后台 | 本身不代表外源准入，不加也不单独归0；若调用既有scene reset则按整组reset处理，不擅改原物理epoch行为 |
| 持久化/restore | 准入历史与IH/IL/epoch/count及身份必须成组恢复或明确建立新基线；现WaterSandSnapshot不含完整源账本，不能从particleState或当前质量倒推出历史次数，缺证据不得填0或复制epoch掩盖 |
| 溢出/损坏 | 超安全整数或非有限不能夹紧/回绕；报告计数无效，正式采样不放行，开发确定原子拒绝/错误路径 |

计数只服务源身份审计，不改变E=VH0+IH+VL0+IL、M=U+P、5%、末1s、固定终点或sourceEpoch静置门。通用debug `inject()`/restore直接重建基线不是自动获批的GESTURE_EXTERNAL；正式run若出现未登记来源必须按既有基线/场景规则披露，不偷偷算入或跳过。

### 9.3 与sourceEpoch关系与独立证据来源

当前实现约束下，reset两者同为0、每个成功外源事务使epoch增1且count增1、其他动作均不增，因此归纳得到**合法run中count==sourceEpoch**。这不是缺陷；缺陷是两份序列化字段来自同一变量、完全无法互相校验。更换为质量或按列计数只是换估计对象，会让正常多列/两相运行触发现硬失败，不能视为O-2修复。

推荐实现原则（待裁决）：sourceEpoch继续由物理成功准入边界维护；externalAdmissionCount由同一事务的**成功准入记录/receipt发布源**独立累计，计数不得赋值为sourceEpoch，也不得在stamp时临时复制。成功记录包含admissionTransactionId、run/reset身份、sourceId/sequence、实际admittedH/L、原子成功标识；消费端去重回调/重导出，不去重两个真正不同成功事务。

这里的“独立”是来源/核对链独立：writer维护成功receipt累计器并输出既有count；checker可从完整、按身份连续的成功准入记录重算数量，再核对count/epoch与源质量增量。不可只新增第二个`++`紧挨epoch并宣称已完成独立证据链。若实现采用两个累计器，它们至少由不同的权威事实维护（物理提交与已成功发布的准入记录），并提供原始记录核验；完整性必须有起点/终点/序列边界。

事务和成功记录要在同一提交中预留空间、构造并原子发布；缓冲满或证据构造失败须按现有事务纪律处理，不能先改物理再悄悄丢成功记录。可用单调汇总序号与有界分片导出，不要求无限增长内存；未导出/丢页明确INCOMPLETE，不从epoch补造count。UI回调/异步日志只是成功记录的副本，重复发送不再加1；handler丢回调不应改变真实计数。

保留现有`EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH`语义：当实际记录/序列化/传输发生漏记、重复或错配时，该校验应能检出；**无需且禁止让无故障的健康run出现不等来“证明可触发”**。正常两者相等并不使坏数据检测失去意义；同源复制才使这条交叉检查失效。

### 9.4 合法样本、反例与缺失分流

以下为架构分析例，不新增已冻结测试判线；具体自动化由developer/tester按裁决落地。

| 情况 | 累计IH/IL | count/epoch（同reset范围） | 正确处理 |
|---|---|---|---|
| reset后无准入 | 0/0 | 0/0 | 合法源身份；不据此签整例PASS |
| 一事务两相分配3列，实际H=0.25/L=0.125 | .25/.125 | 1/1 | 合法，count不是3/2/.375 |
| 同事务试算3次后成功 | 同上一次实际准入 | 1/1 | 重试不重复累计 |
| 后续一次被拒 | 不变 | 1/1 | 零准入不计 |
| 再一笔成功H=.10/L=0 | .35/.125 | 2/2 | 合法，不因同sourceId误去重不同事务 |
| 内部shake LIFT/LAND/MERGE | IH/IL不变 | 2/2 | 内部转移不是外源 |
| 成功记录重发同transactionId | 不变 | 2/2 | 导出/回调副本不重复计数 |
| count被漏更新，epoch=2而独立count=1 | 实际物理量仍原值 | 1/2 | EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH→既有INVALID_LEDGER硬失败 |
| count误按3列加3而epoch只加1 | 相同实际量 | 3/1 | 同上，不能把错误计数当独立量成功 |
| 正确writer产生2/2，导出把count篡成1 | 原成功记录保留 | 1/2 | checker应真实处理该导出并命中现规则；故障注入须明确标故障证据，不冒称健康run |
| count与epoch都误漏同一事务 | 原始记录有2笔 | 1/1 | 原等式单独无法发现；独立成功记录重算揭露完整性缺陷，不能宣称一条等式覆盖所有漏记 |
| 缺context count | 任意 | missing | 现missingFields/NOT_IMPLEMENTED，不填0 |
| count=0.375、负数、bool等 | 任意 | invalid | 现INVALID_EXTERNAL_ADMISSION_COUNT硬失败，不转型取整 |
| 历史count/receipt链不可证但epoch有值 | 不可证 | unknown | 保留原字段检查与明确缺证据；完整独立审计不足记INCOMPLETE，不能倒填历史值 |

D-4沿用selected.context原值覆盖占位值，reportMissingFields不得因null/空串/0伪装齐备；本节不回退a5e197f。独立审计报告的完整性与数值终点状态分开，不能仅为新诊断缺失擅改协议现有状态顺序。

### 9.5 提交manager的唯一裁决请求与developer最小交接

建议manager在**原T04**裁定以下一致方案，不新建重复卡：

1. 采纳§9.2成功准入事务次数（整数）与§9.3合法count==epoch的不变量，独立指数据来源/成功记录审计，不要求不同量纲或正常数值不等。
2. 将原卡自测①“有外源合法run两者不等”修正为“真实成功准入记录独立累计，正常run相等；故障注入/漏更新/序列化损坏时不等并能检出”；保留自测②与现有硬失败语义。此项属于任务验收文字冲突裁决，architect本次只建议、不自行更改测试判据。
3. 同意新增成功准入记录完整性与计数来源登记；tester判断是否需独立协议字段/errata revision，并登记适用范围，**不得回改已发布v1正文**。不改endpoint估计对象/阈值/末窗/双算；若坚持count改为按列/请求/质量则必须另起语义与checker revision，不能沿用当前等式。

developer（裁决后）文件/责任：WaterSandField成功准入边界预留原子记录并累计独立count；receipt/记录类型模块提供ID与分片完整性；P011NaturalObservation stamp第8参改为真实count并更新“同一变量”旧注释；checker保留缺失/整数/epoch mismatch并按明确合同读取独立源证据；不复制epoch兜底，不混入D-2/D-6，不重做D-4。持久化/reset/restore语义一并有证据，不能只改第8实参名字。

开发完成线仍为开发+编译+非真机自测+commit，端到端由tester/effector另行处理。原三项endpoint测试/10条反例、O-2新合法与故障样例及九原通道按原卡回归，架构本轮不重跑、不宣称通过；无需manager验收，但口径冲突的**裁决**必须先于依赖它的实现。

### 9.6 本轮完成范围与红线

architect计数对象/单位/递增/reset/与epoch关系、协议一致性、合法样本/反例分析已完成。O-2代码尚未实施，整T04不能因架构交付而关闭；接力状态为MANAGER_DECISION_REQUIRED，T11仍等待T04真实代码完成。基础三线与T37/T20不受此endpoint专属口径阻塞。

只追加本节，不编辑协议/历史报告/代码/测试，不使用设备、不宣布endpoint或peak生效。页首rev2的既有字段对齐状态只描述已完成R1–R3，不表示本O-2建议已获裁决；本节没有隐式升级endpoint方法身份。

追加记录：2026-10-25，T04 O-2架构分析（§9），依据原卡及manager“不可制造不一致”的路由，推荐成功准入事务计数与独立来源审计，明确旧自测逻辑冲突待manager裁决；D-4/D-2/D-6维持已交付，不重做。
