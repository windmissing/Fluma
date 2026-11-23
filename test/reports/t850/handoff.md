# T850 · PC 业务帧率独立计量

2026-11-23 · tester · Windows PC 离线。采用 [T824 工程合同](../../../docs/architecture-T824-v1.md) 和 [PC 120 FPS 设计](../../../docs/design-pc-effects-120fps-v1.md)。

`python test/t850/check.py original <T830或T840的JSONL>` 核对原 Completion、同根 Present 及退役。两份原件各有 1 个 dry SOURCE 诊断，源量均为 131072 Q20，业务帧率待实测。合成验证使用 1 秒 QPC 窗。120/144 Hz 正控测得 120 新物理帧/秒，P95、P99 均为 8.333 ms。混合负控 120 次尝试中测得 113 个新物理显示帧，重复、STALE、空、失败、插值、SOURCE、显示丢弃各一次。60 Hz、未知刷新率、缺显示确认和序号缺口均有独立检查。

## 采证交接

`python test/t850/check.py measure <合并证据.json> --display-trace <原显示跟踪>`。JSON 含 `plan`、`accepted`、`attempts`，最小样例见 `test/t850/selftest.py` 的 `fixture()`。

- 预先固定 PC、GPU/驱动、活动刷新模式、分辨率/画质、电源、负载、暖机和持续窗口。记录 QPC 频率、窗起止、实时误差、原显示跟踪 SHA、尝试序号范围和采集完整度。窗口左闭右开且至少一秒，正式长度在实跑前按代表性持续动态场景登记。
- 接受流按进程、线程、窗口/设备/交换链/表面/几何代、run/config 和 acceptedRoot/发布代/槽位/源提交/物理提交识别。原接受时间、实际步及仿真时间增量独立记录。SOURCE 行仿真时间增量为零。
- 尝试流保 presentId、原 Vulkan 结果及 Present QPC。独立显示跟踪按进程、窗口、交换链和 presentId 或等价事件关联，提供 DISPLAYED 与同域 QPC/唯一显示 ID，或 DROPPED。工具核对原跟踪 SHA。实际跟踪导出、事件身份关联和活动模式收据由采证方提供。

分别输出 accepted/queue/display 速率、仿真时间比、各分类及丢弃、平均间隔、最近秩 P95/P99、最长间隔、已接受但窗内缺显示确认的数量及计时成本。队列以返回 QPC 入窗，显示要求真实显示事件也在窗内。60 Hz 可单列应用吞吐，120 FPS 呈现保留待测。实跑待对应产品工件、活动显示模式和跟踪原件。
