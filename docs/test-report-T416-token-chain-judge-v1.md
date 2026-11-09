# T416 · GPU 同窗功能与 30fps 报告判读器

2026-11-09，tester。不使用真机。消费 T409 host 档（mixed/late，HOST_SCRIPTED_OFFLINE 如实透传）与 T411/T412 已交工具结论；不复制其代码功能。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t416/token_chain_judge.cjs` | token 链独立关联判读器；`--selftest` / `--summarize <dir...>`（T410 报告入口，exit 0/1） |
| `test/reports/t416/t409_summary.json` | mixed/late 两档汇总事实输出 |
| `test/reports/t416/t416_verdicts.json` | 判读结论+证据 SHA |

## 2. 检查项（K1–K7）

- K1/K1b token 页级与行级 captureId == manifest（身份混窗负控）
- K2 required 事件在场（BEGIN/END/READY/FIRST_OFFER/FIRST_ACCEPT 按 manifest 声明）
- K3 token 链：空场 QUANTUM（sourceCommit=0）独立分列；SOURCE(kind=1) 的后继 = 该 SOURCE 之后到下一 SOURCE 之前的全部 QUANTUM（按 physicsCommit 序，T407 §5 语义）；链完整 = 后继中至少一条 ATTEMPTED 且 requestedToken==renderedToken 逐位且 current/swap 均 true；NOT_ATTEMPTED_NO_SCENE_TOKEN（cadence 跳过）合法不判断
- K4 空场/SOURCE 链/后继 commit 分列输出
- K5 模拟秒（accepted×1/120 整数 tick）与墙钟窗口（SUMMARY 页 startNs/endNs + clock 域名）分列，不混算
- K6 显示账分列：attemptedDraws/distinctPhysicsCommits/nativeSwap/renderTargetHz/systemPresentVerified（缺系统呈现=NOT_MEASURED 如实）
- K7 HOST_SCRIPTED_OFFLINE 透传（不当设备帧率/系统呈现证据）

## 3. 自测（合成正反控，4 ok / 0 bad）

- good：空场 Q + SOURCE 链各 1 完整 → 全 PASS
- broken：SOURCE commit 在全部 QUANTUM 之后 → `NO_SUCCESSOR_QUANTUM` 检出
- wrongtoken：requested≠rendered → `REQUESTED_NE_RENDERED` 检出
- mixedwindow：token 行内 captureId≠manifest → K1b 行级混窗检出

## 4. 消费 T409 host 档

| 档 | 判定 | 链 |
|---|---|---|
| mixed | **PASS 8/8** | 空场 Q(root1) COMPLETE；SOURCE_1(root2,沙)→succ[3,7] COMPLETE；SOURCE_2(root8,水)→succ[9,12] COMPLETE；accepted=10→模拟 0.08333333s；墙钟 88ms（domain=SCRIPTED_OFFLINE_STAMP_NS）；显示 attempted=3 distinctCommits=3 swap=1 renderTargetHz=30 systemPresentVerified=false |
| late | **FAIL（预期=lateSwap 反例）** | root1 ATTEMPTED 但 current=false/swap=false/requested≠rendered——Swap 成功但返回越过 deadline 的拒绝链反例语义，判读器如实标记（不冒 PASS） |

## 5. 边界

- T409 host 档为 HOST_SCRIPTED_OFFLINE：场景经真实 App/ANGLE 执行但脉冲为脚本驱动、SOURCE 来自 Bridge 指令非触摸——本工具透传该标注，其输出不当设备帧率、实际 250ms 延迟、用户掉沙或系统呈现证据
- 系统实际显示证据缺通道：K6 恒 NOT_MEASURED，待 T410 真机窗口补
- 无阻塞；T410 可直接 `--summarize` 消费
