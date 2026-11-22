# T793 · RX SOURCE 侧车保存失败归因与交付合同

2026-11-21 · 目标 20 · P5 · architect · 离线原件复核。

> 2026-11-22 T805 复核。T802 的签名输入实际是 T798 `default` 单体工件，正式侧车应使用 T798 `bootSource`。独立应用沙箱 writer 验证和正确拆分包的低干扰冷启可并行准备，各自取实际原件。 [T805 工件复核与两条执行线](architecture-T805-v1.md)

**T789 已运行 T785 侧车导出，应用保存时返回 `Invalid argument`。** 已识别一处与原错吻合的代码缺陷，`fs.mkdtempSync` 的目录模板少了末尾六个 `X`。先修模板并加逐步回执，再由同一应用入口验证实际保存与 HDC 取件。旧构建只记了聚合异常，T789 的具体抛错阶段仍缺原始回执。 [T789 原 hilog](../test/reports/t491/t789/chain.txt) · [工件与原值](architecture-T793-evidence-v1.json)

## 产物、入口、触发与取件

| 阶段 | 复核结果 |
| --- | --- |
| 签装工件 | 实物未签 HAP SHA `957af8e144c8a08d3b735b6070d55d67f96d24e6eb8c8be930360d2b4bbe534c`，签后 `1079fd0e818019373cc3589ecf8ebe2a6ae9e7f68805e54a1de6b7f5e0bae172`。T257 `verifyApp=PASS`。签前后原包条目逐字相等，入口为 `EntryAbility → G9RxResearchAbility → pages/GpuResidentG9Rx`。 |
| 真实 ELF 与页面插桩 | 两个 HAP 的 `libgrs_g9rx.so` SHA `fde237cd…`、BuildId `e80a72482133670dbaa3ed9e06ebdc141aa591fd`，同 T785 回执。`chain.txt` 同 PID 8890、票 1、owner TID 9118 的 `G9RxConfigure` 实报相同 BuildId 和 SOURCE SHA。包内 ABC 带 `G9RxSourceExport`、`SAVE_FAILED`、`g9RxSourceDiagnostics`、`mkdtempSync`，ELF 有 `RX_SOURCE_EXPORT_V1` 和退役槽标记。 |
| SOURCE 与导出 | Configure 到 `APPLIED`（run/context 1/1），布局为 `PUMP_FAILED code=5`。同 PID 的 UI 线程在 22:41:43.769 报 `G9RxSourceExport: SAVE_FAILED ticket=1 reason=Error: Invalid argument`，布局同时报 SOURCE 保存失败。代码 5 为 `STALE` 的多谓词映射，首拒和真实 PagePulse/receipt age 未知。导出异常与 Pump 拒绝分别归因。 |
| 文件与 HDC | 原报告称只看到 `gpu-program-cache`，未归档精确的 sandbox 目录、HDC 命令/stdout/stderr/返回码。现有 hilog 无 `SAVED` 路径，未取得 terminal 或 retirement JSON；`boot-retire.txt` 是空文件。T785 只有首份保存成功且返回 `PENDING` 后才启动后续退役采集，首份异常本身足以中断该分支。实际 SOURCE 退役仍按未观测登记。 |

原错误已经把首个已知缺口定位在应用保存链路。T789 报告所写“交接不含输出插桩”与实际 ABC、ELF、UI 和 hilog 冲突。没有可靠记录表明 HDC 取件失败引发了这条应用错误。 [签包原回执](../test/reports/t257/T789-rx.json) · [T785 编译回执](<D:/hmprojects/HelloWorld-T771/docs/receipt-T785-compile-v1.json>) · [布局](../test/reports/t491/t789/l2.json) · [T789 原报告](test-report-T789-v1.md)

## 先修真实目录 API

T785 的 `saveTiming` 先按票读取 native snapshot、校验并编码，再把 `filesDir + '/g9rx-source-' + ticket + '-'` 传给 `fs.mkdtempSync`。这个模板缺少 API 要求的 `XXXXXX` 后缀，公开 OpenHarmony 实现会将模板交给 `uv_fs_mkdtemp` 并抛出原始错误。现象与缺目录、`Invalid argument` 一致。原 catch 只保 `String(error)`，无法从 T789 单独确认发生在 mkdir；代码缺陷已足以直接修，下一包应用逐步回执确认。 [T785 页面保存入口](<D:/hmprojects/HelloWorld-T771/entry/src/main/ets/pages/GpuResidentG9Rx.ets#L31>) · [华为 File FS 模板](https://developer.huawei.com/consumer/en/doc/harmonyos-references-V2/js-apis-file-fs-0000001451843016-V2/) · [OpenHarmony 原生实现](https://github.com/openharmony/filemanagement_file_api/blob/master/interfaces/kits/js/src/mod_fs/properties/mkdtemp.cpp#L26-L56)

### Developer 实施

1. 将同一保存函数的目录模板改为 `filesDir + '/g9rx-source-' + ticket + '-XXXXXX'`，也可直接沿同一 API 的异步版本，保持系统原子创建且路径限定在本应用 `filesDir`。自动 terminal、retirement 和手动保存复用同一调用。保现有完整 UTF-8 内容、短写续写、fsync、close、大小核验及同目录改名。一次写入使用唯一目录和一个临时文件。成功回执在最终文件可 stat 后发布，记录路径、字节和 SHA-256。失败的 `.partial` 只作诊断。原分析中的 BOOT 和 SOURCE GLSL、准入阈值及事务保持原值。
2. 原路径保明确步骤 `SNAPSHOT / VALIDATE / ENCODE / MKDTEMP / OPEN / WRITE / FSYNC / CLOSE / STAT / RENAME / SAVED`。失败带步骤、原 `BusinessError.code`、message、kind、PID/start、ticket、run/context/nonce 和已经取得的路径或 bytes。保错误原阶段，close/清理异常单列；诊断出错不改变 GPU source fault、返回值和回收。保 UI 能读到同一个回执，native terminal 与 ArkTS 导出/取件结果各用独立状态。回执不输出完整快照至 hilog。构建回执额外固定 ABC SHA、页面名、ELF、同 ticket schema 与写盘修订，继续沿 T257 正常签包和 payload 同一性核对。
3. 一次冻结并保留同票的 terminal snapshot，后续实际退役快照单独留存。单次落盘失败后保持该票的文件重试能力，避免 `autoTimingTicket` 导致不可恢复。人工重试只重写同票已冻结 bytes，不再次 Configure 或改动真实 pulse；保终态已 `RETIRED`、`NO_PENDING_WORK` 或 `CONTEXT_RELEASE_UNVERIFIED` 的原值。只在 terminal 记录 `PENDING` 且同票后续有退役终态时要求独立 retirement 文件。若文件暂未写好，分别保保存失败和已知的退役观测，无退役证据时保 null。GPU owner 和 PagePulse 线程不等待文件 IO。

### Tester 与 effector 真值

- **Tester** 用原 T785 的 terminal/retirement/partial write/reader 用例继续核验。增加真正挡住此回归的文件 API 适配负控，缺 `XXXXXX` 得到 mkdir 错误，正确模板保唯一目录。注入各步原错误和重试，核首错、无假 `SAVED`、同票快照不被变化的 native 状态覆盖。T785 的 19 项 JS 与 6 项新增 reader 原用例本轮复跑通过，原 JS 只做部分写路径模拟及源码顺序检查，没覆盖真实文件 API 模板。 [T785 JS 用例](<D:/hmprojects/HelloWorld-T771/test/t785/check_export.cjs>) · [T785 reader 用例](<D:/hmprojects/HelloWorld-T771/test/t785/check_trace.py>)
- **Effector** 用一次实际新工件绑定签包 SHA、页面、真实映射、PID/start 与 ticket。若修订包提供同 writer 的最小沙箱写入验证，可在长冷编译前先验证建目录至 rename；该产物独立标记为导出验证。再走一次真实 SOURCE，终态前开始连续采 `G9RxConfigure/G9RxSource/G9RxSourceExport/G9Boot`，避免事后从 log ring 推断。等 `SAVED` 或明确首错，使用记录中该票的完整 `filesDir` 路径按 T785 正常授权 HDC 方式 `ls` 和 `file recv`，保存实际用户/bundle 上下文、命令、原返回，核设备长度和接收文件 SHA-256。终态后先采/取原件再结束进程；无法获取只读目录或拉取权限时保原错。按 T765/T785 reader 复算真实 guard、pulse/receipt、owner/cursor、fence raw 和实际退役。继续保 **250 ms**、**33,333,333 ns** freshness、同根 typed/oracle 与 XComponent+shader。 [T785 归档和 HDC 命令](<D:/hmprojects/HelloWorld-T771/docs/developer-T785-v1.md>) · [原 RX 时序合同](architecture-T772-v1.md)

T793 完成工程首归因与修订合同。产品代码修订和真机 I/O 分别由 developer 和 effector 接续，本轮未使用真机。
