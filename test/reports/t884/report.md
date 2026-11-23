# T884 PC 首轮干沙长窗与基础渲染实测

- 任务：T884 PC 首轮干沙长窗与基础渲染实测
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：`.codex/local-test/t872-release.exe`（`6ec0ec5c…`，T876 同身份工件）；三 SPIR-V 沿原 SHA
- T879 已结束，本机执行；同场景双源与 D1 四臂修订工件不在本卡范围（卡面明示）
- 全程 GPU 驻留、shader 渲染、无 Canvas 回退

## 分项目标结果

| 目标 | 状态 | 证据 |
|---|---|---|
| 1 高度场+粒子·入料 | **PROVEN** | longwindow：20s 持按受理 139681 Q；721 帧持续呈现（25fps）；sourceCommit=1、generation 2→722 |
| 3 触摸掉沙并累积·释放落床与持续存在 | **PROVEN** | dual_release：按住期 138 帧 lease 库存 128878、activeSand=1（粒子活跃）；**UP 后 153 帧库存恒 128878、activeSand=0**（释放停料、床留存）；291 帧全守恒 stock==admittedTotal |
| 2 shader 窗口 | **PROVEN** | 721+291 windowFrame，单 run 身份、presentVk 全 0；interactive `M1_QUANTUM_WINDOW` 帧（m1_full_scene 管线） |
| 4 堆积沙与粒子同帧 | **PARTIAL** | m1SceneLease 逐帧携带 sandStockQ/sandLive 与 windowFrame 同帧；**逐像素配对无接口**（PC 仅单采样点像素 0xff525355）——记实际缺口 |
| 5 空箱触摸 | **GAP** | 空闲探针：无输入 → POINTER_READY 无限等待，0 帧 0 量子。当前 M1 设计首帧必须 SOURCE；空箱视图不可达——记实际缺口（需设计裁定：干沙诊断帧或零发射 SOURCE） |
| 6 compute 长窗·根/重显/呈现 | **PROVEN** | 28.7s 连续窗：721 个顺序量子根全 PRESENT_QUEUED+干净 RETIRED；25fps 持续（低于 60Hz——如实记录，不设门） |

## 实际缺口登记（gravity/sampling 等）

1. **重力接口**：交互路径无运行时重力开关（REST config c[24]/c[25]=0 固定）；fixture 路径有重力字段但非交互窗口。
2. **采样接口**：脉冲节拍固定 10ms（WM_TIMER），无运行时采样控制。
3. **空箱视图**：见目标 5。
4. **帧率 vs 60Hz**：UHD 630 上 127 派发量子约 25fps；60Hz 不可达（记录，不门控）。

## 运行明细

- `longwindow.jsonl`：单源 20s 持按+8s 释放（721 帧/28.7s）
- `dual-release.jsonl`：双源模式 6s 持按+6s 释放（291 帧、lease 全程、UP 前后对比）
- `dual-lease.jsonl`：双源模式 12s 持按（ROOT_DEADLINE 早止，负证保留）
- `empty-idle.jsonl`：空箱探针（0 帧）

## 交付

- 证据：`test/reports/t884/`（4 轮 JSONL + summary.json）
