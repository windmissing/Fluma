# T666：幂等已知链接基线与完整 native rsp

2026-11-19，developer，开发与 native 编译完成。无真机、无测试/产品执行/签名部署。按 T655/T666 当前明确范围交实际 broker ELF/ABI/工具原件，不把 native 构建工具交付冒物理或 App PASS。

## 实现及范围

`tools/build/owned_build.py` 要求用户显式 SHARED/MODULE/EXE 原基线、SDK/compiler/CMake/Ninja 来源及结构化 definitions。首次须新隔离目录；同 cache 仅接纳已知基线或上次同 policy 的 SDK 结果，不从任意污染 flags 猜拆。每次真实 SDK 初始化前回到该原基线，随后原 SDK 只求值一份政策块；SDK guard 的重复进入保持已知结果。没有修改安装 SDK、全局删 cache、全局 REMOVE_DUPLICATES，也没有删除用户库/group/override。FP 门保留，明确 fast-math/contract 放宽定义拒绝。

同工具的 prepare-link 由实际 Ninja `compdb -x` 展开原 driver 完整参数，Windows 原生 argv parser 解码，拒绝未知 shell 前后动作，不执行原 cmd。只替换 **grs_cold_slots** 的真实 link edge/rule：外层206字符，短 launcher+manifest；rsp 含全部 LANGUAGE_COMPILE_FLAGS/ARCH_FLAGS/LINK_FLAGS/对象/搜索路径/库及其顺序，而非再搬对象 rsp。原 source/隐式/有序依赖保留并补 manifest/rsp/launcher 依赖。

`link_from_manifest.py` 校验工具/rsp SHA，以 `[clang, '@absolute/rsp']` 两个 native argv、明确 cwd、shell=False 运行。保存真实 `-###` driver 解码诊断，再实际编译链接；UTF8无BOM/LF响应文件的编码与 Windows native argv 独立。未知动作/来源/已改状态具名拒绝，不任意归一化。对同 build 序列化调用，不同会话独立目录。

新增 `broker_compile` 工程通过仓库现有 cold_slot_broker.cmake 编译真实 broker 源；consumer 只编译链接八符号取址，不加载/执行。不是另写 broker 替代实现，也不将 broker.cpp 静态混进各 route。原业务源码和 CMake 未改；target 集成在生成 Ninja 中仅此边受控替换。README 给出用于实际 App native source 的同样接法，原 T590/T541 可自行用新 cache/已有正确包续测，不增集中准备门。

## 编译与证据

实际 policy `.codex/local-test/t666-policy.json`：用户 SHARED 基线 `-Wl,--as-needed -Wl,--no-as-needed`（两个有序参数均保留），MODULE/EXE显式空，编译 `-fno-fast-math -ffp-contract=off`。实际构建 `.codex/local-test/t666-delivery`。

- 原 SDK read-only SHA 前后相同。首次及反复配置的 SDK `--rtlib=compiler-rt` 块均一份，未重现64份累积；不声称已删除或修补旧污染cache。
- 实际 native broker+consumer 编译链接 rc0、零错误（既有 gcc-toolchain unused 警告）。最后同 policy 重配/prepare 后 Ninja `no work to do`；源/manifest/rsp/launcher 变化仍为真实依赖。
- 实际 SONAME `libgrs_cold_slots.so`、八个 GrsCold/GrsFullGpu export 完整；consumer DT_NEEDED 真实依赖该 DSO。broker NEEDED 为 libc++_shared.so/libc.so。不靠 stub/静态多副本冒共享对象。
- broker SHA `f673442fd4d414c9582038c452c3d639174448f8e4c6f5dd4a4b76c47feca811`，BuildId `9984e49c777f7688234dc84c4e5f2e21a3136f3b`。

`test/reports/t666/compile-receipt.json` 和同目录保存 policy/原基线、实际 cache、原生成 Ninja、全部原/派生 driver argv、完整 rsp、实际 driver diagnostic、工具与源/对象 SHA、ELF metadata/exports、原始编译及增量日志。未执行 unit/产品、HAP部署或设备；本卡作用域为 native 构建wrapper/共享broker ABI，不涉及 ArkTS 行为。

复现与边界见 `tools/build/README.md`。只有当前已知 CMake no-op pre/post shell 形状受支持；其它形状具名拒绝，不能猜 shell 语义后继续。历史 SDK/native链接、设备M2 GLSL LINK、host dispatch 三域不合并归因。
