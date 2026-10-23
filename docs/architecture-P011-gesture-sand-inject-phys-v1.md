# P-011 掉沙物理层注入任务书：injectGestureSand 双相原子事务 v1

日期：2026-10-23。责任：architect。状态：**ENGINEERING_DESIGN / READY_FOR_DEVELOPER**。依据：[手势沙源算法设计 §2/§3/§4](algorithm-P011-gesture-sand-source-v1.md)（已验收）+ [B 路线工程 §1.2 receipt 契约](architecture-P011-falling-sand-B-route-v1.md)（**B 侧已按此实现并入库 bbe8e33**——`P012SandTracer.ets:19-36` 九字段接口已冻结，物理层实现必须交付**同一结构**，字段名/语义逐项一致）。代码现状核对（2026-10-23）：`saveBackup/restoreBackup/audit` 事务基建就位（:4438/:4448/:4293）；`waterLevel` 的单调二分求根（:waterLevel low/high 界）是 η 抬升求解的直接参照；旧 `inject()`（:1191）单相离散事件保留不删。

## 1. injectGestureSand 接口落点（工作 1）

### 1.1 签名与位置

```ts
// WaterSandField.ets 追加（公开方法，独立于旧 inject）：
injectGestureSand(request: P011GestureSandRequest): P011InjectionReceipt | null;
// request: { perColumnHeavy: Float64Array(128), perColumnLight: Float64Array(128),
//            sourceId: number, sequence: number, inputTime: number, x: number, y: number, seed: number }
// 返回 receipt（B 契约九字段）；request 全零量时返回 null（页面不 feed tracer）。
```

**receipt 类型归属**：物理层**re-export** B 侧已入库的 `P011InjectionReceipt`（`from '../render/P012SandTracer'`）——**不复制定义**（防两处漂移；ArkTS 跨目录 import 无碍，render 模块不反向 import physics）。若 import 方向评审不通过，则 receipt 定义上移到 `P011RenderTypes`（render/physics 共享层），两侧同步改 import——二选一由 developer 实现时定，**字段结构不可变**。

### 1.2 调用点：物理子步前、planner 之前（裁定）

消费位置 = `step()` 的子步循环内、`this.saveBackup()` **之前**（:1296-1298 一带）：

```text
for (let index = 0; index < substeps; index++) {
  this.consumePendingGestureSand(substepDt);   // ← 新增：消费页面累积队列（≤1 笔/子步）
  this.saveBackup();
  this.updateSandTop();
  this.beginSubstepMotionBudget(substepDt, true, 1);
  ...
```

**为什么在 saveBackup 之前**：注入事务自身携带 shadow 审计（§2），成功写入后子步的 saveBackup 覆盖的是"已注入"的合法状态；若注入失败回滚，子步照常跑（零状态变化）。**为什么在 planner 之前**：注入使 ΔS=0（写 suspH/suspL），不改 tan30 面——planner 的可行域判定不受影响；但注入抬升 waterTop，而 planner preview 读 waterTop（容量判定）——注入先行保证预演见到的水面是新的。**队列消费 ≤1 笔/子步**：页面事件 60Hz、子步 60-120Hz，正常不积压；积压时（掉帧恢复）队列按 inputTime 顺序逐子步消费，`Δt≤1/30s` 钳制（算法 §2）防补喷。

## 2. 事务实现设计（工作 2）

### 2.1 shadow/backup 审计流程（顺序固定）

```text
injectGestureSand(request):
  0. 前置校验：request 数组 finite/非负/长度 128；qH/qL 同为零 → return null。
  1. this.saveBackup()                          // 事务级备份（与子步备份同机制）
  2. 路由校验（算法 §2 已在页面完成——物理层收到的是已路由 q[c]）：
     复核 perColumn 的列区间有效（0..127）且至少一列 q>0 有水（waterTop[c]-S[c]>EPS）；
     全部请求列干 → restoreBackup(); return receipt{admittedH:0,admittedL:0,
     rejectedReason:'NO_WATER', ...}            // 零状态变化
  3. λ 统一准入（对全部请求一次性）：
     a. 试探 λ=1：aH=λ·qH, aL=λ·qL 写 shadow 列（直接写 suspH/suspL——本事务的
        saveBackup 已兜底回滚）
     b. 全局余量界（必要）：Σ(H−waterTop[c]) ≥ Σ(aH+aL)？不足 → λ 降为
        Σ(H−waterTop)/Σ(qH+qL) 再试（算法 §3：全局和只是必要界）
     c. η 抬升求解（见 2.2）：失败（局部湿容量不可行）→ λ×0.5 重试；λ<λmin(1e-4)
        → restoreBackup(); return receipt{... admitted=λ'·q（可能为 0）, 
        rejectedReason: admitted≈0 ? 'CAPACITY_BLOCKED' : null}
  4. 审计链（全部通过才提交，任一失败按失败类型处理）：
     a. finite/range：suspH/suspL/waterTop 全列有限非负
     b. 相量守恒：VH' = VH+ΣaH、VL' = VL+ΣaL（getTotals 复核）
     c. 水量守恒：ΣwaterTop'−ΣwaterTop == 0（η 构造保证 Vw 不变——注入排水不毁水）
     d. 容量：S+susp ≤ waterTop' ≤ H 逐列
     e. tan30/planner：注入 ΔS=0 → sandTop 不变 → 不触发新违规；若注入前已存在
        违规边（audit(true) 非 null 的 tan30 项），仅在注入不加剧该边时放行
        （susp 不改 S，天然满足——登记豁免说明即可）
  5. 审计全过 → 提交：IH += ΣaH; IL += ΣaL; 更新 sourceEpoch（§3）;
     return receipt{admitted, rejectedReason: null}
  6. 任一审计失败 → restoreBackup(); λ 折半重试（回 3a）；λmin 以下 → 全回滚
     + receipt 带部分准入或 CAPACITY_BLOCKED
```

**关键纪律**：回滚用 `restoreBackup()`（既有 :4448）——与子步/旧 inject 同机制，无新回滚路径；λ 重试上限 8 次（对折），避免病态场景循环。

### 2.2 η 抬升求根（最小实现，不建新求解器）

复用 `waterLevel()` 的**单调二分模式**（:waterLevel low/high 界 + Σ 守恒目标），但目标函数不同——注入的排水构造（算法 §3）：

```ts
// 独立私有方法（不改动 waterLevel 本体）：
private solveInjectionWaterTop(aH: Float64Array, aL: Float64Array): boolean {
  // T[c] = S[c] + suspH'[c] + suspL'[c]（含新注入）
  // waterTop'[c] = min(H, max(T[c], waterTop[c] + η))，η≥0
  // 目标：Σ(waterTop'[c] − T[c]) = Vw（原自由水总量，注入前快照）
  // 二分 η ∈ [0, capacity]，单调性：Σ(waterTop'−T) 随 η 单调不减（min(H,·) 只截断不反转）
  // 收敛容差 EPS（与 waterLevel 同 1e-9 量级）；解出后逐列写 waterTop
  // 边界：η 解出但某列 waterTop'[c] > H 被截断 → 返回 false（局部湿容量不可行，
  //   触发 λ 折半）——算法 §3「触顶/局部湿容量/求解失败时缩小 λ 重解」
}
```

**与既有 waterLevel 的关系**：waterLevel 是**动力学**（每子步按速率 leveling），η 是**事务性瞬时构造**（只做排水所需的整体抬升）——两者并存不冲突：注入时刻用 η 定形，后续子步 waterLevel 照常演化（算法 §3「后续水面 leveling 再按既有速率工作」）。

### 2.3 预算纪律（红线复述）

注入**不扣** totalMotionBudget/bottom/local（算法 §3：外源不是场内搬运）；**注入后的沉降/滑移照常扣**——suspH/suspL 新增量进入 settleSandV2 的既有预算/守恒/guard 链，无任何豁免。不放宽 300/s 与 §19.5 guard。

## 3. 守恒账本扩展（工作 3）

| 项 | 实现 | 兼容策略 |
|---|---|---|
| IH/IL 累计 | 新私有字段 `sourceInjectedHeavy/Light`（构造/reset 清零；仅 §2.1-5 提交路径累加） | 旧无源 run 恒 0——公式退化 |
| expectedW/H/L | `expectedW = initialWater`（**不随注入变**——水守恒不变量）；`expectedH = initialHeavy + IH`；`expectedL = initialLight + IL` | **与旧 inject 的关键差异**：旧 inject（:1230-1231）把 initialHeavy/Light **重置为当前总量**（掩盖漂移的既有缺陷，算法 §3 点名）——新事务**不重置**，基准恒为 run 初值+累计准入 |
| 旧 inject 并存 | 旧接口语义不变（离散场景事件、重置基准）；**同一 run 混用两接口时守恒语义分叉**——登记限制：混用 run 的守恒审计按"最后使用的接口语义"解释，正式 run 不混用（单测覆盖） | 兼容成本最低的选择 |
| depositedFraction | 分母 = expectedH/L（含 IH/IL）；**sourceEpoch**：每次成功注入递增——注入期间/刚注入后 t90 无效（观测窗口重启） | 旧 run IH=IL=0 时分母=initialHeavy，与现状逐位一致 |
| ledger schema | `P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION` +1（v3→v4，追加字段 `sourceInjectedH/L/sourceEpoch/gestureAdmittedTotal`）；旧 run 缺字段按 0 容忍（checker 向后兼容惯例，§24.2-4 同模式） | schema revision 走既有流程 |
| drift 审计 | `audit(true)` 的守恒项分母换 expected 公式；**注入后的漂移=实际−期望**（沙增量不再是"漂移"——算法 §3 红线） | 无源 run 公式不变 |

## 4. NO_WATER/CAPACITY_BLOCKED 判定顺序（工作 4）

```text
判定序（§2.1 流程的位置）：
1. 全列干（q>0 的列全部 waterTop−S ≤ EPS）→ NO_WATER（λ 不进入）
2. λ=1 可行（全局+η+审计全过）→ 无 rejectedReason
3. λ 折半路径：λmin < λ < 1 可行 → 部分准入（admitted=λ·q，rejectedReason=null——
   部分准入不是"拒绝"，是缩放；receipt 的 requested 字段供 tracer 缩放密度）
4. λ ≤ λmin 或 η 不可行 → CAPACITY_BLOCKED（admitted 可能≈0 或精确 0）
receipt.rejectedReason 只在 1/4 填；admitted=0 且 rejectedReason=null 不出现
（零请求在 §2.1-0 已 return null）
```

## 5. 门控与手势接线预留（工作 5，与 B 设计 §4 对齐不重复）

物理层只暴露 `injectGestureSand`；**门控在页面**（B 设计 §4 已定）：`renderMode===MODE_WATERSAND && 画布命中 && !panelHitTest`。页面侧新增**累积队列**（`P011GestureSandQueue`，页面私有）：Pan onActionUpdate → 路由分样（算法 §2 的三角核/q[c] 分配在页面实现——含 Δt≤1/30s 钳制与停住即停）→ push；step 前的帧回调把队列交给 `consumePendingGestureSand`（§1.2）。**canP011GestureOwnInput 退役改造**：Pan→tiltX 删除（算法 §4）；该函数保留为非水沙模式防御断言。长按/Pan 竞争二选一规则照算法 §4。物理侧本任务书不实现页面部分（归 B 的 D3 接线任务，两任务书边界：**本任务=field 方法+队列消费+账本+schema；D3=Pan 路由+门控+feed tracer**）。

## 6. developer 任务书（工作 6）

### 6.1 实现拆解

| # | 模块 | 内容 | 验证 |
|---|---|---|---|
| P1 | receipt 对齐 | re-export B 契约（或上移共享层）；`P011GestureSandRequest` 类型 | 编译+字段逐一对照 P012SandTracer.ets:19-36 |
| P2 | injectGestureSand 主体 | §2.1 六步流程+λ 折半+η 求解（§2.2）+审计链 | 单测 §6.2-1..5 |
| P3 | 队列消费 | `consumePendingGestureSand(substepDt)`（§1.2 位置）+页面队列接口（`stageGestureSand(request)` 公开方法） | 单测 §6.2-6 |
| P4 | 账本+schema | IH/IL/expected/sourceEpoch/depositedFraction 分母（§3）+ ledger v4 字段 | 单测 §6.2-7..8 + checker 兼容烟测 |
| P5 | 旧 inject 零回归 | 全量既有测试跑绿（inject 用例不改动） | 117/117+planning 全绿 |

### 6.2 单测范围（Node 通道）

1. **守恒闭合**：注入 50 次随机量 → `VH−(initialHeavy+IH)` 与 `VL−(initialLight+IL)` 漂移 <1e-9；水量 `ΣwaterTop` 变化 = 0（η 排水不毁水）。
2. **λ 准入**：构造余量不足场（水近满）→ λ<1 部分准入，`admittedH/admittedL` 比例 = requested 比例（两相配比保持）。
3. **NO_WATER**：干场注入 → receipt 全零+reason；field 状态逐字段不变（快照 diff）。
4. **CAPACITY_BLOCKED**：满容+持续注入 → λ→0 路径；admitted≈0；无部分写入残留。
5. **事务回滚**：构造审计失败（注入后某列 susp 超 waterTop−S）→ restoreBackup 后状态与注入前逐位一致。
6. **队列时序**：stage 3 笔 → 3 个子步各消费 1 笔（FIFO）；Δt 钳制（单笔 Σq ≤ rate×1/30）。
7. **receipt 契约一致**：物理层返回的 receipt 序列化后与 B 侧 `feed()` 消费兼容（字段名/类型逐项断言——防两处漂移的机械检查）。
8. **schema v4**：注入 run 的 ledger 含新字段；旧 run（无源）读取字段缺省 0 不报错。
9. **t90 epoch**：注入后 sourceEpoch 递增；静置达标时刻在注入后重新计算。
10. **旧 inject 回归**：既有 inject 用例（场景初始化等）不改一字全绿。

### 6.3 验收口径（与 algorithm §5 ①-③ 对齐；④ 归 B 的 D3/V 场景）

| # | 场景 | 口径 |
|---|---|---|
| A1 | 1s 匀速划过水沙区 | 源事件总准入量 vs VH/VL 增量逐相比对（ledger）；局部先悬浮后沉积（时序断言） |
| A2 | 满容注入 | 逐渐部分准入→停注；水量漂移 <0.1%；无越容/非有限/新增 tan30 违规 |
| A3 | 传感器倾斜与 Pan 并存 | Pan 只改源量（receipt 数值）、姿态只由传感器改（tiltX 不被 Pan 写）——两通道正交断言 |

### 6.4 红线复述

不放宽 300/s 与 §19.5 guard（注入 ΔS=0 不触 guard，后续沉降照常受限）；不删旧 inject；不改冻结锚点；schema 走 revision（v4 向后兼容）；108run 正式批二进制不动（**工作区开发，合入等批后**——本任务实现与 B 的 bbe8e33 同属掉沙链挂起态）。

变更记录：2026-10-23，依据 manager【掉沙·物理层注入任务书】任务产出。六项工作：接口落点（re-export B 契约+子步前 saveBackup 前消费+≤1 笔/子步）/事务六步流程（λ 统一准入折半+η 单调二分最小实现+审计链五项+8 次重试上限）/守恒账本（IH/IL+expected 不重置基准+旧 inject 混用限制+schema v4+t90 epoch）/拒绝语义判定序/门控边界（页面归 D3，两任务书边界声明）/P1-P5 拆解+10 单测+A1-A3 验收。
