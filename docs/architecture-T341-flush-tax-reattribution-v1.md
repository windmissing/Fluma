# T341 · glFlush 固定税机理重估与 Amendment 3 v1

日期：2026-10-31。责任：architect。任务卡：滴答 `6ae5749fe4b069bbd885cb60`。
状态：**MECHANISM_REATTRIBUTED / SINGLE_FIX_RECOMMENDED / AMENDMENT_3_READY / T340_PACKAGE_RULED**。
依据：`test-report-T337-fence-flush-device-v2.md`、`test/reports/t337b/`（rows/m2-m4/p3-p4 全套）、`architecture-T335-fence-stall-attribution-v1.md`（被证伪稿，如实引用）、T330/T336/T339 链、当前源码（PrepareProgram/Create 路径核对）。纯设计零代码。

## 0. 裁定总览

| 项 | 结论 |
|---|---|
| T335 摊销模型 | **证伪成立**（T337b：10 dispatch 后 flush 33.66ms vs T330 130 dispatch 后 fence 37-41ms——停顿量与命令数无关） |
| 机理二次归因 | **(a) 变体：GPU 空闲后的首次真实提交固定路径**（DVFS 频率爬升 + 驱动前端建立/恢复，count 无关的 ~33-41ms 一次性税）；(c) 进程级惰性初始化**被同 pid 复现杀死**；(b) 归并入 (a) |
| 修复唯一推荐 | **(i) 每 run 预热真实工作 flush**：冷窗口（Create 后、首 anchor 前）发 1 个真实 dispatch + glFlush 把税挪出热路径；**Amendment 2 分批 flush 布局保留**（预热生效后每次 flush 零税） |
| (ii) 回退 T329 | **拒绝为主方案**（回到已证伪起点）；作为预热真机失败时的降级预案登记 |
| (iii) GAP 豁免 | **再次拒绝**（T328 已拒，无新论据） |
| 六项条款处置 | 「逐批 ≤8ms」回归由预热修复（首批回到 ~1.7ms 基线）；Amendment 2 flush 布局**保留**；其余五项无回归不动 |
| T340 包选择 | **T338b 包（7aafce3f）立即可用**——M1/M2/M4 与 fence 税正交，不被预热卡阻塞；预热验证是独立卡线 |

## 1. T335 证伪事实确认（不回避）

T337b 三组对照数据彻底推翻 T335 的线性摊销模型：

| 数据点 | 命令数 | 停顿 |
|---|---:|---:|
| T330 run1 末批 fence | ~130 dispatch | 37.65ms |
| T330 run2 末批 fence | ~130 dispatch | 41.28ms |
| T337b 首批尾 flush（三 run 一致） | **10 dispatch** | **33.66ms** |

若 T335 模型成立，10 dispatch 的停顿应为 ~3ms——实测 33.66ms。**停顿量对命令数不敏感**：不是「每命令 ~300µs 的累积结账」，是「每次 flush/fence 类提交同步调用的固定税」。T335 §1.2 的「数量指纹」是伪相关（130 与 10 都落进同一固定税区间，per-dispatch 归一化只是把同一常数除以不同分母）。T335 推荐的分批 flush 因此必然无效——T337b 实测证实（税精确转移到首个 flush 调用点，且 12 次 flush×33ms 的推论成立时会更糟，本例首批即被 GAP 取消）。

## 2. 固定税机理二次归因

### 2.1 判别性证据链（四组事实）

| # | 事实 | 来源 | 排除/指向 |
|---|---|---|---|
| F1 | 同一进程（pid 20103）内 3 次 run 重启，每次首 flush 都收 35.244ms 税 | T337b rows（epoch 1-4 推进、三 run 逐位一致） | **杀死 (c) 进程级惰性初始化**（若一次初始化则 run2/3 免税） |
| F2 | Create() 内 PrepareProgram 已做零工作 dispatch + glFinish，税仍在首次真实 flush 出现 | full_transport.cpp:13 调 PrepareProgram（submit_observer.h:33 `glDispatchCompute(0,1,1);glFinish()`）；T317 交付明言「不承诺零组准备已消除设备驱动的全部延迟编译」 | **零工作提交不触发税**——税绑定「有真实工作的提交路径」 |
| F3 | 10 与 130 dispatch 同税（§1 表） | T330 vs T337b | 税与命令量无关，排除「按量 resolve」类机理 |
| F4 | 每 run 重启前必有 Pause/GAP 期（GPU 空闲）；税在 run 内后续提交中不再出现（T330 的 12 个非末批 0.1-0.2ms、T337b dispatch 5-11µs） | T330/T337b 分账 | 税与「空闲后首次」绑定，GPU 连续工作时免 |

### 2.2 机理裁定

**(a) 变体采纳：GPU 空闲后首次真实提交的固定路径税**——由两个叠加成分构成：
1. **DVFS 频率爬升**：GPU 在 Pause/GAP 闲期降频，首次真实提交触发频率恢复，爬升期数十 ms 量级与 33-41ms 高度吻合；
2. **驱动提交前端建立/恢复**：tile-based 驱动对空闲后的首个提交批次做前端重建（command buffer 映射、tile 内存池恢复）——固定路径、count 无关。

(b) 「tile 首次 resolve 固定代价」**归并入 (a)**——该固定路径就是 resolve/submit 前端建立本身，无需独立假设。(c) 已被 F1 杀死。**(a)+(c') 的每 run 复现**（F1/F4）说明税绑定「空闲间隔后的首次」而非「进程生命周期首次」——与 DVFS+前端恢复的双重解释一致。

**不判驱动 bug**：DVFS 与前端恢复是移动 GPU 的标准行为；问题在调用侧把首次提交放在了计费热路径上。

### 2.3 可判别证据（供真机验证卡消费）

- **判据 A（税挪移）**：若预热生效，热路径首批 SubmitExit wall 应回到 T330 基线（~1.7ms）而预热调用自身收 33-41ms——税**转移**而非消失；
- **判据 B（每 run 复现）**：同进程多 run 下每次 run 的预热都应收税（F1 预测）；若某次 run 预热免税（GPU 未降频），该 run 热路径也应免税——税与预热的绑定关系可交叉验证；
- **判据 C（间隔依赖）**：人为延长 run 间空闲（如 10s）后预热税应更稳定出现；缩短间隔（立即重启）可能免税——佐证 DVFS 成分。

## 3. 修复方向二次裁定（唯一推荐 (i) 预热）

### 3.1 四方向对照

| 方向 | 热路径效果 | 语义代价 | 判定 |
|---|---|---|---|
| **(i) 每 run 预热真实 flush** | 税挪入冷窗口（Create 后、首 anchor 前——T317 已确立的非计费期），热路径首批回到 ~1.7ms、分批 flush 每次零税 | 冷窗口 +33-41ms（本来就是冷准备期，不占 8ms 预算/33ms GAP 窗）；预热 wall 以 warmupWallNs 披露（不隐藏） | **✅ 唯一推荐** |
| (ii) 回退 T329（flush 只在末批） | 末批 fence 收 37-41ms 税——T330 已证击穿 8ms 预算与 33ms 窗 | 回到已证伪起点 | ❌ 主方案拒绝；**登记为预热失败时的降级预案**（见 §3.3） |
| (iii) GAP 豁免 | 不触及税本身 | T328 已拒（停摆检测盲区扩大、违反不钳 delta），无新论据 | ❌ 再次拒绝 |
| (iv) 驱动侧规避（fence 替代原语/pbuffer 属性） | 不确定 | 引入新 GL 依赖面，且 (i) 未失败前无必要 | ❌ 不评估 |

### 3.2 方案 (i) 设计

**位置**：`FullM1Transport::Create()` 尾部（PrepareProgram 之后）——每 run 冷窗口一次。

**内容**（预热调用，~3 行）：
```cpp
// Warm-up: pay the fixed first-real-submit-after-idle tax (DVFS ramp +
// driver frontend bring-up, T341 §2) in the cold window, before the first
// anchor. Real work required — the existing zero-work glFinish in
// PrepareProgram does NOT trigger the submission path (T337b F2).
glDispatchCompute(1,1,1);          // 1 workgroup, existing program/bindings from PrepareProgram
glFlush();                          // force the submission path
warmupWallNs = <elapsed>;           // disclose, INV-10
```

**安全性**：PrepareProgram 已绑定 4 个 SSBO 且零工作 dispatch 已验证不写状态（T317 交付）；1 workgroup 的真实 dispatch 复用同一 program 与绑定——**写目标与零工作预检相同**（candidate/workspace 缓冲），预热写入的中间值会被 run 的首次真实 root 全量覆盖（pass 0 全场重算）——无状态污染路径。需在实施卡断言：预热 dispatch 不触碰 results/publication/eventArenas（绑定即 PrepareProgram 的四个，天然满足）。

**遥测**：新字段 `warmupWallNs`（bridge status 追加）；41 字典新增 stage 42 `SubmitWarmup`（Enter/Exit 一对，每 run 一次）——预热墙钟原值披露，不隐藏（INV-10：预热挪税不构成隐藏长根——热路径后续提交的真实成本仍逐批计量）。

### 3.3 降级预案（仅登记，不预派）

若真机验证发现预热后热路径 flush 仍收税（机理 (a) 判定错误、税不绑定首次）：
1. 回退 Amendment 2 的分批 flush（ExecuteBatch 非末批 flush 删除）——回到 T329 布局；
2. 此时末批 fence 单次 37-41ms 税仍击穿 8ms 预算/33ms 窗——**该状态 T330 已证不可用**，须重开 GAP 窗语义讨论（manager 级，非本卡预决）；
3. 该预案的触发判据：真机 K 判据「预热后首批 SubmitExit ≤8ms」FAIL。

## 4. Amendment 3（T324 合同第三次修订）

- **§2.2 增补（Amendment 3）**：Create() 尾部预热真实 flush（§3.2 设计）；新不变式 **INV-10**（预热墙钟原值披露 warmupWallNs + stage 42；预热在冷窗口非计费期，不占 8ms 预算/33ms GAP 窗，不构成隐藏长根）；
- **Amendment 2 处置：保留**——分批 flush 布局在预热生效后每次 flush 零税（F4：GPU 连续工作时不收税），其防御价值（限制单次 flush 面对的命令量）不变；
- **六项条款处置**：①nonce ②carry ④取消 ⑤submitting≠inflight ⑥41 字典——无回归维持；③「逐批 ≤8ms」回归由预热修复（预期首批回 ~1.7ms 基线），Amendment 3 落地后由验证卡复测；
- **41→42 字典**：`T324_STAGE42_V1`（旧 1-41 不变，新 42=SubmitWarmup）。

### 实施/验证规格

**developer 实施卡（建议 T344——T342/T343 已被 manager 14:15 方向变更派工占用为 CPU 线材质修复卡，本线实施卡编号顺延）**：Create() 尾 3 行预热 + warmupWallNs + stage 42 + 非机回归（T336 的 CASE_F 全量保持——flush 计数不变；新增用例：预热 dispatch 后 candidate 缓冲被首 root 全量覆盖的等价断言、warmupWallNs 披露断言）。**方向变更备注（2026-10-31 14:15 用户指令）**：目标 3 转 CPU 线优先、GPU 线降为备选——本实施卡与 W1-W5 验证线随之降级为备选线保留（不新增投入，待 CPU 线收口后重估）；Amendment 3 合同文本维持有效，实施时机由 manager 排程。

**tester 真机卡（T342 后续）**：
- W1：stage 42 每 run 恰一次，warmupWallNs 原值（预期 33-41ms 量级，判据 A）；
- W2：热路径首批 SubmitExit wall ≤8ms（回到 T330 基线 ~1.7ms，「逐批 ≤8ms」回归修复）；
- W3：分批 flush 各批 wall 正常（≤1ms，无税复现）；
- W4：root 封尾到达 + accepted ≥1 + 双源同 token 链（J3 遗留目标）；
- W5：六项无回归 + K5 环 4096 COMPLETE 保持；
- 判据 B/C（§2.3）为诊断增强项，不设 PASS/FAIL 门。

## 5. T340 包选择裁定

**T340 用 T338b 包（7aafce3f）立即可执行 M1/M2/M4**：
- T340 的 M1（watchdog 死亡族设备复核）/M2（单事件 ≤600ms）/M4 与 fence 税问题**正交**——T338 已在该包上完成 M1/M2 PASS，T340 是其 t284 生成树 harness 的 M3 补测与 M4 收尾；
- fence 税修复线（T342 预热 + W 判据）是**独立验证线**，不构成 T340 的门——两线并行不互阻；
- 唯一注意：T340 若顺带复测 fence 相关判据（K 系列），其 T338b 包结果**不适用**于 Amendment 3 评估（包不含预热）——届时换 T342 包。

## 6. 路由

- 本卡归因+Amendment 3 → **developer 实施卡（建议 T344，编号顺延因 T342/T343 已被 CPU 线材质修复占用；备选线——方向变更后 GPU 线不新增投入，实施时机由 manager 排程）**：预热 3 行 + warmupWallNs + stage 42 + 回归；
- → **tester 真机卡**：W1-W5；
- **T340 即刻解锁**（T338b 包执行 M1/M2/M4，fence 线独立）；
- 与 T328/T335 矛盾处置：T335 摊销模型**如实宣告证伪**（§1），其 (ii)(iii) 方向裁定与 GAP 不放宽原则**维持有效**；T328 六项无回归条款维持；Amendment 2 保留但机理表述修正（分批 flush 的价值从「摊销税」修正为「限制单次提交命令量」——其原动机已在 T324 §7 变更记录中如实标注）。

## 7. 变更记录

- 2026-10-31 v1：初版。T335 摊销模型证伪确认（10 vs 130 dispatch 同税）；机理二次归因 (a) 变体（DVFS+前端恢复的空闲后首次真实提交固定税，F1-F4 判别链：同 pid 每 run 复现杀 (c)、零工作 glFinish 不触税、count 无关、连续工作免税）；唯一推荐 (i) 每 run 预热真实 flush（冷窗口挪税+INV-10 披露）；Amendment 2 保留（机理表述修正）；(ii) 拒绝主方案登记降级预案、(iii) 再次拒绝；Amendment 3（Create 尾预热+warmupWallNs+stage 42）；W1-W5 判据；T340 用 T338b 包即刻执行 M1/M2/M4。
