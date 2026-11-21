# T765 · RX 首 pulse 交错与时限离线判定

2026-11-21 · G20 · tester · 离线

已核对 T763 工件的 HAP、ARM64 ELF 和角色 manifest 字节，绑定 BuildId `4bdd76ddf02618f9579836d437aeca99d9039f63`。Windows x64 编译执行实际 RX `ProgressClockV3` 与时限 guard。锁内发布交错使用两条宿主线程及可控时点。Controller/Transport 的持锁提交和页面 NAPI 顺序另作源码接线核对。模拟 fence 和事务事件只用来反证判定器。[T763 编译收据](../test/reports/t763/compile.json) · [离线结果](../test/reports/t765/offline.json)

| 证据 | 结果 |
| --- | --- |
| 10 个原生时钟与交错夹具 | PASS。首 PagePulse 建 baseline，合法首次零配额 Claim，旧 owner 时间和新 pulse 混采误拒，同锁后取时通过。错 scope、重复 Seq、Target、UI gap、现有 pending、Stop 均覆盖。 |
| 10 个时间向量 | PASS。33,333,333 ns 和 250 ms 的闭区间边界通过，+1 ns 拒绝。旧时间读到新 FIFO receipt 单列，新的合法 PagePulse 不延长根 deadline。 |
| 10 项生产接线检查 | PASS。锁内取完快照后读时钟，持锁提交入口没有 mutex 回调或 EGL/GL 调用，typed proof 在 bank 晋升之前。同根 fence 后再次过 guard 才能 map。 |
| 13 个合成时序用例 | PASS。正确提交、撤权、错 scope、pending、typed 拒绝、Target 假刷新、迟到 fence 和退役均分辨。伪造的迟到提交/回读、提前 Claim、Target pulse 快照及无完成证据的退役均被拒。 |
| T755 历史原件 | 同 ticket 的 SOURCE 编译/链接均在 APPLIED 前，UI 终态为 `PUMP_FAILED code=5`。首个失效谓词和实际 Pump 游标仍未知。 |

在 Visual Studio x64 Developer Command Prompt 从仓库根目录运行：

```text
python test/t765/run.py
```

续测拿到 T763 实机导出的时序文件后，运行同判定器。只有符合设备原件的结论才用于 SOURCE 结果登记。GPU 原始尾、物理 oracle 与首 Swap 仍由真机任务独立取证。本卡未运行 HAP、未签包、未使用真机。

```text
python test/t765/check_trace.py <g9rx-source-N.json> --compile-receipt test/reports/t763/compile.json
```
