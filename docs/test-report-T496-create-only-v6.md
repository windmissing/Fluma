# T496 v6 · 修正臂有界续测（T620 设施返修后，单臂 420s 观察窗）

日期：2026-11-19，tester，不使用真机。
被测：t496.exe（aba5938 设施返修后重编，1,311,232B），attempt=62012，arm=create。
消费：architecture-T620-v1.md（v5 报告勘误裁定）、T496 卡面 2026-11-19 接续。
证据：`.codex/local-test/t496/`（t496_log.ndjson 行 1218097213334300、
t496-stages.ndjson、build-t620.log、t496-v6-summary.json）。

## 勘误（按 T620）

v5 报告"GL sink 未注入/排除 driver/资源已 commit"三结论原件不能支持，撤回：
callId 411=glDispatchCompute 真实发出（sink 在注入），"资源已 commit"仅是
captureCommittedEpoch 读数误用。本报告以 T620 归因为基线重测。

## 设施返修（aba5938，全部落地）

1. hostNow 改真实单调 steady_clock（脚本时钟 profile 未用，不冒产品墙钟）
2. COORDINATOR_WAIT_ITER 独立命名 + 每 emit 带 ownerProgressSession/epoch/
   captureCommittedEpoch（owner 序号与协调 iter 分列）
3. 资源 commit 后的恢复握手块（RecoveryJson 快照→RecoverySessionLocked(serial,
   epoch,run,page)→PagePulseLocked(seq=1)→READY_CHECK；RECOVERY_LEDGER_EMPTY/
   BIND_REJECTED 诚实回退）——产品入口真实调用，不改 Allowed/fresh 布尔
4. D1OwnerHook 非阻塞 observer 化（旧屏障只在退出释放、可能夹死 owner——已消除）

## 修正臂实测（420s 观察窗，进程被杀时仍存活）

| 域 | 实测 |
|---|---|
| coordinatorWaitIteration | 15000（约 30s 一条 emit，真实 clock ✓） |
| captureCommittedEpoch | **恒 0**——资源 commit 从未发生 |
| ownerProgressSession / epoch | 1 / 1 恒定——owner 无推进 |
| 恢复握手块 | **NOT_REACHED**（未到触发前提） |

## 判定与回执

- CREATE_WAIT 卡点在修正设施下**依旧存在，且位于资源创建域（先于恢复握手）**——
  恢复入口返修无从生效；T620 §4 规则"必须证明已到资源阶段才选另一臂"，单臂即停
- 设施改造本身验证有效（真实 clock/分列快照/非阻塞 hook 全部按裁定工作）
- **回 architect**：资源 commit 未落地的具体卡点需产品侧诊断（此前 v4c 原件
  最后未配对 glDispatchCompute 411 + 本轮 owner 恒 epoch=1/session=1）——已超出
  tester 设施可修域；建议 developer 在 Transport create/资源 commit 路径加
  只读进度埋点后 tester 复跑修正臂
- 未重跑三臂（无增量等待）；D2/D1 同源差分不在本轮范围
