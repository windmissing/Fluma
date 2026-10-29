# T236 固定容量逐 pass 记录

2026-10-29，developer，不使用真机。接续 `developer-T236-gpu-resident-M0-id-v3.md`。

`SourceGpuTransport` 增加固定 256 行 trace，逐 dispatch 记录完整 run/owner/context/surface/geometry/config 与三项 SHA 身份、RootRequest、pass ordinal/stage、group 数、实际屏障 bits、状态/工作区/输出分配字节与 CPU steady-clock 提交时间。零等待 fence poll 记录原始返回值；1024B completion 记录映射量与产品校验结果。环满不覆盖旧记录，累计 dropped，Complete=false 明示 INCOMPLETE。冷归档后仅 owner 且无 inflight 时可清空。

此切片无动态 trace 分配；GPU timer 明确 unavailable。字节字段是绑定缓冲区范围，不是硬件带宽测量。尚缺精细 binding offset、GPU timer/disjoint、draw/swap/lease 与失败分支完整 trace；不声称已覆盖 T259 §3 全合同。

hvigor native/ArkTS/PackageHap BUILD SUCCESSFUL，日志 `test/reports/t236/resume/trace-package.log.gz`。软件 GL 自验日志 `trace-host.log.gz`；真实 shader 链同时验证固定环溢出 INCOMPLETE 与冷清理，PC 软件 GL 结果不代表真机。

**部分交付，看评论。**产品 APP_ROUTE/XComponent 唯一异步 owner、生命周期 pump/lease/swap、全失败矩阵与末 ID 实际 shader fixture 尚缺；本卡不关闭，不把这些缺口归因于等设备。
