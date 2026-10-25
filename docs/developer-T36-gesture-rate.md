# T36 开发交付（2026-10-25）

API26 common.d.ts BaseEvent.timestamp 明确 Unit: ns，gesture.d.ts BaseGestureEvent 继承该契约。页面原先将 ns 当 ms，事件间隔被夹到1秒、每笔8unit。现显式 ns→ms 一次，复用 P011GestureRateClock，保留8unit/s与1秒长帧上限。首样本/非法值/反向时间不发射；跨手势重置；离开gate不累积时间；松手/取消/后台停止并取消未准入队列，已准入库存不召回。没有时间戳时不混用Date.now墙钟。

7/7 离线测试通过：ns/ms、1/10/43/60/120分段等时8unit、首帧、长帧、停止/重启、gate、非法/反向时间及页面边界接线。命令node test/run_p011_gesture_clock_units.cjs。工作区hvigor default@CompileArkTS通过（零错误，旧警告保留），日志.codex/local-test/t36-build.txt。未使用真机，T31负责实际速率/观感。未修改物理量纲、预算与冻结指标。
