# P051 T82 触摸接线修复后 DEMO 复测效果报告 v1

日期：2026-10-26。责任：effector。任务卡：`6adef380e4b06ee3573cc257`。技术路线：`P051_TOUCH_TO_SOURCE_V1` + T56 完整 scene。  
状态：**TOUCH_CHAIN_REPAIRED_ON_DEVICE / NEW_PHYSICS_BLOCKER_TILT_PLANNING_5 / NOT_A_PRODUCT_PASS**。  
对比上一版（T74：L1 RECEIVE FAIL，0 touch 事件）：**L1–L5 全链修复得到设备实证**；发现新的物理域阻塞（详见 §3）。

## 0. 环境与构建身份

| 项 | 值 |
|---|---|
| commit | `114a7ee`（T56 完整 scene + T71 输入节点 `0b1f7e3` + T85/T86；清单指定的固定基线） |
| 树指纹 | `403b08514017b326`（worktree 隔离构建，产品代码零修改） |
| HAP | `entry-default-s144-signed.hap`，SHA256 `af049753c8dda7e169bcf0613f384b7affa3aa3cf1fb1fac7cd65ad64db31be8`（4,393,876B） |
| 设备 | VYG-AL00，hdc `62T0225B10010020` |
| 执行 | `tools/t82_touch_fix_retest.ps1` 六臂 + `tools/analyze_t82.py`；产物 `test/artifacts/t82-touch-fix-retest-20261026/`（13 截图 + 六臂 dump + analysis JSON） |
| 采集窗口 | 16:06:52–16:10:09 |

## 1. 事件链 L1–L7 逐级结果（对比 T74）

| 级 | T74（修复前） | T82（修复后） | 证据 |
|---|---|---|---|
| L1 RECEIVE | **FAIL**（五臂 0 touch） | **PASS**（R2/R5 有 `input.RECEIVE`，DOWN/UP 与动作对应，geometryReady=true，实测 W=336.46×H=578.86 vp） | T71 无绘制输入节点设备生效 |
| L2 MAP/START | N/A | **PASS**（`input.START reason=ACCEPTED`；一次 gesture 一次 start；MAP 坐标合理） | 单指状态机工作 |
| L3 CLOCK | PASS | PASS（tick 连续、stale=0） | — |
| L4 REQUEST | N/A | **PASS**（R2 5 笔 / R5 6 笔 `dry.request`，速率中位 8.00 u/s / 6.06 u/s ≈ 8 unit/s 设计值） | 速率积分正确 |
| L5 RECEIPT | N/A | **PASS**（全部 ACCEPTED，R2 admitted 1.864、R5 2.938，无拒因） | 干场准入设备可达 |
| L6 PUBLICATION | N/A | R5 **PASS**（particleMass=1.512、sourceEpoch 0→4=准入笔数、`dry.baselineStatus=NONEMPTY`、occupiedVolumeTotal=1.512）；R2 **FAIL**（§3 阻塞） | 发布链随物理走 |
| L7 RENDER | PASS（回执） | **PASS**（R2-dragging 截图可见 5 颗亮色粒子成排悬浮；R5-new-down 可见 7 颗粒子分布两处；present code=0、零 STALE 帧） | **屏幕真见沙，非 swap 代通过** |

**核心结论：触摸零源已在设备修复闭环**——手指事件→坐标映射→start→按住速率请求→干场准入→发布→屏幕可见沙粒，全链取证成功。

## 2. 停止边界与生命周期（R3/R5 专项）

- **松手停止**：R2/R5 的 `input.STOP reason=UP` 与 `model.stop` 成对出现；UP 后 `dry.request` 立即归零（R5 两次 gesture 之间清晰间隔），无追赶注入。
- **后台不续发**：R5 Home 后 `PAGE_INACTIVE`/`inactive` stop；返回后**无新请求**（须新 Down 才续沙）——`R5-back-no-hold` 截图与诊断一致。
- **R1 教训（注入工具限制，非产品缺陷）**：`uiInput swipe` 同点 0 位移未产生任何触摸事件（L1 FAIL 实为注入器不派发零位移手势）；R2/R5 有位移拖动正常触发。按住不动发沙的主观场景须真手验证（R4 窗口未完成，见 §4）。
- **R3 dump 混臂说明**：R3 dump 开头 19 条 `page-physics-failed` 属 R2 尾部 flush（时间戳早于 R3 首 tick、runId 属 R2 run `…069511`）；按 runId 分离后 R3 本臂（`…097025`）零失败零触摸。分析器已按 runId 过滤，混臂不改变任何判定。

## 3. 新物理阻塞：TILT_PLANNING_BLOCKED (code 5)（硬门槛触发）

R2 拖动臂：5 笔准入完成后（粒子沉降触底过程中），`field.step` 返回失败——页面状态文本 **"物理提交失败： 5"**（`getLastError()=5`=`TILT_PLANNING_BLOCKED`，WaterSandField.ets:302），此后每 tick `page-physics-failed`、物理 commit 冻结在 577、60 帧摘要停止产出。

- 触发组合：设备平放（USB 张力→sensor target tilt ≈79.8°，T50 映射钳制后 effective=30°）+ 干场粒子沉降触底成堆。
- 对照：R5 同样 30° 且已准入 2.938，粒子**全部悬空未落床**（airborneFraction=1.0、depositedMass=0），3s 观察窗内未触发；R0/R1/R3/R4 零粒子不触发。
- 定性（对齐 T46 裁决口径）：**这是 planner 阻断（code=5），不是容量拒绝**；该步/run 记 invalid（fail-closed），不参与主观评分。与 T47/T86 的 step73 离线发现同族——**干场粒子沉降触底 + 倾斜的组合在设备上把规划器打到 blocked**，为 T90（architect 规划机制到件分流）提供了首个设备侧复现样本。
- 分流：**交 T90/T79 线（architect）**；本卡不改物理、不放宽 guard。T71 输入修复本身不受此影响（准入先于失败全部成功）。

## 4. 手持重力（R4）= PARTIAL

R4 手动窗口（40s）内未完成按住发沙操作（无 `input.RECEIVE`），手持方向观察缺证据。原因：拔 USB 后单手无法同时按住屏幕与观察（工具限制）。**手持方向判定未完成**，留待 T52/T88 或后续带辅助夹具的窗口；不影响其余判定。

## 5. 主观分档（仅就 R2/R5 有效证据；及格线 6）

| 维度 | 得分 | 依据 |
|---|---|---|
| 发沙响应性 | 8 | Down→request 延迟一帧内，速率 8u/s 稳定，无爆发无丢失 |
| 粒子可见性 | 7 | 深色背景下亮色粒子清晰可辨（R2-dragging 5 颗、R5 7 颗），但单粒过小、无拖尾 |
| 落床连续性 | 3 | **未证**：两臂观察窗内 depositedMass 均为 0、粒子悬空（airborne=1.0）；R2 更因 code5 中断 |
| 松手干脆度 | 9 | UP 后请求立即归零，无追赶 |
| 硬门槛 | **触发 1 项** | R2 TILT_PLANNING_BLOCKED 致物理冻结（粒子瞬移类级中断） |

**综合：NOT PASS**（硬门槛触发 + 落床未证）。触摸链修复本身达标；产品通过被物理域 code5 与落床验证缺口阻塞。

## 6. 移交与后续

1. **T90（architect）**：接收本报告作为 step73/TILT_PLANNING_BLOCKED 的**设备侧复现证据**（倾斜 30° + 干场粒子沉降组合）；R2 原始 dump 可复核 fail 时序（准入 5 笔→沉降→code5）。
2. **T71 关闭判定输入**：触摸链 L1–L5 设备 PASS，T69 §7.2 表格的输入节点分支可判完成。
3. **落床（LAND）验证缺口**：R5 粒子 3s 未落床（airborne=1.0）——若为正常沉降时长则延长观察即可；若长时间悬空则涉 T59 干场动力学。建议 T78/T52 采集时延长静置窗至 ≥20s 并记录 depositedMass 时间序列。
4. R4 手持重力、按住不动发沙：待有辅助手段（夹具/双手）的窗口补做。
5. T78（双场采集复测）与 T57（G5 真机双测）按清单顺序继续；本卡释放设备。

## 7. 边界声明

- 产品代码零修改；隔离构建 worktree @ `114a7ee`。
- 本报告不宣布任何协议生效；code5 记 invalid 交归因，未重跑凑 PASS。
- 滴答卡评论因 MCP 授权不可用留 manager 代登；清单 T82 行已按有限授权改"进行中"，完成登记留 manager。
