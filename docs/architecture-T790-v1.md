# T790 · G13 探针与暖启动工件身份归因

2026-11-21 · 目标 20 · P5 · architect · 离线复核。

> T808 复核。T770 v7 的主 bundle 探针入口已达，实际 RESET 已发布 `CREATED`。现有 `CONTEXT` 事件暂存到下一次状态发布，CPU 活跃尚无法定位具体调用。先补同代逐调用进度和取消/退役证据，再执行原探针用例。 [T808 首次归因](architecture-T808-v1.md)

**T770 v3 启动了 G13 产品页，T786 启动了 RX。** 前者仅复现 G13 产品 OP65 的 `INIT_ERROR_27`。后者留下 RX BOOT 成功和缓存落盘原件。G13 隔离探针已有 T778 专属工件，G13 BOOT 暖启动仍沿 T760 产品工件取证。 [证据与完整摘要](architecture-T790-evidence-v1.json)

## 实际测到的工件

| 来源 | 包体、映射与入口 | 本轮可计证据 |
| --- | --- | --- |
| T770 v3 | T767 树 `t767-split-product`，未签 `1cf1bfbc…` → 已签 `a44752df…`。两包内均为 `G13ResearchAbility → pages/GpuResidentG13`，`libgrs_g13.so` SHA `dcac1489…`、BuildId `c9a8c0a5…`。设备 `l4.json` 的内存 BuildId、映射文件 SHA 与包内 ELF 相符。PID 47610、start tick 447774、owner TID 47740、attempt 237。 | 原布局记录 `BOOT_EFFECTIVE_TERMINAL:EXIT`，OP65 result 27，OP66 未获资格。程序收据为 `SOURCE_COMPILE`，shader `e1893369…`、key `bacbc911…`，binary SHA 空。该记录也说明相同 key 可以出现在不同签包或不同运行路由。 |
| T786 | PID 43232 的十条原事件是 `G9BootV1 / RX / BOOT`，配置标签 `G9RxConfigure`，映射 BuildId `4bdd76dd…` 与本地已签 T766/T763 RX 的 `libgrs_g9rx.so` 匹配。T786 归档未保存实际安装 HAP SHA。 | RX BOOT `BOOT_AUDIT_OK`，seal 6559303。归档 `.bin` 的 header、stem、长度和 payload SHA 对应 `G9_BOOT`、shader `8e0a89e6…`、key `760d1593…`、payload **37,965 B**、format 38769。该 key 的 JSONL 为源码构建、导出与 `WARMUP_OK`，未见本次进程的同 key `BINARY_LOAD`、link status 和反射链。`CACHE_PRELOAD_READY→APPLY_ENTER` 约 0.030 ms，只覆盖这两个配置时点。 |

T770 将同 bundle 的产品 HAP 当成诊断 HAP 交给了真机。T786 以同 bundle、安装时刻和缓存存在作为工件定位条件，实际选中了 RX 家族。两卡的原件各按其入口保存。修正 T770 归因后，FTZ 决策继续用于产品数值资格，隔离探针先沿已有专属入口执行。 [T770 产品原布局](../test/reports/t491/t770b/l4.json) · [T770 v3 签包回执](<D:/hmprojects/HelloWorld-T767/test/reports/t257/T770-v3.json>) · [T786 原配置链](../test/reports/t491/t786/warm-chain.txt) · [T786 BOOT 原件](../test/reports/t491/t786/warm-g9boot.jsonl)

## 两条正确执行链

### T770 · T778 同 bundle 独立探针

1. Effector 取 `.codex/local-test/t778-authorized-handoff` 的 T778 未签 HAP。完整 SHA 为 `fd24d289746215ef41d02a90157071e41774a4da3c7f99786263d638a7d91cd4`，包内 `libgrs_g13_visibility.so` SHA `25c5d8d9b216812a6cefc1e485941cf7302996f895fdc9ed131fb62d065ed527`、BuildId `e4bc6faae76828c3a350d1a3d88eb6482ffb8fe1`。沿 T257 正常签装，原 T778 交接命令为 `python test/t257/sign.py T778-g13 .codex/local-test/t778-authorized-handoff`。以本次签包原回执绑定签前、签后两个实际文件、授权 profile、证书链及包内 ELF。待本次签名生成后采实际 signed SHA，`a44752df…` 属于旧产品包。 [T778 编译回执](<D:/hmprojects/HelloWorld-T778/docs/receipt-T778-compile-v1.json>) · [T782 签包验证合同](<D:/hmprojects/HelloWorld-T778/docs/test-report-T782-v1.md>)
2. 按 T777 串行备份、覆盖和恢复。实际进入 `EntryAbility → G13VisibilityAbility → pages/G13Visibility`，确认 `G13_VISIBILITY_TRUSTED_MAIN` 和实际映射 DSO，采新 PID/start tick、attempt/nonce。使用页面固定 case 及「运行」「Save」，保 `filesDir/g13-visibility/attempt-*/report.json` 和原 trace。由 T782 reader 读实际未签/已签文件、签收据、product 负控及原侧车，再核 T769 的固定 ordinal、跨 lane、WORK、TAIL、PACK、非法入口和同代退役。T778 使用独立离屏 owner/context 和资源，生产 REST、产品缓存及首 Swap 各沿原证据链。 [原页面](<D:/hmprojects/HelloWorld-T778/.codex/local-test/t778-trusted/entry/src/main/ets/pages/G13Visibility.ets>) · [覆盖与恢复](architecture-T777-v1.md)

### G13 BOOT · T760/T756 同签产品包

1. 读取既有 `T760-split-signed.hap`，完整 SHA `868106e231b1b8362c3537c08bd812d75386ee45a2d4f98fa3292d112e027da9`。入口 `EntryAbility → G13ResearchAbility → pages/GpuResidentG13`，`libgrs_g13.so` SHA `369d5ee8f316adc3810cd8e9c6a3ff96ad8351bcee599d75ce77ec4784ba4d77`、BuildId `e5ec50ff304bc372189b8ad210a24f0b82e24771`。保同设备、后端、环境、配置和签包。可在 T770 覆盖窗口前做，或验证恢复后做。 [T760 签包原收据](../test/reports/t257/T760-split.json)
2. 先只读找 `bacbc9118da2e5c1e8c459602f8d78c0a05b20b95aca1afdf4a4f244435c8cb6.bin`。若有历史同轮实体，核完整文件 302,105 B、format 38769、payload 301,961 B、payload SHA `3368e1911fffcebfa8c95863406e3b0ffebee9801925475224fe5d96397b1b42` 和冷轮原收据。若缺实体或匹配失败，原 T760 签包补冷轮，归档新的实际落盘 payload，再用同签包保数据起新进程。目标是 G13 `G13_COMPUTE` 的 BOOT index 0、OP64/65/66、23,861 B 原 shader `e1893369…`。 [T760 冷轮原始 T475 记录](../test/reports/t491/t760/t756-compute-persisted.jsonl)
3. 暖轮只点一次产品页「配置 REST」，采本次 PID/start、owner、actual mapped ELF、attempt 和 context。用同 key/payload 的 `BINARY_LOAD_ENTER → LOAD_CHECKED → LOAD_LINK_OK_REFLECTION_PENDING → REFLECT_OK → REFLECT_PREPARE_OK` 原事件与本次 BOOT 程序对象、GL/EGL 返回一一联核，证明该 BOOT role 源码 compile/link 均为 0。保原 FP32 终态和 operator 原字。如果加载反射成立而 OP65 仍失败，可记 BOOT 暖窄证。完整角色、原数值资格及 XComponent+shader 首个有效 Swap 继续按原判据。 [T783 冷暖执行与原数值合同](architecture-T783-v1.md) · [T741 G20 口径](architecture-T741-v1.md)

## 归档规则与离线核对

每次触发前当轮声明预期工件和入口，归档实际签包、签前原件、签收据和包内 ELF。装后在同次运行证实实际页面和映射库，再联相同 PID/start 与原始事件。硬件、bundle 名、安装时刻、同 renderer、同 key、已持久化、预读 READY 和其它轮次的证据分别可以作线索。缺项按发生阶段留空，严禁补写预期值、复用旧 PID 或把产品/RX/fixture 的运行时结果填入探针/G13 暖栏。历史 T770/T786 原件保留，结果照本次身份归类。

离线重算 T770 签前后 HAP、实际映射 ELF 与 T760、T766、T778 包内身份。T782 复跑接受 `fd24d289…`，拒绝 `1cf1bfbc…`。逐字节核了 T786 RX 缓存 header、payload 和原 JSONL，确认上面两条分流。T778 本轮证据止于未签工件，签装、探针运行由原 T770 接续。两条修正均未新增 G20 实证计分项。 [机器复核](architecture-T790-evidence-v1.json)
