# T189 单沙明水 A 段独立验证报告

日期：2026-10-28。任务：6ae0b709e4b0d25fef4adbd5（tester，离线，目标7/P3）。消费：T188 0cc0439（`developer-T188-clear-water-source-v1.md` + `test/reports/t188/`），按 T107 §A 与 T108 CW-01/02/03/11/12/15。

## 执行器与独立点

`test/p189_clear_water_independent.cjs`（15 用例）。独立 oracle：本执行器自持独立 Float64 账本（W/Iw/Is/序号），全部守恒与容量判断不调用被测 `audit/rebuild/waterTotal`；CW-11 拒绝族 11 项独立构造；冲突语义从 dedup key 组成（slots 0,3,4,5,6,7）与 payload 位比较（含 -0）独立推导，不镜像 T188 自测。

## 结果：15/15 PASS（原始 JSON：test/reports/t189/independent.json）

| # | 场景 | 结果 | 关键证据 |
|---|---|---|---|
| 1 | CW-01 空运行 | PASS | ΣW=ΣS=ΣP=0、I_water/I_sand=0、medium=SINGLE_SAND_CLEAR_WATER_V1、audit OK |
| 2 | CW-02 干列连续加水 | PASS | 16 列各 .5：只写该列 W、独立 I_water 账逐笔一致、initialWater 保持 0（运行不重置基准）、audit OK |
| 3 | CW-03 部分/满准入 | PASS | 255.5 → .5 partial（admitted=.5/rejected=1.5）→ CAPACITY_FULL；无均摊（W[63]=0）、无负 W |
| 4 | CW-11 非法族 11 项 | PASS | x 越界/NaN、y 负、量 0/负/NaN、timestamp 负、旧 run、旧 geometry、旧 input、时钟窗错——各独立 status，库存指纹逐位不变 |
| 5 | CW-12 幂等+双 kind | PASS | sand/water 同 requestSeq 各自 kindSuccessSeq=1；replay=true 不重复记账；W=.5 与 P=.25 不串账 |
| 6 | CW-12b 冲突语义 | PASS | 同 key 不同 payload（y 差异 / x=-0 位差）→ IDENTITY_CONFLICT；不同 key（gestureId）→ 新请求正常提交；库存变化仅在合法新请求 |
| 7 | CW-15 source 成功后 step 失败 | PASS | step 返回 UNSUPPORTED_FEATURE/NOT_IMPLEMENTED_B_COUPLING、failedAttempt 记录、lastAcceptedCommit=成功事务 commit、库存指纹不变（成功不回滚） |
| 8 | dt=0 | PASS | 队列不消耗、库存不变 |
| 9 | ULP | PASS | W=128 加 MIN_VALUE → NUMERIC_UNREPRESENTABLE（位型未变拒绝），无假成功 |
| 10 | snapshot 隔离 | PASS | 调用者 fill(999)/liveCount=12345 不影响运行库存指纹 |
| 11 | queue 容量 | PASS | queueCapacity=2：第 3 笔 QUEUE_FULL、无覆写、指纹不变 |
| 12 | P 预留 | PASS | 单粒 256 占满 → water PARTICLE_RESERVED_CAPACITY 显式拒绝 |
| 13 | 单沙身份链 | PASS | .3125 → 2 整包 + 真尾包 .0625，pId 1/2/3 递增，phase0/external-source 身份 |
| 14 | B/C/restore 分列 | PASS | step → UNSUPPORTED_FEATURE/NOT_IMPLEMENTED_B_COUPLING；无 restoreSnapshot 入口；medium 非 WET_COLUMN_V1 |
| 15 | 旧 wet 不冒明水 | PASS | WaterSandField 无 SINGLE_SAND_CLEAR_WATER_V1 共享 mode 常量（模块独立） |

## NOT_IMPLEMENTED 分列（按卡要求，不冒 FAIL 也不冒 PASS）

- **B 段**：hydraulics/分段空气湿粒子/完整 step budget+guard+事件 lineage——`advanceSubstepInto` 显式 UNSUPPORTED_FEATURE + implementation='NOT_IMPLEMENTED_B_COUPLING'。
- **C 段**：publication/渲染/输入控制器/页面——无页面启用入口。
- **snapshot restore**：无 restore 入口；sourceBefore/work scratch 不作 authority。

## 基线

T188 开发自测 26/26 复跑 PASS。

## 语义记录

- dedup key 组成 = runGeneration/inputGeneration/geometryEpoch/gestureId/sourceId/requestSeq/kind（slots 0,3,4,5,6,7）；payload 位比较覆盖 x/y/volume/timestamp/unit/seed 等并含 -0 位型——**-0 与 0 是不同 key 冲突**。
- 幂等重放发生在 enqueue（dedup 命中先于当前代检查），停止手势不撤销旧成功回执。

## 边界

- A 段通过不等于单沙明水整体完成（B/C 未实现）；设备归后续真机卡；失败上报：无。无阻断。
