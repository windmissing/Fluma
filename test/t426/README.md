# T428 单沙页面准备交接

依据 `docs/architecture-T426-single-sand-prepare-readiness-v1.md` §3–5。来源为 T414 生成器，版本 `T426_PREPARE_V1`。新增文件均在 `test/t426`，生成后写入独立工程。旧 T414 可以继续按原生成命令复跑。

```powershell
node test/t426/generate.cjs .codex/local-test/<全新目录>
node test/t426/page_units.cjs .codex/local-test/<全新目录>
node test/t426/hold_units.cjs .codex/local-test/<全新目录>
node test/t418/capture_state_diff.cjs .codex/local-test/<全新目录>
python -X utf8 test/t413/units.py
python -X utf8 test/t413/runner_units.py
python -X utf8 test/t427/e2e_negatives.py
```

`page_units` 提取实际生成页的字段、方法、FrameCallback 以及三条 TextInput 链，转译后在 ArkUI/所属窗口/原生状态桩下执行。输入创建、编辑、焦点事件来自真实接线，原生 reserveCapture 也实际执行。`hold_units` 对旧 hold 用例提供“前置许可已通过”的测试桩，页面许可本身由 page_units 检查。合成证据标记 `NOT_RUN`，设备观察由 T408 记录。

SDK 为本机 DevEco Studio 的 API 26 SDK。构建在生成目录运行 `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`。签包使用 `python -X utf8 test/t257/sign.py <唯一标签> <生成目录>`。最终交付路径、SHA 和编译记录见开发报告及 `test/reports/t257/T428-finalized.json`。

T408 按以下顺序续测。

1. 核新签包 SHA，启动单沙隔离页。确认实际为主窗口、全屏手机、固定系统键盘，再点击配置区的“确认全屏手机固定键盘”。这是具名 preflight 配置来源，浮动键盘或未知模式不确认。保留实际模式和可见界面证据。
2. 在配置区选标定/厚床/动态臂，填写三处 ROI、冻结计划与 maxHoldMs。三个输入项分别有独立 `TextInputController`，`defaultFocus(false)`，从创建起接线编辑/焦点观察。配置完成后点击原 `t413-prepare`。
3. 页面进入 PREPARING，切换至固定高度动作区。三个组件执行 stopEditing，随后 clearFocus。页面监听所属主窗口的键盘避让/高度、窗口活动/模式，读取当前 window 与 component 的 px 原矩形、浮点 vp 几何和实际 vp2px 转换结果。两个不同 `onFrame→onIdle` 帧点观察一致才 READY，`t413-reserve` 随后可点。3000ms 只会结束为失败。若键盘曾弹出却无法观察到非零避让，记录 `KEYBOARD_VISIBILITY_UNKNOWN` 并交 architect。
4. 保留 `WS_EVENT t426_prepare` 的原文件名/字节/SHA，以及 app files 中 `t426-prepare-<session>-<pageGeneration>-<prepareId>-<state>-<observationSerial>.json`。每份包含输入历史状态、原 API code/name/message/runStatus、避让四边 px、窗口/frame/原生身份及 preparePermit。状态文件使用独立文件名，后续 READY/失效会另存，完整引用原件一起取回。
5. READY 后沿 `test/t413/README.md` 使用原 capture_hold.py。预约入口会同步重取窗口/编辑/几何/原生状态，任何变化先拒绝。成功预约消费一次 permit。完成 seal、native 归档和释放后，回配置重新准备。hold、clock、PNG、颜色和映射判读继续使用原工具。
6. 分别记录无键盘冷启和主动编辑后准备。再用新事务验证 PREPARING hide/show、READY 后窗口/焦点变化、OPEN 中键盘遮挡导致 hold INVALID。历史 attempt1 的原 hilog 缺口保留。测试包的 API/准备、完整截图链、校准和材质配对分别报告。

prepare 使用自己的代际和待处理标记，导出 `exporting` 继续由归档流程拥有。页面 hide/disappear 按原 callback 引用撤销监听、定时器和帧 owner。旧异步写盘失败不会改新代状态。FAILED/INVALIDATED 时回配置并修正具体前置，再发起新的 prepare。窗口监听注册失败需重新进入页面建立监听，不能把缺失观察当成空避让。
