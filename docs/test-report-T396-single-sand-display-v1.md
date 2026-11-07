# T396 单沙三区颜色与解锁显示真机核对 — 静态三区逐位一致；动态 A/B 与锁定 A/B 全链取证完成，判读为亚像素舍入边界（粒子本体可见、颜色逐位、位置 ±1px）

- 卡面：T394-final-signed.hap 按 developer.md 分别验静态三区和动态 A/B，核同 token candidate/back、实际交换和无损截图，缺配对 DISPLAY_NOT_JUDGABLE。
- 包：T394-final-signed.hap SHA256 `f8503cafd4010fe84f837b54ef0ec8b692abb3ca34684f950353ae9847e8c138`（本地逐位核验一致，verifyApp PASS，签包回执 test/reports/t257/T394-final.json）。
- 路由：`p051v2=1 --ps singleSand 1`（单沙双态通道；首跑漏 `singleSand=1` 导致预约后停在"等待下一次有效绘制"，force-stop 后带参冷启修复）。
- 证据：`test/reports/t396/`（布局快照 l8–l35、4 组 manifest 四件套、4 个 hold 封存件、4 张无损 PNG、判读/分析脚本与输出）。

## 执行序列（全部真实 uiInput 点击）

1. **静态三区**：厚床三区（重床 128 列×48=6144 + AIR 0.1）→ 预填 BED/AIR/BG（`8,8,16,16;579,332,18,18;8,658,16,16`）→ 中性 → 预约一次 → 导出读回（`t394-1-1-1628`，CAPTURED/CURRENT/同 token）→ 无损 PNG（staticA.png，1280×2832 真实 PNG 魔数）→ 封存截图时段（t394-hold-1-1628.json，SHA 76b88fb2…）。
2. **动态对照臂**：对照 A（CAPTURED·CONTROL_A）→ dynA.png → 封存（hold-2-1629）→ 导出读回（t394-2-1-1629）→ 注入 B（CAPTURED·CONTROL_B，准入 2 笔 sequence 1/2 各 admittedH=0.1，steps 2 帧 dt=0.01667）→ dynB.png → 封存（hold-3-1630）→ 导出（t394-3-1-1630）。
3. **动态锁定臂**：锁定 A（CAPTURED·LOCK_A）→ lockA.png → 封存/导出（t394-4-1-1631）→ 注入 B（CAPTURED·LOCK_B）→ lockB.png → 封存/导出（t394-5-1-1632）→ **重锁**（LOCK_RELOCK，probeStatus=EMPTY_BED 重锁后状态正常）→ **导出阶段**（t394-phase-1794018946973.json，firstFailure=''）。

## 判读结果（pair_display.py 正式跑）

| 对 | displayStatus | firstDifference | 差异归因（逐像素分析） |
|---|---|---|---|
| CONTROL A/B | DISPLAY_NOT_JUDGABLE | B_SCREENSHOT_DOES_NOT_MATCH_EITHER_BACK | A 侧三 ROI 屏幕像素与 back **逐位相等**；B 侧 R0/R2 逐位相等，R1 屏幕粒子图案=back 平移 (-1,0) 后 155/155 全匹配 |
| LOCK A/B | DISPLAY_NOT_JUDGABLE | 同上 | LOCK_A 三 ROI 屏幕像素与 back **逐位相等**；LOCK_B R0/R2 逐位相等，R1 同型 -1px 平移全匹配 |

**差异性质定性（非显示错误）**：屏幕截屏与 native back 的差异仅存在于粒子圆边缘 0.4px 级抗锯齿环带（A：右缘 14px，拟合半径 scr 6.6 vs back 7.0，圆心一致 (588.8,341.0)；B：整体 -1px 平移 155/155 匹配）。粒子本体：**颜色逐位一致（91a86f）、中心位置一致、形状一致**；BED/BG 区逐位一致。判读器要求逐字节相等，亚像素舍入即落 NOT_JUDGABLE——这是无损截图与 GPU 光栅的精度边界，非显示缺陷。

## 同 token 与实际交换核对

- 四组 manifest 全部 `captureStatus=CAPTURED`、`presentationState=CURRENT`、`swapSucceeded=true`、`requestedToken==actualToken`、candidate/back 时间戳单调递增（candidateNs→backNs→captureEndNs→swapStartNs→swapEndNs→unbindEndNs）。
- hold 封存件四份全部 `revision=T394_DISPLAY_HOLD_V1`、token/frame/geometry 与 manifest sample 一致、截图时刻落入 hold 窗。
- 实际交换：静态臂 candidate==back（静态画面无差异是预期）；动态 B 臂 candidate==back 且与 A 臂 back 不同（新粒子 ID2 (96,128) 准入并渲染）——**A→B 打包内容变化+新 ID 真实发生**。
- 投影掩膜（pair_display 独立 CPU 重算）：A 臂 id1 在 R0 154px、B 臂 id2 在 R1 156px、BG 区双臂恒 0——粒子只出现在自己 ROI，与投影位置吻合。

## 页面外取证的坐标核对

- XComponent 组件 bounds `[51,1968][1229,2650]` = 1178×682 px，与 manifest `width=1178,height=682` 精确一致（MAPPING_REQUIRES_EXACT_PIXELS 通过）。
- 截屏工具：`uitest screenCap`（真 PNG，魔数 89504e47）；`snapshot_display` 仅出 JPEG 不可用。

## 结论

- **静态三区：PASS**（BED/AIR/BG 屏幕 RGBA 与 candidate/back 逐位一致，中性光学，权威库存 bedMass=6144、AIR 粒子 (64,128,state=1,0.1)）。
- **动态 A/B 与锁定臂：取证链完整、粒子显示真实可证**；正式判读码为 DISPLAY_NOT_JUDGABLE，根因=亚像素舍入（±1px/0.4px 级 AA），非颜色/位置/内容错误。是否把"平移后逐位全等"纳入判读容忍度属判读器修订建议权（architect）。
- 失败/需裁定项：**pair_display 逐字节相等判据与设备亚像素舍入的兼容性 → 交 architect 裁定**（判读器修订或容忍带定义）。
- 未改业务代码；全部点击真实注入；证据原样归档 test/reports/t396/。
