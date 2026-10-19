# P-011 v2.9 设计复盘

> 日期：2026-10-19。候选路线：`bottom-creep-v2.9-central-dune-heavy-interface`。状态：`DESIGN_PENDING_HIL / BLOCKED-V29-ANGLE / BLOCKED-V29-RELEASE`，未批准实现。本轮仅只读复核既有 ETS、报告及 host probe，未运行代码、测试或设备；不更改 `dual-input-v1`、默认几何、预算、门槛或历史结果。算法全文以 `docs/algorithm-P011.md` §6.11–§6.16 的阻断复核与全通道审计闭环为准。

## 已确认的基线

正式新包为 `test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap`，SHA256 `C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`；review commit `130a52c74e8964fb024677b672cad1965f5cf1db`，实际构建含 `fdc8190` 基线和 dirty 48 文件 source snapshot。dual-input 原产品测试 `39/39 PASS`，冷热首 Want、静止设备 fresh sensor、面板双向切换有独立通过证据，不能据此推断物理通过。

`test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json` 与 `green-new-hap/preflight.json` 记录默认 thick `cols=128/capacity=192/vSandTopMax=28`：+15/-15 的 `heavyFluxNonZeroSamples=124/154`，但 `heavyShapeRms≈1.06e-7/5.37e-7`、`heavyShapeMaxDelta≈1.77e-7/1.65e-6`，未达 `0.5/1.0`；green mask 质心约 `+0.0061/+0.0012px`。输入 `0/+15/0/-15/0`、序号 1..5 正确，两个非零目标各约 5.1s wall-clock。结论是默认几何物理 `FAIL(heavyObservation)`，后续 canonical 和正式主观 `BLOCKED`。历史绿样本几何为 `10/1000/30`，不能替代默认结果。

## 候选取舍与本轮修正

| 候选 | 判断 | 依据和边界 |
|---|---|---|
| v2.8 均匀 127 界面输送 | 淘汰 | 平坦场内部相邻 donor/receiver 几乎相消；host 原 ETS 正相位 full-array max 约 `0.02565`，中央 80% RMS 仅约 `3.48e-7`。非零 flux 不等于中央 heavy 形状响应。 |
| 提高 `vBottomCreepMax`/`totalMotionBudget`、换 10 列几何或放宽 0.5/1.0 | 淘汰 | 违反既定约束。 |
| 跨列搬运 | 淘汰 | 违反相邻 donor/receiver 语义。 |
| 中央四列 dipole 与 bottom-first 顺序 | **ANGLE 否决** | `A=2.7,K=2` 的 104 列理想 RMS `0.5295`、max `2.7`；中心 heavy 阶差却达 `5.4`，0.5 share 同向 light 合成的总 `sandTop` 阶差达 `10.8`，均超过 +15° 上界 `tan15°+tan30°≈0.8453`。旧稿 `0.535` 的 ROI 分母也错误。 |
| 104 列平滑半波 dipole | **BUDGET 否决** | 理想 angle/RMS/max 可过，但 heavy/light 相邻通量分别约 `1893/1893`，远超本相位最乐观 bottom 上限约 `51.22`；见下文。 |

旧稿把预算调度探针写得过于接近实现验证。`host_budget_probe.json`/`host_budget_probe-shake.json` 通过 bundled TypeScript 执行的是**原** `WaterSandField.ets` collect/restore/replay；每帧 shake 样本原 v2.8 bottom grant 折合 `0.9355/s`，探针按 bottom 优先假算 `10/s`、其他 `289.03/s`，总 `299.03/s<=300/s`。原 ETS 守恒漂移最大约 `7.28e-12`。这证明既有请求和算术有可行空间，没改变实际 apply 顺序，没执行 dipole，也没证明新顺序下其他通道仍拿到同样额度。collect 中的 `totalMotionBudgetRequested` 是请求统计，不是已扣 reserve；若在 collect 预扣，再在 apply 调 `clipTotalMotion`，会重复扣费。可行契约是仅在 apply 成功提交的实际 grant 上各扣独立 bottom 与共享 total 一次，同时在新顺序下重新核对 donor stock、receiver capacity、split、rollback 和其他通道需求。

相位生命周期也限制候选：同号 100ms Want 应沿用 `F/X` 和 baseline；短暂 `0°` 暂停并保留 residual；反号命令从**当前材料**开新相位，可使相位内形状指标达标，却不自动产生相对原始截图的反向视觉位移。若正相位已留下 `+d`，新相位做 `-d` 大致回到原始形状；若要从原始图两侧形成 `+d/-d`，正峰到负峰至少需 `8A=21.6` heavy 相邻通量，固定 0.5 heavy share 和满额 `10/s` bottom 时需要 `4.32s` 有效物理时间；以最乐观允许 share 0.65 仍约 `3.323s`。壁钟 5.1s 不保证这些物理时间或实际 grant。TEST/EXPERIENCE 边界还必须清除路线私有 target/residual/phase baseline，保留材料、runId 和物理时间；现有 `resetInputState()` 尚未清 `commandPhaseSign`/形状 baseline，不能宣称无残留。

原 §5.5 的同向 `≥2px` 是**主观 green 边界 ROI 成功信号**。现行 `TC-P011-V28-00` 客观预检要求 ROI 可识别且正负 heavy 非零/RMS/max 达标；`greenCentroidDeltaPx` 是诊断值，脚本没有 2px hard gate。旧稿错误地把它加成预检客观门槛，已在算法 §6 修正。候选正相位的连续重沙质量质心估算仅约 `0.0144px`；同一近似下整块质量质心移 2px 需约 `1497.6 model-unit·column` 相邻 heavy 通量，远高于 `10.8`。这不构成 RGB 阈值后二值 mask 或局部边界像素位移的严格不可能证明，却足以否定“模型 RMS 达标便会视觉达标”的推断。主观评估须分开测局部边界与全 green mask 质心，不能偷换名称或阈值。

## 决策与剩余证据

当前不交 architect/developer 实现，也不启动 tester/effector 或设备。用户已将原 `≥2px` 成功信号的测量对象定为局部 heavy 边界位移；整块 green mask 质心仅作诊断，不能代替局部信号。现有四列及半波候选分别因 angle/预算失败，局部 2px 也尚无真实 raster 证据；应继续记录约束矛盾，不得伪造 v2.9 通过，现有阈值、默认几何及预算均不动。

即使用户明确视觉口径，仍需真实默认 ETS 的双向连续 `F/X`、实际 `bh` shapeResidual、H/L grant、两个预算、逐边完整 `sandTop` 角度、容量/守恒、`maxSubstepBh` 与输入 reset 生命周期；再看同分辨率连续录像、现行客观门槛、原主观 HG/≥6/10、canonical 531/冻结审计与独立双报告。现有 host 探针只能是设计证据，绝不是 `READY_FOR_ARCHITECT` 或 HIL PASS。证据路径见 `docs/algorithm-P011.md` §6.10。

## 架构 §16 的数学复核与阻断结论

新的显式 `targetDelta_s[c]` 采用中央 104 列 `r=12`、`n=c-r`、`s=±1`、`a=1.1`：`n=0..51` 时 `d_s[r+n]=-s*a*sin(pi*n/52)`；`n=52..103` 时 `d_s[r+n]=+s*a*sin(pi*(n-52)/52)`；其他列为零。左右半波质量相等，正向每条边只需向右相邻 heavy flux `F_H[j]=-sum_{c<=j}d_+[c]>=0`，反向镜像。104 列理想 `RMS=a/sqrt(2)≈0.77782`、max `1.1`。固定 0.5 heavy share 且 light 在同一边成比例时，`ΔS_target[c]=2(d[c+1]-d[c])`（平坦 phase baseline），绝对值每边最多 `2a*sin(pi/52)≈0.13284`。因此 +15° 每边落入 `[-0.30940,+0.84530]`、-15° 落入 `[-0.84530,+0.30940]`，最坏 `|ΔS_target-tan(±15°)|≈0.40079<tan30°≈0.57735`。同步放大 `λd,0≤λ≤1` 的理想中间形状同样安全；**实际**每子步仍须用 `bh+bl+mh+ml` 的逐列差及 `effectiveTilt` 重新判定，动态 share、其他通道或非同步 grant 不受这项目标证明保护。

目标的单个正相位相邻 heavy 成本精确为 `B_H=52*a*cot(pi/104)≈1893`，light 同样约 `1893`，合计 bottom `≈3786`；若从全局 `+d` 到 `-d`，反向还要双倍。正式 +15° 约 5.122s wall-clock，假定有效物理时间乐观地等于全部壁钟，也只有 `10*5.122=51.22` bottom units，实际旧场约 2.722s physics time 的上限是 27.22；共享 total `300/s` 不能提高独立 bottom `10/s`。这个平滑目标因预算失败。对**平坦 phase baseline、固定 `h≤0.65`、各边 H/L 同向成比例、无其他通道改 total slope、只许目标方向相邻输运**的任意 profile，离散分部求和与 angle 上界给 `B_total≥104*(0.5)^2/[h²*(tan15°+tan30°)]≥72.8>51.22`。这证明整个固定 share 类在这一正式相位里不能同时过 RMS/angle/budget。动态 share 或 light 逆向补偿未被此下界覆盖；它们没有现成的可行构造、完整成本或实际 ETS 证据，故不能当作已解。

架构 `BLOCKED-V29-RELEASE` 的设计契约改为 `e[c]=bh_actual[c]-bh_phase0[c]-targetDelta_s[c]`，`shapeResidualRms=sqrt(sum_ROI e²/104)`、`shapeResidualMax=max_ROI|e|`；`F-X` 只诊断传输缺口。对上述 `a=1.1` 理想目标，候选 release 容差 `RMS_e≤0.10/max_e≤0.05` 可保证实际 RMS≥0.67782、max≥1.05，且须在完整 angle、预算、容量、TC03 等门槛下持续至少 12 个有效子步和 0.20s physics time 才入 `STABLE`。稳定后连续 2 子步**形状残差**超界则进入 `REPAIR`，从实际材料计算差额，不重建 baseline 或重发目标；angle/预算/容量/守恒硬门槛的候选 grant 在该子步就裁剪/拒绝，绝不等待稳定窗口。若接口阻塞、剩余时间/预算不够或相位结束，显式 `BLOCKED`。这些是未执行的设计判别参数，半波的预算失败意味着不能配置为已实现释放。

生命周期：完整 scene/run reset 重置材料及 route 私有相位；同号 100ms Want 不重设 `targetDelta/F/X/baseline`，短暂 0° 暂停并保留 residual；反号 Want 从当前材料开启新 phase，先等待既有方向 hold，旧符号不能继续计 grant；TEST/EXPERIENCE 切换仅清 route 私有 target、baseline、residual、phase sign 与稳定计数，保留材料、runId、物理时间及首 Want/fresh sensor 语义。collect 只记请求，apply 成功才提交材料、`X`、残差和两预算各一次。现有 ETS 未验证上述生命周期。

视觉 2px 口径已由用户于 2026-10-19 定为**局部 heavy 边界**：测量指定列/局部 ROI 的 `bh` 边界像素随倾角同向移动，whole green mask 质心仅作诊断，不作为同项 2px 判定。局部口径仍需实际 raster 标定，半波峰值 1.1 model unit 要达到 2px 至少约 `1.82px/model unit`，尚无证据且该半波预算失败。whole-mask 连续质量近似仅保留为历史诊断：预算上限下正相位 heavy 一阶矩最多 `0.65*10*5.122=33.29 model-unit·column`，同近似的整块质心最多约 `0.0445px`；RGB 阈值非线性使这些数不是严格像素界。口径定案不解除上述 angle/预算/释放阻断。

**判定**：旧四列的 `BLOCKED-V29-ANGLE` 已被严格确认；平滑半波在理想 angle 上可行但预算明确失败，未证明真实动态 split/其他通道可行。`BLOCKED-V29-RELEASE` 虽有实际 `bh` 残差与崩塌规则，尚无可到达目标及设备验证。两项阻断均未解除，保持 `DESIGN_PENDING_HIL / BLOCKED`，不交 architect。manager/用户若要求继续该路线，需先拿到不改默认几何、预算、相邻输运、原质量分数及 dual-input 的新可行证明；若无法提供，应记录约束冲突并决定路线取舍，不伪称 v2.9 完成。

## 动态 share 最后一轮审查

固定 share 的 `B_total≥72.8` 下界不能代替对真实 `splitBottomPhaseBalanced()` 的证明。原函数由 donor 中的 `bh/(bh+bl)` 和全局 H/L 相位残差决定每笔 share；普通库存足够时目标 share 被夹在 `[0.35,0.65]`，库存不足的补分路径须另查实际比例，不能借它绕过原 share/容量语义。令每条边已提交总 flux 为 `G_j=H_j+L_j≥0`，普通路径累计实际 `p_j=H_j/G_j∈[0.35,0.65]`。平坦 phase baseline、**仅计 bottom 通道**下，`totalDeltaS[c]=G[c-1]-G[c]`，而 `heavyDelta[c]=0.5*totalDeltaS[c]+(p[c-1]-0.5)G[c-1]-(p[c]-0.5)G[c]`。因此动态 share 的空间变化可在内部总沙高不变时形成 heavy 组成变化；一个理想的常数 `G`、交替 0.35/0.65 share 例子可得内部 heavy `±0.30G` 而 total `0`。它**不是**真实函数能实现的目标：段首尾 angle、预算、库存、残差反馈和逐子步输出仍须一并满足。

现有约束能给必要量，尚不能否证所有动态 profile。正相位平坦 baseline、单向 flux、`B=sum G`、angle 正斜率上限 `U=tan15°+tan30°≈0.84530` 下，有 `||totalDeltaS||₂²≤U B`。heavy RMS≥0.5 使 `||heavyDelta||₂≥sqrt26≈5.099`，所以 share 调制项 `D((p-0.5)G)` 的 L2 必须至少 `sqrt26-0.5*sqrt(U B)`；按 5.122s 的乐观 wall-clock 上限 `B≤51.22` 得约 `1.81`，按旧正式物理时间预算 `B≈27.22` 得约 `2.70`。由 `|p-0.5|≤0.15` 又有 `||D((p-0.5)G)||₂≤0.30||G||₂`，要求 `||G||₂` 至少约 `6.0/9.0`；这些条件尚未互相矛盾。实际函数的 donor 组成与全局残差可能使 p 变化远小于任意选边的极端例子，但没有无需运行的严格上界。其他通道若改变 total slope，必须加入完整 `sandTop` 和成本，不得从这个简化式声称通过。

因此这是**条件性不可行证明的终点，而非全动态类别的绝对无解证明**。仍缺精确计算：约束真实 `p_j` 的空间/时间可达范围，联立角度、每子步 bottom `10dt`、total `300dt`、容量/守恒和正负相位时长，给出具体可行 `targetDelta_H/L` 与有序 grant，或更强的普适下界。目前两者都没有，记录“动态约束类未覆盖／无已证设计解”。release 的 `RMS_e≤0.10/max_e≤0.05`、12 子步/0.20s 仍只是候选；形状塌回可进入 `REPAIR`，angle/预算等硬门槛逐子步裁剪或拒绝，预算耗尽即 `BLOCKED`。2px 口径已定局部 heavy 边界；不以此解除上述数学审查或状态阻断。状态保持 `DESIGN_PENDING_HIL / BLOCKED`，不交 architect/tester。

### 对真实 split 偏置的补充下界

源码常量为 `P011_HEAVY_SHARE_BIAS=0.10`，两个 phase ideal 同量增长，`step()` 将每子步 `dt` 限在 `1/60s` 以内。普通库存充分路径，令每笔 donor 基础比例偏差 `u=clamp(bh/(bh+bl),.35,.65)-.5`，全局偏置差 `D=(idealH-actualH)-(idealL-actualL)=actualL-actualH`，则实际 heavy share `p=clamp(.5+u+.1D,.35,.65)`；每笔 grant `q` 后 `D'=D-directionSign*2q(p-.5)`。它按边逐笔更新，不能把各边的 p 当作自由独立控制。库存不足时函数的额外 heavy/light 补分可让**实际** share 超出目标 clamp，须单独标记 stock fallback，不在普通路径证明内。

正相位若从 `D=0` 起、所有待执行 donor 在 split 前都满足 `|u|≤δ≤.075`、无 fallback，则 `D'=(1-.2q)D-2uq`；因 `q≤10dt≤1/6`，归纳得整个相位 `|D|≤10δ`、`|p-.5|≤2δ`。尤其均匀 `bh=bl=72` 且只有成对 bottom 输运时，`u=D=0` 永远成立，真实 split 精确 0.5；动态 share 必须由其他通道或旧材料历史制造组成差。**若再限定其他通道不贡献净 heavy 形状或总沙高斜率**，由 `||heavyDelta||₂≥sqrt26`、`||totalDeltaS||₂²≤.84530B`、`||D((p-.5)G)||₂≤4δB`，必要条件是 `δ≥[sqrt26-.5sqrt(.84530B)]/(4B)`。乐观 `B=51.22` 时阈值约 `.0088`；旧正式正相位预算量级 `B=27.22` 时约 `.0248`。如果新路线全过程 donor 偏差有小于相应阈值的严格上界，并能限制其他通道贡献，才可把此条件升级成该正相位的不可能证明；现有报告没有新路线逐笔 donor 上界，不能代填。

反相位因 phase actual 按方向带符号累加，普通路径递推为 `D'=(1+.2q)D+2uq`，已有 donor 扰动可能被放大，不能镜像使用正相位的稳定界；`u=D=0` 的均匀特例仍保持 0。这个符号差是当前实现的代数事实，不能假定符合新路线的质量目标。最小缺失状态是新路线每笔 grant 前的 `bh/bl`、真实 share 与 fallback 标志、全局 `D`、有序 `q`、其他通道带来的逐列 `bh/bl/mh/ml` 改变以及有效 `dt`。没有这些的严格上界或可达性构造，就无法证明整个动态类可行或不可行。算法 §6.13 给出完整推导；两个架构阻断仍有效。

## 全通道与运行参数复核：条件性结论的最终边界

算法 §6.15 修正了架构 §16 的一个源码归因：用户要求的**形式验收**仍是逐子步实际 `|ΔsandTop-tan(effectiveTilt)|<=tan30°`，先前半波和 `B_total>=72.8` 均按此条件计算；但正式 thick 在 `SandCanvas.ets` configure 前把默认 `angleDeg=30` 覆盖为 **4°**，并设 `maxAvalancheFlux=80`。运行时 `relaxAngle()` 使用 `tan4°≈0.06993` 作为**请求崩塌的触发差**，不是 tan30 形式验收，也不是自动保持的硬不变量。+15° plateau 的无崩塌触发带是 `[0.19802,0.33788]`，-15° 镜像为 `[-0.33788,-0.19802]`；半波理想总坡度绝对值仅 `<=0.13284`，虽然通过用户 tan30 条件，在没有其他通道补坡时仍会触发实际崩塌。架构 §16 对运行参数的陈述应在其后续维护中勘误；本轮没有修改架构或参数。

正式 thick 还在每帧调用 `setShakeInput(min(1,abs(effectiveTilt)/90))`，±15° plateau 约为 `1/6`，所以初始 `bh=bl=72` 的 bottom-only `p=0.5` 不变量**不能**外推到整个正式产品轨迹。`liftByShake` 可生成悬沙，`diffuseSuspension` 在 collect/apply 预算账本之前移动相邻悬沙，`settleSandV2` 生成 `mh/ml`，`percolateHeavyConservative` 的同列更新 `Δmh=-(q+r),Δbh=q+r,Δbl=r-q,Δml=q-r` 给出 `ΔsandTop=0` 但 `Δbh>0`；崩塌 top flux 也可能从 donor 的 `bh` 取走重沙。渗滤/崩塌/摇动/沉降扣共享 `300dt`，渗滤不扣 bottom 独立 `10dt`；扩散另受其速率与库存约束。它们是否能在 5.1s 壁钟、真实有效物理时间内造出符合方向、RMS/max、角度、容量、原质量分数及 release 的目标，尚无证明。允许这些通道后，`B_total>=72.8`、donor δ 下界、bottom-only whole-mask 连续质量估算 `0.0445px` 都不再是全类别结论。

可审查的物料账本是：边通量从左到右记正、从右到左记负，`div F[c]=F[c-1]-F[c]`；以 `G/H/A` 为 bottom 总/bottom heavy/top 崩塌边通量，`T/L` 为本列沉降/摇动升起对 `sandTop` 的增加/减少，`P` 为渗滤增加的 `bh`，`R_H/L_H` 为崩塌/升起实际移出的 `bh`，则 `ΔsandTop=div(G+A)+T-L`、`Δbh=div H+P-R_H-L_H`。这说明仅限制 `G` 的 10/s 不足以界定 `Δbh`；但 `P` 和动态 share 也不是任意可指定控制量。要给出真正的 target 或完整无解证明，仍缺同一默认场景每子步的逐列 `G,H,A,T,L,P,R_H,L_H`、悬沙/混合库存、split 前 donor 比例与 fallback、按边顺序的相位 `D`，以及这些量在 `300dt` 和真实 `dt` 下的可达上界；现有正式报告与旧 host probe 没有这些状态。**本轮既未构造可行 `targetDelta_H/L`，也未证明全部实际动态 share/既有通道策略数学无解**，不能把条件性 bottom-only 反例或预算探针升级成 v2.9 方案。

给 manager 的明确选择是：确认 tan30 额外形式验收与 thick tan4 运行崩塌并行存在；确认 v2.9 是否允许依赖现有摇动/悬沙/沉降/渗滤/崩塌形成 heavy 形状。若路线限定 bottom-only，已证不变量和下界足以否决；若允许全部既有通道，须先补上述可达状态或解析界，再评估 actual `bh` residual 的候选 `RMS_e<=0.10/max_e<=0.05`、12 子步/0.20s 稳定窗和崩塌后 `REPAIR`，逐子步 angle/预算仍不能靠持续修复越界。用户已将 2px 成功信号确定为**局部 heavy 边界位移**；whole green mask 质心仅为诊断，两者不可互换，也不把 2px 擅增为 TC-P011-V28-00 客观 hard gate。状态保持 `DESIGN_PENDING_HIL / BLOCKED-V29-ANGLE / BLOCKED-V29-RELEASE`，不交 architect，也不启动 tester/设备。

## 未批准的状态机与未来判别口径

算法 §6.14 记录 `INACTIVE→TRACKING→STABLE/REPAIR→BLOCKED` 的**条件性契约**，不是可实现路线：必须先有通过真实动态 split、逐子步 angle、两个预算及实际 `bh` residual 的新目标。collect 只记请求；apply 成功才提交相邻 H/L grant、`F/X` 与预算各一次；完整 `sandTop=bh+bl+mh+ml` 的 angle、守恒、容量和 `maxSubstepBh` 在每个有效子步末审计，违规当子步裁剪/拒绝。实际 `e[c]=bh_actual[c]-bh_phase0[c]-targetDelta[c]` 的 RMS/max 与候选 12 子步/0.20s 窗口才可放行；`F-X` 只是诊断。稳定后形状连续崩塌可进 `REPAIR`，但 hard angle 没有等待窗口，预算/时间不足进 `BLOCKED`。

假设有目标，route 状态与逐边 grant/angle/residual 每子步须为 `O(cols+m)=O(cols)` 时间、`O(cols+m)` 额外空间（`m<=cols-1`），无限重试或跨列直接搬运不符合此目标。未来断言须覆盖默认 `128/192/28`、全部 127 边实际角度、bottom `10dt` 与 total `300dt` 单次扣账、实际形状稳定/塌回、run/reset/同号 Want/0°/反向/TEST↔EXPERIENCE 清理，以及旧四列、半波、固定 share 三类失败反例；动态 share 未证明不得标 `READY_FOR_ARCHITECT`。这些仅为设计审查口径，本轮未改测试文件、未执行测试或设备。2px 已定局部 heavy 边界位移，不擅自改口径。

## 全通道审计闭环

算法 §6.16 已补全“逐函数可达增量审计”的结论：bottom-only 下界不能外推为全物理类无解；但现有标量预算/速率也不足以构造合法 target。因此当前没有可交 architect 的 v2.9 方案。manager 需二选一：终止 bottom-only 路线，或授权全通道可达性账本/解析上界后再回 designer。全通道路线的后续证据契约、tan30/tan4 区分、实际 `bh` release 与局部 heavy 边界 2px 口径已写入算法 §6.16.1–§6.16.4。状态不变：`DESIGN_PENDING_HIL / BLOCKED-V29-ANGLE / BLOCKED-V29-RELEASE`。
