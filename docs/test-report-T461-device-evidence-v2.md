# T461 续办取证记录（T521 §3 最小补证）

日期：2026-11-17 上午，tester，使用真机（62T0225B10010020），设备串行。
被测包：T482-final-signed.hap，SHA256 `300bd52a97e360cf533e261bf35433b465f560329f4a73232d8e119cdb51381e`（本地逐位核验==T521 声明）。ELF 未重编（沿 T521 §1 核对结论 783cb115）。
冷启 PID 58485，`bm clean -d` 后 `aa start`，弹窗键 (240,620) 已处理。
证据目录：`test/reports/t491/t461r2/`。

## 现场 bounds（每次动作前 dumpLayout 实测，非硬套）

| 控件 | l1/l2/l7 bounds | 中心 |
|---|---|---|
| 开启独立 GPU M2 V3 空场 | [254,1414][1027,1554] | (640,1484) |
| 导出 M2 创建原始诊断 | [304,2384][976,2524]（后移 [304,2339][976,2479]） | (640,2454)/(640,2409) |
| 导出上一创建诊断 | [360,2545][920,2685]（后移 [360,2500][920,2640]） | (640,2615)/(640,2570) |
| 锁存 V3 冷档 | [189,2178][626,2318] | (407,2248) |
| 导出 V3 冷档 | [654,2178][1091,2318] | (873,2248) |

原始注入命令：`uitest uiInput click <x> <y>`（各动作命令+exitCode 在本记录；文件目录 stat/ls 原文 `files-ls-before.txt`/`files-ls-after-both.txt`）。

## 状态机事实链（截图 s1..s7 + layout l1..lA 原件）

1. 进入 M2 V3 页：`STATE_LOST WAIT_RETIRED / run 0:0`，四账全 0。
2. 无人操作转 `WAIT_NEW_SESSION WAIT_NEW_SESSION / run 0:1`，四账全 0（s1/l2）。
3. 点沙(407,573)/点水(836,573)：状态转 `PAUSED_CLOCK CLOCK_GAP / run 0:2`，四账仍全 0（l9）。
4. **NO_PREVIOUS_CREATE 正确拒**：bootSerial=1 时点"导出上一创建诊断"→提示 NO_PREVIOUS_CREATE（s1 底部），未产生文件——符合 T521 §3"上一为 0 应 NO_PREVIOUS_CREATE，不冒 previous"。
5. **NO_TRACE**：锁存 V3 冷档→导出 V3 冷档，提示恒 NO_TRACE。源码核（app_bridge.cpp:495-517）：锁存前置 `m2TraceRequested` 且 `CaptureRootIdle()`+`Freeze`+completion c[1]==3——WAIT_NEW_SESSION/CLOCK_GAP 域下 root 非 idle/非完成态，TRACE_UNAVAILABLE/未请求 → 导出无产物。与页面状态自洽，非按钮未命中（同坐标沙/水按钮均生效）。

## 创建诊断导出取证（本轮核心新事实）

三次导出（当前×2、上一拒绝后新 serial×1）产生 3 份 raw log：

| 文件 | bytes | sha256（设备端 sha256sum） |
|---|---|---|
| t468-create-1-1794875258311-0.log | 0 | e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855（空串 SHA） |
| t468-create-1-1794875905328-0.log | 0 | 同上 |
| t468-create-2-1794875898791-0.log | 0 | 同上 |

**全部缺 native.json / manifest.json，WS_EVENT t468_create 收据零条**（hilog-run.txt/hilog-run2.txt 全文无 WS_EVENT）。

页面提示（lA）：`CREATE_EXPORT_FAILED Error: update failed`。

### 根因定位（页面源码核）

`GpuSandWaterExperience.ets:215-223` 导出链：`m2CreateLog(serial,i)` 逐 object saveRaw → files.save(native.json) → manifest.json → WS_EVENT。
`M2CreateColdFiles.saveRaw`（M2CreateColdFiles.ets:7-14）：写文件+fsync 成功后调 `cryptoFramework.createMd('SHA256'); md.update({data:bytes})`。
**当 raw hex=''（Program.raw 未尝试/空，T521 §2 明确合法）→ bytes=Uint8Array(0) → md.update 空 data 抛 "update failed"** → catch → CREATE_EXPORT_FAILED，收据链断。

即：0B raw log 本身是合法写盘（三份都在盘、SHA=空串 SHA 逐字）；断链点是页面收据计算对 0 字节数据不兼容——**取证流程缺陷（tester 域/页面导出实现），非 create 物理缺陷，也非文件完整性缺陷**（文件在盘 bytes/SHA 与实际一致）。

### 按 T521 §3 判读表归位

- 判读4 命中：只取得取证错误（CREATE_EXPORT_FAILED），tester 修证据流程/必要工程缺口回 architect，**不伪派物理缺陷**。
- 结构 JSON（native.json）因上述断链不可得 → m2CreateDiagnostics 的 code/sealed/firstFailure/queried/emptyLog 字段**本轮未取到**，不猜值。
- 四账全 0 + CLOCK_GAP 域维持：同 scope sealed/firstFailure 原件仍缺 → 判读1/2/3 均不满足前置，功能维持 **NOT_REACHED**。

## 本轮已补证 vs 仍缺

已补证（T521 §3 要求项）：
- 启动/安装/PID/包 SHA 逐位 ✓（300bd52a，ELF 沿 T521）
- 现场 bounds/中心/命令/exitCode ✓
- 操作前后 ls -l/stat ✓（before/after 两份）
- 三份 raw log 原件+设备端 SHA+bytes ✓（0B 合法域 + 空串 SHA 逐字）
- NO_PREVIOUS_CREATE 语义正确性 ✓（bootSerial1 无 previous 拒，未冒 previous）
- NO_TRACE 与页面状态域自洽性（源码级）✓
- hilog 全文两份留档（WS_EVENT 零条=收据链断的直接证据）✓

仍缺（前置未消）：
- native.json/manifest.json（被 CREATE_EXPORT_FAILED 阻断；修需页面 saveRaw 对空数据 md.update 兼容——工程缺口回 architect/developer）
- m2CreateDiagnostics 结构 JSON 的 code/firstFailure/queried/emptyLog → 缺 native.json 即缺
- READY/session/pulse 交接域证据（GPU 状态域恒非 CURRENT，与 T510 同根，T530 归因）

## 结论

T461 续办（T521 §3）本轮完成"先复用现有诊断补证"可达部分：**三份 0B raw log 合法性+设备端 SHA+根因链（md.update 空 data 抛错）全部落证**；卡面要求的完整诊断原件被页面导出缺陷阻断——精确缺口=saveRaw 对空 raw 的收据计算（一行兼容修），回 architect 分流 developer。功能域（M2 V3 能力/整根）维持 NOT_REACHED。
