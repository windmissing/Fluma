# T504 · GPU 资源长序列与故障回收独立验证（v1 阶段：静态账独立复算 10/10 全 PASS；GL 32cycle 长序列待 T496 基建跑通后续做）

2026-11-14，tester，离线不使用真机，commit `ff37e7a` 基础上新增本卡文件。按 T500 §7 执行。本报告覆盖：四档账公式独立复算、高水位公式、T500 §4.1 host 布局独立断言。**GL 长序列 32cycle 部分依赖 T496 重建的测试 host（t492-m1-delivery 单源 + 来源门），该 host 的 ANGLE JIT 编译等待参数本轮已修复（900s），待其跑通后接续，不冒交。**

## 一、静态账独立复算（`test/t504/static_ledger.py`，10/10 PASS）

| 判据 | 结果 | 说明 |
|---|---|---|
| P1 kProfiles 四档齐 | PASS | 192/64、192/128、512/64、512/128 全在位 |
| P2 B2 公式复算（×4 档） | PASS | **每档公式值 == payloadBytes 逐位一致**（初版脚本漏了 M2 增量项 `3A128+8pad+33280+32768`，差 659456/1644544——修正后四档全对；B1 值同时独立得出 34551504/37728720/82374608/85551824，与 T500 §2 表一致） |
| P3 高水位公式（×4 档） | PASS | 普通 `B2+F`、twoBatches `max(B2+E,B2+F)`；192 档两者相等（E<F），512 档 twoBatches 更高（110.6/114.9MiB），与 §3.1 表一致 |
| P4 host 布局独立断言 | PASS | 从 T500 MSVC /Zs 审计原件独立提取：App M1=2365824 / TimelineSnapshot=754200 / EvidenceCapture=2818864 / AppTransport M1=182640，与 §4.1 表逐值一致 |

## 二、GL 32cycle 长序列（NOT_STARTED，如实）

- 前置 host（`test/t496/external_source_diff.cpp` + `test/t496/build.cmd` 来源门）已重建：单源 t492-m1-delivery、`/Zs /d1reportSingleClassLayout` 双 TU 一致 GATE_OK、exe 生成。
- 首轮运行暴露等待参数缺陷（ANGLE JIT 编译 8-15min vs 主线程 120s 等待）已修复（900s + 心跳），后台验证进行中；其 D1 正控（真实外线程 SOURCE 接受+非空 QUANTUM）是 T504 长序列的动作源，未通前 32cycle 无法开始。
- 按卡面：不修改产品容量/期限；RSS 曲线、driver 退役计数等设备项归后续真机卡。

## 三、判定

- 静态部分 **PASS**（10/10）；四档账公式与产品 kProfiles、T500 表三方一致，公式链无缺口。
- 长序列 NOT_STARTED（前置 T496 基建），不冒交；本卡转看评论等待续做窗口。
