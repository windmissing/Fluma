# P-006 主观测试证据记录模板（EC-OL-01~03）

> 用途：逐轮记录 P-006 主观测试环境、输入、证据、硬门槛与分数。本模板不生成任何默认结论；未观察、未记录、缺证据的项必须如实标注。

## 1. 测试对象

| 项目 | 记录 |
|---|---|
| 问题 | P-006：场景一油水分层与容器动态主观效果缺口 |
| 用例 | `EC-OL-01`、`EC-OL-02`、`EC-OL-03` |
| 技术路线 | `<route-slug>` |
| 测试日期 | `<YYYY-MM-DD>` |
| 被测 commit | `<full-git-commit-hash>` |
| 可运行入口 | `<page-or-command>` |
| 启动配置 | `rendermode=3`，`testmode=true`，`forceinvert=false` |
| 等价映射 | `<none-or-detail>` |
| 颜色映射 | 重相 `<color>`；轻相 `<color>` |
| 评分员 | `<name-or-role>` |
| 报告文件 | `docs/effect-report-P006-<route-slug>.md` |

## 2. 环境 JSON

每个场景每轮均必须保存一份独立的 `environment.json`。以下为合法 JSON 结构示例，其中的具体取值仅用于展示结构；实际测试文件必须逐字段替换为实测值，禁止提交示例值、空值、空字符串、`null` 占位或 `<...>` 占位：

```json
{
  "schema_version": 1,
  "problem": "P-006",
  "route_slug": "route-liquid-baseline",
  "case_id": "EC-OL-01",
  "test_date": "2026-10-15",
  "git_commit": "1234567890abcdef1234567890abcdef12345678",
  "package_sha256": "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef",
  "entry_point": "npm run demo:liquid",
  "device_model": "Tester Device 1",
  "os_version": "OS 1.0",
  "screen_resolution": "1920x1080",
  "refresh_rate_hz": 120,
  "screen_brightness": "80%",
  "performance_mode": "high-performance",
  "recording_fps": 120,
  "recording_software": "Recorder 1.0",
  "background_state": "unrelated-apps-closed",
  "startup_params": {
    "rendermode": 3,
    "testmode": true,
    "forceinvert": false
  },
  "sensor_takeover_disabled": true,
  "param_mapping": "none",
  "phase_colors": {
    "heavy": "#0F4C81",
    "light": "#F4C542"
  },
  "evaluator": "effector-01",
  "round": 1,
  "video_file": "route-liquid-baseline/2026-10-15/EC-OL-01/round-01/video.mp4",
  "screenshot_files": [
    "route-liquid-baseline/2026-10-15/EC-OL-01/round-01/T0.png",
    "route-liquid-baseline/2026-10-15/EC-OL-01/round-01/T5.png",
    "route-liquid-baseline/2026-10-15/EC-OL-01/round-01/T10.png",
    "route-liquid-baseline/2026-10-15/EC-OL-01/round-01/T20.png",
    "route-liquid-baseline/2026-10-15/EC-OL-01/round-01/T30.png"
  ],
  "event_log_file": null
}
```

### 2.1 字段校验与必填项

| 字段 | 必填 | 校验规则 |
|---|---|---|
| `schema_version` | 是 | 固定为整数 `1`。 |
| `problem` | 是 | 固定为字符串 `P-006`。 |
| `route_slug` | 是 | 非空字符串，仅允许小写字母、数字和连字符，且与报告文件名一致。 |
| `case_id` | 是 | 枚举：`EC-OL-01`、`EC-OL-02`、`EC-OL-03`；每轮 JSON 只能记录一个场景。 |
| `test_date` | 是 | 合法日期，格式 `YYYY-MM-DD`，与实际执行日期一致。 |
| `git_commit` | 是 | 40 位或 64 位十六进制 commit hash；不得使用 `HEAD`、短 hash 或分支名。 |
| `package_sha256` | 是 | 64 位十六进制 SHA-256；仅当无可打包产物时允许 `null`，必须在报告中说明原因。 |
| `entry_point` | 是 | 非空可运行页面或命令，且与本次实际启动方式一致。 |
| `device_model`、`os_version`、`screen_brightness`、`performance_mode`、`recording_software`、`background_state`、`param_mapping`、`evaluator` | 是 | 均为非空字符串；`param_mapping` 无等价映射时填写 `none`。 |
| `screen_resolution` | 是 | 格式为 `<width>x<height>`，宽高均为正整数。 |
| `refresh_rate_hz`、`recording_fps` | 是 | 正数；建议 `recording_fps` 不低于 `refresh_rate_hz`，低于时必须在报告中说明。 |
| `startup_params` | 是 | 对象必须且只能包含 `rendermode=3`、`testmode=true`、`forceinvert=false`。 |
| `sensor_takeover_disabled` | 是 | 必须为布尔值 `true`；若实际无法关闭传感器接管，本场景为 BLOCKED。 |
| `phase_colors` | 是 | 必须包含非空 `heavy`、`light` 两个字段，建议使用 `#RRGGBB`，且两者不同。 |
| `round` | 是 | 整数 `1` 或 `2`；Round 3 若执行，填写 `3`。 |
| `video_file` | 是 | 相对仓库根目录的非空路径，文件必须存在且可播放。 |
| `screenshot_files` | 是 | 非空相对路径数组，文件必须存在；至少覆盖该场景模板规定的截图点。 |
| `event_log_file` | 条件必填 | `EC-OL-03` 必须为存在的相对路径；`EC-OL-01`、`EC-OL-02` 必须为 `null` 或省略。 |

### 2.2 缺失与不可校验时的 BLOCKED 口径

- 上述任一必填字段缺失、类型错误、格式非法、为空字符串、为占位值，或场景条件字段不满足时，该场景该轮判为 `BLOCKED`，不得评分、不得推断或补写主观结论。
- `git_commit`、`package_sha256`、`route_slug`、`case_id`、`entry_point`、`startup_params`、`sensor_takeover_disabled`、`phase_colors` 与实际运行记录不一致，或无法回放/复核时，该轮判为 `BLOCKED`。
- `video_file`、规定的 `screenshot_files`、`EC-OL-03` 的 `event_log_file` 缺失、损坏、不可读或无法校验存在性时，该轮判为 `BLOCKED`。
- 因环境缺失导致无法执行测试时，同样记录 `BLOCKED`，并在报告“BLOCKED 原因”中写明缺失项、影响范围和可复测条件；不得把 BLOCKED 计为 0 分，也不得折算为 FAIL。

## 3. 场景记录

### 3.1 EC-OL-01　静置水平界面

| 项目 | Round 1 | Round 2 | Round 3（可选） |
|---|---|---|---|
| 执行时间 |  |  |  |
| 录像文件 |  |  |  |
| 规定截图 | `T0/T5/T10/T20/T30` | `T0/T5/T10/T20/T30` | `T0/T5/T10/T20/T30` |
| 100% 稳态检查 |  |  |  |
| 400% 稳态检查 |  |  |  |
| 触发硬门槛 | `NONE / HG-OL-__` | `NONE / HG-OL-__` | `NONE / HG-OL-__` |
| 硬门槛时间码 |  |  |  |
| `U-OL-03` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |
| `U-OL-04` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |

| 维度 | Round 1 | Round 2 | Round 3 | 中位数 | 变化说明 |
|---|---:|---:|---:|---:|---|
| 分层可读性 |  |  |  |  |  |
| 界面平滑度 |  |  |  |  |  |
| 稳定流畅度 |  |  |  |  |  |

主观观察记录：

- 分层与相序：
- 界面形态：
- 稳定性：
- 亮点：
- 槽点：

### 3.2 EC-OL-02　摇晃暂时混合与静置恢复

| 项目 | Round 1 | Round 2 | Round 3（可选） |
|---|---|---|---|
| 执行时间 |  |  |  |
| 前置稳态确认 |  |  |  |
| swipe 次数 | 10 | 10 | 10 |
| swipe 总时长 | 6.0s | 6.0s | 6.0s |
| 输入偏差 |  |  |  |
| 录像文件 |  |  |  |
| 混合段截图 | `M1~M6` | `M1~M6` | `M1~M6` |
| 恢复段截图 | `T+0/+2/+4/+10/+30` | `T+0/+2/+4/+10/+30` | `T+0/+2/+4/+10/+30` |
| 触发硬门槛 | `NONE / HG-OL-__` | `NONE / HG-OL-__` | `NONE / HG-OL-__` |
| 硬门槛时间码 |  |  |  |
| `U-OL-02` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |
| `U-OL-03` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |
| `U-OL-04` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |

| 维度 | Round 1 | Round 2 | Round 3 | 中位数 | 变化说明 |
|---|---:|---:|---:|---:|---|
| 分层可读性 |  |  |  |  |  |
| 动态自然度 |  |  |  |  |  |
| 恢复可信度 |  |  |  |  |  |
| 稳定流畅度 |  |  |  |  |  |

主观观察记录：

- 摇晃混合：
- 恢复过程：
- 液滴/碎块可信度：
- 亮点：
- 槽点：

### 3.3 EC-OL-03　180° 倒立与翻回交换

| 项目 | Round 1 | Round 2 | Round 3（可选） |
|---|---|---|---|
| 执行时间 |  |  |  |
| 正立稳态确认 |  |  |  |
| `invert180()` 开时间码 |  |  |  |
| 倒立稳定时间 |  |  |  |
| `invert180()` 关时间码 |  |  |  |
| 翻回稳定时间 |  |  |  |
| 未稳定处理 |  |  |  |
| 录像文件 |  |  |  |
| 倒立段截图 | `before/+0.2s/+1s/+2s/+4s/stable` | `before/+0.2s/+1s/+2s/+4s/stable` | `before/+0.2s/+1s/+2s/+4s/stable` |
| 翻回段截图 | `before/+0.2s/+1s/+2s/+4s/stable` | `before/+0.2s/+1s/+2s/+4s/stable` | `before/+0.2s/+1s/+2s/+4s/stable` |
| 事件日志 |  |  |  |
| API 合规 | `invert180()` / 绕过 API 私改数组 | `invert180()` / 绕过 API 私改数组 | `invert180()` / 绕过 API 私改数组 |
| 触发硬门槛 | `NONE / HG-OL-__` | `NONE / HG-OL-__` | `NONE / HG-OL-__` |
| 硬门槛时间码 |  |  |  |
| `U-OL-01` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |
| `U-OL-03` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |
| `U-OL-04` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` | `PASS / UNRESOLVED_FAIL` |

| 维度 | Round 1 | Round 2 | Round 3 | 中位数 | 变化说明 |
|---|---:|---:|---:|---:|---|
| 分层可读性 |  |  |  |  |  |
| 界面平滑度 |  |  |  |  |  |
| 动态自然度 |  |  |  |  |  |
| 恢复可信度 |  |  |  |  |  |
| 稳定流畅度 |  |  |  |  |  |

主观观察记录：

- 倒立交换：
- 翻回交换：
- 过渡连续性：
- 最终相序：
- 亮点：
- 槽点：

## 4. 硬门槛总表

| 硬门槛 | Round 1 | Round 2 | Round 3 | 时间码/证据 | 客观问题移交 |
|---|---|---|---|---|---|
| HG-OL-01 崩溃、冻结 >1s、白屏/黑屏 | `NOT_ASSESSED / PASS / FAIL` |  |  |  | `NO / YES` |
| HG-OL-02 撕裂、破面、整块闪烁 | `NOT_ASSESSED / PASS / FAIL` |  |  |  | `NO / YES` |
| HG-OL-03 液滴或界面瞬移 | `NOT_ASSESSED / PASS / FAIL` |  |  |  | `NO / YES` |
| HG-OL-04 可见体积突增/消失 | `NOT_ASSESSED / PASS / FAIL` |  |  |  | `NO / YES` |
| HG-OL-05 永久乳浊或无法分层 | `NOT_ASSESSED / PASS / FAIL` |  |  |  | `NO / YES` |
| HG-OL-06 倒立/翻回状态破坏 | `NOT_ASSESSED / PASS / FAIL` |  |  |  | `NO / YES` |
| HG-OL-07 持续流畅度失败 | `NOT_ASSESSED / PASS / FAIL` |  |  |  | `NO / YES` |

## 5. 客观问题移交

| 现象 | 发生场景/轮次 | 时间码 | 建议移交 | 备注 |
|---|---|---|---|---|
| 崩溃/冻结/白屏/黑屏 |  |  | tester |  |
| 精确守恒漂移 |  |  | tester |  |
| 精确恢复时限 |  |  | tester |  |
| `fpsP95` 不达标 |  |  | tester |  |
| 坐标切换审计异常 |  |  | tester |  |

## 6. 证据索引

```text
<route-slug>/<date>/EC-OL-01/round-01/
<route-slug>/<date>/EC-OL-02/round-01/
<route-slug>/<date>/EC-OL-03/round-01/
```

补充说明：

- 完整录像：
- 关键截图：
- 事件日志：
- 环境 JSON：
- SHA256：
