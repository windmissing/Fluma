# T458 · 单沙帧观察生产路径独立差分

2026-11-10，tester，P4。不使用真机、不改产品。基于 T455 实际生成页 `PrepareFrameObservation.ets`（T455-release-final 同源类）与 T449 合同，独立驱动注册/onFrame/didLayout/willDraw 语义（typescript 转译实际类，不经开发 65 项判定路径）。复用 T432 oracle 许可语义结论（65 agree / 0 diverge，未重跑）。真机由 T408 独立执行，不作其前置。

## 交付

| 文件 | 内容 |
|---|---|
| `test/t458/frame_obs_diff.py` | 独立差分器：node 驱动实际类跑 8 场景 + Python 侧独立判定 12 项 |
| `test/reports/t458/t458-frame-obs-diff.json` | 逐项判定 |

## 结果：verdict PASS，12/12

| 场景（T449 §5） | 断言 | 结果 |
|---|---|---|
| 从未 Idle 可达稳定 | S1 无任何 Idle 调用：SAMPLE=2、STALE_DROP=0、两帧点不同 | **PASS** |
| 未交付 onFrame | S2 无 frame 直接 draw → NO_FRAME_TICKET 丢弃 | **PASS** |
| 同帧/旧帧拒绝 | S3 同 ns frame 先收 DRAW_NOT_DELIVERED 再 FRAME_NOT_INCREASING，共 3 drop；合法递增帧仍可进入 | **PASS** |
| 旧回调不能占新槽 | S4 旧实例 draw 不影响新实例 ticket（DRAW_ENTER=1，无 drop） | **PASS** |
| 同帧重复 draw | S5 第二笔 draw NO_FRAME_TICKET（DROP=1） | **PASS** |
| 晚布局漂移 | S6 layoutChanged 后旧 anchor 拒（NO_CURRENT_LAYOUT），新 anchor 过，layoutGeneration==2 | **PASS** |
| 超时分类优先序 | S7 空观察→WAIT_POST；S2 post 无 frame→WAIT_FRAME_ENTER；S8 空 anchor→WAIT_FRAME_ENTER（frame 未发生优先于 layout，独立判定与 timeoutReason 序一致） | **PASS** ×3 |
| 2 帧稳定 | S1 两笔真实快照在不同帧、同布局、零 drop | **PASS** |

两处初版预期偏差（S3 drop 计数、S8 超时分类）经核对合同文本确认为**差分器预期错**（未计 DRAW_NOT_DELIVERED 叠加、未按 timeoutReason 优先序），实际行为符合 T449，已修正预期后全过。**无产品缺陷。**

## 边界

- 预约零副作用：T432 oracle O6/O8 项已证（REJECTED 且 phase/request/hold 零变更），本卡复用不重跑。
- 真机（VSync 真实稀疏、onFrame 实际时序）归 T408；本卡 device=NOT_RUN。

复跑：`python -X utf8 test/t458/frame_obs_diff.py`。
