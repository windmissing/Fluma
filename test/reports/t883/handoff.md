# T883 · PC 物理与事务独立验收准备

2026-11-23 · tester · Windows PC 离线。Python 编译、17 项负控、T836/T838/T850/T852/T865 回归通过。以下正例为离线夹具，实跑证据单列。

| 组 | 已交付与核证 | 真实接力 |
| --- | --- | --- |
| A · T880 | 复读 T876 run5 原 JSONL。FIFO 四槽 96–99 的年龄下界为 85.362/68.886/53.941/38.116 ms，100–101 队满。参考模型覆盖相邻 producer 拍与独立 native 拍、最新单 waiter、四个已接受 QUANTUM 后的合法边界、单终态、旧 hold/缺拍/失焦/设备失效/晚 fence、配额与源量守恒。 | T882 新版后采同场景双源、原 SOURCE 原字和接受/拒绝终态。 |
| B · T881 | pending 与已 signal 两类 drain，分验同 fence、tombstone、owner、WSI、请求与信用归还。先比 OFF×2，按动作保实际时域首差。复核旧 T875 四臂，三臂原 drain=false，四臂真实源间隔不同。 | T882 后先采两次 OFF，等时域和退役均闭合才用 T865 严格比四臂。 |
| C · 目标 1/3/4/5 | 133 根合成长窗验证 SOURCE 零步、逐根 S/P 与 LAND 守恒、停源后仅累计落床和同根同 lease 的沙床/粒子像素。补空箱、换代与四个方向的重力输入序列。T830 原件仅登记一条诊断呈现；缺库存/事件或同根像素明示缺证。 | T884 或对应 PC 工件提供长窗、真实库存与像素、空箱和原接受重力字段。 |
| D · T845/T838 | 按 DM2/R1/RX 的 family/schema/backend、容器与尾头、SHA、原封存接受区间选择三族 reader。三路由夹具及错族、坏 SHA、拒绝尾负控通过。历史模式调原 reader。Vulkan PC 暂报 MISSING_PC_ROLE_EVIDENCE，等待真实 PE/角色和绑定证据，之后再做三态解释与同根窗口。 | 目标 9 实际角色工件和同源三态原件。目标 10 维持暂停。 |

`python -m py_compile test/t883/check.py test/t883/physics.py test/t883/transactions.py`

`python test/t883/check.py selftest` 运行全套夹具及原件回归。 `history` 单独复算 T876/T875。 `emit` 输出从 T836 扩展的空箱和重力输入。其余 CLI 模式为 `input <json>`、`drain <json>`、`off-pair <json>`、`physics <json>`、`gravity <json>` 和 `archive <原档> --envelope <声明.json>`。采证字段见 `check.py` 内各 fixture。SOURCE/捕获/窗口使用原 QPC 及根与 lease。像素探针应取独立选区与预登记的参考 RGBA，实际证据随原采集文件保留。
