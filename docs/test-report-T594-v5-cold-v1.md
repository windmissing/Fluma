# T594 v1 · V5 ROOT/FULL 完整冷档独立验证（T596 7650cb7 交付消费，首受阻）

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T594-v5-signed.hap（t596-t614-delivery 树，m3b 源与 7650cb7 主树逐文件
SHA 一致 0 diff；unsigned bc4c5ec3→signed 83486466，sign=PASS）。
消费：development-t619.json（HAP 717e8650 记录域，sourceSHA 与当前树一致）。
证据：`test/reports/t594/`（v5-cold-part.jsonl INCOMPLETE 原件 + 页面状态）。

## 执行链

V5 独立研究页 →"新空运行·合成研究"（m5ResearchConfig mode=2 →
**M5_NEW_EMPTY_RUN_REQUESTED ✓**，diagGeneration 2/attempt 4）→ 投沙/加水
（真实来源层意图）→"开启合成信号" →"导出 V5 冷档"。

## 结果：V5 controller 未建立，冷档导出 INCOMPLETE（原件固定）

| 域 | 实测 |
|---|---|
| m5Configure | M5_NEW_EMPTY_RUN_REQUESTED（wire 配置接受） |
| controller 建立 | **停滞 M5_WAIT_PAGE_PULSE**（app_bridge:923——GL 线程 PagePulse
  前不赋 m5Controller） |
| m5ColdCapabilities | **M5_NOT_READY**（API_SEMANTIC，attempt 1/2 两次） |
| 冷档导出 | part.jsonl `T535_V5_ARCHIVE_MANIFEST_V2 state=INCOMPLETE
  reason="V5_CAPABILITY:REJECTED"`（rejectedRaw 原字节保档） |
| XComponent 渲染 | 灰度均值 4.6/std 5.3≈黑（GPU 场未呈现） |

## ROOT/FULL 完整冷档验证：NOT_REACHED

- ROOT/FULL 四档、真实 BASE+SOURCE、paired61 段、cursor64/2920B token、
  Meta2/FULL CONTROL2、双 slot reservation、独立 reader 实跑——全部 NOT_RUN
  （前置=V5 controller 激活，当前被 M5_WAIT_PAGE_PULSE 停滞阻断）
- 编译/声明不代原件（reader 未独立实跑，不冒验证）

## 回 architect（保原件）

- 原始错误：M5 controller 建立停滞于 M5_WAIT_PAGE_PULSE——GL 线程 PagePulse
  未到（surface/EGL 或页面 pulse 链）；XComponent 无渲染输出佐证
- 已试：合成研究激活×2、投沙/加水、等待、冷档状态查询、导出×2（两次
  INCOMPLETE part.jsonl 均保档）
- 所需输入：M5_WAIT_PAGE_PULSE→PagePulse→m5Controller 赋值链的产品侧诊断
  （同 G13 CONFIG_PENDING 域的 GL 线程激活问题，建议 developer 加 m5 域 hilog）
