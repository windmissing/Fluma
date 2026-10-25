# T36 开发交付（2026-10-25）

API26 common.d.ts BaseEvent.timestamp 明确 Unit: ns，gesture.d.ts BaseGestureEvent 继承该契约。页面原先将 ns 当 ms，事件间隔被夹到1秒、每笔8unit。现显式 ns→ms 一次，复用 P011GestureRateClock，保留8unit/s与1秒长帧上限。首样本/非法值/反向时间不发射；跨手势重置；离开gate不累积时间；松手/取消/后台停止并取消未准入队列，已准入库存不召回。没有时间戳时不混用Date.now墙钟。

8/8 离线测试通过：ns/ms、1/10/43/60/120分段等时8unit、首帧、长帧、停止/重启、gate、非法/反向时间及页面边界接线。命令node test/run_p011_gesture_clock_units.cjs。工作区hvigor default@CompileArkTS通过（零错误，旧警告保留），日志.codex/local-test/t36-build.txt。未使用真机，T31负责实际速率/观感。未修改物理量纲、预算与冻结指标。

补充：丢失finger样本也暂停积分锚点；重复/倒序时间戳不倒拨已接受锚点，避免恢复时重复计时。原有用例补入该边界断言，既有1秒上限/8unit/s不变。

新增真实WaterSandField准入事务：ns/ms各按1/43/120段的一秒输入累计准入8unit，检查H+L真实总量增加8，非仅验证请求速率。

入库离线证据：test/reports/t36/clock.json；最终工作区共同编译证据见T34开发报告。
