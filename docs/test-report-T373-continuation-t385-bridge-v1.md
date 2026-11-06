# T373 续（T385 桥接修订）· 默认 ROI 冒烟 PASS + 三区取证被新发现阻断（渲染冻结，交 architect）

- 承接：T383 归因（N-API 字节/元素转换错误）→ T385 桥接修订签包 `db9e3ae46798642526703c567a18c138f92e8c66925f68317f81b53e3d607f68`（卡面确认解锁）。
- 路由：`p051v2=1` 隔离单沙页，pid 10694。证据：`test/reports/t373b/`（17 文件，SHA 见 `t373b_verdicts.json`）。

## 步骤① 默认 ROI 冒烟 — PASS（T383 修订真机证实）

『预填默认 ROI』→『预约一次』（状态『等待下一次有效绘制』）→『导出读回』——**首次真机 CAPTURED**：

| 契约项 | 预期（T385 developer.md） | 实测 |
|---|---|---|
| JS 字节长度 | 4/16B | **jsByteLength=16** ✓ |
| 原生字节长度 | 16B | **nativeRawByteLength=16** ✓ |
| 归一元素 | 4 个 | **requestedRoiWords=[0,0,16,16]** ✓ |
| 两份各 1024B | candidate/back 各 1024B | **均 1024B** ✓ |
| SHA | manifest 记录 + sidecar | candidate/back SHA `b204cdad…`（与 manifest 一致；空场背景两读点相等合理）、sidecar 对 manifest 本体 ✓ |
| token/state | 同 token CURRENT | **requestedToken==actualToken、CURRENT** ✓ |
| captureMs | — | 8.70ms |
| 请求 ID | — | requestId=1 |

T373 首轮的 `CAPTURE_FAILED/INVALID_ROI`（合法 ROI 被拒）已由 T385 修订**在真机消除**。

## 步骤②③ BED/AIR/BG 三区 — BLOCKED（新发现，交 architect）

- 注入 → 锁定 → 『预填 BED/AIR/BG』→ **『夹具未就绪：需床、AIR 和背景』**（两次：首次 15 粒无床；加重注入 16 轮 + 150s 沉降后仍无床）。
- **新发现（渲染冻结）**：解锁（『恢复仿真』）后注入，物理层活着——材质收据 **count=203、physicsCommitId 持续上涨、CURRENT、swapSucceeded=true**——但画布**逐位冻结**：s4（注入后）与 s5（重锁定）画布区 mean-abs-diff=**0.0**，三轮截图均为同 ~15 粒对角线分布。锁定→解锁→重锁定后**渲染不恢复实时**：物理/publication 在推进（packet 计数 203 vs 画面 15 粒），swap 报成功但系统合成画面不变。
- 尝试：加重注入 16 轮、150s 沉降、解锁/重锁定循环——画布均无变化。床无法形成 → 三区预填持续『夹具未就绪』→ 中性/纹理三区读回不可执行。

## 移交 architect / developer

1. **渲染冻结根因**（新）：单沙页锁定→解锁后，swap 报成功、packet 更新，但 XComponent(TEXTURE) 系统合成画面逐位冻结。嫌疑：native swap→合成器提交链、或 fixture 帧缓存未失效。判别入口：`single_sand_pass.cpp` Draw/Swap 后 buffer 提交、`prepareSingleSandFrame` 每帧重打包路径。
2. 修复后 T373 三区（BED/AIR/BG）+ 中性/纹理读回可一次补齐（预填按钮在位，契约已验证）。

## 边界

未改业务/native；预填按钮替代手输（T373 首轮的 uitest 文本注入缺陷已被 T385 预填路径绕开）；渲染冻结现象三重证据（截图逐位 diff=0.0、receipt 计数矛盾、夹具未就绪复现）如实登记。
