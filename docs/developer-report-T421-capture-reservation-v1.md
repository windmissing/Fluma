# T421 开发交付

2026-11-09。依据 T417 与 T419，将采集申请改为一槽异步预约。原生控制入口立即返回 `RESERVED`，当前 GL owner 在真实循环边界核 Run 与 transport、撤权排空、同 root 可执行许可和未交接 lease。安全边界同时固定初始账、BEGIN、预约对应的 captureId 与 startNs。准备块由冷启动及监督线程补充，GL owner 只转移已有所有权。独立监督在 `now >= armByNs` 终结预约，迟到 owner 不能开启；取消和换代保持旧冻结档。原 T409 冷档 schema 保留，预约回执另存。

| 检查 | 结果 |
| --- | --- |
| 实际 App 控制入口 | 同配置重复、异代、旧 ID、取消、同刻超时、旧冻档、幂等 end 通过 |
| 独立监督 | 无 owner 推进时按原边界超时，保留 WAIT_PRESENT，迟到检查无 BEGIN |
| PC ANGLE 实际 owner | 控制线程在 Pump 中提交一次，下一合法边界自动开启，首账及 BEGIN 核验通过。混合沙水 2 SOURCE、8 QUANTUM、7 跳绘、3 原生呈现。晚 Swap +1ns 拒 CURRENT，保留已接受前缀 |
| 路径边界 | 半提交、撤权排空和呈现 blocker 在控制断言中覆盖。T419 五场景基线自测通过。完整独立生产差分由 T422 执行 |
| 旧消费 | 实际混合档正例与 9 类拒判通过，页面控制器 25/0 |
| Hvigor 与签名 | PackageHap BUILD SUCCESSFUL，签名和验签通过 |

实际 ANGLE 用例使用脚本时钟及直接桥接源，不使用真机。保持真实 GAP、250ms permit、120Hz 配额和 V2 30Hz 绘制计量。渲染由 XComponent + shader 执行。

签包 `.codex/local-test/t257-sign/T421-release/T421-release-signed.hap`。SHA256 `5450ad3052654de11994e180870e0a6b9e0db7c80d2a6e95aa572003367ae751`。

T410 复测步骤：冷选 V1/V2 并开始新 run，核屏显实际 revision/run/epoch/session。待窗口绑定且 storage ready 后只预约一次，核 RESERVED→ARMED。确认 ARMED 后按既有同窗方案行使空场、沙、水。健康窗避免 dumpLayout 或外部 status 轮询。冻结并导出后成套保留 T409 manifest、分片及 `gpu-capture-request-<requestId>-<captureId>.json`，核 scope 和 startNs。若超时保存 `evidenceCaptureRequest` 终态中的 phase、blocker、observedNs、detectedNs 与检测延迟。停止源按钮保持真实 pulse，真实 GAP 仍由单独的回调暂停负控验证。重复选择先显式取消预约或结束采集。

复跑 `node test/t409/build_app.cjs`、`node test/t409/ui_units.cjs`、`python -X utf8 test/t419/safepoint_baseline.py`。ANGLE 输出 `.codex/local-test/t409-actual-pages` 与 `t409-late-pages`，用 `node test/t409/cold_units.cjs <pages目录>` 冷校验。T422 可在正常控制线程调用 `ReserveEvidenceLocked`，使用生产 owner 循环和 `ReadCaptureRequestLocked` 取证。不要在 Wake 注入 arm。
