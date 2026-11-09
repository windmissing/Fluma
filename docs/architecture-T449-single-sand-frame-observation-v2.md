# T449 · 单沙准备帧观察修订

2026-11-09，architect，P4。不使用真机。修订 T426 的帧采样实现，保留同代准备许可、两帧稳定及预约提交边界。

## 归因

T408 attempt2 报告四次复现，实际归档两个 prepareId 的 PREPARING/FAILED 冷档。失败文件 SHA 前缀分别为 `bd25d8e230c8`、`49053f542fff`。两次失败均为 `PREPARE_TIMEOUT:WAIT_OBSERVATIONS`，没有 FRAME 或 permit。失败快照记录 windowActive=true、nativeReady=true、页面及 native geometry=3，viewport 1178×1716。编辑历史、固定键盘声明和避让取值可供复核。这些单点数据无法证明两个独立的已完成布局帧。报告提到的 hilog 收据时刻和 VSync 间隔在当前提交中缺完整原流，不能按日志采样密度推算真实帧频。 [T408 attempt2 报告](test-report-T408-attempt2-frame-idle-block-v1.md)

`test/t426/generate.cjs` 的 `T426PrepareFrame.onFrame` 只写 frameNs。真正快照、释放 `prepareFrame` 和下一次 post 都在 `page.inc::prepareIdle`。窗口 connect 在 CONFIGURING 就可排一个旧 prepareId 的回调。若其 Idle 尚未到，点击 prepare 时 `postPrepareFrame` 遇到非空槽就返回，本次 id 连第一笔 post 都可能拿不到。若 Idle 迟到，旧 id 那次只会清槽及重排。原档缺 post/enter/idle 原始分段计数，故无法区分没有排进当前代、onFrame 未到、Idle 未到或迟到。代码已足以确认 **进度依赖 Idle 且同页不同 prepareId 共用悬挂槽**。T432 桩总是紧接 onFrame 调 onIdle，能证状态与接线，未覆盖平台只交付 onFrame 的行为。 [T432 独立差分](test-report-T432-prepare-permit-diff-v1.md)

ArkUI 的 onFrame 提供下一帧渲染开始时刻，onIdle 可随剩余预算延后。SDK 26 声明的 UIObserver didLayout 是布局完成通知，willDraw 是绘制指令下发前通知。两者均应订阅本页 UIContext。 [FrameCallback API](https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-uicontext-framecallback/) · [UIObserver API](https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-uicontext-uiobserver/)

## 实施合同

1. 在隔离生成页引入 `T449_FRAME_OBSERVATION_V2`。只在当前 PREPARING 事务内启动观察泵。先绑定 prepareId/pageGeneration/windowId/windowGeneration/UIContext 与 UI action-branch generation，再订阅 didLayout/willDraw 和切换动作分支。动作分支提供唯一可查的、固定尺寸的锚点。要求切换后的 didLayout 及当前动作锚点真实存在，收集新布局 generation。组件浮点几何、避让、输入、viewport 仍走现有采样及 T426 谓词。
2. 用 onFrame 生成单次、严格递增的 frameTimeNs 标记，不从回调开始点直接取“已布局”证据。仅随后同 UIContext 的 willDraw 领取未消费的当代标记，并在确认动作锚点、布局完成证据及无待处理布局变更后读快照。较早的 willDraw、未配对 willDraw、重复帧标记均跳过。下一 onFrame 使未消费标记失效。每个合格帧只可被采一次。didLayout 只在布局实际发生时更新证据，第二帧布局没有变化时可复用同一已完成 layout generation，仍必须有新的 onFrame→willDraw 及重新核对的快照。若 UIContext 无法提供可配对的绘制阶段，失败并保留原证据。
3. 优先复用页面现有 schedule/渲染循环驱动观察。若静态帧不触发实际绘制，可在现有固定尺寸诊断区使用有界、非几何的显示更新请求下一帧，仍须等真实渲染回调。不可用时报告能力缺口。timer、Promise、固定延时、强制滑动和同一帧内两次取值均不能生成帧证据。onIdle 可诊断，但不再负责释放槽或推进状态。
4. didLayout/willDraw 和帧回调均须核当前 owner、generation、frame serial、prepareId 和唯一 outstanding 槽。回配置、失败、READY、隐藏或换窗释放帧票与定时器，注销本次 UIObserver 回调，保留原窗口/键盘失效监听。新 prepareId 可立即排自己的第一笔。迟到的回调不得清新槽、改新许可或续约截止。保留 T428 同步 reserve gate 和一次性许可消费。
5. 输入、避让、窗口、可见性、几何、动作分支或 native owner 改变则丢弃稳定样本。若变化会触发布局，必须重新观察对应布局完成，禁止拿旧锚点、旧 geometry 或偶然一致的整数 bounds 放行。连续两笔真实快照须在不同帧、同布局和所有观察代内完全一致，并满足 T426 全谓词。预约前同步重读本页当前快照，许可/代际不匹配时拒绝且 phase/request/hold 零副作用。
6. 3000ms 仍以 app 单调时钟计算，仅为 PREPARING 失败截止。帧时钟只做排序，不与 app elapsed 相减。分别记录 POST/FRAME_ENTER/LAYOUT_DONE/DRAW_ENTER/SAMPLE/STALE_DROP 和 first/last 序号、各点 app 时刻及帧原时刻。超时可区别没有 post、未进帧、缺布局完成、缺绘制配对、谓词失败和未稳定。热回调只记有界记录，冷导出留实际原值与缺证原因。

## 开发和真机接续

developer 在 T428 的隔离生成器上实现并验证实际生成页。不使用真机。非机桩拆开注册、onFrame、didLayout、willDraw 和 Idle，覆盖从未 Idle 仍可达到 READY、未交付 onFrame、同一帧多次 draw、只有 onFrame 没有布局证据、动作锚点迟到、真实两帧正例、延迟布局漂移、旧代抢槽、精确超时边界、预约零副作用、READY 后失效与 OPEN 首因。T432 既有状态语义回归继续跑。交新签包、SHA 与复跑说明。设备实测另属 T408。

tester 串行验证冷启无键盘、编辑后固定键盘确实收起、静止页面真实两帧采样、重准备和后台旧回调，保留新冷档、原始事件/时钟及截图。验证同代许可后继续 T426 §6 的原 hold、独立时钟、PNG 及颜色链。缺回调按具体阶段原件回 architect。

执行者完成当前范围与角色检查，局部受阻先做其余。失败报告原错、复现、证据、尝试与前置。不得修改后续工作清单或滴答同名清单，本人行状态为唯一例外。渲染沿 XComponent+shader，故障为 FAULT 或完整旧帧明确标 STALE。
