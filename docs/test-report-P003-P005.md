# P-003 / P-005 客观测试报告

## 2026-10-13 · RED 基线（功能未实现）

- **被测范围**：`WaterSandField` / `MODE_WATERSAND` / `WS_*` 测试钩子。
- **被测代码 git commit hash**：`UNAVAILABLE`。当前工作区没有 `.git` 元数据，无法取得真实 commit hash；本报告不虚构版本号。
- **测试性质**：测试先行 RED 基线。产品功能尚未实现，因此预期结果为 `BLOCKED / NOT_IMPLEMENTED`，不是通过。
- **产品代码修改**：无。
- **测试资产位置**：`test/artifacts/P003-P005-baseline-20261013/`。

### 1. 执行摘要

| 项目 | 结果 |
|---|---|
| `test/water_sand_check.py` | 可执行；无数据输入时正确输出 9 项 `BLOCKED`，退出码 `1` |
| `test/water_sand_pixel_check.py` | 可执行；占位帧正确输出 `BLOCKED`，退出码 `1` |
| `test/record_tc_ws.py --plan` | 可执行；成功输出 `TC-WS-01` 场景计划，退出码 `0` |
| `test/record_tc_ws.py --skip-install` | 可执行；因 Want 启动编排未实现输出 `BLOCKED`，退出码 `2` |
| `--self-check-defect` | 可执行；合成 §3.3 缺陷被正确判为 `FAIL`，退出码 `0` |
| 语法检查 | 3 个脚本 AST 语法全部通过 |

### 2. 逐用例基线状态

| 用例 | 当前状态 | 直接原因 | 证据 |
|---|---|---|---|
| TC-WS-01 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `driftWater/driftHeavy/driftLight` 与分段 `WS_EVENT` | `ws_check.json` |
| TC-WS-02 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `separationRatio/lowerHeavyShare/upperHeavyShare` | `ws_check.json` |
| TC-WS-03 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `t90Heavy/t90Light` | `ws_check.json` |
| TC-WS-04 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `maxSlopeDeg/maxCurvature` | `ws_check.json` |
| TC-WS-05 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `waterSlopeDeg/waterAmplitudePx` | `ws_check.json` |
| TC-WS-06 | `BLOCKED / NOT_IMPLEMENTED` | 缺水面斜率指标与 `tilt` 事件时间轴 | `ws_check.json` |
| TC-WS-07 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `invert180` 事件与 `phaseOrderScore` | `ws_check.json` |
| TC-WS-08 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `shake` 事件与 `suspensionFraction` | `ws_check.json` |
| TC-WS-09 | `BLOCKED / NOT_IMPLEMENTED` | 分选、安息角、悬浮、瞬移指标均未暴露 | `ws_check.json` |
| TC-WS-10 | `BLOCKED / NOT_IMPLEMENTED` | 缺 `bedDrift` 或带时间戳的 `WS_SNAPSHOT.sandTop` | `ws_check.json` |
| TC-WS-11 | `BLOCKED / NOT_IMPLEMENTED` | `WS_FRAME` 样本数为 `0`，要求至少 `100` | `ws_check.json` |
| TC-WS-12 | `BLOCKED / NOT_IMPLEMENTED` | 模型面积占比与颜色分类钩子均未实现 | `ws_pixel_check.json` |

**结论**：12/12 用例当前均不能形成有效客观证据，全部处于 RED 基线；这不是放宽阈值后的通过结果。

### 3. 执行命令与退出码

```text
python test/water_sand_check.py --input test/artifacts/P003-P005-baseline-20261013/empty.hilog --all --report test/artifacts/P003-P005-baseline-20261013/ws_check.json --markdown test/artifacts/P003-P005-baseline-20261013/ws_check.md
exit_code=1
```

```text
python test/water_sand_check.py --self-check-defect --report test/artifacts/P003-P005-baseline-20261013/self_check_defect.json --markdown test/artifacts/P003-P005-baseline-20261013/self_check_defect.md
exit_code=0
```

```text
python test/water_sand_pixel_check.py --frames test/artifacts/P003-P005-baseline-20261013/placeholder-frame.txt --hilog test/artifacts/P003-P005-baseline-20261013/empty.hilog --calibration test/artifacts/P003-P005-baseline-20261013/calibration.json --report test/artifacts/P003-P005-baseline-20261013/ws_pixel_check.json --markdown test/artifacts/P003-P005-baseline-20261013/ws_pixel_check.md
exit_code=1
```

```text
python test/record_tc_ws.py --case TC-WS-01 --plan
exit_code=0
```

```text
python test/record_tc_ws.py --case TC-WS-01 --skip-install --output-dir test/artifacts/P003-P005-baseline-20261013
exit_code=2
```

### 4. 脚本自身发现

1. **像素脚本 BLOCKED 原因不完整**：`water_sand_pixel_check.py` 在模型面积占比缺失时只提示 `model area shares are unavailable`，没有同时提示“渲染颜色分类也未实现”。后续应把两类缺失钩子都写入 `note`，避免误判为只差模型侧数据。
2. **分段统计仍未闭环**：`water_sand_check.py` 已有 `build_segments()` 骨架，但事件缺少时间戳，无法做逐分段守恒统计；当前只能输出全序列 `BLOCKED`。
3. **报告目录 ACL 限制**：`test/reports/P003-P005-baseline-20261013` 创建被拒绝，基线产物改存于 `test/artifacts/P003-P005-baseline-20261013/`。这是环境问题，不是脚本判定缺陷。
4. **Git 元数据缺失**：当前工作区无 `.git/HEAD`，无法记录 commit hash。后续若仓库元数据恢复，需要补充真实 hash。

### 5. 后续动作

- P-003 完成实现后，优先补齐 `WS_METRIC/WS_EVENT/WS_FRAME/WS_SNAPSHOT` 与 `MODE_WATERSAND`。
- 重跑 TC-WS-01..TC-WS-12，要求全部从 `BLOCKED / NOT_IMPLEMENTED` 转为可测，且不得降低既有阈值。
- 修正像素脚本的 BLOCKED 说明，使模型占比与颜色分类缺失同时可见。
