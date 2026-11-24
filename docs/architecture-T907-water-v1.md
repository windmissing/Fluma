# T907 · PC 水侧扩散和飞包同根采证合同 v1

2026-11-24 · architect · 目标 7/8 · 离线归因。T901 C 缺少实际窗口上的逐列、事件及 packet 像素原件。现有 FullM1 已写逐列、WATER_EDGE、LAND、ARRIVE 和飞包 publication，PC 也已有单次同根 ROOT 与通用 ROI。首因定为**工程采证接入缺口**，包括锁定连续根、按真实 packet 选取像素及适配独立 checker。待这些证据接齐，根据实际首差再分流。 [T832 工程与判据](architecture-T832-v1.md) · [T852 离线夹具](../test/reports/t852/handoff.md) · [T901 原报告](../test/reports/t901/report.md)

## 1. 已有证据与实际数据源

T896 两种源顺序已提供非零同 lease 共存证据，支持相应目标 7 分项。T895 根 `15944411374190` 有 63048 B 的 M1PROOT1 和实际窗口 768 B ROI，T897 已关联出档哈希、publication 代和 image；沙床和活粒共用一个探针区域。T901 C 未提供独立的水边流/飞包窗口原件，单次日志 `bedEventCount=0` 不能证明事件写入器缺失，也不能作为侧扩散结论。 [T896 离线核对](../test/reports/t496/t896-d1-offline.md) · [T897 独立核对](../test/reports/t897/handoff.md)

冻结水平 FullM1 K192/Kw128、128 列、Q20、wire rev2。沿以下原 backing 读取，先验物理原字，再验同一图像的像素。

| 原件 | 字段、范围与用途 |
|---|---|
| Accepted `Completion256`（1024 B） | 沿现有 root archive reader 核 rootKind、nonce、base/final bank（32/33）、publication slot（35）及 generation、event arena/start/end/capacity（51–54）、生产 fence、同代和原 owner 结果。保 commit/sourceCommit、命令、有效输入、实际受理 Q20。SOURCE 与 QUANTUM 分开解码。 |
| State bank + publication | M1PROOT1 从已接受 final bank 复制完整 state，和同代 publication 交叉核对 128 列。publication SOA 基址以 word 计，`sand16=0, water20=3072, column16=5632, header=7680`，共 31744 B。列组件 `S/W/η/J/P` 在 0/1/2/3/4，索引为 `5632+component×128+column`。原 Q20 非负，`η=S+W`、`S+P+W+J≤256×2²⁰`。另取 packet mass/ID/generation（water20 组件 0/5/11）、位形的实际 FP32 words（16/17）和 header live count。按 ID+generation join，publication compact index 可能重排。 |
| Accepted event arena | M1PROOT1 保留始端填充到 `eventEnd`，只把 `[eventStart,eventEnd)` 当作该 QUANTUM 的增量。每行 64 LE U32、256 B。用 schema used mask、row ordinal、nonce（2/3）、frame（9/10）、quantum ordinal（11）、base/next commit 及 event arena 的真实 backing 核定归属。kind（6）为 LAND=8、BED_EDGE=9、WATER_EDGE=10、ARRIVE=11。端点（20/21）、两端前后账（32–35）；粒/包旧 slot、generation、ID（16–18）及 mass before/after（22/23）供 LAND/ARRIVE 定位和守账。SOURCE 事件沿自己的种类和原表，分清 `P`、`DIRECT_W`、`FLIGHT_J`、床内拒绝和重复请求。 |

原字布局与抽取入口以这些文件为准。 [M1 layout](../entry/src/main/cpp/gpu_resident/m1/layout_generated.h) · [事件 writer](../entry/src/main/cpp/gpu_resident/m1/quantum_core.comp) · [事件阶段顺序](../entry/src/main/cpp/gpu_resident/m1/quantum_stages.glsl) · [M1PROOT1 reader](../test/t496/check_pc_d1.py)

## 2. 采集事务与像素关联

1. 复用 `CaptureM1Quantum`、M1PROOT1 四段和 T890/T895 窗口 ROI 复制实作，新增显式采证模式。按 run、请求序号和根逐个冻结描述符，保有界根档及像素配额。每个连续物理窗口从空档、首 SOURCE 或有实物全量的接受 BASE 起，经过两种源的受理、活动飞包、停源、LAND 和 ARRIVE，直到受控的下一 QUANTUM。物理窗口中每个已接受根留档，即便该根未显示。压线或 resize 记真实终态，缺一个连续接受根就结束当前可判窗口，留下首个缺口及配额/拒绝原因。低频支持 T852 正式判定，支持窗口参数和实测包选择的版本都写进收据。
2. 在业务 fence 和原 completion 通过后，固定这份 root 的 lease。顺序上让 cold reader 及 draw 分别固定同一个后备资源代，在允许复写前把 Completion、final bank、event slice、publication 复制到独立 staging。每个读取方持自己的 pin 并在实际完成后解除；同根共享 lease 身份不等于重复释放同一 pin。现例的 claim/busy 路径同一时间一个冷根，扩展时注册有界 pending/staging，所有完成、取消、排空路径都归还配额。就绪态可同步读同根作诊断，生产帧时须另用关闭诊断的臂记录。复制、invalidate、graphics submit、image copy 和展示分别留原 fence/VkResult/HASH/QPC。超过生成期、跨代及设备失去后的 pin 按真实退役处理。
3. 记录实际 adapter/device/queue、HWND/window/surface/swapchain/geometry/config/optics 代，和 `run/nonce/rootKind/baseCommit/commit/sourceCommit/bank/arena/publicationSlot/publicationGeneration/leaseSerial`。存真实 buffer/allocation/generation/range、物理生产 fence 与 copy fence。图像按同 lease 对齐 `imageIndex/presentId`、acquire/submit/graphics-fence/present 与 WSI 退役；presentId 不可用时以同 swapchain 代、imageIndex 和单调的真实提交/展示回执关联，字段显式缺席。ROI 从同一参与 present 的颜色附件或 swapchain image 合法读回，记格式、通道顺序、rect、有效 viewport、行距、编码和原像素 SHA；区分 Present 入队回执和实际可见证据。失败/STALE/错代不作 CURRENT 样本。
4. 保留实际 `--m1-observe` 单次通用 ROI。新增带 root/lease 约束的 packet 探针选择 `{packetID,generation,phase,ROI}`。用已冻结飞包的位置和质量在图像录制前确定小区域，可用有界 GPU witness 或已复算的帧索引夹具冻结 rect，再用同根原档核实。按 PC 实际 shader，映射中心 `(x/128,1−y/256)×extent`，半径 `2√(mass/131072)` 像素。保至少一个实际有覆盖的内部像素、相邻无 glyph 对照和独立同帧 W/沙对照，检查裁剪、覆盖、alpha、高光及单次 sRGB。安排无重叠或遮挡的 fixture，ARRIVE 前选目标包活动且对比充分的一帧，后帧锚定其真实 ARRIVE 行、同 ID/代不再出现在活包列表、W/η 增量和原画面中该 glyph 已消失。后帧可见底色随 W 变化，独立按后态算期望。无可识别粒包像素时保原因及原件，`packetProbe` 只给予已确认对应的像素。

沿 PC Vulkan 把实际角色、视口与渲染格式写入，不用一张示意图或重绘替代窗口帧。场景资格继续核 T832 层序、线性合成、W0、干槽、深水、真实 η 和飞包消失；2 LSB 沿用 T281 的固定深水对照。GPU 驻留、同根 shader 场景，禁止 Canvas 回退。 [PC ROI 接入](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [ROOT 复制与 pin](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc) · [PC 真实着色器](../pc/vulkan/m1_dry_scene.frag)

## 3. 独立判定与接手角色

- 四组从空 run 开始，沙→水、水→沙各做 `κ=24` 和 `κ=0`。沿原 Win32 真实选源、Down/持按/Up 输入和时钟，存 command32、源终态和换源/撤权。现 PC 的 quantum 参数默认 24，新增只在采证 profile 使用的冷配置 0，收据冻结实际参数、输入和 SPIR-V/PE 身份，无运行中改 κ。种子选水平中央入水且两侧容量可达，必须覆盖两个方向真实 WATER_EDGE，零 κ 无该类事件，落沙及入水停源后仍能继续。另保 DIRECT_W 和床内拒绝、重复/过期不增加账本的负控。
- tester 从原始 BASE 逐事件核 Q20 前后值、真实行序和整根 `Σ(S+P)`、`Σ(W+J)`，来源新增只从接受 SOURCE 终态记。水平 fixture 用两色 `serial&1`、其异色顺序，从阶段旧态独立重算 head2 的五界。每条 WATER_EDGE 的给收方 W 等量，BED_EDGE 是 S 搬移，LAND 是 P→S，ARRIVE 是 J→W。两个水色全部完成后才冻结 η 及湿介质并推进在途粒，ARRIVE 的新增 W 下一 QUANTUM 才参与扩散。按实际原序重放沙 BED_EDGE 和 LAND；T852 v1 标准化 checker 只表达两色水边和 LAND/ARRIVE，存在 BED_EDGE 时扩展独立 decoder 与测试覆盖，原事件都保留。多包画面使用 ID+generation 的独立核验，T852 v1 的按列 `bool(J)` 只适用于列内对应包无歧义的受控夹具。
- developer 复用现有 ROOT/ROI，先交一组真实 pin、范围、fence 和四段容器收据，再接连续采证、packet 探针、κ=0 冷对照和原证书。tester 可以先接原档标准化、事件序和像素 checker，并准备缺根、错 lease/旧代、错 imageIndex、假 packet ID、晚 fence、错误库存和编码等负控。effector 用同一可复现工件采实际 PC 窗口、接受根及图像，逐项报告可见结果和原件来源。已受理但曾被捕获配额漏掉的事件仍记缺证。若原字连续但事件/账或其实际 renderer 读绑定出错，保首差回 architect；实现与独立物理 oracle 都一致，而目标效果仍偏离时，交 designer。目标 13→14→12 的纯流及载荷资格独立。

旧软件后端或合成测试仅给 checker/像素基线，PC 产品目标等待上述同根证据。 [T852 check.py](../test/t852/check.py)
