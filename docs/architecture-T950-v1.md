# T950 · 集成 Demo 出生与世界重力复测归因

2026-11-24 · architect · Intel UHD 630／Windows Vulkan · 离线。T947 的 108.4 px 来自旧粒子的 QUANTUM 运动行；同批存档的四条真实 SOURCE 出生在指针，首粒像素仍缺。正负倾斜的像素原件是空盒轮廓，粒子自由落体方向仍缺同根证据。G15 除业务超期外，已封存负向臂另有可复算的中点物理拒绝。 [逐项机器复算](architecture-T950-evidence-v1.json) · [T947 原报告](../test/reports/t947/report.md)

## 1. 出生 · 纠正判据的取行，保持 T940 门槛

旧结论把相同的字偏移当成相同事件。在 M1 Event64，word 26/27 对各类事件均为 after x/y，只有属于已接受 SOURCE 的 BIRTH 才是出生。以下原件来自同一 test/reports/t947/runs/pixel-birth。

| 层级 | 可复查证据 |
|---|---|
| 指针与实际命令 | 日志 264，MOVE seq8、hold1、客户区 (312,215)，呈现 serial53、transformRevision1、DPI96。日志 288 的同一指针 SOURCE nonce **16800600508902**、sourceCommit13；实命令 x=64.14501159、**y=32**，binary64、命令 SHA、实际接受与 4 出生数一致。 |
| GPU SOURCE BIRTH | root-16800601145319.m1root 的四条 SOURCE 前缀为根类/阶段/事件 **1/1/1**。id/gen 为 50/2、51/1、52/1、53/1，origin=13，words 26/27 **67260919/33554432**，回投 (312.499998,215.5)，与采样中心差约 **0.0000023 px**；实收合计 504,470 Q20。归为水平中心持按样本的出生原字阳性。 |
| 108.4 px 来源 | 归档 ordinal5 属上一 QUANTUM nonce **16800600838798**，根类/阶段/事件 **2/2/3**，是 id5/gen1、origin2 的 MOVE。y 从 605394 到 **586753** Q20，tau 为 0→65536，两端为干介质，确认为干沙运动小段。回投 (312.50,323.9066)；后续 id5 出现 LAND。 |
| publication 与实际像素 | 源后首个 Q 的正常 WSI 回执在日志 288，commit62/slot0，已报告 birth priority。日志 293 的 fenced 帧 50 是后续 Q，commit63/slot1，同根 publication 中四个新 id/gen 仍在 (312.50,215.745)，距指针约 0.245 px。实际读回 ROI 在 y=308–323，只有床附近，无法检验上述四粒附近的实际像素，也无法代先前那一帧。 |

原 SOURCE shader 对干沙直接消费 Q20 位置，只有床下才按原拒因拒收，确认该样本没有贴床出生分支。保 T940 的出生回投 ≤1 物理 px、首个绘制位置像素中心误差 ≤2 px、空盒首粒距指针 ≤3 px。非空床、真实高位、新几何及被遮挡仍按各自原件判定。 [原日志](../test/reports/t947/runs/pixel-birth/demo.jsonl) · [同根冷档](../test/reports/t947/runs/pixel-birth/root-16800601145319.m1root) · [SOURCE writer](../entry/src/main/cpp/gpu_resident/m1/source_core.comp)

## 2. 世界重力 · 区分箭头、盒框和真实轨迹

- 三轮 freefall/freefall2/freefall-neg 各取第 15–39 帧，共 **75 帧**。全部在各自 Down 前约 **2.75–3.78 秒**，实际接受和绘制角只有 +5°/+10° 或 −5°/−10°。每份同根归档 EVENT、活动沙、床 S 和在途 P 都为 0。B8G8R8A8_UNORM 的 sand 色 (89,145,184) 为 0 像素；最大 48 的彩色计数来自 (223,192,116)，对应 shader 盒框混色。旧报告的左右镜像是盒框，缺少已出生的同一粒子，因此无法裁定屏幕水平位移。 [正臂原件](../test/reports/t947/runs/freefall2/demo.jsonl) · [负臂原件](../test/reports/t947/runs/freefall-neg/demo.jsonl) · [盒框和沙色 shader](../pc/vulkan/m1_dry_scene.frag)
- 后续日志有实际 SOURCE 和第一 Q，正负两臂首个 SOURCE 后画角与接受角均为 **±5120/256°**，但这些帧已超出观察窗口。采样不足先归在观察触发和检查器，当前没有证据指向 T942 的自由段物理公式出错。原箭头在集成分支用固定屏幕 (0,1048576)，它只证明送画的意图；另取实际 HUD 像素核箭头。 [实际箭头与帧绑定](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc)
- 独立用实际角表和现有干沙自由段：±20° 时 sin Q20=±358634、cos Q20=985339；g=512、无阻、整 Q 时，局部 Δv 的原字期望 (±1530172,−4204113)。以同 accepted CompactView 回投，水平投影误差约 **5.8e−7°**，只是当前算式与量化的参考。从同一 id/gen/origin 的连续实际 Q 取 x/y、vx/vy 及 tau，选能证无墙/床/液面阻断的干沙段，核每段 Δv 的方向与独立位置更新。水平投影取 δv 而非仅看端点，避免初速度、槽重用、进入下一列、接触和视角变化掺入。跨列后若有预算/墙面事件，按碰撞组另归因。 [角表](../entry/src/main/cpp/gpu_resident/angles_generated.h) · [量子积分](../entry/src/main/cpp/gpu_resident/m1/quantum_t16.glsl)

### 最小补证和交接

1. **Tester 先纠正取证。** 从完整 M1PROOT1 的 SOURCE 有效前缀读根类/阶段/事件和真实 SOURCE nonce，不依赖现 extractor 默认只截本 Q 有效后缀。再联动 pointer seq/hold、几何/View revision、sourceCommit、slot/id/gen/origin、compute/graphics fence、slot/publication generation、imageIndex、像素哈希。遇 SOURCE 失败/去重、后缀被轮换、同槽换代、同色盒框、观察窗落在 Down 前分别拒。第一根及后续仅按 accepted token 入样。以当前归档作为「正确出生 + 旧 MOVE 混入」负控，图形色别从脚本实际格式解码。
2. **Developer 补窄观察入口。** 优先复用 T942 已有 birth priority、WSI fenced ROI 和冷档。把观察起点绑定到真实 Down 的首个接受 SOURCE 或其第一张含该 BIRTH cohort 的 Q，最多采该 SOURCE 及接下来一段限定窗口，跟踪真实 sand 身份及同帧角度。当前 observe-packet 的热坐标入口是水包，需要复用出版段格式新增 sand 路由。第一粒/同 SOURCE 同位置粒组在空盒先完成，图形检查先取 pointer ROI 与稍下方，再按同帧 publication 跟踪。实际取不到时具名 NO_MATCHING_ROOT、NO_BIRTH_PIXELS 或 MOTION_CONTACT，不能用默认固定帧号或假标记。默认启动逻辑、shader、SOURCE 位置与重力继续用 T942。观察要记录是否关闭了 pacedPresent，首帧资格需要能以默认节奏保取，不把打开观测后的帧率写成产品结果。
3. **Effector 只补缺口。** 同包跑水平中心和高位，再在**实际已接受且已画** ±20° 后对空盒固定点下按，动态取同 cohort 的第一张 fenced 像素和三个以上有效自由运动点。另测已出沙的固定角场景、BED_SHAPE 受限和回正，分别拍重力箭头。已过记录保留。T947 中心 ROI 有同根沙色 [1,3,0]，从 Down 到 Present **1.576835 s**，代表早期可见，原 T908 三列各 6 px 的较强证据尚缺。默认主轮在 60 s 后实际再接受 25 笔 SOURCE，末笔 89.170 s；新包还需保四秒见堆、高位不降出生、松手留存、六十秒和恢复/续沙。同一 manifest 通过后才交唯一集成入口。

## 3. G15 · 共用裁槽接力，同时保独立干前沿问题

- T947 正反各三轮原包均 45/45 建管线返回、BOOT 达，正式 opcode23 资格已证。六个首 QUANTUM 均 2404 逻辑派发、业务 VkResult=2，正式接受缺席。负臂封套例从 grant 到提交返回 **48.0729 ms**，剩 **201.9271 ms**，用同句柄 gen2 业务及 gen3 诊断。新试验只用 UHD 630，不重复 T947 同条件重测。
- 负向封套 18 件 SHA 已核，BASE==CURRENT_BASE。晚尾完成计数仍 2404/2404/2404、0x3ffff，但 **outcome4 / reason7 / stage26 MIDPOINT_BUILD / micro0 / col19 / operand40**，selected J=0。六个 J 的失败 tuple 相同。由 COMMON 实际 offset 解最后保留的 WORK：col19 进入预测的 F=0、pEntry=0、pTrial=pEnd=−64868，得到 pMid=−32434；对应边20 重构 F=(0,1395)，实际 q=0、jp=+64868，形成了给空 F 列的净动量。原硬门阻止了候选提升。具体动量数值只取封存 WORK，正臂目前只有业务超期。 [负臂原封套](../test/reports/t947/runs/g15/neg/diagnostic/1680438448607400-1/envelope.json) · [物理预测与中点](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp)
- **Developer** 复用 T946 的 Q 同 fence 计时、T948 的只裁已证零副作用空 extent 判据、原计数/拒绝前缀和实际 RAW 冷档。G15 仍绑定自身镜像 seed、角色包、角表、±11520 端点、真实角时序和独立 typed/守恒/同根 shader。先带一对有实际作用的冻结 profile 与 Q 相位采证；裁空槽须保无可用 J/全部拒绝负控，确保不跳过 6 次真实失败前缀，时间优化独立于物理。已有旧超期记录作历史对照。
- **Designer** 用现成负臂 COMMON/WORK 和干湿流界的窄反例审查「零移动库存 + 无搬运质量边仍传压力动量」在冻结 F 预测中的分流。比较原 S40 同条件、单面有/无源以及镜像前沿，需用守恒/壁反作用给出确切的动量归属或一致的中点库存方案。保持 q=0 但两侧有实际移动库存时合法的压力耦合，保原 reason7 和守恒门。待有算法裁定再给修时序、原动量账的实施。集成 Demo 出生/箭头取证独立进行。

本轮能纠正出生与轮廓误读，并给出 G15 负向晚尾复现。±20° 带粒的同根自由段、第一新粒的 fenced 像素与新版 G15 typed 根仍待上述实际取证。GPU 驻留、shader 渲染，禁止 Canvas 回退。
