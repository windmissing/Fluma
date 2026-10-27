# T151b/T177 真实手持重力补采执行案

日期：2026-10-27。责任：effector。任务卡 T177：6ae06c77e4b0127832f14a12。  
设备包：s150 主工程包（`f28ec28e…`，含 T166），installTime `1792130688871` = 2026-10-27 16:31:28 与 T95 装机一致——**在装确认**。  
状态：**分两段执行**——A 段（传感器→重力链静置验证，无需手持）立即执行；B 段（真实手持窗口）**需用户配合**，明确询问后执行或如实记录未执行。

## 0. 关键边界（任务卡原文要求）

1. **不虚报**：真实手持必须真实手持/真实转动设备；**不能用程序注入倾角冒充手持**（`applyDefinedTilt`/tiltWant 均不可用于本卡判定）。
2. **辅助支架/第二操作方式**：保障"手持/转动 + 触摸可操作"——用户手持+脚本触摸，或辅助支架倾斜+用户触摸，均算真实手持（设备真实转动是判据本体）。
3. SCENE_MATERIAL_DATA 已知缺陷单列；若阻碍观感只交物理证据。
4. 不扩大重跑 T95 宽矩阵；不改产品。

## A 段：传感器→重力向量链（静置基线，脚本可独立完成）

冷启 DEMO → 采集 `sensor` 诊断（三轴原始值）与 `tick`/60 帧摘要（`input.targetTiltDeg`/`effectiveTiltDeg`）→ 验证链路：加速度计投递 → `P051GravityInputAdapter.consumeAccelerometer` → `latest()` 产出 `targetTiltDeg` → `field.setTilt` → `effectiveTilt` → `gravityDirX/Y`（渲染用）。静置水平时 target≈90°（gx≈-9.1? 设备平放 USB 张力 79.8°→effective 30° 钳制已知）。此段证明"传感器到重力方向"管道在设备上工作，并给出静置基线数值。

## B 段：真实手持窗口（需用户配合，两方案）

方案一（推荐）：**用户手持设备**，effector 脚本只做触摸注入（uitest swipe 触摸可操作）与日志/截图；用户依次做 3 个姿态各 ~8s：左倾 ~30°→回正→右倾 ~30°。
方案二：**辅助支架**（如书本垫起）固定倾斜 + 用户/脚本触摸。

每姿态窗口记录：sensor 三轴、targetTiltDeg 变化、effectiveTiltDeg、60 帧摘要 gravityDir、粒子轨迹（截图序列）+ 用户主观观察（沙流向低端=1）。

## 判据

| # | 判据 | 通过 |
|---|---|---|
| A1 | 传感器链 | sensor 计数持续、targetTiltDeg 随姿态变化、effectiveTilt 跟随（钳制规则内） |
| B1 | 真实手持执行 | 用户确认完成 3 姿态（或如实记录未执行） |
| B2 | 重力方向正确 | 每姿态粒子流向低端（0/1 + sensor 值与轨迹时间对齐） |
| B3 | 停止/沉积 | 松手/回正后粒子沉积、源停止 |
| H | SCENE_MATERIAL_DATA | 单列披露，不混入重力判定 |

## 交付

回执 `docs/effect-report-P051-T177-real-handheld-v1.md`；产物 `test/artifacts/t177-real-handheld-20261027/`；滴答卡评论（含"已开始"已完成）。
