# T376 续（T384 探针）· GPU 批尾停顿分段定位 — 停顿在 44 Flush GL 调用内部，GAP 撤销结构性证实

- 承接：T382 归因（批尾停顿/上屏断链）+ T384 分段探针签包 `d5dce2a52b217cc856f320619cdef166b00262a11898995a12b269367370a7d1`（卡面确认解锁）。合同：同设备先关探针复现、再开探针定位；每轮原 T306 冷导出 + 签包 SHA 登记。
- 证据：`test/reports/t376b/`（21 文件，SHA 见 `t376b_verdicts.json`）。SHA 注册沿用 4×16 hex 分段注入法（零丢字，dumpLayout 逐位核对）。

## 轮次

### 轮1 · 探针关（复现）— pid 48621，epoch2/3/4 三 run

- 旧字典 339 行 COMPLETE：批1 wall **0.531/0.526/0.348ms**；批2 尾停顿 **43.842/48.405/61.338ms** —— T376 现象逐 run 复现。探针行零（关=旧字典 1–42，契约一致）。
- statusAfter：`PAUSED_CLOCK/CLOCK_GAP`、acceptedCount=0、commit=0（与 T376/T382 一致）。

### 轮2 · 探针开（定位）— run5/epoch4

T384_STAGE50_V1 新字典行决定性证据：

| 行 | 读数 | 含义 |
|---|---|---|
| 43/44 批1 | beforeFlush=190.0µs、**flushNs=22,812**、afterFlush=469ns | 首 flush 22.8µs 正常 |
| 43/44 批2 | beforeFlush=252.97µs（批内 op 干净）、**flushNs=62,078,907**、afterFlush=1,718ns | **62.08ms 全部在 44 Flush GL 调用内部** |
| 45 | code=16（OK_SUBMITTING）、probeStart→End=62.34ms | ContinueSubmit 全程=批2 含 flush |
| 49 | code=0、probeA=anchor 原始 ns、**probeB=0** | GAP 撤销前记录，**撤销时 accepted=0 精确证据** |
| 46/47/48/50 | 缺席 | root 未封尾即被 GAP 撤销（T382 归因契约一致） |

### 轮3 · 探针开 + 加重注入 — 两个提交周期

- 周期1 flush=**62.08ms**；周期2 flush=**38.49ms**（驱动预热后下降 38%）——**仍 > GAP 窗 33.33ms** → 两次均 GAP 撤销（gapCount=2），无 46–50 行。

## W 记账（T382 合同分别记账）

| 判据 | 结果 |
|---|---|
| **W2 首批 ≤8ms** | **PASS**（批1 0.35–0.53ms；探针 flush 22.8–25.9µs） |
| **W3 各批 ≤1ms** | 批1 PASS / **批2 FAIL**——停顿**精确定位在 44 Flush GL 调用内部**（beforeFlush 253µs 干净、afterFlush 1.7µs 干净） |
| **W4 双源同 token 上屏** | **BLOCKED（结构性）**——第二次 flush 固定税 38–62ms > GAP 窗 33.33ms → root 永不封尾 → 永无 accepted/Poll/token；stage49 probeB=0 证明撤销时零接受 |
| **W5** | 时间线三轮全 COMPLETE（339 行等），K5 保持 |

## 结论（T382 归因真机证实 + 量化）

1. **批尾停顿 = 第二次 flush 调用的驱动固定税**（62.08/38.49ms 探针内证；轮1 旧字典 43.8–61.3ms 互证），批内 op 与 flush 前后段全部干净。
2. **上屏断链 = 工程参数冲突**：flush 税（38–62ms，预热后仍 38ms）> GAP 窗 33,333,333ns → **结构性必然 GAP 撤销**。修复方向不是 Draw/材质/GLSL——是 flush 税消除或 GAP 窗语义（architect 裁定；T382 已排除猜测性修复）。
3. 驱动预热使第二次 flush 税从 62→38ms（-38%），但仍不足以越过 GAP 窗——单纯加注入不能修复。
4. GAP 撤销样本按契约拒判呈现（本轮全部样本属此类，如实登记）。

## 边界

未改业务/native；探针开关按卡使用（先关复现再开定位）；每轮 T306 冷导出 + SHA 登记；GAP 撤销样本不冒呈现成功；PC 43.66s 未外推。
