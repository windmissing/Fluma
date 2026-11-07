# P-013 P0a hello-surface 真机确认收据 v1

任务：6adc4394e4b06ee35718609d —— effector 真机运行确认（EGL 上下文 / 最小渲染帧 / 帧时遥测）。
设备：VYG-AL00（62T0225B10010020），OpenHarmony 7.0 / API 26。
构建产物：`entry-default-s137-signed.hap`（含临时 harness 门 `--ps p013page true`，**不提交主干**）。
数据目录：`test/artifacts/p013-measurements/`。

> 红线：本确认基于 s137 临时门（harness-only）产出的试点数据，不等同 P0 主体正式签核；
> `EntryAbility.ets` 的 p013page 门为临时调试开关，严禁进入 main。

## 验收② EGL 上下文 / 最小渲染帧 / 清屏色

| 检查项 | 结果 | 证据 |
|---|---|---|
| EGL 初始化 | ✓ GLES3，vendor=HUAWEI | `EGL init ok (GLES3); vendor=HUAWEI`（hilog） |
| XComponent 表面尺寸 | ✓ 1280×1559 | `OnSurfaceCreated: 1280x1559` |
| 最小帧 swap | ✓ 28 次心跳全部 ok | `hello frame #N swap ok` 第 #1 → #8101，无失败 |
| 清屏色 | ✓ 深灰 (46,51,55) | Phase B 截图像素值（s137 运行截屏） |
| napi 桥 | ✓ 仅 1 次瞬时 napi_unwrap | `napi_unwrap OH_NativeXComponent failed`（注册瞬间，不复发） |

**结论②：PASS。** EGL 链一次性成功，最小渲染帧连续 28×300≈8400 帧无掉 swap，清屏色正确。
单条 `napi_unwrap` 错误与 Phase B 结论一致——属 XComponent 注册时序的瞬时事件，运行期不复现。

## 验收③ 帧时遥测 + 前后台 RSS 曲线

### 帧率（前景稳态）
- 冷启动后前两个心跳区间偏快（63 / 83 fps，启动期）。
- **稳态区间（frame #601 → #3901，跨越整段 60s 前景窗口）：每 300 帧恒定 5009 ms = 59.9 fps。**
- 设备 vsync 调度下前台稳定 60 fps，稳态零掉帧。

### 前后台 10 次切换（bg/fg）
- 切换期间（frame #3901 之后）出现 35–38 fps 区间——因 Home 键将应用切后台、`postFrameCallback` 暂停，恢复后心跳跨后台暂停期，属预期行为。
- 每次 `aa start` 恢复后帧率立即回到 60 fps，无卡死、无崩溃（全程 pid 恒定 64449，无重启）。
- XComponent 表面在 10 次 bg/fg 中**始终存活**（仅冷启动 1 次 `OnSurfaceCreated`），surface 重建路径未被触发——证明持久表面路径稳定。

### 内存（RSS，/proc/<pid>/status）
| 采样点 | VmRSS (kB) | VmHWM (kB) |
|---|---|---|
| t60 稳态 | 108868 | 122556 |
| cycle1_bg / fg | 114460 / 116108 | 122556 |
| cycle2_bg / fg | 116660 / 116704 | 122556 |
| cycle3_bg / fg | 115432 / 115512 | 122556 |
| cycle4_bg / fg | 115968 / 116424 | 122556 |
| cycle5_bg / fg | 115484 / 115912 | 122556 |
| cycle6_bg / fg | 116092 / 116640 | 122556 |
| cycle7_bg / fg | 115684 / 116004 | 122556 |
| cycle8_bg / fg | 116380 / 116836 | 122556 |
| cycle9_bg / fg | 115996 / 116404 | 122556 |
| cycle10_bg / fg | 116800 / 117292 | 122556 |
| final_fg | 118072 | 122556 |

- 高水位 VmHWM 全程恒定 **122556 kB（122.5 MB）**——首段预热后峰值不再抬升。
- 稳态 fg 采样紧密聚簇 115–117 MB，无随周期单调增长；t60（108.9 MB）偏低仅为冷启动尚未填满缓冲池。
- **结论③：PASS，无内存泄漏。** 10 次前后台切换 RSS 平稳，高水位恒定。

### 应用错误
- `com.example.helloworld` ERROR 级日志 = **0**（除上文单条瞬时 napi_unwrap）。

## 验收③补充：旋转 viewport（代码验证 + 设备限制）

- `OnSurfaceChanged` 实现正确：重查 `OH_NativeXComponent_GetXComponentSize` 更新 `g_width/g_height`，下一帧 `DrawHelloFrame` 的 `glViewport(0,0,w,h)` 即生效（hello_surface.cpp:178-188）。
- **设备限制**：本机 shell 无 `wm` / `settings` / `display` / `hc` 等旋转指令，无法在自动化中强制旋转；物理旋转不可脚本化。
- 旋转相关的 surface 生命周期（destroy→recreate→EGL 重初始化）代码路径正确，且持久表面路径已通过 10 次 bg/fg 证明稳定。
- **处置**：旋转 viewport 留待 P0 主体页面在真机手动旋转复核，或单独旋转测试 harness。本项不阻塞 P0a 工具链确认。

## 总判定

| 项 | 状态 |
|---|---|
| ② EGL 上下文 / 最小帧 / 清屏色 | PASS |
| ③ 帧时遥测（60fps 稳态） | PASS |
| ③ 前后台 10× 切换无泄漏 | PASS（RSS 平稳，VmHWM 恒定） |
| ③ 旋转 viewport | 代码验证 PASS；设备强制旋转不可自动化，推迟 P0 手动复核 |

**P0a 工具链全通确认完成**：首个 native 模块（XComponent + EGL + GLES3）在真机跑通——构建链、EGL 上下文、最小渲染帧、帧时遥测、前后台生命周期均达标。可移交 P0 主体开工（见 toolchain-notes §9 路标）。

## 数据产物
- `test/artifacts/p013-measurements/p013-s137-telemetry-hilog.txt`（50674 行全量 hilog）
- `test/artifacts/p013-measurements/p013-s137-rss-curve.txt`（RSS 曲线）
- `test/artifacts/p013-measurements/p013-telemetry-run.log`（脚本运行日志）
- `test/artifacts/p013-measurements/p013-s137-run.jpeg`（Phase B 清屏色截屏）
- Phase B 收据：EGL init / 心跳 / 清屏色像素值（46,51,55）。

## 遗留 / 移交
1. 清理临时 harness 门（`EntryAbility.ets` p013page 分支）后方可合入主干。
2. 旋转 viewport 真机复核（P0 主体）。
3. s137 HAP 仍装在设备上，确认完毕可 `aa force-stop` 收尾（不影响）。
