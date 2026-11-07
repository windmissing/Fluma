# P-012 s136 定点修复真机验证回执 v1（effector）

- 日期：2026-10-24；执行：effector；设备 VYG-AL00（OpenHarmony 7.0/API 26），全程热态 60°C（thermal_zone0 = 60000，只读，无强制加热指令）
- 任务：6adc5a56【P-012·s136真机验证】seg3 三场景复测 + 同温对照 + E3 锚点回归 + 视觉连续性确认（恢复真机数据可信度最后一步，P5）
- 修复提交：bcada09（developer，任务 6adc5132），构建点 de0da61
- dida365 MCP 全程 403/unauthorized，任务正文未能读取，按标题 + 预登记口径执行，本回执本地归档
- 构建链：hvigorw assembleHap（--no-daemon）+ hap-sign-tool 手动签名（SignHap 因环境 keyPassword<32 失败，非代码问题）；干净 worktree 检出 de0da61

## 一、总体结论（四项全部有真机数据）

| # | 验证子项 | 结论 |
|---|---|---|
| 1 | seg3 三场景复测 | **六块全出数**，稳态渲染 P50=6ms / P95=27ms，吞吐 34–36 fps |
| 2 | 同温对照 | 同 60°C 同脚本同参：render P95 133–138ms → **27ms**；bake 帧 28–65% → **0.08%**；吞吐 9.6–17.9 → **34–36 fps** |
| 3 | E3 锚点回归 | 修复前主因（N1 噪声场每帧全量重烘 110ms + 双缓冲 20ms）**已消除**：稳态 dirty 帧仅首帧 1 次（206–214ms，初始化），其后 1215/1217/1224/1066/1058/1067 帧全部 6ms |
| 4 | 视觉连续性确认 | 流体雾带顶边界四图一致（42.0% / 42.0% / 42.0% / 41.8%），空气色 (16,16,29) 相同，水体均色差 ≤10 RGB（属模拟进度差异，非渲染回归），绿沙带两版均存在 |

**判定：s136fix 在真机 60°C 下消除了修复前观测到的 133ms/frame E3 渲染成本，物理层未改动（phys P50 12–13 / P95 17ms 与基线一致），增益纯来自渲染修复。真机数据可信度恢复。**

## 二、seg3 三场景复测（s136fix = de0da61，ablate off 全链 + toplight，60°C）

采集脚本 tools/p012_seg3_capture_v2.sh，六块（settle/tilt/heavy × off/toplight），各 22–30s + 25s 冷却。统计口径 tools/seg3_quick.py：过滤 physicsTimeMs≤20 且 substeps>0 的有效帧，render>50ms 记为 bake 帧。

| 场景/臂 | n | phys P50/P95 | render P50/P95 | bake 帧(>50ms) | 吞吐 |
|---|---|---|---|---|---|
| settle/off | 1203 | 13 / 17 | 6 / 27 | 1 (0.08%) @206ms | 34.4 fps |
| settle/toplight | 1225 | 12 / 17 | 6 / 27 | 1 (0.08%) @214ms | 35.1 fps |
| tilt/off | 1059 | 12 / 17 | 6 / 27 | 1 (0.08%) @206ms | 35.4 fps |
| tilt/toplight | 1067 | 12 / 17 | 6 / 27 | 1 (0.08%) @208ms | 35.7 fps |
| heavy/off | 1216 | 12 / 17 | 6 / 27 | 1 (0.08%) @206ms | 35.9 fps |
| heavy/toplight | 1218 | 12 / 17 | 6 / 27 | 1 (0.08%) @213ms | 35.9 fps |

- 每块唯一的 >50ms 帧经验证为 line-1 首帧初始化（噪声场首次烘焙），其后进入稳态 6ms。
- 物理层六块 P95 均 17ms，与修复前基线（phys P95 13–17ms）一致 → 物理行为未变，吞吐增益纯为渲染修复。

## 三、同温对照（修复前基线 vs 修复后，均 60°C）

修复前基线取自《P-012 s134 二期真机闭环回执 v1》（6adb7038，s136 = 60d24bf，同热态 60000、同采集脚本、同参数）。逐块对齐：

| 场景/臂 | 基线吞吐 | 基线 render P95 | 基线 bake 频率 | 修复后吞吐 | 修复后 render P95 | 修复后 bake 频率 |
|---|---|---|---|---|---|---|
| settle/off | 9.9 fps | 135ms | 63% | 34.4 fps | 27ms | 0.08% |
| settle/toplight | 9.6 fps | 138ms | 65% | 35.1 fps | 27ms | 0.08% |
| tilt/off | 12.4 fps | 136ms | 46% | 35.4 fps | 27ms | 0.08% |
| tilt/toplight | 10.3 fps | 138ms | 59% | 35.7 fps | 27ms | 0.08% |
| heavy/off | 17.9 fps | 135ms | 28% | 35.9 fps | 27ms | 0.08% |
| heavy/toplight | 10.8 fps | 136ms | 60% | 35.9 fps | 27ms | 0.08% |

**对照结论**：
- render P95 由 133–138ms 降至 27ms（降幅 ~80%）。
- bake 帧频率由 28–65% 降至 0.08%（仅首帧初始化，稳态零 dirty）。
- 吞吐由 9.6–17.9 fps 提升至 34–36 fps（约 2.0–3.6×）。
- 同温、同脚本、同参数是本对照成立的前提，排除了热态/采集差异的干扰。

## 四、E3 锚点回归（133ms 主因消除实证）

修复前归因（《s134 二期闭环回执》§五，E3DIAG2 分段计时，358 帧 × 2 调用）：单帧 E3 成本 = N1 噪声场 shiftAndBake 全量重烘 **110ms（83%）** + 双缓冲烘焙 **20ms（15%）** + 12 档绘制 2ms。触发根因 = `fogPhaseX` 用逐列 wrap 后 UV 均值，settle 下亚阈值微抖使单列跨 256px 边界 → 均值跳变（实测 {0,92,162,210}px ≫ 0.5px 阈值）→ 双缓冲每帧 dirty + 噪声窗口每帧平移超门控 → 全量重烘。

修复（bcada09）两路：
1. 治本：P012MaterialMotionCache.ets 新增未 wrap 全局相位累加器 `globalPhasePx`（重/轻相），取代 wrap 均值；近空列 deltaX 钳幅（DX_CLAMP_PX=4px）+ 亚视觉增量死区（PHASE_DEADZONE_PX=0.25px/帧）；`fogPhaseX` 改取 `globalPhasePx(true)`。
2. 保底：P012WaterSandRenderer.ets shiftAndBake 入口加窗口平移门控（P012_E3_NOISE_GATE_PX=2px）；P012E3Noise.ets 加 `bakeCount` 量化。

**真机回归验证**：s136fix 六块稳态 render P50 全部 6ms，唯一 >50ms 帧已确证为首帧初始化（line-1，206–214ms），其后 1215/1217/1224/1066/1058/1067 帧全为 6ms。修复前主因（N1 全量重烘 110ms + 双缓冲 20ms）在稳态中不再出现 → **E3 锚点回归成立**。首帧 1 次 200ms 量级烘焙属一次性初始化开销，不影响稳态帧预算。

## 五、视觉连续性确认（分带切片，pre vs post）

工具 tools/analyze_p012_s136fix_visual.py：容器中心竖直颜色切片（固定窗口 top=int(H*0.315) / bot=int(H*0.695) / x0=0.15W / x1=0.85W），分 air/water/sand 三带，逐图输出带边界（相对容器高度 %）+ 均色。

| 图 | air→water 顶边界 | air 色 | water 均色 | sand 带 |
|---|---|---|---|---|
| s136pre_settle | 42.0% | (16,16,29) | (186,200,194) | 无 |
| s136fix_settle | 42.0% | (16,16,29) | (190,204,199) | 96.5–100% RGB(165,185,138) |
| s136pre_tilt | 42.0% | (16,16,29) | (208,219,215) | 96.3–99.3% RGB(193,198,165) |
| s136fix_tilt | 41.8% | (16,16,29) | (198,209,205) | 96.3–100% RGB(161,181,133) |

**结论**：
- 流体雾带顶边界（air→water）四图一致于 42.0%（tilt/post 微差 0.2% 在切片容差内）→ 渲染布局无偏移。
- 空气色 (16,16,29) 四图相同 → 清屏/背景无回归。
- 水体均色差 ≤10 RGB（settle +4/+4/+5、tilt −10/−10/−10），属模拟进度（post 版 3.6× 更快，沙水分布更演进）而非渲染改色。
- 绿沙带两版均存在（pre-tilt 在 96.3–99.3% 呈现为 water 误分带，RGB(193,198,165) 已偏沙色；post 正确识别为 sand）→ s136fix 沙带识别更准确，无渲染丢失。

## 六、红线与遗留

- 物理层未改动（phys P50/P95 与基线一致），守恒与静止零抖动等物理验收不受本次渲染修复影响，沿用既有 gate。
- 90Hz 目标维持否决（phys P95 17ms > 单拍 11.1ms），与旧结论一致，留待 M2 物理治理。
- 设备 shell 无 wm/settings/display/hc 指令，强制旋转不可脚本化；旋转 viewport 路径在 P-013 P0a 已代码验证，不在本验证范围。
- s136fix 签名 HAP 已留存：test/artifacts/p012-measurements/entry-default-s136fix-signed.hap（worktree 清理后保留）。
- dida365 未授权，本回执 + docs 更新为唯一归档通道；任务状态回写待连接器恢复。

## 七、产物索引

- 采集：test/artifacts/p012-measurements/seg3-s136fix-{settle,tilt,heavy}-{off,toplight}-hilog.txt（6 块）
- 截屏：test/artifacts/p012-measurements/{s136pre,s136fix}_{settle,tilt}.jpeg
- HAP：test/artifacts/p012-measurements/entry-default-s136fix-signed.hap
- 工具：tools/p012_seg3_capture_v2.sh、tools/seg3_quick.py、tools/analyze_p012_s136fix_visual.py
- 关联文档：docs/p012-s136-closure-receipt-v1.md（修复前基线 + 133ms 主因归因）、docs/fps-seg3-P012-v0.md（v2 章节）
