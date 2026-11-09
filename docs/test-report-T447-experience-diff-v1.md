# T447 · GPU 显式入口双 profile 生产差分报告

2026-11-09，tester，P5，不使用真机。消费 T438 实际生成页（commit ac4abad，两 profile 隔离工程重新生成）与 T439 25 项基线，对显式 GPU 沙水入口做生产差分。

## 交付

- `test/t447/experience_diff.cjs` — 独立差分执行器（verdict PASS，24 agreed / 0 diverged）
- `test/reports/t447/t447_experience_diff.json` — 判定原件
- 隔离工程 `.codex/local-test/t447-regular` / `t447-experiment`（T438 generate.cjs 重新生成，frozen-source 0228a7d）

## 差分对象

T438 实际生成页 `GpuSandWaterExperience` 的真实接线（ExperienceFrame pulse 循环、selectProgress/touch/生命周期、GpuM1DemoController 输入路径），native port 为可观测桩。期望值来自 T439 基线（t439_baseline.json）——**T439 的 5 项 NOT_IMPLEMENTED 在 T438 到件后本卡全部可核**。

## 双 profile 差分结果（每 profile 12 项，共 24）

| 项 | regular | experiment |
|---|---|---|
| I20 V2 unsupported | `PROGRESS_V2_UNSUPPORTED` 且不触发新空 run ✓ | —（实验 profile 支持） |
| I21 V2 冷选新 run | — | `selectV2→newRun` 次序 ✓，tick 后 progress='V2' ✓ |
| I22 无 snapshot 导入 | 静态核生成页无 loadSnapshot/configureM0 ✓ + 显式新空 run ✓ | 同 ✓ |
| I23 内存不足明确拒因 | FAULT code5 显示 `MEMORY_PROFILE_UNSUPPORTED` ✓，FAULT 中 DOWN 拒 `NOT_READY` 零包 ✓ | 同 ✓ |
| I25 UP 后 J 继续 | DOWN 期间 tick 持续入队；UP 后零新 offer（已准入 J 由 GPU 侧继续）✓ | 同 ✓ |
| I01 down 恢复 | UP 后重新 down 恢复入队（首个 tick 记 lastNs、次 tick 入队语义核过）✓ | 同 ✓ |
| I08 时钟 GAP | delta>33.3ms → `CLOCK_GAP_NO_CATCHUP` ✓ | 同 ✓ |
| I18 重置新空 run | reset→resets+1 ✓ | 同 ✓ |

## 与开发 page_host 16/0 的关系

开发自测 16 项复跑确认仍绿；本卡期望由 T439 基线独立给出并补三类开发未覆盖的断言：I20 拒 V2 不触发 reset、I25 UP 后零新 offer 的精确包计数、I23 FAULT 后 DOWN 零副作用。初轮 2 处 FAIL 为差分器自身断言点错（down 后首 tick 仅记 lastNs 不入队），核实后修正——无产品分歧。

## 边界

- 本卡为页面接线/native port 合成域；真机触摸、系统呈现与体验归 T448（effector）
- 拒因与 native 能力分支的包级核验见 T438 `check_signed_profiles.py` 与两签包回执（regular `87692250...`、experiment `10cc8e83...`）
- 设备串行：T410/T408 先行，T448 等设备窗口
