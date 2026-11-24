# T496 · T937 私有 OFF/ON 原件独立复核

2026-11-24 · Windows PC 离线。运行 `python test/t496/check_t937.py`，机器收据见 [t937-private-offline.json](t937-private-offline.json)。

- 冻结参考从 T927 原日志独立复读。三臂的 reference.bin 逐字相同，真实相邻采样间隔 15.0267 ms，固定 SOURCE 请求映射后每臂实收 126053 Q20。验证了三个独立 run、当前授权时标、SOURCE 与 QUANTUM 成功 fence、空 BASE/证据回读 fence 及原字摘要。等待期间到达的当前拍可能晚于 wait enter，按实际 wait exit 核时间。独立 spare 比正式候选早于 seal 最终元数据写入，四个实际差字均符合生成布局的 carry、发布代和 bank 收尾。
- ON 成功臂的 M1PROOT1 与四段原字逐字相同。原业务 fence 与独立冷复制的 nonce、代、等待、映射和 invalidate 均关联成功。所有完成臂保持 null acceptedRoot，无窗口发布。ON 首次运行在 M1_PRIVATE_REAL_PULSE 停止，成功重试的 argv 收据缺 OS exit 字段，原终态明确。
- 身份与当前命令时标归一后，OFF/OFF 和 OFF/ON 的首差都在 quantum-candidate word 7215，分别为 480228000→489240000 和 480228000→713508000。对应 Completion word 108 是每对唯一的另一处差字。两处均为同一 timeCarry 字段，其他物理载荷无首差。严格私有原字仍返回 WORD_DIFF。
- Python 编译、真实原件复算、错冷复制 fence 和伪发布负控，以及 T938/T904 回归通过。正式四臂等真实同输入、同实际时域及完整 owner/退役证据。T504 仍待业务正控。
