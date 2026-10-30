# T295 bridge 首帧分段时间线

2026-10-30，developer，不使用真机。消费T291，生产实验 `app_bridge.cpp` 增固定容量 `BridgeTimeline`（1024条，固定struct，容量/实际sizeof随导出报告，满保留先到原件并dropped++）。热路径仅定长写入，同App mutex；无JSON/hilog。steady_clock原ns与worker/caller线程hash TID、serial、run/epoch/surface/input、active/window/permission、queue、原now/last/delta/carry/quanta/code/root分列。记录Invalidate/InitBegin/Ready/ReadyRejected/WaitEnter/Wake/FirstTick/AdvanceBegin-End/AnchorSet/PumpBegin-End/RootSubmit-Poll/DrawBegin-End/SwapBegin-End/PauseBegin-End/Offer/Consume。pause后停止重复wait/Pump取证，保首次pause，下一初始化重新记；不把同一次pause的反复UI采样当独立失败。旧事件epoch不改写，初始化开始身份未知时保持旧快照并以新epoch标记，Ready行才是新完整身份。

首tick `last=0,delta=1` 与成功Advance后的真实anchor明确；延迟原值仍>33,333,333ns即原Pause/撤权，不钳值、不补跑、不丢carry、不改120Hz/F≤4/协议。RootSubmit使用transport已提交trace nonce；Poll/Draw/Swap标实际调用边界，不将CPU wall时段当GPU timer或OS挂起唯一原因。T282原子权限/旧代隔离保留。

status仅新增count/dropped/serial及冷API说明；`libgrsm0.timeline(cursor)`精确整cursor 0..1024，每页最多4行，ns/TID/U64值用十进制字符串防JS精度丢失，next=count后停止。单次页有界，无自动全表日志；整个1024行冷JSON预算1MiB（固定字段/最大十进制宽度）。末页dropped/incomplete保留，不能将溢出导出当完整。NAPI调用需同生产模块，不用UI轮询时刻替代worker时刻。

同生产App::loop编译的host测试通过 **16/0**：正常首tick/空帧、等待40ms、Pump40ms、Draw诊断段40ms、源队列旧epoch与1024满溢出；真实ResidentRun/Fixed120Clock，EGL/window/transport为显式平台替身。Draw延迟通过同Mark/Hook诊断接点注入，非真实GL/swap/设备证据；实际Draw/swap接线由生产编译覆盖。T282竞争18/0、原CLOCK_GAP/取消14/0、Node NAPI17/0。hvigor PackageHap **BUILD SUCCESSFUL**。签包verify PASS，`.codex/local-test/t257-sign/T295/T295-signed.hap` SHA `133cb07ea25426f267bfac1a717a1185eb1a274d2ae16dc06b8b7c5c4a49cec6`。原始 `test/reports/t295/{host,ready,clock,napi}.txt`、package.log.gz、`test/reports/t257/T295.json`。

T296在此签包采worker trace并配系统时间线；本卡不宣READY稳定、完整产品链、Doze唯一因或设备收益。所有背景前景仍XComponent+shader，无Canvas回退。
