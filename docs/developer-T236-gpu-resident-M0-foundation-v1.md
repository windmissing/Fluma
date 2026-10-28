# T236：GPU 驻留 M0 基础切片与工程接点回执

2026-10-28，developer。原卡 `6ae1ea8be4b0c3103c13e7d0`。**PARTIAL / M0_PRODUCT_NOT_IMPLEMENTED / NEEDS_ARCHITECT_CONTRACT**。不使用真机；未改生产默认、旧 pair wire、T211 CPU 路由、现有 renderer/NAPI 页面。仅本卡新增模块与根 CMake 接点。不能将本报告基础自测通过登记为 M0 GPU 产品通过。

## 一、已提交的可复用切片

目录 `entry/src/main/cpp/gpu_resident/`：

1. `layout_generated.h`：从原 T233 manifest 自动生成全部 SoA/AoS 字段 index、wordCount、常量与 manifest SHA，不手改字段、不占用 reserved。配套重生成 `test/gpu_resident/generate_t236_contract.py --check`。
2. `integer.{h,cpp}` / `integer_primitives.glsl`：显式 carry/borrow U64、32×32→64、64×64→128、U128 比较与除 U32、signed mulDivRNE、signed floor/ceil、binary64 raw bits floor-Q20、软件 RNE F32 pack/原列投影、checked bytes/align。C++ 只作基础原语/开发差分，不是 CPU 整步后端；GLSL 为独立库，尚未运行。
3. `angles_q20.bin` / `angles_generated.h` / certificate：不调用 designer 模型；以 Decimal Machin π 和 Taylor sin/cos，在 80/110 位精度分别生成 15361×4 I32，字节相等。tan sign×floor、sin/cos RNE、reserved0；真 tan 和本机 CPU libm 误差均 <1/Qm，Hq/Hfloor 保守界核对。角表 SHA `23b5210055d0b3f51892967170134a6cad14360579b038e26ee48f3ea5bbb368`。仅离线生成证据，未标设备/效果通过。
4. `control.{h,cpp}`：独立控制面原型，固定120 carry、根 nonce/epoch/版本/摘要绑定、精确256word completion 长度/字段/保留位校验、候选 bank 选择、已接受源保留、stop/late/timeout quarantine、STATE_LOST、双 publication lease/release 背压。测试使用 mock FenceState 与显式 TEST CompletionPolicy；**没有真实尾 GL fence/map，也没有 GPU authority 数据**。CompletionPolicy 默认值全0，必须显式提供完整编码才可初始化，测试编码不冻结为产品 wire。
5. `resources.{h,cpp}`：K192/K512 manifest 原载荷 15059984/36997904B 求和、block/4 binding/64 invocation 门、checked range 与 state segment alignment 规划、事件/dispatch 上界、K192 双 pad 差额诊断。不曾进行 GPU 冷分配，`deviceValidated=false`；不得将计划字节称真实 GL/RSS。
6. `foundation.{h,cpp}` + 模块 CMake：真实 OHOS native 编译并生成 `libgrs_foundation.a`；提供冷表/GLSL库字节与 SHA。`GetFoundationInfo()` 明确返回 `implementedFeatures=0, stepShaderImplemented=false, connectedToXComponent=false`。根 CMake 只构建/链接这个隔离静态库，不打开新路由。

没有 CPU 补跑、不借 shader 显示移动点冒充整步、不启 M1 水/J、M2 pair 或 M3 旧通道。后续仍只能 XComponent+shader 含前背景，故障 FAULT/完整有效 STALE，禁止 Canvas。

## 二、阻止冻结产品 wire 接通的工程问题（已评论原卡）

### Q1：K192 的 pad256 排序存储需明确

T233 §3 规定 pad256、36 层 bitonic；manifest `two_index_arrays=8*K=1536B`，按两份各 K 个 U32 解读不足双 pad256 的 2048B，差512B。`check_t236_spec.py` 独立执行256项排序网络：初始 ID0..191 有效、192..255 padding，在 `k=128,j=64` 首次有64个有效ID进入索引192..255（首个为 ID128）。因此不能简单把 >=192 的临时写全部丢弃。

这**不是 GPU 不可行的证明**。可由 architect 明确增加512B并修实际资源模板，或指定原容量内的 in-place/disjoint-pair、重排 workspace 等布局，同时明确读写隔离/barrier与真实阶段计数。developer 未擅自扩冻结数组、变更排序revision或用CPU排序。

### Q2：T233 承诺的事件/结果编码域未落 manifest

T233 §2 写明 event64 的 used mask/保留0由 architect 落 manifest；当前 `layouts.event64` 仅 `revision/wordCount/storage/words`，未提供逐 kind used/zero mask。`kind/flags/phaseSourceState` 的位编码、rootKind/outcome/terminal status/reason/magic/seal 的数值域也未给出。completion 还需区分正常拒绝、同 key replay 的原终态字段、成功/partial 与 fatal 首错：不能把 metadata-only replay 的原 admitted 字段误当本次新源成功。

**需要 architect 在对应工程稿/manifest 补齐**：事件 kind/flag/状态编码及逐字段身份/保留零规则、completion/source terminal 合法编码与 replay/拒绝语义、K192 索引布局裁定。当前 host test 的显式 policy 只用于开发控制面 fixture，不可当已批准产品 wire。没有改 architect/designer 文档，也没有修改独立 tester oracle。问题评论已成功同步原 T236 卡。

## 三、非设备开发验证

| 检查 | 结果 | 证据 |
|---|---|---|
| 控制面、completion 翻位、late/stop/lease、时钟、资源规划 | Od 76 PASS /0 FAIL；O2 76 PASS /0 FAIL | test/reports/t236/native-foundation.txt |
| Python int/Fraction 独立 raw-bit 数值差分 | 每档82205向量，Od/O2均0差异 | integer-differential.json |
| manifest绑定/角表重生成 | --check PASS，80/110位字节一致 | generation-check.txt；angles_certificate.json |
| 排序工程问题复现/缺失编码核查 | 问题证据已生成；非产品FAIL | sort-storage-gap.json；handoff-questions.json |
| 真实工程构建 | ArkTS/native检查 + PackageHap BUILD SUCCESSFUL，最终零错误 | package.txt |

构建曾在 native 编译成功后遇到打包工具 `spawn EPERM`；不改代码直接重试后成功。首次原始日志保留 `package-spawn-eperm.txt.gz`，不隐藏失败执行。

native archive SHA `cc16fc7f627223f55eb50bc04371dc0fa6938a0da787b49e2dea20b0d08143d6`；unsigned HAP SHA `ce81cfc63d5d4b67f3b5e833a2c381e670ef1e999bb4c51fb21aeaa9a14b3e41`。逐文件源码与产物 hash 见 delivery-manifest.json。静态库构建不表示新增物理符号已被页面调用，也不表示设备运行通过。

## 四、真实未完成范围与接力

M0 source32/dedup64/event64 的 GPU 写者、积分/DDA/CROSS/LAND、三色床/actual guard、全 audit/spare event replay、publication/F32 审核、尾 fence/completion GL transport、同 XComponent 沙场景均**未实现**。没有 shader dispatch、实际 SSBO 分配、GPU/端到端/收益或设备显示结果。GR01–GR09 产品判定仍 NOT_IMPLEMENTED；本轮基础差分不替代 T234 独立产品验证。

未完成原因是上述工程 wire/布局待裁定，**不是等待T232设备窗口**；离线开发已按解锁范围推进。基础切片提交后，本地仅 T236 状态记“看评论”；待 architect 补齐原工程接点后继续 T236 主核与平台串行集成，不应关卡或记完整 M0 交付。manager 按原卡问题分流即可，不新增验收门。
