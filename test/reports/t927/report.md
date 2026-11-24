# T927 · PC G15 连续搬移、DM2/R1 封存与 OFF 双臂实测（effector）

2026-11-24。本轮优先 T926 Demo 子项（卡面末段"本轮优先新增 T926 Demo 子项"）。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924），GPU 驻留、shader 渲染，无 Canvas 回退，无真机。证据根 `test/reports/t927/runs/`。

## 优先子项：T926 Demo 退役恢复与吞吐实测

PE `t926-ninja/hello_world_pc.exe`（sha `38aa0c162a7df69d…`，commit a7fd38b0），紧凑档 t918-shaders（旧 127 派发）与 t926-shaders（候选 35 派发）两模块组。驱动 `test/t927/drive_t927.py`（recovery/longrun/parity 三场景，单窗口进程、PostMessage 按压）。

### ① 同 fence 迟退役恢复 + 同一长按续沙（t926-recovery）

单次 Down 14s 持按（无中间 Up）+ 4s 沉降：

- **恢复链**：`SUSPEND→RETIRED→NEW_BASELINE→HOLD_REVALIDATED` 完整链 ×29；持按 id=1 全程不变（WIN32_POINTER DOWN=1/UP=1）。
- **同一长按续沙**：27 个接受 QUANTUM 跨 session 1→2→7→8→9→10 连续入账，liveSand 5→115 单调增长——**换代会话恢复后同一 Down 继续出沙**。
- **旧授权拒绝**：预约终端 762 条中 `AUTHORITY_CHANGED` 30 次（旧会话/旧 epoch offer 拒绝）+ `NEWER_ADJACENT_INTERVAL` 704 次；`M1_INPUT_WAIT_NEW_DOWN` 13 次（迟到脉冲要求新 Down）。
- 量子接受 27/70（ROOT_DEADLINE 39 次为 250ms 预算烧满，见 ③）。

### ② ≥60s 重复持按长程（t926-longrun）

10 循环（6s 持按+4s 空闲）+ 10s 沉降 + 6 探针，墙钟 175.2s：

- **接受坍缩复现（T920 签名仍在）**：558 提交仅 25 接受；接受集中在 overload 18–615（前两循环），其后 overload 单调涨至 13819 全拒；ROOT_DEADLINE 513（waitCount≤16、250ms 预算烧满）、LATE_FENCE_SIGNAL 15、CLOCK_GAP 5。
- **恢复环本身正常**：247 条完整恢复链全部 HOLD_REVALIDATED——T926 修复了"恢复"，但**未修"重负载下量子预算耗尽"**（T920 衰减根因，developer 已知）。
- 末段探针 0 接受：负载后仍无法出沙（与用户 920 反馈一致的残留问题，归 developer 的 T908 A/replay+fragment 优化路线）。

### ③ 2–4s 见堆 + 像素证据（t926-pile-pixels）

4s 持按（one-quantum-grant+observe 帧 50，三 ROI 对准 T920 先例位置）：

- **像素级见堆**：帧 50（持按中）`FENCED_READBACK` 真实像素，ROI[296,308] 沙色 (89,145,184)×2px、ROI[312,308]×5px（沙行 14–15）——**4s 内可见沙堆**（T920 容差判据沿用）。
- **归档对账**：同帧 root 归档沙床 col 63/64/65（940,389Q），liveSand 峰 84。
- 帧号受机器吞吐波动影响（physicsCommit 61–82/12s），已按实测调整请求帧并记录。

### ④ 候选 127→35 派发同输入对比（t927-parity-t918 vs t927-parity-fast）

同 exe、同 4s 中心持按+6s 沉降、无 grant 上限、dual-source：

| 臂 | 每量子派发 | 接受 | record→stop 中位 | sandStock 终值 |
|---|---|---|---|---|
| t918-shaders（旧） | **127**（实测） | 59 | 112.5ms | 4,560,212Q |
| t926-shaders（候选） | **35**（实测） | 86 | **82.6ms** | 5,445,557Q |

- **窄业务字段等价**：liveSand 状态点 5/11/16/21/26/31 的（活沙,活水,事件数,运动行,到达行）两臂逐一相同（6/6）；≥32 状态事件计数发散——候选的 bitonic 单 workgroup 批产生 7-沙中间步（步长粒度不同），非账目错误。
- **实际收益**：单量子录制→停机 112.5→82.6ms（-27%），同窗多接受 45.8%量子（86 vs 59），沙总入账多 19.4%。
- SOURCE 与 COMMIT 的 SPIR-V 哈希两档相同（合同确认）；`--m1-inventory-v1` 收据在每条量子行（`m1InventoryV1:true`）。

## 其余子项状态（按卡面顺序）

- ② DM2/R1 封存六包双侧比对：**已在 T923 续作（commit 6bda4c0c，2026-11-24 下午）完成**——六包 T922 资格核+负控、四 DM2 接受根归一化语义 6 对全等、R1 四臂 diagnostic 封存、prepare-retired 实测收益（DM2 接受 0/3→3/3）。本卡不再重复采证，直接引用。
- ① G15 dune 正反 ±45° 与 ③ OFF 双臂：**未开工**（本轮时间耗尽于优先子项的长程+像素+候选三线实测）。前置：G15 需要 t909 Release G15 dune 入口实采（`--g15-dune/--g15-dune-mirror`，kG15Dm2 宿主）；OFF 双臂沿 `run_t909_d1_off_pair.py`。两项均无阻塞，纯采证工作量，交下轮 effector 或本角色续卡。

## 结论

| 子项 | 结果 |
|---|---|
| T926 迟退役恢复+续沙 | **完成**：29 条完整恢复链、同 hold 跨 10 会话 27 接受量子、旧授权 AUTHORITY_CHANGED 30 拒 |
| T926 ≥60s 长程 | **完成（负结果如实）**：恢复链 247/247 正常，但接受坍缩 T920 签名仍在（overload 单调涨、ROOT_DEADLINE 513）——修复范围确认仅限恢复，不含预算耗尽 |
| T926 2-4s 见堆 | **完成**：帧 50 真实像素沙色 2+5px，归档床 940,389Q 对账 |
| T926 候选 35 派发 | **完成**：127→35 实测；record→stop -27%；同窗接受 +45.8%；窄业务字段 6/6 等价（中间步差异记录） |
| DM2/R1 封存 | **引用 T923 续作成果**（同日 6bda4c0c） |
| G15 dune / OFF 双臂 | **未开工**，无阻塞，交下轮 |

GPU 驻留、shader 渲染全程；250ms typed 门原值。
