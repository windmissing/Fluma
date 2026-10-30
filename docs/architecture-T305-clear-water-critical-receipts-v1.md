# T305 明水长窗逐笔收据缺口首次归因 · 工程修订 v1

2026-10-30，architect；原卡 `6ae41460e4b031ea82cab2a0`。消费 T293 正式报告和七项原件、T270 隔离诊断源码与 T289 正控分流；不使用真机、不重跑测试、不改物理/光学/endpoint 协议。本稿限 C 诊断证据的保留/同 run 关联。

## 已证范围与缺口

T293 长窗 `arm1_diag.json` 为 run `cw-run-1793330568446-1-1`，frame900–902 的 requested/rendered token 均相等、`frameData` W 全局约 0.033377875 且 slot 验证为 1；本 run profile 的实量水在列127，ROI `(1172,0,4,4)` 反推列127。层2/3 raw 为 `12161fff`，层4 有 `22252bff`，是**同 run、同采样帧水面层非零字节差**的窄证据。body 层3不变与不足一个像素的薄水厚度相容，不能因 surface 变化声称 body 已绘出或整体观感已通过。长窗 `count=256,dropped=2913,incomplete=true`，保留事件计数为 STOP63/TICK192/SELECT1，DOWN/ENQUEUE/TERMINAL_QUERY 均缺。短窗 `arm2short_diag.json` 是另一 run `cw-run-1793330732437-1-1`，留有 DOWN、ENQUEUE、TERMINAL 及准入 0.011133 的源收据；**不能与长窗 frame900 的 token/W/ROI 拼成一条源到像素闭环**。

源码 `ClearWaterDiagnostic.event()` 在 `count===capacity` 后对所有事件一视同仁只 `dropped++`，`ClearWaterSourceController.tick()` 每次记 TICK，stop/select 也频繁写入，`sample()` 的 CLOCK_WINDOW/ENQUEUE/BATCH/TERMINAL_QUERY 随后同样被拒。这是**诊断优先级与容量接线缺口**，不构成水源物理失败或 shader 公式错误证据。T270 的 frame 槽另行封存，故能有同 token 的 W/像素窄证；但该槽不含已经丢失的逐笔请求/终态。现阶段不派参数调节或光学设计；体验问题待同 run 完整链和 effector 再判。

## developer 工程合同（不使用真机）

为隔离 `cwdiag=true` 路线新增版本化 `CW_C_DIAG_V2`，保留 V1 原文件只读兼容。把逐帧/常规 TICK 与重复 STOP 放低优先有界 lane；为 DOWN/有效 CLOCK_WINDOW/ENQUEUE/TERMINAL_QUERY/OLD_KEY_QUERY、源停止边界与首错设**独立、预分配、定容 critical lane**，不得让 TICK/STOP 抢其槽。首版容量按 T293 实际 40 秒源窗口的请求数测算并留余量，给出每 lane 精确行宽/字节、字符串/导出上限；容量满时 `criticalDropped/incomplete/firstLostKey` 必须显式，不能宣“逐笔完整”。若单窗源频率超过容量，提供显式冷分段封存/续 cursor，以同 run 连续段号、首末 key/源 commit 和每段 SHA 拼接；不得在热路径逐帧 JSON/hilog、隐式覆盖首记录或无界内存。低优先 lane 可限频并记录 skipped 数及采样规则，但 `CLOCK_WINDOW` 与每笔请求/终态的因果链不得被限流。

每笔 critical 行复制当时的完整 `runId/runGeneration/configRev/geometryEpoch/inputGeneration/gestureId/sourceId/requestSeq/kind/x/y/requestedVolume/timestamp/unit/clockWindowId`，ENQUEUE 原 ACK、query 的 TERMINAL/未终态状态与 `disposition/status/admitted/rejected/transactionId/kindSuccessSeq/sourceCommit/physicsCommit/W 全局与触点列前后/I_water/errorStage/replay`；保留原数值/字符串，不以 hash 或最终可变 `request` 对象代历史快照。DOWN/STOP 绑定同 run/gesture 和原因，旧 key 查询标只读不重发。冷导出包含两 lane 的 revision、容量/count/dropped/skipped、原事件词典、包 SHA、run 身份、段号、整段字节长度/SHA 和 END；part 缺失或 SHA/序号不合即 INCOMPLETE。frame 槽仍独立封存 3 帧 profile/header/ledger/optics/token/native ROI，不能拿当前状态覆盖帧时原件。

诊断开关只增观测，不改变 source 准入、W/I/commit、时钟窗口、反馈、已有 candidate/LastGood/Swap 或显示；关闭时不分配大环、不读回 GPU。非机自测覆盖 40 秒等价事件洪水下 critical 不受常规 lane 淹没、容量真实溢出/分段缺失显式、同 key ENQUEUE→TERMINAL 原值和旧 key 查询、开关前后物理位型/提交/源反馈一致、导出分片校验。编译/非机自测与签包为开发交付；设备长窗证据交 tester，产品可见性不作为开发门。前景/背景仅新 XComponent+shader，失败 FAULT 或完整旧 token 显式 STALE，不走 Canvas。

## tester 同 run 判线（新包，真机串行）

一个新 run 从新 DOWN 起，封存有效 clock→每笔 ENQUEUE/ACK→TERMINAL 查询与源账、`criticalDropped=0`/全部冷段；选其后 frame 的 sourceCommit ≥终态 sourceCommit，核 runId、runGeneration、frame token requested=rendered、slot/native W/ledger、profile 列127 的**本 run 实量**、box/镜像/GL 左下坐标映射、同 token 层2/3/4 原 RGBA 与 swap/截图。若关键行缺失，只能保留现有层4窄证，整条源→像素链标 INCOMPLETE；不同 run 或不同 token 不得拼。若完整链成立而逐层像素不符预期，携原 profile/ROI/光学/首错回 architect 定位工程；逐层符合而体验差再交 designer/effector。不得因诊断缺档先调水量、sigma 或 shader。
