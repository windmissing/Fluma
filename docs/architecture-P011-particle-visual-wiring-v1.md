# P-011 路线 C 粒子视觉接线架构稿 v1

日期：2026-10-24  
状态：DESIGN / D4-D5 输入稿 / NOT IMPLEMENTED / NOT VALIDATED  
范围：只读渲染设计；不授权 D4/D5 实现，不占真机。

## 0. 输入与边界

本附件按 [P-011 路线 C 双场粒子架构 rev2](architecture-P011-dual-field-particle-v1.md) §1.2、§2.2、§5、§5.6、§6、§7、§9、§13.2、§13.5 执行；使用现有 `P011ParticleField.ets`、`P012SandTracer.ets`、`P012WaterSandRenderer.ets`、`P012WaterSandAdapter.ets`、`P012RenderTypes.ets`、`SandCanvas.ets` 作为接线参考；P-013 P1 材质/合成契约以 [P-013 P1 工程设计](architecture-P013-p1-material-composite-v1.md) 为准。

D1 B 报告当前为 `NOT_ALL_GREEN / NOT_D1_ACCEPTED`，D2 WaterSandField 生产接线及 v7 checker 尚未交付。因此本文先固定通用渲染接口、路径推荐、显示语义与互斥约束；凡需 D2 的实际输出、原型观感或真机帧时证据，逐项登记责任人和门序，不伪造已验证结论。

硬边界：渲染只读；不改粒子状态、质量、route、guard、预算、冻结锚点或预注册 estimand；Nmax、粒子速度、寿命、粒径、色彩数值均不在本稿定标；B tracer 视觉点不作为 C 的质量、守恒或落床证据。

## 1. 粒子位置数据与只读投影

### 1.1 物理坐标就是渲染输入

M1 SoA 已存连续列坐标 `pX`、材料高度 `pY`、速度 `pVx/pVy`、质量 `pMass`、相别 `pPhase`、状态 `pState` 和稳定粒子 ID `pId`。`pX` 是列坐标（0..cols），`pY` 是材料坐标中的高度；并非“整数列+列内高度”丢精度表示，也不是像素坐标。渲染按当前 `P012FrameContext` 的同帧 transform 将连续 `(x,y)` 映射到 view pixel，严禁把物理位置回写或在 Canvas/GLES 中另积分。

粒子池不存 birth time、age 或 per-particle seed。因而 C 路径能表达当前真实位置、速度方向、相别、状态及稳定身份，但**不能**从池快照推出真实飞行时长、淡出龄期、按粒子重放的确定性轨迹或从手势出生到落床的粒子级耗时。禁止用 `particleId`、`sourceId` 或 receipt 时间差伪造粒子 age。v7 SPAWN/LAND journal 可作同帧诊断事件来源，不替代位置快照，也不建立缺失的持久年龄字段。

### 1.2 渲染投影契约

D2 为 `WaterSandField` 增加只读、提交后投影入口；适配器不获得 `P011ParticleField` 可变实例：

```ts
export interface P012ParticleRenderSnapshot {
  readonly physicsCommitId: number;       // 粒子/床层/journal同一原子提交身份
  readonly particleRevision: number;      // 每次成功的池发布递增，不用 substep 猜版本
  readonly routeId: string | null;
  readonly active: boolean;              // 从D2唯一物理门快照复制，页面不重算
  readonly count: number;                // 活粒子数；0 <= count <= capacity
  readonly capacity: number;             // 实际池容量，当前候选不是冻结值
  readonly cols: number;
  readonly x: Float64Array;               // 连续列坐标，前count项
  readonly y: Float64Array;               // 材料高度，前count项
  readonly vx: Float32Array;              // 列/秒，仅方向/调试可读，不用于render-side积分
  readonly vy: Float32Array;              // 材料单位/秒
  readonly mass: Float64Array;            // 真实质量，仅诊断/大小候选读取，不改池质量
  readonly phase: Uint8Array;             // 0 heavy, 1 light
  readonly state: Uint8Array;             // 1 FLYING, 2 CONTACT_WAIT
  readonly particleId: Uint32Array;        // 诊断关联，不作age或绘制顺序随机种子
}

copyParticleRenderSnapshot(out: P012ParticleRenderSnapshot): P012ParticleRenderSnapshot;
```

这是接口形状而非现存实现。生产约束如下：数组由 renderer/adapter 一次预分配至池容量并跨帧复用；每帧只拷贝前 `count` 项，`count` 之后的数据无效。对外视图只读，任何调用方都不得保留并写入 backing arrays。M2 在物理提交完成后、`buildP012FrameContext` 之前生成同一 `physicsCommitId` 投影；不得从 collect 的 scratch、半提交事务、第二次 collect 或过时 snapshot 绘制。单 UI/渲染线程可同步复制；若物理线程与 UI 线程分离，M2 发布双缓冲并原子切换完整 `(physicsCommitId, particleRevision, arrays, count)`，renderer 只读已发布 buffer，不能逐字段读活动池。

`P012FrameContext` 增 `renderFrameId`、`particleSnapshot` 与 `particleLayerEnabled`（默认 false）；`renderFrameId` 由 renderer 每次建帧递增，`physicsCommitId` 标识物理已提交快照。粒子投影与床层/sampler 必须带相同 `physicsCommitId`，由一个 render frame 一次性消费；不得要求 D2 猜测渲染帧序号。`particleSnapshot.active` 是唯一身份，want 参数只请求启用/关闭视觉层，不能构造 route active。`active=false` 时 snapshot count 必为0或被 renderer 忽略，且走现有 B 路径；禁止页面按 want 字符串重算 route。snapshot 由 D2 提供 coherent state，v7 `particles.byState` 和七类 journal 提供 checker/诊断关联：渲染计数与 `FLYING + CONTACT_WAIT` 数可对照，但绘制点数不回写或替代账本字段。

M1 已有 `snapshot()` 会复制完整 SoA 并分配；M4 不应把该诊断快照每帧调用。应由 D2/M2 提交边界提供预分配投影入口；若 D2 当前实现无法提供，D4 首版先使用单一复用的 adapter FrameSlot 完成一次 copy，禁止每帧 `slice()` / 对象数组映射 / JSON 序列化，帧时与分配事实登记后交 manager，不把“有界 Nmax”说成零开销。

## 2. 渲染路径比较与推荐

| 项 | 路径甲：P-012 Canvas2D 粒子层 | 路径乙：P-013 GLES 粒子 pass |
|---|---|---|
| 现状与落点 | 复用现有 P012 adapter/renderer、Canvas2D layer slot 和当前分层主画面；增加 `P012ParticleLayer.ets`，按同帧只读投影绘制 | 需在 P013 P1 完整沙区 composite 之外增粒子 pass、同步 frame identity/route/snapshot/context epoch，并接入 P1 合成 controller |
| z 顺序 | 主 Canvas 内由 renderer 明确置于水/雾/沙面及容器前景之间；粒子不应覆盖容器边框/交互控件 | 必须与完整沙区 pass、透明 surface、前景 Canvas 和页面 UI 共同定序；P1 的三 surface/sRGB 能力是前置 |
| tracer | 使用同一 active route 互斥快照；C active 只画真粒子并停 feed/draw tracer，C inactive 继续 B | 同样遵循 §6 互斥，但不能让 GLES 粒子与 Canvas tracer 在相邻 surface 上重叠 |
| 帧时 | 512 候选下 CPU 遍历并画点，调用开销可能随 count 增长；可用分相 path/batched primitive，真实 P95/GC 必须真机测 | 可由 GPU 批量实例化/点精灵，CPU 遍历与 draw 数可能更低；增加 GLES 上传/同步/透明过绘与合成 pass 成本，不能以 GPU path 假定更快 |
| 色彩 | 与现有 Canvas 主视觉同栈；粒子相色/alpha 必须在 Canvas 输出色域内定义，并同 B/C 材质资产保持可比 | 若并入 P1，同样走 P1 的 EOTF→材质/光照→ToneMap→sRGB 输出规则；不能用 P0 `DIAG_GRAY_V1` profile 画产品粒子，也不能遗漏 P1 色彩版本/attachment 校验 |
| 失效回退 | 绘制异常可跳过 C layer 并报错；若是资源/适配器整体失效，按 route 状态保持“本帧不画粒子”，不能擅自把已准入库存喂给 tracer 补画 | context/surface/profile/epoch 失效需按 P1 controller 完整回退；必须原子切换责任，避免 C 粒子层缺帧时又画 tracer 重复显示 |

**推荐：路径甲作为 D4/D5 首个真实粒子呈现路径。** P-011 当前运行中的产品画面及 sampler/遮蔽/层序都在 P-012 Canvas2D；D4/D5 的责任是把已提交 M2 状态忠实呈现，不需要把物理路线再绑定到尚待实现/真机验证的 P-013 P1 surface 生命周期。路径甲保留 B 旧画面的配对和统一 Canvas frame identity，失败面与验证变量较少。粒子数上界虽有限但不是帧时证明，所以进入产品默认前仍须 D6 真机帧时和 effector 双报告。

路径乙不是本卡首选。待 P1 完整 composite 已实装、G0 surface/sRGB/透明合成前置通过、并行 Canvas/GLES 层序和恢复链已验收后，manager 可另开迁移评估；届时粒子像素走同一 P1 ToneMap/sRGB pipeline，消费同一 `physicsCommitId` snapshot，保留 adapter contract，只替换 renderer backend。不得在此之前把粒子临时画到 P0诊断 pass 或宣称色彩等价。

## 3. 可见性与落床表达

### 3.1 FLYING（state=1）

以 `x/y` 当前物理位置绘制小型实心沙粒；heavy/light 只用已登记相别映射不同的既有材质/色调，不用速度反推出未验收的粒径或粒子年龄。尺寸、alpha、颜色均是视觉候选（§9.7），不在本设计冻结。遮挡每帧读取现有材质 sampler：沙面以下被沙 mask 隐去，水面裁剪/雾带沿用 renderer 的逐帧采样语义；禁止缓存上一帧 `sandTop`。容器边框和前景 UI 最后覆盖。

### 3.2 CONTACT_WAIT（state=2）

`CONTACT_WAIT` 是尚未获准入床的真实在途库存，池质量仍存在；按实际 snapshot 的 `(x,y)` 贴近当前床面绘制为“落在表面的停留颗粒”，颜色/alpha可与 FLYING 有轻微状态区别，但不得先淡出、隐藏或删除。它仍计入渲染 count 与 v7 `byState.CONTACT_WAIT`，也继续随物理状态按 M1/M2 下一次提交更新。

### 3.3 真实入床（T3/LAND）

只有 M2 原子事务实际 grant 后，池质量按 grant 扣减、床层 mh/ml 增加；全额才移除粒子，部分 grant 后剩余质量/ID/state=2继续可见。绘制层不根据 `y <= sandTop` 自行删粒，也不把触碰帧等同于落床。成功提交后，从同一 `physicsCommitId` 的新床面 profile 重绘连续沙面，增长直接表现为沙堆抬高；落床粒子 sprite 同帧不再出现，避免双计/双画。若一帧粒子消失但床面未按权威物理快照变化，这是 renderer/物理快照不一致，记录诊断并提交 M2 修复，不制造残影补质量。

一帧纯视觉强调（例如刚落床位置短促高亮）若之后想做，必须仅由成功 LAND 事件触发、生命周期固定为渲染效果、无质量/遮挡逻辑副作用并列于独立 effect flag；本版不需要它作为验收条件。落床是否成功及 grant 数额只看 journal/checker，不看视觉高亮。

## 4. 手势到粒子的连续性与池上限

手势输入已有 display px 的 receipt `(x,y,inputTime,sourceId,sequence)`，物理 spawn 将准入质量转成真实粒子并以材料坐标初位置/初速写入池。画面映射在同帧沿用 `P012FrameContext` 的 display→material/material→display 变换。每颗粒子首次出现以第一次已提交 snapshot 中的池坐标为准；不从手势点另外生成第二批“视觉粒子”，不同时保留 receipt tracer 点。Touch cursor/手指反馈如需保留，作为独立 UI 输入反馈，不代表库存且不能形成落沙轨迹。

如触点在画布外、落在已沉积沙面内或水面外，出生位置钳制仍归 M2 的物理 spawn 规则；renderer 只映射该结果，不重演/修正。无法从当前池字段观察年龄，不能声明可调“飞行尾迹时长”；若视效将来确需粒子 lifetime，须 designer/manager 另定是否增加权威字段及守恒/快照代价，不能只加渲染本地计时伪造轨迹。

Nmax 512 为候选容量之一，不是默认冻结值/视觉目标。池满时物理 M1/M2 按已定合并/降级/拒绝事务处理；renderer 不丢真粒子来做视觉降级、不按 receipt 重建、也不缩减物理数目。渲染侧只能在可见 density 的绘制算法上做受控 batching/绘制合批；报告真实 `snapshot.count/capacity`、未绘制数和原因，默认完整绘制所有有效 snapshot 项。实际点径/绘制批策略由 D6 真机定标。

## 5. 模块、接口落点与责任

| 模块 | 文件/责任 | 接口与硬约束 | 阶段 |
|---|---|---|---|
| M1 pool projection | `physics/P011ParticleField.ets` | 提供只读拷贝到复用定容输出；字段为 x/y/v/mass/phase/state/id；不 import render，不公开可变 SoA | D2/M1 先完成生产 pool/snapshot 事务，再由 D2确认可投影口径 |
| M2 frame snapshot | `physics/WaterSandField.ets` | post-commit `copyParticleRenderSnapshot(out)`，携带 `physicsCommitId/particleRevision/routeId/active/count`；唯一物理route门仍由本模块负责 | D2原型出口前只定契约；实际projector和v7一致性随D2验收 |
| M4 adapter | `render/P012WaterSandAdapter.ets` | FrameSlot预分配并复用数组；一次复制形成不可变至本帧末的视图；关联同 `physicsCommitId` 的material sampler与snapshot | D4 |
| M3 particle renderer | 新增 `render/P012ParticleLayer.ets` | `draw(snapshot, frameContext, sampler, canvas)` 只读；批次按相别；不 update physics、不 store state、不消费事务 | D4 |
| renderer/z order | `render/P012WaterSandRenderer.ets`、`render/P012RenderTypes.ets` | 新粒子 slot 放在材质遮蔽与容器前景之间；真实 particle 与 tracer 仅选一组；ablation显示开关仅控制层 | D4 |
| page/want/lifecycle | `pages/SandCanvas.ets` | `buildP012FrameContext`从M2快照取得active；GESTURE receipt仅在route inactive时feed tracer；页面want可禁显示但不解除物理route；隐藏/恢复不清空M2池 | D5 |
| v7 telemetry/checker | WaterSandField事件快照与现有checker | 对齐 `particles.byState`、SPAWN、LAND、CROSS_COLUMN等journal；可诊断映射，不驱动绘制/物理 | D2先验收；D4加只读关联字段 |

绘制层序建议（Canvas主路径）：背景/容器底 → 既有水/雾与沉积材质绘制 → FLYING/CONTACT_WAIT 粒子层（按遮蔽mask处理）→ 容器前景/边框 → 页面交互/UI。具体实际 slot 必须由 D4 在 `P012WaterSandRenderer` 现有绘制序中登记一次，不允许页面在 Canvas 上另开一个叠加surface导致 z 序与输入区域分叉。

## 6. D4/D5 时序、共存及 tracer 退出

### 6.1 单帧时序

```text
物理 step 成功提交（M2）
  → publish field + particle state + journal/v7 summary 同一 physicsCommitId
  → renderer 建立 renderFrameId，adapter 取得该提交的唯一 route/snapshot（复用buffer）
  → buildP012FrameContext(renderFrameId, snapshot, current material sampler, wants)
  → if snapshot.active && particleLayerEnabled: draw真实粒子；skip tracer feed/draw
    else: draw现有 B tracer；不创建/绘制粒子层
  → 输出本帧呈现源/帧时遥测；不回写物理状态
```

### 6.2 并行期与默认行为

- 原型出口、D4/D5实现卡和 manager 放行之前：C 产品显示关闭；现有 B tracer 是唯一当前外观，行为不变。隔离 D2 Node prototype 不得提前接真实画面。
- D4/D5灰度验证：仅在 M2 `particleRouteActive()` 的已发布快照为 true 时切为真实粒子层。切换必须在 frame boundary 原子完成：`C active => particleLayer only; tracer feed=false and tracer draw=false`；`C inactive => tracer path only`。不得在 active 判断失败或 snapshot 过期时同时画两层。
- C `active=true` 但粒子 `count=0` 表示当前确实无活动粒子：该帧两层都不画，不回退 tracer。否则会复画已由粒子池呈现/已入床的库存。
- C route want/产品 visual ablation 关掉粒子显示时：只关闭屏幕显示，不卸 route、不清池、不改变账本，也不以 tracer 代画；记录 `presentedSource=NONE` / `particleLayerEnabled=false`。只有物理 route inactive 才由 B 路径承接后续输入。
- Gesture receipt 已确认 admitted 后，B `feed(receipt)` 只允许在同一提交身份 `active=false` 且 C未选的分支。拒绝注入或 admitted=0 的现有零 tracer 红线继续有效。C active 收到已准入库存时，只供 journal/sourceId 关联，不喂 B tracer。
- C 与 B 的库存语义不可桥接：B 点没有质量；C 粒子是真库存。不得让已存在的 B points 在切换帧上叠在 C 粒子上；切入 C 的边界帧清空 B 视觉点（无物理副作用）并记一次切换 reason。若 C 物理 route 暂时失效但池仍有库存，按 §13.4 不可静默卸武；renderer 应保持已有 C snapshot可见或呈现明确故障空画面并报错，不得把池质量改由 B tracer“补画”。

### 6.3 B tracer 退出的可判据与步骤

`P012SandTracer.ets` 只在 rev2 §6.1 三个条件**同时**满足后才可删除：

1. 路线 C 已通过隔离原型和真机双报告：checker守恒与 effector 主观报告分别通过；B录像不得替代 C 证据。
2. manager 明确裁决 B 退场，并确认 C 是该产品线路唯一显示源。
3. 粒子层 Nmax 档位定标完成，粒子容量/帧时/长稳风险被 D6 报告覆盖。

下线顺序：①先合并共享 sampler/三段遮蔽 helper，确保 C renderer 首消费者已接上逐帧 waterTop/fog/sand mask；②删除 D5 页面 `tracer.feed`、`sandTracerEnabled` 与 TRACER ablate 旧语义引用，保留共用 receipt 物理输入契约；③移除 renderer tracer layer、tracer sampler adapter 和 `P012SandTracer` 纯视觉对象；④移除只服务 tracer 的测试/资产，保留注入 receipt 的物理测试；⑤扫描 import/route-off/ablation 旧引用，Canvas B基准/冻结锚点是否保留由 manager决定；⑥重跑 D4/D5客观与主观报告，确认不再存在 tracer双画/旧feed。

在退出门未满足前 B 仅作为 route inactive 的兼容显示路径，不得删除、改写为 C 账本证据或改变 §6 的互斥定义。B 对 C 的“并存”指产品包同时保留两套 renderer 供 route 分支选择，**不指在同帧叠加两套粒子图像**。

## 7. 生命周期、异常降级、帧时风险与观测点

页面隐藏/显示、窗口尺寸改变、P-013 overlay切换、renderer destroy/recreate 均只影响 renderer 资源和 snapshot buffer，不调用 `clearFallingSand`、`reset` 或物理 route setter。恢复时重新取得最新完整 post-commit snapshot 和 frame transform；旧 physicsCommitId/revision 丢弃。粒子池生命周期仍由 M2 §5 的 reset/invert/dormant 决定。生命周期变更发生在物理子步中时，D2确保 publish 后才可被绘制。

| 风险 | 结果 | 检查/回退 |
|---|---|---|
| snapshot 和材料帧不同步 | 粒子漂移、穿床或显示旧位置 | 要求同 `physicsCommitId`；不匹配整层跳过并记 `PHYSICS_COMMIT_MISMATCH`，不得用旧 tracer代替C库存 |
| route身份由页面重算 | 与物理真实route冲突、重复feed/draw | active只从M2 snapshot；D5路由对照测试覆盖 ON/OFF/冲突/BUSY |
| `CONTACT_WAIT` 被隐藏或提前fade | 看起来落地即消失且床未增长 | 固定 state2仍画；床变化仅认commit后的material snapshot/LAND |
| 每帧快照对象/数组分配 | GC尖峰、P95抖动 | 复用Nmax定容buffer；采样分配/GC及高count区间 |
| Canvas大量点调用 | UI线程超预算 | 按phase批处理；报告 count分桶与 draw CPU；性能不达标交manager评估P1后端迁移，不降Nmax/冻结物理频率求通过 |
| P1与Canvas色彩不一致 | 粒子色彩/光照边界跳变 | D4先按Canvas现有色彩域；GLES迁移依P1同色彩pipeline及色块证据 |
| context/页面重建清空视觉缓存 | 粒子消失或旧帧重播 | renderer不拥有权威粒子；恢复时copy最新snapshot，不回放journal生成粒子 |
| shader/canvas draw失败 | C帧缺失 | 上报error code、presentedSource；该帧C库存不改、不feed tracer、不宣称已呈现 |

后续 D4/D5/D6 遥测字段建议（挂在现有 P012 seg3/per-frame 记录，异步 GPU 字段不适用于Canvas首选路径）：

| 字段 | 采样口径 | 目的 |
|---|---|---|
| `particleRouteActive`、`particleRouteId` | 每个渲染frame复制M2快照身份；非C填false/null | route分支审计 |
| `renderFrameId`、`particlePhysicsCommitId`、`particleRevision`、`particleSnapshotCommitLag` | 每帧；commitLag按最新已提交物理revision与投影revision的差计，单位为physics commits，不是particle age | 同源性/陈旧快照检测 |
| `particleCountFlying`、`particleCountContactWait`、`particleCapacity` | 每帧快照计数；与v7同子步summary抽样配对 | 负载与账本一致性 |
| `particleDrawnCount`、`particleSkippedCount`、`particleSkipReason` | 每帧绘制后；首版默认 skipped=0 | 可见性完整性 |
| `particleCopyCpuMs`、`particleLayerCpuMs`、`particleLayerDrawCalls` | 每帧；单调时钟；分别计adapter copy和绘制CPU耗时 | Canvas成本归因 |
| `particleFrameIntervalMs`、`particleFrameP50Ms`、`particleFrameP95Ms` | seg3口径连续分段采样；仅对有效实际呈现源计数；帧丢弃单独列数 | 与现P012帧时报告对齐 |
| `particleGcCount`、`particleHeapDeltaBytes` | 运行平台有可读GC/heap信息才填；不可读为null并说明 | 不用RSS/无增长代替零分配证明 |
| `particleLandEventCount`、`particleLandGrantMass`、`particleContactWaitCount` | 来自同frame/相邻提交身份的只读v7 journal/summary；不由画面反推 | 可见落床与真实事务并列检查 |
| `particlePresentedSource`、`particleFallbackReason`、`particleLayerEnabled` | 每帧枚举`C_PARTICLE/B_TRACER/NONE`及明确原因 | 切换、故障和ablation审计 |

主观视觉/真机帧时采集归后续 effector 队列，不在本卡执行。P50/P95分位、帧时预算及粒子参数必须由 tester/manager 沿既有 P012 seg3口径和设备基线冻结；本稿只登记字段与样本单位。

## 8. 依赖、待出口决定与 D4/D5 交接

| 问题 | 当前确定结论 | 尚须的证据/决定 | 责任与门序 |
|---|---|---|---|
| pool字段是否足以绘制 | x/y/v/phase/state/id 已满足首版位置显示；age/seed缺失明示 | D2需确认M2真实snapshot与物理commit同源、状态枚举及v7字段一致 | D2原型出口/manager验收后，D4实现 |
| 场景坐标映射 | 统一使用P012同帧material/display transform，不能 renderer-side积分 | spawn坐标在不同tilt/resize/gesture原点中的实际映射核对 | D2数据接线后；D4纯渲染联调，不占真机 |
| tracer feed时序 | C active不feed不draw；C inactive沿用B | receipt与route active以哪个post-commit frame身份交叉核对 | D2接线+manager D4/D5卡 |
| Canvas首选路径 | D4首版Canvas2D粒子层 | CPU P95/GC能否满足产品预算尚未知 | D4离线证据后，D6真机双报告；不足再由manager评估P1 GLES迁移 |
| 粒径/alpha/速度/寿命 | 均不在本文定值；无age不可真实fade | D6效果扫描及如需生命周期字段的另案设计 | designer/effector→manager |
| B下线 | 三条件+下线步骤在§6.3可判 | 原型出口、双报告、Nmax定标、manager裁决 | D4/D5/D6完成后manager单独放行删除 |
| P1色彩链 | 若后续GLES迁移则严格走P1完整链；P0诊断profile不画产品粒子 | P1实现及sRGB/合成前置证据 | P013 P1实现/验证后另行评审 |

**实现次序**：D1 B收口并manager认可 → D2 M2全链与原型出口（含post-commit snapshot/v7/守恒检查）→ manager批准 D4/D5 实现卡 → D4 Canvas只读粒子层/adapter/遮蔽与tracer互斥 → D5 page/want/lifecycle接线及离线验证 → D6 effector真机双报告和Nmax/帧时定标 → manager裁决是否下线B、是否迁GLES。

当前 D1 B报告未达接受，D2尚未就绪；因此本文只交通用设计与完整门序，不宣称 D4/D5 已可开工，不阻塞 manager 后续依证据安排。

## 9. 不可越界项与修订记录

- 不改 300/s、128列、守恒/库存/容量/有限值、26/20、tan30 等冻结锚点；不改测试预注册 estimand。
- 物理状态由 M1/M2 决定，Canvas/GLES只读；不让渲染分支影响预算、guard、route或粒子池。
- 不将 B tracer 当粒子，不将绘制点数/效果录像当守恒与落床证据；C真实质量只能由 v7事件与 checker 对账。
- 不自定粒子大小、alpha、Nmax、速度、寿命、颜色数值、帧预算或效果门槛；这些仍按 §9/effector/tester/manager流程确定。
- 不占用真机，不安装 HAP；帧时和观感验证归后续串行 effector 队列。

变更记录：2026-10-24，v1，按卡 6adc92d0 产出。完成位置数据契约、Canvas/GLES比较与首选路径、state=2/真实落床表达、触点连续性、模块与依赖、B互斥及退出门、生命周期/异常/遥测登记。纯设计零代码；D1/D2未通过/未完成状态如实登记；未宣称实现、效果验证或真机验证。
