# T457 · GPU 显式入口同包首错真机配对（v1：两臂首错链已取，回 T451）

2026-11-10，tester。设备 62T0225B10010020，T456-experiment-finalized 签包（SHA256 96ae7ca2…，验签回执 test/reports/t257/T456-*-finalized.json）。K192/Kw128。

## 交付

`test/reports/t457/`：三份 `T451_GPU_BOOT_PROBE_V1` 探针原件 + 菜单布局 + Want 臂截图。

## 两臂配对结果

| 项 | Want V1 臂（--pb grsm1 true 直达） | 菜单 V1 臂（Index"进入 GPU 沙水体验"） |
|---|---|---|
| PID | 31861 | 33895 |
| actualClock | GRS_M1_PROGRESS_CLOCK_V1 | GRS_M1_PROGRESS_CLOCK_V1 |
| K/Kw | 192/128 | 192/128 |
| 首错 attempt0 | SURFACE_CREATED（窗口 0×0 时代，a=1,b=2） | SURFACE_CREATED |
| 首错 attempt1 | **V1_ANCHOR_CLOCK_GAP** delta=49,728,437ns（anchor=…959498766, now=…009227203） | **V1_ANCHOR_CLOCK_GAP** delta=40,226,250ns |
| viewport | 1280×1471（诊断页含额外面板） | 1280×1609（体验页） |

## 新空场负控（同进程第三代）

attempt2（epoch3, surfaceGeneration=1, windowId=1）：无首错、`terminal=false` 进行中、latest=SUBMIT_OBSERVER a=26、dropped=0——代际隔离正确，旧代终态不被覆盖，新代独立计数。

## 结论（回 T451 归因）

两入口首错**一致为 V1_ANCHOR_CLOCK_GAP**：V1 anchor 建立后下一次 owner 检查点 delta 超 33.3ms GAP（49.7ms / 40.2ms），非 EGL_CREATE 失败、非 SURFACE 生命周期故障（SURFACE_CREATED 只是 0×0 窗口时代的代际记录）。与 T448 hilog `EglWrapperHookLayer init Failed` 及 shader 编译 40.9s 证据合并：首错=GPU 提交/编译卡顿超过 GAP 阈值的**环境时序**失败，生产 fail-closed 正确。T448 体验阻断与 T410/T437 CLOCK_GAP 无法恢复同域同因。

## 未行使（如实）

- experiment V2 切换与 `actualClock` V2 核：设备在 V1 已 GAP，V2 切换只会复合同一环境失败，本轮未消耗。
- 后台/尺寸变化负控、attempt 六代溢出：探针机制已由 attempt1→2 代际滚动自证，未重复构造。

T448 体验仍按原卡另行办理；本卡只交证据，不改判 T448 结论。
