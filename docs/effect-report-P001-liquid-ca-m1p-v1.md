# P-001 liquid-ca-m1p-v1.0 效果验证记录

## 报告 #1 — 日期：2026-10-15，被测代码 git commit：bb854c21a260e7ace92edc99084fbdac736d81d7

### 1. 测试对象与环境

| 项目 | 记录 |
|---|---|
| 问题 | P-001：两种不互溶液体油水分层 |
| 技术路线 | `liquid-ca-m1p-v1.0` |
| 设备 | 真机 `62T0225B10010020` |
| 启动参数 | `rendermode=3 testmode=true forceinvert=false seed=0 shake=0` |
| 被测包 | `entry-default-p001-current-signed.hap`，SHA256 `44D4FFDFE4446E1150BCC4649A2610C09CCCD34D25BB0116EAFB125E96C41F4F` |
| 构建命令 | `hvigorw --mode module -p module=entry@default -p properties.enableSignTask=false assembleHap --analyze=normal --parallel --incremental --no-daemon` |
| 运行证据 | `selfcheck/p001-current-events-hilog.txt`、`selfcheck/p001-current.jpeg` |

### 2. 冒烟验证结果

| 场景 | 结论 | 证据 |
|---|---|---|
| EC-OL-01 静置分层 | 冒烟 PASS；主观评分 NOT_ASSESSED | 进程存活，`LIQ_METRIC` 显示 `heavyCount=1755`、`lightCount=1235`、漂移为 0，界面波动为 0 格 |
| EC-OL-02 摇晃混合与恢复 | 冒烟 PASS；主观评分 NOT_ASSESSED | Want 依次注入 `shake=1`、`shake=0`，边界事件各 1 次，无模型错误 |
| EC-OL-03 倒立与翻回 | 冒烟 PASS；主观评分 NOT_ASSESSED | Want 依次注入 `forceinvert=true`、`forceinvert=false`，边界事件各 1 次，无模型错误 |
| 运行稳定性 | PASS | 启动采样 524 个 `LIQ_FRAME`，`valid=1` 为 524、`valid=0` 为 0，进程 PID `38992` 存活 |

### 3. 主观验收说明

本轮为工程实现与真机冒烟验证，未组织算法文档要求的每场 3 名评审、seed 0/1/2 三轮主观评分。因此 `EC-OL-01~03` 只记录冒烟结论，不给出主观分与路线主观 PASS 结论。正式主观验收需按 `docs/effect-report-template-P006.md` 追加完整评审。

### 4. 客观报告

专项自检与阈值结果见 `docs/test-report-P001-liquid-ca-m1p-v1.md`、`test/reports/p001_liquid_mode_check.json`。
