# T496 · T904 真实冷档离线复核

`python test/t496/check_t904.py`。复用 T496 M1PROOT1 reader、T905 冷复制、T897 owner 和 T888 drain。新版 T904 输出六段范围，其中最后两段占位长度为零。已按 T903 写入合同核正该 reader，保其余同根、摘要和拷贝门。

| 原件 | 档数 | Owner/退役 |
| --- | ---: | --- |
| REPLAY_OFF | 3 | 52 动作，逐入队唯一终态，同 fence tombstone |
| REPLAY_ON | 3 | 52 动作，逐入队唯一终态，冷复制 fence 已成功 |
| Cold reanchor | 4 | 52 动作，逐入队唯一终态，同 fence tombstone |

10 档均核原字、摘要、同代 backing 和成功复制。三臂 WSI 发布均已释放。第 1 动作距 READY 分别 24.2693 / 30.4594 / 32.0633 ms。SOURCE 7 的接受量分别 135556 / 144576 / 133027 Q20，原脉冲间隔及 command 摘要保在机器收据。严格时域/输入状态为 CLOCK_OR_INPUT_DIFFERENCE。当前三臂包括一个 reanchor 对照，T865 四臂门仍返回 ARM_SET。继续等待完整、同实际输入/时域的四臂原件，满足后才能做严格原字比较和 T504 交接。 [机器收据](t904-d1-offline.json)
