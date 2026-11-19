# T496 v7 · C0 修正臂有界执行（T646 六步合同落实）

日期：2026-11-19，tester，不使用真机（host 设施）。
设施：aba5938（T620 四项）+ 本轮 T646 修复（步骤1/2/3/6）。
证据：`test/reports/t496/`（gltrace-t646cA.ndjson 533 行 / log / v7-c0-result.json）
+ `.codex/local-test/t496/`（out-cA.txt）。

## v6 勘误（按 architecture-T646-v1 §1 逐项落实）

撤回 v6 充分性判断：v6 实际 runId=t496-1218097213334300 进入默认 D1_OFF（非具名
C0/C1/C2——argv 误传静默走 D1）；"owner 无推进/资源 commit 未发生/设施全部有效"
不成立（ownerProgressSession 非 heartbeat、captureCommittedEpoch=0 只证无该发布
标志、420s 为外部观察非精确调用时长）。本轮 wallNs/scriptNs 混名已在 Emit 显式
clockDomain 字段修正（D1 臂）；CREATE_ONLY 臂 log 域仍脚本 clock（gap 如实记录）。

## T646 步骤落实

| 步骤 | 实现 |
|---|---|
| 1 严格入口 | main 只认 C0/C1/C2，未知参数 stderr+exit 2（禁止静默走 D1） |
| 2 runner | %ATTEMPTID%→%ATTEMPT% 修复；watchdog while 方向修复（存活且未到期）；
  PATH 注入；LAUNCH_AT 时间记录 |
| 3 clock 域 | Emit 加 clockDomain="steady_real"/wallNs 显式字段 |
| 5 有界采样 | 循环耗尽→DIAGNOSTIC_SNAPSHOT（RecoveryJson+owner progress）后退出 |
| 6 TIMEOUT guard | 资源未 commit→RESOURCE_COMMIT_TIMEOUT（不再无条件
  RESOURCE_COMMITTED/脚本 stamp 首 pulse） |
| 4 GL shadow 表 | 未实现（本轮 scope 外，gltrace sink 已覆盖 callId/functionId/
  ENTER-EXIT/wallNs） |

## C0 修正臂实测（attempt t646cA）

| 域 | 实测 |
|---|---|
| 严格入口 | CREATE_ONLY_START/STORE_CONFIGURED ✓（身份/sink 注入正常） |
| sink | gltrace 533 行真实记录（wallNs=真实 epoch ns）✓ |
| **终止** | **callId 266 glDispatchCompute(x=1) ENTER 后无 EXIT、进程消失**（非悬挂：
  tasklist 空、stderr 空、无 WER 对话框） |
| 与 v4c 差异 | v4c 三臂在 dispatch 411 **悬挂 430s+ 存活**；v7 C0 同调用**崩溃秒死**
  ——同一调用点、不同进程结局 |

## 判定与回 architect

- CREATE_WAIT 悬挂现象在本轮转化为 **glDispatchCompute x=1 硬崩溃**——这是
  精确新原件：崩溃点、callId、functionId、wallNs、gltrace 全序列在档
- 按 T646 §5 判据：有 ENTER 无 EXIT 不等永久 driver 死锁——但"进程消失"较
  "悬挂"更强，指向 dispatch 内部硬错误（program/binding 状态、x=1 参数、或
  driver），需要产品/驱动域归因（超出 tester 设施域）
- D1/D2 差分与 32cycle 仍 NOT_MEASURED（未到正控非空 SOURCE 前置）
