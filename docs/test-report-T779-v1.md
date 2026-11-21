# T779 目标9三态双向动力学独立离线差分

离线验证通过。真机未执行。现有 Android ARM64 差分入口的实际 GPU ROOT/FULL 提交、冷档和物理演化在本轮均未观测。

| 范围 | 离线结果 | 实际 GPU 证据 |
| --- | --- | --- |
| DM 动态动员 | 复跑 T556 七项独立有限 oracle，T670 事件正控和 q 篡改拒绝通过 | 未观察实际动员事件 |
| S/F/P 与稳定堆积 | 规定快照覆盖 S→F、F→P、P→F、F→S，以整数逐来源核账。来源错换、拒绝后写入、重放分歧均被拒绝 | 未观察实际粒子轨迹、沉积或稳定性 |
| RX 位移拨动 | 复算 T637/T642 中 27 项手势资格和量化，命中死区边界、符号、饱和、无穷域拒绝以及 20/200ms 同位移不变性。MSVC 重编 T765 host 夹具，实际 C++ 时钟和拒绝测试、合成时序与接线静态检查通过 | 未观察拨动实际提交、FIFO 消费或场景 |
| 容量、ROOT/FULL 和重放 | 重建 DM20/22/29/31 原模板、配置和输入 SHA。覆盖异族输入、两根请求与 FULL−8MiB 输入信用。现有 DM reader 拒绝截尾 ROOT/FULL，RX reader 拒绝页错序 | 实际容量拒绝、接受态逐字节重放和冷档未观察 |

复现使用 `python test/t779/check.py`、`python test/t670/check.py --selftest`。在 MSVC x64 环境运行 `python test/t779/check.py --rx-host-output test/reports/t779/rx-host.json` 可复编并重跑 native fixture。实际冷档后续按族交给 `test/t670/check.py`、`tools/g9dm/read_cold.py` 或 `tools/g9rx/read_cold.py`。三态规定快照和位移算术均为有限用例。[离线证据](../test/reports/t779/offline.json) [RX host](../test/reports/t779/rx-host.json)
