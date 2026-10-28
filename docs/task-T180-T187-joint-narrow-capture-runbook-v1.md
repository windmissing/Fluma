# T180+T187 联合窄补证执行案

日期：2026-10-28。责任：effector。任务卡：T180 `6ae0914fe4b0d25fef445f37` + T187 `6ae0b709e4b0ab6770126265`。  
基线：主工程 HEAD `d9b4afb`（含 T178 材质首错诊断 + T185 V2 物理首错 + T166 + T123 索引 + T174），**需重建 s152**（当前设备 s151 为 T156 基线，无 T178/T185 诊断）。  
 discipline：T184 v2 采集器（冻结旧 PID/run→force-stop→冷启读新身份→PID+run 双过滤→cmd-audit）。

## 联合逻辑

- **T180（材质首错）**：控制臂（无注入静置，预期 SCENE_MATERIAL_DATA 不出现）→ 注入臂（T95 同款 swipe，预期故障帧出现）→ 捕获 `WS_EVENT scene_material_rejection` 分片（原 bits/mask/writerStage/同 frameToken）→ 回 T176。
- **T187（DEMO 首错 V2）**：同一注入臂内，若 `field.step` 失败 → `WS_EVENT first_physics_failure_v2`（原码/生命周期 8 槽，整包单行）自动导出 → 回 T181；若未触发，如实记录一次未复现（不跑宽矩阵）。
- 两卡共用冷启与注入臂，各自判定独立出报告。

## 臂矩阵

| 臂 | 页面 | 操作 | 预期 |
|---|---|---|---|
| C0 | DEMO（T187 域） | 冷启静置 15s | 无首错（健康基线） |
| C1 | SandCanvas 双场（T180 域） | 冷启静置 15s（无注入） | 无 SCENE_MATERIAL_DATA（控制臂） |
| C2 | SandCanvas 双场 | shake=0 + 2×swipe 注入 + 15s | **SCENE_MATERIAL_DATA 出现**→ scene_material_rejection 分片 |
| C3 | DEMO（T187 域） | 冷启 + 4×swipe（T103 同款）+ 15s | 若 step 失败→first_physics_failure_v2；未复现如实记录 |

每臂 PID 隔离、cmd-audit、终态归属校验。

## 判据

| # | T180 | 通过 |
|---|---|---|
| M1 | 控制臂 C1 无材质故障 | code 全 0 |
| M2 | 注入臂 C2 故障复现 | code=100 出现且 scene_material_rejection 分片导出 complete=true |
| M3 | 拒绝证据可读 | SOURCE/VALUE/LAYOUT mask、writerStage、frameToken 与 present code=100 帧一致 |
| # | T187 | 通过 |
| P1 | C0 健康基线 | 无 first_physics_failure_v2 |
| P2 | C3 首错命中 | first_physics_failure_v2 单行导出，rawCode 非 null（或 NOT_CAPTURED 明示） |
| P3 | 未复现记录 | 若 C3 无首错：如实"一次未复现"交 T181 |
