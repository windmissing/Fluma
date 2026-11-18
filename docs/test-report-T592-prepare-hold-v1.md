# T592 v1 · 单沙预约 HOLD 与严格 PNG 标定续证（T598-final v4 包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T598-final-signed.hap（T598 9ec800f 单权威 v4/响应式 reserveEnabled/
fresh ticket→reserve→RESERVED 后 consume，SHA
8edc11ee2c898375a60757f46af99ce18b25c60a7f4027c82a8434e0c793c741），
bm clean 后冷启，PID 6277。
证据：`test/reports/t491/t592a/`（prepare-failed-layout.json 两轮复现 + obs_failed.jpeg）。

## 执行链与结果

1. M2 页 → **单沙显式冻结截图 V2**（P051DerivedV2Demo，T598 v4 页）：
   CONFIGURING · IDLE，"窗口读回未预约"
2. 点"确认全屏手机固定键盘"→ 文本变"**已确认固定键盘**" ✓（fixedKeyboardConfirmed
   响应式生效）
3. 点"收键盘并准备"（t413-prepare，id 在档）→ **稳定复现
   `FAILED PREPARE_OBSERVATION_FAILED · IDLE`**（两轮独立尝试均同）
4. prepare 后按钮区切到 v4 布局（预约/归档/时钟/状态/封存/取消/释放/配置——
   **T598 单权威布局切换生效** ✓），"预约"按钮在档但 **enabled=false**
   （reserveEnabled 响应式要求 prepareGate.state==='READY'，未达）
5. 3000ms 内无凭空 READY TTL（符合 T598"原 3000ms 仅 PREPARING"合同）

## 判定

- **正向预约链（真实点击→reserve 回执→RESERVED/HOLD→原生严格 PNG→冻结映射标定）
  被 PREPARE_OBSERVATION_FAILED 阻断**——NOT_REACHED（如实，不以旧 V3 READY 或
  JPEG 冒新正证）
- T598 v4 部分合同真机验证 ✓：确认键盘响应式、prepare 后单权威按钮布局切换、
  reserveEnabled=false（非 READY 不启用）、无凭空 READY
- 首拒 PREPARE_OBSERVATION_FAILED 在 prepareGate.observe 链 catch（窗口读回/
  avoid area 观察）——**回 architect 精确缺口**：需确认键盘收起后窗口读回为何
  抛异常（PrepareMainWindow.value/observe 链），不凭缺 code 派物理返修
- 未快点/未改坐标绕 disabled；未放宽像素容差；无 JPEG 冒 PNG
