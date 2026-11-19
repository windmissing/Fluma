# T600 v2 · 目标 11 真实通量视觉组合独立 GPU 验证（受阻归因）

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T600-composite-signed.hap（正树 T566 g11vis+T582 修复，SHA 334fd90f…，
已在档部署）。
证据：`test/reports/t491/t600c/`（页面状态 l1–l7 + state.jpeg）。

## 组合链现状（真机+源码两路证据）

1. **g11vis 模块已编译**：g11vis/{cold,kernel_assets,scene,visual_transport}.cpp.o
   在 foundation（11-18 17:20）；组合生成链可运行
   （`test/t566_generate_composite.cjs` → R1 pipeline sha256=eb7bd68b67037d39…
   source=19/quantum=2543、DM sha256=364381fcb1cfc736… source=19/quantum=2923，
   与 T600 卡面 R1 19/2543、DM 19/2923 数一致）
2. **组合执行入口缺失**：g11vis 四个编译单元在 g9spf/g9dm/m3b/app_bridge 均无
   消费者引用（grep 零命中）——VIS dispatch arms（512–516）没有任何产品页/
   controller 路径可以触发；R1/DM 组合 pipeline 无法被产品执行
3. **G9 页研究入口被首拒阻断**（T590 v1 记录的 G9_REAL_TOUCH_CONFIG_REJECTED
   / expectedN=1 ELEMENT_COUNT），真机复测确认同拒仍在
4. M1/M2/SandCanvas 各页均无 G11 组合触发路径（昨晚 NEWPIPE_FAULT 场景确认
   SandCanvas 为 CPU CA 域，与 G11 GPU 组合无关）

## 判定

**G11 组合真机执行 NOT_REACHED——组合 wire 缺失（产品实现缺口）+ G9 首拒双重
阻断**，按卡面"失败先 architect"回 architect：

- 缺口 1：T566 交付为 g11vis 模块+生成链，**dispatch arms 无消费者接线**
  （需要 developer 在 g9spf/g9dm controller 或 app_bridge 增加组合触发端点）
- 缺口 2：G9_CONFIG64 N1 首拒（T590 v1 未知根因）阻塞 G9 页全部研究入口
- 不以编译/生成链成功冒视觉 PASS；R1/DM 组合实测帧、逐 word/同 publication
  像素验证、同 parent 回归、bit18/19/3bank/barrier/lease 全部 NOT_RUN

## 与 T601 关系

T601（effector 观感验证）前置="T600 真实非空组合帧"——本卡未产出非空组合帧，
T601 继续等待。
