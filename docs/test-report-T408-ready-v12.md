# T408 attempt12 · prepare v4 真机 READY 首证（T579b-prepare 包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T579b-prepare-signed.hap（t574-prepare-tree，T574 prepare transaction v4）。
证据：`test/reports/t491/t408vC/`。

## 决定性突破：READY 首次真机达成

操作链（与 attempt11 路径 A 完全相同的用户序列）：冷启（bm clean -d）→ 单沙页 →
确认固定键盘 → **无输入焦点直接"收键盘并准备"** → 结果：

- 状态行 **`READY · IDLE`**（首次），截屏 ready.jpeg 实锤
- WS_EVENT 全链：`t540-prepare-s1794969337770-1-1-1-PREPARING-18.json`（8113B）→
  `…READY-25.json`（76995B）——READY 原件 sha256=7d6ab6da…
- READY 原件：run=p011-run-1794969337780-3 / geometry=11 / winGen=1 / page=1——
  **同代无换代**；kbCfg=TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD、kbH=0、avoid 归零

## 对 T570/attempt11 的验证

同一用户序列在 attempt11（旧包）撞 run/geometry 硬换代 PREPARE_STALE；本 attempt
（v4 包）**同序列 READY**——T574 prepare transaction v4 的软重采样/硬换代门/同代
verify 语义真机生效，T570 §2 工程合同落地验证。

## reserve 域（本轮未过，如实）

三次"预约"点击后状态行仍 READY、files 无 t413-reserve.json——reserve 内部早退
（evidence===null 或 configuring 分支，未落回执）；READY 有效窗（3000ms）与 dumpLayout
往返延迟可能也是因素。reserve→HOLD→PNG→标定维持 NOT_REACHED，留后续 attempt
（快击序列或 UI 时序调整）。

## v4 witness 域

- 首失效 witness 只在失效 attempt 生成（本次 READY 无失效，无 witness 文件）——口径正确
- files 目录仅 2 份（PREPARING/READY），无旧失败残留

## 判定

- **prepare→READY 真机链 PASS**（MUST-PASS：同代无换代、谓词全过、无 jscrash）
- T574 v4 实现真机消费验证通过；T570 归因合同闭合
- 剩余：reserve→HOLD→PNG→独立标定（原卡后续轮）
