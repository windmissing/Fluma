# T888 · PC 长窗与 D1 原始证据复核

2026-11-23 · tester · Windows PC 离线。运行 `python test/t888/check.py --selftest`。编译及五项 B 负控通过，含用历史真实 SOURCE 字和合成 B 专有时标/退役字段构成的 52 动作完整适配检查。机器收据为 [offline.json](offline.json)。

## A · T884 原长窗

| 运行 | 原队列新根 / 重显 | 新根率 / P95 / P99 / 最大间隔 | Up 后去重根 / 库存 Q20 |
| --- | --- | --- | --- |
| longwindow | 721 / 0 | 25.045/s / 43.740 / 45.656 / 105.122 ms | 230 / 139681 |
| dual-release | 291 / 0 | 24.660/s / 42.874 / 57.428 / 114.003 ms | 153 / 128878 |
| dual-lease | 3 / 0 | 早停，仅保观察样本 | 0 / 未观察 |
| empty-idle | 0 | 无窗口呈现样本 | 未观察 |

每个队列帧核原 Completion、同根 scope/commit/generation/slot、fence 和 Present。逐根总库存与来源账一致。dual-release 的 291 份同根 lease 另逐条核活粒子数和库存。两个长窗各一笔接受 SOURCE，Up 后无新增源提交，库存恒定。首次观测到活粒子归零分别在 Up 前约 15.98 s 和 2.12 s。该证据支持停源和持久留存。逐笔 LAND、上屏沙床/粒子图层各自像素、真实显示速率均缺输入。现有单色采样及队列返回不足以通过 T883 图层/事件门或 T850 显示门。上表采用事后观察窗的首末 Present 返回 QPC、相邻间隔与最近秩百分位，刷新率待实际活动模式。历史 OFFER 多引号只在内存按既有窄规则解析，原文件未改。

## B · T882 / T887

本机 T882 B PE 的 Windows x64 格式、大小 3748352 B 和实际 SHA-256 与交付一致，离线 `t882-d1.exe` 自测通过。适配器严格读取原 JSONL，关联 52 动作与 owner 脉冲、SOURCE 终态和实际时标，核 END_INTAKE、独立 fence/WSI、配额销账及 CLOSE。先通过每臂退役，再核 OFF×2 首个实际时域差异。四臂原字比较沿 T865 的独立前置。

实际复验 `python test/t888/check.py --off test/reports/t887/b-off1.jsonl test/reports/t887/b-off2.jsonl`。两臂均为严格原 JSON，52 动作及 intake/drain/close 齐备。独立匹配原根、fence 对象、退役原码与 tombstone Poll，确认 fence 已在 drain 额外探查前退役。两臂销账闭合。

严格门返回 `OWNER_PULSE_COVERAGE`。OFF1 已投递动作 22–25 缺 owner 终态，OFF2 已投递动作 15–17 缺 owner 终态。SOURCE 7 实际相邻拍 15,729,600 / 14,961,400 ns，实际接受量 131949 / 125505 Q20，原 command SHA 各自留存。SOURCE 29 两臂均以 STALE_ENDPOINT 过期。详见 [OFF×2 收据](off-pair.json)。未放行四臂原字等价。接手由 architect 核 owner 缺项与时域差异，随后补取完整 owner 终态及可比输入。
