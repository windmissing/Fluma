# T408 attempt10 v1 · T553 §1.1 可执行取证路径（T540-final 包域）

日期：2026-11-17，tester，真机（011124264R000337 / Pura 90 Pro）。
被测：T540-final-signed.hap（重装，设备 so 逐位核验=T540 包内 so）。
依据：T553 §1（IME 准备域拆项取证）+ §1.1 可执行路径 1-3。
证据：`test/reports/t491/t408vA/`。

## 决定性新证：attempt9 的"uitest 无法弹真 IME"结论被推翻

**真实系统 IME 键盘可见（截屏实锤）**：
- 序列=预约→**真实点击 `t413-roi`**（非先 inputText 注入，按 T553 §1.2）→`uitest inputText` 补充文本
- `ime.jpeg`/`ime4.jpeg`：系统 IME 键盘面板完整可见（约 y≈2050 起），输入框获得焦点、内容在档
- 这修正 attempt9 报告"inputText 注入不触发真实 IME"的结论——点击聚焦+注入文本的**组合**可弹出真实 IME；attempt9 单独 inputText（无聚焦前置）不弹的观察成立，但"设施不可弹 IME"的全称结论**不成立**

## keyboardAreaSeen 证据（T553 §1.1 核心待证项）

- prep-41 / prep-74 冷原件：`sessionEverEdited=True`、**`keyboardAreaSeen=True`**——
  IME 弹出时 `avoidAreaChange`(TYPE_KEYBOARD) 回调**确实到达 UI 并被记录**
  （T553 §1 "曾见非零 avoid"历史位在真实 IME 下成立）

## UNKNOWN 真因拆项（T553 §1 拆项要求达成）

prep-74（FAILED PREPARE_TIMEOUT:KEYBOARD_VISIBILITY_UNKNOWN）完整快照：
- avoid=四项全零矩形（收键盘后归零）、kbH=0、**kbCfg=UNKNOWN**、winGen=1、winId=413、active=true
- predicate 链（PrepareReadiness.ets）UNKNOWN 分支包含
  `keyboardConfiguration!=='TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD'`——
  **attempt9–v10 前段所有 UNKNOWN 的真因=未点击"确认全屏手机固定键盘"**
  （fixedKeyboardConfirmed=false → kbCfg=UNKNOWN），而非 IME 不可见
- 补做确认后（按钮变"已确认固定键盘"）再 prepare：UNKNOWN 不再出现——验证归因

## 新阻塞域：PREPARE_STALE（INVALIDATED）

确认键盘后 prepare 循环得 `INVALIDATED PREPARE_STALE`（prep-41 风格，state=INVALIDATED、
reason=PREPARE_STALE、observe 期间 snapshot JSON/serial 变化）：
- changed() 在非 READY 态走 invalidateHold 推进 observationSerial；KEYBOARD_AVOID_CHANGED/
  INPUT blur 等 prepare 动作自身引发的回调都会推进 serial
- Back→等 12s（avoid 稳定）→prepare 仍 STALE：prepare 窗口（3000ms）内的
  stopEditing/clearFocus/blur 回调本身推进 serial → verify 时 permit 失效
- attempt8（T521）曾有成功先例——时序差异（当时键盘状态/焦点史）待精确比对；
  本轮如实记录 STALE 域原件（prep-41/74 与 lI–lR 全序列 layout）

## T510 侧同批发现（T553 §2 相关）

- 设备上 T540 包运行日志（WS_EVENT t426_prepare）全部在档（hilog JSAPP 域抓取成功，
  含文件名/bytes/SHA256）
- 应用进程曾 onProcessDied（19:57:49，sceneboard 记录）——与本卡取证操作相关，
  原件留 hilog 输出

## NOT_REACHED（维持）

- READY→预约→HOLD→严格 PNG→独立标定：仍被 PREPARE_STALE 阻塞
- 已证域保留：T532 §3 整数框位置域（attempt9）、T510 Down→Offer 链（v2）

## 移交

- **architect**：①UNKNOWN 真因=固定键盘确认缺失（attempt9 结论修正为"未确认固定键盘"
  而非"IME 不可弹"）——T553 §1 拆项完成；②PREPARE_STALE 域：prepare 窗口内
  stopEditing/blur/avoid 回调推 serial 与 permit verify 的冲突需裁决
  （预期收键盘序列下 serial 稳定性设计意图 vs 实测）
- **tester 后续**：真人路径（T553 §1.2 effector/真人同路径）或 STALE 时序裁决后继续
  READY→HOLD→标定域
