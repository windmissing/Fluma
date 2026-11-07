# P-012 s134 二期真机闭环回执 v1（effector）

- 日期：2026-10-24；执行：effector；设备 VYG-AL00，热态 60°C（thermal_zone0 全程 60000）
- 任务：6adb7038【P-012·s134二期·真机闭环】60d24bf 上 dyn 复测 + 8 提交二分 + seg3 三场景复测 + 133ms 主因归因（阻塞真机数据，P5）
- dida365 MCP 全程 403/unauthorized，任务正文未能读取，按标题+预登记口径执行，本回执本地归档
- 构建链：hvigorw assembleHap（--no-daemon）+ hap-sign-tool 手动签名；干净 worktree 逐提交检出

## 一、总体结论（四条全部有真机数据）

| # | 任务项 | 结论 |
|---|---|---|
| 1 | 60d24bf 上 dyn 复测 | **阴性**：q 阈值修复正确但打错目标，dyn 档仍 131ms/46% 烘培帧 |
| 2 | 8 提交二分 | **窗口内 8 提交全部免责**：基线 8f090db 同样慢；"s133 dyn 有效"是采集窗口假象 |
| 3 | seg3 三场景复测 | s136 正式口径 6 块已出数（§四），修复前正式基线恢复 |
| 4 | 133ms 主因归因 | **噪声场每帧全量重烘 110ms（83%）+ 双缓冲烘焙 20ms（15%）+ 绘制 2ms**；触发=phase 代理数值不稳定 |

## 二、60d24bf dyn 复测（s136 = 60d24bf 干净工作树构建）

seg3 settle/tilt/heavy × dyn，60°C：

| 场景 | n | phys P50 | render P50/P95 | 烘培帧 | 吞吐 |
|---|---|---|---|---|---|
| settle/dyn | 430 | 12 | 5 / 133 | 198 (46%) @131ms | 12.4 fps |
| tilt/dyn | 312 | 12 | 131 / 135 | 189 (61%) @133ms | 10.3 fps |
| heavy/dyn | 82 | 12 | 4 / 134 | 30 (37%) @132ms | 15.6 fps |

E3DIAG 取证（228 帧）：**219 帧 dirty 触发原因是 phase（P=true），qH/qL 增量恒 0**——60d24bf 的 q 相对化修复在真机上验证有效（q 不再触发），但 q 从来不是观测到的触发源。**修复未消除每帧 E3 成本，判阴性。**

## 三、8 提交二分（候选 9d11849/7ec8f14/2b7de23/0f62f48/bbe8e33/693a724/beb11fa/653dce7）

静态剪枝：0f62f48 纯计时埋点（diff 逐行核对）。9d11849 只改上屏采样步长（segU/segV 自适应），7ec8f14 只改外观参数化，均与 dirty 逻辑无因果静态路径；适配器 P012WaterSandAdapter 与 P012MaterialMotionCache 在窗口内零改动。

运行时探针（settle/dyn，同脚本同参数同热态，frameTimeMs 口径因 9d11849 早于 seg 埋点）：

| 探针 | n | P50 | mean | 吞吐 | >100ms 帧 |
|---|---|---|---|---|---|
| **b3-8f090db（s133 构建点，基线）** | 394 | 141 | 89 | **11.2 fps** | 212 (54%) |
| b2-9d11849（窗口首个） | 339 | 146 | 104 | 9.7 fps | 223 (66%) |
| b1-bbe8e33（窗口中点） | 349 | 143 | 100 | 10.0 fps | 221 (63%) |

**判决：基线 8f090db 与窗口内构建同处慢档（54% vs 63-66% 慢帧），窗口内不存在单一回归提交，8 提交全部免责。**

### 对文档旧结论的更正

docs/fps-seg3-P012-v0.md §6 的"s133+dyn = 18.3fps / render 13ms（dyn 消融在 s133 有效）→ s134+dyn 133ms（失效，回归定位于 s133..HEAD）"**不成立**。重析该次采集原始数据（seg3-s133-bisect-settle-dyn-hilog.txt，n=632）：慢帧成对分布于前 2/3（21%/18%），**后 1/3 零慢帧、mean 33ms**——18.3fps 是被安静尾部抬高的均值。同一构建点（8f090db）用现行同参数脚本复测为 11.2fps/54% 慢帧。每帧 E3 成本自 E3 相位接线（a46fcb5/9827739，10-22）起即存在，非 s133..HEAD 区间引入。

## 四、seg3 三场景正式复测（s136 = 60d24bf，ablate off 全链 + toplight，60°C）

| 场景/臂 | n | phys P50 | render P50/P95 | 烘培帧 | 吞吐 |
|---|---|---|---|---|---|
| settle/off | 353 | 11 | 133 / 135 | 221 (63%) @133ms | 9.9 fps |
| settle/toplight | 339 | 11 | 132 / 138 | 221 (65%) @134ms | 9.6 fps |
| tilt/off | 381 | 12 | 7 / 136 | 175 (46%) @132ms | 12.4 fps |
| tilt/toplight | 311 | 11 | 133 / 138 | 185 (59%) @134ms | 10.3 fps |
| heavy/off | 93 | 12 | 6 / 135 | 26 (28%) @133ms | 17.9 fps |
| heavy/toplight | 50 | 12 | 121 / 136 | 30 (60%) @133ms | 10.8 fps |

- toplight（E2 路线②原型）无可测成本增量（与 off 同档），E2 原型性能中性。
- render 全场景双峰（clean ≈5ms / dirty ≈133ms），烘培频率随场景静置程度变化（settle 63% → heavy 28-60%）。

## 五、133ms 主因归因（E3DIAG2 分段计时，settle/off，358 帧 × 2 次调用）

单帧 E3 路径成本分解（dirty 帧，P50）：

| 段 | 耗时 | 占比 | 说明 |
|---|---|---|---|
| **N1 噪声场 shiftAndBake** | **110ms** | **83%** | water 调用全量重烘；iface 调用同窗口已满足 → 0ms 短路 |
| 双缓冲烘焙 | 20ms | 15% | bakeWater 16ms + bakeIface 4ms |
| 12 档矩形带绘制 | 2ms | 1.5% | fillFogTiers，draw=1ms/次 |

### 触发机制（为什么每帧 dirty）

1. `fogPhaseX() = motionCache.meanDeltaXPx(true)` = **逐列 UV 平流偏移的均值，UV 被 wrap 到 256px tile 周期**（P012MaterialMotionCache.ets:96-98）。
2. settle 场景持续存在亚阈值质量再分布（悬浮微抖：层高增量 <LAYER_EPS 0.25、q 相对增量 <20%，故 L/Q 检测安静），但逐列 UV 持续漂移；**单列跨越 wrap 边界使均值跳变**——实测 phaseDelta 量子化于 {0, 92, 162, 210}px（阈值 0.5px）。
3. 该不稳定相位同时造成两条每帧成本：(a) E3 双缓冲 phase 检测每帧判 dirty → 20ms 烘焙；(b) **噪声场覆盖窗口 `needX0 = -fogPhaseX() - MARGIN` 每帧平移上百 px → shiftAndBake 的"同窗口短路"（|Δ|<0.5px）永不命中 → 全量重烘 600×1300 噪声格（110ms）**。
4. 近空列 deltaX 分母 `max(mbar, 1e-6)`（MotionCache:89）在列质量趋零时放大位移，是 wrap 跳变的放大条件之一。

### 归因结论

**133ms 主因 = N1 噪声场每帧全量重烘（110ms），其驱动是 phase 代理（wrap 均值）在 settle 下的数值不稳定；双缓冲烘焙（20ms）为次因。** 60d24bf 修复的 q 触发在真机上未出现（qH=qL=0），故对观测问题无效。

## 六、移交 developer 的修复方向（按预期收益排序）

1. **相位代理稳定化**（治本，同时消停 110+20ms）：mean 改在未 wrap 的累积量上计算后单次 wrap；或用逐帧增量 + 运动幅度死区（max|deltaX| < ε 列不参与）；近空列 deltaX 钳幅。
2. **噪声场维护门控**（治标保底）：shiftAndBake 入口对窗口平移量加下限（如 <2px 跳过），或相位变化幅度小于视觉阈值时冻结 needX0——iface 调用已验证同窗口 0ms 短路可行。
3. 90Hz 目标维持否决：单拍 11.1ms，phys P95 16ms 已超预算，M2 需先物理治理（与本回执一致，docs §8）。

## 七、红线与遗留

- 封存数据（108run 等）未混入本次结论；无锚点冻结前不出 PASS/VALIDATED。
- dida365 未授权，本回执 + docs 更新为唯一归档通道；任务状态回写待连接器恢复。
- E2 toplight 原型：性能中性已确认；视觉效果正式判定属 E2 任务项，未在本闭环内出数。
- s136/s136diag/s136diag2/b1/b2/b3 六个签名 HAP 存于 entry/build/default/outputs/default/（SHA256 见构建日志）。

## 八、产物索引

- 采集：test/artifacts/p012-measurements/seg3-s136-{settle,tilt,heavy}-{off,toplight,dyn}-hilog.txt（9 块）、seg3-b{1,2,3}-*-settle-dyn-hilog.txt（3 探针）、s136diag-full-hilog.txt（E3DIAG 触发原因）、s136diag2-full-hilog.txt（E3DIAG2 分段计时）
- 工具：tools/p012_seg3_capture_v2.sh（通用采集）、tools/seg3_quick.py（分段统计）、tools/p012_ablate_probe_s134.sh（A0 消融探针）
- 文档：docs/fps-seg3-P012-v0.md（v2 章节：本回执摘要 + 旧结论更正）
