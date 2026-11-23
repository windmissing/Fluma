# T888 · T892 原件和 T891 判据续接

2026-11-23 · Windows PC 离线 · tester。直接读取 T892 JSONL 和 M1PCOLD1，使用 T848 reader、T888 同根窗口校验及 T496/T865 原件适配。机器收据见 [resume.json](resume.json)。

- **释放与沉降。** 同代 DOWN→UP，Up 前最后一个同根 lease 采样在 Up 前 60.7862 ms，为 1 个活粒。Up 后首个样本也是 1 个活粒，前 83 个新根活粒非零，随后 36 个新根持续归零。首次归零在 Up 后 3542.104 ms。119 个 Up 后新根的沙库存均为 121460 Q20，与唯一已接受 SOURCE 一致，未见 Up 后新增 SOURCE。120 个同根 lease、零重显，收尾 release/WSI retire 成功。按首末 Present 返回 QPC 复算新根率为 23.597/s，P95/P99 为 49.325/72.026 ms。显示追踪与活动刷新模式仍缺证。
- **冷档。** 94408 B，72 B 容器头及各区边界由 T848 原读取器验证。两页连续回收 65536 + 28800 B，共 94336 B payload，完整 32 字节摘要、请求、busy root、同代作用域和退役同根通过。原 Completion 的 SOURCE 库存为 0→136607 Q20，QUANTUM 为 136607→136607 Q20。两区分别各 2 条根级事件，两个 Completion 的 bedEventCount 同为 0。逐笔 LAND 保留缺证。
- **OFF×2。** 两份旧 T887 原件继续报 `OWNER_PULSE_COVERAGE`。OFF1 缺 22–25，OFF2 缺 15–17。首动作距各自 READY 为 30857400 / 30246000 ns，SOURCE 7 真实相邻拍为 15729600 / 14961400 ns，对应接受量 131949 / 125505 Q20。已补充独立 owner 终态、同 scope/session/seq、实际 QPC、每会话队列与接受/拒绝/撤销销账判据，重锚归撤销。新增 8 项定向负控通过。四臂门仍沿严格实际时域和原字判定。

`python test/t888/resume.py` 复算原件及负控。T894 新版到件后，使用 `python test/t888/resume.py --owner <按同一运行从原行提取的JSON>` 各查一次。字段示例为脚本的 `selftest()`，该结构供离线提取，实际字段映射以新版原追踪为准。保留真实 producer 和 owner 时标、drain 逐会话覆盖原值。两臂通过后，以 `python test/t888/check.py --off <OFF1.jsonl> <OFF2.jsonl>` 复核完整动作、输入时域及退役，保留首差；新版若调整原行接口，按实际字段接续适配。T895 同根 ROI 到件后复用 `python test/t893/check.py roi <证据.json>`；T892 单个采样点不足以核层归属。实际新版 OFF 和 ROI 输入在本轮均未到件。
