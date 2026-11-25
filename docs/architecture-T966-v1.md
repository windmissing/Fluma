# T966 · 集成 Demo 四秒见堆与 G16 第三撞首次归因

2026-11-25 · architect · 冻结 T960 `7dd68625` 和 T965 `627798d9` 的原件，仅离线读取。Demo 的首个可量化缺口是**已产 SOURCE offer 到够龄根边界实际领取的有效时段覆盖率**。G16 的确定工程缺陷是 **Completion 余时字段覆盖了完整模块摘要的末两字**，正置和镜像夹具皆在宿主身份检查失败。两项都有可立即实施的工程工作。真实按住和同粒倾斜、G16 第三撞具体 key 与底库逐字证明仍依赖实际补证。 [T965 报告](effect-report-P053-T965-integrated-demo-v1.md) · [T960 工件合同](developer-T960-v1.md) · [离线复算](../test/artifacts/t966/evidence.json)

## 1. Demo · SOURCE 实收和四秒墙钟推进

### 冻结和判定

- `b5-*` 的实际原 PNG 逐像素计数为 **0 → 38 → 42 → 107**，取精确 RGB `(184,145,89)`。四秒图在完整 PNG `y=328–330` 有 7 px，`y=350–354` 有 31 px。六十秒后图为 `x=317–340,y=348–354`。采用完整 PNG 坐标时，四秒已经有零散沙色到达底部，同时保留高于底部的沙色。报告中的小坐标未给出裁切原点。PrintWindow 图为 640×470，日志中客户区和 Vulkan viewport 为 624×431、DPI 96，截图没有逐图 QPC、lease 或图形 fence，故仅用于观感与总面积复算。38/107 不能作同一粒的两点轨迹，也不能据总面积证明三列物理床高。 [四秒与六十秒原图](../test/artifacts/t965-g16-demo-20261125)
- 匹配首段 118 条 MOVE、第二持按 ID 122、时窗和 `b5` 原图时间的是 `%TEMP%/HelloWorld-Demo-t965-r4/demo.jsonl`。已冻结完整 5,259 行，原始 SHA256 `89c2ada92870a24869fd55c5f7cfe8834447af9ac4ea0d07020c9adad52a3879`。T960 manifest 五件复验全部相符，运行日志三个计算模块也与 T960 对照。`b5` 时点和 r4 对应属于日志/文件时间关联，逐帧图形代仍须补。T965 现场 GPU 为 UHD 630 `8086:3e98`，Vulkan 驱动原值 `1656924`。 [冻结原日志](../test/artifacts/t966/demo-r4.jsonl.gz) · [复算脚本](../test/artifacts/t966/audit.py)
- 输入是 `DOWN seq=1 → 118 MOVE → UP seq=120`，实际按下窗口 **4.040672 s**，`CAPTURE_LOST seq=121` 后 `DOWN seq=122` 开启另一笔 **5.014162 s** 持按。**122 是第二次按下的事件身份**。两轮均为 `PostMessage` 合成输入，记录点击客户区 `(320,235)`，SOURCE binary64 为 `(66.465197,26.199536)`。T940 默认中心 `(约64,32)` 和低位 `y≈16` 各需自己的固定点测试。 [T940 指针中心及体验口径](architecture-T940-v1.md)

### 按原字分解

| 墙钟窗（首 Down 为 0） | SOURCE 实收与出生 | 实际量子与呈现 |
| --- | --- | --- |
| `[0,4)` | 12 根，52 条 BIRTH 计数，6,158,872 Q20 = **5.873558 单位**，1.468390 单位/s。12 个已实收窗口的真实相邻脉冲合计 **183.5488 ms**。 | 196 个已接受 Q，**1.6333 仿真秒**，93 个同根 Present 回执。末 Q 有 14 活沙、178 空沙槽。首个 SOURCE fence 在 +0.0981 s，首个相关 Q Present 回执在 +0.1366 s。 |
| `[0,UP1)` | 13 根，57 条 BIRTH 计数，6,726,176 Q20 = **6.414581 单位**。第 13 笔在 +4.014 s，已超四秒边界。 | 197 Q、94 Present 回执。第一轮松手后的 +5.3471 s，已接受根活沙首次归零，库存仍为 6,726,176 Q20。 |
| `[DOWN2,UP2)` = `[9.0745,14.0887)` | 16 根，63 条 BIRTH 计数，另实收 **7.034540 单位**。到第二次松手有一个原始 `SOURCE_HOLD_REVOKED`，无新增实收。 | 255 Q、126 Present 回执。 |
| `[0,60)` | 合计 29 笔、120 条 BIRTH 计数、**13.449121 单位**。最后实收在 +13.703 s。 | 4,043 Q、1,428 Present。全日志 4,356 Q 序号连续，沙总库存逐根等于对应 sourceCommit 累计实际入量。60 秒以后 313 Q、104 Present 回执，0 笔新 SOURCE。 |

1. **输入到 SOURCE 首差。** Hold 在窗口中有效。前 12 个接受 offer ID 为 `1,14,36,42,48,80,116,136,142,170,185,209`。已实收根逐笔额度等于 `32 × 相邻真实脉冲秒数` 的 Q20 floor，已接受 SOURCE 的拒收量均为 0，通常每根出生 3–5 粒。原码 `WM_MOUSEMOVE` 更新 held 指针快照，`WM_TIMER` 名义 10 ms 并按真实回调构造相邻 pulse，`OfferLatest` 保存最新 offer，owner 只在有效量子根/计划边界调用 `takeFresh()`。MOVE 数、持按 ID、PRESENT 数分别计量。现成日志缺逐 offer 终态，不能把所有 skipped ID 精确分到替换、过期、未到边界或每类授权拒绝。第一批证据支持优先检查 fresh offer 可领取覆盖和边界耦合。 [Win32 Hold 和脉冲](../pc/vulkan/main.cpp:692) · [SOURCE 计算额度](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc:786) · [领取时点](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc:1546)
2. **物理推进与画面。** `35 dispatches` 是一根 Q 的实际计算派发数。原节奏由 `pacedPresent` 和下一帧 1/30 s 目标控制，可在同一呈现窗接受最多四根 Q，首个合格 BIRTH 可优先。前四秒 accepted Q 的 submit-return→fence 中位 **16.5684 ms**，含排队和宿主观察，尚无同根 GPU 分相计时。实际推进和沉降时间均需计入。对照 T925 的旧慢/停条件，本轮四秒末空槽 178，日志后段两次 `CLOCK_GAP` 已在松手后的 34/43 s 左右，后面仍有新 Q，时序无法解释四秒缺口。 [呈现节奏](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc:1585) · [T925 退役和停料归因](architecture-T925-v1.md)
3. **初步分流。** T908 三相邻列 6 物理像素门，旧 784×461 参考视口的必要沉积至少 10,481,211 Q20（约 10 单位）。用 r4 实际 scale 3.448 按同一公式，三列需要约 10.69 单位，四秒实收 5.874 单位，因此即使全部瞬时沉积也不足该数量级。应对正确视口用实际 128 列 S/P 重算，当前日志中该路径 Completion 的通用 `settled/flight` 字段没有有效值，只有总账、活槽和外部 PNG。四秒具体 LAND/底部 ROI、实际物理耗时下界按后续 fenced 样点核验。更大堆的 107 px 出现在两次早期入料和后续沉降后，原长续沙门须在 +60s 后实际再接收新 SOURCE。 [T908 床高和守恒门](architecture-T908-v1.md#3-b--入料落床和可见堆积)

### 工程交接

1. **Developer 可立即实施。** 复用同 T960 SHA 的固定率、SOURCE 发根、真实坐标与呈现，补轻量计数或受控诊断。计每 hold 实际合法相邻 pulse 宽度、授权快照、`OfferLatest` 终态（提供/替换/过期/被领），GPU 宿主边界是否 idle、activeRemaining/pending、claim、实际请求/实收/拒收、同代 Q/WSI 和第一次底部达标。默认与窄观测保持统一调度规则。第一个修订对照可在每个**现有合法计划/根边界**优先检查仍新鲜的 SOURCE，避免空过 eligible 边界或被呈现等待延误。资格核 hold、geometry、material、clock session、permission epoch 和现有来源上限。保最新有效指针、重复归零、取消及无旧配额补领。先量承诺率及 accepted SOURCE 时段，固定 32 rate 时旧参考至少约需 **78 ms/s** 合规实收窗口，本轮仅 **45.9 ms/s**。正向减少错过的真实窗口后，再复核 Q/s、低位与默认中心落距和同根 S/P。若仍有充分合法窗口、同时实际业务 budget 或积分速度不能满足，按 T925 分纯 Q/SOURCE+Q、同代 GPU tick 和 WSI，转对应工程优化。若产品选择改变出料规律、物理时间或落点，则由 designer 接受实测和约束后裁定。
2. **Tester 可并行离线核。** 复用现成 T950/T940 extractor 与本卡冻结样本。独立核 offer 账不可重复，置换/超龄的终态无实收，两个 hold 不串联，失焦、换材、移出、旧 transform 和重入的动作边界保留。用例应可杀死“每个 MOVE 就发一粒”“窗口/旧 capture 补历史欠账”“把早期两次持按算作六十秒续沙”“把已接受 Q 的总库存当静床”这些假阳性。
3. **Effector 可先补原包真实输入和粒子证据。** 固定同一 manifest、实际 viewport、角和初始空盒，独立运行前台真鼠标静按 4 s 与真鼠标短幅扫动对照，另用同坐标同持续时长 PostMessage 静止/118 MOVE 回放作合成对照。真实臂记录前台、实际鼠标物理按住认证、指针坐标、QPC、capture、Down/Up，明确输入产生方式，不能根据同一个 `WIN32_POINTER` 标签判断是否人手。默认臂以实际中心为主，高位、低位各记录，六十秒后新 Down 加非零 SOURCE，松手后无新 SOURCE 并留床。默认模式取 Q/WSI 时序，`Start Demo.cmd observe` 补中心、高位和实际接受且已画 ±20° 的首次 BIRTH `id/gen/origin`、至少三点连续无接触干沙段、回正实际接受角和箭头像素。按已接受 SOURCE 的 nonce/commit 与 Q publication generation、View revision、graphics fence、imageIndex 和 ROI hash 绑定。按 T940 出生门 ≤1 px 逆投、≤2 px 绘制误差、空盒首粒距指针 ≤3 px，实际自由加速度角偏 ≤0.1°。冷档中变量存在与计划通过分别裁定。无法取到则报告具体拒因，若已有观测入口在实际 GPU 证实缺字段，再转 developer 补接口。 [T950 同粒及取行口径](architecture-T950-v1.md#2-世界重力--区分箭头盒框和真实轨迹)

## 2. G16 · 确定首差与夹具重验

| 臂 | Runtime `cfg62/63` 及 Completion 回显 | 原 1024B Completion 的 outcome/reason/stage/micro；w126/127 | 对应公开结论 |
| --- | --- | --- | --- |
| B13_FREE | `0/0` | `2/11/9/16`；`2/3` | GPU 执行，普通场景根未提升，身份失配。 |
| B13_SUPPORT | `1/0` | `2/11/9/16`；`2/3` | GPU 执行，场景根未提升，身份失配。 |
| THIRD | `0/1` | `2/11/9/15`；`1/3` | `passed:false`，`privateCommit=0`，`acceptedRoot=null`。 |
| THIRD_MIRROR | `0/2` | `2/11/9/15`；`1/3` | `passed:false`，`privateCommit=0`，`acceptedRoot=null`。 |

**锁定第一处身份差。** 模块 `freeze()` 把 config `64..103` 的五组完整 digest 复制到 Completion `88..127`。其中 `120..127` 为实际 G16 模块 SHA `b9cab892f95da3b5402696d26ec41d1fe8acd2e89f636ada358208cf2c557923`。随后 `seal()` 以余时次数及末分支覆盖 w126/127，宿主 `CheckIdentity` 仍进行 `same(88,64,40)`。独立比较四臂冻结 config/Completion 摘要，**第一差 w126**，实际 1（THIRD/MIRROR）或 2（FREE/SUPPORT），预期 `3473441333` (`0xcf088235`)，随后 w127 实际 3，预期 `595154220` (`0x2379552c`)。实际没有尝试时 `put(126,0)` 也会覆盖摘要。原 Completion 合法 magic `0x36314743`、version 1、长度 256、seal `0xC016CAFE`，余时 tape 的独立 magic 为 `0x47393654`。 [宿主检查](../pc/vulkan/sand/g16_multi.cpp:138) · [冻结和封印](../pc/vulkan/g16_multi_v1.comp:930) · [实际原字](../test/artifacts/t966)

**安全拒绝与期望正臂。** T965 两臂业务 fence 实际成功且 current/fresh，身份失配令 `valid=false`，同 fence 诊断只写 `late-completion.u32le`，`Export` 未执行，因此没有 `certified-tail-v1.u32le`、冷拷代 BASE 或 `thirdVerified_`。码 3/微步 15 与第三撞分支一致，具体碰撞 key、TOI 括号和 BASE 相等当前尚缺原字。正置 `disk=13, wall=3`，镜像 `disk=13, wall=2`，要求微步 15 两次真事件后，余时内约 4 µs 向内根（夹具现有容差 ±0.05 µs），Q24 夹括及两侧有界正/负间隙，第二盘无、余时消费为 0、BASE 逐字等于实际加载种子。此处 `passed:true` 是夹具正确检测并安全拒绝第三撞，其产品资格始终为 `privateCommit=0 / acceptedRoot=null`。`support-check` 独立标量通过仍有效。真实 B13 FREE/SUPPORT 的 micro 16 是另一次余时拒绝，须从完整 tape 核实首碰及未消费余时后才定后续物理归因。 [原夹具判据](../pc/vulkan/sand/g16_multi.cpp:452) · [SAFE 提升门](../pc/vulkan/sand/g16_multi.cpp:670) · [T956 物理合同](architecture-T956-v1.md#2-g16--交-designer-的承托与余时裁定)

### 最小修订与复测

1. **Developer 可立即实施 ABI 修订。** 以成对版本化 PE/SPIR-V 修复字空间。建议在 256 words 中保全 `88..127` 五组 digest，迁到明确分配的 Completion `128/129` 余时计数/状态，并让对应新版本的 reserved 检查从 130 起。同步普通/夹具 PE、模块 SHA allowlist、布局元数据/报告和读档 checker。老无余时版本沿各自合法布局解析。T960 发生重叠的旧版本只能作为历史失败诊断。完整摘要比较、原 250 ms、`current/fresh`、source fence/代严格关联、`rootKind/outcome/reason`、fixture 特殊安全拒绝都保持。余时冷档采用已有 `certified-tail-v1.u32le`，`late-completion.u32le` 标为同 fence 后续读回的 Completion；验证 tape 的 magic、行数与 Completion 镜像值，输出逐检查项的首差字段，不再只给一个 `identityOK:false`。修复仅影响归档布局、宿主读取与诊断时，保物理核的算术和派发合同。
2. **Tester 离线可并行准备。** 用本卡四份 `late-completion.u32le` 作为 v1 重叠负控，独立构造新版零尝试/一次/两次、SHA 末字错、身份版本错、模式位错误及 padding 越界。分别断言 digest 完整性、余时 tape 元数据、当前试验资格和业务提升门。fixture 的原子负控包括不可晋升、镜像墙号错、TOI 越窗、余时已消费、BASE 被改、过期 fence/错代，复用 T958 的静壁 FP32 独立参考。旧格式已留原证，无需再上机复现身份失败。
3. **Effector 复测前置为配对新 PE/SPIR-V/manifest 和独立 checker。** UHD 630 按 T960 同五命令复用 B13/static/J128、同种子及镜像约定。记录同一原 fence/代的 hot Completion、首字差、诊断声称身份、冷拷代、整盘/壁计数、完整余时行、BASE/CANDIDATE、运行 QPC 和两个安全字段。预期修复后四业务臂身份通过，THIRD/MIRROR 在具体 key/TOI/符号/未消费/BASE 全证时 `passed:true`，仍零私有 commit。SUPPORT_LOCAL 继续独立局部原样核验。若身份完全通过以后仍有冷档/语义首差，先回 architect 判归因，只有独立原字证实物理合同或反力/区间界本身矛盾时才交 designer。

两路软件和证据可分别推进。唯一体验入口仍需真实同包回归。仅 Intel UHD 630 为交付与验收环境。本卡没有启动实际窗口、设备测试或 GPU workload。仿真 GPU 驻留，XComponent+shader 唯一上屏，禁止 Canvas 回退。
