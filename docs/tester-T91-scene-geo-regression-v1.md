# T91：新 scene 几何独立回归 v1（SCENE-GEO-01..05）

日期：2026-10-26。角色：tester。任务卡：T91。合同：`docs/architecture-T89-scene-regression-routing-v1.md` §3–5（`P013_SCENE_GEOMETRY_REGRESSION_V1`，manager 已裁决 T89：旧入口字面断言不适用于授权 scene/独立输入迁移）。执行器：`test/p091_scene_geo_regression.cjs`（真实 `P013SceneController`+`P013ScenePreparation`+native 桩；不镜像源码字符串断言；不拿 T56 开发 5/5 作唯一证据）。raw：`test/reports/t91/p091_scene_geo_results.json`。**18/18 PASS**。身份：T56 114a7ee（当前树在其上仅追加 T78/T82 报告与 T85 trace/T88 执行器，scene 源未动）。

## 1. SCENE-GEO-01 冷启动（3 PASS）

- A：真实 prepare 回 800×480（`sizeSource=EGL_QUERY_SURFACE`），全框 packet 使用**真实整数 px**（surfaceWidthPx/HeightPx=800/480，boxWidthPx/HeightPx=800/480），frameToken 携带同一 commit——首次 surface 尺寸不依赖 caller 猜测；
- B：prepared 尺寸 0 → null receipt、`PREPARE_…` 身份，无 scene draw 调用；
- C：NaN/非正 scale → `SCENE_SCALE` 拒绝。

## 2. SCENE-GEO-02 生命周期竞争（4 PASS）

- A：prepare 失败 → null receipt，**无任何 scene draw 调用**（调用序仅 init/prepare）；
- B：prepare 后 surface epoch 竞争 → `STALE_PREPARED_SURFACE`，无 draw（ getState 复核拦截）；
- C：draw 层失败（FAULT/firstFailureStage=SCENE_MATERIAL）→ receipt 原样保留，controller.failure=firstFailureStage，**无假 CURRENT**，lastGood 保留旧 token；
- D：STALE + 兼容 LastGood → `staleSwapSucceeded=true` 与 `requestedSwapSucceeded=false` 两类 swap 身份可区分（T68 身份记账口径落点）。

## 3. SCENE-GEO-03 Float32 保护（3 PASS）

- A：T61 右列临界合法源（127.99999618530273、127.99999999）经**真实 scene 链**：授权的 `COLUMN_PRESERVING_F32_RENDER_V1` 分支**接受并投影**到域内前一 Float32，权威源字节不变（T61 证明 RAW 修订拒绝它们；T89 已授权 scene 走投影分支——预检只拒 Float64 域外源）。执行器首版期望沿用 T61 拒绝语义，属**期望模型失配**，按 T89 授权语义修正（非产品/判据变更）。
- B：非法源（NaN x / x=128.5 越域 / 负 y / 零质量 / 非法 state / 非法 id）逐一 `INVALID_PARTICLE`，slot 与 publication 不变；
- C：恢复合法源后同链 pack OK（对照）。

## 4. SCENE-GEO-04 同框与密度（4 PASS）

- A：非整数 DPI（scale=3.3）全框 packet 用真实 prepared px（640×1136），**不乘小数 scale**；
- B：偏移框 box px = box·scale 精确（16×3.3=52.8、32×3.3=105.6）——vp→px 唯一转换点；
- C：128 列密度恒等（列数固定，px/列随 DPI 缩放；逆映射权威性由 B 与 T61 共同覆盖）；
- D：publication 身份：snapshotId/physicsCommitId/frameToken 同 commit。

## 5. SCENE-GEO-05 完整 scene 失败身份（3 PASS）

- A：全场失败且无旧好帧 → presentationState=FAULT、lastGoodFrameToken=null、两类 swap 均假——**无局部床/粒子帧冒充全场**；
- B：sourceFailure 经 receipt 上传且物理 publication 不被污染；
- C：receipt 暴露 requested/stale/lastGood/generation 全部身份字段（T68 记账）。

## 6. 历史 T62 断言版本化

原 `run_p062_render_repair_units.cjs` 第六项 `submitDualFieldSurfacePx` 字符串 FAIL **原样保留**，标 `SUPERSEDED_BY_AUTHORIZED_SCENE_MIGRATION`（manager 裁决，T89 §4）；未原地改期望造绿；原五项与 RAW/非法保护阈值不动。

## 7. 边界

host 桩层证据（native bridge 真实签收在 T81/真机卡）；无 Canvas（T56 单元另有断言）；未改产品/判据/协议；未用真机；无 NOT_IMPLEMENTED 项（全部授权功能在 114a7ee 已实现且走通）。真实失败 0 项 → 无 architect 分流项。
