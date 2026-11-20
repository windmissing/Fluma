# T739 G13 Boot 生效首因与签包收据

## 旧样本

T737 核实历史票据 250 在真实 Boot fence 返回后，op65 的密封尾为 `[ctrl255=0xc0de1301, ctrl7=27]`。WORK 的次正规加零结果为 0。op66 的两个封印都可见，两组次正规保留和次正规生成位置均为 0，对应原位比较掩码各为 `0x0c`。宿主先按 op65 失败返回 27。op66 独立失败，旧页面最后显示的 `INIT_FP32_RESULT:EXIT` 只是最后一项观测。该结果属于已测 GLES 程序与环境，驱动内部原因及重复性待证。

## 开发改动

- 保留原 `G13BootReadbackV1`、32768 B 四页 ABI 和原错误/门顺序。新增 `G13BootQualificationV2`，op65 和 op66 各有终态。op65 记录有效 seal、尾部原错、实际 WORK 五项。op66 记录每轮封印是否存在、原 16 字、每轮错位 mask 和资格结果。已有 audit fail 优先决定 `effectiveRejectStage=OP65`，只有原 audit 通过才可由 op66 拒绝。最终阶段给出原返回码，页面按当前票显示生效拒绝。
- 真实 fence 返回后在同一 CTRL GL 对象读取原 `[744..747]` 和 `[18648..18652]`。只在 map/unmap 与 GL 检查成功时输出该组字。额外诊断读取错误独立保存，保原 gate、firstGl 和退役。无数据字段留空或 −1。输入和样本均来自 GPU buffer 回读。
- 附属收据绑 boot/attempt/queue/run/context/surface、PID/进程启动 tick、owner TID、程序/CTRL/WORK GL 对象、已观察 fence、映射 BuildId 及匹配文件 SHA。原配置/种子收据 SHA 与已反射程序的来源、缓存 key、真实加载 binary 的 SHA 及 GL/EGL 指纹分列。没有可验证的加载文件或进程启动 tick 时按实际缺失保留。
- HAP 角色收据直接从各臂的实际 `assets_manifest.json` 导出 compute/graphics、正常派发 opcodes、`reworked` 和 compute 的私有 `qualificationOpcode`。原文件 SHA 与实际编译的 LF SHA 分列。仅当源文件含 CRLF 时标 `CPP_TRANSLATION_LF`，同时逐字节核真实 ARM64 ELF 内的源码和 NUL。

## 编译及交接

T730 针对相同输入优先使用 SPLIT，同票记录 `G13Init`、`G13BootScope`、`G13BootProgram`、`G13BootOp65`、`G13BootOp66` 及原四页。先核 PID/start、实际映射和 program/cache 路径，再按终态、原位和 `effectiveRejectStage` 判定。仅在输入与结果矛盾时另开同 owner 的 uint echo 对照。后端更换与放宽原门均不在本卡范围。

两臂通过 Hvigor assembleHap、ARM64 原生与 ArkTS 编译、GLSL 静态编译和 T729 的 `sign_pair.py --verify-only`。最终包内各角色源码 SHA 与 T732 相同。MONOLITH 未签 HAP 为 `ce84ac77…`，BuildId 为 `23209a13…`。SPLIT 未签 HAP 为 `3ff632a0…`，BuildId 为 `16704862…`。T730 直接消费 `test/reports/t739/MONOLITH.json` 和 `test/reports/t739/SPLIT.json`。开发侧无真机和签包，设备 FP32 资格仍待复验。
