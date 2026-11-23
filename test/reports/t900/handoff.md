# T900 · PC 长按、停源落床与期限恢复独立判定

2026-11-23 · tester · Windows PC 离线。机器收据见 [offline.json](offline.json)。

| 原件 | 已核实 | 待采证 |
| --- | --- | --- |
| T898 现场快照 | SHA 与原登记一致。hold 83 约 630 ms 内接受三笔 SOURCE，分别 129602、127272、131757 Q20，合计 388631 Q20。原相邻拍、命令及生产者摘要、根与 commit 对齐。八个 QUANTUM 在 Up 后、下一 Down 前启动，活粒仍为 5，库存和 sourceCommit 固定。 | 缺逐笔预约终态，八根无 LAND 原字。 |
| T898 期限退役 | 独立读得 43 个唯一 ROOT_DEADLINE 根记录了原 fence 成功退役、无待定 fence 及额度平衡。 | 新 baseline 后无 SOURCE 的首张 CURRENT 帧未形成证据链。 |
| T892/T895 | T892 Up 后 119 个新根库存恒定，约 3.542 秒活粒归零。94408 B 冷档通过 T848，NEXT_FRAME 成立。 | 两个冷档 Completion 的 bedEventCount 都是 0，逐笔 LAND 缺证。T895 床和粒子 ROI 重叠，无法核独立图层。 |

从仓库根运行 `python test/t900/check.py`。带 `--history .codex/local-test/demo-recovery-20261123-202032-832.jsonl` 同时复算现场原件，并执行以原记录为底的合成预约终态正负控。Python 编译、17 项负控以及 T836/T848/T888/T893 回归通过。新增夹具按 T898 的仍按住、已松手两条路径分别走通过例，保实际退役、额度清零、新会话和真实新拍。

T899 实物到件后分别运行以下离线判定。

- `python test/t900/check.py --hold <原JSONL> <hold编号>`，按预登记的静止长按和移动长按各记录一个 Down，核至少三笔有逐 offer 终态的非零 SOURCE，保同一 Windows PC、PE、三模块、配置及初态。墙时与进展率另登记。
- `python test/t900/check.py --land <原件提取JSON>`，依脚本 `selftest()` 的结构保 Up 边沿、原输入与 SOURCE 接受序列、前后粒床库存及实际 CURRENT 帧。使用有效事件段原字和摘要核 P→S 与来源守恒。
- `python test/t900/check.py --recovery <原件提取JSON>`，分别保仍按住、已松手故障样本。使用原拒因、同 fence 退役、tombstone/Idle、额度、新真实 baseline 及首张接受 QUANTUM 原 Completion 和同 lease 呈现。上述提取结构用于离线接缝，实际字段映射以 T899 原件为准。

本轮未运行 PC 产品或使用手机真机。T899 新实物及其输入映射待交接。
