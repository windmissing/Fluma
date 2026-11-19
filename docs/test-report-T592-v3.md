# T592 v3 · 单沙预约 HOLD 续证——T659 修复真机验证 + PREPARE_TIMEOUT 新拒因

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T592-fix9-signed.hap（t659-delivery-source product 树=T659 9b0959b3 修复
新构建，inputSha f3a0beb3→signed 8a323863，sign=PASS；按 T659 要求不复用旧
signed 包）。
证据：`test/reports/t491/t592b/`（fix9-screen1–5.jpeg 序列、v3-fix9.json）。

## 执行链与结果

| 步骤 | 实测 | 判定 |
|---|---|---|
| 加载 | P051DerivedV2Demo CONFIGURING·IDLE，软键盘弹出 | ✓ |
| 确认固定键盘 | 按钮变"**已确认固定键盘**" | ✓（响应式合同成立） |
| **收键盘并准备** | **进程存活（pid 6577 稳定）——无崩溃**；v4 布局进入（预约/归档/时钟/状态/封存/取消/释放/配置全出） | **✓ T659 修复真机验证成立**（v2 的 JSON `.same()` TypeError 崩溃消除） |
| prepare 结果 | **FAILED PREPARE_TIMEOUT:RASTER_NATIVE_MISMATCH · IDLE** | 新具名拒因（下游） |
| 拒因明细（上屏） | rasterRevision=`T532_WINDOW_HOST_RASTER_V3` vs planRevision=`9` 失配；rasterWidth 1154/rasterHeight 1644 | 原件已存 |
| 预约 | 点击后状态已采（fix9-screen5.jpeg） | HOLD 链在此新门前 NOT_REACHED |

## 判定

- **T659 修复验证 PASS**：owner 纯值身份/票据链修复在真机生效——prepare 路径
  崩溃彻底消除，观察窗口正常进入 v4 布局
- **新拒因 PREPARE_TIMEOUT:RASTER_NATIVE_MISMATCH**：raster host（T532 窗口
  栅格 V3）与 plan revision 9 配对失配——回 architect/designer 对照
  raster-native revision 配对合同；这是 HOLD/严格 PNG 链的当前唯一门
- 崩溃域（T592 v2 根因）正式关闭

## 回执

T659 developer 交付验证通过（本报告即验收输入）；T592 续测脚本就绪——
raster-native 配对修复落地后完整 HOLD/严格 PNG 序列即续。
