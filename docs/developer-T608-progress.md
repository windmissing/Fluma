# T608 developer 交付：独立 G13 GPU 管线

2026-11-18。本卡实现与编译交付；未运行测试、reader、物理 oracle、shader、真机、签名或部署。设备资格、数值精度全域、F01–F16、F05 物理压力、耗时、250ms、30fps 与显示呈现均未测，不据编译或求解器残差宣称通过。

消费 T607 `422eeae` 的工程主稿/engineering/layout/dispatch/math 四表、T605 算法及 T613 复用边界。目标 1–11、10 暂停及水线 13→14→12 不变。CPU 不增加流体 stepper；旧默认构建、旧 profile、旧页面路由不改。T596 未完成的 FULL 不作为本卡前置。

## 实现落点

- `entry/src/main/cpp/gpu_resident/g13flow/`：独立 `grs_g13_core` / `libgrs_g13.so`；四 SSBO、三 state bank、双 trace/pack。表驱动固定 820 QUANTUM、16 SOURCE，SOURCE+4Q=3296；单组 64 lanes，persistent 全组屏障。
- 真实 MAC 守恒迎风/CFL、midpoint Euler/非势力、八匹配 Coriolis、隐式黏性三环缓冲 Chebyshev（8×16、真残差与冻结 NOOP）、整数 Q32 多层压力 MG/兼容性/规范/证书/投影、真实 FP32 散度与投影身份。
- integer-limb S128/U256、Q40 RHS→Q32 RNE、整数压力差后一次 RNE 转 FP32；软件 binary128、512-bit range reduction、sin/cos/exp 系数及 SI 单位精确量化审计。无 shader double 或 CPU 场演化。
- 逐算子能量/动量、实际壁面离散冲量与壁功、120 个成功 Q 窗口/正功不退款、无功/零态门；不由动量闭合残差回填墙力。壁面离散诊断不冒已定标的任意沙面载荷。
- ENTRY/PROJECTED 时间插值 RK2 tracer，周期模式才 wrap，闭域越界拒绝，无 clamp；SOURCE 零时间、失败不提交时间/年龄。实际/重放全场及 trace 原件比较，首错保留，重放不等即 FAULT，Native 接受前不发布私有候选。

## owner、API、生命周期

`controller.h` 导出 Configure、Offer、BeginRoot、BeginGranted、PumpBounded、Poll、NativePulse、AcquireScene/ReleaseScene、PinCold/CaptureSpan、Rebind、Release/ContextRetired。编译所得符号列在 `test/reports/t608/cpp-entry-symbols.txt`。

C++ 调用者使用真实当前 EGL context，`Seed.nativeControl=true` 进入 COMPUTE_ONLY_NATIVE_CONTROL，并自行提供真实 NativePulse/permission；不能伪造 UI pulse。`Offer(input, now)` 的 now 为 Native 单调时钟。手工 BeginRoot 与自动路径共用真实配额快照和资格门。Poll 提供尾原件，是否发布须看 Native root receipt 与 FieldToken，不能只读 GPU publicationEligible。

生产 NAPI 的十二属性与 d.ts 对齐。输入同步定长复制，核类型/N/offset/backing，自零 SHA、完整 scope、reserved、支持域；保原包及盖 sampleNativeNs/enqueueSerial 的规范包。回复具名，带 invocation 的 boot/diagGeneration/attemptId/阶段/原始 NAPI 状态；错误输入带有效性与原字快照。owner 固定命令/回复槽串行，GL/map/hash/IO 在 UI mutex 外。

每 Pump ≤10 顶层提交或 8ms CPU 提交墙钟；每根绝对 250ms，不续期限；尾一个 fence / 1024B map。Stop/生命周期撤销在入口生效，逐次提交、persistent 返回后及原子接受前再查资格。33,333,333ns GAP 不放宽，最多四额度，不追补断档。GPU 完成时间未测时尾保持 0，真实 Native Poll 时间另存。

同 context resize 撤销、drain、VIEW_REBIND；旧 BASE token 保留，新根写新私有 geometry，随后新接受 token 才呈现。native window 持引用，context 销毁失败保留真实待退休资源，不逻辑取消即复用。资源账沿现有 ledger 和 shared broker；账为逻辑申请/实际 host 容量，非 driver RSS。

## 新页面与 scene

`GpuResidentG13.ets` / `G13ResearchAbility.ets` 为隔离研究入口，默认 OFF。显式配置 REST 后激活，按实际 scope 发 pulse，页面/Ability 隐藏停止；提供重力、RAMP、力时段及五种固定范围视图。

新 XComponent + shader，从实际已接受场和同代 pack 以六组整数 instanced attributes 读取，不依赖 vertex SSBO。FullFrameTargets 完整帧、固定色标/图例、真实姿态/盒尺寸、≤6 draws，禁止 Canvas/软件 GL 回退。Swap 成功后封 PresentedMapping；sceneToken 为去除 successfulSwapFrame 字段的具名 mapping UTF8 原字 SHA，重复完整 token 不加入变化的计数。成功 Swap 与实际 display present 分域；失败记录 stale 上一有效新管线帧。

## 冷归档与 reader

route13 复用唯一 `libgrs_cold_slots.so`，无 `grs_foundation` 链接（ELF 原件见 `elf-dynamic.txt`）。先预约预算/槽并暂停新 claim，当前根仍可结束；root-idle + 场景 fence 退休后 pin 原 bank/WORK/CTRL/pack，冻结复制后放 GPU pin，Native raw 留给有界 reader。双槽/单 builder/reader/TTL60s、不续租，实际释放 raw 后才退信用。

ROOT/FULL 原段、65536B prefix、自零 canonicalPrefixSHA、原 prefix/段/页/文件 SHA；实际根 permit、原/规范输入、seed/boundary/motion/config、数学原始 probe、配额、能力/编译/scene/Native root receipt。公开块使用现有 TaggedReceiptHeader16；超容量明确 RESOURCE_LIMIT，不截断成功，不拿 hash 代替字段原件。

失败、取消、旧尾或未进入阶段保为 FORENSIC，不冒 CURRENT。prefix 身份来自冻结 permit，不能拼新 scope 与旧根；描述符保存实际 bank/commit；BASE token 字节与 permit SHA 绑定。map/unmap、GL、IO、期限/容量分阶段保原错。

`GpuG13Cold` 只把 token/目录发 worker；`G13ColdWorker` / `G13ColdArchiveCore` 按 ≤1024 U32 / UTF8≤16384 一页流写、增量摘要、fsync、rename、外部文件 SHA 收据，不聚合完整场。取消/失败保存 INCOMPLETE 标记；reader 默认要求外部文件收据，不仅认末 manifest。构建准备器显式注册新 worker。

`tools/g13/read_archive.py` 为有界临时文件 reader，核页序/长度/目录/profile/SHA/保留零/身份/原包规范化/permits/收据及真实尾，输出结构完成或 FORENSIC，physicsPass 始终 false。本轮只语法编译，未执行 reader。

## 构建与交付身份

`node tools/g13/prepare_build.cjs .codex/local-test/<fresh-name>`：只创建隔离 unsigned 构建，注册独立 ability/page/type/worker，最小 native CMake，不改旧默认。生成器校验正式输入 raw 指纹、字段覆盖、opcode/行数/范围和常数位型，规范 LF 后组装 shader 并记录逐依赖 SHA。CMake 缺源/旧输入/生成失败即失败，不读旧 .cxx 源头。

最终产物与全部 51 个实现文件 SHA 在 `test/reports/t608/development.json`：

- 产品：native/ArkTS/注册 worker/unsigned HAP 全通过，最终构建 13.716s；HAP `395d8ea7e7207323bccb22da734a0b543582ecc6ecd5fc377206ce804eb2d3d7`。
- 独立 GRS_G13_TEST_HOOKS 构建：同样编译/打包通过；与产品 shader SHA / HAP 身份不同，产品默认 false。仅 mode3 的显式诊断可使用 pressure disabled/sign、RHS incompatible、gauge offset、normal wall 五类负控，始终不发布 CURRENT；本轮未执行负控。
- Khronos glslang 16.6.0 离线编译生产/诊断 compute 和 vert/frag；Python py_compile、Node --check。均无 shader/物理执行。
- 两个共享 cache header 新增独立 G13 分支；旧 V5 工程重新 native/ArkTS/HAP 编译成功 18.241s，未跑旧线测试。

原卡数学口径问题评论 `6afdaa64e4b004b3f877fcf4` 已保留：机器错误枚举没有 NUMERIC_DOMAIN 独立码。实现保持原精度门，使用已有 NUMERIC_OVERFLOW=17，并在实际 mathStatus / cold diagnostic 具名 NUMERIC_DOMAIN；不增公共码，不静默 underflow、不放宽 exp 相对误差。若 architect 后续明确另一码，仅按修订合同调整，不把本轮编译冒精度通过。

当前为 developer 实现/编译完成。后续 tester/effector 的设备资格、独立数值门、真实压力/能量/成本与观感执行沿原卡接力，不作为开发关闭前置。本会话未修改其它任务状态或滴答同名清单。
