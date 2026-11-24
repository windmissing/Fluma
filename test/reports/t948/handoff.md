# T948 · 目标 6/9/13 集显候选独立离线判据

2026-11-24 · tester · Windows UHD 630 原件离线复核。运行 `python test/t948/check.py selftest`，机器收据见 [offline.json](offline.json)。

| 子项 | 检查与原件结论 |
| --- | --- |
| 目标6 | 冻结 BASE、原输入、真实 SOURCE Δ、角、授权及 GPU/驱动。按完整 SoA 复算逐列和总库存、活槽身份，比较 candidate/spare、Completion、有效事件、源账和未写区，保首字差与首审计错。SOURCE 和 Q 的时标各自绑定 fence、queue、pool/reset。T941 复算 SOURCE 25 样本 p50/p95 27.202/30.226 ms，QUANTUM 1074 样本 14.984/18.595 ms。 |
| 目标9 | 以原 2404 逻辑行约束只裁有同代零副作用证明、超实际 J 的 AUTH/REPLAY 微步。核物理派发数、barrier、拒绝前缀、实际 J、掩码、非别名独立 replay 和各区原字。T923 同进程原件核到 SOURCE ACTUAL 等于后续 Q CURRENT/BASE，Q 业务 vk=2、无接受根，2404/2404/2404 和 EVENT 260/PATH 1 属晚读回。 |
| 目标13 | 核黏性 48、压力 49、其余 52/54–60/62 三闭包，各有独立 OP66、五绑定、24 B push、64 lanes，保 820 逻辑槽和同代范围。复用 T866 的 F02 压力和 MAC 速度门，另从 u/v 原字算散度，并检查原黏性残差。固定实际 F02 CONFIG/STATE 摘要。T901 历史 F02 仍为 GRANT_DEADLINE，迟到 GPU 根约 1770 ms。 |

独立入口为 `m1 <JSON>`、`dm2 <JSON>`、`g13 <JSON>`，字段示例见脚本 `fixtures()`。入口使用从新工件提取的同根证据和原字，实际 T946 格式到件后接适配。正例和 12 项负控当前由固定夹具执行。未运行设备测试。
