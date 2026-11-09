# T426 · 单沙采集页面准备与键盘就绪修订

2026-11-09，architect，P4。不使用真机。首次归因消费 T408 attempt1 报告、归档布局和截图、T413/T414 生成源码及 T418 报告。本稿修订 T413 §4.2 的准备动作和 §4.4 的必要检查，版本为 `T426_PREPARE_V1`。hold、独立时间包络、系统 PNG、映射和 RGB 零容差继续执行 T413/T414 原合同。

## 1. 裁定和证据边界

**分流为页面准备流程的工程问题，交 developer 修订并签包，然后 T408 新 attempt 续测。** 当前 `prepare()` 先 `clearFocus()`，再等待 `hideTextInput()`；仅 Promise 成功才置 `configuring=false`。任意拒绝均写同一 `KEYBOARD_HIDE_FAILED`，动作区因此不可进入。相反，Promise 成功分支也没有验证键盘实际消失、布局稳定或异步完成时页面仍属原会话。这是可从源码确定的双向缺口。

| 本轮核对对象 | 可以确定的事实 | 保留的边界 |
|---|---|---|
| `test/t413/page.inc::prepare/reserve`，T414-finalized 对应生成页 | `clearFocus → await hideTextInput → configuring=false`；catch 不保留原错误；预约依赖 configuring | 现有页面没有准备代际和观察式就绪门 |
| `test/reports/t408/attempt1/t408-l{0,1,5,7,8}.json` | 实际归档 5 份布局；l1/5/7/8 仍为配置区并显示错误，没有预约等动作区控件；组件整数 bounds 均为 `[51,990][1229,2706]` | 报告称 l0–l10、十次布局，本次不将未归档轮次视为逐件核验 |
| `t408-s4.jpeg`、`t408-s5.jpeg` | s4 有软键盘遮住组件下部，s5 键盘已消失但错误与配置区仍在 | 相同 bounds 不能证明系统可见区域无遮挡；JPEG 只用于界面诊断，不消费为颜色标定 PNG |
| T408 报告引述 pid6345，13:31:16.742–751 的 hilog | 报告记载 clearFocus、CloseKeyboard、safeHeight0、not bound、runStatus9/errorCode55 | 当前 attempt1 归档未见原始 hilog。具体调用归属、同会话因果和 55 的 SDK 映射尚不能独立复核 |
| T414 最终签包 | 本地 SHA256 与报告相同，`7850376c9ceec002e72008b79b77ddc785bf0ab1d4ccafc71a7e9b2f2e55f61f` | 本轮没有安装或运行该包 |
| T418 33/33 差分 | 覆盖 T413Capture 状态、身份和时间工具合成语义 | 没有执行生成页的真实焦点、IME Promise 或窗口事件，不能推出 prepare 已覆盖 |

最可能的设备时序是失焦已触发收键盘并解绑，随后显式收键盘请求失败。该时序符合报告与截图，但仍标为待原始日志补证的因果推断。工程修订不依赖认定“55 永远表示已无键盘”，也不把 `not bound` 当无遮挡证据。T408 本轮未建立预约和 OPEN，后续标定与动态逐像素配对仍未执行。

## 2. API 选择与有效范围

本页三个输入项使用 ArkUI `TextInput`。修订为每个输入项绑定独立 `TextInputController`，以 `stopEditing()` 退出编辑，再清理本页焦点。保存实际 `onEditChange` 与焦点回调，不能只在调用后自行将观察值写 false。移除该标准组件准备路径中直接调用 `inputMethod.getController().hideTextInput()` 的依赖。官方将后者用于自绘输入场景，组件编辑状态应由对应组件控制器管理。参考 [输入法 API](https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inputmethod/) 与 [键盘管理](https://developer.huawei.com/consumer/en/doc/harmonyos-guides/arkts-manage-keyboard/)。

本机 SDK `26.0.0/ets/component/text_input.d.ts` 的 `TextInputController.stopEditing()` 为 API10 起的 void 接口。`api/@ohos.inputMethod.d.ts` 对 `hideTextInput()` 声明的客户端错误为 12800003 等，包含未聚焦/未绑定等可能原因，没有将内部 55 声明为独立公共成功结果。不得自行建立 `55 → READY` 映射。SDK 的 `hideSoftKeyboard()` 标注仅系统应用且要求 `CONNECT_IME_ABILITY`，本卡禁止用它替换，也不申请系统权限或调用输入法提供方 panel 接口。

窗口观察使用**本页所属主窗口实例**的 `getWindowAvoidArea(TYPE_KEYBOARD)` 初始快照及 `avoidAreaChange` 专属监听，记录 windowId、区域四边矩形和单位。键盘高度/动画事件可作附加证据，但不能单凭一次 height=0 放行。窗口 API 的键盘事件和避让区域存在固定键盘、窗口模式与设备能力限制，不能据空避让区域推导浮动键盘已经消失。参考 [Window API](https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-window-window/) 和 [窗口常见问题](https://developer.huawei.com/consumer/en/doc/harmonyos-guides/window-faqs/)。

本轮支持配置明确为当前主窗口、全屏手机、固定系统键盘。tester 在采集 preflight 记录该配置和可见界面。浮动键盘、分屏/自由窗口或无法确认配置时返回 `KEYBOARD_VISIBILITY_UNKNOWN`，本轮不声称支持。不能由主窗口 API 获得的其他系统浮层仍需 T408 实际系统 PNG 和无遮挡检查证明，页面 READY 只是一项前置。监听注册失败、取窗口失败、区域单位/配置来源未知均返回具体原因，不能改成空区域。

## 3. 页面准备状态和提交点

在隔离页面增加独立 `CONFIGURING → PREPARING → READY` 状态，可转 `FAILED/INVALIDATED`。它与 T413 hold 状态分离。READY 不建立 hold、不分配采集 attempt、不改变 phase，也不证明系统截图成功。保留现有稳定按钮 id。

1. 接受 prepare 前核原 `allowed()`、页面 active、当前无 hold/导出冲突。分配严格递增 prepareId，绑定本次 pageGeneration、windowId 和配置输入 revision。重复点击在同一次 PREPARING 中拒绝，零副作用。
2. 输入编辑/焦点观察必须从三个输入组件本代创建时开始接线，禁用自动聚焦，不能等点击 prepare 后才订阅。以完整事件历史建立当前输入状态，窗口避让另取同步初始快照。退出三个 TextInput 编辑并 clearFocus，分别保存调用结果。不得强制重建输入会话、唤起键盘再关闭，或 detach 系统输入服务。
3. 切换至固定尺寸动作区，状态显示“准备中”，预约禁用。`configuring=false` 此时仅表示 UI 已切换，不能独立放行 reserve。保持 T414 固定控制区高度，确保最终布局而非输入布局参与后续稳定判定。PREPARING 允许必要调度完成页面布局和新管线绘制。
4. 等观察条件全部成立，随后在两个不同显示帧的布局完成观察点采相同完整几何、窗口和编辑状态。帧点必须来自实际 ArkUI 帧回调/布局完成机制，不能用连续同步取值或两次 setTimeout 代替。任何输入、焦点、键盘、布局/缩放、可见性、窗口或 owner 变化重置稳定计数。
5. 发布不可变 preparePermit，再置 READY。到期或失败保留原始诊断，可回到配置重新准备。选择 3000ms 单调时钟作为 PREPARING 看门狗，只决定本次超时失败；不能靠等满该时长获得 READY，不改 hold 的 maxHoldMs。每次准备回调和最终 READY 提交均核真实 elapsed，定时器晚触发不能绕过期限。READY 没有额外的3秒预约期限，许可依当前观察持续有效性撤销，预约仍必须即时复核。

每次 await、帧回调、窗口回调和 finally 写状态前均验证 prepareId/pageGeneration/windowId 仍当前且页面 active。`onPageHide/aboutToDisappear`、窗口销毁、回配置、输入修改、owner/context/surface 改代均撤销 permit 并终结当前准备。hide 后重新 show 必须新准备，迟到的成功/失败不得改新页面状态，不得清除新任务的 busy 标志。prepare 使用自己的 pending 标志，不复用异步导出的 `exporting` 所有权。按原 callback 引用解除自己的监听，禁止无参 off 清除他人订阅。

## 4. READY 谓词和预约复核

以下条件同代成立才 READY。任何 unknown 都不可转 true。

| 条件 | 观察、身份及拒绝边界 |
|---|---|
| 页面和窗口 | active、同一 pageGeneration/windowId、窗口有效且前台，注册与快照成功；更换窗口返回 `PREPARE_STALE` |
| 输入会话 | 三输入项当前均未编辑且无焦点，最后编辑/焦点观察序号属于本页。组件创建前接线、禁用自动聚焦且完整历史从未进入编辑时，可形成冷启未编辑状态；曾编辑/获焦则必须观察到相应退出/失焦。历史缺失为 unknown，不能在 prepare 时默认 false，也不臆造 SDK 未提供的同步 isEditing 查询。API 命令回执只记诊断 |
| 键盘和遮挡 | 已核支持的固定键盘配置，当前 TYPE_KEYBOARD 四边区域为空，且无仍在进行的显示/隐藏动画信号或相反观察。固定键盘实际弹出必须能观察到非零区域；曾弹出却从未观察到区域则 `KEYBOARD_VISIBILITY_UNKNOWN`。键盘有残留或覆盖组件返回 `KEYBOARD_OCCLUDED` |
| 几何和可见性 | 组件 fullyVisible、布局节点仍在、窗口内实际边界完整包含组件。记录浮点 global box、vp2px、窗口位置/尺寸、几何代及原生 viewport；位置、尺寸、缩放、方向/配置必须共同稳定。仅 onVisibleAreaChange 的 ratio=1 或整数 bounds 相同均不够 |
| 帧稳定 | 两个不同帧点的完整快照相同，期间所有观察序号无变动，无待完成布局/键盘动画；变化返回 PREPARING 重新计数，最终超时记录具体未满足项 |
| 原生身份 | 同一 controller/session、surface/context 与最新 native 状态有效，绘制几何代和 viewport 匹配。沿 T414 既有 native ready 检查，缺有效新管线帧则 `PREPARE_NATIVE_NOT_READY` |

避让矩形按 SDK 所属窗口 px 坐标保存。组件 global vp 先经实际 vp2px 和窗口原点变换到同一坐标系，屏幕截图 bounds 另存 screen px；不得混用坐标或加几何容差消除漂移。浮点几何沿既有精确身份规则比较，整数矩形仅作交叉核查。

`preparePermit` 至少绑定 revision、prepareId、pageGeneration、windowId、controller/session、surface/context、geometry/configuration revision、最终浮点 box/vp2px/viewport、输入及避让观察序号、两个 frameSerial、readyMonoMs。它是本地冷诊断记录，不能伪装为 hold token。

reserve 在解析 ROI 后、任何 `beginT394`/request/phase 副作用之前，同步复核当前 READY permit、编辑/焦点/避让快照、几何和最新 native owner。变化即拒绝并撤销许可，保留旧采集状态。真实异步查询必须先结束再进入同步复核，不得在校验与提交之间 await。新增 gate 必须落在实际 `reserveCapture` 的唯一调用入口或其强制前置适配器，禁止另一路只凭 `configuring=false` 预约。原 T414 ROI/budget/busy/归档/owner 校验全部保留。

成功预约后配置输入仍锁定。RESERVED/OPEN 期间出现新键盘、焦点编辑、几何/可见性或窗口变化，先按 T413 身份使该 hold INVALID 并保留首因，禁止隐式 seal、释放或重采。seal/PNG/时间包络仍执行原检查，READY 不能豁免后续任何失败。释放后须重新准备或用新一轮实际观察生成新 permit，旧 permit 不跨采集事务复用。

错误记录分别保留 API 名、调用起止单调时刻、prepareId、原始 code/name/message、可用的底层 runStatus，以及最终 readinessReason/未满足项。不把 hilog 的内部 55 直接写成 JS BusinessError.code。历史 `hideTextInput` 适配器若在回放夹具中 reject55，仅记录命令失败，仍必须逐项判定就绪；正式新路径不再调用此适配器。标准组件 API 抛错、监听失败或无法取得必要观察时明确 FAILED，禁止空 catch。成功/拒绝记录写独立 `t426-prepare-<prepareId>.json`，现有 T413 hold/manifest/clock/capture schema 不改。

## 5. developer 交付任务书

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部分支受阻先完成其他可做项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人任务行状态有限例外，不改滴答同名清单。共享代码中不回退他人修改。前景背景只允许 XComponent+shader，禁止 Canvas 上屏或失败回退；失败报告 FAULT，或保留有旧 token 且标 STALE 的上一有效新管线帧。

以 T414-finalized 为已知来源，建立可复现的新隔离版本，建议 `test/t426/generate.cjs` 和对应 inc，复用 T413/T414 hold 与工具。改动限生成页准备/布局生命周期、预约唯一入口的许可适配和冷诊断，必要时在生成 EntryAbility 提供所属主窗口引用。不要依赖 `getLastWindow` 误取后来浮层窗口。保留历史 T414 包来源可重放、现有按钮 id 与工具入口，不新建在线 endpoint、不改材质 shader、CPU/GPU 仿真、颜色和采集时间容差。

非机必要检查应**执行实际生成页采用的准备逻辑和接线**，可抽小型 helper 并用 ArkUI/窗口/帧桩驱动，另核页生命周期和 TextInputController 绑定。不能只测一个另写的理想状态机，或将 T418 controller 差分当页面覆盖。

| 场景 | 必须验证的结果 |
|---|---|
| 冷启本来无键盘、输入未编辑 | 组件创建以来的完整未编辑历史、窗口初始快照和两帧稳定后 READY，不能因没有 hide 事件永久等待 |
| 正常输入后准备 | 退出编辑观察、区域非零→零、最终动作区两帧稳定后 READY，预约身份正确 |
| 历史 reject55，分别搭配真实隐藏/仍显示/未知状态 | 回执自身不能成功或失败代替页面判定；仍显示与未知必须拒绝。正式新路径断言不调用 hideTextInput |
| 命令成功但键盘仍在，ratio=1/bounds 不变却受覆盖 | 不生成 permit、不预约；不能只测 catch 分支 |
| 同尺寸位置移动、vp2px 改变、动作区切换引起延后布局、native 旧几何 | 稳定计数重置，旧许可撤销，预约失败无 phase/request 副作用 |
| 连点准备、监听异常、标准组件 API 抛错、超时回调晚到 | 原始拒因可读，3000ms 后不可凭晚回调变 READY，恢复配置后可重新准备 |
| 等待中 hide→show、窗口重建、旧 Promise/finally/帧事件迟到 | 不改新代状态或 busy，不留下监听，不创建旧代 permit |
| READY 后重新输入/弹键盘再 reserve，或 OPEN 时弹键盘 | 前者拒预约零副作用，后者 hold INVALID 保留首因，不成功封存 |
| 浮动键盘/不支持窗口配置/空避让但无能力证明 | `KEYBOARD_VISIBILITY_UNKNOWN`，不因区域零放行 |

完成适当 T414/T418 既有 hold、零副作用、文件/SHA 和时间包络回归，完成 ArkTS/native 编译、签名验签。README 给出新来源 commit、生成/编译命令、SDK、签包路径和 SHA、准备冷档读取方法与失败恢复流程。交付完成即可结束开发卡，T408 真机结果不作为开发关闭前置，T427 离线反例卡不新增为本修复门。

## 6. T408 续测和补证任务书

真机标签，设备串行排队。前置为上述开发交付与新签包。tester 先核 SHA 和新版本，不复用旧 attempt 的 hold/时钟/图片。执行持续完成、本人行状态边界与新渲染管线约束同第5节。

1. 先尝试从原 tester 留存取回 attempt1 的**原始** hilog，至少覆盖所述 13:31:16.742–751 前后各1秒，含设备/包/PID/TID、日志时钟域和未裁剪文件 hash，并给出点击 prepare 与日志的定位依据。若原件已经丢失，明确历史因果缺证，另在新 attempt 采当前版本的原始日志；不补造旧证据。此历史补证不阻止 developer 修复已确定的页面合同缺口。
2. 新包分别做冷启本来无键盘、主动编辑弹固定键盘后准备，保留动作日志、完整布局、诊断截图/系统 PNG、prepare 冷档和窗口事件/编辑/帧/原生身份观察。证明不会因“本来已隐藏”卡死，且真实遮挡消失、动作区稳定后才可预约。设备只显示整数 bounds 相同不足以通过。
3. 准备中切后台再回前台、READY 后重新打开编辑或改变窗口，核旧许可失效；RESERVED/OPEN 中引入可控键盘/遮挡变化，核失效首因和禁止成功 seal。负控使用独立新事务，不污染正常标定样本。固定键盘配置下非零区域始终不可观测则记录能力缺证回 architect，不能跳过观察门。
4. 页面准备通过后，继续 T413 §5 与 `test/t413/README.md` 的完整正常采集。核同 id OPEN、native 归档、独立原域时钟、PNG 调用与取回 SHA、之后 seal、bundle 校验及严格 calibrate，然后沿原冻结映射做后续配对。保留原 hold 时间负控，不放宽颜色、几何和时间包络阈值。

API/页面准备通过、采集链通过、标定通过和材质配对结果分别报告。任何失败/缺证按原卡记录具体复现、原始错误、原件、已试办法、未完成项与真实前置，交 architect 分流。不用本稿的工程交付宣称单沙效果或大目标通过。

## 7. 本轮交付记录

完成源码、T408 报告和归档原件边界、T414 签包 SHA、T418 覆盖范围及本机 SDK API 声明核对。完成 API 选型、准备谓词、同代异步约束、预约提交前复核、开发非机检查与签包/T408 接续合同。不运行真机、不修改业务/测试实现。本卡工程方案可交付，历史原始 hilog 缺口由第6节精确补证，未将其改记为已证实因果。

本轮自行选择标准 TextInputController 路径、独立准备许可、两帧稳定和3000ms失败看门狗。这些是工程实现裁定，实际设备能力、准备动作和后续标定仍待开发及 T408 验证。
