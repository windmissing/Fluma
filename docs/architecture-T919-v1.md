# T919 · DM2 / R1 split-v2 启动与同输入对照工程修订 v1

2026-11-24 · architect · PC Vulkan · 离线。split-v2 两族发布包均由旧 owner 角色表搭配新 SPIR-V，首个可静态定位的拒绝是 `0x380` 文件长度校验。DM2 的两个真阳性均只完成 SOURCE。连续臂没有提交 QUANTUM。R1 v1 的 GPU 根已降到约 224–233 ms，所测宿主前段仍使 250 ms 业务窗口不足。 [T914 原报告](../test/reports/t914/report.md) · [复算原件与 SHA](architecture-T919-evidence-v1.json)

## 1. split-v2 · 单包内的角色表与模块版本不一致

T914 两族各四臂只切 `plain / detail / prewarm / detail-prewarm`，8/8 退出 1、stdout 为 0 B，stderr 原文为 `DM2/R1 VULKAN_ROLE_PIPELINES VkResult=-3`。PE 的 SHA 与运行 manifest 一致。用 PE32+ image base 将两个指针换成 RVA，再按节表换算文件偏移，逐行解码 `kModules` 的 24 B 记录（`opcode u32, bytes u32, name VA u64, SHA VA u64`）。磁盘角色均与本包生成的 `sand_passes.h` 和 `roles.json` 一致。

| 族 | PE 原始偏移 / 一致前缀 | 首处分歧 | PE 预期 | 磁盘实物 | 紧随其后的不一致 |
|---|---|---|---|---|---|
| DM2 | `0x752b0` / 45 行 | row 45，`0x380` | 8,764 B，`fc5bc453…5ea9` | 9,084 B，`0c294ab3…8180` | `0x381, 0x382` |
| R1 | `0x7b5e0` / 49 行 | row 49，`0x380` | 9,576 B，`5ff5e484…61b5` | 9,896 B，`3379b03b…f864` | `0x381, 0x382, 0x390–0x393` |

`VulkanOwner::Open` 解析 Vulkan 入口、创建 descriptor set layout 和 pipeline layout，随后按序打开模块文件。到首个坏角色时，`read(entry.bytes)` 能取满旧长度，`peek()` 仍能读到字节，立即触发宿主自己的 `VK_ERROR_INITIALIZATION_FAILED`。该行既未计算 SHA，也未进入 `vkCreateShaderModule`。这是包内可确定复现的 `-3` 路径。旧原件只有聚合错误，无法排除同一进程更早的 Vulkan 调用另有故障，也无法把 `-3` 记为该角色的驱动返回。detail 和 prewarm 的生效点在坏文件检查之后。 [Open 调用顺序](../pc/vulkan/sand/owner.cpp#L262) · [T914 运行清单](../test/reports/t914/runs/manifest.json)

**构建证据。** 两族 `owner.cpp.obj` 和静态库都含旧 SHA，host 对象已含新的 `roleSetSHA`。旧 owner、静态库早于生成头，随后生产日志各只有 host 编译和 PE 链接两步。现有 CMake 通过 `execute_process` 在 configure 阶段生成，Ninja 查询 `sand_assets/sand_passes.h` 为 unknown target，两个 owner 对象 `ninja -t deps` 都是 `#deps 0 (VALID)`。已确认生成头没有进入 owner 的有效构建依赖。MSVC include 扫描为空的具体原因目前缺独立收据。

**设备与布局对照。** T914 同轮 baseline/v1 的成功 BOOT 锁定 UHD 630 `8086:3e98`、驱动原值 `1656924`（31.0.101.2140）、queue family 0。split-v2 原件无 BOOT，其运行 device/queue/UUID 和实际 VkCreateInfo 尚待采。按工程合同每族保持四 SSBO 加一 UBO，DM2 / R1 的 Common 240 / 304 B、push 20 / 36 B，K192 arena 12,767,232 / 11,718,656 B。两族各自控制布局。整个角色集的源码字节与 SPIR-V 字节分别登记，文件长度校验使用 SPIR-V 实际字节。 [当前构建定义](../pc/vulkan/sand/CMakeLists.txt) · [T913 工件](developer-T913-v1.md)

**Developer 修订与最小复测。**

1. 先以新 build 目录重建两族每个消费者。把生成器、适用 shader/include 与选定 variant 接入构建图，生成头和包清单有明确输出，owner/pass/host 等实际消费者对头有可验证的依赖。生成头变动后增量构建必须重编相关对象。发布在临时目录先核 PE 实际消费的 role table、生成头、role set 和磁盘模块全量 opcode/path/bytes/SHA，再组成同一不可变包并发布。运行时的长度/SHA/magic 和 `roleSetSHA` 校验继续生效。编译侧收据来自 owner 自己实际用的表，独立 checker 再和落盘包比对。
2. 每族先保相同设备及真实布局，最小三臂为旧 PE+新 assets 的原坏包、清洁重建的全新配套包、复制新包后仅改 `0x380` 文件长度的负控。另用等长改 1 B 的负控验证 SHA 分支。记录调用阶段、opcode/path、期望及实测长度/SHA、模块与管线原 VkResult、flags/cache、layout 指纹、物理设备/driver/queue、完整 argv/CWD/包 SHA。正臂须走完全部角色资格和 BOOT，负控须准确报告本地拒绝阶段且无该角色 Vulkan module 调用。
3. 只有一致包仍失败时，固定 layouts、绑定、push、驱动和同一调用顺序，先测原角色前缀，再只加新 `0x380`。之后按发布顺序逐个加入其余新角色缩小最小失败前缀。新进程独测首个失败角色及“原前缀+角色”，显式记录 Vulkan cache 参数，另记进程冷启动和驱动实际缓存隔离证据。最小角色臂没有完整资格、提交和发布权限。单角色、完整前缀分别报读文件、`vkCreateShaderModule`、`vkCreateComputePipelines` 的返回值与耗时。

## 2. ModelEnvelope · 实际资格与诊断比较

T914 每臂同用 512 B fixture（`3580ccb1…2f55a`）。其合成业务体是 kind 1、seq 1、sourceId 2、请求质量 0.2、位置 (64,200)、初速零。该 SHA 只标记 fixture。两族 InputGate 会填 run/owner/context（字 4–9）、input epoch（12–13）、sample（14–15）、pose stamp/domain（16–17、22–23）、角度、配置 revision 和参数 hash。非零命令还写 **43–44** 的 command stamp。Offer 校验真实时效后填 receipt（18–19）。Freeze 再校验 pose/command，并填 nonce（10–11）及 freeze 时刻（20–21）。T914 host 在该步骤取同一个真实 `HostDevice::Now()`，上传并哈希的是封装完成的 128 字。DM2 两份真接受日志的实际 input SHA 已不同。

时间字进入了可见契约。两族的命令相等性覆盖 40–127 和所列身份/参数字。DM2 的 `g9ConsumeCommand` 持久化 128 字 capsule，`g9SourceBirth` 的来源记录含真实命令段 40–127 的四个 FNV32。R1 的 seed scope、父/视觉 Expected、permit、BASE/lease 等也绑定本根身份。raw diff 的范围因此跨越 INPUT、状态字段和派生标识。 [DM2 InputGate](../entry/src/main/cpp/gpu_resident/g9dm/input_gate.h) · [R1 父 InputGate](../entry/src/main/cpp/gpu_resident/g9spf/input_gate.h) · [DM2 命令消费](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp#L1394) · [宿主冻结](../pc/vulkan/sand/host.cpp#L502)

1. **真实业务证据。** 每族各臂用同 fixture、初始 BASE/参数/角度、有效范围及真实运行时钟。保真实 grant、freshness、nonce、Expected224、实际 inputSHA、250 ms 截止及 1,024 B completion readback 门。独立封存实际 upload 的完整 INPUT128、permit、Expected/Completion、BASE、ACTUAL/REPLAY、已用 EVENT/PATH 和各自范围/代，R1 另附 seed、map/marker 和 WORK。业务接受后冷端取档。DM2 可扩展现有 archive 冷端收据，R1 现有 witness 要求续根，单 SOURCE 应补独立冷端采集。迟退役的原件在独立诊断命名空间采，并绑定同 fence 和代。缺一侧原字时把对应比较记为证据不足。
2. **跨进程语义差分。** 先在每臂原字上验证 raw SHA、capsule、派生 digest、typed 证书、时钟域、真实年龄与前后顺序，再由 tester 用每族有版本的字典映射对应 run/owner/context/epoch/plan/nonce 和 lease。时标保留原值并核各 grant 的相对顺序和约束。输出差分只在已证明语义为传输盖章的白名单位置映射对应标识和时间。对本合成 kind 1 的实际命令，先从实际原 capsule 核验 43–44 在内的原生来源 digest，再在规范化视图重算派生项。配置、seq、sourceId、物理载荷、范围、保留字和未获分类的字逐字比较。已有去重链先按各自原 40–127 及身份校验，包含同 seq 真重复、同 seq 但原内容不同的拒绝对照。派发按逻辑工作和真实 ACTUAL/REPLAY 边界对应，保留各版物理派发数。全量原字与语义差分分别存档。
3. **严格同字冻结重放。** 若仍需相同输入逐字可复现，可在私有设备诊断入口使用同一冻结 INPUT128 和相匹配的 BASE/seed/scope/permit/Expected。测试专用可控时钟覆盖验证该镜像所需的整条时钟域，另外记录真实提交、GPU 与 fence 时钟和 watchdog。入口无业务 CURRENT 推进、`PulseClock::Accept`、acceptedRoot 或展示资格，结果仅可用于同字算法与工程对照。生产继续取真实时钟和原业务门。

**Tester 补证。** T914 现有 checker 候选 roleSetSHA 采用 BOOT 自报和 timeline 互比，须从外部冻结包独立核对。split-v2 的调度名与查询粒度要独立校验，plain/prewarm 为粗粒度 6/7 点位，detail 为 31/57，实测调度应是 14/27 个物理派发。对截止用实际业务资格/接受时点判定，`stopNs` 是单独边沿，确认细化计时后用 `promoteExitNs` 或单独打 accepts 采样点作上界，并明确其与 gate 决策顺序。至少增加错 roleSet、晚接受及同 seq 载荷冲突的拒绝对照。旧 T914 缺跨臂完整冻结 INPUT/真实镜像与有效同字差分。 [T914 checker](../test/t914/check_t914.py)

## 3. 各族下一步

GPU 时长从同 device/queue/fence/代的可用 query 首尾复算，`((end−begin)&(2^36−1))×83.333 ns`，覆盖 upload 至 tail copy。宿主时长单独以 HostDevice 时钟计算。两套时钟与排队可能重叠，差额用来定位预算问题，实际接受以业务时标为准。

| 原臂 | 宿主录制 / commandReset，ms | submit 返回时余量，ms | GPU 根 / dispatch 边界合计，ms | 实际业务结论 |
|---|---:|---:|---:|---|
| DM2 baseline detail+prewarm | 42.471 / 42.289 | 207.446 | 201.715 / 201.623 | SOURCE 接受，promoteExit 244.540 ms |
| DM2 v1 detail+prewarm | 49.791 / 49.622 | 200.152 | 171.476 / 171.372 | SOURCE 接受，promoteExit 221.635 ms |
| DM2 v1 首次 chain | 138.233 / 137.900 | 111.701 | 234.603 / 234.479 | SOURCE 截止，QUANTUM 未提交 |
| DM2 v1 chain-r2 | 42.604 / 42.437 | 207.332 | 522.179 / 522.085 | SOURCE 截止，随后同 fence drain |
| R1 baseline detail | 50.745 / 44.501 | 199.168 | 426.335 / 426.240 | SOURCE 截止 |
| R1 v1 detail | 113.260 / 113.017 | 136.674 | 233.091 / 232.992 | SOURCE 截止 |
| R1 v1 detail+prewarm | 85.055 / 84.865 | 164.866 | 224.031 / 223.933 | SOURCE 截止 |

### DM2 · 先证明同一次运行提交并接受续根

两个接受臂均 `commit=1 / acceptedQuanta=0 / quotaBalanced=true`，promoteExit 距截止分别 5.460、28.365 ms。单次接受说明合成 SOURCE 的真实 typed 门已走通。五份 DM2 chain 原件和一份 R1 baseline-chain 都只有 nonce=1 的 SOURCE。它们在进入下一轮循环之前退出，现有失败证据定位在首根。DM2 的其他三个 chain SOURCE GPU 根为 393.291、317.207、416.929 ms。r2 的独立退役先返回 `VK_TIMEOUT=2`，后 `DEVICE_DRAIN` 实际等待和同 fence 确认返回 0，仍无业务接受。

用已可运行的 v1 做首组 A/B。相同初始状态、同设备/包、同诊断选项，交错运行 SOURCE-only 与一根 SOURCE 加 `--steps 1`，首组关 FULL、witness 和 WSI，每臂至少三次，记录原顺序与全量时标。SOURCE 接受后按 `activeRemaining` 实际信用判定是否复用 plan 或领取合法 fresh grant，记录 plan/根 nonce、根初始及剩余信用、是否已退役、真实 deadline、同 fence 和代。首个 Q 确认两根均有独立业务 receipt，SOURCE 后 commit=1/A=0，QUANTUM 后 commit=2/A=1，SOURCE 的 CURRENT/lease 成为 Q 的 BASE，quota 平衡。若 SOURCE 仍被拒，先按其录制与 GPU 长尾立案。若 SOURCE 接受但 Q 缺席，先区分 grant/credit 入口和实际已 submit 的拒绝。Q 实测通过后单独引入 FULL / witness 分支，量其额外复制和读回。DM2 和 R1 的真实 Q 各有 2,404 / 2,543 个派发，SOURCE 数据不能外推其耗时。 [宿主 plan 和接受链](../pc/vulkan/sand/host.cpp#L174) · [T914 原始运行](../test/reports/t914/runs/)

### R1 · 分开定位宿主首根和设备剩余热点

R1 v1 仍分别缺约 96.417 ms（detail）和 59.165 ms（detail+prewarm），尚需给 fence、readback 和 gate 留余量。即便 GPU 稳定在 233.091 ms，宿主、排队和接受总共只剩 16.909 ms。`commandResetNs` 汇总 `vkResetCommandPool`、`vkResetFences` 和 `vkBeginCommandBuffer`，现据不足以归到其中一个调用。无 detail 的两个臂录制为 54.712 / 58.267 ms，没有可用的 GPU query tick。当前矩阵按顺序运行且每组合只有一次，无法独立量出 prewarm 收益或 query 开销。

Developer 先给三个 Vulkan 调用分别打 ENTER/EXIT 与 VkResult。做同一设备、包、fixture 和查询开关的交错对照，一组保留当前路径，一组在合法 retire 后提前准备与根无关的 pool/command buffer/fence 代。真正的冻结输入绑定、录制、提交、原 deadline、同 fence 退役与当前根身份继续关联。待 R1 的 27 派发一致包可进入 BOOT，再用同一宿主条件分离 v1/v2 的 GPU writer、独立 replay、审计/尾读热点。先取一个有真实 DEVICE seed、typed accept 与 visual lease 的 SOURCE 阳性，随后验证 Q 和 WORK/WSI。产品视觉经 shader，保留两份独立 ACTUAL/REPLAY。 [Vulkan 录制入口](../pc/vulkan/sand/owner.cpp#L401) · [T911 writer/replay 预算](architecture-T911-v1.md)

## 4. 分流与未取得的证据

现有 T914 checker 在 22 份非空原件上重跑均返回 0。该检查覆盖它已有的证据结构及拒绝边界，两份业务接受见上表。8 个空 stdout 臂按原 manifest 与 PE/模块核对单独复现。

- **工程 → developer。** 已确认构建依赖/混包、真实输入封存接口及宿主录制边界。先修包与独立量测，可同时准备不受 split-v2 启动影响的 v1 续根对照。
- **验证 → tester / effector。** tester 补离线包核对、冻结字典和拒绝负控。effector 在集显 PC 执行新版一致包 BOOT 和 DM2 / R1 的业务对照，附原始收据。现存件可复现坏包与原始超期，尚缺一致包的 API 级正臂、两侧完整冻结原字、实际 DM2 QUANTUM 和 R1 typed SOURCE。这里已解码 PE、复算原点和核源码顺序。进一步确认需要 developer 的配套实物和后续 PC 采样。
- **算法回流条件。** 若 input/BASE 正确配对后 ACTUAL/REPLAY 有语义残差，先定位字及首个逻辑角色，再带守恒/去重/视觉反例交 designer。当前两份真实 DM2 SOURCE 与未接受 R1 的计时只支持包、录制与预算结论。
