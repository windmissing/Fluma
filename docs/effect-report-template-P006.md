# P-006 主观效果测试报告模板（EC-OL-01~03）

> 用途：为 P-006 每条技术路线生成独立、可追加的主观效果报告。复制本模板到 `docs/effect-report-P006-<route-slug>.md` 后填写；同一文件只追加新报告，不覆盖历史。不同技术路线不得共用报告或合并分数。

## 报告 #<N> — 日期：<YYYY-MM-DD>，被测代码 git commit：<full-git-commit-hash>

### 1. 测试对象与环境

| 项目 | 记录 |
|---|---|
| 问题 | P-006：场景一油水分层与容器动态主观效果缺口 |
| 主观用例 | `docs/effect-cases.md` 中 `EC-OL-01~EC-OL-03` |
| 技术路线 | `<route-slug>` |
| 被测包 | `<file-name>`，SHA256 `<sha256>` |
| 可运行入口 | `<page-or-command>` |
| 启动配置 | `rendermode=3`，`testmode=true`，`forceinvert=false` |
| 参数等价映射 | `<none-or-detail>` |
| 颜色映射 | 重相 `<color>`；轻相 `<color>` |
| 测试环境 | 设备 `<model>`；系统 `<version>`；分辨率 `<widthxheight>`；刷新率 `<hz>`；亮度 `<setting>`；性能模式 `<mode>` |
| 评分员 | `<name-or-role>` |
| 证据目录 | `<path>` |

### 2. 场景执行结果

| 场景 | 轮次 | 结果 | 有效证据 | 说明 |
|---|---:|---|---|---|
| EC-OL-01 静置水平界面 | 2（或 3） | `PASS / FAIL / BLOCKED / N/A` | `<video-and-screenshots>` |  |
| EC-OL-02 摇晃暂时混合与静置恢复 | 2（或 3） | `PASS / FAIL / BLOCKED / N/A` | `<video-and-screenshots>` |  |
| EC-OL-03 180° 倒立与翻回交换 | 2（或 3） | `PASS / FAIL / BLOCKED / N/A` | `<video-and-screenshots>` |  |

### 3. TIERED 分档

#### 3.1 EC-OL-01

| 维度 | R1 | R2 | R3 | 中位数 | 与上一版 | 判定 |
|---|---:|---:|---:|---:|---:|---|
| 分层可读性 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 界面平滑度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 稳定流畅度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |

#### 3.2 EC-OL-02

| 维度 | R1 | R2 | R3 | 中位数 | 与上一版 | 判定 |
|---|---:|---:|---:|---:|---:|---|
| 分层可读性 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 动态自然度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 恢复可信度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 稳定流畅度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |

#### 3.3 EC-OL-03

| 维度 | R1 | R2 | R3 | 中位数 | 与上一版 | 判定 |
|---|---:|---:|---:|---:|---:|---|
| 分层可读性 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 界面平滑度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 动态自然度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 恢复可信度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |
| 稳定流畅度 |  |  |  |  | — | `IMPROVED / ACCEPTABLE_DECLINE / REGRESSED / NOT_SCORED` |

### 4. 硬门槛

| 硬门槛 | 结论 | 证据 | 移交 |
|---|---|---|---|
| HG-OL-01 崩溃、冻结 >1s、白屏/黑屏 | `PASS / FAIL / NOT_ASSESSED` |  | `NO / YES` |
| HG-OL-02 界面撕裂、破面、整块闪烁 | `PASS / FAIL / NOT_ASSESSED` |  | `NO / YES` |
| HG-OL-03 液滴或界面瞬移 | `PASS / FAIL / NOT_ASSESSED` |  | `NO / YES` |
| HG-OL-04 可见体积突增/消失 | `PASS / FAIL / NOT_ASSESSED` |  | `NO / YES` |
| HG-OL-05 永久乳浊或无法分层 | `PASS / FAIL / NOT_ASSESSED` |  | `NO / YES` |
| HG-OL-06 倒立/翻回状态破坏 | `PASS / FAIL / NOT_ASSESSED` |  | `NO / YES` |
| HG-OL-07 持续流畅度失败 | `PASS / FAIL / NOT_ASSESSED` |  | `NO / YES` |

### 5. 可暂失败项

| 项目 | 结论 | 证据 | 缺口说明 |
|---|---|---|---|
| `U-OL-01` 翻转后 90% 上下交换的 2~4 秒时序 | `PASS / UNRESOLVED_FAIL / NOT_ASSESSED` |  |  |
| `U-OL-02` 扰动停止后 <=4 秒恢复近似水平界面 | `PASS / UNRESOLVED_FAIL / NOT_ASSESSED` |  |  |
| `U-OL-03` 界面过渡带宽 1~2 像素且无锯齿 | `PASS / UNRESOLVED_FAIL / NOT_ASSESSED` |  |  |
| `U-OL-04` 油水视觉真实感与液滴/卷曲细节 | `PASS / UNRESOLVED_FAIL / NOT_ASSESSED` |  |  |

### 6. 与上一版对比

| 对比项 | 上一版 | 本版 | 变化 | 可能原因 |
|---|---|---|---|---|
| EC-OL-01 有效维度最低分 |  |  |  |  |
| EC-OL-02 有效维度最低分 |  |  |  |  |
| EC-OL-03 有效维度最低分 |  |  |  |  |
| 硬门槛触发数 |  |  |  |  |
| 主观总体结论 |  |  |  |  |

对比规则：

1. 只与同一技术路线上一版比较；禁止跨路线做全量优劣结论。
2. 分档分数提升为“改善”；下降但仍 `>=6` 为“可接受”；跌破 `6` 或触发硬门槛为“恶化/未通过”。
3. `N/A/BLOCKED` 不得推定为改善或恶化；上一版 `N/A/BLOCKED`、本版有效且 `>=6` 判为“解除阻塞并达到及格”。

### 7. 体验亮点与槽点

**亮点**

- <highlight-1>
- <highlight-2>

**槽点**

- <issue-1>
- <issue-2>

### 8. 主观验收结论

| 项目 | 结论 |
|---|---|
| 场景结论 | EC-OL-01 `<PASS/FAIL/BLOCKED/N/A>`；EC-OL-02 `<PASS/FAIL/BLOCKED/N/A>`；EC-OL-03 `<PASS/FAIL/BLOCKED/N/A>` |
| 硬门槛 | `<NONE-TRIGGERED / HG-OL-__-TRIGGERED>` |
| 路线主观结论 | `<PASS / FAIL / BLOCKED>` |
| 是否达到预设主观验收标准 | `<YES / NO / BLOCKED>` |

判定规则：所有适用场景 `PASS` 且无任何 MUST-PASS 硬门槛触发，路线主观通过；任一场景 `FAIL`、`BLOCKED` 或硬门槛触发，路线主观不通过或阻断。

### 9. 客观问题移交 tester

| 问题 | 场景/轮次 | 时间码 | 证据 | 移交理由 |
|---|---|---|---|---|
| 崩溃/冻结/白屏/黑屏 |  |  |  | 需要复现路径、崩溃日志与时长量化 |
| 精确守恒漂移 |  |  |  | 需要模拟层粒子计数审计 |
| 精确恢复时限 |  |  |  | 需要 2~4 秒与 <=4 秒客观计时 |
| `fpsP95` 不达标 |  |  |  | 需要帧采样统计 |
| `invert180()` 审计异常 |  |  |  | 需要事件日志与坐标切换审计 |

### 10. 证据索引

- 完整录像：
- 关键截图：
- 环境记录：
- 事件日志：
- 被测包 SHA256：
- 执行脚本：`docs/effect-runbook-P006.md`
- 证据模板：`docs/effect-evidence-template-P006.md`

## 追加规则

1. 新报告追加到本文件末尾，标题递增报告编号，保留日期与被测 commit。
2. 不修改、删除或重排历史报告；发现历史错误时追加勘误。
3. 不同技术路线必须另建 `docs/effect-report-P006-<other-route-slug>.md`，不得合并分数。
4. 未运行真机或缺少证据时，如实记录 `BLOCKED`，不编造评分。
