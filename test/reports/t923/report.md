# T923 · PC 沙线新版续根与 G13 单角色对照（effector）

2026-11-24。三个独立子项按卡面顺序交付。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924），GPU 驻留、shader 渲染，无 Canvas 回退，无真机。证据根 `test/reports/t923/runs/`。

## ④ 续作（2026-11-24 下午）：T924 封存配套到件后的双侧独立比对与 prepare-retired 实测

用户重发 923 后 T924 developer 已交六包（`ae27fea0`，`.codex/local-test/t924-release`，`docs/developer-T924-v1.md`）。补采当时唯一待件项，证据根 `test/reports/t923/runs/t924-runs/`（下称 `t924-runs/`）。

**六包资格核（T922 package）**：DM2/R1 × baseline/t913-v1/t913-split-v2 全过（`issues=[]`，exe/roleSetSHA 与 developer 文档一致：DM2 `3639e57f/ada9bc45/f51229f4`、R1 `f7d812d6/86b479da/58c999ca`；split-v2 两批 roleSetSHA 与 t921-release 完全一致、48+56 静态角色模块逐一同 SHA）。收据 `t924-package-checks.json`。

**0x380 负控（t924 包）**：dm2-baseline 截断 256B → 宿主 `--check-package` exit=1 `ROLE_REJECT phase=FILE_LENGTH opcode=1`（期望 88416/实际 88160）；r1-v1 等长翻 1B → `ROLE_REJECT phase=FILE_SHA`。`t924-negatives/*/reject.jsonl` 留宿主拒绝原行（含 ROLE_LAUNCH/ROLE_DEVICE/OWNER_ROLE_SET 前置行）。

**双侧真实 SOURCE 交错对照**（冻结输入 `t894-model-source.u32le`，detail+prewarm，`--raw-envelope`）：

| 臂 | 接受 | 停时（grant→stop） | 封存 |
|---|---|---|---|
| dm2-baseline（12 派发） | plain-r4 接受（245.0ms；detail 模式 20 连拒 250–321ms 边缘） | 245.0ms | accepted 20 件，`plain-r4` |
| dm2-t913-v1（12 派发） | 首跑接受 | 241.5ms | accepted 20 件 |
| dm2-t913-split-v2@t924（14 派发） | 首跑接受 | 212.0ms（GPU 时标和 span 4.6ms） | accepted 20 件 |
| dm2-t913-split-v2@t921（14 派发） | steps1-r2 接受（232.6ms） | 232.6ms | accepted 20 件 + 同进程 QUANTUM diagnostic 19 件 |
| r1 四臂（19/19/27/27 派发） | 全拒 vk=2 | 250.0–263.7ms | diagnostic 22 件×4（含 VISUAL_DEVICE_SEED/MAPS/MARKERS） |

- **T922 verify**：三个 DM2 接受封存 + t921 steps1 接受封存 `verifiedSource=true`；timeline 八臂全过（detail 点数 27/27/31/31、41/57/57，r1-baseline 早超时路径无 query 行属拒绝路径常态）。
- **T922 diff 命令合同分歧（如实记录）**：对真实封存跑 `diff` 恒 `REJECTED UNCLASSIFIED_HISTORICAL_ORIGINS`——真实宿主 seed 的 BASE 自带 `base[19]=1` 个 origin，而检查器合同（check.py L271-273"cross-process certification currently covers one empty-origin seed"）只覆盖空 origin 种子。两臂一致、verify 各自通过，是合同前提与真实产物结构的分歧，非任一侧数据错误。
- **独立语义比对（自实现归一化，按 check.py 同一规范化字段集）**：修正 origin 行定位（`origin+16*base[19]`）后，四接受根两两 6 对 **`input/base/actual/event/result` 归一化后逐字节全等**（`semantic-pairs.json`）。`INPUT128==GPU_INPUT128`、`ACTUAL==REPLAY` 逐字（4/4 封存，`envelope-identity.json`）；未归一化差异仅在 scope/nonce/时钟字段（T914 已证 inputSHA 每跑必异属墙钟入输入，归一化后消除）。origin 行内 4 词 digest 各臂自洽等于 `source_digest(raw)`，归一化后四臂同为 `400410870…`。
- R1 DEVICE seed/visual lease：四臂 diagnostic 封存各含 VISUAL 三件原件。

**`--source-prepare-retired` 实测（T924 ② 留给 effector 的收益项）**：同包（各族 split-v2）同输入同机，默认 vs 开启：

| 族·臂 | record→submit | commandReset | 接受 | 检查器 |
|---|---|---|---|---|
| DM2 default | 39.44ms（3 轮 51.5–64.8ms） | 39.2ms 在 grant 内 | 0/3 | PASS |
| DM2 prepare-retired | 0.33ms（3 轮 0.26–2.05ms） | 0.0ms（移出 grant） | **3/3** | PASS |
| R1 default | 47.98ms | 47.8ms 在 grant 内 | 0/3 | PASS |
| R1 prepare-retired | 0.24–1.10ms | 0.0ms | 0/3 | PASS |

- `check_t924_command_preparation.py` 四臂全过：`vkResetCommandPool/vkResetFences/vkBeginCommandBuffer` ENTER/EXIT 与 VkResult=0 全序、pool/fence 代 `useGeneration==priorGeneration+1`、retired 臂 `retireProof.vkResult=0` 且退出先于 grant、`SOURCE_COMMAND_PREPARED` 行同 run 同收据。
- **实测收益**：`vkResetCommandPool` 39–48ms（占 grant 窗大头）整体移到 grant 前退役窗口，录制段从 ~40–65ms 压到 0.3–2ms；DM2 接受率 0/3→3/3（集显 250ms 边缘摆动被吸收）。R1 仍 0/3——提交后 fence 等待 205.6–249.9ms 主导，与 T914/T923"R1 拒因在 GPU 侧等待"结论一致，准备段收益真实但不改变 R1 结局。
- 各臂明细：`prepare-retired.json`（单臂）+ `prepare-retired-stability.json`（3×4 轮）。

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
- **T924 配套 baseline/v1 封存包**：续作到件后已补齐（见 ④）：六包资格核+双负控过；四 DM2 接受根归一化语义 6 对全等；R1 四臂 diagnostic 封存含 DEVICE seed。

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
| ① split-v2 包复采 | **完成**：自检+0x380 双负控+DM2 v2 typed 接受（14 派发/31 点/14-14-14 尾）+同进程 QUANTUM 续根+raw-envelope 双态封存（accepted 20 件/diagnostic 22 件含 R1 device seed）；R1 27 派发拒沿 T914 |
| ① 续作（T924 到件） | **完成**：六包资格核+负控过；四 DM2 接受根（baseline/v1/split-v2×两批）归一化语义 6 对全等（diff 命令合同分歧如实记录：真实 seed 带 1 origin，超出 empty-origin 合同）；R1 四臂 diagnostic 封存；prepare-retired 实测：record→submit 39–65ms→0.3–2ms，DM2 接受 0/3→3/3，R1 仍 GPU 等待主导 |
| ② OP_48 default vs noopt | **完成**：4/4 CALL_UNRETURNED 无分化；45s RIP 收敛 igc64+0x411080/85；owner CPU 90–97%；按合同不二分，缓存隔离 UNKNOWN |
| ③ 飞包取证 | **完成**：T912 唯一剩余缺项闭环——packet ID/代、ARRIVE、同根中心/近邻 ROI、消失/遮挡全采，独立 reader 三分判据通过 |

GPU 驻留、shader 渲染全程；迟到退役仅诊断；250ms/typed 门原值。
