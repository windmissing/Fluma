# T728 G13 同父着色器构建

基于父版 `1b56ac557e2cb0278739d8df4bff82613732e5af`。两份隔离工程共用同一 native、ArkTS、图形 shader、契约和生成器输入。MONOLITH 保留原 compute GLSL 的 259025 字节，SHA256 为 `7563c61459861cec1eb5cea8f0e1a89387d0f2874daaf9f7e29e47d972f9f2d2`。SPLIT 只链接以下六份 compute 源和原图形源。拆分包内不含旧整段 compute 源。每份 GLSL 都从实际 ARM64 ELF 和 HAP 反查，角色/大小/SHA、反射原件和 BuildId 见两臂回执。

| SPLIT 角色 | 实际字节 | 正常派发 |
| --- | ---: | --- |
| BOOT | 23834 | 64–66 |
| OP_00 | 48759 | 0–17、50 |
| OP_18 | 48612 | 18–36 |
| OP_37 | 48445 | 37–47、51–53 |
| OP_48 | 48577 | 48–49、54–61 |
| OP_62 | 11458 | 62–63 |

生成器从原入口按具体 opcode 消去可静态确定的分支与无条件返回后的不可达语句，再保留调用闭包。合并角色按闭包重叠和实际字节预算确定。函数体、控制流中未特化的条件、buffer 声明与生成数值字不改。构建拒绝超过 49152 字节的任何拆分单元。原 820 个 quantum pass 与 16 个 source pass 沿原顺序消费，按表切换 program，复用四个 SSBO、原 barrier 和 root/scene fence。每个程序单独反射实际活动 uniform 长度和 SSBO binding，保持缓存源码及角色身份独立。

Boot 沿原 64、65、66 和 init fence 得到 `BOOT_READY`。随后 GL owner 为各后续 program 各做原 op 66 的两个 FP32 探针，等 fence 后读取本角色的原始位样本，通过后才允许上屏配置进入 `COMPUTE_READY`。后续角色的 op 66 私有探针不占用正常 opcode 路由，不发 root permit、不推进 Q。窗口退役与撤权沿用原 owner，取消后不再排新的角色。界面报告 `bootReadyNs` 和配置耗时。页内只在完整 READY 后开放激活。

## 工件及交接

| 臂 | 隔离工程 | HAP SHA256 前缀 | ARM64 BuildId |
| --- | --- | --- | --- |
| MONOLITH | `.codex/local-test/t728-single` | `fd7ed272` | `7c64adb33546ba1a24e8aca89e078b671830996a` |
| SPLIT | `.codex/local-test/t728-split` | `9edec170` | `a9363c84680c230ac8ecfe92d594d7c8e5889146` |

两臂均通过 Hvigor assembleHap、ARM64 原生链接、ArkTS 编译和 glslang 静态编译与反射。两次并行 PackageHap 遇到 Windows `spawn EPERM`，串行补打包成功。T729 的 `sign_pair.py --verify-only` 已联核同父输入、最终包内角色和 BuildId。T729 用 `test/reports/t728/MONOLITH.json` 和 `test/reports/t728/SPLIT.json` 续签，两份工程的 HAP 都位于 `entry/build/default/outputs/default/entry-default-unsigned.hap`。

T730 按 T727 冷暖合同在同一 G13 真实页进行：REST 配置，一次激活，保留 Boot 原回执、cache 角色记录及 32768 字节四页原件。配对完整时间步使用同一 T631/T649 非零输入。走现有页面 RAMP 时，先保同 scope 的基线已接受 field，再提交同一档输入，保存实际配置/种子/输入原字节，辨认 source 接受后的完整 quantum。 `G13FirstFrame` 在真实 EGL Swap 成功且再次通过许可检查后记录 accepted root 时刻、auditBits、sourceSeq、quantum、fieldToken 和 sceneToken。用 status、pack mapping 与冷档原件核同 token，按 oracle 核实非零及数值门。只有 REST 或只有首个 source 帧时，保留独立原件，继续等待 accepted quantum。首帧日志证明成功 Swap 调用与同 token，设备画面另行留证。

本卡未签包、未使用真机。编译时间收益、真实首帧、角色 FP32 与非零物理等价性均待设备实证。
