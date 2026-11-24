# T938 · G16 活动槽原子拒绝与私有 OFF 离线判据

2026-11-24 · tester · Windows PC 离线。机器收据见 [offline.json](offline.json)。

- **G16**。四份 T932 固定原件的 BASE、种子、Completion、首盘及私有候选逐字复核。重建 B13/B52 及镜像。活动槽 word13 必须全值为 1，word14/15 和空尾为零。96 组注入夹具覆盖两个 bank、首盘/第 2 盘/末盘、flags 0/2/3、保留字及 generation/origin/material。拒绝可留下合格的私有前缀，但活动 BASE、commit、quanta、state/scene generation 与 lease 无变化。离线核新 T936 工件的 6 份摘要和双 PE 内模块钉定，复算开发交来的 19 项 host 负控收据。独立执行正常 PE 的末槽篡改预检，返回 G16_SEED_SHA 且无输出目录，未进入 Vulkan。
- **私有 OFF**。从冻结 T927 OFF1 原日志重建 T933 参考，逐字绑定 action 7、action 6/8 的相邻 callback/receipt、producer/原命令及两种 Completion。实际间隔 15026700 ns，实际接受 126053 Q20。私有 checker 核当前臂独立时钟、业务/拷贝 fence、空 BASE、SOURCE/Q 原据及有效事件范围，ON 另核观察归档与四段原字。比较保留原字摘要，仅按代码列出的运行外壳字段归一后返回首个字差。合成 OFF/OFF、OFF/ON、伪 acceptedRoot/窗口、未完成等待与首字差均已核。正式四臂等价继续由 T496。
- **交接**。运行 `python test/t938/check.py selftest` 复算。用 `fault <T936输出目录> <原stdout>` 接注入 GPU 回执，`disk <前后状态JSON>` 对照活动 bank，结构见 `disk_fixture()`。用 `private <reference.bin> <左stdout> <左目录> <右stdout> <右目录> [--observe]` 核 T937 私有原件。未采到 T936 新 GPU 注入或 T937 实际私有臂，本轮通过的是离线判据和 host 预检。
