# T496 · PC 四臂与忙点原件独立复核

2026-11-23 · tester · Windows PC 离线。运行 `python test/t496/check_pc_d1.py`，完整输出见 [机器收据](pc-offline-join.json)。

## 四臂 D1

四个主臂各 52 个唯一动作与 T865 时间表一致。原 `M1_OFFER_RETURN/TERMINAL` 的 actionId 后多出一个引号。适配器仅对该固定位置做内存修复，逐文件记原 SHA 与行号。SOURCE 7 在四臂均接受，由 actionId、offer、run/geometry、原 producer/command 双摘要及 Completion 逐根对齐。SOURCE 29 在四臂均为 EXPIRED，根为零。旧运行的 CLOCK_GAP、PERMIT_REVOKED 与 FENCE_OR_PERMIT_FAILED 仍登记在原臂。

| 主臂 | SOURCE 7 实际间隔 ns | 接受量 Q20 | 原完整 drain |
| --- | ---: | ---: | --- |
| OFF | 14571100 | 122231 | false |
| ON | 16076100 | 134856 | false |
| REPLAY_OFF | 16079800 | 134887 | true |
| REPLAY_ON | 16031500 | 134481 | false，inFlight=1 |

四臂均有 WSI Present 退役。严格 T865 四臂对照在 DRAIN 停止，REPLAY_OFF 单臂随后停在时域等价。实际请求与接受量已有非身份差异，因此原字等价未成立。27 份隔离重放 M1PROOT1 各自在同进程同根通过，逐项验证 SHA、Completion、bank 槽、发布槽、事件范围及 copy fence/invalidate。它们证明同根归档，跨臂原字等价仍缺证据。

## D2 与交接

T869 runA 首根 M1PCOLD1 严格读取通过，两页连续覆盖 94336 B。runB 后继根 M1PROOT1 的四段、同根接收/拷贝和单页覆盖通过。runC 的热页经 T865 校验根、时序、256 原字和 SHA，返回 FROZEN_HOT_PAGE。主 ON 的 D1 热页也已与同根原 Completion 精确对齐。

Python 字节编译、T865 自测和两项新连接负控通过。T496 完整对照继续需要 architect 对原 drain 和时域差异分流，再由相应角色补取等输入、完整 drain 的四臂。T504 的 32 轮沿原真实业务前置执行。
