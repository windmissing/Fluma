# T309 明水关键收据 V2 保留与冷分段

2026-10-30；developer；不使用真机；消费 T305 工程。只改隔离 `cwdiag=true` C诊断路线，非GPU M1新效果，也不把此F64参考线路冒M1当前权威。T306/T303已按顺序交付。

`test/t309/generate.cjs <fresh .codex/local-test/...>` 沿T270生成后叠加V2/refine。V1原实现保存为ClearWaterDiagnosticV1.ets，既有V1档仍可按原revision只读。V2 critical独立16384×64 F64，低优先256×64 F64。DOWN/CLOCK_WINDOW/ENQUEUE/ACK/BATCH/TERMINAL_QUERY/OLD_KEY_QUERY与有效STOP不被TICK挤占；同run、同reason、期间无新有效活动的重复STOP进低优先lane。低优先每60次尝试记一次，skipped/dropped单列；不修改原stop调用或input代演进。

critical复制完整原key、runId及request revision、数值/单位/时钟、ACK/终态/只读旧key状态、disposition、准入/拒绝、transaction/kindSuccess、源/物理commit、全水/列水前后与I_water、errorStage/replay。原始request复用不回写历史。额外四个当前源账标量通过隔离Engine只读copyDiagnosticLedgerInto复制，不开放canonical、不加全场扫描或改变source/物理。保留原三帧profile/header/ledger/optics/requested/rendered/LastGood token与native ROI；导出使用已封存帧槽，不能拿当前状态代旧帧。

满critical或主字符串预算耗尽明确criticalDropped/incomplete，首次丢失的完整数字key、run/单位/revision/reason独立固定槽保留；不覆盖首记录。普通热记录只写预分配数组/字符串引用，不做JSON/日志/文件。关闭诊断仍不创建该对象与大lane，原候选/源反馈/光学/LastGood不变。

容量边界：T293原长窗只留256+dropped2913=3169次事件，关键请求实际分母已丢，不能从中捏造请求频率。新容量为该全部事件上界约5.17倍。另以40秒×60Hz=2400请求、每笔五类关键行再加停止边界的更强离线洪水验证，共14401行仍无criticalDropped；不是声称支持任意频率。更高频/更长窗真实耗尽必须INCOMPLETE，不能冒逐笔完整。

精确typed payload：critical8,388,608B+low131,072B；原三帧数组共28,416B，复用V1低lane所带未使用帧数组另28,416B，firstLostKey192B，总8,576,704B。主要逐行文本按UTF16保守累计限制8,388,608B；固定revision共享字符串/数组引用和JS对象开销不充精确RSS，运行时/driver内存NOT_MEASURED。未自动加大物理预算或GPU profile。

显式冷出口由实际ClearWaterDemo诊断按钮调用：每256行分段，严格cursor/next、首末完整key、包SHA、原run/行、SHA256/长度、固定字典与rowFields；完整frame/ROI、low lane另文件。沿T306文件writer写全+fsync+SHA，段文本总预算32MiB，所有清单文件短回执。busy/异常退出不清原环；缺END或缺段/hash不符由独立check.cjs判INCOMPLETE；导出期间critical追加/溢出同样INCOMPLETE。包SHA沿原cwdiagSha外部签包身份，由tester核对实际包；不伪称self-hash。分段是固定窗口冷封存，不在热tick自动落盘/清环或无限续存。

非机7/0：40秒洪水、真实critical溢出首lost key、可变请求/ACK/终态历史、字符串预算、三段cursor/hash/缺失/损坏、导出增长、IO中断。原T270行为/物理/旧key/源反馈/三帧断言26/0（只更新diagnostic revision期待，不改数值判据）。ROI为原mock-native边界检查，不冒设备readback PASS。生成源、证据、构建和签包SHA在test/reports/t309/delivery.json。开发编译/签包完成即可交tester长窗，不等设备或可见效果通过。
