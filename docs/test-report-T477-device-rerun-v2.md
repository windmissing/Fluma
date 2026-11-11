# T477 复跑 · GPU 编译缓存冷暖主链真机验证（v2，T482-final 修复包）

- 日期：2026-11-11
- 角色：tester（真机 62T0225B10010020）
- 前置：T482 developer 修复 coherent 值传参边界（新 SOURCE SHA `b7d0a027128811db…`），签包 `.codex/local-test/t257-sign/T482-final/T482-final-signed.hap`
- 结论：**主链全部打通（PASS）**。上一轮（T475 包）SOURCE 首行即 `COMPILE_OR_LINK_FAILED`（coherent 限定符驱动拒绝）的阻断在本轮**完全消失**：三 program 冷启动全部编译→链接→反射→导出→**PERSISTED**，暖启动（保留数据强停重启）三 program 全部 **BINARY_LOAD 且 compile=0**。冷暖墙时首次取得真机数据。
- 证据：`test/reports/t477/device-rerun-v2/`（三 key 收据 jsonl、2 个 PERSISTED binary、暖轮收据、缓存目录清单×2、UI 布局×4、截图×4、时间线导出×2 轮）

## 签包身份

| 项 | 值 | 核对 |
| --- | --- | --- |
| HAP SHA256 | `300bd52a97e360cf533e261bf35433b465f560329f4a73232d8e119cdb51381e` | 与 t482 delivery.json 一致 ✓ |
| 包内 libgrsm1.so SHA256 | `783cb115038e83e22897400c8e350b9562fba7783fa6fbe2b8c766c9249fa5b2` | 与 t482 delivery.json elfSHA 一致 ✓ |
| 安装 | `hdc install -r` 成功 | ✓ |

## 冷启动（bm clean -d 后首启，`--ps grsm1 true`）

缓存目录 `files/gpu-program-cache/v1/` 清数据后由应用自动重建，三 key：

| program | shaderSHA | key(前16) | 收据事件链 | 结果 |
| --- | --- | --- | --- | --- |
| SOURCE | `b7d0a027…`（=T482 新 SOURCE） | `71ad318e…` | COMPILE_ENTER→LINK_ENTER→LINK_OK→REFLECT_OK→PREPARE_OK→EXPORT_LENGTH→EXPORT→EXPORT_WRITE_SUBMITTED | **PERSISTED**，binaryFormat=36705 |
| INTERMEDIATE_QUANTUM | `54946590…` | `48d746c4…` | 同上（无独立 prepare/warmup，符合 T475 合同） | **PERSISTED**，36705 |
| FINAL_QUANTUM | `0ef4f045…` | `31031d6d…` | 同上+WARMUP_OK | **PERSISTED**，36705 |

三 program shaderSHA 与 t482 delivery.json programs[] 完全一致。`COMPILE_OR_LINK_FAILED` 0 次。supportedFormats=[36705]（与上轮相同）。

### binary 完整性（离线核对）

- 文件=144 字节头（magic `T475B001` + u32 binaryFormat=36705 + u32 binaryBytes）+ payload
- SOURCE：文件 466133 B，payload 465989 B，payload SHA256=`99671708…` == 收据 binarySHA ✓
- INTERMEDIATE：文件 266453 B，payload 266309 B，payload SHA256=`2f6d0120…` == 收据 binarySHA ✓
- FINAL_QUANTUM .bin 523425 B 在设备上已落盘（本轮未拉回离线核对，收据 persistence=PERSISTED、binaryBytes=523281）

### 冷启动墙时（收据跨行字段配对，Maleoon 真机首测）

| program | compile | link | 全 build(ENTER→buildEnd) | 含导出(endNs) |
| --- | --- | --- | --- | --- |
| SOURCE | 6710.8 ms | 21891.5 ms | 28612.9 ms | 28629.2 ms |
| INTERMEDIATE | 2445.3 ms | 11232.8 ms | 13683.8 ms | 13690.9 ms |
| FINAL | 9734.7 ms | 40141.1 ms | 49886.8 ms | 49934.5 ms |

注：Maleoon 源码编译+链接显著慢（SOURCE 全链 ≈28.6s、三 program 串行 ≈92s）；反射/prepare 均 <0.2ms；FINAL warmup 30.8ms。

## 暖启动（保留数据，aa force-stop 后重启，同参数）

三 program 收据尾部均出现 **BINARY_LOAD_ENTER→LOAD_CHECKED→LOAD_LINK_OK→REFLECT_OK→(PREPARE_OK)→REFLECT_PREPARE_OK（FINAL 另有 WARMUP_OK）**：

| program | binaryBytes | load span | BINARY_LOAD_ENTER→buildEnd | 暖轮 compile 次数 |
| --- | --- | --- | --- | --- |
| SOURCE | 465989 | **4.80 ms** | 20.93 ms | **0** |
| INTERMEDIATE | 266309 | **1.04 ms** | 5.62 ms | **0** |
| FINAL | 523281 | **1.89 ms** | 12.15 ms | **0** |

冷→暖对比（同 program 全 build）：SOURCE 28613ms→21ms（≈1360×）、INTERMEDIATE 13684ms→5.6ms、FINAL 49887ms→12.2ms。**暖启动收益主链首次在 Maleoon 真机成立**。

时间线导出（原生 bridge timeline，2 轮独立导出相互印证）暖进程 epoch3：Ready 于 epoch 起点 +101.7ms；首次 SubmitEnter +192.4ms；10 对 DispatchEnter/Exit 全部成功（span 0.20ms，单对 ≤50µs），root 提交开始推进。

## NOT_MEASURED / 如实记录

- **首 root 上屏（Draw/Swap/eglSwapBuffers）未闭环**：UI 状态持续 `STALE_PAGE_SESSION / GPU_WORK_PENDING`，渲染区黑屏。时间线 root=1 段止于 DispatchExit，无 DrawBegin/SwapBegin 行；页面脉冲（pagePulse）与 native 的 session 令牌失配（`progress_clock_v2.h:43 token!=session||!open`）。**这不是缓存功能失败**——三 program BINARY_LOAD 与反射/prepare 均合格；属运行链路（page session/脉冲域）问题，与 T473/T410 记录的 READY 后 GAP 复发同族。首 root/Draw 墙时因此无法测得，归 architect/developer 后续归因。
- **timeline 冷导出 INCOMPLETE（INVALID_ROW），复现 2/2**：两轮导出均在恰好 30 页/120 行后抛 `INVALID_ROW`，坏页未持久化无法离线定位；已落盘的 120 行 serial 连续、字段全部合法（独立复核 0 违规）。另首页 dropped=3568305/4683513（环形覆盖丢弃巨大），即使导出成功也会触发合同里的 `NATIVE_DROPPED` 判定。移交 developer：native `timeline(cursor)` 在特定 cursor 返回含违规字段的行（疑与环形覆盖窗口/并发写相关）。
- 二冷（重签 key 分离/清数据二冷）：本轮签包即修复包首装，未做重签对照，维持上轮 NOT_MEASURED 状态。
- M1 路径真机：本包仅暴露 M2 V3 入口（同上轮），M1 检查仍为离线 fixture。

## 判定

- T477 卡面主链（冷缓存→编译→导出→PERSISTED→强停→BINARY_LOAD→compile0→冷暖墙时）**全部取得真机证据**，三 program × 冷暖 = 6/6 收据链合格，无任何 PASS 冒认。
- coherent 修复（T482）是真机主链打通的直接原因：同设备同入口，上轮 SOURCE 首行失败 vs 本轮三 program 全 PERSISTED。
- 交付移交：① developer——上屏链路 STALE_PAGE_SESSION/GPU_WORK_PENDING 归因（T473/T410 同族）；② developer——timeline 导出 INVALID_ROW（120 行处确定性复现）+ 环形 dropped 判定；③ architect——T477 卡面可判「主链 PASS、上屏与导出工具链缺陷分列移交」。
