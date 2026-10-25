# T31 主观端到端执行案：粒子落沙与高度场累积效果验证（v1）

日期：2026-10-25（Asia/Shanghai）。责任：effector。任务卡：6add769ce4b06ee35724ea6b（T01后续·主观端到端）。标签：仿真线·渲染线·effector·真机。
状态：**BLOCKED（T20 页面侧装配入口缺失）**；本文为装配入口就绪后的设备执行案与阻塞取证，不宣称任何设备结论。

## 1. 验证目标

在真机上验证双场粒子路线（`dual-field-particle-v1`）接通后的主观效果：

1. 粒子落沙可见：手势/摇晃产生的粒子在下落段（FLYING）画面可辨，不是仅账本数字变化；
2. 落床累积正确：粒子落地（LAND）后从渲染中消失，同时高度场床层质量增加，二者同一物理提交（physicsCommitId）;
3. 高度场累积连续：连续注沙后沙丘顶面（sandTop）逐列抬升，形态平滑；
4. 与 B tracer 不双画：粒子路线激活时 sandTracer 退出，`particlePresentedSource` 记录唯一显示来源；
5. 静止稳定：无输入时段粒子归床、画面零抖动。

## 2. 依赖与当前阻塞（取证）

| 依赖 | 状态 | 证据 |
|---|---|---|
| T36 触摸注沙速率修复 | 已提交 | commit c1ae739 `fix(p011): normalize gesture nanoseconds and stop emission at boundaries (T36)` |
| T20 D4/D5 粒子视觉接线 | 工作树已实现渲染/快照/tracer 退出部分；**页面侧装配入口缺失** | 见 §2.1 |
| T30 客观端到端 | tester 卡（6add769ce4b017ab33e71bd7），与本卡无硬耦合（客观结论不阻塞主观采集） | docs/后续工作清单.md |

### 2.1 阻塞事实：页面侧无粒子路线装配入口

`particleRouteActive()` 的三条件（WaterSandField.ets:2556-2559）：

```text
naturalObservationRouteId === 'dual-field-particle-v1'
  && particleRouteEnabled(particleRouteId, particleParams)   // routeId 匹配且 params 非 null
  && particleBinding !== null                                 // 由 setParticleRoute(params) 创建
```

全树检索（entry/src/main/ets/，含 pages/entryability/render）：`setParticleRoute` 仅存在于 physics 定义与 `test/*.cjs` Node 测试，**无任何页面/Ability 调用点**；`defaultP011ParticleParams` 同样无页面引用。因此真机上 `particleBinding` 恒为 null，`particleRouteActive()` 恒 false：

- SandCanvas.ets:2142 `particleVisual = ws.particleRouteActive() && !waterSandSnapshotEnabled` 恒 false → P012Renderer 不会因粒子路线启用；
- SandCanvas.ets:2448 tracer 抑制条件同样不触发；
- T31 全部主观场景（落沙可见、落床累积、摇晃扬沙粒子化）无法在真机复现。

该缺口在 T20 交付范围（D4/D5 粒子视觉接线）内：渲染侧已通（7/7 离线单测 PASS，`test/run_p012_particle_visual_units.cjs`），缺"从输入到路线武装"的最后一跳。

### 2.2 装配入口的最小修复要求（交 developer/T20 会话）

两个候选实现，任选其一，需满足全部约束：

- **候选 A（schedule want 扩展）**：EntryAbility 现有 `p011route` 白名单只接受 `''`/`relax-correction-weighted-v1`（EntryAbility.ets:161-167）。扩展为接受 `dual-field-particle-v1`，并新增 `p011particle=1`（或参数串）透传；`consumeP011ScheduleWant` 内在 route 为粒子路线时调用 `scheduleField.setParticleRoute(defaultP011ParticleParams())`。注意顺序：`setParticleRoute` 先于/同于 `setNaturalObservationRouteId` 调用，因为后者对存活池非空且目标非粒子路线时会拒绝（WaterSandField.ets:2506-2508）。
- **候选 B（调试面板按钮）**：测试模式面板增加"粒子路线"开关，直接 `setParticleRoute(defaultP011ParticleParams())` + `setNaturalObservationRouteId(P011_PARTICLE_ROUTE_ID)`。

约束（来自 WaterSandField.setParticleRoute，:2521-2552）：

1. `cols` 必须为 128（当前页面常量满足）；
2. `naturalObservationRouteId` 为 null 或已是粒子路线，否则 `ROUTE_CONFLICT`；
3. advection/deepCreep/moisture 三类参数必须均未武装，否则 `ROUTE_CONFLICT`（即粒子 run 不能与 S3/plan-3/S4 同场）；
4. 池非空（liveCount>0）时切换/卸载路线返回 `BUSY_PARTICLES`；
5. `nmax ≤ 512`（P012ParticleRenderSnapshot 容量 512，SandCanvas 按 `particleRenderCapacity()` 自适应扩容前上限即字段容量）；
6. 默认参数：`massPackage=0.1, nmax=512, terminalSpeedHeavy=64, terminalSpeedLight=38, dragHeavy=3, dragLight=2, liftSpeedHeavy=8, liftSpeedLight=8, liftSpread=2, mergeColRadius=1, mergeHeightRadius=2, contactEps=1e-6, channelRateCap=0`（`defaultP011ParticleParams()`，P011ParticleM2Field.ets）。

## 3. 渲染链路核对点（设备执行前预读）

设备运行按下表核对 hilog/状态字段，任一不符即停采并记录：

| 核对点 | 期望 | 位置 |
|---|---|---|
| 路线激活 | `particleRouteActive()===true` 后 `particleVisual` 为真，P012Renderer 首次构建 | SandCanvas.ets:2141-2143 |
| 快照填充 | `buildP012FrameContext` 调 `copyParticleRenderSnapshot`，`count>0`、`active=true`、`physicsCommitId` 与床层快照同号 | SandCanvas.ets:2491-2496；WaterSandField.ets:3884 |
| 粒子层门 | `particleLayerEnabled=true` 且 commit 匹配才画；`skipReason` 为空 | P012ParticleLayer.ets:13 |
| tracer 退出 | 粒子激活帧 `particlePresentedSource='C_PARTICLE'`；粒子撤下后回落 `'B_TRACER'`/`'NONE'` | P012WaterSandRenderer.ets（render 内 particle 分支） |
| 外观档 | 粒子激活且面板档位为 OFF 时自动升 E 档 | SandCanvas.ets:2524-2526 |
| 注入量 | T36 修复后单次 swipe 请求量≈设计值（历史缺陷为 43× 放大） | 见 §5.2 |

## 4. 设备执行步骤（装配入口就绪后）

前置：构建 `entry-default-sN-signed.hap`（N 递增）→ `hdc install -r` → 防熄屏 `power-shell timeout -o 600000` → USB 弹窗键 (240,620) 关闭。

录制坐标（沿用 record 标定，uitest dumpLayout 复核）：

- 调参按钮 (1100,230)；测试模式按钮 (640,1802)（先开测试模式，重力全由手势构造）；
- swipe 用全宽 (100→1200)，最小有效位移 ~50px；注入点击用盒内中部坐标；
- snapshot 连拍 0.74s/帧；亮度 >120 的帧判污染剔除。

场景矩阵（每场景先静置 3s 基线，再动作，动作后连拍 ≥8s）：

| 场景 | 操作 | 主观通过标准 |
|---|---|---|
| S1 落沙可见 | 单次全宽 swipe 注沙 | 下落段可见独立粒群（非一片纯色糊块）；粒子数 count>0 持续 ≥1s |
| S2 落床累积 | 同位置连续 5 次 swipe | 每次落床后该列沙顶可见抬升；床层质量增量=注入量-悬浮残留；粒子消失与床层增长同帧 |
| S3 摇晃扬沙 | 开 shake 后摇晃设备（或手势构造等效输入） | 粒子扬起段悬浮可见；静置后全部归床，无残留悬浮 |
| S4 倾斜流动 | 测试模式手势 ±30° 倾斜并保持 | 粒子/沙丘顶面沿倾斜方向流动，界面不出现直角断裂 |
| S5 静止稳定 | 全部静置 10s | 零抖动（相邻帧床层行号差为 0）、粒子计数归零、帧间隔无 >1s 间隙 |

采集物：每场景 ≥3 张关键帧截图 + 全程 hilog（`WS_FRAME/WS_METRIC/WS_EVENT` 逐行 dump 后按行去重）+ 遥测 JSON。产物目录 `test/artifacts/t31-subjective-particle-<date>/`。

## 5. 验收判据

### 5.1 主观分（对照用户效果标准：细沙、平滑沙丘、倾斜流动、静止稳定）

| 维度 | 及格 | 优秀 |
|---|---|---|
| 粒子可辨性 | 下落段能数出粒群边界 | 单粒可辨、有速度层次 |
| 沙丘平滑度 | 无阶梯/断裂/直角 | 连续曲线、安息角一致 |
| 流动自然度 | 倾斜时整体方向正确 | 有流线先后、无瞬移 |
| 静止稳定 | 10s 零抖动 | 零抖动且床面保持 |

### 5.2 客观锚点（与主观分并列记录，不互相替代）

1. **分相守恒**：每场景首尾 H、L 各自守恒（只验 total 会漏分布错乱——s72 教训）；
2. **同提交一致性**：粒子 LAND 帧的 `physicsCommitId` 与床层快照一致，床层质量增量 = 该提交沉积量；
3. **无双画**：同一帧不存在 C_PARTICLE 与 B_TRACER 同时呈现（`particlePresentedSource` 唯一）；
4. **注入量**：单次 swipe 的 WS_EVENT gesture_sand 请求量 ≈ 设计值（T36 前历史为 43×）；每笔 `rejectedReason=None`；
5. **帧健康**：无 frameTimeMs>1000ms 的真冻结（`dormant`+`substeps=0` 为满容闲置，不算冻结）。

### 5.3 已知不阻塞项

- `model_error code=5 TILT_PLANNING_BLOCKED` 按设计不冻结（WaterSandField.ets:2015-2022 豁免），不计缺陷；
- 测试模式放置面不水平导致的传感器倾角由手势构造替代，不使用真实传感器数据。

## 6. 缺陷反馈规则

1. 发现问题按"每问题每技术路线"记入 `docs/effect-cases.md`，附场景/帧号/账本号/截图路径；
2. 分级：P0=效果不可辨或守恒破坏；P1=可见瑕疵；P2=轻微；
3. effector 不改代码；P0/P1 附最小复现路径反馈 developer 原卡；
4. 复测只在对应修复提交后进行，不叠加多项修复混测。

## 7. 产物与回执规范

- 原始产物：`test/artifacts/t31-subjective-particle-<date>/`（截图 + hilog + 遥测 JSON + 采集时刻）；
- 回执：`docs/t31-subjective-particle-receipt-v1.md`（场景×判据矩阵、逐场景结论、缺陷清单、buildCommit/sourceTreeHash/设备型号）；
- 入库用临时 index（并行会话共享工作树），只提交本卡产物与回执。

## 8. 变更记录

- 2026-10-25 v1：建立执行案；记录 T20 装配入口阻塞取证（§2.1）与最小修复要求（§2.2）；未使用真机，未宣称任何效果结论。
