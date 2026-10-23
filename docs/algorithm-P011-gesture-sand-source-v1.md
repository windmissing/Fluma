# P-011 手势外部沙源注入算法设计 v1

日期：2026-10-23。责任：designer。状态：**DESIGN_ONLY / ARCHITECT_REVIEW_PENDING**。依据：用户 08:54 要求“手交互滑动时往下面掉沙子”；manager 08:59 裁决“滑动不倾斜，手机转才倾斜”。本稿不改代码、测试或既有硬阈值。

## 1. 现状和选型

`WaterSandField.inject(WaterSandInjection)` **已经存在**：按列区间均分单相 `amount`，可写 `suspH/suspL` 或 `mh/ml`，事务失败回滚，成功后重置 `initialWater/Heavy/Light`。它是离散场景事件，不具备双相原子配比、按时间连续注入、容量部分准入和保水语义。`SandCanvas` 的 `PanGesture` 目前仅在 TEST 且 `canP011GestureOwnInput` 为真时以 `offsetX×0.4` 写 `tiltX`；长按 400 ms 切换 TEST。新功能应沿用 field 的原子事件思想，增加**专用双相外部沙源事务**，而非在页面连续调用两次旧 `inject()` 或直接改公开数组。

**交互裁决固定**：用户可见 Pan 只用于水沙画布的掉沙；用户可见倾斜只读真实手机传感器。TEST 仍由 script/Want/schedule 或内部测试钩子模拟姿态，不依赖真实 Pan。400 ms 长按 TEST 开关保持。非水沙模式也不得继续将用户可见 Pan 解释为倾斜；该模式若需其它手势行为应另行定义。这里的“传感器常开”指 EXPERIENCE 的姿态来源；TEST 的姿态来源仍是测试命令，以免自测被设备姿态污染。

## 2. 外部源的单位、路径和配比

使用现有**等效列高体积单位**，不标作 mg：`1 unit = 一列中 1 个模型高度的沙体积`；`N=128, H=192` 为当前常用几何。配置候选（待效果定标，不是冻结值）：`gestureSandRate=8 unit/s`、`gestureSandRadius=2 列`、`gestureSandHeavyShare=0.5`；建议可调验证域分别为 `[0,30] unit/s`、整数 `[1,6] 列`、`[0,1]`。固定配比作为默认源材料身份；**不继承现场沙比例**，因为现场可能为空、随分选变化或不同位置不同，无法给同一手势稳定语义。面板显示“掉沙速度 / 范围 / 重沙比例”，注明速度是模型体积速率，不是质量或物理粒径。`heavyonly/lightonly` 预设的初始库存不强制外部沙源配比；如产品要“仅此相”应由显式源预设实现，不能暗中按库存改配比。

画布局部坐标 `x` 映射连续列坐标 `u=clamp((x-boxX)/boxW×N,0,N−ε)`；触点只在画布有效区受理。每个有效 Pan 事件间隔 `Δt` 以单调时钟计，限制单次累计 `Δt≤1/30 s` 防后台/掉帧补喷；本次总请求 `Q=rate×Δt`。沿前后触点组成的线段按不大于 `radius/2` 列间距分样，把 Q 按段长分给样点；每样点以紧支撑三角核 `k_c(u)=max(0,1−|c−u|/(radius+1))` 分配到列，**先对有效且有水的列归一化**，边缘不丢量；若样点支撑内无湿列，该份请求直接拒绝，不跨远距离搬到别处。所得已路由请求 `q[c]≥0`，其总量不超过 Q；`qH[c]=share×q[c]`，`qL[c]=(1−share)×q[c]`。与帧率无关的主口径是手势**有效运动时长**，不是事件数或累计位移；停住超过一个事件窗口即停止请求，不形成“按住自动灌沙”。Pan 首帧只建起点，移动后才计量；结束/取消清队列，不补尾量。坐标系和 ArkUI 事件字段由 architect 在页面实际 canvas/缩放下核对。

以候选值粗算：持续有效滑动 1 秒请求 `8 unit`，若集中在 5 列则均量约 `1.6 unit/列`；显示增量按 `boxH/H` 换算，最终入沙面量还受沉降、预算和 guard 控制。该数字只证明请求尺度清楚，**不是可见像素验收结论**。快划越长，单位路径沙量越薄；主观效果若不足，应按真实手势时长/路径宽度定标，不在实现中临时按触点事件数增发。

## 3. field 原子事务、保水与安全

新增语义接口建议：`injectGestureSand({ perColumnHeavy, perColumnLight, sourceId, inputTime }): InjectionReceipt`，在每次物理子步前消费页面积累的外源请求；单次提交应包含两相、容量、水面调整和账本回写。不得把 `HEAVY`、`LIGHT` 两笔旧 `inject()` 分开提交。只在有水的 `MODE_WATERSAND` 列注入 `suspH/suspL`；干列/`nowater` 不伪称有“水中下落”，本版拒绝并回报 `NO_WATER`。不直接写 `mh/ml`，因直接增加 `S=sandTop` 可能在事件瞬间越过 `tan30`，绕开逐 writer guard。悬浮注入使 `ΔS=0`；后续 `settleSandV2→percolateHeavyConservative` 仍用现有预算、守恒与 §19.5 guard，受阻时留在悬浮态，不偷改表面。

令提交前 `Vw=Σmax(0,waterTop[c]−S[c]−suspH[c]−suspL[c])`。对全部请求统一取最大可准入比例 `λ∈[0,1]`，实际 `aH[c]=λqH[c]`、`aL[c]=λqL[c]`，以保持两相配比。提交后找水面 `waterTop'[c]`，满足 `S'=S`、`0≤waterTop'[c]≤H`、`suspH'[c]+suspL'[c]≤waterTop'[c]−S[c]`，并且

```text
Σmax(0,waterTop'[c]−S[c]−suspH'[c]−suspL'[c]) = Vw
VH' = VH + ΣaH[c] ; VL' = VL + ΣaL[c]
```

给定试探 λ，令 `T[c]=S[c]+suspH'[c]+suspL'[c]`；在原水面形状上用共同抬升量 `η≥0` 构造 `waterTop'[c]=min(H,max(T[c],waterTop[c]+η))`，对单调函数 `Σ(waterTop'[c]−T[c])=Vw` 求 η。该构造只做外源排水所需的整体抬升，不借一次注入瞬时改造成目标倾角水面；后续水面 leveling 再按既有速率工作。触顶/局部湿容量/求解失败时缩小 λ 并重解，若 `λ≈0` 则回报 `CAPACITY_BLOCKED`、无状态变化。`Σ(H−waterTop[c])` 是必须满足的**全局必要**余量界，局部湿容量和水面可行性还需逐列验证；不能仅凭此和全局总量判断成功。事务先在 shadow/backup 上做相量、水面、finite/range、体积和 `tan30`/planner 审计，全部通过后原子提交；失败无局部写入、无耗尽后继续累计的“欠沙债”。外源注入不扣 `totalMotionBudget=300/s` 或 bottom 预算，因为它不是场内搬运；**新注入后发生的沉降/滑移/蠕动照常扣预算**，不提高原阈值。若当前已违规，事件只能在 `ΔS=0` 且不增加违规边的前提下受理；不得借外源豁免已有违规修复。

**守恒账本**：保留 run 初始值 `Vw0,VH0,VL0`，增加累计外源 `IH,IL`。每笔已准入量才写入 `IH/IL`；期望值 `expectedW=Vw0`、`expectedH=VH0+IH`、`expectedL=VL0+IL`。漂移按“实际总量−期望值”计算，不能把增加沙量报作守恒失败，更不能像现有通用 `inject()` 那样把**所有**基准改成当前总量来掩盖既有漂移。当前 `initialHeavy/Light` 可作为兼容缓存同步为上述期望值，`initialWater` 保持初始值；审计日志仍需明确 `initial` 与 `sourceAdded`。`depositedHeavy/LightFraction` 的分母取**当前累计期望沙量**，且显式标记 `sourceEpoch`；`t90` 是静置再沉降指标，注入期间/刚注入后须作无效或新 epoch 重算，不能沿用注入前首次达标时刻。TEST/EXPERIENCE 记录同一注入协议；原无外源的 P-011 用例维持 `IH=IL=0`。

## 4. 姿态输入与生命周期交接

- `PanGesture` 的用户事件只产生沙源路径；删去/失活 `tiltX=offsetX×gestureSensitivity`、`gestureHoldUntil=now+2500`、Pan end 的 `useSensor` 赋值。`canP011GestureOwnInput` 不再是 Pan 沙源门控；要按画布命中、场景和源容量门控。EXPERIENCE sensor 和 TEST script 的倾角选择维持 `P011InputMode` 的模式隔离；`selectP011Input` 中的 `gesture` 来源及 TEST 空命令回零由 architect 迁移为明确的 `idle/script`，不能留下可见 Pan 倾角路径。`P011TiltWant`/schedule/forceInvert 测试入口可保留或扩展。
- 400 ms 长按优先完成 TEST 切换，不产生沙源；若触摸已被识别为 Pan，长按不切换。需在手势竞争结束时保证二者恰有一个生效，并沿用浮层面板的 hit-test 隔离，避免在调参面板滑动时向画布注沙。
- 手势结束/取消、模式切换、场景 reset、页面离开、容器重建、180° 反转均清未消费路径和时间积分；已提交的沙与 `IH/IL` 随物理场保存，reset 才清外源账本。反转后下一次手势按新重力底部方向由场内悬浮自然沉降，不重放旧路径。
- 每笔外源事务带 `sourceId/sequence/inputTime/requested/admittedH/admittedL/rejectedReason`。物理快照/遥测须标记外源边界，P-012 前后相库存配对在注入处不能把新增量当旧材料平流；现有 renderer 约定 `inject` 断配对。连续手势会频繁触发该边界，architect 应给出**注入感知的连续纹理方案**或明示源活跃期间降级的可见成本，不能直接删掉相量校验以保画面流畅。无外源原基线与含源 run 分开统计，观测③/SLIP 的封闭守恒分母不得混入注入 run。

## 5. 可见“掉落”的能力边界与验收建议

现有 canonical ledger 只有每列悬浮总量 `suspH/suspL`，**没有粒子纵向位置**。因此本版能严格保证“手指经过处新增沙源 → 同列悬浮增加 → 在既有沉降速度、预算和 guard 下逐步进入沉积层”，不能仅凭 `inject(suspended)` 证明有从触点到沙面的可跟踪细沙流。若用户要求清楚看到空中下落轨迹，需在 architect 评审时二选一：新增守恒的物理在途库存/高度态并扩展 snapshot，或渲染层读取已准入源事件绘制短暂喷流且**不把视觉轨迹计作额外沙量**；后者须遵守 P-012 外观只读和 checker 隔离。不得用未准入请求画出沙，也不得以视觉喷流替代最终沉积。

建议验收场景：① 1 秒匀速划过水沙区，源事件总准入量与 `VH/VL` 增量逐相比对，局部先见悬浮、后见沉积；② 多次重复直到满容时逐渐部分准入/停注，水量漂移仍 `<0.1%`、无越容/非有限/新增 `tan30` 违规；③ 传感器倾斜与 Pan 同时发生，Pan 只改源量、姿态只由传感器改，TEST script 倾角链继续可用；④ 长按、面板、画布外、取消、模式切换不误注；⑤ 有源与无源分别验证 P-012 纹理连续性及帧时。速度/半径/配比范围和“可见下落”主观门槛在效果试采后冻结，不把上述候选数值冒充通过标准。

交 architect 需重点审：保水水面求解在当前 `WaterSandField` 中的可复用入口、双相原子提交及 `λ` 可行性、P011 手势来源删除后的 TEST 回零、P012 高频 source epoch 配对，以及纵向掉落视觉选型。未审定前状态维持 `DESIGN_ONLY`。
