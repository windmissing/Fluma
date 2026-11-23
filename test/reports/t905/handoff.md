# T905 · PC 冷档与样机退役独立离线判据

2026-11-24 · tester · Windows PC 离线。实际 GPU 业务验证由 T904 接续。未建滴答卡，本文件为本地交付回执。机器收据见 [offline.json](offline.json)。

- 冷档沿用 T496 的 M1PROOT1 原字、SHA 和 backing 读取。补查接受根业务 fence、独立复制 fence、提交/等待/读回原返回、代和缓冲区范围、归档唯一性、逐根覆盖与 END_INTAKE→DRAIN→CLOSE。新增 COLD_COPY_SIGNALED 由 T888 共同 drain 判据核同 fence；owner 终态沿用 T897。跨臂真实输入和原字等价继续由 T496/T865 判定。
- DM2/R1 各自验证 SOURCE ROOT/ROOT_TIMELINE、原 wait、退役及 query。迟到退役与业务接受分别记录，错 fence 或代拒绝。检查时间戳 availability、按族点位和有效位回绕以及 span 复算。无退役和缺关键字段分别返回状态。
- T896 三份重放历史记录均保留 M1_COLD_ROOT_BACKING、0 动作和缺 drain；两份样机历史 ROOT 均 vk=2，旧 RETIRE 为 DM2 vk=0、R1 vk=2。两族均缺 ROOT_TIMELINE，旧结果不能完成新版同 fence 关联。
- 自测使用 T896 实际接受尾字加显式合成拷贝收据与载荷。覆盖 4 个归档和两族迟到退役、无退役、部分点位可用，并通过 12 项针对性负控。没有运行 PC 产品。

运行命令：

```text
python test/t905/check.py selftest
python test/t905/check.py cold <原JSONL> <单臂m1root目录>
python test/t905/check.py sand DM2 <原JSONL>
python test/t905/check.py sand R1 <原JSONL>
```

退出码 0 表示证据门通过。样机输出 accepted=false 时，即使退出码为 0，也仅表示拒绝根退役证据可核。退出码 2 表示证据缺失或不一致，3 表示尚无成功退役或设备丢失。T904 到件后按各有/无查询臂分别执行，并保实际 GPU、驱动、模块和输入版本。
