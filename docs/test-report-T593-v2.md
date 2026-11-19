# T593 v2 · 真实 typed SOURCE 与同序 Claim 续证——STATE_LOST 域阻塞原件

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T591-collector-signed.hap（t562-m2-fix 树=M1 体验页，与 T591 v3 同包），
bm clean 冷启直达沙水 DEMO V4·GPU M1。
证据：`test/reports/t491/t591c/t593_l1–l9.json`（九轮布局/状态原件）。

## 按 T623 合同执行的序列

1. 选源（"沙"按钮→setSource 路径）
2. 同代状态导出：`STALE · GRS_M1_PROGRESS_CLOCK_V1 / run 0:0 / epoch 3 /
   session 1 / sample 递增 · STATE_LOST`（pulse sample 持续增长=页面活、
   pulse 线程活，但 clock 域判定 STATE_LOST）
3. 新空场（epoch 2→3 重置）→仍 STATE_LOST
4. Home→回前台→仍 STATE_LOST
5. 真实触摸 Down 220ms（uitest swipe 同点=Down/Up，220ms 跨多个 pulse
   采样周期）→Up：最近源收据恒 `#0 0.0000`（**零 SOURCE 入队**）
6. "开始2秒同窗采集"窗口内嵌触摸+"冻结并导出同窗"→files 无同窗 JSON
   （导出按钮无产出或写其它域）

## 判定（T623 §证据分流表对照）

- 状态行恒 STALE/STATE_LOST、收据 #0——**down 拒绝（原因=时钟域
  STATE_LOST）**，offer/typed enqueue/Claim 全链 NOT_REACHED
- 按 T623 分流：`down 拒绝→按原 reason 修操作或具体产品状态`——此处 reason=
  clock 域 STATE_LOST 在页面 active/sample 递增下持续存在，属**产品状态链
  缺陷**（STATE_LOST 恢复路径缺失），回 developer
- pulse 采样递增（4205→12441）证明 page pulse 线程活着——"调度/页面 active
  链"分支不成立；是真 clock 域状态机在 STATE_LOST 后无恢复路径

## 原件与设施

- 九轮状态原件（选源/新空场/回前台/触摸/采集窗口全部组合）
- collector 侧 t591-collector-1795082916966.ndjson 同进程在档
- 不合成 SOURCE（T623 红线）；无 handler 观察合同需求——down 拒绝发生在
  model.down 具名拒绝（现有 reason 通道），先修产品状态

## 回执

回 developer：M1 页 clock 域 STATE_LOST 无恢复路径（page pulse 活/页面活/
epoch 重置/Home 往返均不能退出 STATE_LOST），修复后本卡全序列（选源→
Down 220ms 跨 3 pulse→Up→typed enqueue→同 seq Claim→SOURCE 终态采证）
脚本就绪即续。
