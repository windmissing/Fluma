# T852 · PC 沙水源账与窗口合成独立夹具

2026-11-23 · tester · Windows PC 离线。按 [T832](../../../docs/architecture-T832-v1.md) 和 [T826](../t826/t826-case-migration-ledger.json)，覆盖双源切换、同帧双态、侧扩散、守恒和 ROOT/呈现身份。采证档为水平 K192/Kw128、128 列、Q20，四组均从空 run 开始。

## 复现与结果

- `python -m py_compile test/t852/check.py test/t852/selftest.py` 通过。
- `python test/t852/selftest.py --emit <夹具.json>` 生成正反源序、κ=24/0 的四组记录，另验证 15 项负控。 `python test/t852/check.py <夹具.json>` 独立读取并通过。正 κ 两臂均累计左 41943、右 62913 Q20。零 κ 无边流。四臂最终沙 131072、水 262144 Q20。停源后 LAND/ARRIVE 不增加源账，ARRIVE 的新 W 下一步才参加边流。
- `python test/t852/selftest.py --replay .codex/local-test/t280/output-192-128.bin --pixels .codex/local-test/t281` 验证本机留存历史原件。复用 T279/T280 oracle 重核 38 条 K192/Kw128 完整重放。T281 五项像素原件 SHA 与既有收据匹配，独立 check 覆盖 W0、干槽、深水、飞包与已到达飞包消失。深水 RGBA [46,81,95,255] 与公式一致，沿用 2 LSB 界。

负控覆盖越界、旧几何许可、重复记账、晚 fence、拒绝 owner、拒绝根改 commit、缺边事件、错误库存、总账误差、沙水根不一致、像素错交换链、占用 lease、STALE、Canvas 路由和已到达飞包重显。源账使用被接受量，记录撤权、床内拒绝与重放零新增。

## 实际 PC 证据接入

`check.py` 消费四个已标准化的 run，字段示例为 `selftest.py` 的 fixture。接入方保留原 Win32 输入与时钟、command32、实际 owner 和 GPU completion、逐列 S/P/W/J/η、边事件及 LAND/ARRIVE、lease、窗口像素和原 Vulkan 结果。用实际 adapter/device/window/surface/swapchain/geometry/config/optics、run/nonce/commit/sourceCommit/slot/generation、imageIndex/presentId 分别标记沙、水、像素的取证身份。只在对应飞包像素探针上标 `packetProbe`。正式记录需随原件保存，`check.py` 筛查标准化数据的一致性。

本次已验证合成夹具和历史软件后端原件。Windows Vulkan FullM1 两相、真实业务窗口和实际像素仍待产品接入，沿 T832 取新证。
