# P-011 两个 legacy FAIL：阶段二重分类预研 v1

日期：2026-10-21。责任：designer。依据 [priority.md 2026-10-20 C-2](priority.md)、manager本任务20:57补充，以及 [KNOWN FAILS历史台账](known-fails-P011-v1.md)。状态：**PRE_RESEARCH / PENDING_PHASE2_REACHABILITY_AUDIT / NOT_FINAL_RULING**。

本稿仅作设计分析，不实施、不修改用例/参数/checker/白名单/历史报告，不运行host测试、正式可达性审计或真机采集。两项建议均附 **待阶段二可达性审计确认**；B·续guard交付前置已满足，不再等待其落地。

## 1. 当前身份核对：不能沿用“两项30°场景仍FAIL”的概括

只读核对时HEAD为`b2e00a8`；下列三个实现文件和测试文件无工作区改动。交付链已核实：`891370d`实现逐writer guard及回写，`36d5288`补直接拦截/零干预/退款覆盖，`d78335b`修复预演guard镜像及完整shadow管线。

| 对象 | 当前源码中的实际场景和断言 |
|---|---|
| KF-1 `bottomCreep_tiltMovesLowerLayers` | `WaterSandFieldTest.ets:448`；16列、capacity192，bh=72/bl=72、waterTop=182、悬沙0；`angleDeg=5`是relax机制角，不是输入角；`setTilt(30)`，60×1/60s=1s。要求所有迭代不报错、最终`max(|Δbh|,|Δbl|)>0`、三相守恒误差<1e-8 |
| KF-2 `p011_commandSignFlipResetsPhaseAndReversesDisplacement` | `WaterSandFieldTest.ets:567`；2列、同厚层初态；`vSandTopMax=1`、totalMotionBudget=100；**+20°执行8步，再−20°执行8步，每步1/60s**，不是±30°。默认`angleDeg=30`是relax角。要求step.ok、phaseId 1→2/sign=−1、两相`ActualDisplacement<−1e-12`、heavyShapeRms/MaxDelta>1e-12 |

共同helper `deterministicRateParams()`仅将heavy/light diffusion置0、waterLevelRate置0.001；仍使用实际planning/guard，未启用correction候选路线。KF-1的`bottomCreepRate=96`不是当前实际申请流率：当前`creepBottom()`用target角与phase residual生成请求，并受`vBottomCreepMax=10/s`限制，不能按96/s推导可达能力。

### 1.1 已有证据的时序，不代做终局裁定

- [guard_units_check.json](../test/reports/guard_units_check.json)仍记录两项FAIL，错误均只给`false !== true`；**没有精确断言位置**，不能单凭它认定一定是最终位移失败，step.ok也是候选断言。
- [architecture §25.1](architecture-P011.md)明确记载：d78335b完整交付guard shadow镜像，三项旧白名单移出后全绿，并有后续独立重跑59/0及checker PASS。
- 当前 [verify_d1_final.json](../test/reports/verify_d1_final.json) 和 [p011_planning_units.json](../test/reports/p011_planning_units.json)均记录本两项PASS。JSON不带逐子步状态轨迹和完整源码manifest；本轮只是读取现有证据，不宣称已对当前HEAD重新跑过，也不从PASS推断其每步都`accepted`或effectiveTilt恰达命令角。
- 当前 [p011_planning_pass_check.py](../test/p011_planning_pass_check.py)已无旧legacy放行集合；git记录该变化在d78335b，代码注释将其归于R-3-a修复。历史KNOWN FAIL台账仍写“保留白名单/等待重测”，与较后工程证据存在时间差。本稿登记差异，不擅自恢复白名单、不修改台账，也不把历史FAIL改成PASS。

因此当前预研基线应表述为：**历史失败具名保留；后续guard+预演修复已有host PASS证据；阶段二的机制适用范围和正式重分类仍待审计。** 不能再把guard刚落地时的集成缺陷作为新guard现状。

## 2. 已实现的三层语义：材料clip不等于整步BLOCKED

实现依据：[P011MaterialGuard.ets](../entry/src/main/ets/physics/P011MaterialGuard.ets)、[P011TiltPlanningPass.ets](../entry/src/main/ets/physics/P011TiltPlanningPass.ets)、[WaterSandField.ets](../entry/src/main/ets/physics/WaterSandField.ets)。当前列间距在这些公式中为1；不额外引入显示像素dx。

设`S=bh+bl+mh+ml`、`g_e=S[e+1]-S[e]`、`d_e=g_e-tan(thetaAccepted)`、`H=tan30°≈0.577350269`。一笔预算限幅后grant为Q，列变化`ΔS=q*v`，则受影响边系数`k_e=v[e+1]-v[e]`。guard求：

```text
0 <= q <= Q
-L_e <= d_e + k_e*q <= L_e       对全部受影响边取交集
L_e = H                         原合法边
L_e = abs(d_e)                  原违规边，仅non-worsening
qAccepted = 闭式交集内最大值
```

相邻搬运联合约束中央边及两邻边；单列settle/lift约束左右两边，percolate为S-invariant。当前调用点逐笔更新S；被裁掉的量回写local/total，bottom通道还回写bottom used。当前channelRateCap为0，没有可假设独立使用的channel预算池；通道观测记录实际量。退款不是增加预算或自动重发请求。

`hardGuardAvailable=true`已在观测writer接线，表示保护能力可用，**不表示此步有非零grant、发生了clip或全run已通过**。实际截断看requested/allowed/hardGuardRejected等证据；需要按受影响物理edge记录限制原因。

| 结果 | 实现语义 | 对原用例的影响 |
|---|---|---|
| writer grant缩小或归零 | `p011GuardAdmit`返回accepted/rejected；0 grant可继续其它writer，不直接抛planning错误 | step.ok可为true，但“发生运动”可能不满足，二者分别判断 |
| planning `accepted` | 速率限制后的thetaRequested可提交；thetaRequested本身未必是targetTilt | 有合法运动机会，不保证最终目标幅值 |
| planning `clamped` | 两轮预演后在较小可行角提交，writer照常运行 | 可合规通过原step.ok；不得把clamp记作target已到达 |
| planning `held` | 按旧角重新规划可行，保持参考角但仍真实提交材料子步 | 可能仍有材料移动，不是整笔事务未提交 |
| planning `blocked` | 无安全旧角方案/原态已违规等；返回`TILT_PLANNING_BLOCKED`、step.ok=false，回滚失败子步，记录原因 | 原用例会失败；安全拒绝行为可以是安全测试的正确结果，但不是该运动回归PASS |
| preview/commit mismatch | 显式错误和失败子步回滚，不能当作普通clamp | 实现/一致性问题，不以KNOWN_FAIL永久放过 |

guard不直接限制参考角；§20 planning负责角度。d78335b之后预演逐通道演化shadow材料、共享受影响边推导和guard求交，避免“预演承诺修复、apply被guard裁0、参考角先越线”的旧R-3窗口。实现路径是：规划→collect/一致性核对→真实writer apply→普通audit；当前`audit()`检查有限值、库存、容量、守恒，不能因返回ok就当它另做了全边tan30复核。正式审计仍应独立重算每笔及每个提交的残差。

已违规初态虽然writer局部支持non-worsening，planning入口仍可报`PREEXISTING_INFEASIBLE_STATE`；不能推断guard会自动把非法初态修好。BLOCKED只回滚失败子步，不撤销本frame先前已提交子步，也不撤销此前外部命令身份；不能把整个run当未发生。

## 3. 分用例预期与可达性分析

### 3.1 KF-1：30°命令下下层是否发生过最终非零变化

**预期：guard+shadow可使原场景在受限通量/受限参考角下合规运行，后续host PASS已有依据；不要求必然BLOCKED，也不保证无clip地达到精确30°。** 原断言没有检查最终effectiveTilt=30°、2px可见性、1秒内目标形状或全宽收敛。

初态平坦时`g=0`。正向内部邻列搬运q使中央边`Δg=2q`、两邻边`Δg=−q`。若参考角**恰为+30°且所有相关边仍平坦**，`d=−H`，邻边要求`−H−q≥−H`，故q=0；16列任一单独相邻搬运都至少有一条此类邻边。这说明“精确硬边界平坦态上每笔都必须非零”不合理，不能放宽guard来保证它。

但原场景不是瞬间把参考角强设30°：maxTiltOmega=720°/s，dt=1/60s，每步最多12°，实际还有planning限幅。在θ<30°时，平坦邻边余量为`H−tanθ>0`，早期合法小grant有空间；例如12°下约0.36479。后续已非平坦的S也会改变各边可行余量。**精确30°平坦态的零grant结论不能推广成整个1秒轨迹不可能有下层变化。** 早期发生移动也不自动证明最后快照仍有净变化，最终`lowerMove`仍须原断言和轨迹核对。

阶段二不能把这个弱回归外推为128列±45°、30°±2°全宽终点/5.1s可达。原16列、非零变化、1秒的目标远弱于阶段二全局目标；不得拿大场景预算不可达结论反向否决这个回归。

### 3.2 KF-2：相位重置与负向bottom通量是两件事

**预期：相位重置由setTilt立即完成，实际反向运动取决于合法grant；后续host PASS支持当前原配置可执行，但不能从命令翻转推出每一步无条件反向。** `beginCommandPhase()`在目标符号变化时递增phaseId、设置sign、清零ideal/actual并以当前bh/bl重建baseline；它与planning是否接受下一步角度不同步。

两列仅一条边，没有KF-1的两个邻边约束。平坦态在θ=+20°时，正向q上限为`(H+tan20°)/2≈0.470660`；仍处+20°时负向q上限为`(H−tan20°)/2≈0.106690`，都存在正余量。翻转时已产生的g、方向保持时间和参考角过渡会改变上限，应逐步按`|g±2q−tanθ|≤H`计算。

**条件性预算/几何构造（不是正式审计或原实现复跑）：** 若每步其它通道不占用这些额度，取q=0.01，H/L各0.005，则每端local使用0.01≤1/60，bottom使用0.01≤10/60，总使用0.01≤100/60。前8步正向使g从0到0.16；翻转后8步负向回到0。按速率未受额外限幅的候选角`12,20,…`及反向`8,−4,−16,−20,…`，整个构造残差均在H内，反向阶段最大约0.44397。每相反向bottom累计为−0.04；相位baseline相对bh变化可非零。这个例子只排除了“guard/预算原则上禁止该两列反向目标”的断言，不证明实际共享通道会分配出同一轨迹。

原正向8步约133.3ms，超过默认directionHoldTime=80ms；在顺利提交的原序列中，翻转不必被旧方向保持窗持续阻塞。若此前步BLOCKED、物理时间未推进，不能继续照搬这个时间假设。参考角每步最多12°，20→−20至少约4步，但安全限幅可更慢。

特别注意：`heavyActualDisplacement/lightActualDisplacement`是`creepBottomRequests()`对分相**bottom搬运量**的有符号累加，不是材料质心，也不是所有通道总位移。悬浮平流即使带来真实反向重沙运动，也不会自动使这两个旧指标为负。不能把新通道的位移灌进旧字段伪造原断言通过。

## 4. 阶段二候选机制：能增加什么，不能保证什么

| 路线 | 对KF-1的可能作用 | 对KF-2的可能作用 | 正式审计必须补齐 |
|---|---|---|---|
| 现有bottom/relax，完整guard+shadow | 已有host PASS证据；非零移动不必依赖方案3。仍可在边界clamp/held/零grant | 相位语义与合法反向bottom通量已有通过迹象 | 固定build下每步参考角、writer grant与最终形状；不能只读汇总PASS |
| 方案3悬浮平流（仅搬已有悬沙） | 两用例初始悬沙0、无shake；原lift需要shakeEnergy>0，故仅增加`advect(susp)`对零库存没有作用 | 同左；且即使有悬沙，也不直接改变旧bottom位移指标 | 悬沙来源、lift/settle时序、相库存、每边输运、完整预算、重沙是否能从上层提取 |
| 方案3含合法起沙→平流→落沙链 | 可绕过“每笔局部表面输运的邻边卡死”的某些路径，但不绕过guard | 可改变全材料一阶矩与终态；若bottom仍零，则须以独立全通道目标评价，不能替代原bottom回归 | 逐笔可提交构造、悬浮容量、重/轻分配、全部成本与终点悬沙；不能只证明终态合法 |
| 体力驱动蠕动候选 | 可在小表面梯度下提出请求；若最终仍是同样相邻表面delta与同预算，guard可行域不变，不能靠加大force跨过q=0 | 可改善反向驱动力，但相位更新不应绑定“必有grant”；必须说明该新通道是否仍属bottom或独立账本 | 力→流率律、耗散/方向、local/bottom/total归属、与原通道是否重复计功、preview/apply一致性 |

一个说明方案3**局部安全可行性而非总体可达性**的例子：精确+30°平坦16列，先在最左列lift q（使edge0的g增加q），再在悬浮场向最右输运，最后只在最右列settle q（使edge14的g增加q），两端表面变化在`0≤q≤2H`内可不破坏tan30。中间悬浮搬运ΔS=0。若以逐相邻边搬运记成本，15条边的输运加lift/settle至少涉及17q量级的操作账目，表面最终一阶矩增量为15q；**不是一次q预算免费跨15列**。当前无shake/悬沙来源不具备该链的实际前置，heavy还受到上覆轻层和提取规则限制；因此该例不能宣布KF-1或阶段二目标已可达。

一阶矩审计应分别记录`Mdep,H/Mdep,L/Msusp,H/Msusp,L`，以列坐标加权。邻边搬运q贡献±q；lift/settle在同列于沉积和悬浮之间转移，二者总矩互相抵消；只有真实水平运输改变全材料矩。不得把lift、advection、settle对同一批材料的变化重复累计为多倍收益。终点稳定与悬沙≤初始沉积量5%等适用阶段二目标约束继续沿用；不能临时全扬起、截取中途图像作稳定终点。

体力蠕动若只做S-invariant相交换，也不能据此宣称重轻两相都同向取得净质心位移：在总沉积场不变且无悬浮变化时，总材料一阶矩不变。需要把“相内重排、bottom通量、全材料输运、可见bh形状”分开验证。

## 5. 每项三选一建议：均选③，待阶段二可达性审计确认

| 用例 | ①重调参数转正 | ②永久KNOWN_FAIL豁免 | **③重新设计场景语义（推荐，非实施指令）** |
|---|---|---|---|
| KF-1 | 不推荐。降低30°、增加budget或延长窗口求绿会丢失原边界覆盖；已有host PASS也无此必要 | 不推荐。安全不变量不能永久豁免，工程修复已显示不必永久失败 | 保留原16列30°/1秒回归不动；另将“边界安全提交/合法拒绝”和“有可达见证的下层运动”拆成独立语义。前者允许真实clamp/held/guard零grant但必须全边安全；后者必须实际非零移动，不能以零grant的安全通过替代。**待阶段二可达性审计确认** |
| KF-2 | 不推荐。原输入是±20°，不存在直接把“30°调低”这一事实基础；不增133.3ms反向窗口来掩盖延迟 | 不推荐。命令相位重置和账本方向应长期保留回归，不能因guard存在而永久免验 | 保留原两列±20°回归；独立验证命令phase身份/清零baseline、实际accepted角和合法bottom grant、全通道最终形状/矩目标。新机制使用新指标身份，旧bottom字段保持原含义。BLOCKED不冒充反向成功。**待阶段二可达性审计确认** |

这里“重新设计语义”是**补充分层验证和目标归属**，不是把原`expect(ok)`直接改成“任何BLOCKED均PASS”。原用例既有host PASS若在冻结基线上复证且安全账本完整，应作为无需重调的回归保留；若未来失败，则精确定位是保护正确、动力不足还是实现退化。任何新用例/期望变更仍经manager裁决、tester执行，不在本稿实施。

建议的最终分类决策树（审计后才执行）：

1. 冻结原用例及候选route/source/参数，原断言和完整安全账本均通过 → tester报告原回归继续PASS，manager决定历史KF台账如何RESOLVED/RECLASSIFIED；历史FAIL保留。
2. 保护正确但原有限窗口内运动没有充分可达见证 → 区分安全子用例PASS与运动子目标PENDING/FAIL；审计给出构造或条件性不可达证明后再重定义新场景，不擅自把原失败删掉。
3. 出现提交越界、预算/库存/守恒错账、preview/apply分歧、phase身份串线 → 作为实现/安全回归处理，不准用legacy或阶段二延期豁免。
4. 仅必要条件未排除、尚无可执行轨迹 → 仍待审计，不宣布机制目标可达，也不判永久无解。

## 6. 依赖、审计交付与状态边界

| 前置 | 当前状态 | 说明 |
|---|---|---|
| B·续guard本体/直接覆盖/预演镜像 | **已交付✅**：891370d+36d5288+d78335b | `hardGuardAvailable`已接线；不再列“等guard实现” |
| 阶段二全通道可达性审计 | **NOT_STARTED / 结论未到** | 正式启动仍按manager阶段边界；本稿不是该审计，不启动实验 |
| 旧台账与工程PASS证据同步 | **待manager/tester核对** | 现有白名单已移除，不恢复；现有PASS记录不等于阶段二正式重分类已裁定 |
| 候选机制落地/验证 | **未由本稿授权** | 先审计，再设计/架构/实现/独立测试；新路线与LEGACY及correction分别登记 |

正式审计最小交付：原KF-1/KF-2各自manifest、逐步target/requested/accepted角与状态、物理dt/提交次数；每writer的预算前grant/guard后grant、受影响物理边、pre/post S及H/L库存；底层/表面/悬浮一阶矩；phaseId/sign及baseline/ideal/actual；合法提交与blocked attempt逐笔对齐。先复核原1s与8+8步目标，再另报阶段二128列±45°/5.1s/稳定终点/悬沙目标，不能混用几何或时限。若浮点边界或接口身份缺证据须单列，不用改阈值求绿。

采用已存在的审计/账本字段；确需补字段或工具另行路由，不在本稿改代码。全通道审计大纲沿用 [阶段边界复议 §8](review-basis-P011-stage-boundary-v1.md)。预算、守恒、库存、capacity、tan30、参考角事务与原跟手指标均不放宽。

## 7. 本次只读核对指纹与交接

| 文件 | SHA-256（本次读取） |
|---|---|
| WaterSandField.ets | `ec010a27c5504eed43ff0d2c607a2f1a8dba12bbc700527ffd2adcbd71353cb8` |
| P011MaterialGuard.ets | `caa9780a44ea114592c3f7cf444af47c2799d543b53279a486c29151b13f2ef6` |
| P011TiltPlanningPass.ets | `d60ee5167fcbf7f65771f1388d76f048600389f58ca4a8c3363779b77012c975` |
| WaterSandFieldTest.ets | `007b91a35afa5a2980ef253b30fa3147235b3aa587205d92c5abf2fc41bca2d8` |
| guard_units_check.json（历史FAIL） | `591c767718d7329acdd027b5b33ef03438d9e587a97b6bfa71f328c3f2c9d64b` |
| verify_d1_final.json（后续PASS记录） | `4588642ddcdd00cb7c4facee30f0c63599a372d99d2b96e3d5ab620f2f4ca35b` |

交manager的预研回执：两项均建议③，**待阶段二可达性审计确认**；不推荐调参求绿或永久豁免。请将“历史失败、已交付guard/预演修复、后续host PASS、阶段二目标待审”分别登记。此次仅新增本设计稿，未改任何现有裁决、状态台账、白名单、用例或实现。
