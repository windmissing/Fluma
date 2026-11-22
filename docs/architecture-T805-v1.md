# T805 · RX 冷编译与侧车独立验证

2026-11-22 · 目标 20 · P5 · architect · 离线工程合同。

**T802 签的是 T798 默认单体包。真实 SOURCE 侧车验证需要 T798 的 BOOT_SOURCE 包。** 两轮都只留下编译中或取消请求的页面状态，未采到本次 native 编译返回、SOURCE 终态或 writer 结果。先纠正 T802 工件，同时为同一文件 writer 准备几秒即可触发的独立沙箱验证。 [原件核验](architecture-T805-evidence-v1.json) · [T802 原报告](test-report-T802-v1.md)

## 工件错配与两轮原件

| 工件 | 本地实物联核 |
| --- | --- |
| **T802 实际签包的输入** | T798 `default` 未签 SHA `9c9097d3a8a505a9ae557a61e5fd260a331cb6d66205f8347bc325a82578196b`，经 T257 签为 `6eedc7fcb678ee0d9f39650b350229b57582305204226a2abf6cfa9065086a6d`。签前后原 payload 14 条逐字一致。包内 `libgrs_g9rx.so` BuildId `775c7116dd30f1a8675962e4b06862166d6422e6`，真实内嵌 **146,301 B** 单体 GLSL、SHA `2cb9a848…`，缺 `RX_SOURCE_EXPORT_V1` 与 `SOURCE_BUILD_ENTER`。 |
| **T802 正确目标** | T798 `bootSource` 未签 SHA `da5ec9b492f7c3b6b50b88e06fd1e62ac8f47bfd42d64c41a3edb898aee1ba27`。相同 Ability、page 和 ABC `8a1b8b43…`，包内 DSO BuildId **`f96d365d162691cdfd7ebfc86d1be291b635d6cc`**，剥离 ELF SHA `3891c1e177c9eb8826197da61360b39162bbf4c3918645e1a6c579777126e11a`。内嵌 BOOT **22,318 B**（`8e0a89e6…`）、SOURCE **41,767 B**（`ee66f5f2…`），native 导出和 SOURCE 启动标记均在。真实 roleSet `8fae2b8c…`。新签后 SHA 待实际签包。 |

两包都含 `G9RxResearchAbility`、`pages/GpuResidentG9Rx` 与 T798 writer ABC 标记。单体 native 产物走 `programcache::Build` 的单体分支。页面只在 `status.source.variant === 'BOOT_SOURCE'` 且同票出现 fault/commit 的条件下触发 SOURCE 侧车。默认单体跑完也无法满足该触发。T802 原归档缺本次设备实际映射 BuildId、库字节和同票 native 阶段链，因此实装身份仍须新一轮在设备上核对。 [T802 签包原收据](../test/reports/t257/T802-rx.json) · [T798 双工件回执](<D:/hmprojects/HelloWorld-T771/docs/receipt-T798-compile-v1.json>) · [T798 触发入口](<D:/hmprojects/HelloWorld-T771/entry/src/main/ets/pages/GpuResidentG9Rx.ets#L210>) · [分变体原编译接线](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L281>)

| 轮次 | 原件支持的结论 |
| --- | --- |
| v1 | `l1.json` 显示 `RX #1 SHADER_COMPILE_ENTER`。`hl.txt` 留 PID 29672 的 `isForeground:false` 与 `DOZE_AFTER_ENTER_BG`，后续布局和 `diag.json` 留 `CANCEL_REQUESTED`。前后台/休眠切换与取消请求有证。页面 `onPageHide/aboutToDisappear` 会经 `stop()` 发取消，该请求不证明已退出编译、进程被杀或驱动调用被中断。具体触发及完成边缺同票 native/进程原件。 |
| v2 | `lf–lp.json` 仍停 `SHADER_COMPILE_ENTER`，`lr/lt.json` 为 `CANCEL_REQUESTED`，`hl2–hl4.txt` 有 `51412` 的 XPower 高负载条目。报告记 15 分钟以上未返回及 CPU 原值 159600→367764 tick；归档未留同 PID/start/TID 的 `/proc` 原采样、单调时间窗口或 `CLK_TCK`，无法复算为已证的单 API CPU 时间。高负载和页面阶段只能说明仍有活动及编译状态未前进，热、频率下降、其它任务争用及周期 swipe 对墙时的贡献均待受控对照。 |

两轮目录内的 `chain*.txt` 和 `app-domain.txt` 为空，未见本轮 `G9RxConfigure/G9RxSource/G9RxSourceExport` 原事件及本票 `SAVED`/`SAVE_FAILED`；`saved.txt` 是其它进程的 ArkCompiler 日志。T789 的旧 BOOT_SOURCE 原件有实际 SOURCE 编译 **87.037 s** 和链接 **234.839 s**，合计约 **5.36 分钟**。它说明同类拆分路径的历史量级。T802 的单体签包结果不能用这两个历史阶段配对，也没有测到 T798 修订的真实 SOURCE writer。 [v1 布局](../test/reports/t491/t802/l3.json) · [前后台原日志](../test/reports/t491/t802/hl.txt) · [v2 持续状态](../test/reports/t491/t802/lp.json) · [后段高负载原日志](../test/reports/t491/t802/hl4.txt) · [T789 历史原日志](../test/reports/t491/t789/chain.txt)

## 短路径 · 同一 writer 的真实沙箱验证

**Developer 交显式隔离构建，tester 交独立校验，effector 在自己窗口签装、执行和恢复。** 从 T798 提取实际 `RxPlatformFiles` 适配器、`TextEncoder` 和 `RxSourceWriteJob.save` 的落盘状态机为两入口共用的模块，保原 T798 模板、阶段和错误语义。正式入口仍从 native 请求并校验真实 `RxSourceSnapshots`，再把冻结的 UTF-8 输入交给共同 writer。测试入口使用独立 probe id 和序列化文本，经**同一**模块与系统 `@ohos.file.fs` 适配器写进实际应用 `context.filesDir` 的 `g9rx-writer-probe` 子目录。两个入口的目录/受控文件名不同，落盘代码只有一份。 [T798 writer 核心](<D:/hmprojects/HelloWorld-T771/entry/src/main/ets/physics/RxSourceArchiveCore.ets#L136>) · [实际系统文件适配](<D:/hmprojects/HelloWorld-T771/entry/src/main/ets/pages/GpuResidentG9Rx.ets#L11>)

1. **探针入口与包。** 用独立构建开关切换到单独的 writer 验证页，复用已经授权的主 bundle 调试签名方式，冻结签前后 SHA、实际入口与 ABC、writer/adapter 源摘要及版本标记，按同 bundle 测试窗口保数据并恢复。测试页只提供一次按钮操作和状态文本，无 `XComponent`/EGL、Configure、NAPI SOURCE、PagePulse、真实 tick、oracle 或产品 cache 写入。普通研究/产品构建没有探针入口。只承诺实际应用沙箱路径，探针可与 T802 分别在现有串行窗口排，不作为 T802 的新开始门槛。 [同 bundle 覆盖和恢复合同](architecture-T777-v1.md)
2. **真实文件阶段。** 在触发前开始采探针日志。用稳定、有非 ASCII 内容的小型固定 JSON（≤4 KiB）走 `ENCODE/HASH → MKDTEMP(-XXXXXX) → OPEN → WRITE → FSYNC → CLOSE → STAT(partial) → RENAME → STAT(final)`。最终文件与临时文件在同一唯一目录。最终大小匹配后才发布 `RX_WRITER_PROBE_SAVED`，保 writer 版本、probe id、PID/start、路径、bytes、SHA-256；失败 `RX_WRITER_PROBE_SAVE_FAILED` 带实际阶段、原 `BusinessError.code/message` 与清理次错。以同一冻结 bytes 重试，沿 T798 对 rename 后 stat 失败的处理复核原最终路径。Tester 复用原故障夹具覆盖目录模板、短写、各阶段异常、错误归属、重复触发及重试，真机至少保一例真实完成，拉回逐字核 UTF-8 末尾换行/长度/SHA 和实读内容。 `.partial` 只按未完成取证。
3. **证据隔离。** 测试 payload 采用 `RX_SOURCE_WRITER_PROBE_V1`、`fixtureOnly=true`，文件按探针专名归档，保真实装载身份和 probe id。无模拟 native SOURCE ticket、nonce、fence 或提交字段。独立 tester reader 只输出 writer IO 结果。T765 现有入口对此信封因缺 `status` 非零退出，验证设施应在交给 SOURCE reader 前报明确的域不匹配。成功可定位「真实文件 API 与 HDC 路径可用」，对生产 `terminal/retirement` 触发、同票快照、源码编译或首帧继续各用真实原件验证。

## 正式路径 · 低干扰 BOOT_SOURCE 冷量测

1. **先纠正签装，再开始长轮次。** Effector 取 T798 `t798-source` 的 `da5ec9b4…` 未签原件，沿 T257 签名，验包内 `f96d365d…` DSO、两个角色的实际 GLSL、native marker 和 `G9RxResearchAbility`。部署后通过设备映射和原日志 preflight 联该进程 PID/start、ELF、入口和 roleSet，BuildId 不一致在 Configure 前停止采证。若存量包已签先以完整收据核对。用同一研究输入和 K192，留实际 cache key/文件、BINARY_LOAD 或源码路径。正式冷样本用已有隔离方式取得目标 key 的 miss 并留实际证据，应用/系统缓存状况记录在案。禁止用清应用数据制造冷态，保真机串行及同 bundle 恢复。 [T798 正式取件](<D:/hmprojects/HelloWorld-T771/docs/developer-T798-v1.md>) · [T727 计时合同](architecture-T727-v1.md)
2. **建立低干扰观察窗口。** 用测试机已有、可恢复的正常亮屏超时设置与同一充电/亮度条件，测前确认实际窗口能覆盖本轮。一次真实配置，持续前台和相同负载，采集期间避免周期 swipe/click、IDE 调试附着或渲染负载变化。前台/锁屏、Owner/TID 和取消边都保设备时间戳。先从 `G9RxConfigure/G9RxSource/G9RxSourceExport/G9Boot` 开始连续向 host 留原日志，分文件顺序保存，低频只读抓阶段状态。同读屏 v1/v2 差异独立记，热状态/频率/争用只在有设备原值时作解释。记录监控命令、频率、观察窗口和时钟源。 [HarmonyOS HDC](https://developer.huawei.com/consumer/en/doc/harmonyos-guides/hdc/) · [窗口生命周期](https://developer.huawei.com/consumer/en/doc/harmonyos-guides/window-lifecycle/)
3. **按原事件闭合编译与提交。** 同 PID/start、owner TID、ticket、run/context 和 BuildId 依次归档实际 configure、cache route、BOOT 及 SOURCE 的 `BUILD/COMPILE/LINK/REFLECT` 原 enter/exit、返回和错误、BOOT qualification、`APPLIED`。若现有导出仍丢视窗，在原 compile/link 回调处增加有界分角色原时戳，不要求 GL 调用内提供进度百分比。每阶段保独立 monotonic wall，能安全实采时附同线程 CPU 或同 TID/start 的原 ticks、`CLK_TCK` 及前后单调采样时刻。`CANCEL_REQUESTED`、`CANCELLED`、后台/进程消失、API RETURNED 分别记。到观察截止仍无退出，就留该已证阶段耗时下界、最后一条原事件和进程状态。建议首个正确工件窗口以能覆盖旧拆分冷路径并留余量为准，若设备窗口允许可先按 **10 分钟** 计划；这个值是取证窗口，SOURCE 的 **250 ms** 根限与 **33,333,333 ns** 真实 PagePulse 新鲜度保持不变。 [T736 正式 SOURCE 契约](architecture-T736-v1.md)
4. **按真实前置分流。** 正确 BOOT_SOURCE 冷轮若仍稳定停在同一个 compile/link API，且已有匹配的原始边与可比环境，才转 developer 分析角色编译或优化；保原数学、ABI、shader 体积门和 G20 冷/暖分账。若实际到 `APPLIED`，再拿同票 `firstReject/recentPass`、真实 pulse/receipt、dispatch/fence/WORK/typed 原证。只有真实故障/提交触发的 `SAVED`，JSON 与返回的 bytes/SHA-256 对齐，经同票 reader 复算才记写盘成果。terminal 已含实际退役终态时一份可记，terminal 为 `PENDING` 且后续退役可观测时才要求独立文件。保存失败交实际 writer 阶段，拉取失败保 HDC 原返回。产品首 Swap 只由同 token 的 XComponent+shader 实证记。 [T793 文件与同票回执](architecture-T793-v1.md) · [T800 离线校验](<D:/hmprojects/HelloWorld-T771/docs/test-report-T800-v1.md>)

T805 已本地解包联核签前/签后 payload、两种 native ELF 和实际嵌入 GLSL，并复跑 T800 的 13 组离线检查及 T785 的 6 个新增 reader 和原基线。尚未生成或运行 writer 验证包。 [证据摘要与原始文件索引](architecture-T805-evidence-v1.json)
