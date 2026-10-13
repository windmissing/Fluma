# P-003/P-005 主观效果测试报告

## 报告 #1 — 日期：2026-10-13，被测代码 git commit：UNKNOWN_NO_GIT_METADATA

> 基线性质：`BLOCKED / NOT_IMPLEMENTED`。本轮仅确认当前代码未实现 `MODE_WATERSAND`，未对 `EC-WS-01`~`EC-WS-08` 进行主观评分，避免编造结果。  
> 说明：当前工作区不存在 `.git` 目录，且本机 PATH 中无 `git` 命令，无法取得真实 git commit hash；为保留可复现标识，记录当前 `entry/src/main/ets` 源码树 SHA256 为 `df1422bf6605118d0baa1fd29ebe2d7f49088350c5fa56cfd9bb7514e2261d0a`。后续补齐 git 元数据后，应在报告中回填真实 commit hash。

### 1. 测试对象与环境

| 项目 | 记录 |
|---|---|
| 问题 | P-003 / P-005 |
| 用例 | `docs/effect-cases.md` 中 `EC-WS-01`~`EC-WS-08` |
| 源码状态 | `entry/src/main/ets/pages/SandCanvas.ets` 仅定义 `MODE_SAND=0`、`MODE_FLUID=1`、`MODE_HYBRID=2`、`MODE_LIQUID=3`，`MODE_COUNT=4`，未定义 `MODE_WATERSAND=4`；源码中不存在 `WaterSandField`、`wspreset`、`wscols`、`wsmixband`、`wssnapshot`。 |
| 构建结果 | 当前源码可构建出 unsigned HAP：`entry/build/default/outputs/default/entry-default-unsigned.hap`，SHA256 `d98f95b035dfffe22e0bebfe0c401902b991f4c45a9319d002773198ce39c93f`。 |
| 签名状态 | 当前工程签名配置在 hvigor 6.26.8 下因密码长度不足且可用 p12 缺少与 `entry-default.p7b` 匹配的 debug 证书私钥，未能生成当前源码对应的 signed HAP；因此无法直接安装当前源码构建产物。 |
| 安装实测 | 对 `entry-default-unsigned.hap` 执行 `hdc install -r`，设备返回 `code:9568320`、`no signature file`，确认当前源码构建产物无法直接安装运行。 |
| 运行取证 | 使用最近一次可安装的既有 signed 包 `entry-default-s77p-signed.hap`，SHA256 `5a011ea5599c736aea0bbf5565a710ea1c72d3a7efcc0b6fef9530ebf8e573c9`，在真机 `62T0225B10010020` 上以 `rendermode=4` 启动并采集截图与 hilog。该包与当前源码在 `modules.abc`、`sourceMaps.map` 上存在差异，因此仅作为“现有应用无水沙入口”的旁证，不作为主观评分依据。 |
| 运行证据 | `demo/effect/P005/p005_mode4_baseline.jpeg`、`demo/effect/P005/p005_mode4_panel.jpeg`、`demo/effect/P005/p005_mode4_layout.json`、`demo/effect/P005/p005_mode4_panel.json`、`demo/effect/P005/p005_mode4_startup_hilog.txt`。 |

### 2. 场景执行结果

| 场景 | 结果 | 说明 |
|---|---|---|
| EC-WS-01 初始混合注入后的静置分选 | BLOCKED / NOT_IMPLEMENTED | `MODE_WATERSAND` 与 `WaterSandField` 未实现，无法进入水沙场景。 |
| EC-WS-02 水面水平与倾斜响应 | BLOCKED / NOT_IMPLEMENTED | 同上。 |
| EC-WS-03 沙丘轮廓与安息角观感 | BLOCKED / NOT_IMPLEMENTED | 同上。 |
| EC-WS-04 重沙/轻沙分区与混合带 | BLOCKED / NOT_IMPLEMENTED | 同上。 |
| EC-WS-05 快速旋转与沙滑移 | BLOCKED / NOT_IMPLEMENTED | 同上。 |
| EC-WS-06 摇晃、悬浮、沉降与再堆积 | BLOCKED / NOT_IMPLEMENTED | 同上。 |
| EC-WS-07 180° 倒立与翻回 | BLOCKED / NOT_IMPLEMENTED | 同上。 |
| EC-WS-08 60fps 流畅度与长时稳定 | BLOCKED / NOT_IMPLEMENTED | 同上。 |

### 3. 结论

1. 当前代码未实现 `MODE_WATERSAND`，`EC-WS-01`~`EC-WS-08` 全部无法执行。
2. 本轮不给出任何主观评分，也不计算加权分；总体状态为 `BLOCKED / NOT_IMPLEMENTED`。
3. 现有应用运行旁证显示，`rendermode=4` 会被日志记录为 `renderMode=4`，但界面与源码均无水沙模式入口或分支，符合“入口不存在”的基线预期。
4. 需先实现 P-003 的 `MODE_WATERSAND`、`WaterSandField` 与相关 Want 参数，再按 `docs/effect-cases.md` 重新执行全部 8 个场景并追加正式评分报告。

### 4. 证据索引

- 源码检查：`entry/src/main/ets/pages/SandCanvas.ets:39-45`、`entry/src/main/ets/entryability/EntryAbility.ets:14-29`
- 运行截图：`demo/effect/P005/p005_mode4_baseline.jpeg`
- 面板截图：`demo/effect/P005/p005_mode4_panel.jpeg`
- 首屏布局：`demo/effect/P005/p005_mode4_layout.json`
- 面板布局：`demo/effect/P005/p005_mode4_panel.json`
- 启动日志：`demo/effect/P005/p005_mode4_startup_hilog.txt`
