# P-011 路线 C（粒子↔高度场双场）DUALFIELD-* 用例预注册 v1

日期：2026-10-24。责任：tester。状态：**METHOD_FROZEN / NOT_IMPLEMENTED / NO_CAPTURE_AUTHORIZATION**。
依据：[design-P011-falling-sand-vertical-routes-v1.md](design-P011-falling-sand-vertical-routes-v1.md) §4/§6（用户 2026-10-24 选型：路线 C = 真实活动粒子 + 128 列高度场，为手势掉沙与摇晃扬沙共同架构）、[algorithm-P011-gesture-sand-source-v1.md](algorithm-P011-gesture-sand-source-v1.md) §2/§3/§5、[architecture-P011-gesture-sand-inject-phys-v1.md](architecture-P011-gesture-sand-inject-phys-v1.md)（外源注入已实现，schema v4）、docs/test-cases.md 现有 SLIP/gesture 用例。
架构稿 `architecture-P011-dual-field-particle-v1.md`（architect 在写）落地后，本稿 §2 遥测契约的字段名以架构稿为准**对齐复审**（契约的语义与恒等式不变；字段改名不构成方法变更，重新登记 revision 号即可）。

## 1. 适用范围与身份

- **protocolId**：`p011-dual-field-particle-v1`；**protocolRevision**：`preregistration-v1`。
- **对象**：路线 C 的粒子活动层（每粒子 `id, phase, mass, x, y, vx, vy, sourceId`，canonical 物理库存）与既有 128 列高度场（`bh/bl/mh/ml/suspH/suspL/S/waterTop`）的**统一账本**。两个入口：`sourceKind=GESTURE_EXTERNAL`（手指落沙，外源准入）与 `sourceKind=SHAKE_LIFT`（摇晃扬沙，相内搬运）。
- **routeId**：候选 `p011-dual-field-particle-v1`（以架构稿登记为准）。**默认关闭**：未显式绑定的运行不触达粒子层任何代码路径；关闭时既有链路必须逐位不变（DUALFIELD-LIFE-04）。
- **与 B tracer 的边界**：B 路线视觉 tracer（`P012SandTracer`）无物理质量，**不得作为 C 的守恒、落床时刻、摇晃扬沙的验收证据**（design §6）。本组用例全部只信物理账本与事件遥测。
- **与 108-run 批的边界**：correction 矩阵批（82/108 封存，`archive-P011-correction-matrix-closure-v1.md`）物理构建冻结，C 的物理实现须等该批统计提取完成后单开 route/build/pilot，**不用新 schema 重解释旧批**；本预注册不授权任何采集。
- **与 gesture 注入 v4 的边界**：外源准入事务（`injectGestureSand`、`sourceInjectedH/L`、`sourceEpoch`、`P011InjectionReceipt`）已实现并入账；C 在其上增加粒子层，**IH/IL 仍只累计实际准入量一次**（design §4：在途粒子加在 `actual`，不另加 `expected`）。

## 2. 遥测契约（ledger schema v7 `particles` 块，字段名待架构稿对齐）

用例消费的**可观测契约**如下。实现前缺失即记 `NOT_IMPLEMENTED(missing_field)` 并列出确切字段路径，不得用替代量冒充。

### 2.1 路由身份（context 级）

| 字段 | 语义 |
|---|---|
| `particleRouteActive` | 本子步粒子层是否运行（false=路线关闭，既有链路） |
| `particleRouteId` | 路线身份串；与 `particleRouteActive=true` 必须同时非空 |

### 2.2 粒子池摘要（每子步 `particles` 块）

| 字段 | 语义 |
|---|---|
| `count` / `massH` / `massL` | 活粒子数与分相质量（=守恒恒等式的在途项） |
| `bySourceKind.GESTURE_EXTERNAL.{count,massH,massL}` | 外源入口分账 |
| `bySourceKind.SHAKE_LIFT.{count,massH,massL}` | 扬沙入口分账 |
| `capacityUsed` / `capacityLimit` | 池占用与上限（候选 `Nmax=512`，**非冻结**，待效果/性能定标） |
| `degraded.{count,massH,massL}` | 累计 `PARTICLE_DEGRADED_TO_COLUMN` 降级量 |

### 2.3 粒子事件账本（每子步 `particleEvents[]`，逐笔可追溯）

| `kind` | 必须携带 |
|---|---|
| `SPAWN` | `particleId, phase, mass, sourceKind, x, y, sourceId, requested, admitted` |
| `LAND` | `particleId, phase, mass, column, requested, granted, deltaS, guardPreview(联合预演结论), remainder, remainderDisposition` |
| `CROSS_COLUMN` | `particleId, phase, mass, fromColumn, toColumn, budget{local,bottom,total}, distance` |
| `LIFT` | `particleId, phase, mass, fromLayer(bh/bl/mh/ml), column, requested, granted` |
| `MERGE` | `particleIds[], phase, massBefore, massAfter, positionMomentPreserved` |
| `DEGRADE` | `particleIds[], phase, mass, toColumn, toSuspH/toSuspL, flag='PARTICLE_DEGRADED_TO_COLUMN'` |
| `TO_SUSP` | `particleId, phase, mass, toColumn, suspDelta, reason` |

事件不变量（checker 逐笔验）：同一 `particleId` 的 `mass` 跨事件守恒；`LAND/TO_SUSP/DEGRADE` 的 `granted+remainder==requested`；`MERGE` 的 `massAfter==massBefore`（1e-9）。

### 2.4 与既有 schema 的衔接

守恒恒等式直接消费既有字段：`stateAfter.{bh,bl,mh,ml,suspH,suspL}`、`audit.{heavyTotalBefore,heavyTotalAfter,lightTotalBefore,lightTotalAfter}`、`context.sourceInjectedH/sourceInjectedL/sourceEpoch`（v4）、`budgets.{local*,bottom*,total*}`。run 初值 `VH0/VL0` 取首条记录的 `audit.heavyTotalBefore/lightTotalBefore`（reset 后、任何注入前）。

## 3. 守恒 estimand（DUALFIELD-CONS-*）

```text
VH_actual(t) = Σ_c (bh[c] + mh[c] + suspH[c]) + Σ_particle(phase=H) mass
VL_actual(t) = Σ_c (bl[c] + ml[c] + suspL[c]) + Σ_particle(phase=L) mass
VH_expected(t) = VH0 + sourceInjectedH(t)   （外源准入只加一次）
VL_expected(t) = VL0 + sourceInjectedL(t)
drift_H(t) = VH_actual(t) − VH_expected(t) ；|drift_H|、|drift_L| ≤ 1e-9（与 gesture 注入单测同容差）
```

- **互斥原子性**：任一子步，同一质量不得同时计入 `suspH/suspL` 与粒子池；粒子↔床面/悬浮的每次转移必须经由事件账本的一笔 `LAND/TO_SUSP/DEGRADE` 原子完成（先扣粒子池再写目标层，同一子步内可对账）。
- **部分授予余量**：`LAND` 的 `granted < requested` 时，余量留在沙面之上的接触粒子队列（画成细小滞留）或按明定规则显式转入 `susp`（`TO_SUSP` 一笔）；**不得删除在途量、不得不受 guard 直写 `mh/ml`、不得双计**。
- **沉积口径**：`depositedHeavy/LightFraction` 分母=当前累计期望（`VH0+IH`），并标记 `sourceEpoch`；`t90` 在活动源 epoch 内无效（或新 epoch 重算），不沿用注入前首次达标时刻。
- **水量**：注入/粒子排水不毁自由水（η 构造保证，v4 已实现）；粒子进入 `susp` 或沉积时按同一保水约束调水面；判据 `|ΣwaterTop'−ΣwaterTop| / Vw0 < 0.1%`（算法 §5 ②）。

## 4. 预算 estimand（DUALFIELD-BUD-*）

粒子每个离散相量操作（`LAND` / `CROSS_COLUMN` / `LIFT`）必须依次经过**四层**并逐层可审计：

1. **逐列 local**：`vSandTopMax·dt = 28·dt`（limit/used/remaining 逐列）；
2. **bottom**：`10·dt`（仅 bottom 通道相关操作；requested/used/limit/clipped）；
3. **共享 total**：`totalMotionBudget·dt = 300·dt`（requested/used/remaining/clipped）；
4. **§19.5 tan30 guard 联合预演**：以 `ΔS[c]=±q` 逐笔预演受影响两侧，截断余量按 §3 规则处置（可行性层，非额度但计入事件账本 `guardPreview`）。

- **跨列输运上限**：`CROSS_COLUMN` 的水平输运计入 local/total 预算，速率上限 `300/s`（与 total 同源）；预算用尽时水平位移**等待或受限**，不得为上层开未经批准的免费传输通道；跨列后重核算该列在途占用与水量。
- **满额合并→降级链**（质量闭合 MUST-PASS）：池满 → 确定性合并同相/近列/近高度粒子为带质量有限包（保持总量与一阶位置矩）→ 包槽仍满 → 新准入量进入现有 `susp` 作列均值降级并标记 `PARTICLE_DEGRADED_TO_COLUMN`（或缩小准入）。**不得删除已准入质量、不得暗升上限、不得无限排队**；降级部分不冒称有真粒子轨迹。
- **来源免层**：外源注入事件本身不扣 `totalMotionBudget`（不是场内搬运，算法 §3）；注入后发生的沉降/滑移/蠕动照常扣。

## 5. 生命周期 estimand（DUALFIELD-LIFE-*）

- **reset**：清粒子池与事件账本，外源账本（IH/IL）随物理场 reset 清零（与 v4 reset 语义一致）。
- **倒置/翻转**：先按新重力底部重映射粒子 `y`（与在途 cohort 的生命周期变换同理，design §2），再推进；不得把旧屏幕坐标当重力坐标。
- **模式切换**：TEST↔EXPERIENCE、场景切换、页面离开、容器重建清**未消费**手势路径与时间积分；**已提交**的沙与 IH/IL 保留。
- **snapshot/checker**：snapshot schema 扩展携带粒子池摘要（至少 count/massH/massL/bySourceKind）；checker 把 §3/§4 恒等式纳入硬闸门；旧无源数据**不迁移**成"曾有粒子"的假历史。
- **等价回归（LIFE-04）**：路线关闭的 run 与关闭前基线**逐位一致**（同输入同种子，锚点容差同 D-1 先例：effectiveTilt 1e-9、sandTop 1e-6）；路线开启的 run 与配对 legacy run 在**同 sourceKind=无源**段做容差等价（不要求逐位——粒子层改变场内动力学这一点须如实标注为路线差异，等价断言只覆盖"关闭时"）。

## 6. 来源分账 estimand（DUALFIELD-SRC-*）

| 入口 | 库存动作 | IH/IL | 遥测分账 |
|---|---|---|---|
| `GESTURE_EXTERNAL` | 外源准入量 → 粒子池（或 susp） | **只增一次**（准入时才增） | `bySourceKind.GESTURE_EXTERNAL` |
| `SHAKE_LIFT` | 从 `bh/bl/mh/ml` 已准入量 → 粒子池 | **不变**（相内搬运） | `bySourceKind.SHAKE_LIFT` |

- 两入口**不混账**：任一子步 `bySourceKind` 两类质量与事件账本按 `sourceKind` 分列闭合；同一质量不得既经外源准入账又经扬沙搬运账。
- `admitted=0`（`NO_WATER` / `CAPACITY_BLOCKED` / λ≈0）：**零粒子生成**、零视觉点（B tracer 规则扩展到 C）。
- 被拒注入（receipt `rejectedReason` 非空）：对应时段**不得出现任何粒子**；出现即判路由 bug（design §3 同款规则）。

## 7. 手势掉沙累积场景 estimand（DUALFIELD-GES-*）

TEST 模式构造触摸（ Want/script 注入，禁真实传感器 ）驱动，全链可观测点（design §1 候选观测，**数值阈值待效果试采预注册，当前不冻结**）：

| 观测点 | 定义 | 通道 |
|---|---|---|
| 首粒延迟 | 有效触点 `inputTime` → 首个 `SPAWN` 事件 `physicsTime` | 事件账本 |
| 连续滑动漏段率 | 有效滑动段内应有粒子生成的子步中缺失的占比 | 事件账本+输入时间戳 |
| 轨迹到沙面交点误差 | 粒子 `y` 轨迹与只读 `S(x)` 的交点 vs 视觉落点（若配录像） | 事件账本（±录像交叉） |
| 粒子消失与 settle grant 时差 | 最后一个 `LAND`（或 `TO_SUSP`）事件时间 vs 对应 `settleSandV2` grant 时间 | 事件账本 |
| 床面增长量=准入沉积量 | `ΔS[c]` 累计 == 该列 `LAND.granted` 累计（guard 截断部分按 §3 余量规则另计） | 状态差+事件账本 |

- **TEST 构造触摸与 EXPERIENCE 真实触摸共用同一物理角/触点路径**：两者只差输入源（ Want 注入 vs `PanGesture` 事件 ），物理层消费同一 `P011GestureSandRequest` 路由与同一粒子积分器；TEST 不得为构造触摸开特殊物理路径。
- 场景判据遵循 algorithm §5 五场景（匀速划过/重复至满容/传感器+Pan 并发/长按面板画布外取消/有源无源纹理帧时）在 C 下的对应版本。

## 8. 用例注册表（DUALFIELD-*，16 例）

| 用例 ID | 组 | 覆盖 | 门类 | 今日 RED 状态 |
|---|---|---|---|---|
| DUALFIELD-CONS-01 | 守恒 | §3 三相+粒子守恒恒等式（per-substep drift≤1e-9） | MUST-PASS | NOT_IMPLEMENTED（缺 `particles` 块） |
| DUALFIELD-CONS-02 | 守恒 | susp↔粒子互斥原子性（事件账本双计检测） | MUST-PASS | NOT_IMPLEMENTED（缺 `particleEvents`） |
| DUALFIELD-CONS-03 | 守恒 | 落床部分授予余量滞留不丢不双计 | MUST-PASS | NOT_IMPLEMENTED（缺 `particleEvents`） |
| DUALFIELD-CONS-04 | 守恒 | depositedFraction 分母=累计期望+t90 epoch 无效 | MUST-PASS | NOT_IMPLEMENTED（缺 `particles`/epoch 联动字段） |
| DUALFIELD-BUD-01 | 预算 | LAND/CROSS/LIFT 四层计费逐层可审计 | MUST-PASS | NOT_IMPLEMENTED（缺事件预算字段） |
| DUALFIELD-BUD-02 | 预算 | 跨列 300/s 上限+无免费传输通道 | MUST-PASS | NOT_IMPLEMENTED（缺 `CROSS_COLUMN` 事件） |
| DUALFIELD-BUD-03 | 预算 | 满额合并→降级链质量闭合（PARTICLE_DEGRADED_TO_COLUMN） | MUST-PASS | NOT_IMPLEMENTED（缺 `MERGE/DEGRADE` 事件） |
| DUALFIELD-LIFE-01 | 生命周期 | reset/invert 粒子重映射与清理 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-LIFE-02 | 生命周期 | 模式切换/手势清理/已提交账本保留 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-LIFE-03 | 生命周期 | snapshot/checker 扩展+旧数据不假迁移 | MUST-PASS | NOT_IMPLEMENTED（缺 schema v7） |
| DUALFIELD-LIFE-04 | 生命周期 | 旧无源 run 等价回归（04a 关闭守卫可执行；04b 配对等价） | MUST-PASS | **04a 今日可执行（期望 PASS）**；04b NOT_IMPLEMENTED |
| DUALFIELD-SRC-01 | 来源分账 | sourceKind 两入口不混账 | MUST-PASS | NOT_IMPLEMENTED（缺 `bySourceKind`） |
| DUALFIELD-SRC-02 | 来源分账 | admitted=0 零粒子生成 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-SRC-03 | 来源分账 | 被拒注入零粒子 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-GES-01 | 场景 | 全链五观测点（TEST 构造触摸） | TIERED（阈值待试采冻结） | BLOCKED（缺粒子层+真机/seg3 计时，等 s134 修复构建 6adb3508/6adb7038 或附数据污染声明） |
| DUALFIELD-GES-02 | 场景 | TEST/EXPERIENCE 同一物理角与触点路径 | MUST-PASS（路径同一性） | BLOCKED（缺粒子层+双模式真机对照） |

门类说明：MUST-PASS = 不产点估计的安全/账本不变量，任一失败即该 route 不合格；TIERED = 效果类分档评分，阈值在效果试采后另稿冻结（本稿不冻结任何观感数值）。

## 9. RED 基线规则

1. **NOT_IMPLEMENTED(missing_field: [...])**：路线已开但契约字段缺失——列出确切字段路径（§2），不用替代量（如渲染像素、tracer 点数、视觉估计）冒充。
2. **BLOCKED(dependency)**：依赖真机采集或 seg3 计时修复（s134：6adb3508 / 6adb7038）——等修复构建或附**数据污染声明**（设备姿态/帧时污染范围）后方可采信。
3. **路线关闭守卫（今日可执行）**：`particleRouteActive` 缺失或 false 的账本，任何粒子字段出现即 `LEAKAGE`（FAIL）——防默认链路被静默掺入粒子物理。
4. **等价回归锚点**：LIFE-04b 的配对锚点在 C 实现交付时由 tester 与 developer 共同冻结（同 D-1 先例：漂移=行为变更，走复审而非改锚点）。
5. 任何用例转绿必须**逐粒子/逐笔可溯源**（事件 `particleId` 全链可追），不接受只有聚合量的 PASS。

## 10. 安全闸门继承（零豁免）

现行硬闸门由既有 safety checker 原样执行，本组不豁免、不复制、不弱化：tan30（`≤tan30+1e-9` 引擎容差）、逐 writer guard、local/bottom/total 预算、守恒（1e-9）、库存、容量、有限值、账本对齐、边身份、stop 语义。**不改任何冻结阈值**：26/20 双角为 factory-recommended 用户可配值（非标定）；效果候选值（`gestureSandRate`、`Nmax`、速度律、观测点阈值）一律非冻结，试采后另稿。粒子终端速度律（`v_phase≈settleSpeed_phase` 等）是**新模型候选**，不继承比例式沉降的已标定语义（design §4）。

## 11. 红线

- 不用 B tracer 视觉点冒充 C 的守恒/落床/扬沙证据；
- 不改冻结阈值、不追认旧 route 结果、不用新 schema 重解释 108-run 旧批；
- 不自行放宽门槛、不删不利规则；消除误排同时消除漏检；
- 资料缺失不补造；缺失字段如实登记 NOT_IMPLEMENTED；
- 本稿不授权采集；C 的 route/build/pilot 须 architect 验收 + manager 批准后单开。

## 12. 证据通道

- **账本级闸门（本稿交付）**：`python test/p011_dual_field_gate_check.py --ledger <ledger.ndjson> --output <out.json>`——路由身份/泄漏守卫/守恒恒等式/原子性/预算闭合/来源分账的 ledger 级断言；字段缺失输出 NOT_IMPLEMENTED 及缺失清单。合成自测：`python test/test_p011_dual_field_gate_check.py`（每条判定分支+泄漏守卫+前向合成粒子账本）。
- **字段级单测（C 实现后由 developer+tester 建 Node 1:1 通道）**：参照 `test/run_p011_gesture_inject_units.cjs`（编译实际 .ets 执行），覆盖粒子 spawn/land/cross/lift/merge/degrade 的逐笔行为。
- **设备级场景（GES 组）**：真机 ledger + 可选录像交叉；计时类等 s134 修复构建。
