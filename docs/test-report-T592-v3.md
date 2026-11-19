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


## v3b 追加（2026-11-19 19:57，冷导出精确字段锁定）

按 T664 handoff 要求取得**同 attempt FAILED prepare 冷导出原件**：
`t540-prepare-s1795089517841-1-1-1-PREPARING-18.json`（T659_PREPARE_OWNER_VALUE_V1，
prepareId=1，observationSerial=18）+ FAILED-25.json（reason=
PREPARE_TIMEOUT:KEYBOARD_VISIBILITY_UNKNOWN——另一次运行的拒因，键盘可见性
判定域）。

**精确失配字段锁定**（v3b-field-lock.json）：
- `raster.geometry=9` vs `raster.native.geometry=7`（snapshot.nativeGeometry=7）
- **native 侧几何确认落后 UI raster plan 2 个 generation**（9 vs 7）
- 其余域全部一致：rasterW/H=1154/1644、vp2px=3.5、nativeReady=true、
  didLayoutSerial=301>requestedAtLayout=300（布局门槛满足）
- 证实 T664 判定：非协议名 vs generation 误读；真实谓词失配对=
  **native geometry ack 滞后**

回 architect：native 侧 geometry ack 路径（9→7 滞后）为精确工程域；按 T664
警示不盲目改 publishRaster 推进策略（geometry-only 反馈回避），由
architect→developer 精确分流。
