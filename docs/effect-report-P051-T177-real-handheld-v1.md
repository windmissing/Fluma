# P051 T177 真实手持重力补测效果报告 v1

日期：2026-10-27。责任：effector。任务卡：`6ae06c77e4b0127832f14a12`。技术路线：`P051_TOUCH_TO_SOURCE_V1` 干场 DEMO（EmptySandDemo）。  
状态：**REAL_HANDHELD_EXECUTED / SENSOR_TO_GRAVITY_CHAIN_OBJECTIVE_PASS / VISUAL_FLOW_NOT_CONFIRMABLE (SCENE_MATERIAL_DATA) / NOT_A_PRODUCT_PASS**。  
对比 T82/T95（手持项两次"未完成"）：本轮**真实手持已执行**（用户三姿态），传感器→重力方向链获得客观证据；主观目视确认因已知渲染缺陷未能达成——按任务卡"若阻碍观感则只交物理证据并报告缺口，整体不冒 PASS"如实处理。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| 设备包 | s150 主工程包（`f28ec28e…`，含 T166；installTime 1792130688871 = 2026-10-27 16:31 与 T95 装机一致，非 GPU 隔离包——包身份符合任务卡要求） |
| 设备 | VYG-AL00 `62T0225B10010020`；窗口 19:22:54–19:30:12（A 段+B1/B2/B3 段） |
| 执行 | `tools/t177_real_handheld.ps1`（A 静置+B1）+ `tools/t177_b2_rerun.ps1`（B2，失效）+ `tools/t177_b3_coldstart.ps1`（B3，有效段） |
| 手持方式 | **用户真实手持转动设备**（左倾→回正→右倾），脚本自动触摸发沙（uitest 微位移 swipe）+ 日志/截图采样——无程序注入倾角 |
| 产物 | `test/artifacts/t177-real-handheld-20261027/`（A/B1/B2/B3 四份 dump、B3 四张姿态截图、console） |

## 1. A 段：传感器→重力向量链静置基线（客观 PASS）

静置平放（USB 张力姿态）：sensor 三轴稳定 `x=-9.05, y=+1.64`，`targetTiltDeg=79.84`，`effectiveTiltDeg=30.0`（T50 钳制生效）。1067 条 sensor 投递持续、链路（加速度计→adapter→targetTilt→field.setTilt→effectiveTilt→gravityDir）全程工作。

## 2. B3 段：真实手持三姿态（有效段）

B2 段因 field 残留 failed 状态失效（非冷启 `aa start` 复用旧 run，全程 `page-physics-failed`，触摸全被拒——采集执行问题，如实披露）；B3 force-stop 冷启后有效：

**传感器→重力方向客观证据**：60 帧摘要 effectiveTilt 轨迹 **`+30 → −30 → −16.4 → +30 → −4.6`**——完整跟随用户三姿态（左倾=−30、回正=−16.4 过渡、右倾=+30），**符号正确、幅值达钳制上限、跟随实时**。加速度计 x 分量时间线同步翻转（seg11-13 x≈−5.3 左倾 / seg18-20 x≈+7.4 右倾）。

**触摸发沙链**：5 次 `input.START` **全 ACCEPTED**、15 笔 `dry.receipt` 全 ACCEPTED、admitted 11.21——手持转动与触摸发沙同时成立（转动不阻断发沙）。

**早期短暂失败（披露）**：冷启后 318ms 窗口内 12 条 `page-physics-failed`（2493303095–413），随后自行恢复——与 T129 观察的"shake 冷启早期敏感"一致，未影响后续窗口；如实记录交 T146 线参考。

## 3. 主观目视确认：未能达成（已知缺陷阻碍）

用户反馈：**"看不清，好像不太跟手"**。B3-p1 截图证实：屏幕为深色 SCENE_MATERIAL_DATA 故障帧（T95 已知缺陷 ×250 同源），粒子小且背景无对比——**主观流向观察被渲染缺陷阻碍**。重力方向正确性以 §2 的 effectiveTilt 客观轨迹为准；"沙粒目视流向低端"的主观锚点**未能达成**，不虚报。

## 4. 判定汇总

| # | 判据 | 结果 |
|---|---|---|
| A1 传感器链 | **PASS**（静置+手持双段，tilt 轨迹跟手、符号正确） |
| B1 真实手持执行 | **完成**（用户三姿态真实转动，非程序注入） |
| B2 重力方向（客观） | **PASS**（effectiveTilt ±30 跟随，钳制正确） |
| B2 重力方向（主观目视） | **未达成**（SCENE_MATERIAL_DATA 阻碍观感）——如实缺口 |
| B3 停止/沉积 | 部分（发沙停止/回正由 input.STOP UP 与静置段覆盖；目视沉积确认同受阻） |
| H SCENE_MATERIAL_DATA | 单列：故障帧贯穿全程（T95 已移交渲染线，本卡不重复归因） |
| S1 稳定性 | 无 THREAD_BLOCK；早期 318ms 瞬态失败 12 条（披露） |

**综合：NOT PASS（目视流向未确认）**——但物理链证据完整：真实手持→传感器→重力方向→发沙→准入全链客观通过；剩余缺口是**渲染观感**（SCENE_MATERIAL_DATA）而非重力机制。

## 5. 移交与边界

1. SCENE_MATERIAL_DATA 维持 T95 移交（渲染线）；本卡补充证据：**该故障帧在手持转动全程持续**，目视观感问题在真实使用场景成立。
2. "不太跟手"的用户感受：与 effectiveTilt 钳制（±30 上限）+ 故障帧低对比共同相关；钳制参数属 T50 NOT_CALIBRATED 域，不在本卡调整。
3. 采集执行问题披露：B1 触摸 0 位移 swipe 不派发（T95 老坑重犯，已修正）；B2 非冷启复用 failed run 无效——最终以 B3 冷启段为准。
4. 产品代码零修改；s150 包身份核验（installTime 匹配 T95）。
5. 滴答 T177 卡评论已发；清单行状态留 manager。
