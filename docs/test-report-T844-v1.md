# T844 OP7 双臂原字差分离线夹具

按 T841 固定 OP7 逻辑槽 815、push、四个 descriptor 范围及同次 permit。判定器核 LE U32 字节 SHA、字段与驱动各自 CTRL 写集、未写范围、原 first-error 优先级与 seal、逻辑计数只增一次、两臂屏障/提交/fence 和同模块双轮 OP66。读取合格单程序对照时按全缓冲区逐字比对，并可联核下一槽 OP8 原字和时间推进。缺单程序回读时仅给写集结果。撤销第二臂的用例要求候选不发布。

`python test/t844/check.py` 运行 7 种模式与 10 项负控。模式包括 REST、匀转、RAMP 短相位/结束边界、MMS 力窗内外及字段首错。用 `python test/t844/check.py --emit FIXTURE.json` 写出接口示例，`python test/t844/check.py CAPTURE.json` 读取同结构原始数组与边沿。T837 六项原判定自测通过。

当前证据为合成夹具。T833 实际双臂管线、GPU 私有差分及完整可信流场仍待真实原件。本轮未使用手机或运行 GPU 对照。[回执](../test/reports/t844/offline.json)
