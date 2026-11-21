# T766 v1 · 目标 20 RX 修订包 SOURCE 与首拒真机续证

日期：2026-11-21，effector（tester 兼，含出包），真机（011124264R000337）。
被测：T766-rx（T763 RX_SOURCE_GUARD_COMPILE 修订，inputSha 376720d8 回执
一致→signed 0c980f70，sign=PASS；linkedElf BuildId 4bdd76d hilog 映射验
证）。证据：`test/reports/t491/t766/`（l1–l5 布局、v6/chain.txt 阶段链、
v6/g9boot-t766.jsonl 12 事件、v1 判定）。

## 执行（两次单次 Configure，均到 SOURCE 终态）

**run1**（hilog 环形缓冲先被刷，仅布局）：全链→APPLIED→**PUMP_FAILED
code=5（STALE）**。

**run2**（hilog 重置后采全）：ENQUEUED→CACHE_PRELOAD→APPLY→
SOURCE_BUILD_ENTER **41767B sha=ee66f5f2**（T736 冻结字节）→SHADER_COMPILE
→PROGRAM_LINK→SOURCE_REFLECT_EXIT（program=4，qualified）→**APPLIED**→
PUMP→**PUMP_FAILED code=6 = Result::DEADLINE**。

## 首拒确认（跨族一致）

**RX 修订包本轮首拒=DEADLINE=6——与 DM（T751/T762 DEADLINE=6/9）完全
一致**。run1 STALE=5 与 run2 DEADLINE=6 为同一 Pump 相对 250ms 窗口的不
同时点，同一新鲜度窗族。

## G9BootV1 原件（12 事件）

pid 927、映射 BuildId 4bdd76d、BOOT_AUDIT_OK 链、seal 6559303
（UNMAP_EXIT/PREPARE_EXIT/BOOT_AUDIT_OK）。RX BOOT 事件 fenceWaitRaw 字
段为 None（RX 版 schema 与 DM 不同——已记）。

## T736 合同项记账

| 项 | 结果 |
|---|---|
| 同票 BOOT | ✅（BOOT_AUDIT_OK） |
| 原 SOURCE 输入提交 | ✅（ee66f5f2/41767B） |
| 首合格 pulse | ❌ Pump DEADLINE |
| fence/1024B/typed proof/209715 Q20/commit+1/FIFO/冷档/有效 Swap | ❌ NOT_REACHED |

## 回执

- RX 修订包确认 DEADLINE 首拒——两族（RX/DM）新鲜度窗证据齐备
- 归 architect：窗口放宽/两段式 claim 决策（RX+DM 双侧夹逼完成）
