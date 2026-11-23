# T882 A · PC 交互 SOURCE 最新相邻拍预约

Windows x64 Release 编译通过。A 单独 PE 为 `.codex/local-test/t882-a-release.exe`，SHA256 `53667e5ca95fe81b455371dd8f77d9a1939dc21d1b61b2c2428ea5821d5a2798`。

Win32 将当前 hold 的相邻真实 callback／native receipt 配成区间。待认领只有一项，较新区间替换待项并给旧项唯一 `Superseded` 终态。已经认领的项保留原字至接受或撤销。合法边界在 `activeRemaining==0` 且 owner 可用时重核 scope、session、真实端点年龄与消费水位。SOURCE 的量仍按原速率乘本次区间长度，跨拍跳过的量不累加。D1 显式动作保原四槽。

`--m1-source-trace` 的 `M1_RESERVATION_RETURN/TERMINAL` 携带真实两拍的 QPC、session、Δ 与终态。原根收据补 `m1SourceBoundary`、`m1ReservationPulses`、原 SOURCE command 与 Completion。离线 C++ 自测通过，覆盖边界前连拍替换、已认领不可变、过期、旧 baseline、真实缺拍和撤销唯一终态。生产 120 Hz、四份预算、33,333,333 ns freshness、250 ms 期限及 shader 路由沿原合同。
