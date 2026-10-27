# P051 T95 DEMO 物理阻断后复测效果报告 v1

日期：2026-10-27。责任：effector。任务卡：`6adf1e5ee4b017ab3402929f`。技术路线：`P051_TOUCH_TO_SOURCE_V1` 干场 DEMO。  
状态：**CODE5_FIXED_ON_DEVICE / LAND_FIRST_DEVICE_EVIDENCE / STABLE / SCENE_MATERIAL_FAULT_NEW / PARTIAL_PASS_WITH_NEW_DEFECT**。  
对比上一版（T82：code5 硬门槛 NOT PASS、LAND 未证、手持未完成）：**三项主判全部翻转**——code5 零复现、LAND 首次设备实证、稳定性零冻结；**新增渲染缺陷 SCENE_MATERIAL_DATA**（独立移交）。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit / 树指纹 | `a419f5a`（含 T166 修复 `1f8e16c`：waterTop<-EPS 对齐 audit + T167 独立 46/46）/ `aee7374caa8ff7cb` |
| HAP | `entry-default-s150-signed.hap`，SHA256 `f28ec28e639e2d5b7ccec265b9023890664531819b8b98094ca8348c7c3bd726`（主工程包，覆盖 T164 GPU 隔离测试包） |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；16:26:59–16:31:37 |
| 执行 | `tools/t95_demo_unblock_retest.ps1` 五臂；PID 隔离判读（T124 纪律） |
| 产物 | `test/artifacts/t95-demo-unblock-retest-20261027/`（五臂 dump、13 截图、console） |

## 1. J2：code5 零复现（T166 修复设备验证 PASS）

| 臂 | 注入 | physics-failed | first_code5_trace | 准入 | 收尾 |
|---|---|---|---|---|---|
| R1 | 4×swipe | **0** | **0** | 7 笔全 ACCEPTED，admitted 4.354 | 存活 |
| R2 | 6×swipe（加压） | **0** | **0** | 9 笔全 ACCEPTED | 存活 |

T82 触发序列（拖动注入+倾斜沉降 30°）在修复版上**两臂均未复现 code5**——T146 定位的"waterTop 判据口径不一致"（Hot 严格 `<0` vs audit `<-EPS`）+ T166 对齐修复，在设备上有效。`-2^-55` 浮点记账残差以原 bits 合法保留（T167 离线 46/46 的设备印证）。

## 2. LAND：首次设备实证（T82 缺口翻转）

R1 60 帧摘要序列（倾斜 30°）：
`dep 0 / pM 0 → pM 3.642 → pM 4.354（准入完成）→ **dep 1.155 / pM 3.200**`

**depositedMass 从 0 增长至 1.155、particleMass 同步下降**——粒子从 FLYING 完成落床准入进 mh/ml 的完整 LAND 链在设备上首次被账本+时间序列证实（T82 时 3s 窗全 airborne）。落床在准入完成后约 20-30s 内发生，与 T82 报告建议的"延长静置 ≥20s"一致。

## 3. 稳定性：零冻结（T78/T94 卡死线未复发）

R1（PID 24446）/R2（24752）全程存活、零 THREAD_BLOCK/appfreeze；R0/R3 同。T94 的 merge O(N²) 与 T129 的 audit 排序卡死在本负载下均未复发（干场 DEMO 页无双场 merge 竞争路径，与 T94/T129 卡死均发生在 SandCanvas 双场页的历史一致——干场线持续干净）。

## 4. 新缺陷：SCENE_MATERIAL_DATA 渲染故障（移交渲染线）

R1 出现 `present code=100` ×176 / code=0 ×248 混合（首次 16:27:52，swipe2 后）；R2 code=100 ×74；状态文本"新管线故障： SCENE_MATERIAL_DATA"；R1-t45 截图可见 8 颗粒子但容器背景为深色故障帧（背景层未出）。R0/R3（无注入或纯静置）code=0 全绿——**SCENE_MATERIAL_DATA 与注入/粒子出现相关**，属 T56 scene compositor 的材质数据通道问题（T149 V2 六图资产/上传链在该页面的接线）。

**分流**：客观可量化 → 渲染线（T56/T113/T114 域）另派；不阻塞本卡三项主判（粒子证据在故障帧中仍可见，账本判据全部来自 NDJSON/诊断流）。

## 5. 手持重力（R3）= 未完成

手动窗口 60s 内无 input.START（单手拔线操作限制同 T82）。手持项保持缺口，留待带辅助夹具的窗口；本卡不重复 T82 的此项移交。

## 6. 判定汇总（对 T82 对比）

| 维度 | T82 | T95 | 变化 |
|---|---|---|---|
| code5 硬门槛 | **触发** | 零复现 | **改善（修复有效）** |
| LAND 落床 | 未证（3s 窗 airborne） | **首次设备实证**（dep 1.155 序列） | 改善 |
| 稳定性 | 无 freeze（本页） | 无 freeze | 持平 |
| SCENE_MATERIAL_DATA | 未出现 | **出现 ×250** | 恶化（新缺陷，渲染线） |
| 手持重力 | 未完成 | 未完成 | 不变 |
| 主观分档（可评维度） | 响应 8/可见 7/干脆 9 | 同级保持（收入证据一致） | 可接受 |

**综合：PARTIAL PASS——物理阻断三项全部翻转，产品整体仍 NOT PASS（SCENE_MATERIAL_DATA 新缺陷 + 手持缺口），但 P051 触摸→准入→LAND 主线首次在设备全通。**

## 7. 移交与边界

1. **渲染线**（manager 派工）：SCENE_MATERIAL_DATA（code=100）与注入相关的触发条件、T56 material 通道排查——原始 dump 可复核。
2. T166 修复判定：设备两臂印证，T146/T166/T167 闭环可关。
3. 手持重力：维持 T82 的移交（辅助夹具窗口）。
4. 产品代码零修改；s150 隔离构建（树指纹 `aee7374caa8ff7cb`）。
5. 滴答 T95 卡评论已发；清单行状态留 manager。
