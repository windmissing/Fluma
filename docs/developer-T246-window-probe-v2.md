# T246 异步窗口探针接续

2026-10-29，developer，不使用真机。按T255补齐独立probe工程；真实window/GPU/生命周期执行交T247，产品M0 adapter仍未接通，整卡部分交付/看评论。原2aa1dca历史证据不覆盖；本轮test/reports/t246/resume/。

run(profile,slowDispatches)不再同步等GPU一秒，而是建立真实fence/flush后返回PENDING；poll以glClientWaitSync(...,0,0)收取，未signaled绝不map。可控0–64个额外GPU compute dispatch、每个4096轮整数工作，最后恢复正确pattern，真实pendingPolls/wait结果决定有没有观测到迟到，不能强称必迟到。取消后保留inflight到真实fence退休，不map/不显示候选；lease由真实render-release fence保护，显式hold/release期间新run拒绝分配覆盖。

页面有慢GPU、cancel、hold/release、XComponent销毁/重建、日志按钮；UIAbility实际background/foreground调setActive，旧context销毁后所有GL名失效，重新current后再分配，不能复用旧GPU authority。真实XComponent create/change/destroy、提交/信号/取消/lease/后台前台记录在256条有界日志（带dropped计数、owner/context/surface/token/raw、实际context/surface创建销毁数）。只有完整shader场景draw/blit/swap成功才CURRENT，失败FAULT；无Canvas/CPU场景回退。private1024B tail明确不是T241 completion、productCommit=false，不动物理selector。

六档实际资源分配与compute全写/首尾读、fragmentSSBO及三全帧target沿用，分配/热poll界限分开。冷资源校验仍有显式map，不能把整个run当无等待热帧或设备性能通过；program/driver/RSS仍NOT_MEASURED。原T246_V1消费前错代钩子已由真实API/回调取代。

离线host通过256词篡改、高word、五代、cancel/lease/range以及AsyncGate真实使用的在途拒重用/取消到退休/resize旧代拒绝断言。实际窗口和GPU未执行，不从host测试推设备PASS。隔离工程.codex/local-test/t246-v2真实ARM64 native、ArkTS与PackageHap BUILD SUCCESSFUL，包SHA/ELF/源码在delivery.json；签名由下一张T257处理，未改凭据。公共平台/生产默认/T236主核未修改。

剩余：T236的实际M0 root/publication到本窗口的独立产品adapter与产品tombstone/前缀/commit证据仍未接通；不得把平台private tail当物理提交。T255要求的独立异步生命周期工程已可交tester，不因产品adapter而阻整段平台取证；是否关闭或分段接力由manager按本回执登记。清单仅本人状态看评论。
