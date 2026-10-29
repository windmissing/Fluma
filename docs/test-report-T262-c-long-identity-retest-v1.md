# T262 明水 C 长身份真机定点复测 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-29 · 状态：**PASS（定点范围）**
- 被测包：**T261-signed.hap**（T258 固定源码 `408c457` 重建签包，SHA `419f36459af356276e3485b712d0b1611a57b72b738c1c7b29d414a52040eaf9`；T257 授权链签名，`verify-app` PASS）
- 页面：`pages/ClearWaterDemo`（`--ps cwdemo 1` want 路由，XComponent id=`p013_clear_water_surface`）
- 对象：T252 FAIL 根因（CW frameToken 999B 超 napi `String()` 255B 上限 → 每帧 `CW_V3_PARSE` 拒绝）的修复定点复测，沿 T252 失败报告三项合同接缝
- 归档：`test/reports/t262/`（t262a–l 截屏 + t262g.json 布局 dump）

## 定点复测结果

### ① CW_V3_PARSE 长身份读取 —— **PASS（原缺陷消除）**

| 项 | T252（s144） | 本轮（T261 包） |
|---|---|---|
| 状态文字 | `FAULT_CW_V3_PARSE`（每帧复现） | **`CURRENT`**（全程未见任何 FAULT） |
| T252DBG error 日志 | 连续多条 draw fail | **0 条**（打点仅在 draw 失败时输出；无日志=全部 `currentFrameSucceeded`） |
| hilog tag | T252DBG code=100 INVALID_ARGUMENT stage=CW_V3_PARSE | 无 |

T258 的 `clear_water_identity.h`（查询实际 UTF8 长度→8192/4096 checked 分配→完整读取核验）在真机 ArkTS↔native 真实接缝上工作正常：页面从加载起持续 CURRENT，长时间运行（14:54 启动至 15:10+）无一次 parse 拒绝。**T252 根因确认修复。**

### ② 六层同帧实际像素 —— **PASS（GPU 六层链路出内容）**

- 点击画布注入（uinput -T click）：**每粒注入的沙都在截屏中可见**（RGB≈160,165,118 沙色像素簇），6 粒位置与注入点一一对应（`t262d.jpeg` 聚类：5 粒新注入 + 1 粒先注）。
- 画布底色 RGB(18,22,31) 为 GL clear 背景层输出——**swap 链路在真实 window 上工作**（T252 时代画布为全白无 GL 输出）。
- 切回沙源后再次注入又出粒子（`t262l.jpeg` 中央白点），证明帧持续提交、非冻结画面。
- 渲染可见性分级：**沙粒子层可见**；水层（"明水"）注入后未观察到独立水色像素——页面自身标注"水直接加入水层；**参数待定标**"，透明水在当前定标下视觉不可见属**待定标参数域**，不判 GPU 故障（背景层在画、粒子层在画、无 FAULT 回执）。

### ③ 切源 ACK / 旧代隔离 —— **PASS（UI 级 ACK；设备侧 receipt 通道无导出）**

- 点"水"：状态行 `CURRENT · 当前源: 沙` → **`CURRENT · 当前源: 水`**（截图对比 status 行 hash 变化），无 FAULT/STALE ✅
- 切回"沙"：恢复 `CURRENT · 当前源: 沙`，注入仍生效 ✅
- 源 controller 语义（`select()` 内部先 `stop('KIND_SWITCH')`）：切换全程渲染回执保持 CURRENT，与"旧源终态隔离不炸当前管线"一致。
- **如实边界**：`ClearWaterSourceReceipt` 的 terminal disposition（ACCEPTED/admitted/transactionId/kindSuccessSeq）存在于内存 receipt 对象，本包无日志/导出通道，设备侧**逐笔 ACK 字段未读取**——该层在 T244 离线真实驱动已覆盖（terminal ACCEPTED）；本卡以 UI ACK + CURRENT 回执 + 像素呈现为设备侧证据，不冒充 receipt 字段级核验。

## 定点结论

| T252 合同项 | 复测结果 |
|---|---|
| ①真实 window 六层 GL 同 token CURRENT | **达成**：无 parse 拒绝、持续 CURRENT、GPU 输出上屏 |
| ②swap 后同 token 像素有内容 | **达成（沙层）**：粒子像素簇与注入点对应；水层视觉待定标（页面自注） |
| ③切源 ACK/旧源隔离 | **达成（UI 级 + CURRENT 回执）**；receipt 字段级证据仍以 T244 离线为准 |

## 边界（如实）

- 离线 23/0（T258 Node N-API ABI 测试）与签名通过不作为本报告判据；本报告全部结论来自真机运行时观测。
- 未重跑 T218 整矩阵（按卡约定）。
- frameToken 长度未直接量测（999B token 无设备侧打印），以"原 100 code 每帧复现 → 0 次复现"的行为差异为修复证据。
- 水层可见性：待定标参数域，非本卡缺陷；如需水迹可见性验收须另立参数定标任务。
- 主观体验（水质感/沙形态）不在本卡范围。

## 失败上报

无——定点范围全 PASS。

## 证据

- `t262a.jpeg`：启动后 CURRENT·当前源：沙（对比 T252 FAULT）
- `t262b/c/d.jpeg`：沙注入→单粒→6 粒聚类（像素坐标与注入点对应）
- `t262e/f/h.jpeg`：切水按钮 → `CURRENT·当前源: 水`
- `t262k/l.jpeg`：水期间/切回沙后再注入粒子可见
- `t262g.json`：按钮精确 bounds（沙[332,456][500,596] 水[528,456][696,596]）
- 运行时：pid 53665，14:54:34 加载 `pages/ClearWaterDemo`，XComponent `p013_clear_water_surface` native OnSurfaceCreated，进程存活至采集结束
