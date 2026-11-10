# T466 · GPU 实际 V2 首错与 session 恢复真机取证（v1）

2026-11-10，tester。设备 62T0225B10010020，T456-experiment-finalized 签包 SHA256 96ae7ca2a0d78dc8f47bc0796eb7cee96f67236d9adc37fb427821778a1a9310（与卡面一致；注意 T461 的 T452 包同 bundle 覆盖过，取证前已重装回 T456 包并核 `T456_EXPERIMENT` 标签）。

## 交付

`test/reports/t466/`：4 份 `T451_GPU_BOOT_PROBE_V1` 探针原件 + 截图 + 布局。全程低频导出（4 次），无轮询。

## 取证链（pid 57914，dropped 0/0）

| 代 | boot | epoch | actualClock | viewport | readyEpoch | session | 首错 |
|---|---|---|---|---|---|---|---|
| attempt0 | 1 | 2 | V1 | 0×0 | 0 | 1 | SURFACE_CREATED（0×0 窗口代） |
| attempt1 | 2 | 3 | V1 | 1280×1609 | 3 | 1 | V1_ANCHOR_CLOCK_GAP（delta 41.3ms） |
| attempt2 | 3 | 5 | **GRS_M1_PROGRESS_CLOCK_V2** | 1280×1564 | **0→5** | **1→2** | **V2_PULSE_CLOCK_GAP** |

### 实际 V2 核（卡面第一项）

- 路径：体验页 `T438_EXPERIMENT_V2 · K192/Kw128` capability 确认 → 点"实验 V2"→ `PROGRESS_V2_NEXT_RUN` + reset → attempt2 `actualClock=GRS_M1_PROGRESS_CLOCK_V2`（探针 `a.v2` 直接记录，非页面文案）。
- **V2 首错**：`V2_PULSE_CLOCK_GAP`，anchor a=3694656579281110，超时点 b=3694695663084078，**delta=39,083,802,968ns（39.08s，pulse nativeGap 域）**——与 V1 的 anchor 域 41.3ms 不同域（V2 无 anchor，pulse 断流判定），两域独立分列，不互推。

### session 恢复核（卡面第二项，回 T410 v3 发现）

前后台负控（Home→resume）：transitions 序列 `ACTIVE_BACKGROUND → ACTIVE_RESUME → PAGE_SESSION_BEGIN`，attempt2 恢复后 **session 1→2、readyEpoch 0→5==epoch（READY 短暂达成）**——证实 V2 的 `progress.open` 恢复路径确实经 `endPageSession`+`beginPageSession` 闭合（T410 v3 发现的恢复手段成立）。恢复后 pulse 再次断流，CLOCK_GAP 复发（`latest=T398_PRESENT_REVOKED`），touch 被 NOT_READY 拒（准入 Q 0，fail-closed 正确）。

### 新空场负控

"新空场 V1"重置：attempt 代际滚动（boot1→3）、XCOMPONENT_SIZE/CALLBACK_WINDOW_REFERENCE/SURFACE_CHANGED 全程记录、dropped=0。

## 结论（回 T451/T464）

1. **实际 V2 与 V1 首错不同域**：V1_ANCHOR_CLOCK_GAP（anchor 域，~40ms）vs V2_PULSE_CLOCK_GAP（pulse nativeGap 域，39s——设备 GL 长卡顿）。V2 首错量大说明断流发生在 shader 编译（40.9s）窗口内，与 T451 归因一致。
2. **V2 无运行时自愈**：READY 仅在 PAGE_SESSION_BEGIN 后短暂达成，GAP 复发后无二次恢复路径——生产恢复手段缺失实锤（唯一恢复=页面级 session 重建）。
3. viewport V2(1280×1564) 与 V1(1280×1609) 不同，两卡数据不可配对，已分列。
4. 未从 40.9s 编译推 49ms 首错同因（两域独立取证）。

T457 未行使的 V2 与背景负控范围本卡补齐完毕。
