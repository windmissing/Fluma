# T432 · 单沙准备许可生产逻辑独立差分报告

2026-11-09，tester，P4，不使用真机。前置 T428 实际生成页（commit 8bd7c1e，签包 eb6ae00b）已到。

## 交付

- `test/t432/prepare_diff.cjs` — 独立差分执行器（对隔离生成工程 `.codex/local-test/t432-diff` 运行，exit 0/1）
- `test/reports/t432/t432_prepare_diff.json` — 判定原件

## 方法与独立性

- 期望值由本卡按 **T426 §3–5 合同文本独立推导**（独立 `oraclePredicate` 复刻 §4 READY 谓词表的每一行），不引用开发自测断言、不把开发 16/0 当独立结论
- 驱动对象是 **T428 实际生成页的 `PrepareReadiness` helper + 真实页面接线**（真实 `TextInputController.stopEditing`/`clearFocus`/窗口避让监听/FrameCallback `onFrame→onIdle`/`reserveCapture` 唯一门），ArkUI/窗口/原生状态桩下执行——不是另写的理想状态机
- 与 T418（T413Capture hold 状态差分）、T427（归档判读反例）不重复

## 结果：54 一致 / 0 分歧 / 0 漏接线；静态接线 7/7

| 组 | 覆盖 | 关键判定 |
|---|---|---|
| 静态 W1–W7 | 3 个 TextInput 独立 controller、无 hideTextInput、`prepareGate.consume()` 唯一门、stopEditing 先于 clearFocus、defaultFocus(false)、t426-prepare 冷档命名、permit≠hold token | 7/7 |
| O1–O5 READY 谓词 | 冷启无键盘两帧稳定 READY（stopEditing×3 原始 API 记录、两个不同帧点）；编辑→非零避让→退出→零避让→READY；键盘仍在（ratio1 不能覆盖）不 READY；曾编辑无非零避让 `KEYBOARD_VISIBILITY_UNKNOWN`；浮动键盘/非全屏不 READY | oracle 逐阶段拒因与实际一致 |
| O6–O8 许可生命周期 | 位置漂移重置稳定计数+旧许可撤销+预约零副作用；native 旧几何/迟到布局不能出许可；唯一预约门（无许可 REJECTED 且 sequence 不变、有许可 RESERVED、permit 一次性消费） | 一致 |
| O9–O11 失效与首因 | READY 后编辑失效；OPEN 中键盘遮挡 hold INVALID 且首因不被后续事件改写；连点零副作用+恰 3000ms 等号超时失败；hide/show 精确解绑监听+旧代回调/定时器/监听惰性 | 一致 |
| O12–O14 诊断与所有权 | 原始 code55/name/message/runStatus 保留不赋 READY 义；窗口订阅失败具体拒；隐藏页旧异步写盘失败不影响新准备、不碰 exporting 所有权；vp2px 漂移重置+viewport 出窗即 INVALIDATED | 一致 |
| O15–O18 负控与冷档 | 历史 reject55 三态夹具不能替代实际观察（隐藏后 READY、仍显示/未知拒）；窗口重建终结准备；**期限等号**（started+3000 恰好边界 FAILED）；冷档独立命名全状态（CONFIGURING/PREPARING/READY/FAILED/INVALIDATED）+SHA 收据 | 一致 |

## 过程记录（非产品缺陷）

初轮 5 处 DIVERGE 全部为差分器自身假设错，逐项核实后修正：stopEditing API 记录名带序号后缀；O2 观察需两帧（与开发用例同拍）；期限用 report 的 `startedMonoMs` 同域比较；冷档命名实际含 CONFIGURING 状态（`prepareId>0` 时回配置也落盘，产品真实行为）；帧队列需消费两次（`prepareFrame!==callback` 引用匹配）。修正后 0 分歧。

## 边界

- 合成/桩域：真实 IME 事件、设备窗口能力、系统 PNG 可见性归 T408 真机采证
- 本卡不卡 T408 设备窗口，不作开发关卡；结果供 T408 续测直接消费
- 未发现漏接线与语义分歧——T428 生产准备逻辑与 T426 §4–5 合同一致
