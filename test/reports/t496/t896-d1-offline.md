# T496 · T896 新版 D1 原件独立复核

2026-11-24 · Windows PC 离线。用 T897 核 owner 终态、T888 核退役、T865 核严格门。机器收据见 [t896-d1-offline.json](t896-d1-offline.json)。

| 原件 | Owner 终态 | 退役 | T865 首拒 |
| --- | --- | --- | --- |
| OFF1 | 28 入队，26 接受、2 撤销 | 同 fence tombstone | ACTION_TERMINAL |
| OFF2 | 48 入队、48 接受 | 原业务 fence wait 成功 | CLOCK_EQUIVALENCE_UNPROVEN |
| ON | 29 入队，27 接受、2 撤销 | 同 fence tombstone | ACTION_TERMINAL |

三臂均记录 52 个动作、全部入队拍的唯一 owner 终态、三时标与会话掩码。额度闭合，WSI 发布已释放。实际 drain 使用 generation 字段，已修正 T897 适配。OFF1 END 被撤权，ON 捕获 RESERVED→REVOKED，缺热页。

OFF 首动作距 READY 为 24.1087 / 23.4631 ms。SOURCE 7 实际间隔 14.9645 / 15.1418 ms，接受量 125531 / 127018 Q20，原 command 摘要已留存。SOURCE 29 分别撤销和超龄，严格等输入未通过。

REPLAY_OFF 两次和 REPLAY_ON 一次均在 M1_QUANTUM_COLD_CAPTURE 返回 M1_COLD_ROOT_BACKING，0 动作，OWNER_TERMINATED_BEFORE_DRAIN_PROOF。T865 四臂返回 ARM_SET。由 T902 首次归因，修订后复采等输入四臂。

复现 `python test/t496/check_t896.py`。Python 编译、错代和错业务 fence 负控、T888/T897 回归通过。本轮未执行设备测试。
