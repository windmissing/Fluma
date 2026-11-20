# T593 v5 · CURRENT 窗口实测——READY 门过/忙态拒/收据读数窗口窄

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T593-probe-signed.hap（正式 M1 t597-fixed 重部署，signedSha 133f1257）。
证据：`test/reports/t491/t593c/`（p4–p7 布局、t1–t6 触摸序列、
t676b-boot3.json POST_TOUCH boot 原件、v5-current-window.json）。

## 执行链

1. 重部署正式 M1 → 新空场 → run2/epoch4（与 v4 READY 原件同 scope）
2. 30s 后 touch1（220ms）：**状态行 `CURRENT · NONE`**（sample 5981，物理
   0:649）——**READY 门过、源链门开**（首次 CURRENT 实测）
3. touch2（250ms，紧随 CURRENT 观察）：状态转 STALE/DRAW_PENDING，收据 #0
4. busy 振荡期连续 6 次触摸（240ms，GPU_WORK_PENDING/DRAW_PENDING，物理
   0:846→、sample 7808–9279）：全在忙态，收据 #0
5. POST_TOUCH boot 导出（pid 13252）：recovery **phase=READY / resetSerial=2 /
   epoch=4 / resources=true**；attempt4 terminal / readyEpoch=4 /
   publishedRun=2 / firstFailure 空

## 判定

- **CURRENT 窗口存在且可实测**（非纯推断）——T678"忙态不撤 ready"被正向
  证实：CURRENT 观测后物理持续推进
- CURRENT 窗口**窄**（单 sample 命中后即回忙态）；忙态期 6 次触摸全被
  READY 门拒（如实）；CURRENT 窗口内那次触摸收据仍 #0——UI 状态采样
  不足以定位 enqueue/claim（T678 §B.4 明示需 handler/changedTouches 域内
  观测）
- boot 原件：READY/资源/发布历史与 v4 一致，POST_TOUCH 无回退

## 剩余（精确）

同页 handler 域观测（T678 §B.4：PAGE_TOUCH_HANDLER_ENTER 类域内 capture 或
冷导出 pairing）——T671 离线解析通道就绪；UI 采样路径已到极限。
