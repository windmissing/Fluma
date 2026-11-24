# T920 · 紧凑 Demo 2–4 秒见堆与转盒 PC 实测（effector）

2026-11-24。T918 交付（commit `1f7661a3`）实测。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924），GPU 驻留、shader 渲染，无 Canvas 回退，无真机。驱动脚本 `test/t920/drive_t920.py`（PostMessage 按压/键盘/按钮），工件 SHA 记录于各 `argv.json`。

- **PE**：`.codex/local-test/t918-ninja/hello_world_pc.exe`（`f5aff05d…622d`，T918 交付核验一致）
- **紧凑档**：`--m1-compact-demo` + `t918-shaders`（source `e9c7c123…`）；**Baseline**：`--m1-empty-preview --m1-manual-gravity` + `t830-m1-20261123` 三份原 shader
- **窗口**：紧凑 624×431（盒体 80 列×64 单位 = 276×221px，scale=min(w,h)×.008）；Baseline 784×461（128 列×256 单位全容器）

## ① 4 秒长按入料与见堆（紧凑档 vs Baseline）

| 项 | 紧凑档（compact-pile） | Baseline（baseline-pile） |
|---|---|---|
| 4s 持按受理 | **21 笔 SOURCE，9.73 模型单位，2.43 u/s** | 29 笔，3.65u，0.91 u/s |
| 源出生高度 | 喷嘴钳制 y≤16 单位（盒顶附近） | 中心点击映射 y≈128 单位（高落差） |
| 松手后沉降 | 床高持续增长；25s 后单帧实测 col64 **4.7px**（maxS 1.37u） | 10s 窗口末仍有 31 粒在飞 |
| LAND/床事件（系列 120 帧归档） | 31 LAND、40 BED_EDGE、3,219 SAND_MOVE，75 根同根绑定全过 | —（无观察系列） |

- **堆可见性实测**（compact-pile-late4，frame 160 单帧 + 三 ROI 对准盒底）：col63/64/65 沙色像素行 12–15/16，实测可见堆高 **4 行像素 ≈ 归档床高 4.72px，偏差 ≤1px（T910 容差内）**；ROI 中心矩形 16 行中 4 行沙色（20 沙像素，颜色 BGR(89,145,184)=sRGB 沙色）。
- **"2–4 秒明显见堆"未在墙钟内达成**，量化根因（沿 T908 框架）：
  1. 入料吞吐达标（2.43u/s ≥ T908 所需 2.5u/s 的 97%）——紧凑档 32 单位/s 请求档 + 盒内钳制已把入料从 0.91 提到 2.43 u/s（2.7 倍）；
  2. **仿真时钟仍 ~0.13–0.16 s/s**：4s 墙钟仅 ~63 个量子（1/120s/量子），粒子飞行需多个仿真秒才落床——松手时 81 粒在飞；
  3. 落床后床高 4.7px < T908 6px 门（差距 22%）。
  堆在**松手后 ~25 墙钟秒**达到 4.7px 并留存（8.15u 库存恒定、沉降持续）。结论：见堆主瓶颈已从入料转移到仿真时钟，符合 T908 预判。
- **松手留存**：stock 全程守恒（8.19u 旋转期恒定；沉降期 stock=已沉降+在飞，无负账）；晚期帧床高稳定非衰减。

## ② 转盒与复位（compact-rotate）

16 笔键盘输入全部 `ACCEPTED_INPUT`：6×LEFT（−5°/击至 −30°）→ 8×RIGHT（边界反向回 +10°）→ R 键 → 屏上按钮。

- **边界行为**：−30° 请求（seq29）被床形裁到 **−15.07°**（`limitReason=BED_SHAPE`，31 个受限帧）；随 LEFT→RIGHT 反向，接受角逐步恢复（−1941→+2560 逐帧实进）。
- **物理 vs 重绘区分**（卡面要求）：全部角度变化走 **`m1GravityFrame` 物理接受根**（4 帧/输入档，带 nonce/commit/slot）；`M1_BOX_VIEW_PRESENT_QUEUED` 重绘帧 **0 笔**——本机物理接受足够快（每档 3–4 帧内跟上请求），重绘路径未触发，不存在"把重绘当仿真"的混淆；旋转期 stock 恒 8.19u、活沙 72→42（落床进行）。
- **旋转物理归档**（compact-rotate-archive，frame 55 @ t=11.6s，−25° 请求期）：接受角 −15.58°（BED_SHAPE 裁限），单帧 3 笔 SAND_MOVE、68 CONTACT_WAIT、1 RELEASE——床料在倾角下真实搬运，角度受床坡判据约束。
- R 键与屏上按钮均回 0°（seq38/39 accepted 0→1907→0，随后稳定 0）。

## ③ CLOCK_GAP 行为（沿 T916）

- rotate 两跑各出现 1 次 post-UP `CLOCK_GAP`（SUSPEND/RETIRED/NEW_BASELINE，hold=0）——**UP 后无按压，正确跳过重绑**，新会话自主恢复（`M1_READY_PHYSICS`）；
- late 系列各 1 次 `ROOT_DEADLINE` 自主恢复（非 gap）；
- 持按期内未再遇 gap（对照 T912 修复后自然发生率），同次按压恢复链已在 T912 补采（385ee285）完整验证。

## 结论与缺项

| 卡面项 | 结果 |
|---|---|
| 空箱实际尺寸对比 | **完成**：624×431 盒体 80×64 单位 vs 784×461 全容器 |
| 源高度（喷嘴钳制） | **完成**：紧凑 y≤16 钳制实测（CompactPointer 拒绝出盒点击） |
| 实际 SOURCE/逐笔 LAND/同根底部 ROI | **完成**：21 笔受理、31 LAND+40 BED_EDGE 同根归档、底部 ROI 像素 vs 归档床高 ≤1px 偏差 |
| **2–4 秒明显见堆** | **未达**：入料已达标（2.43 vs 2.5 u/s），瓶颈为仿真时钟 0.13 s/s 与粒子飞行时延；堆在 ~25 墙钟秒可见（4.7px）且留存 |
| 连续左右/边界反向/R/按钮 | **完成**：−30° 边界 BED_SHAPE 裁 −15.07°，反向逐步恢复，R/按钮回 0° |
| 画面姿态 vs 实际角（不把重绘当仿真） | **完成**：全走物理根；重绘帧 0（未触发，无混淆风险） |
| CLOCK_GAP 同次按压恢复 | **完成**（post-UP 正确跳过；持按期行为见 T912 补采） |
| 视频 | 未录（像素+日志+归档已覆盖判据） |

缺项：2–4s 见堆的仿真时钟瓶颈归 developer（沿 T908 A 方案"时钟拆分"项）；沙色像素/床高对齐可作后续回归判据。

证据：`test/reports/t920/runs/`（compact-pile 系列 120 帧、late1–4 单帧、rotate×2、rotate-archive、baseline-pile，全部含 argv.json + demo.jsonl + M1PROOT1 档案）。
