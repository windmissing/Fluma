# T373 单沙真实窗口颜色链验证（真机）— 部分完成 + 具体阻断交 architect

- 卡面：按 T370 核中性/非零 gain 及颜色校准、同 token candidate/default-back 及实际 swap；系统截图单列；失败交 architect。
- 对象包：T372-signed.hap SHA256 `98339521d6eea6ab58fe6799167d6ca7c78ba350af3243665d94b91359066992`（T372 交付，卡面确认正式解锁）。设备 pid 43554/55252/59884（三次冷启，见下）。
- 证据目录：`test/reports/t373/`。

## 已达成

### A. 中性 / 非零 gain 切换（PASS）

材质收据三连（`receipt3.json` → `receipt_neutral.json` → `receipt_texture.json`，SHA `3782a7ac…`/`721a32dd…`/`c5676294…`）：

| 状态 | actualConfigDigest | generation | count | token 相等 | state |
|---|---|---|---|---|---|
| 纹理（生产 gain） | `f10927bd34d1be04…` | 1 | 246 | true | CURRENT |
| 中性（四 gain 置零） | `02ae5ec0236d1e53…` | 2 | 246 | true | CURRENT |
| 纹理（恢复） | `d23234e1f1038ea4…` | 3 | 246 | true | CURRENT |

- 中性切换 digest 变化、generation 单调递增、count 不变（物理不受影响）、每次同 token CURRENT——与 T372 developer.md §2 契约一致（每次切换更新 digest/revision/generation，不要求恢复原值）。
- grainHash 恒 `3b0a40d6…`（T365 独立复现已证）；shaderHash 恒 `f22a5e86…`（T364 一致）。

### B. 单沙场景构造（PASS）

- 隔离页 `useSingleSand=true` 每帧 `drawSingleSandScene(prep.packet, …)`；触摸注入 10-14 轮 swipe（500,2000→900,2200 px）后 count 0→246/269/239（`receipt3/4` 与冷收据 probe）——粒子出生与单沙渲染管线工作。
- 预算冷档 `budget0.json`（SHA `8af4a0f0…`）：重置后空窗 Q=256/A=0/live=0——注入未到达时零采纳正常形态。

### C. INVALID_ROI 判定路径（PASS，契约内反例）

- uitest 文本注入缺陷导致 ROI 字符串损坏（见阻断①）→ 点「预约一次」→ 状态 **『ROI 无效』**（`parseWindowRoi` 拒绝，截图 `s17_invalid.jpeg` SHA `59ec99ec…`）——契约「越界记作 CAPTURE_FAILED/INVALID_ROI」的 JS 侧判定实测。

### D. 系统截图单列（达成）

`s0_initial/s1_after_inject/s2_state/s17_invalid` 等按会话时序单列存档，未与读回数据混用。

## 具体阻断（交 architect / developer，按卡面「失败交 architect」）

### 阻断①（产品捕获路径缺陷候选）：默认 ROI 真机 CAPTURE_FAILED / INVALID_ROI

- 冷启后使用**默认合法 ROI `0,0,16,16`**（未做任何文本编辑）：预约成功（状态『等待下一次有效绘制』，`layout8.json` SHA `ede6698a…`）→ 点「导出读回」→ 状态 **『CAPTURE_FAILED t372-1793948058304-1-177.json』**。
- 冷收据 `t372-cold-receipt.json`（SHA `9c288098…`）+ sidecar `t372-cold-receipt.sha256`（SHA `859bb77f…`）：`probe` 完整（token==、CURRENT、count=239、packet.width=1178/height=1186、scale=3.5），但 **`captureStatus=CAPTURE_FAILED`、`captureStage=INVALID_ROI`、`captureMs=0`、`rois=[]`、`candidate=null`、`back=null`**——fail-closed 正确（无部分数据），但合法界内 ROI 被拒。
- 源码定位（`test/t372/`）：`Fits(1178,1186)` 对 (0,0,16,16) 数学上应 PASS（0≤1162、0≤1170）；`captureStage=INVALID_ROI` 是 n==5 分支的初始值，Configure 失败或 5 参调用未达 native 均可产生该形态。JS→NAPI 参数桥（`DrawSingleSand` n==5/typedarray 读取）或 pendingRoi 时序为嫌疑层。**host GLES pbuffer 自验（T372 developer.md）未覆盖此真机路径**。
- 判读入口：`window_probe_napi.inc` DrawSingleSand 参数读取 + `window_probe.h` Configure/Fits + controller patch 的 pendingRoi 生命周期。

### 阻断②（工具级，非产品）：uitest 无法注入 ROI 文本

- `uiInput text` 字符随机丢失（多次试验）、`keyEvent 2055`（DEL）在 TextInput 无选区时无效、`2056`（FWD_DEL）仅在全选后有效、`keyEvent 2042`（DPAD_RIGHT）污染输入为字母 `z`、`inputText` 坐标形式无效果、长按菜单「全选」点击仅一次成功。
- 导致无法把默认 ROI 编辑为床/AIR/背景三区坐标。**前置**：developer 在隔离页增加固定 ROI 按钮（或 prefill 多组 ROI）绕开 TextInput 键盘注入；接手后本卡三区读回可一次补齐。

## 未达成（因阻断①②）

- 同 token candidate/back 双目标 RGBA 原始读回与 SHA 校验（candidate/back=null）。
- 中性/纹理下 ROI 像素级颜色链判定（读回数据缺失）。
- 实际 swap 后窗口默认 back 与 candidate 的颜色差判读。
- 三区（床/AIR/背景）覆盖。

## 角色检查与移交

- tester：真机取证、判据核验、证据归档——已完成可做部分；两阻断均非 tester 可修（红线：不改业务/native）。
- architect：阻断①根因判读（native/bridge 层）。
- developer：阻断②前置（固定 ROI 按钮/prefill）+ 阻断①修复。

## 证据清单（SHA256 前 20 位）

t372-cold-receipt.json `9c288098ada747191951` / .sha256 `859bb77f9dadf9bc6602` / receipt0 `dd88dbb263ba9d6b886f` / receipt3 `3782a7ac86bc51d81ac9` / receipt_neutral `721a32dd5abebfa37043` / receipt_texture `c5676294d4b7eead4b0f` / receipt4 `1731a60c36ebf1bf6f6b` / budget0 `8af4a0f0715ea6bf1ad4` / s0 `aade6ee81b31d5e5eff0` / s17 `59ec99ecb46b07bc8548` / layout8 `ede6698a98cd22bbfd99`（其余 layout/s 截图按文件名在目录内）。
