# T944 · 目标6在 UHD 630 上的实时推进和呈现

2026-11-24 · architect · Windows Vulkan。目标环境为现有 Intel UHD 630（8086:3e98，31.0.101.2140），正式复采按单一集成入口。目标6分别衡量 30 fps 和真实物理推进。T941 证明持续接纳和后段仍出沙，帧率和物理速度均有差距。 [T941 原报告](../test/reports/t941/report.md) · [按根复算](architecture-T944-evidence-v1.json)

## 基线和预算

| 口径 | 同版 UHD 630 数据 |
|---|---|
| 默认长窗 | T933 PE 4a40a141…、35 派发，117.2406 s / 6571 个接受 Q，54.7583 仿真秒，0.4671 仿真秒/墙钟秒，约 56.05 Q/s。实际提交 6574，2 CLOCK_GAP、1 WINDOW_REVOKED。实收 SOURCE 129 根，最后一笔在首 Down 后 113.046 s。末次真实接受根的配额账 14005 = 6571 accepted + 7419 overload + 9 revoked + 4 pending + 2 active。 |
| 默认呈现 | 2821 个不同物理提交的成功呈现入队，均有对应 graphics fence。fence 间隔 p50 39.81 / p95 50.95 / max 452.49 ms。跨度 117.096 s 的 2820 个间隔，平均约 **24.08 fps**；中位间隔倒数约 25.12。fence/入队是交付到呈现路径的代理计量，实际新图像是否显示还要同帧像素或显示事件配对。 |
| 短窗诊断 | 同 PE/模块加 --m1-stage-timing。1101 条查询中 1075 Q、26 SOURCE，两种各一拒。按 run/geometry/nonce/fence 和真实接受根连接。**1074 个接受 Q** 的设备 span p50/p95/max **14.98/18.59/43.55 ms**，宿主 record→stop p50 15.59 ms，接受间隔 p50/p95 16.04/20.72 ms。**25 个接受 SOURCE** 设备 span p50 27.20 ms，91.09 ms 最大值来自此类。15.076 ms 复现的是合并 Q/SOURCE 的中位。 |
| 短窗图形 | 440 个同根 draw，device p50/p95 0.253/0.329 ms；graphics submit→fence p50 0.529 ms。fence 间隔中位倒数 25.32 fps，跨度平均 24.62 fps。实际 WSI 节奏、显示采样和 draw 要分别看。 |

从长窗到 1.0 推进约需 2.14 倍，等效每 Q 的平均墙钟成本需从 17.84 ms 到 8.33 ms，减少约 **9.51 ms/53%**。30 fps 从约 24.08 fps 提升约 25% 即可达到。提高图形节拍可能先达 30 fps，物理仍可能慢速。以单队列实际 root 计量，模拟 120 Q/s 时 8.33 ms 还要分给真实 SOURCE、host/同步、图形与尾延迟。可先用 7.0–7.5 ms GPU Q 和 ≤0.8–1.3 ms 其余摊销作为测量预算，沿真实同窗核实际开销。预算是推导值；根业务期限仍为 250 ms。

| 进栈活跃物料 | Q 数 | Q p50 / p95 ms | PREPARE | MOTION 段 | 独立审计至 replay 开始 | replay event | SEAL 区间 |
|---|---:|---:|---:|---:|---:|---:|---:|
| 0–7 | 521 | 12.50 / 16.33 | 3.74 | 1.94 | 2.19 | 1.41 | 1.14 |
| 8–15 | 251 | 15.61 / 19.14 | 3.80 | 2.96 | 2.24 | 2.97 | 1.16 |
| 16–31 | 302 | 16.52 / 19.14 | 3.85 | 3.37 | 2.26 | 3.60 | 1.17 |

短窗段值为同队列边沿间隔，包含必要同步，不能直接相加各自中位为某根。主排序约 0.09 ms，先处理常驻固定成本和随活动行增长的段。T931 的旧 35 派发重负载 p50 190 ms 属 T926 版、活沙 96–107，当前诊断根无 ≥32 样本。高位如实出生、满载及水/干复合须新取工件，不能把旧 190 ms 外推成集成版重态。 [T931 工件界限](architecture-T931-v1.md) · [T940 真实出生合同](architecture-T940-v1.md)

## 一轮实施，先消除每根固定工作，再降低活跃行成本

1. **基线和对照接缝。** 固定同一 GPU BASE、真实 SOURCE 输入/物理 Δ、角、许可档和源前缀，采用 T931 同输入诊断及现有私有 replay 接缝。旧相邻时钟只保参考，本次 permit/fence 用真实时钟。记录 pool/reset serial 与根出入；同包切现有 35 点位开关，参数一致比较 observer 效应。38 派发用 --m1-audit-breakdown 独立采库存、角/来源与余下审计，在结果中保持实际 38。补 PREPARE 库存/角/复制边沿、SOURCE 旧扫描、motion 计算与提交、SEAL 归约和有序摘要边沿。默认长窗留独立结果。 [T933 构建及开关](developer-T933-v1.md)
2. **首个可交增量是 PREPARE + 独立库存审计。** 当前 PREPARE 仍用 compact_inventory 中的活 ID 二次比较；T933 的排序去重与二分角门接在 commit 模块，不能当 PREPARE 已优化。先版本化一组 candidate/commit 的两份 independently checked 实现，按槽并行提取质量、有效范围、字段和来源键，按列固定分块归约，活 ID 稳定排序后邻接核重。设 K=分配槽，C=128 列，A=活粒/包，将重复扫描压至 O(K+C+A log A) 工作量级或固定 radix 等价界。使用 checked 宽总账和原饱和序，确定性保相同首失败阶段/列/槽。分别从冻结 BASE 和真实 CANDIDATE 扫，保空槽/边界、实用来源和独立性。T933 角表 15,361 行排序先验证，运行继续用二分；SOURCE 前缀仍按完整账试验，严禁以候选摘要代替独立核对。Profile 先保 35 路径的字段，新增物理 dispatch 时按实登记并更新对应计划版本。
3. **同轮第二个独立增量是 motion 与 replay 的行并行。** 记 R 个有效行、D 个有序依赖深度、L 个封存来源、O 个触及槽/列。可并行的是行形状/范围/类型、几何候选和只读前提，目标是 O(R+O) 的工作量；实写集合冲突、容量信用、跨列账和老值依赖按原顺序。只在证明两行读写范围分离、相同首错及原位语义成立时并行提交；可用 t933_event_work 的槽/邻列依赖上界，再以实际字域证书纳入执行。T941 同帧观察根 16745311764926 有 47 条事件都触及列 64，保守链为 47，适合作共同列的对照。独立 replay 仍从 BASE 开始，核全部使用行及确定未改区。记录并发行批与临界 D。SOURCE 独立复用按槽库存原语并核其 32 字命令、逐笔 BIRTH/LAND、质量/来源。若源前缀 L 或摘要形成实际大头，再做独立第三步；链式 SHA 保原顺序和输入。 [事件工作量工具](../pc/vulkan/t933_event_work.py) · [GPU 时标入口](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc)
4. **预计收益和窗口接力。** 诊断的 PREPARE+独立审计约 6 ms，固定段若去掉 2–4 ms 即先复测；由 17.84 ms 基线粗算约 63–72 Q/s。该增量不足以独立达到 120 Q/s。要求 16–31 档继续实测 motion/replay 两组降低、SOURCE/SEAL 和 host 头部压缩，按有效待调初始工作量逐项归因。调度沿现 4 信用上限，独立调节计算产出和以 30 Hz 消费最新同代有效 lease。现串行 nextPublication 在主图形循环内积累多 Q，抓取 33ms 相位漂移、graphics pin/WSI image ready、SOURCE 占用、主动 sleep 和空闲期。先试以绝对显示截止取最新实际 accepted publication，保护两槽、fence、OVERLOAD 及帧代。空闲/旧帧和重复 commit 单独报，无有效 Q 依照原 STALE 规则。若要改 shader 像素路径，只有更大场景证实 draw 已成负载才投入 tile/bin，保原像素/色彩和同根序数。

## 证据和完成条件

Tester 用冻结 BASE 测空床、等质量全床、A=8/16/32/64/96/120 的落沙和代表水包，覆盖高 R 与高 L 独立分层以及轻量重复场景。每次只切一个子增量，ABBA 对照相同 GPU/驱动/工件和实际冻结输入。实写 candidate/spare、Completion、有效 EVENT、来源及未写范围逐字复核，错代/迟 fence/重复源/越界与错误事件顺序都保原失败。Developer 优先交前两项和两组独立开关；合入时一份默认启动器、manifest 和版本。完整 shader 输入 ≤49,152 B，保 GPU 驻留及 shader 渲染，禁止 Canvas 回退。

Effector 按 T941 的 10×(6 s 持按+4 s 空闲)+停 8 s+探针和 T935/T940 体验流程，120 Hz 生成需求情况下实测 accepted/overload/revoked、实际推进比、Q/SOURCE 延迟 p50/p95/p99/max、60 s 后实际入料、实际积床和 T940 几何。用实际 fence/WSI、窗口及像素/显示证据分开报告 30 fps，按固定有效窗和连续分窗列实际显示帧、重复帧及帧间尾部。保持现有目标的正确性及首堆留存。现阶段无新版集显达标结论。T945 的 G9 三态与 G13 MAC 继续按各自 typed 路径实测，本卡复用计量和原子接缝。 [长窗命令](../test/reports/t941/runs/goal6-longrun/argv.json) · [诊断命令](../test/reports/t941/runs/goal6-stage-timing/argv.json) · [T945 工程合同](architecture-T945-v1.md)
