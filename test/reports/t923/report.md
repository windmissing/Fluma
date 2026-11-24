# T923 · PC 沙线新版续根与 G13 单角色对照（effector）

2026-11-24。三个独立子项按卡面顺序交付。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924），GPU 驻留、shader 渲染，无 Canvas 回退，无真机。证据根 `test/reports/t923/runs/`。

## ① t921-release split-v2 包复采（T919 修复验证 + T922 判据）

**工件核验**：两族 `--check-package` 通过（`OWNER_ROLE_SET` DM2 roleSet `f51229f4…`/R1 `58c999ca…`，manifestSHA 各异）；package.json SHA 与 T921 文档一致（`b2fc7207…`/`09e6e4cc…`）。

**0x380 负控**（T919 首个拒绝点复核）：
- 截断 256B：`ROLE_REJECT phase=FILE_LENGTH opcode=896 expected=9084/9896 actual=8828/9640`（带期望/实际字节与期望 SHA）——先于任何该角色 Vulkan 对象创建；
- 等长翻 1B：`ROLE_REJECT phase=FILE_SHA opcode=896`。
两族各两负控全过，ROLE_LAUNCH（真实 argv/CWD）与 ROLE_DEVICE（vendor/device/driver/UUID）行在拒绝前留档。

**交错对照**（T914 同 512B fixture `3580ccb1…`，detail+prewarm，SOURCE-only vs `--steps 1`）：

| 臂 | SOURCE | QUANTUM | 备注 |
|---|---|---|---|
| dm2-v2-source | 14 派发，拒（GRANT_DEADLINE） | — | |
| **dm2-v2-steps1** | **14 派发，接受**（stop 243.7ms，tail 14/14/14，`TAIL_ACCEPTED`） | 同进程续根：2404 槽计划，vkResult=2 拒 | **split-v2 首个 typed 接受 SOURCE + 同进程 QUANTUM 续根尝试** |
| r1-v2-source / steps1 | 27 派发，均拒 | — | R1 沿 T914 结论（GPU 233ms+host 段超界） |

- **14/27 物理派发实测**：DM2 ROOT `dispatches=14`、typed 尾 14/14/14；R1 `dispatches=27`——与 T913 v2 合同计数一致；DM2 fine 时标 31 点（2×14+3）全可用，GPU 派发和 209.7ms（与 v1 同量级：拆分增加粒度不减总工作量）。
- **`--raw-envelope` 封存**：dm2-v2-raw-r3 SOURCE 接受（4 次尝试 1 中——250ms 边缘），`accepted/` 20 件（INPUT128、GPU_INPUT128、ACTUAL、REPLAY、BASE、SEED、EVENT、EXPECTED224、COMPLETION256、QUOTA、WORK、arena×2 等）——**INPUT128 与 GPU_INPUT128 逐字节相同（SHA 31d7ebcc…）、ACTUAL 与 REPLAY 相同（74ba27b6…）**，envelope.json 带 permit/fence/generation/roleSetSHA。R1 两跑 SOURCE 拒绝，`diagnostic/` 封存 22 件含 **VISUAL_DEVICE_SEED(271KB)、VISUAL_MAPS(512KB)、VISUAL_MARKERS(768KB)、WORK(4.8MB)** 与 rootVkResult=2、diagnosticGeneration——T921"超期同 fence 退役可封存诊断"合同实测。
- R1 DEVICE seed 与 visual lease：raw 封存内已含 device seed/maps/markers 原件。
- **T924 配套 baseline/v1 封存包**：未到件（developer 卡在途），本子项按卡面"只缺分版工件的部分可单独等待"处理——v2 侧证据已齐，双侧独立比对待 T924 工件。

## ② G13 OP_48 默认 vs `--disable-optimization`（T921/T915 合同）

同一 T916 PE（`6dd3378f…`）、同模块（`f01959cc…`）、同驱动、同监督窗（60s 调用/120s 进程），ABBA 新进程 4 臂：

| 臂 | 结果 | 20s RIP | 45s RIP | owner CPU（~50.6-50.8s 观测窗） |
|---|---|---|---|---|
| default 01 | CENSORED CALL_UNRETURNED | igc64+0x410f7e | **igc64+0x411080** | 49.0s |
| default 02 | CENSORED CALL_UNRETURNED | igc64+0xb20498 | **igc64+0x411085** | 47.5s |
| noopt 01 | CENSORED CALL_UNRETURNED | igc64+0xdc6f5d | **igc64+0x411085** | 45.8s |
| noopt 02 | CENSORED CALL_UNRETURNED | ntdll+0x27388 | **igc64+0x411085** | 47.1s |

- **4/4 全部 CALL_UNRETURNED**：`--disable-optimization` 不改变结果——与 T912 补采/T921 结论一致（Intel 编译器 igc64 内持续 CPU 长调用：owner CPU 占观测窗 90–97%）；45s 采样收敛到 `igc64.dll+0x411080/0x411085`（相邻 5 字节，同 T921 已记录位置），20s 采样分散（编译早期不同 pass）。
- 按合同：**默认与 noopt 无分化 → 不做进一步二分**；缓存隔离记 UNKNOWN；每臂 `modules` 收据含 vulkan-1.dll(1.3.300.0)/igc64.dll/igvk64.dll SHA+版本，20s/45s minidump 与 owner 栈引用在档。
- 收据通过 `t921_op48_stacks.py` 解析（`op48-stacks.json`）；noopt 臂因文件名后缀由内联分析补充（数据齐备入报告）。

## ③ 飞包单列取证（T912 唯一剩余缺项，`--m1-observe-packet`）

T909 r2 PE（`d37e45b9…`）+ `--m1-observe-packet`（request `firstFrame count packetId generation` = `1 120 2 1`）+ `--m1-dual-source`，水料持按 6s：

- **包样本绑定**：360 帧 `m1Observation` 带真实 `packetId=2, packetGeneration=1`，每帧记录 packetSampleRoot、mass、FP32 X/Y 字；357 帧 `FENCED_READBACK`（768B 真实像素 + SHA256 校验全过）。
- **同根中心 ROI**：138/138 有质量帧的包中心（由 FP32 字换算窗口像素）落在 rect0（8×8 包中心 ROI）内——100%；三 ROI 组 = 包中心 + 近邻 + 底部（rects 实测如 `[388,376,8,8] [395,376,8,8] [388,453,8,8]`，随包位置逐帧移动）。
- **独立 reader 核验**（`read_t909_observation.py --packet 2 1`）：120 sameRoot 图像中 **46 帧包中心在根档内且落 ROI**、74 帧 `packetCenterInRoi=None`（包已 ARRIVE/不在该根）——包出现、同根、消失三分判据齐。
- **ARRIVE 与消失**：归档 153 根共 33 笔 kind-11 ARRIVE == 33 笔 SOURCE birth；轨迹中包 mass 从 126,483 Q 出现、在持按后期 sample 帧 mass=0（219 帧，包已落/被新包替换）——**"实际 ARRIVE、同根中心/近邻、消失或遮挡"卡面三要素全采**。
- 水物理照常：33 SOURCE、2,488 WATER_EDGE、1,518 WATER_MOVE（同 T912 量级）。

## 结论

| 子项 | 结果 |
|---|---|
| ① split-v2 包复采 | **完成**：自检+0x380 双负控+DM2 v2 typed 接受（14 派发/31 点/14-14-14 尾）+同进程 QUANTUM 续根+raw-envelope 双态封存（accepted 20 件/diagnostic 22 件含 R1 device seed）；R1 27 派发拒沿 T914；**T924 baseline/v1 封存配套未到件**，双侧独立比对待件 |
| ② OP_48 default vs noopt | **完成**：4/4 CALL_UNRETURNED 无分化；45s RIP 收敛 igc64+0x411080/85；owner CPU 90–97%；按合同不二分，缓存隔离 UNKNOWN |
| ③ 飞包取证 | **完成**：T912 唯一剩余缺项闭环——packet ID/代、ARRIVE、同根中心/近邻 ROI、消失/遮挡全采，独立 reader 三分判据通过 |

GPU 驻留、shader 渲染全程；迟到退役仅诊断；250ms/typed 门原值。
