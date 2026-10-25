# T42 执行案：返修后有效帧与 G0 复测（v1）

日期：2026-10-25。执行角色：effector（真机）。状态：**RUNBOOK_READY / BLOCKED_ON_T41**。
上游：T41（developer，TDATA_LENGTH 返修）。前置交付：T41 提交（含 native 长度契约修正 + `nativeTypedByteLength` 遥测 + `p013_tdata_boundary` 事件）。

## 1. 背景与 T35 遗留未闭环项

T35（回执 `docs/p013-p1g0-retest-receipt-v1.md`，s140）结论：G0 五项中仅 ③z序（TEXTURE 路径，R0 hello 臂）与 ⑤输入透传 PASS；①帧提交 FAIL 且阻塞 ②④：

| T35 遗留项 | 现象 | T41 修复点 | T42 复测臂 |
|---|---|---|---|
| ① 帧提交 | 15,940/15,940 `code=100`，`errorStage=TDATA_LENGTH`，`T_dataHash=0`，EGL 表面从未创建 | DrawFrame 长度判据 `kFloats`→`kFloats*sizeof(float)`（Ark NAPI 返回字节数）；新增 `nativeTypedByteLength/Offset/BufferBytes` 遥测 | R1/R2/R5/R7 重跑 |
| ② sRGB | A/B 层 UNPROVEN（依赖 ①） | 无（修复后自然可采） | R1 diag 臂 + captureProbe 读回 |
| ④ 透明 | 二值 mask UNPROVEN（帧全拒，GLES 区空纹理） | 无（依赖 ①） | R1 TEXTURE 臂盒内/盒外分区判读 |
| ③ z 序 | R0（hello 臂）PASS；renderer 臂（R1）无 GLES 内容，不可判 | 无（依赖 ①） | R1 有内容后完整判读 |

**返修机制说明**（T41 工作区 diff 已见，待提交）：`napi_get_typedarray_info` 在设备上返回的 `length` 为字节数（512 元素 → 2048 字节），与元素契约 `kFloats=512` 比对恒失败；修复后判据改为字节数比对。`TypedView`（DUAL 双场通道）与 `CaptureProbe`（12 元素）同步修正。

## 2. 复测设计（沿用 R0–R7 骨架，判据升级）

复用 `tools/p013_p1g0_retest.ps1` 与 `tools/analyze_p013_p1g0_retest.py`（已含 A/B/C 三层判读骨架）。T41 提交后需先同步两处脚本扩展（见 §4）。

| 臂 | want 档 | 采集内容 | 主判项 |
|---|---|---|---|
| R0 | hello + TEXTURE | 截图 + g0 日志 | z 序基线（全屏 GLES 灰上 fg 洋红）——T35 已 PASS，作回归 |
| R1 | renderer + TEXTURE + diag on | 截图 + g0 日志 + 边界事件 | **①有效帧**：code=0 率、swapSucceeded、T_dataHash≠0、surfaceCreate/contextCreate>0 |
| R2 | renderer + SURFACE | 截图 + g0 日志 | SURFACE 对照：XComponent 恒上层事实仍在（结构性事实登记，不算 FAIL） |
| R3 | hello + TEXTURE + hit 三档 | 输入计数 | ⑤输入透传回归（Default/None/Transparent） |
| R4 | renderer + TEXTURE + diag 同帧 | g0 日志 | ②sRGB：authored 值 vs captureProbe 读回 vs 屏幕采样 |
| R5 | renderer + TEXTURE 前后台 10 次 | g0 日志 | 生命周期：surface/context 创建销毁配对、recovery/stale=0、帧不拒 |
| R6 | renderer + TEXTURE 动态连拍 | 多截图 + g0 日志 | ③z 序完整版：GLES 有内容时 fg 洋红线穿盒可见；帧率健康 |
| R7 | renderer + TEXTURE 长时 90s | g0 日志 | ①持续有效帧：90s 内 code=0 持续、无 1s 级间隙、无 model_error |

## 3. 判据（逐项可执行）

### 3.1 ① 有效帧（R1/R5/R7）

| 判据 | 通过条件 |
|---|---|
| 准入率 | `draw_error` 计数 = 0（或 code=100 占比 < 0.1% 且首帧暖机例外已披露） |
| 边界事件 | `p013_tdata_boundary`：`length=512`、`byteLength=2048`、`nativeByteLength=2048`、`errorStage=null` |
| 遥测 | 末条 `p013_g0_telemetry`：`code=0`、`swapSucceeded=true`、`T_dataHash≠0`、`surfaceCreateCount≥1`、`contextCreateCount≥1` |
| EGL | `OnSurfaceCreated`/`EGL init ok` 日志出现且与帧提交同源 |
| 长时 | 90s 帧间隙 max < 1000ms（H01 冻结门槛），真崩溃/ANR = 0 |

**T_dataHash=0 的语义**：native 未拷贝入参即拒；修复后应逐帧变化（帧内容指纹）。

### 3.2 ② sRGB 三层（R4）

| 层 | 判据 | 方法 |
|---|---|---|
| A 声明 | `queryCompositionProfile` 收据 `actualColorSpace`/`attachmentEncoding` 为显式 sRGB 值 | g0 日志 JSON |
| B 读回 | `captureProbe` 读回值与 authored（46,51,56）一致（容差 ±1，8bit 取整） | g0 日志 JSON |
| C 屏幕 | 截图像素采样与 authored 一致；线性工作空间应读 (118,124,129) 作反例对照 | cv2 采样 |

三层全过 = PASS；任一缺失 = UNPROVEN 并注明缺失层。

### 3.3 ③ z 序（R6，含 GLES 内容）

| 判据 | 通过条件 |
|---|---|
| fg 在上 | 洋红横线穿越沙盒区（GLES 内容区）可见；左侧洋红标记可见 |
| 层级 | zIndex 0/10/20/30 四层中 GLES(TEXTURE) 在 bg 之上、fg 之下 |
| 反例 | 若洋红被 GLES 内容遮盖 = FAIL（回退 T29 结构性结论） |

### 3.4 ④ 透明（R1 截图）

| 判据 | 通过条件 |
|---|---|
| 盒外 | 非沙区棋盘格透出（Canvas 底层可见） |
| 盒内 | 沙盒内 GLES 渲染内容可见（非棋盘、非纯黑） |
| 二值 mask | shader 盒外输出 alpha=0；若盒内 alpha=255 不透明 = FAIL |

### 3.5 ⑤ 输入透传（R3，回归）

TEXTURE 下 hit=Default 不再吞触摸（T35 已登记的行为差异）；None/Transparent 正常透传。无回归 = PASS。

## 4. 脚本扩展要求（T41 提交后、设备执行前完成）

`tools/p013_p1g0_retest.ps1`：
1. R1/R5/R7 臂抓取 `p013_tdata_boundary` 事件（现有 g0 日志已含，无需改采集）。
2. R7 长时臂 90s（T35 p2.ps1 长时模式）合并入本脚本。

`tools/analyze_p013_p1g0_retest.py`：
1. 新增 `p013_tdata_boundary` 解析：`length/byteLength/nativeByteLength/nativeByteOffset/nativeBufferBytes/errorStage` 分布。
2. `draw_error` 码分布已支持（文本行计数）；补充 `swapSucceeded` 统计（telemetry JSON）。
3. 有效帧判据函数化：`frame_admission_verdict(arm)` 输出 PASS/FAIL/UNPROVEN + 依据。
4. captureProbe 读回值解析（R4 diag 臂 `p013_g0_color_readback` 事件）。
5. 沙盒内"有内容"判据增加 GLES 灰底 (46,51,56) 容差匹配（hello 臂基线），排除把灰底误判为无内容。

## 5. 执行顺序

1. T41 提交确认（`git log --grep=T41` 或工作树出现提交）。
2. 同步 §4 脚本扩展并离线自检（analyzer 对 T35 旧产物回跑不崩）。
3. 构建 s142（源码树静默 3 分钟后，登记树指纹），手动签名，安装。
4. 前台执行复测脚本（**禁后台 PowerShell：会被杀**，T35 教训），R0–R7。
5. 分析器判读 → 回执 `docs/p013-p1g0-retest-receipt-v2.md`（v1 不回改，遗留项逐项销项）。
6. G0 五项全 PASS → 报 T23/manager 复裁；任一 FAIL → 缺陷反馈卡（按 §6）。

## 6. 缺陷反馈规则

- 设备行为与判据冲突时，先区分"实现缺陷"与"判据/预期错误"（T29 教训：B3 同 order 是碰撞应 INVALID_LEDGER）。
- 新缺陷按 P0/P1 分级建卡交 manager 派工；本执行案不自行改判据或协议。
- 采集失败（脚本/注入/设备）记 BLOCKED 并附失败点，不补造数据。

## 7. 身份与限制

- 构建版本：T41 提交后的首个干净树构建（预计 s142），登记源码树 SHA256 指纹与 HAP SHA256。
- 真机：VYG-AL00（62T0225B10010020），USB 弹窗先关，防熄屏 `power-shell timeout -o 600000`。
- 不使用真实传感器数据（测试模式）；重力/摇晃由 want/手势构造。
- 本执行案不预先宣布 G0 通过；五项判读以回执为准。
