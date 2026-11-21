# T774 · 目标 9 产品、研究工件和离线证据映射

2026-11-21 · G9 · P3 · architect · 离线交付。

**目标 9 目前只做离线验证和修复。** T779 可直接用独立 oracle 做数值与协议差分，T780 可对实际包内字节和生成 GLSL 做回归，T781 可修复现有合同覆盖且在当前工件可复现的缺陷。三卡均不依赖 T541、T569、T645 的真机结果。T569/T645 以下保留的是未来恢复后的实际前置。 [当前排期](后续工作清单.md)

## 五条工件路线

| 路线、入口和 profile | 能复用的证据 | 能力与证据边界 |
| --- | --- | --- |
| **默认 DM 正式物理** · T648 实现 `G9_SPF_DM_Q20_V2`（GDM2、`libgrs_dm_t647.so`），独立 Config64/R0 S40、合格 seed、默认合成输入身份。真实 owner/Controller/Transport 才能驱动正式事务。SOURCE 12/0xffff、QUANTUM 2404/0x3ffff，后者新增动员和中点证书 16/17。 | T554/T647 机器字典、T648 开发编译收据、T556 公式和反例、T569 静态十项。T718 是当前已知 readonly 修订工件，核其未签 HAP `19e75d78…`、签包 `cb636553…`、库 BuildId `9954e008…` 与 T715 旧版本。生成和包体可直接离线复核。 | T648 构建容器说明没有注册新的产品 UI。T541 的旧 DM HAP 经 EntryAbility 启动缺 `grsm0/grsm1`，应归应用壳构建域。只有匹配默认 DM 的实际完整计算才可证明 GDM2 算法，页面启动、已链接库和 host gate 只支持各自域。 |
| **DM 独立 NDK 差分** · T670 `g9_dm_differential` 加实际匹配的 DSO 和 DT_NEEDED。旧 driver `ee6dead0…`，T718 driver `cb94c860…`。用 Config64 + settings、identity/seed/commands/quota/cold-credit，实际 EGL pbuffer。 | T685 已将此入口与旧 HAP 分离；T569 历史真机 Create=8，slot@1728。T712 后续曾见 coherent@1063，T715 readonly@1987；T718 记录仍采样在编译窗口，没有成功 CREATE，且报告存在三进程并发、缺实际映射身份。T670 fixture 及 DM 冷流 reader 可离线复用。 | 准确运行时身份用 driver 和真正映射库的独立 SHA/BuildId，不比较未剥离库与打包库。通过真实 COMPUTE 才能证根与冷流；pbuffer 证据无 XComponent 场景或 Swap。长调用入点可证，不能把并发观测推成单路耗时或判修复已过。 |
| **RX 完整位移拨动** · T641/T644 的 `G9_DM_NATIVE_RECEIPT_SPATIAL_R1`（GNR1），默认 OFF 的独立 Ability/页面、NAPI、`libgrs_g9rx.so`。保真实 native receipt、XComponent scope、GeometryKey/FrameWitness、RAW64、Dref 125 ms；SOURCE 12/QUANTUM 2404。 | T645 旧包映射已证，slot@1728 拒；T713v2 确认 1097 coherent 修订后见 readonly@1987。T716 qualifier 修复与 T720 实际 RX 单程序工件可做 T780 静态回归，T720 原回执 HAP `c24d08cc…`、BuildId `27c55879…`、计入的 GLSL SHA `2cb9a848…`。T720 文档引用的前一轮设备入点对应旧签包 T719 / `0f591129…`，分开登记。 | RX 验证只覆盖接收时间的资格与位移模型，合法不同耗时同位移必须同 J/cap。不能证明硬件采样年龄、真实手速、旧 REAL_TOUCH_R2 或厂商时钟资格。历史 LOAD/COMPILE 入点不能证本新包 READY/首帧/真实拨动。测试 seam 单列，不能伪作真人触摸。 |
| **RX / DM BOOT+SOURCE 研究臂** · T752/T763 RX 真页面，T748/T758 DM 独立 NDK 研究驱动；分别保本族 GNR1/GDM2 物理、实际 roleSet 和独立 BOOT/SOURCE 闭包。只允许 K192/S40 的单次非零 SOURCE、零初始 pending；零时间推进。 | RX T755/T766 有 BOOT 原 fence/尾及 SOURCE 编译、链接、APPLIED，随后 Pump STALE/DEADLINE；DM T751/T762 有 BOOT 原尾、SOURCE 实发 12 步，后续运行到执行 fence，期限到时仍未就绪。T759/T768/T772 已冻结各自身份和失败边。可复用编译/绑定/入队/计时/事务失败判据及 BOOT 原尾。 | SOURCE 的正判须有同根 WORK 1,024 B、typed、209715 Q20 和实际 commit/FIFO；目前缺。即使成立也只证明 SOURCE_BIRTH 及零时事务。该变体阻止 QUANTUM，不能交 DM_NATURAL/midpoint、完整运动、RX 拨动或 FULL trial。 |
| **SPF 冻结基线** · T524 `G9_SPF_Q20_V1`；T670 恢复 T564 冻结，`g9_differential` PID/映射依赖 `libgrsm1.so`，Config16，12/2024。历史签包 `f1994d31…`，不是 T597-fixed。 | T520 三态参考、独立 SPF ELF `d5283056…`、T670 fixture 和原 cold 字典。T541 的旧 SPF 页面停在 WAIT_RETIRED，经 T685 分清旧 scope 的 program 开放和新 epoch 等待。 | 旧页面状态可做生命周期对照；T541 v2 未实际跑 T670 独立入口。SPF 无 GDM2 动员证书及 GNR1 输入语义。保跨族共同质量/来源/对比 invariant，不能用另一族 raw profile、快照、事件字段或旧设备 READY 推本工件通过。 |

出处：[T524 SPF 工程](architecture-T524-g9-spf-engineering-v1.md) · [T554 DM 工程](architecture-T554-g9-dynamic-mobilization-v2.md) · [T648 开发](developer-T648-v1.md) · [T641 RX 工程](architecture-T641-v1.md) · [T644 RX 入口](developer-T644-v1.md) · [T685 启动归因](architecture-T685-v1.md) · [T670 差分交接](test-report-T670-offline-handoff-v1.md) · [T718 回执](../test/reports/t718/compile.json) · [T718 设备转录](../test/reports/t491/t712b/v4-t718.json) · [T720 工件](../test/reports/t720/compile.json) · [T720 设备源归因](developer-T720-v1.md) · [T736 研究边界](architecture-T736-v1.md)

本轮比对了实际 HAP 和 ELF。**T720 原未签路径当前内容已漂移**。该路径现 SHA `0b55dbc6cd792aa4429ac2f8fcc513dd7626881ffa9d62514e35675ca7491e97`、BuildId `b6be7bd668334a5d0aa5a55877bc4578a85f1d5c`，与原编译回执的 `c24d08cc…/27c55879…` 不符。原签包 `3c6af26b…` 内 ELF SHA `4b08d763…`、BuildId `27c55879…` 与 T720 回执一致，可作历史锚点。两包提取的主 GLSL 均为 146,301 B、`2cb9a848…`，能复用同 shader 的静态期望，不能共享 native 行为或实际部署身份。T763 回执中的单程序默认 RX 未签对照 `acef1bc6…/0db444d0…` 已核对实际 HAP/BuildId，主 GLSL 同 SHA，可作独立当前候选。T780 以实物分别报这三项，历史测试只挂对应原签包。本轮 T718 DM 已核其未签/签包、打包库和独立 driver，T670 SPF/DM 历史签包也逐字联核。 [T720 授权签包](../test/reports/t257/T720-rx.json) · [T763 完整与研究变体回执](../test/reports/t763/compile.json)

## ROOT/FULL、物理与显示证据

| 冷档族 | K192 ROOT / FULL | K512 ROOT / FULL |
| --- | ---: | ---: |
| SPF `G9_SPF_COLD_V1` | 24,653,824 / 94,965,760 B | 45,953,024 / 179,671,040 B |
| DM `G9_SPF_DM_COLD_V2` | 26,750,976 / 103,354,368 B | 48,050,176 / 188,059,648 B |
| RX `G9_DM_RX_COLD_V1` | 26,750,976 / 103,354,368 B | 48,050,176 / 188,059,648 B |

ROOT 支持 prefix、BASE/FINAL 和 SELECTED/权威 EVENT+PATH；FULL 在此基础增加六 trial 实际分段，逐段核 offset、状态和 SHA。ROOT 缺 FULL 的失败 trial，未运行 trial 保具名状态。RX/DM 同字节预算不代表相同 profile，RX 另核 CURRENT/BASE witness、journal、输入和 GeometryKey，按本族 reader 验证。FULL−8 MiB 是拒绝负控，不能用短档冒完整轨迹。冷档验证预算、段身份、实测物理和真机能力各自留原件。 [SPF 机器表](architecture-T524-g9-spf-manifest-v1.json) · [DM 机器表](architecture-T554-g9-dm-manifest-v2.json) · [RX 冷档表](architecture-T641-storage-cold-v1.json)

- **物理** 用本族 fixture、同一冻结 BASE/input/permit 的真实 GPU Completion、事件、路径与 FINAL。独立核 S/F/P 转换写入、局部/全局质量和动量、来源前缀、能量、容量、真实接受时间、重放、commit 和拒绝无污染。DM QUANTUM 要有实际事件15/16和 2404 pass 对应的审核证书，SOURCE 的无事件15 属预期。host 结果可判公式、工具及协议行为，不能制造 GPU 成功凭据。
- **呈现** 单独核现场 `SCENE_ATTR` 和 `PRESENT`，同 accepted generation、GeometryKey/FrameWitness、实际 XComponent+shader Swap 返回与非空帧。独立 NDK/pbuffer、场景静态链接、黑屏或旧 STALE 帧均无新帧信用。
- **驱动与工程** 把包/Ability/依赖、GLSL 生成、驱动 compile/link/Create、协议资格、物理验收和呈现的首个失败边分列。编译开放可说明已观察阶段，不能当算法数值失败。实际 GPU 失败先回 architect；当前静态复现的生成器问题已有 T674/T716/T718 合同就交 T781。

## 真机恢复后精确前置

- **T569** 等用户恢复目标 9 真机，再以 T712 或同等实际原件出具同一 **GDM2 DM20** 可执行链。必须有同一 driver、实际映射 DSO 和本族 fixtures 的 SHA/BuildId、PID/start/renderer、CREATE 与真实 COMPUTE 资格成功，以及该臂进入真实完整 QUANTUM 的证据；旧错误逐行保留，新的首失败重新归因。通过此设施门可执行 DM20 强动员、DM22 foreign-policy/replay 拒绝、DM29 后 CFL 与两根接续、DM31 FULL 原信用和短预算拒绝。DM20 最终数值和提交是该卡验收内容，可直接消费真实对照，无需等待 T541 形式关闭。 [T569 历史真机](test-report-T569-v2.md) · [T712 报告](test-report-T712-v2.md)
- **T645** 等用户恢复，再保 T713 续测或同等原件。真实签装 **完整 GNR1**，映射 `libgrs_g9rx.so` 与包、实际生成 GLSL、role/config/seed 同一身份，原驱动返回明确、Create/READY、真实 XComponent 的首个同 token Swap 和有效窗口。达此前置后才跑真人 RAW64、接收资格与几何证据、N01–N11、物理/重放及 ROOT/FULL 读回。BOOT+SOURCE、旧 REAL_TOUCH_R2 证据和软件渲染均不放行。两卡前置以实际原件判定，未实施的字段如实未观测。 [T645 历史真机](test-report-T645-v1.md) · [T713 报告](test-report-T713-v2.md) · [T642 判据](test-report-T642-v1.md)

## 现在可并行交付

1. **T779 逐层差分。** 先复用 T520 三态独立值和坏例，覆盖 P→F/S、F→P/S、S→F/P 的实际适用触发、零输入稳定床、持续/停止动员、来源记录与守恒。M0/M1/V5 的发表值只作历史，不代本族前态。对 DM 跑 T556 的最大可行 q/q+1、top-first 连续前缀、局部 WAIT、动量不变与 pEntry→pMid→actual 一次力，保 DM17 旧序反例；用 T670 生成 ROOT/FULL、K192/K512、0/15/30°、单/多根和假档负控。现有 `test/t670/check.py --selftest` 可先跑解析和 q 篡改拒绝，实际 GPU 差分只消费已有同域真冷流，缺真实事件标明。对 RX 复用 T642 端点同位移、20/200 ms 等 J/cap、时限/舍入/错误位型，独立对比 actual host seam 可达的 FIFO/过期/去重及冷 reader；其余 N01–N11 保 GPU/设备待测。交独立期望、真实调用目标、通过的域、输入版本与反例。
2. **T780 二进制和静态 GLSL。** 复用 T670/T687 区分生成源码、嵌入 GLSL、未剥离和 HAP payload。对 T718 默认 DM、T720 完整 RX 及当前构建分别记录 ABI、入口、DT_NEEDED、SHA/BuildId；仅对绑实工件运行生成器/静态编译专项。固定 slot 声明、coherent/readonly 实参和原 line map；保修订前后可解释的逐字/生成差异。目标 20 的 48 KiB 规则约束新增或重构 shader，不把历史单体标成已拆分。
3. **T781 确认缺陷后修。** 复用 T674/T716/T718 的既有合同和隔离生成产物。只改当前仍复现的工程缺陷，已修的分支交可重建的编译和离线自测证据。T779 与 T780 可以并行启动，T781 可同步按既有合同复现，出现新增数学或用例分歧回 architect 分流。计算仍 GPU 驻留，不增加 CPU 产品仿真；上线仅 XComponent+shader，禁止 Canvas 回退。

离线公式和静态检查可尽早判错并推动修改。后续 GPU 性能、实际呈现和真人效果单列，沿用户恢复决策和真实前置开展。 [T520 oracle](test-report-T520-three-state-oracle-v1.md) · [T556 oracle](test-report-T556-dm2-oracle-v1.md) · [T569 静态审计](test-report-T569-static-consistency-v1.md) · [T670 校验](../test/t670/check.py)
