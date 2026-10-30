# T277 V2 同 run 落床锚点续证 · 报告 v2（附 v1 低电报告，2026-10-30 重跑）

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-30 · 状态：**FAIL（新缺陷实锤：T276 heavy 臂每帧 cold 导出阻塞主线程 >6s，系统 watchdog THREAD_BLOCK_6S 杀进程；回 T274 architect）**
- 被测包：**T276-v5-signed.hap**（SHA `e9c0b37c…`，源 `5a06b06`）——同 v1
- 证据：`test/reports/t277/`（cold_raw{,2,3,4}.txt 四轮分片、reassembled{,2,3,4}/、tb18387_full.log 阻塞日志、tb18387_stack.txt 栈摘录）

## 前情修正（v1 勘误）

v1 报告将进程死亡归因"电量 3% 低电 Doze 冻结"。**2026-10-30 重跑推翻该归因**：设备实际电压 4.18V（对应真实电量约 74%；BatteryService 的 `batteryLevel:3` 是冻结期缓存未刷新），进程 CPU TIME 正常推进（00:01:08→00:01:13），Doze 未介入。**真实死因见下。**

## 新缺陷实锤：T276 heavy 臂每帧 t276Cold 导出阻塞主线程

**故障签名（4 个 run 完全一致）**：pid 16474/18387/22123 等全部在 frame≈30–42（accepted≈0.3–1.2s 物理秒）死亡；`AMSI9791 Kill faultApp errorName: THREAD_BLOCK_6S`、`exit with signal:9`。

**阻塞栈（THREAD_BLOCK_6S eventlog，两份独立样本一致）**：

```
#00 SharedSparseSpace::TryAllocateAndExpand  (JS heap 分配卡死)
#03 RTStub_CallRuntime / BuiltinStub_StringPadStartStwCopy
#06 at t276Export (diagnostics/t248/T276Export.ets)
#07 at t276Cold (physics/derived/P051DerivedDemoV2.ets)
#08 at tick (pages/P051DerivedV2Demo.ets)
```

主线程在 `t276Cold → t276Export` 中做**海量 JS 字符串分配/拼接**（SharedHeap 已用 11–15MB，GC count 72、maxPause 1944ms），单次事件处理超 6 秒 → watchdog `THREAD_BLOCK_6S` → signal 9。

**触发机制核对（源码）**：`P051DerivedDemoV2.ets` tick 内每帧调用 `t276Export`（`P013SceneController.ets:75`：`if(this.t266CapturePacked||AppStorage.get('t266heavy')===true) t276Export('PACK|'…)`）——**heavy 臂下 PACK/FRAME/STEP/LAND 全部每帧走 cold 分片导出**，每帧数千 console.info 分片（单 run 观测 11628–17098 行），主线程被导出工作淹没。

**归因（按卡交 T274 architect，tester 不修）**：T276 交接文档设计为"冷档（完成时封存）+正常帧固定阶段摘要"，但实现在 heavy 臂下把 **cold 导出变成了每帧路径**。修复方向供裁决：① t276Cold 仅在锚点/显式触发时执行（恢复交接文档语义）；② PACK 每帧导出改为环内采样+完成时一次性冷档；③ 至少把分片 console.info 移出主线程（worker）。

## 各锚点现状（四轮 run 汇总）

| 锚点 | 状态 |
|---|---|
| T267 缺陷 A（sinceFirstBirth 差值法） | ✅ 修复确认（静态核对 + 四轮运行无冻结现象） |
| 分片/重组工具链 | ✅ 真机 PASS（reassemble invalidLines=0 全四轮；COMPLETE 档含 SHA256：LAND/STEP/PACK/FRAME 共 12 份；MISSING_PART 如实 INCOMPLETE） |
| 首生基线/since≥8s | ❌ **BLOCKED（本缺陷）**：进程活不过 frame≈42（≈1.2s 物理秒），永远到不了 8s |
| LAND writer→床→publication→packed→像素 | ⚠️ 结构链齐备；PACK 床四量 0（T249/T267 同线）；单 run 存活窗口内无法触床 |

## 完整性账目（四轮）

| 轮 | pid | 分片行 | COMPLETE | INCOMPLETE |
|---|---|---|---|---|
| 1（昨 16474） | 16474 | 12017 | 3（LAND/STEP/PACK） | 2（T266_FRAME） |
| 2（昨 24189） | 24189 | 17098 | 7 | 1 |
| 3（今 18387） | 18387 | 11628 | 6 | 1 |
| 4（今 22123） | 22123 | 11755 | 7 | 1 |

全部 `invalidPhysicalLines=0`；每轮 T266_FRAME 主档均 MISSING_PART（进程死于导出中途）——与阻塞栈位置（t276Cold 导出 FRAME 大对象）自洽。

## 复现（交 architect/designer）

装 T276-v5 → `aa start --ps p051v2 1 --ps t266heavy true` → 等待 ≤60s → `hilog | grep THREAD_BLOCK_6S` 或 `AMSI9791 Kill faultApp errorName: THREAD_BLOCK_6S`；faultlog 栈见 `test/reports/t277/tb18387_full.log`。

## 边界（如实）

- 本卡所有锚点（首生基线/since≥8s/LAND→床→像素）因产品缺陷 NOT_RUN，不冒 PASS；PACK 床 0 如实。
- v1 低电归因作废（保留 v1 文件不回改，本报告勘误）；T273 的 Doze 阻断归因**不受影响**（该卡证据是 CPU TIME 冻结 + Doze hilog，非 THREAD_BLOCK）。
- 未改任何物理/阈值/导出参数。
