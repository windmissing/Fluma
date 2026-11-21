# T755 v1 · 目标 20 RX 非零 SOURCE 签包与真机验证

日期：2026-11-21，effector（tester 兼，含出包），真机（011124264R000337）。
被测：T755-rx（T752 one-shot RX BOOT+SOURCE 7b8c0953，inputSha 6cdcfc7d
回执一致→signed 7f8a2900，sign=PASS，roleSetSHA b718fe04）。证据：
`test/reports/t491/t755/`（l1–l4 布局、h755.txt 全 hilog、
v1-rx-source-stale.json）。

## 执行（one-shot BOOT+SOURCE，原研究页/XComponent/Controller）

启用研究 → **BOOT_AUDIT_OK**（15:30:30，G9BootV1 全链）→ SOURCE_BUILD
（41767B，sha=ee66f5f2——与 T736 冻结字节一致）→ SOURCE 编译 **85.6s**
→ SOURCE 链接 **235.3s** → SOURCE_REFLECT_EXIT（program=4，activeUniforms
=40，qualified=1）→ **CONFIGURE_RESEARCH_EXIT result=0 → APPLIED
result=0**（映射 BuildId 7bd1cae4）→ ~15:39 **PUMP_FAILED code=5**。

## code=5 归因（源码定位）

`Result` 枚举（research_config.h）：OK=0/IN_PROGRESS=1/WOULD_BLOCK=2/
INVALID=3/UNSUPPORTED=4/**STALE=5**——`transport_.Pump()` 返回 STALE，
RevokeLocked 撤权。

## 跨族对照（与 T751 DM 同族）

| 族 | SOURCE 拒绝点 | 结果码 |
|---|---|---|
| DM（T751） | 第 12 dispatch 后、fence 未建 | DEADLINE=6 / ControlResult DEADLINE=9 |
| **RX（本轮）** | **APPLIED 后 Pump 超窗** | **STALE=5** |

两族同因：**SOURCE claim/pulse 新鲜度窗口（250ms 级，T754 已归因）被
编译墙时击穿**——本轮 SOURCE 编译 85.6s+链接 235.3s 本身就远超 250ms
预算。T754 的 DEADLINE 归因与 T743 的硬件 FTZ 同属"设计决策域"。

## T736 合同项记账

| 项 | 结果 |
|---|---|
| 同票 BOOT | ✅（BOOT_AUDIT_OK ticket 1） |
| 首合格 pulse | ❌ NOT_REACHED（Pump STALE） |
| 原 SOURCE 输入 | ✅ 提交（ee66f5f2/41767B） |
| 12 pass / fence/1024B / typed proof / 209715 Q20 | ❌ NOT_REACHED |
| commit+1 / quanta 不变 / FIFO 消费 / 冷档 | ❌ NOT_REACHED |
| 同代 XComponent 有效 Swap | ❌ NOT_REACHED（独立记录） |

## 回执

- BOOT+SOURCE 编译/反射/APPLIED 全通（RX 侧新里程碑——比 DM 多走两步）
- 终点=Pump 新鲜度窗口：与 DM DEADLINE、硬件 FTZ 同属 architect 设计
  决策域（窗口放宽/两段式 claim/编译后即时 claim 三选）
- READY/有效 Swap：NOT_REACHED（按实证）
