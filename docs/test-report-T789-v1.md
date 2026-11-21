# T789 v1 · 目标 20 RX SOURCE 同票终态与退役侧车真机验证

日期：2026-11-21，effector，真机（011124264R000337）。
被测：T789-rx（T785-authorized-handoff 957af8e1——**交接包未签名**，
本机 sign.py 签成 1079fd0e 后安装成功）。证据：
`test/reports/t491/t789/`（chain.txt 15 相位链、l2.json 终态、v1 判定）。

## 执行（单次真实 Configure，pid 8890，ticket 1）

15 相位链：ENQUEUED→CACHE_PRELOAD→APPLY→BOOT 编译/链接→SOURCE 编译/
链接→**APPLIED**→PUMP→终态 **PUMP_FAILED code=5（STALE）**。

（T776 轮为 DEADLINE=6——同 Pump 相对 250ms 窗的运行间时长差异，同一
新鲜度窗族。）

## sidecar：缺失（如实）

**自动 terminal/retirement JSON 未由本构建产出**——设备文件域仅有
gpu-program-cache，无 sidecar 文件。

## 退役原件：未采获（如实）

终态后 force-stop，G9BootV1 退役段事件已被 hilog 环形缓冲刷除——退役
原件本轮未捕获。

## 缺证（按原失败报告，如实记录）

- sidecar JSON 缺失：**T785 交接不含 terminal/retirement sidecar 输出**
  插桩——需 developer 出带插桩构建
- 退役原件：hilog 环形缓冲刷除（终态→force-stop 间隔内被滚出）

## 回执

- 终态/首拒：Pump STALE=5（同族），时序链 15 相位已存
- tester 管线已就绪（T776 流程可复用）——**前置=developer 出带
  sidecar 插桩的 T785 修订包**
