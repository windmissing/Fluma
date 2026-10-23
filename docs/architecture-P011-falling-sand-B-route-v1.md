# P-011 掉沙 B 路线（渲染喷流 tracer）工程设计 v1

日期：2026-10-23。责任：architect。状态：**ENGINEERING_DESIGN / READY_FOR_DEVELOPER（挂起中——随掉沙链启动）**。依据：[三路线设计 §3/§5](design-P011-falling-sand-vertical-routes-v1.md)（manager 10:26 裁决 B 首版+C 后备）、[手势沙源注入设计](algorithm-P011-gesture-sand-source-v1.md)（已验收，**物理层尚未实现**——receipt 从零定义）。纯设计零代码；不改物理实现/冻结锚点；数值候选全部标「待效果定标非冻结」。

## 0. 前置依赖声明（重要）

B 的输入是手势沙源的 `InjectionReceipt`——**物理层注入事务（algorithm-P011-gesture-sand-source §3）尚未实现**（仓内 grep 零命中，2026-10-23 核对）。因此本设计书把 receipt 结构定义为**B 与物理层的共享契约**：物理层实现任务（另册）产出 `injectGestureSand` 时按本 §1.2 的 receipt 字段交付，B 的 tracer 只消费该结构。若物理层先实现且字段有出入，以物理层为准回改本节——**两侧不得各自发明 receipt**。

## 1. tracer 数据通路（工作 1）

### 1.1 实现位置：独立文件 P012SandTracer.ets（renderer 旁路，不进 WaterSandField）

裁定理由：tracer 是纯视觉对象（无体积零库存零预算），与物理域必须物理隔离（源设计 §3 红线「不写 WaterSandField、不进 checker/物理遥测」）；与 renderer 同级放 render/ 目录，只 import RenderTypes 与 receipt 类型。

### 1.2 InjectionReceipt 契约（与物理层共享）

```ts
// P011GestureSandSource.ets（物理层文件，本节只定契约）：
export interface P011InjectionReceipt {
  sourceId: number;        // 每 Pan 手势会话一个（手势开始时递增）
  sequence: number;        // 会话内事件序号（单调）
  admittedH: number; admittedL: number;   // 实际准入量（unit）；双相同刻原子
  requestedH: number; requestedL: number; // 请求量（部分准入缩放用）
  rejectedReason: string | null;          // NO_WATER/CAPACITY_BLOCKED/null
  x: number; y: number;    // 触点画布坐标（显示 px，页面坐标系）
  inputTime: number;       // 单调时钟 ms（与旅行时间基准同钟）
  seed: number;            // 每事件固定种子（事件身份，tracer 复现性）
}
```

**admitted=0（含 rejectedReason 非 null）→ 不生成 tracer**——硬红线，源设计 §1/§3 一致。

### 1.3 数据流与环形队列

```text
Pan onActionUpdate（页面，仅 MODE_WATERSAND+画布命中+湿列区域）
  → 路径分样+q[c] 路由（物理层算法 §2，页面侧累积）
  → 每物理子步前 injectGestureSand(...)（物理层事务）
  → receipt 返回 → SandCanvas push 到 P012SandTracer.feed(receipt)
  → tracer 生成视觉点（数量∝admitted，见 §2.2）
  → render() 帧回调：P012SandTracer.draw(ctx, view, heightToY, gravity)
```

**环形队列**：容量 64 receipt（每手势事件一条；Pan 事件 ≈60Hz，64 条 ≈1s 窗口——超龄先出）。**过期不重放**：`feed()` 只在事件发生时调用（push 即消费，页面直接持有 tracer 引用），队列的意义是**限界防积压**而非缓冲重放；页面切后台/掉帧恢复后队列里的旧 receipt 照常衰减（按 inputTime 相对当前帧时间算进度），进度 ≥1（已过旅行时间+寿命）的自动剔除——**天然不重放**，无需显式时间闸。

**延迟控制**：receipt 生成（物理子步边界）到 tracer 可见（下一 vsync 帧）延迟 ≤2 帧（33ms）——Pan 60Hz 事件流下喷流起点紧跟指尖，无需更精细同步。

## 2. tracer 运动与遮蔽（工作 2）

### 2.1 重力方向显示坐标变换（含 180° 反转）

tracer 的运动方程在**显示坐标系**解：`dir = R(effectiveTilt) · (0, +1)`（显示重力单位向量，R 为倾角旋转）。现有 renderer 的 heightToY 已处理显示映射；方向向量由页面传入（`frameContext` 扩展 `gravityDirX/Y`，从 SandCanvas 的 tiltX/forceInvert 派生，与传感器同源——**不建第二姿态源**）。180° 反转（forceInvert）：方向向量取反；**在途 tracer 不做世界坐标重投影**（源设计 §3「按同一显示重力变换重投影」）——实现为**反转时刻全部在途 tracer 立即清空**（§4 生命周期），新事件按新方向生成。理由：反转是低频事件，在途点重投影的连续性收益小于状态机复杂度；C 后备的真粒子才需要重投影语义。

### 2.2 视觉点生成与旅行时间

每个 receipt 生成 `n = clamp(round(k × (admittedH+admittedL)), 1, 24)` 个视觉点（k 候选 2.0/unit，**待定标**）；点出生位置 = 触点 ± 抖动（seed 驱动，±radius/2 列宽内）；初速 = 显示重力方向 × (vPhase + 抖动)，vPhase 按相身份取 `heavy/lightSettleSpeed` 的显示量纲（候选 64/38 unit/s 的 px 换算，**待定标**）。**旅行时间**：`tTravel = clamp(depth / vPhase, 0.3, 2.5)`，depth = 起点到该列 `sandTop(x)` 的显示距离（连续采样 P012MaterialColumnSampler.sandTopAtPx——复用既有 sampler，禁第二插值实现）；两相不同速（H 快 L 慢）→ 同 receipt 的点分相生成。到达 sandTop 后进入 `fade` 态（0.15s 淡出，候选）；**接触闪烁不写库存**（源设计 §3）。

### 2.3 三段遮蔽裁剪（逐帧采样实现点）

绘制时每视觉点按当前位置 y 采样三界（全部连续采样，sampler/heightToY）：

```text
1. waterTop(x)：y < heightToY(waterTop) → 完全裁剪（水面上方不可见——出生即在水下，正常不触发）
2. 界面雾带：|y − heightToY(sandTop)| < halfW（P012_A_WFULL_PX/2）→ alpha × smoothstep 衰减到 0
   （进入雾带的点渐隐——与 E3 界面带同一几何，天然不冲突）
3. sandTop(x)：y ≥ heightToY(sandTop) → 完全不可见（沙 mask 内，到达点已在 §2.2 转 fade）
```

遮蔽在 draw 循环内逐点判定（O(Nvis)，无位图）；**帧间不缓存**（sandTop 每帧可变，缓存即过期风险——E3 LAYER_EPS 教训）。

## 3. 渲染层接入（工作 3）

**绘制次序（裁定：插在 B 颗粒之后、液面高光之前）**——render() 的 3.5) 与 4) 之间：

```text
… drawSandGrain（B 颗粒，沙区）
→ drawSandTracers（新增 3.75）：水体域内的下落点
→ 液面高光 → drawInterfaceBand（A 界面雾带，最后盖住雾带内的残留点）
```

理由：tracer 全程在水区（遮蔽 §2.3 保证），画在沙层之上不遮挡沙（点在水区）；A 界面带最后绘制使雾带衰减自然（2.3-2 的带内点被雾盖住一半，符合「进入雾带渐隐」）。

**draw 实现**：≤256 活跃点上界（超界时新点降采样生成——按 seed 跳点，不随机丢弃已生成的）；**同色合并**：H/L 两相各一个 Path（fillRect 1-2px，候选）——每帧 ≤2 draw call（H 群+L 群），点内 alpha 用 globalAlpha 分档（≤3 档，候选）。**E3/B/C 锚点不冲突的约束**：tracer 只画在水区（沙区被遮蔽裁剪），B 颗粒的沙区 std/hfH 锚点不受影响；E3 雾 std 的测量窗（雾带 rows 950-1100）可能含少量带内衰减点——**验收录像时 tracer 开关配对**（外观输入版本化，§3.1），锚点测量在 tracer OFF 臂跑。

**外观输入版本化**：`frameContext` 扩展 `sandTracerEnabled: boolean`（默认 true，产品档；want 参数 `p012ablate` 增 `tracer` 枚举——OFF 臂与消融共用现有机制）；tracer 不进 P012AppearanceParams 六参数（它是功能开关不是风格参数）。

### 3.1 成本上界

Nvis≤256 × O(1) 逐点更新+遮蔽采样（3 次 sampler 调用/点）+ ≤2 draw——**预算 ≤0.1ms/帧候选**（对照：B 颗粒 drawSandGrain 3 带全量 ~0.5ms）。真机 P95 进 effector 录像验收（§6）。

## 4. 生命周期（工作 4）

| 事件 | 动作 |
|---|---|
| reset（场景重置） | 清全部 tracer + 环形队列 + sourceId 保留（事件身份连续） |
| invert180 / forceInvert | 清全部在途 tracer（§2.1 裁定）；队列同步清 |
| 模式切换（TEST↔EXPERIENCE / renderMode 变更） | 清全部（tracer 属水沙画布外观，跨模式无意义） |
| 页面 aboutToDisappear | tracer 实例随 renderer dispose（无引用即回收）；队列清 |
| 手势结束/取消（onActionEnd/Cancel） | **清「未消费路径积分」**（物理层语义，页面侧）；已 feed 的 receipt 不回收（在途喷流自然走完——视觉上"抬手后余沙落完"是正确行为） |
| 注入被拒（admitted=0） | 不生成（§1.2 红线）；若 UI 出现喷流=路由 bug，单测断言 |

**canP011GestureOwnInput 门控改造**（与物理层注入设计的交接）：现有 Pan→tiltX 映射按源设计 §4 将被删除（Pan 不再倾斜）；`canP011GestureOwnInput` 退役为无沙源路径下的防御断言（非水沙模式 Pan 无任何效果）。**沙源门控新条件**：`renderMode===MODE_WATERSAND && 画布命中 && !panelHitTest`——与 P-011 输入模式隔离（TEST script/schedule/传感器的倾角路径零改动，长按 TEST 切换与 Pan 的手势竞争按源设计 §4 二选一规则）。

## 5. C 后备预留（工作 5）

| B 产物 | C 启用时 | 理由 |
|---|---|---|
| **InjectionReceipt 契约（§1.2）** | **保留**——C 的真粒子准入同样需要事件流（sourceId/sequence/admitted 是物理账本输入） | receipt 是物理层契约不是 B 私有 |
| **遮蔽裁剪三段式（§2.3）** | **保留**——C 粒子的渲染同样要 waterTop/雾带/sandTop 遮蔽（真粒子在水下同样不可见） | 遮蔽是显示层通用逻辑 |
| **生命周期钩子（§4）** | **保留**——reset/invert/模式切换清粒子池与清 tracer 同一触发面 | 触发点复用 |
| **p012ablate=tracer 开关** | **保留改义**——C 下变为「真粒子渲染层开关」（消融真粒子视觉，物理库存不受影响——C 的粒子是物理对象但绘制仍是外观） | 消融机制通用 |
| **P012SandTracer.ets 的运动积分（§2.2）** | **废弃**——被真粒子物理替代（C 的粒子在物理层积分，有质量守恒） | B 的核心即弃件 |
| **旅行时间定标/视觉点抽样** | **废弃**——C 无需"代理旅行时间"（真粒子真实下落） | 同上 |

升级路径：C 实现时 P012SandTracer 整文件删除，遮蔽/开关逻辑移入 C 的粒子渲染模块——**预计 C 复用 B 约 40% 代码量**（receipt 消费链+遮蔽+生命周期），运动与生成层重写。

## 6. developer 任务书（工作 6）

### 6.1 实现拆解（建议顺序）

| # | 模块 | 内容 | 依赖 |
|---|---|---|---|
| D1 | P012SandTracer.ets | 视觉点数据结构（SoA：posX/Y/velX/Y/phase/age/fadeSeed Float64Array×N=256）+ feed()/update(dt, gravity)/draw(ctx, view, heightToY) | receipt 契约定稿（本节） |
| D2 | renderer 接入 | render() 3.75 槽位 + frameContext.gravityDir/tracerEnabled 扩展 + ablate=tracer 枚举 | D1 |
| D3 | 页面接线 | Pan 沙源路由（物理层实现后）→ receipt→feed；生命周期钩子（reset/invert/切换/dispose） | 物理层注入任务 |
| D4 | 单测+录像 | §6.2/§6.3 | D1-D3 |

**注意**：物理层注入（injectGestureSand）是独立任务（另册任务书）；B 的 D1/D2 可先行（用合成 receipt 单测），D3 等物理层就绪。

### 6.2 单测范围（Node 通道）

1. **tracer-receipt 逐笔对应**：feed 合成 receipt（admitted>0）→ 视觉点数 = clamp(k×admitted)；admitted=0 → 零点生成（三种 rejectedReason 各一例）。
2. **部分准入缩放**：requested=10/admitted=3 → 点数按 3 算。
3. **遮蔽正确性**：构造 sandTop/waterTop 已知场 → 点在沙内 alpha=0、雾带内平滑、水区正常（逐段断言）。
4. **旅行时间**：depth/vPhase 落在 [0.3,2.5] clamp（三个边界用例）。
5. **生命周期**：reset/invert/切换后 activeCount=0；手势取消不回收已 feed 点。
6. **上界**：连发 100 receipt → 活跃点 ≤256。
7. **同色合并**：draw 调用计数 ≤2（H/L 群各一）——用计数 ctx 桩。
8. **两相速度**：H 点先于 L 点到 sandTop（同 depth）。

### 6.3 验收场景（effector 录像口径，manager 裁决 B 是否合格的依据）

| # | 场景 | 采集口径 |
|---|---|---|
| V1 | **静置掉沙**：画布水平，1s 匀速划过水区 | 喷流起点贴指尖、轨迹竖直、抵达沙面后消失；**错时测量**：tracer 到达时刻 vs 同列 settle grant 首次非零时刻（ledger 侧）的时差逐列记录 |
| V2 | **倾斜掉沙**：手机倾 30°，同手势 | 轨迹沿显示重力（非竖直）；错时同测 |
| V3 | **满容拒注**：持续注沙至 CAPACITY_BLOCKED | admitted=0 后**零喷流**（录像目视+ledger 对账） |
| V4 | **雾带交界**：划过沙面附近（触点接近 sandTop） | 出生即近沙面 → 旅行时间短；雾带内渐隐不突兀；无穿沙长线 |

**判定输入**：V1-V4 的错时数据+主观描述（沿 SV 量表不新立线）→ manager 裁决「视觉落地与真实沉降错时」可接受性；不可接受则触发 C 升级评估（C 物理实现等 108run 统计提取后另开批——不在本任务书范围）。

### 6.4 红线复述（developer 须逐条自查）

tracer 无体积零库存零预算（不写 WaterSandField 任何字段/不进物理遥测/checker）；admitted=0 零喷流；不改冻结锚点（E3/B/C 锚点测量在 tracer OFF 臂）；P-012 外观只读+物理零触碰；数值候选（k/vPhase/衰减/点径/上限）全部标「待效果定标非冻结」；108-run 批内只在**隔离外观构建**试验（不动正式批二进制）。

变更记录：2026-10-23，依据 manager【新需求·掉沙·B 首版工程设计】任务产出。六项工作落盘：数据通路（独立文件+receipt 共享契约+环形队列限界不重放）/运动遮蔽（显示系运动+反转清空裁定+sampler 三段遮蔽）/渲染接入（3.75 槽位+≤2 draw+ablate 机制）/生命周期六钩子+C 后备保留表（receipt/遮蔽/生命周期保留，运动层废弃，~40% 复用）/developer 任务书（D1-D4+8 单测+V1-V4 验收）。前置依赖：物理层注入事务未实现（receipt 契约已代定义，两侧共享）。
