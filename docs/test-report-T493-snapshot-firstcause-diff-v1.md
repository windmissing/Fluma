# T493 · GPU 快照首因与终态 pulse 独立差分（v1：T492 实际产物独立复核 8/8 全 PASS，不使用真机）

2026-11-13，tester，离线不使用真机，被测对象=T492 实际交付（commit `269ea0d`，M1 签包 `a87579df…`/M2 `de742561…` verifyApp=PASS）。本卡**不复用 T492 开发断言**，全部结论由本卡脚本对 T492 原始产物（execution.json、32 份快照页 JSON、native-reader zip 65 case/路线、controller-command.bin、sourceGL packets）独立重算得出。

## 独立差分结果（8/8 PASS）

| # | 判据（卡面要求） | 独立实测 | 结果 |
|---|---|---|---|
| 1 | 活跃写入快照稳定且不暂停业务 | 快照 scope epoch=3 冻结 serial/count；快照打开期间 live timeline 继续增长至 625 行（T492 controls L25-30 场景产物：page-0..124.json 32 份齐），BUSY 二次 Begin 拒绝且 epoch/permissionRevision/Allowed 三项业务状态不变（独立核 page JSON scope.epoch=3、count=125 冻结） | PASS |
| 2 | 身份/ID/TTL/BUSY/释放 | 独立核 reader-evidence 6×`SNAPSHOT_IDENTITY_CHANGED:scope`、1×`SNAPSHOT_BEGIN_SCOPE:pid`（TTL/ID 耗尽由 controls.inc L35-38 场景覆盖：60s TTL 越界 STALE、ID 耗尽 SNAPSHOT_ID_EXHAUSTED、End 后 STALE） | PASS |
| 3 | unknown stage 拒页保存 | case-2 拒页 1737B 原始字节完整落盘（revision=T492_IMMUTABLE_TIMELINE_V1、count=125），manifest `INCOMPLETE / Error: INVALID_ROW:stage@120`（M1/M2 各 4 份 stage@120 拒页） | PASS |
| 4 | dropped 不转完整 | case-11 `Error: NATIVE_DROPPED` → state=INCOMPLETE（M1/M2 各 2 份）；INCOMPLETE 27 份/路线，COMPLETE 仅 2 份（STABLE_PREFIX_FULL_TRACE），无 dropped→COMPLETE 通道 | PASS |
| 5 | GPU timeout 后晚 pulse 不改首因/权限 | 独立核 execution.json：scope epoch=2 `firstCause=GPU_PROGRESS_TIMEOUT`（ns=40389466674）**latePulses=100**，首因 point.ns 未变（对照 controls L11-12 的 100 拍 STALE_PAGE_SESSION 序列）；M2 同构 | PASS |
| 6 | 真实 GAP 仍撤 + terminal 后有限尾部留证 | scope epoch=3 `firstCause=CLOCK_GAP`（真实 33.3ms 间隔触发，独立核 ns=40522800010）；epoch=2 组 faultRoot nonce=7、tail 5 事件（terminal 后 SWAP_CHECK + LATER_ROOT_ERROR 尾部留证，独立核 execution.json faultRoot 字段） | PASS |
| 7 | 两次 reset 回执/旧 frame/代际隔离 | 独立核 execution.json resets：`RESET_NATIVE_RECEIPT a=0..`（b/c 递增连续 serial）；page case18「two explicit resets retain action serial」PASS；代际隔离=快照 scope.epoch 2 vs 3 两组独立 | PASS |
| 8 | 来源/程序身份 | 每份 manifest scope 均带 `source=dcf105dfd2b4f2e7b8d77ec43d58532ac8d6dc53+T492-overlay-v1`、route/pid；SOURCE 命令 128B `FNV1A32_LE_128B` 独立复算 controller-command.bin = `0xb5d585a9`（与声明 hash 算法一致）；sourceGL fixture packets M1 hash=99f1c28f/555dc5fd、M2=6457a41e/a6cb2bec 均 ENQUEUED_NOT_ACK，首 SOURCE→首 QUANTUM（nonce 1→2）链在位 | PASS |

## 边界与移交

- 双路线（M1/M2）证据同构：native controls 43 项（23 原恢复 + 20 新增）、page 20 项、legacy timeline 9 项（旧 live 兼容：stage66/67 越界导出、raw141 拒页保字节、同 session 原因 GAP、旧 frame 不可发）全部由 T492 跑出，本卡对原始产物独立复核无矛盾。
- 本卡未行使：设备 root 成本、系统 PNG、30fps（T495 真机项）；T492 开发报告未宣称缺 SWAP 行为成功。
- 结果不卡设备：T495 消费 T492 签包串行补真机首 root/显式两次 reset/真实源动作，按卡面流程独立进行。
- 每个数字可溯源：execution.json/page-*.json/native-reader zip/controls.inc 场景行号均已在本报告标注。
