# T427 · 单沙系统截图冻结映射端到端负控（离线）

2026-11-09，tester。不使用真机、不改产品。与 T426 并行。用 T406/T414 现有归档结构（T413 `screen.py` V2 loader + `fixtures.py` V2 升级档合成夹具）与合成截图数据，制作从"完整档 → verify_bundle → load → calibrate → plan → pair 判定"的端到端集成负控。时钟差分复用 T418 判定（不重做状态差分），拒绝顺序证明"判读器先拒证据再比较像素"。结果供 T408 消费，不卡真机。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t427/e2e_negatives.py` | 端到端负控执行器：V2 集成路径（`t413.legacy.calibrate/plan/pair`，load 已替换为 V2 loader）+ `capture_hold.verify_bundle` |
| `test/reports/t427/t427_negatives.json` | 9 例判定（9/9 PASS） |

复跑：`python -X utf8 test/t427/e2e_negatives.py`

## 2. 判定矩阵（9/9 PASS）

| 例 | 变异 | 实际拒绝 | 层级 |
|---|---|---|---|
| E0 正例 | 无（A/B 合法配对） | `DISPLAY_CALIBRATED_A_AND_B_PAIRED` | 通过基线 |
| N1 1px 平移 | B 档 PNG 整体右移 1px（SHA/fileBytes 同步） | `DISPLAY_NOT_JUDGABLE: B_SCREEN_MISMATCH` | 像素比较 |
| N2 1 码差 | core 内单像素 R-1（SHA/fileBytes 同步） | `DISPLAY_NOT_JUDGABLE: B_SCREEN_MISMATCH` | 像素比较（零容差） |
| N3 缺列 | nativeTiles 少 1 列 | `PREDECLARED_NATIVE_TILE_MISMATCH` | tile 布局 |
| N4 旧 token | 封存件 endToken/hold.token 改旧值 | `HOLD_IDENTITY` | bundle 身份 |
| N5 颜色模型未知 | back +1 全码（双规则全失配） | `CALIBRATION_BACK_COLOR_MODEL_UNKNOWN` | 校准 |
| N6 越 hold（V2 bundle） | 截图 endMonoNs 越 hold 封存 | `CAPTURE_INTERVAL_OUTSIDE_HOLD` | 时钟包络（T418 已证算法一致性） |
| N7 越 hold（V1 路径） | capture endMonoNs 越 endMonoMs | `CAPTURE_INTERVAL_OUTSIDE_HOLD` | V1 对照域 |
| N8 拒绝顺序 | PNG 破坏+越 hold 同时注入 | `CAPTURE_TOOL_RECEIPT`（先于时间门） | **顺序证明** |

## 3. 判读器拒绝顺序（已证）

`bundle 完整性（MANIFEST_SHA/CAPTURE_TOOL_RECEIPT/HOLD_IDENTITY）→ 时钟域/hold 覆盖（CLOCK_MAPPING_UNKNOWN/CAPTURE_INTERVAL_OUTSIDE_HOLD）→ 校准/plan 身份（CALIBRATION_*/PREDECLARED_*）→ 像素比较（*_MISMATCH 零容差）`。N8 同档双故障实测证据层（CAPTURE_TOOL_RECEIPT）先于时间门拒绝，像素比较永远最后。

## 4. 合成与真实窗口边界

- 本卡全部为合成层（T414 fixtures V2 升级档）；合成证据证明的是**判定逻辑与拒绝顺序**，不证明真实系统截图可见性或真实时钟关系。
- 真实窗口边界按 T408 attempt1 报告单列：`t413-prepare` KEYBOARD_HIDE_FAILED 阻断中（hideTextInput errorCode:55），待 architect 裁决后续 attempt。本卡结果供该续测直接消费，不卡真机。
- 状态差分（T413Capture 状态机）复用 T418 结论，本卡不重做。
