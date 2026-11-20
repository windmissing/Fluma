# T685 · DM 执行入口与 SPF 应用启动基线归因

2026-11-20，architect。结论是 **T541 v2 的两个应用启动都未行使 T670 约定的独立 NDK 差分入口**。DM 应用壳加载失败成立，具体差分执行保 NOT_RUN。SPF 应用启动已到旧 epoch3 的 transport Create 内，之后 UI reset把状态推进到 epoch4 的 WAIT_RETIRED，不能把新等待状态直接解释为旧 EGL 退休 API 卡死。

| 产物 / 原件 | 核对结果 |
|---|---|
| DM HAP 79769acc… | 签包匹配收据，arm64 包含 `libgrs_dm_t647.so` 和 `libc++_shared.so`。jscrash latest pid60973 启动通用 EntryAbility，ArkTS 进入 onForeground，缺 grsm0/grsm1 时访问 setActive 抛 TypeError。栈实际源映射是 EntryAbility.ts:466:19。保留为 APP_SHELL_WRONG_ENTRY；未见独立差分 ELF 的 argv、进程、execution.jsonl。 |
| 实际 DM NDK | `g9_dm_differential` SHA db325543… / BuildId ee6dead096651e4b1efe70a301f115ebc5712a47，依赖 DM专用so、EGL/GLESv3、hilog、libc++/libc。ABI及DT_NEEDED与原收据一致，无 grsm0/1。DM包作为本次 NDK 依赖来源合法。若未来确要分发可启动 DM 页面，独立壳应接真实DM Ability与依赖，下游另立明确范围，当前无需给测试包凑两套库。 |
| 实际 SPF NDK | `g9_differential` SHA d9fd39b8… / BuildId d528305615899e445bf9d08565a72eb77769a13e，依赖 grsm1、EGL/GLESv3、ace_ndk、libc++/libc。SPF签包 f1994d31… 包内grsm1 BuildId 29c44678dc8bd590d4b1a23d3d9bbbdb0d649df1，和正式T597-fixed/T593的dc54802f…属于不同构建。 |

## SPF 时间线

- pid62965，ownerTid63189。attempt1封口 SURFACE_CREATED，attempt2首次 RESET_REQUIRED、后 UI_RESET。epoch3/resetSerial1 的 CONTEXT_RETIRED 已有原件。它不能替 epoch4/resetSerial2 的回执。
- attempt3/epoch3，windowId=1是boot计数，candidateRun1。EGL create context、create surface、make current、query surface 均返回成功；327153277228752ns 进入 TRANSPORT_CREATE。submit26 ShaderEnter 后27 ShaderExit result=1，再28 ProgramEnter=327157703508752ns，原件无ProgramExit29/TRANSPORT_CREATE_EXIT。按冻结源码第一程序为 SOURCE 候选，28 只是复合Program边界，尚无 object/cursor 锁定 `glLinkProgram` 或 status/error 的任一单API。
- 327193511288440ns UI 请求 resetSerial2/epoch4。此时旧 ProgramEnter 距重置约35.808s，下一 boot 点以 UI_RESET 封口，用不上将该差当已知API耗时。epoch4 的 recovery 为 RESET_REQUESTED/WAIT_RETIRED，retired=false/resources=false、run/context/session=0，windowAvailable=true，回执中没有当前 serial2 的 retire-owner ENTER/EXIT；inCall=false只覆盖已记 recovery 函数。故当前可判 waiting-for-owner-retirement 且旧Create未见返回；未能定位 unbind/destroy/terminate 开始、已失败还是尚未执行。boot terminal/complete只代表生命周期封口/未报告丢行。
- 正式M1 T593 READY 属另PID/BuildId/入口；T590 object2单API属另一M2路径。三者不组成三态物理差分，也不证明 SPF 生产缺恢复或同一个驱动问题。

## 执行接力

tester按 T670 既有收据在离线包上核 ELF/依赖完整列表、各SHA/BuildId/ABI、实际 argv 与相应 input，入口命令注明是 `g9_differential INPUT NEW_OUTPUT` 或 `g9_dm_differential INPUT NEW_OUTPUT`。DM依赖与SPF依赖分别部署到独立可写目录，系统EGL/GLES和日志库用实际设备依赖，独立 ELF 从该目录取匹配的同版私有 so 和 libc++。核真实进程加载与stdout/stderr/退出/EGL原返回；应用 HAP 不作为该步骤入口。config16与config64+settings.u32分开，锚定实际fixture SHA。误走 EntryAbility 的证据保留给测试交接，当前不派产品加库。

effector 真机串行，先同合同最小实际执行，再覆盖 T541 原完整三态、K192/K512、0/15/30度、1..4根、ROOT/FULL各自条件与原能力负控；DM20/22/29/31 按测试合同归实际接受、拒因及冷档源。收 execution.jsonl、输入/request/completion、原GPU冷流、真实状态与返回码，并由现有 receipts/check.py/read_cold.py 联核守恒与 DM_NATURAL oracle。每个进程≤240s、单根250ms合同和根起点保持。没有运行结果则按首拒原件登记 NOT_REACHED/NOT_MEASURED，不把启动页面当差分的一臂。

独立 compute 差分与 SCENE_ATTR、PRESENT 三域各报实际能力/动作及原件。若独立 ELF 只行使 COMPUTE，就用正确产品页、同产物与身份另证场景属性和真实 Swap，不能借旧 SPF UI 或 pbuffer 替呈现。保 T541 的 S/F/P writer、局部/全局动量能量、来源/互斥/容量、去重/J 资格/replay、坏身份与 late、EVENT/PATH、sealed prefix 和资源界。FULL−8MiB 是独立拒绝负控，不能用短 span 的状态代 ROOT/FULL。

若独立运行确有 loader 错误，保准确 argv、device loader 错误及缺的实际 DT_NEEDED，交tester修设施。若确需追 SPF 应用基线，复用同进程现有 boot/recovery/resource 出口对齐 epoch3/epoch4、resetSerial、owner及实际ctx，取ProgramExit或 RETIRE_ENTER→UNBIND/DESTROY/TERMINATE 的同scope原返回，重置前后分别保存。首个真正工程失败再由 architect 精确修文档转 developer。无证时不猜修恢复或删门。

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。GPU驻留、仅XComponent+shader、CPU停投。
