# T648 developer 交付 v1

2026-11-19。本卡完整范围开发完成；不使用真机，不运行单测/产品/仿真，不签名、不安装、不部署。结论仅 **COMPILE_PASS_NOT_PRODUCT_PASS**。

## 实现与边界

- 默认 g9dm 已替换错误物理构造，完整 Config64 与正式 R0/S40 模板分别校验 LE 字节 SHA；实际 run/owner/context/inputEpoch/parameter 身份写入后重算上传 seed SHA，五银行一致上传，初始化审核由 GPU 执行。Config DM8 来自 WORK Config+112..119；DM48 使用 K192=1286784/K512=1289344 的独立尾段，每列48word，24576B，未借用 ColumnStage。
- 整数 DM 求解保宽 H/Hs/chiS、最终量钳制、q 上界/32轮二分/final q 与真正 q+1 重算、top-first 来源连续前缀、容量守卫。post-FOLD 完整私有预测包含 HR/bed/gxδ，按 entry mode 完整摩擦得到 pEnd，RNE 中点被 corrector 真正消费；actual 从 pEntry 累积且力只加一次；support、静止/捕获、32/J 及 stopping shear 走同源实现。事件15/16的字段掩码不变。
- 保真实 scope/stage/micro producer 与拒前缀证书，controls120..127 按 T647 rev1，审核16/17来自真实证书；SOURCE12/0xffff、Q2404/0x3ffff，rootKind 显式独立，Q 接受 quanta+1、SOURCE零时/去重零提交。中点拒绝5/6/7，28/29保硬错，未扩大 BLOCKED 1..18。
- 默认 gate 只接受实际 Transport 封装的 current-context/owner/program、原始 mapped tail、真实 dispatch/barrier/fence/time/live-guard 证据。证明绑定 Producer，同一 flight 提交复核 current context、deadline 与原 permit。controller 对 Accepted/Blocked 都先复制预演计划，再在同一 owner 锁内完成提交；失败不先切 bank/推进计划。递归锁仅允许同线程的提交复核，不释放跨线程事务边界。
- 新共享内部纯验证核心 `entry\src\main\cpp\gpu_resident\acceptance_t647\contract.h`：十条 family/rootKind 行固定于正式机器表；Expected224 为只读 view、流式896字节 LE SHA；private native program/observation 与 private validated proof，不接受 JS 授权布尔、不补 CPU PASS。新 G11 R1/DM 父证明分别是 ValidatedCompositeParentR1/DM，及各自 blocked 类型；全256word相关协议语义与156..239扩展/240..255保留逐项校验，SOURCE duplicate/BLOCKED不改不可变候选 Expected hash。
- R1 默认 completion_gate 既有 Common/Accepted/Blocked 函数体原字节前缀保留、原 API/常量与12/2024行为不变；仅新增 Composite 方法/类型，R1其余原件 SHA 均未改。g9rx、G11 owner/scene、T549 冻结源码均未改。
- schema/generator/cache/reader/seed/config 身份同步；旧错误 DM rev1/2024/0xffff 禁止热续接。旧 T555 生成入口委托正式同源生成器。冷 prefix 不扩容，在 word1408捕获真实128word controls，T647证书位于1528..1535；reader 核 config/input/段哈希和真实 scope 证书，不制造 live proof。K512 root/FULL/双FULL=48050176/188059648/376119296B。
- legacy NAPI include 的新65word封装 = 完整 Config64 + 独立 fullTrialTrace 0/1，参数 SHA 仍只覆盖正式64word。该孤立 include 已做 SDK 语法编译，未伪称注册产品页面。

数学实现固定参考为本工作区已交 T644 正确 DM 构造的字节冻结副本，位于 g9dm/frozen_t647；本卡不 import/改写 g9rx。只替换默认 profile/input 身份、Config偏移及原默认 flick 输入验证，不借 RX 的125ms规则或 REAL_TOUCH资格。冻结 reference 原件及默认 construction 的各自 SHA 见 identity.json/receipt.json。

## 编译与身份

- hvigor 无签名 HAP 构建通过：**12.603秒，零错误**。实际 libgrs_dm_t647.so 链接默认 DM transport/controller、共享验证核心及两父组合门；ArkTS 与打包通过。HAP是独立编译容器，不是新增产品 UI/研究页。
- GLSL 静态编译、SDK AArch64 NAPI include 语法编译、Python 语法编译通过；未执行 shader/物理/测试。NAPI语法适配器只提供旧App声明表面，不作为产品或证据生产者。
- ELF SHA `3a77a98354e3636f2fb248e9bd5a053bb5ef13c751065d159242334832fa7ff6`；BuildId `83281f1ad8a08225dd57e7d658613686bc8cb9fd`。
- unsigned HAP SHA `f61253e87cdec1c119fb35eb75e5ff71e2b2a941ddb855e760f46b2cdccf57eb`。
- implementationSHA `79c58d83feea110905dd43df0041d8de2f16a631f2bd553ade6523ad86cf69b3`。
- actual normalized shader SHA `a85f6da894d361f2013f878c313cd4a7dd4ed2d30131c31a92ac7d175c980bf4`；staticContractSHA `7d786425590c689fb22eac7fb5b8141020d7259393a0a829645b74cee1a1ded6`。
- 原始日志、全源/生成产物 SHA、编译收据在 test/reports/t648/compile-receipt.json 与 hvigor-build.log；生成表/reader 字典在 tools/g9dm/acceptance_dictionary.json。

## 实际 AArch64 ABI 与内存账

以下来自实际 ELF 导出 grs_dm_t647_proof_abi 的静态读取，未加载库：

| 项 | word/byte值 |
|---|---:|
| revision | 1 |
| expectedWords | 224 |
| expectedBytes | 896 |
| ProgramFacts | 408 |
| ActiveProgramIdentity | 416 |
| ExpectedContract | 56 |
| ActualEvidence | 64 |
| NativeMappedObservation | 80 |
| defaultDMValidatedRecord | 1384 |
| ValidatedCompositeParentDM | 1064 |
| ValidatedCompositeParentR1 | 1064 |
| DMTransport | 15864 |
| DMConfig | 392 |
| DMController | 45560 |
| DMBlockedRecord | 1384 |
| R1DefaultValidatedRecord | 1320 |

ExpectedContract 56B为短生命期只读引用/计数对象，无常驻896B前像，也不把 raw-tail 当 expected。ProgramFacts/ActiveProgramIdentity/Observation 尺寸为具名对象sizeof，不冒充编译器实际栈峰值；Controller账含内嵌Transport/Config，排除动态cold槽、驱动/FBO/编译器栈。T639若保存自己的 claim视图/helper，应按本ABI另计其实际owner成员，不复用默认Controller当联合owner。

## T639 可直接消费的真实接口

详细调用与生命周期见 tools/g9dm/README.md。仅真实 grs::g11vis::VisualTransport friend 能调用 ActiveProgramIdentity::ForComposite 和 NativeMappedObservation 私有构造；ExpectedForActivePipeline 从实际program、冻结typed permit、显式parentKind选择十行合同，无任意count/mask参数。AcceptedComposite/BlockedComposite 分别返回对应父私有proof，具备 Words()/ExpectedHash()/Producer()。不能cast默认ValidatedRecord，不能把19/2543/2923尾改成12/2024/2404。

共享行表覆盖五family：旧R1默认沿原门；RX沿T644独立native工厂；本卡实现修正默认DM及G11双父可信入口。T639仍负责组合GPU扩展上传/写入、真实视觉审核、联合guard/双bank提交/publication/scene/bridge。本卡已提供并链接真实父证明，不以设备PASS或manager新审批作接口前置。

后续独立产品/设备验证由tester/effector按原任务推进；本卡不将编译成功冒成物理/审核producer设备验证通过。
