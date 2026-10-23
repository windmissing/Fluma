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

**90Hz 可达性实测级结论**：（待 tilt/off 出数后由分析脚本生成）
