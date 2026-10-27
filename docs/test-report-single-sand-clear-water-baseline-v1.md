# T108 单沙明水测试先行基线报告（RED）v1

日期：2026-10-27。任务：6adf343de4b06ee35743c76b（tester，离线，不使用真机）。消费：T122 e1d0ec7 交接（`algorithm-T122-single-sand-clear-water-handoff-v1.md` §4）→ 原算法 §8 CW-01..18。被测：当前树（目标7 零实现）。

## 结论：RED 基线登记完成——19 NOT_IMPLEMENTED / 2 PASS / 0 FAIL

执行器 `test/p108_clear_water_red.cjs`；原始 JSON `test/reports/t108/red_baseline.json`；用例条目登记 `docs/test-cases.md` §18。

## 符号存在性核查（实现面全缺，8/8）

| 依赖符号 | 状态 |
|---|---|
| SAND_WATER_SOURCE_V1（注入 API 版本） | 缺 |
| CLEAR_WATER_SCENE_V1（同帧渲染版本） | 缺 |
| CLEAR_WATER_OPTICS_V1（透射光学） | 缺 |
| 显式 W 权威库存数组（waterStock/realWater/explicitWater） | 缺 |
| SINGLE_SAND_CLEAR_WATER 独立 mode 常量 | 缺 |
| SourceRequest/WaterSourceReceipt typed 接口 | 缺 |
| injectWater( 掉水双源入口 | 缺 |
| FrameToken 同帧透射字段 | 缺 |

## CW-01..18 逐组判定（全部 NOT_IMPLEMENTED，预测试 RED）

| ID | 覆盖 | 状态 | 依据 |
|---|---|---|---|
| CW-01 | 新 run 全空 ΣW=ΣS=ΣP=0、L/U=0、无水 mask | NOT_IMPLEMENTED | mode/API 缺 |
| CW-02 | 干列连续加水只改该列 W、I_water 独立审计 | NOT_IMPLEMENTED | 注入 API+W 库存缺 |
| CW-03 | 湿/满/P 预留列局部准入或拒绝、对账、无负 W | NOT_IMPLEMENTED | W 库存缺 |
| CW-04 | 掉沙出生→空气→入水→LAND 同 ID、U=0、L=0、H 守恒 | NOT_IMPLEMENTED | 掉水入口缺 |
| CW-05 | 水/空气对照：跨面时间/湿时长/速度位移解析比对 | NOT_IMPLEMENTED | 同上 |
| CW-06 | LAND q 后 W 不变、P 减 S 增、η 抬升、身份正确 | NOT_IMPLEMENTED | 同上 |
| CW-07 | 纯水通量 q 与 5 项上界、成对审计、平头零流 | NOT_IMPLEMENTED | W 库存缺 |
| CW-08 | 干高床隔水/墙：无瞬移、无外损、head 残差 | NOT_IMPLEMENTED | W 库存缺 |
| CW-09 | 倾角 head 趋同、accepted θ 同入物理与 shader | NOT_IMPLEMENTED | W 库存缺 |
| CW-10 | 满接收列容量拒绝仍守恒、无伪成功 | NOT_IMPLEMENTED | W 库存缺 |
| CW-11 | 注入失败族库存/身份/成功账不变 | NOT_IMPLEMENTED | 注入 API 缺 |
| CW-12 | 幂等重试一次、序号独立、不串账 | NOT_IMPLEMENTED | 注入 API 缺 |
| CW-13 | 按住切换/次指针/CANCEL 生命周期 | NOT_IMPLEMENTED | 注入 API 缺 |
| CW-14 | 后台/reset/几何代失效、无补发 | NOT_IMPLEMENTED | 注入 API 缺 |
| CW-15 | step 审计失败与 source 事务失败分别注错 | NOT_IMPLEMENTED | API+W 缺 |
| CW-16 | 水上原色/水中 T 混合/水下床可见/同 FrameToken | NOT_IMPLEMENTED | 渲染版本缺 |
| CW-17 | shader/swap 失败 FAULT 或完整 STALE | NOT_IMPLEMENTED | 渲染版本缺 |
| CW-18 | 长时定容运行/复杂度实测 | NOT_IMPLEMENTED | API 缺 |

每组依赖映射与判定自动化（`deps` 表），实现到件后逐组从 NOT_IMPLEMENTED 转 RUN。

## 可先行基线（对照用，非明水通过）

- 旧 WET_COLUMN_V1 注入/审计链活着：注入 .3 → audit(true)=null（PASS，仅作实现后对照；CW-02 明示旧成功不推明水通过）。
- 独立观察：dry 演示（无水面）注入按 NO_WATER 拒绝——现有 wet 准入语义本身将作为目标7对照起点。

## 边界

- 新参数 κ/rateWater/rateSand/一次上限/dragWet/settlingSpeed/sigmaWater/tint/高光/分段误差全部 **PENDING_CALIBRATION**：实现后测试仅输出测量值与候选 configRev，不贴既有门槛。
- 算法 §9 十二反例清单随实现阶段保留核验（清单登记 PASS）。
- 效果观察六组（透水辨床/入水减速/真实抬面/倾角方向/切源停止/多水深对比）归 effector 后续派工，本卡不执行。
- 未实现 RED 归 developer/工程侧后续实现；本卡不虚报产品 FAIL 缺陷，失败先 architect。

## 交接状态

T107（architect 工程方案）未开工；T108 基线在 API 到件后扩断言。目标7 实现与产品指标待开发/独立双测。
