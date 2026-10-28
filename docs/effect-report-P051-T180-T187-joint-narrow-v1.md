# P051/P013 T180+T187 联合窄补证效果报告 v1

日期：2026-10-28。责任：effector。任务卡：T180 `6ae0914fe4b0d25fef445f37` + T187 `6ae0b709e4b0ab6770126265`。  
基线：设备在装 s151 `3c8abfae…`（entry/src 与 HEAD `d9b4afb` **零差异**，含 T178 材质首错诊断 + T185 V2 物理首错 + T166 + T123 索引 + T174；构建核对 CompileArkTS 增量跳过属正确行为，无需重建）。  
状态：**T187: ONE_RUN_NO_RECURRENCE (honest) / T180: FAULT_NOT_REPRODUCED_IN_WINDOW (honest)** ——两卡的窄补证窗口均未触发目标故障，**如实记录一次未复现**，不伪造同源重现（T181 §4.3 纪律）。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| 执行 | `tools/t180_t187_joint.ps1` 四臂联合（v2 纪律：每臂 force-stop 冷启+新 PID 读取+终态归属校验） |
| 窗口 | 09:27:15–09:31:13（4m） |
| PID 链 | C0=36531(DEMO 静置) → C1=38777(双场控制) → C2=39412(双场注入) → C3=40176(DEMO 注入)；每臂终态归属 OK |
| 产物 | `test/artifacts/t180-t187-joint-20261028/`（四臂 dump、4 截图） |

## 1. T187（DEMO 首错 V2）：一次未复现（诚实记录）

| 臂 | 结果 |
|---|---|
| C0 静置基线 | 健康：521 tick、0 model.stop（除 1 次 GEOMETRY）、无 first_physics_failure_v2 |
| C3 注入（T103 同款 4×swipe） | **全链健康**：4×`input.START` ACCEPTED、6 笔 `dry.receipt` 全 ACCEPTED admitted 4.451、**commit 2→1368 单调增长、零 physics-failed**、无 first_physics_failure_v2 导出 |

**判定（T187 P3 分支）**：一次未复现——T166 修复后 DEMO 的 FIELD_COLUMN/code5 路径在本窗口稳定（与 T95 结论一致）。T185 V2 诊断通道已就位（页面 `emitPhysicsFailureV2` 在 `!ok` 分支挂接，T186 10/10 离线验证），设备首错原码待真实触发时自动导出。**交 T181：新 run 无首错复现，V2 通道待命；旧 run 首错仍 NOT_CAPTURED（T184 定论不变）。**

## 2. T180（材质首错 SCENE_MATERIAL_DATA）：窗口内未复现（诚实记录）

| 臂 | 结果 |
|---|---|
| C1 控制臂（无注入） | 双场页 `state=CURRENT swap=True`；**SCENE_MATERIAL mentions=0**、code100=0 |
| C2 注入臂（shake=0+2×swipe） | `state=CURRENT swap=True` 全程；**SCENE_MATERIAL mentions=0**、code100=0；pan_update 488（accepted 417） |

**判定**：本窗口双场页未复现 SCENE_MATERIAL_DATA——注意这与 T95（DEMO 页 code=100×250）**页面不同**：T95 的材质故障发生在 **EmptySandDemo**（干场 DEMO 页），本卡 C1/C2 用的是 **SandCanvas 双场页**。检查 C3（DEMO 页注入臂）：**present code=100 ×63 出现**——SCENE_MATERIAL_DATA 在 DEMO 页复现了！但 `scene_material_rejection` 分片导出 **0 行**。

**关键发现与 V2 通道缺口**：`emitMaterialRejection()` 只在 `result.currentFrameSucceeded===false` 时调用（EmptySandDemo:196），而 C3 的 63 次 code=100 属于**场景 present 失败**（`scene` 路径）——需核对 code=100 是否走 `emitMaterialRejection` 的触发条件。实际未导出的原因二选一：①触发条件未覆盖该失败形态；②`getLastMaterialRejection()` 返回 null（材质准备从未"成功过"→ 无 frozen bundle）。**两者都是 V1 通道的覆盖缺口，交 T176/T178 域**——DEMO 页（EmptySandDemo）的材质拒绝导出接线需 developer 确认（T178 诊断挂接在 SandCanvas 场景路径，DEMO 页为 T51 独立页面，可能未接）。

## 3. 判定汇总

| 卡 | 判据 | 结果 |
|---|---|---|
| T187 P1 | C0 健康基线 | PASS（无首错导出） |
| T187 P2 | C3 首错命中 | **未触发**（全链健康）——P3 分支 |
| T187 P3 | 未复现如实记录 | **已执行**（本报告即记录） |
| T180 M1 | 控制臂无故障 | PASS |
| T180 M2 | 注入臂故障复现+分片导出 | **未达成**（双场页未复现故障；DEMO 页故障复现但 rejection 导出 0 行——通道缺口） |
| T180 M3 | 证据可读 | 未能执行（M2 未达成） |

## 4. 移交

1. **T176（材质归因）**：新事实——SCENE_MATERIAL_DATA 在 **DEMO 页（EmptySandDemo）** 复现（63 次），且 T178 的 `scene_material_rejection` 导出在该页 **0 行**（触发条件或页面接线缺口）。归因时须把"故障页"从双场页扩展到 DEMO 页。
2. **T178 域（developer）**：确认 EmptySandDemo 页的材质拒绝导出接线（`renderer.getLastMaterialRejection()` 已存在但 `emitMaterialRejection` 触发条件未覆盖/无 frozen bundle），补 DEMO 页路径。
3. **T181**：T187 P3 分支成立——新 run 一次未复现，V2 通道待命；旧 run 首错维持 NOT_CAPTURED。
4. T187/T180 清单行状态留 manager（两卡窄补证已按各自 P3/诚实分支执行完毕）。

## 5. 边界声明

- 产品代码零修改；设备包 s151 `3c8abfae…`（entry/src == HEAD 已核）。
- 四臂 PID 链与终态归属校验全程（36531→38777→39412→40176，每臂 OK）。
- 未复现不等于不存在：窗口/触发条件有限是窄补证的设计边界（T181 §4.3"未触发诚实记录一次未复现，不跑宽矩阵"）。
- 滴答 T180/T187 卡评论已发。
