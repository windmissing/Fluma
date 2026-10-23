# P-011 correction 矩阵：conditioning 通道真机验证回执 v1

日期：2026-10-22 夜间。执行：effector。状态：**通道接通 / stratum 因子未激发（STRATUM_FACTOR_NOT_EXCITED）**。本回执只做通道门控与事实登记，不做任何效果判定（统计提取归 tester，转正归 manager）。

依据：`preregistration-P011-correction-matrix-v1.md` §4（场景/配对）、§1（权重可辨识性检查）、§7.2（身份/测量失败即停）；任务【correction·矩阵正式108run】要求"先验证 conditioning 通道"。

## 1. 验证对象与判据

正式 108run 批要求 `p011conditioning=<stratum>` 在正式九相之前注入一段 conditioning 前缀，把场从 reset 态推到该 stratum 的工作初态。三个问题必须在开批前回答：

| 编号 | 问题 | 判据（采前定） |
|---|---|---|
| Q1 | 通道是否接通 | 账本出现 `CND_*` 段，且正式段首个命令相对首记录的偏移 ≈ 前缀时长 65 s |
| Q2 | 前缀是否真在做事 | 正式段起点的场状态 ≠ 未处理（none）状态的场状态 |
| Q3 | G1/G2/G0 是否可区分 | 跨档（G1 vs G2 等）状态差异显著大于同档重复（G1 vs G1）差异——即差异来自档位而非同输入下的逐 run 抖动 |

## 2. 采集配置

| 项 | 值 |
|---|---|
| 构建 | `entry-default-s133-signed.hap`，SHA256 `b0c67920c9e2f0b5b54f86f66f5f3de5e4900ca76c7783bd3e2e4d9df124d6f3` |
| 源码 | commit `9d11849`；`WaterSandField.ets` SHA256 由脚本实时计算并写入 want 执行身份 |
| 设备 | 62T0225B10010020，真机；testmode=true、rendermode=4、wspreset=thick、wscols=128、wscapacity=192、wssandtopmax=28 |
| 输入 | `p011schedule=18.7:0.5`（九相模板，CSV `calibration/schedule-obs3v3-A18.7-rate0.5.csv` 已哈希校验） |
| 变体 | 全部 8 run 为 A（原 relax，route 不绑）——只隔离 conditioning 这一个因素 |
| 阈值 | 26/20 factory-recommended，thresholdCalibrationId=user-configured |
| seed | 202610220（验证专用，与正式批 block seed 不重叠） |
| 单 run 时长 | 100 s 强制停止（前缀 65 s + 正式约 35 s），**短 run，不进任何统计** |
| 产物 | `test/artifacts/p011-corr-matrix-cndverify-20261022/`（含 `collect-manifest.json`、`cnd-verify-summary.json`、每 run 账本） |

run 清单：首轮 `cnd-G1-A / cnd-G2-A / cnd-G0-A / cnd-none-A`；补轮（同档重复，用于抖动基线）`cnd-r2-G1-A / cnd-r2-G2-A / cnd-r3-G1-A / cnd-r3-G2-A`。共 8 run，约 21 分钟。

## 3. Q1：通道接通（成立）

| run | 记录数 | CND_ 段 | 首记录段 | 正式首个命令段 | 正式起点相对首记录偏移 |
|---|---:|---|---|---|---:|
| cnd-G1-A | 2255 | CND_H0/H15/HZ/Hm15/HE/CND_R（6 类） | CND_H0 | H0 | 65 064 ms |
| cnd-G2-A | 2251 | 同上 | CND_H0 | H0 | 65 067 ms |
| cnd-G0-A | 2259 | 同上 | CND_H0 | H0 | 65 022 ms |
| cnd-none-A | 2512 | 无 | H0 | H0 | 0 ms |

正式段起点相对首记录平移 65.02–65.07 s，与前缀设计时长（5 个 1 s 保持 + 4 段 1°/s 斜坡，共 65 000 ms）一致；`none` 档无 CND_ 段、偏移 0——want 参数确实解析并驱动了播放器。

## 4. Q2：前缀在做事（成立）

以正式段起点 `stateBefore.S` 与未处理 reset 态比较：

| 对照 | max ΔS | rms ΔS | 结论 |
|---|---:|---:|---|
| G1-formal vs reset | 0.6487 | 0.4877 | 明显不同 |
| G2-formal vs reset | 0.6473 | 0.4873 | 明显不同 |
| G0-formal vs reset | 0.6538 | 0.4944 | 明显不同 |

状态哈希：reset `7570e1e52a471693`；G1 `20ad79c3ccadbd29`、G2 `0cba689aeb5e92db`、G0 `8dee2babac7e4357`。7 组「已处理 vs 未处理」对照平均 maxΔS = 0.653。前缀确实把场推离了 reset 态。

## 5. Q3：G1/G2/G0 可区分吗（**不成立**）

| 对照池 | 配对数 | max ΔS 均值 | max ΔS 范围 | rms ΔS 均值 |
|---|---:|---:|---:|---:|
| 同档重复（G1 vs G1、G2 vs G2，抖动地板） | 6 | **0.025964** | 0.020937 – 0.038798 | 0.010261 |
| 跨档（G1/G2/G0 两两） | 15 | **0.029149** | 0.014611 – 0.043702 | 0.010282 |

跨档 / 同档 = **1.123**，跨档差异没有超出同输入重复 run 的抖动地板；两组范围几乎完全重叠。三个 stratum 标签把场送到**同一个分布**的工作初态。

### 5.1 根因（代码层，可复核）

`entry/src/main/ets/physics/P011SchedulePlayer.ets` 的 `p011BuildConditioningPrefix(kind)` 中，`kind` 只被回填到返回结构体的标签字段，命令构造部分（`hold`/`rampTo` 调用序列 0→+15→0→−15→0）对 G1/G2/G0 完全相同。也就是说：注入通道修好了，但三档共用同一条注入序列。

这与 c054021 的提交说明一致——该提交只承诺"p011conditioning ∈ {G1,G2,G0,none} 且前缀在正式 schedule 之前派发"，未承诺三档内容不同；预注册 §4 要求的是三种结构性不同的初态（竞争场 / 非均匀热点与邻边约束 / 均匀无正贡献控制），目前只做到了"one shared conditioned state"。

### 5.2 判定

**STRATUM_FACTOR_NOT_EXCITED**：按预注册 §1「权重可辨识性检查」的同一逻辑——若用来比较的场景不能区分被比较的因素，应如实报告因子未激发，不得临场改算法或输入去制造差异。

## 6. 对正式 108run 批的影响与本次裁决

1. 采集价值：**A/B/C/D 四组对照仍然成立**（这是主比较 D vs A 的对象），G1/G2/G0 三档退化为"同一 working state 下 3 个可交换的分层标签"。
2. **不再成立**的口径：G2 稳健性比较（同一 gain 在邻边受限场是否不倒退）、G0 控制（无正贡献时的诚实停滞）失去对照意义；把三档分开统计会造成伪重复（同一分布重复采样会让 block bootstrap 的 CI 虚假收窄）。
3. **本次裁决（用户 2026-10-22 夜间）**：照常跑满 108run，并按"27 个可交换 block、单一 working state"登记；回执与后续统计必须带 `STRATUM_FACTOR_NOT_EXCITED` 标记，不得把 G1/G2/G0 当作三个独立场景写入结论。
4. 正式批执行登记：`calibration/p011-corr-matrix-formal-plan-v1.json`（master seed 20261022，Williams 序列按 block 轮转，block 执行序冻结）；采集脚本 `tools/run_p011_corr_matrix_formal.ps1`，每 run 带身份哨兵、前缀哨兵、硬安全哨兵，任一失败即停批。

## 7. 上报项（非本会话职责，不自行修代码）

| 对象 | 事项 |
|---|---|
| architect / developer | 需要真正区分 G1/G2/G0 的初态注入（三种结构不同的场：竞争场 / 非均匀热点与邻边约束 / 均匀无正贡献控制），并在冻结前给出影子核验（预注册 §4 要求"不能通过减少其他通道需求优化 D"） |
| designer | 确认"27 个可交换 block、单一 working state"这一降级口径在统计层的写法（是否等价于把首轮样本从 12 block 扩到 27 block，是否需要撤销 G2/G0 的假设） |
| tester | 统计提取时不得把 G1/G2/G0 当独立场景；主比较只能在"单一 working state"这一适用域内陈述 |

## 8. 红线遵守

- 8 个验证 run 全部 only variant A（身份回执 8/8，`identityOk=True`）——没有靠标签冒充变体。
- 验证 run 为 100 s 短 run，**明确不进任何统计**，不用于主比较或 pilot 之外的任何结论。
- 未改任何物理/渲染代码；验证只使用冻结构建 s133。
- 三档同分布这一结论是实测 + 源码双向确认，没有事后挑选对照对或换指标。
- 滴答清单 MCP 本轮返回 unauthorized，回执未能写回任务评论（见下节），待授权恢复后补写；本地文档先行落盘。

## 9. 正式批首 run 的 tan30 边界接触（登记，不隐藏）

正式批第 1 run（`corr-formal-G1-b04-D-r001`，D 变体）的实测：

| 项 | 值 |
|---|---|
| 记录数 / 前缀记录 | 5 821 / 1 422（正式起点偏移 65 036 ms） |
| 身份 | variants=[D]，D-1b 闭合（15 239 = 14 947 + 292），stalled 204 |
| 硬安全 False 计数 | range/conservation/finite/stock/capacity 全 0（引擎自身闸门通过） |
| `maxSandTopInvariantError` | 0.5773502691896465，出现在 effectiveTilt = −18° |
| 与 tan30 的关系 | tan30 = 0.5773502691896257；差值 2.1e-14，**在浮点表示层面等于 tan30**（即"贴着边界"，未越过） |
| 预算超额 | 5.33e-14（浮点噪声量级） |

**哨兵判据修正（本会话，已登记）**：采集脚本第一版把判据写成"maxErr ≤ tan30"（无容差），比引擎自身的闸门更严，于是在第 1 run 触发 `INVARIANT FAILURE` 停批。引擎的正式判据是 `WaterSandField.ets:1917` 的 `maxSandTopInvariantError <= tan(30°) + EPS`，`EPS = 1e-9`（同文件 556 行），其结论落在 `audit.range` 字段上。已把哨兵改为与引擎同一容差（`tan30 + 1e-9`），并新增 `invariantAtBoundary` 字段逐 run 记录"是否贴到边界"。第 1 run 未重新采集，直接按已存扫描结果重新判定（不删、不改、不重跑）。

**需要 designer 知道的事实**：pilot（无前缀）的 maxSandTopInvariantError 峰值为 0.499–0.532，加了 conditioning 前缀后第 1 run 峰值贴到 tan30 边界（−18° 处）。当前仍在引擎闸门内（range=True），但余量已耗尽；若后续 run 出现 `range=False`，脚本会立即停批，不会带病跑完。

## 10. 正式批进展（截至 6/108，采集进行中）

| run | 计划 ID | 变体 | 记录数 | 前缀记录 | 身份 | D-1b 闭合 | 硬安全 False | maxSandTopInvariantError | 贴边界 | 单 run 墙钟 |
|---|---|---|---:|---:|---|---|---|---:|---|---:|
| 1 | corr-formal-G1-b04-D-r001 | D | 5 816 | 1 426 | [D] ✓ | ✓ | 全 0 ✓ | 0.5773502691896251 | 是 | ~385 s |
| 2 | corr-formal-G1-b04-A-r002 | A | 5 813 | 1 428 | [A] ✓ | ✓（A 组计数不适用） | 全 0 ✓ | 0.49972836359194345 | 否 | ~385 s |
| 3 | corr-formal-G1-b04-C-r003 | C | 5 846 | 1 434 | [C] ✓ | ✓ | 全 0 ✓ | 0.5773502691896535 | 是 | ~385 s |
| 4 | corr-formal-G1-b04-B-r004 | B | 5 818 | 1 425 | [B] ✓ | ✓ | 全 0 ✓ | 0.5773502691896464 | 是 | ~385 s |
| 5 | corr-formal-G1-b08-D-r005 | D | 5 836 | 1 439 | [D] ✓ | ✓ | 全 0 ✓ | 0.5773502691896340 | 是 | ~385 s |
| 6 | corr-formal-G1-b08-A-r006 | A | 5 790 | 1 423 | [A] ✓ | ✓（A 组计数不适用） | 全 0 ✓ | 0.5010211934187098 | 否 | ~385 s |

- 成本：约 6.4 min/run（270 s 物理 + 2.1 GB 账本回收与扫描），108 run 预计 **≈11.5 h**（预注册估的 8.35 h 不含 conditioning 前缀的 65 s/run，也不含 2.1 GB 级账本的回收）。
- 磁盘：每 run 只保留 trimmed 账本（2.11 GB），原始账本在扫描确认无误后释放（字节级等价，仅尾部残留行被裁），108 run ≈ 228 GB，D 盘剩余 541 GB 足够。
- 首 run 的第一次采集因哨兵判据过严被判停批，其目录已按"不删失败 run"原则保存为 `corr-formal-G1-b04-D-r001-partial-<ts>`；第 1 run 随后重采（新 runId）。两版都在盘上，统计取后者，前者仅作诊断。

**观察（登记，不等于效果结论）**：前两个 block 里 B/C/D 三组的 maxSandTopInvariantError 峰值都贴在 tan30 边界（0.5773502691896xx 区间内浮点抖动），A 组峰值为 0.4997 / 0.5010。这提示带预留的修正臂会把局部界面偏差推到硬闸门允许的上限，A 组留有约 0.077 的余量。当前全部仍在引擎闸门内（range=True），但一旦某 run 出现 range=False，脚本立即停批。该观察需交给 designer/tester 判断：是"余量被用尽"（需要讨论 30° 形变上界的适用性），还是修正臂的正常饱和行为。

## 11. 附：未能写回的部分

本轮 `dida365` MCP 工具对所有请求返回 `unauthorized`（get_task_by_id / get_project_by_id 均失败），因此：任务正文细节未能再次读取（本回执依据任务标题与预注册稿执行）、回执未能写入任务评论。本地已落盘的等价文件：本文件、`test/artifacts/p011-corr-matrix-cndverify-20261022/collect-manifest.json`、`cnd-verify-summary.json`。授权恢复后需补一条评论，内容为第 3/4/5 节结论与第 7 节上报项。
