# P051 T74 触摸零源设备补采效果报告 v1

日期：2026-10-26。责任：effector。任务卡：`6adeb1e9e4b017ab33f703e0`。技术路线：`P051_TOUCH_TO_SOURCE_V1`（干场候选 DRY_AIR_V1，T73 离线 17/17 PASS 后的设备侧取证）。  
状态：**DEVICE_CAPTURED / FIRST_PROVABLE_FAILURE_AT_L1_RECEIVE / RENDER_ALIVE / NOT_A_PRODUCT_PASS**。  
按 T69 §7.2 分流表，本轮结果落入第一行分支：**交 T71 实施 §5 独立无绘制输入节点**。

## 0. 环境与构建身份

| 项 | 值 |
|---|---|
| commit | `545f45e`（HEAD：T72 被动诊断 `0c86c47` + T66 收尾 + T55/T75 材质；含 T59 干场 DRY_AIR_V1） |
| 隔离构建 | `D:/hmprojects/t44-build` worktree（与并行会话未提交改动隔离；产品代码零修改） |
| entry/src 树指纹 | `5b34bc45ce9cc5b7` |
| HAP | `entry-default-s143-signed.hap`，SHA256 `babc44c65f8fae41daecfd8f911b0bb53d01261a12d6e9c11bb1395061c9d440`（4,454,646B） |
| 设备 | VYG-AL00，hdc `62T0225B10010020`，API 26 |
| 执行器 | `tools/t74_touch_source_capture.ps1`（五臂）+ `tools/analyze_t74.py` |
| 产物 | `test/artifacts/t74-touch-source-capture-20261026/`（15 文件：五臂 hilog dump、9 截图、layout、analysis JSON、console log） |
| 执行窗口 | 2026-10-26 14:16:31–14:18:51（五臂共约 2m32s） |
| 渲染合同 | 仅 XComponent+shader 新管线；无 Canvas 路径参与（本页本就无 Canvas） |

## 1. 事件链逐级判读（五臂汇总）

判读链：L1 RECEIVE(touch) → L2 MAP/START(model.start) → L3 CLOCK(tick) → L4 REQUEST(dry.request) → L5 RECEIPT(dry.receipt) → L6 PUBLICATION(metrics) → L7 RENDER(present)。

| 级 | 判据 | R0 | R1 | R2 | R3 | R4 | 结论 |
|---|---|---|---|---|---|---|---|
| L1 RECEIVE | `touch` 诊断事件 | **0** | **0** | **0** | **0** | **0** | **FAIL（首失败点，五臂一致）** |
| L2 MAP/START | `model.start/move/stop` | N/A | N/A | N/A | N/A | N/A | 未达（上游断） |
| L3 CLOCK | `tick` 活跃、stale=0 | PASS(484 tick, sensor 源) | PASS(435) | PASS(482) | PASS(487) | PASS(693) | 页面帧循环健康，零 stale |
| L4 REQUEST | `dry.request` | 0 | 0 | 0 | 0 | 0 | 未达 |
| L5 RECEIPT | `dry.receipt` | 0 | 0 | 0 | 0 | 0 | 未达 |
| L6 PUBLICATION | metrics 非零 | 全零(CANONICAL) | 全零 | 全零 | 全零 | 全零 | 与零源一致 |
| L7 RENDER | present code/swap | 483/483 swap | 434/434 | 481/481 | 486/486 | 692/692 | PASS（管线活着，status 文本"新管线：按住发沙，松手停止"） |

**首个可证失败点 = L1 RECEIVE**：页面在前台、帧循环/传感器诊断/present 全部活跃，但五臂所有触摸注入（R1 同点按住 3s、R2 中心拖动、R3 右壁、R4 reset 后再拖）产生 **0 条 `touch` 诊断事件**。T72 诊断 hook 就挂在页面 `touch()` 入口第一行——零行即 `onTouch` 从未被调用，不是"收到但被丢弃"。

## 2. 命中区域核实（排除"点错地方"）

`uitest dumpLayout`（R4 臂，`R4-layout.json`）：

| 节点 | bounds (px) | hitTestBehavior |
|---|---|---|
| Stack（容器） | `[51,624][1229,2650]` | Default |
| **XComponent** `p013_lmat_surface` | `[51,624][1229,2650]` | Default |
| Button 重置为空场 | `[444,456][836,596]` | Default |

注入坐标全部落在 XComponent 区域内：R1(700,1200)、R2(400→850,1200)、R4 同 R2；R3 右壁(1160→1220,1200→1700)在区域内右缘（右界 1229）。`touch` 事件坐标为 local vp，但事件入口在 local 映射之前——只要 onTouch 被调用就会有诊断行，与坐标映射正确性无关。**命中区域证据充分。**

## 3. 交叉验证（排除"注入工具不工作"）

- 每次冷启后 `uitest uiInput click 240 620` 关 USB 弹窗均生效（采集流程未被弹窗阻断）——注入工具本身对系统 UI 有效。
- R0 无触摸臂与 R1–R4 触摸臂的诊断流形态完全一致（tick/sensor/present 照常），触摸臂唯一差异就是预期中的 touch 行没有出现。
- 截图像素审计：容器区（layout bounds 缩放采样）**非背景像素 0/10000，全部八张截图一致**；R1-holding 与 R1-after、R2-dragging 与 R2-after MD5 逐位相同（`2ee0f5e749`）——触摸期间与之后画面零变化，与零源零准入一致。
- R4 的"重置按钮"click(300,260) 实际落在按钮 bounds `[444,456][836,596]` 之外（脚本估算坐标失误），该子步对 L1 结论无影响（R4 的 swipe 注入独立成立）；已如实记录，不计为按钮失效证据。

## 4. 结论与移交（对齐 T69 §7.2 表格第一行）

| T69 §7.2 分支 | 本轮证据 | 判定 |
|---|---|---|
| "活跃页面、内容区命中证据充分，却无 RECEIVE，且仍是 libraryname＋空 native 触摸桥" | 全部满足：页面活跃（L3/L7）、命中区内注入（§2）、0 RECEIVE（§1）、`napi_bridge.cpp` callbacks 第四项仍为 nullptr（commit 545f45e 未变） | **成立 → T71 实施 §5 独立无绘制输入节点**（同内容区 Stack 覆盖 + raw onTouch + 单指状态机 + 逆映射） |

附注（不改变分流）：
- T69 §4.1 的平台契约缺口（带 libraryname 的 XComponent 触摸由 C API 侧响应、native 未接 DispatchTouchEvent）与设备观测完全吻合；本轮把"离线推断"升级为"设备实证"。
- 后续 T71 完成后的复测（T69 §9）须重跑本卡五臂同款动作，并核对 L2–L7 逐级转 PASS；届时坐标映射 (u=W/2,v=H/2)→(64,128) 等量化判据按 §9 执行。
- 干场物理与渲染侧本轮无可判新证据（零源上游阻断），T73 17/17 离线结论与设备观测不矛盾（无冲突项）。

## 5. 诊断流质量披露（T72 交付验证）

- 五臂共 9,302 诊断行 / 6,116 chunk，**dropped=0、sequence 断档=0、截断解析失败=0**——环形缓冲 256 条 + 每次最多 8 行 flush 的设计在本轮负载下零丢失，T72"被动有界诊断"设备可用性得到验证。
- `wet.receipt` 0 条（干场不触发湿 sink，符合 T72 设计）；`demo_gravity_config`、60 帧 `empty_sand_demo` 摘要正常。
- tick source 分布：`sensor` 为主（`none` 仅启动头几帧），stale=0——设备平放（USB 张力下传感器 tilt 值未参与注入，本卡不标定）。

## 6. 边界声明

- 本卡未改产品代码；`tools/t74_*`、`analyze_t74.py` 为验证脚本。
- 本报告不是产品 PASS：触摸零源在设备上**复现并实证**，修复归 T71；完整场景（可见沙、落床累积、摇晃重力）待 T71 修复后按 T69 §9 复测，完整画面呈现缺口归 T56/T78。
- 滴答卡评论因 MCP 授权不可用未能在卡上留言，交 manager 代登；清单 T74 行已按有限授权改"进行中"，完成登记留 manager。
