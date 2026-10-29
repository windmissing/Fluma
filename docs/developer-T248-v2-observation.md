# T248 V2 同帧只读诊断交付

2026-10-29，developer，不使用真机。工程/离线实现已交；SignHap 配置失败，不能交付有效签包，状态看评论。未改原生产源码、dt 或 V2 数值语义。

`test/t248/generate.cjs` 冷生成独立工程，唯一页面仍使用 XComponent+shader。对生产 model/facade/field/controller/compositor 的改动全部在复制件进行，逐项匹配唯一原串并登记逆向可还原 diff。公共平台接点的集成 patch 就是生成 manifest 的 edits；由平台 owner 串行处理，不能覆盖整文件。

- step 原始 offered dt、原回执、成功子步数×substepDt、accepted 累计秒分别记录；partial 原 step 只按实际成功子步计，derived fault 不冒成功。保留原首帧0及每帧1/30上限。reset 新 run 的累计值仍须设备消费者按 runId 分组，不能跨 run 求和。
- source 原 receipt/offered/admitted、同 run/epoch/commit/frame 的权威冷副本与 publication bh/mh、live/state/y/vy；LAND 从原 events 缓冲只读，使用 epoch/substep/requestOrdinal/particleId 去重，不调用会消费 pageBacklog 的 exportParticleEvidence。
- pack 原始2048F32数据、状态/viewport；native 按同 frameToken 记录 target Begin、七层 draw、Present、Swap 的 steady-clock inclusive 时间；Present 前读取当前候选 framebuffer 三个指定像素。读像素不吞 GL 错误，由原 pipeline 处理。CURRENT/STale/FAULT 仍以原 receipt 为准，像素采样不冒 swap/显示已验证。

upload 的独占时间尚无单独 scope；TARGET_BEGIN_UPLOAD_INCLUSIVE 和各层计时只能按其名使用，不能相加冒 GPU runtime。真实原码、同 token 像素内容/设备帧耗时、≥8s模拟时间长按窗口由 T249 取证，本轮 NOT_RUN。

离线 `node test/t248/selftest.cjs .codex/local-test/t248-delivery/entry/src/main/ets`：5项通过，65步期间 LAND 3行、granted合计0.3；观察前后完整 checkpoint/acc/cache 相等，publication bh/mh 对齐。行数是页出现次数，非跨提交累计独立事件统计。

生成：`node test/t248/generate.cjs .codex/local-test/t248-NEW`（必须新目录）。在新工程设置现有 JAVA_HOME 后执行 hvigor `--mode module -p product=default -p module=entry@default default@CompileArkTS default@BuildNativeWithNinja default@PackageHap`；SignHap 单独执行。实际输出 `.codex/local-test/t248-delivery`，证据及 ELF/HAP SHA 在 `test/reports/t248/delivery.json`。CompileArkTS/native/PackageHap 全通过；SignHap 原错00303116，storePassword/keyPassword字段长度小于32。未改任何签名密码/profile，不称安装通过。
