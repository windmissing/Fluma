# P-011 主观效果报告 — bottom-creep-v2.7

## 报告 #1 — 日期：2026-10-16，状态：BLOCKED(no_device_access)

### 1. 测试对象

| 项目 | 记录 |
|---|---|
| 问题 | P-011 倾斜跟手性/底层运动响应 |
| 技术路线 | `bottom-creep-v2.7` |
| 主观用例 | `docs/effect-cases.md` 的 `EC-TT-01~EC-TT-05` |
| Runbook | `docs/effect-runbook-P011-bottom-creep-v2.7.md` |
| 证据模板 | `docs/effect-evidence-template-P011-bottom-creep-v2.7.md` |
| 可运行工件 | `.ws-p011-v27-review-signed.hap`，SHA256 `978E5A72099E1ADEBE5AAA2BD56479EECBC7EF868A813F5DEA9F4DA23108F678` |
| Git commit | 本机当前 `git` 命令不可用，未记录 commit hash；解除阻塞后必须补记。 |

### 2. 当前执行结论

1. **当前不能真机执行**：本机未检测到 `hdc` 命令，也未获得已连接真机证据。
2. **本轮不评分**：`EC-TT-01~EC-TT-05` 均为 `BLOCKED`，所有 TIERED 维度保持 `NOT_SCORED`，不填充默认分数。
3. **硬门槛不清除**：`HG-TT-01~HG-TT-06` 均为 `NOT_ASSESSED(no_recording)`。
4. **主观验收结论**：`BLOCKED`；未达到“所有适用场景 PASS 且无 MUST-PASS 触发”的验收标准。
5. **移交 tester**：精确时延、相位滞后、跟踪率、传感器采样异常和 `fpsP95` 属于客观口径，不由主观评分替代。

### 3. 后续入口

1. 解除阻塞条件见 runbook §6。
2. 正式执行时按每场景 2 轮保存完整录像、hilog、环境 JSON、关键截图、0.25 倍速复查与逐帧证据。
3. 将各场景、各维度得分和硬门槛复核结果以追加模式写入本文件，不覆盖本条 BLOCKED 记录。
