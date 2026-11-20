# T691 · RX coherent 标量实参与 V5 工件身份

2026-11-20，architect。T594-v2 的设备编译拒绝属于 **T674 RX 独立页**。设备原错是 `0:1063: S0001: Function call discards 'coherent' access qualifier`。RX 修复是 T645 的前置，T594 的 V5 ROOT/FULL 可用已交的 T669-v5 续行。

T594-rx（签包4efb9d63…）与 T687-rx 签包各自核收据，两包同一个 `libgrs_g9rx.so`，1404632B、SHA `72d3b7214d7fbf64e5f5aaf3d3a2828f34e8c0625648670aaddf126c6f91f69c`、BuildId `e989ed1053e9a6e207065806bb15c42d8469e939`。包内 NUL 前真实 compute 源与 canonical 145458B / 2144行、SHA `4faf48aa0ebeafe575349f30a36ec686ac9a63ed6fece05e2002d4032706f526` 一致。l2 是 LOADED_FILE_OBSERVED，运行态以原件中的文件、内存 BuildId 字段为准。h594第5570行保原S0001、布局保 CONFIGURE_FAILED_8。报告列的 v2-red.json 当前目录缺失，本轮依据日志/布局/签包/编译原件。

实际源978–980行的 C/W/E buffer 带 coherent。1019行 `g9AddMass(uint a,uint b,out uint result)` 是值复制，out 实参 next 为局部变量。1063行 g9MapAdd 的 merge 从 `C[at+3u]` 直接传 a。1522行 g9ProvisionalAdd 同类，从 `W[at+3u]` 直接传 a。其余两个调用从局部或函数返回值读取。父代码早已有此形式，此次 g9Capture 声明修复后观察到下一个拒因。GLSL ES §6.1.1 对普通非不透明实参使用值复制，设备编译器仍拒绝此处直接实参，静态编译通过不代表同设备通过。按工程兼容接缝处理，驱动内部原因未定。[Khronos GLSL ES 3.10 §6.1.1](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf)

## Developer 修订边界

静态排查也见1548行 `g9ProvisionalAdd(...,W[at+3u])` 及 `g9U32(C/W[...])` 等其它直接传值。它们是待核位置，driver 原件只定位1063。首版修订限定 g9AddMass 的两处精确消费点。编译/复测若出现同型新拒绝，用确切行与qualifier映射扩表，保持原短路与求值时点，不能预读所有缓冲来消除警报。

1. `tools/g9rx/generate_assets.py` 在真实 canonical 合成链增加有版本的局部变换。只在两处已核 merge 分支和原读时点，把 coherent 槽读入局部 `uint prior`，再调用 `g9AddMass(prior,q,next)` 或 `g9AddMass(prior,mass,next)`。checked add、溢出退出及成功写同槽保原顺序、地址和次数。保 coherent、屏障、绑定、线程可见性、原子/排序/事件及固定整数语义。不给普通参数乱加限定符，不全局删 coherent，不广泛替换未核调用。
2. 保 frozen parent bytes、T665 g9Capture 作用域修复及默认 DM/SPF/G11。生成时固定父/锚/函数范围，核各处变换恰一次和禁止二次转换，登记实际源行与最小 diff。更新 comp/header、cache ABI、reader/implementation/manifest/hook 身份，生成/链接/包内原字节逐层核对。保存实际编译工具和调用原件，developer仅实施编译，tester签包离线核对。
3. T645 等修订产物可用后，在实际RX页冷配，核同进程 loaded ELF/内存BuildId、同 attempt Compile/Link 首错。分别登记已消失的 S0022 和 S0001 是否还出现。一旦越过真实配置，继续原时钟/Fresh、接收时间、typed SOURCE/Claim、守恒及呈现。后继首错继续回 architect。

## T594 的独立 V5 入口

T669-v5 基于 T653 unsigned `60e28a67…`。V5 native `libgrsm1.so` 3201504B / SHA `35ac238c…` / BuildId `7f31c1de…`。按 T669 handoff 由真实体验页“打开 V5 独立研究”进入 GpuResidentM5，分别新建 ROOT/FULL 与四档真实配置。保真实配置票据、同 owner/pulse、BASE/SOURCE、分段流、capsule/公开身份与原档案。沿原 GpuM5Cold 和 `tools/v5/read_archive.py` 核完整段、SHA及终态；旧 M5_WAIT_PAGE_PULSE 属其旧包，RX 的 CONFIGURE_FAILED_8 也不得填成 V5 工作流状态。当前V5 真机完整冷档与产品结论仍待执行。

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。GPU 驻留、CPU 停投，前景背景仅 XComponent+shader，失败 FAULT 或明确 STALE。
