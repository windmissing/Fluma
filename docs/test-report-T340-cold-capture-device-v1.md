# T340 冷采 captureMs 设备补测（P1-P3，T338 M3 闭环）（真机，完成——P1 FAIL 如实）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T331 冷采分段化交付（T284Cold enqueue 原子交接 + drain 惰性 digest）。依据：`docs/test-report-T338-v2-continuation-device-v1.md` §3（M3 NOT_RUN 移交）+ T341 包选择裁定（T338b 线并行不互阻，本卡独立 t284 生成树包）。

## 0. 包身份（T151 三件套）

- 生成树 `.codex/local-test/t340-v1`（`test/t276/generate.cjs` 全链含 `t284/apply.cjs`，t284-manifest budgets `captureMs:4` 在位）hvigor 全绿（BuildNativeWithNinja 12.7s + CompileArkTS 22.9s + PackageHap，SignHap 00303116 已知独立登记）→ 重打包 → localSign + verify-app **PASS**。
- **T340-signed.hap SHA256 `3b387fe6c4e290d58e3217a24afb6b4a6bda1b066cba58c3c9ee338662b10875`**。
- 启动参数：`--ps p051v2 1 --ps t266heavy true --ps t284captureframe 60`；pan swipe 注入触发出生/落床，SINCE_BIRTH_8S 等待 8s 后触发。

## 1. P1 captureMs ≤4ms：**FAIL（如实，不放宽）**

三锚点真机实测（`T284_INDEX` 行，captureMs 字段）：

| 锚点 | captureMs | status |
|---|---|---|
| MANUAL（frame 60） | **18.447ms** | INCOMPLETE / BUDGET_CAPTURE_MS |
| FIRST_BIRTH（frame 142/commit 286） | **20.626ms** | INCOMPLETE / BUDGET_CAPTURE_MS |
| SINCE_BIRTH_8S（frame 381/commit 769） | **34.231ms** | INCOMPLETE / BUDGET_CAPTURE_MS |

全部超 4ms 预算 4.6-8.6 倍；**T331 分段化预期的 memcpy 级 ≤2ms 未达成**。异常点：`rawBytes=0, parts=0, hashes=[]`（锚点窗口 payload 为空、零分片产出）却仍耗 18-34ms——锚点窗口内仍存在超预算工作（引用检查/字节算术实际遍历等），分段化目标未实现，归因移交 developer（本卡红线不改实现）。

## 2. P2 INCOMPLETE 规则不放宽确认：**PASS**

- 超预算样本全部 `status=INCOMPLETE, reason=BUDGET_CAPTURE_MS`，`rawBytes=0/parts=0/emitted=0/hashes=[]`——**零冷分片产出，fail-closed 正确**。
- INCOMPLETE 标记与 T284 原语义逐位一致（对照 T312 时代同样行为）；无任何放宽或降级。

## 3. P3 分段化前后对照：**有旧基线，对照完成**

- **前（T312 设备报告，分段化前）**：captureMs 19.3 / 46.5ms（两锚点）→ 同样 INCOMPLETE。
- **后（本轮）**：18.45 / 20.63 / 34.23ms（三锚点）。
- 对照结论：SINCE_BIRTH_8S 位置 46.5→34.23ms（**-26%**）——分段化有改善但未达预算；19.3ms 档位的锚点 18.45ms 几乎无变化。**全部样本仍超 4ms 预算，冷采质量未达设计目标**。

## 4. T338 M3 闭环

T338 §3 M3 由 NOT_RUN 转为 **FAIL**（本卡实测在案）——闭环完成。冷采质量属 TIERED 指标（预算 4ms）当前不达线，不阻主线（fail-closed 保证无坏数据落盘）；是否立修复卡交 manager 裁定。

## 5. 红线遵守

未改 T284Cold 分段化实现与 4ms 预算/INCOMPLETE 规则（生成树由 generate.cjs 原样产出）；未发现实现与 T323 分流稿语义矛盾——实现 INCOMPLETE 语义正确，仅性能预算未达成。

证据：`test/reports/t340/`（t340_m3_verdicts.json + t284_index.txt + layout0.json）；签包 `.codex/local-test/t318-sign/T340-signed.hap`。
