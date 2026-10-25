# P-012 帧时三段拆分：埋点与采集就绪报告（v0——待真机出数）

任务：6adac84de4b06ee35703c571（渲染/合成/物理分段实测——P-013 换栈立项前置数据）。
状态：**埋点/脚本/分析链就绪，待真机出数**（设备占用中；本任务约束「暂时不用真机」遵守——全部工作离线完成）。

依据：architecture-P012-E2-route3-xcomponent-egl-assessment-v1.md §5.3（立项前置）。

## 1. 三段埋点（已入库，随路线②原型同构建）

帧循环（postFrameCallback 回调内）三段计时，纯计时不改物理/渲染逻辑：

| 段 | 采集点 | 语义 |
|---|---|---|
| `physMs` | `ws.step()` 前后 | 物理 substeps 全耗时 |
| `renderMs` | render 分发（P012 renderer / renderWaterSand）前后 | Canvas draw 提交（P012 含烘焙/E3 全链） |
| `presentMs` | `frameDeltaMs − physMs − renderMs` | 呈现等待段（vsync 空转+RS 合成；节拍锁定值见 §3 注意） |

输出格式（testMode 的 WS_FRAME 行追加 `seg=` JSON，主 payload 未动——P011 采集通道零影响）：

```text
WS_FRAME {"frameTimeMs":17,"physicsTimeMs":12,...} seg={"physMs":12,"renderMs":4,"presentMs":1}
```

## 2. 采集矩阵（tools/seg3_capture.sh，Bash 版——PowerShell 管道挂起坑已规避）

三场景 × OFF/ON（`p012ablate=toplight` 路线②原型开关复用）各 30s 窗 + 块间 25s 冷却：

| 场景 | 手势 | 窗口 |
|---|---|---|
| settle | 无（静置） | 30s |
| tilt | 8s 慢倾（1200→100） | 24s |
| heavy | 3s 快倾+4s+3s 快回 | 22s |

## 3. 判读口径（tools/seg3_analysis.py，已干跑验证）

- **热态过滤**：`physicsTimeMs>20ms` 帧剔除（F1 §2.1：热态降频使绝对值失真；
  physicsTime 是物理引擎内计时，不受埋点干扰）
- **vsync 节拍锁定注意**：静置帧 present 段 P50 会接近节拍余量（17ms−phys−render）
  而非真实 RS 合成开销——判读只用 tilt 场景（动态轻载代表窗）
- **90Hz 单拍可达性**：`物理 P95 + shader 化渲染残段（保守 2ms）< 11.1ms`？
  替换 architect 报告「条件可能（中置信）」的实测级判断
- shader 化理论可省段 = renderMs（Canvas draw 提交）——present 段的 RS 合成
  部分在换栈后改为 GPU pipeline，保守不计入节省

## 4. 待真机执行清单（effector 下次窗口，与参数面板/路线②原型同批出包）

1. 构建出包（HEAD 含埋点+toplight+参数面板）
2. `bash tools/seg3_capture.sh`（~8 分钟含冷却）
3. `python tools/seg3_analysis.py <tag>`（P50/P95 三段表+可达性结论）
4. 结果填入本报告 §5 并升 v1（出数后）

## 5. 实测数据（待填）

| 场景 | 臂 | n | phys P50/P95 | render P50/P95 | present P50/P95 | frame P50 |
|---|---|---|---|---|---|---|
| settle | off | — | — | — | — | — |
| settle | toplight | — | — | — | — | — |
| tilt | off | — | — | — | — | — |
| tilt | toplight | — | — | — | — | — |
| heavy | off | — | — | — | — | — |
| heavy | toplight | — | — | — | — | — |

**90Hz 可达性实测级结论**：见下 v1 章节（2026-10-23 effector 真机窗口出数）。

---

# v1（2026-10-23 effector 真机窗口出数）

## 6. 实测数据（热态 60°C，hilog 时间戳口径）

完整表与分析见 `docs/window-receipt-5in1-20261023-v1.md` §2。要点：

| 项 | 值 |
|---|---|
| s134 settle/off 吞吐 | 9.8 fps（E3 雾层每帧重烘，render 132 ms） |
| s134 tilt/off 吞吐 | 9.7 fps，render 双峰 7/156 ms |
| s134 heavy/off | 50 fps（dormant 段）/ 活跃段 7.6 fps |
| 二分 s106（无 E3） | **37.2 fps / render 15 ms**（同设备同热态） |
| 二分 s132（E3 无修复） | 11.4 fps / render 139 ms |
| 二分 s133（E3+9827739 修复） | 9.0 fps / render 139 ms——**修复对 settle 缓变场景无效** |
| 二分 s133+dyn | 18.3 fps / render 13 ms——dyn 消融在 s133 有效 |
| s134+dyn | **133 ms / 10 fps——dyn 在 s134 失效，回归定位于 s133..HEAD** |

根因两条（详见窗口回执 §2.2，移交 developer）：
1. E3 `e3Invalidate` 的 q 阈值 0.01 在沙面微动时每帧触发，双 buffer 每帧重烘；9827739 的 LAYER_EPS 只护层高不护 q。
2. s133..HEAD 区间另有渲染回归（dyn 档也慢），含掉沙链/E2/seg 埋点/S4 共 8 个工作区提交，未逐个二分。

## 7. 90Hz 可达性实测级结论

最乐观组合（s106 物理 + s133 dyn 动态层）：物理 P95 16 ms + 动态层渲染 P95 13 ms ≈ **29 ms ≫ 11.1 ms → 不可达**。物理段本身（12-13 ms）已超单拍预算；换栈只省渲染段，M2 里程碑需先物理治理。

## 8. 数据有效性警示

s134 全链数据被两个因素污染（§6 根因）。**P-013 判读用 s106/s133 二分数据，s134 数据仅作回归证据**。修复后需重测全套。采集脚本 `seg3_capture.sh` 已加 `p012ablate dyn`（注意：正式口径修复后应还原为全链）。

---

# v2（2026-10-24 effector s136 闭环回执，任务 6adb7038）

完整数据与证据链：`docs/p012-s136-closure-receipt-v1.md`。本节只记录对 v1 的**更正与深化**。

## 9. 对 v1 §6 两条根因的更正

1. ~~E3 q 阈值 0.01 每帧触发~~ → **q 从来不是观测到的触发源**：E3DIAG 取证 228 帧中 219 帧 dirty 由 **phase** 触发，qH/qL 增量恒 0。60d24bf 的 q 相对化修复真机验证有效但目标错误，s136 复测烘培帧率仍 46-63%。
2. ~~s133..HEAD 渲染回归（8 提交未二分）~~ → **8 提交全部免责**：同参数复测基线 8f090db（s133 构建点）= 11.2fps/54% 慢帧，与窗口内构建（9.7-10.0fps/63-66%）同档。v1 的"s133+dyn 18.3fps 有效"系安静尾部假象（该采集后 1/3 零慢帧，mean 33ms）。

## 10. 133ms 主因归因（E3DIAG2 分段计时）

| 段 | P50 | 占比 |
|---|---|---|
| N1 噪声场 shiftAndBake 每帧全量重烘 | **110ms** | 83% |
| E3 双缓冲烘焙 | 20ms | 15% |
| 12 档矩形带绘制 | 2ms | 1.5% |

机制：`fogPhaseX` = 逐列 wrap 后 UV 均值（tile 256px），settle 亚阈值质量微抖 + 单列跨 wrap 边界 → 均值跳变（实测 {0,92,162,210}px ≫ 0.5px 阈值）→ (a) 双缓冲每帧 dirty；(b) 噪声覆盖窗口每帧平移 → shiftAndBake 同窗口短路（|Δ|<0.5px）永不命中 → 600×1300 噪声格全量重烘。iface 调用同窗口 0ms 短路证明门控可行。

修复方向（移交 developer）：① 相位代理在未 wrap 累积量上求均值 / 逐帧增量 + 运动幅度死区 + 近空列 deltaX 钳幅；② shiftAndBake 入口对窗口平移量加下限。90Hz 结论不变（§7 维持否决）。
