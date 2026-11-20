# T734 RX / DM BOOT-only Create

交独立的 `GRS_G9_BOOT_ONLY=ON` 构建选项和 unsigned 工件。默认编译仍选择原单程序。两臂使用 T733 的同一闭包算法，从各族最终生成源特化原 `g9Run` 的 12 / `0x7fffff00`，校验已批准父源、角色 SHA、原 12/2404-pass 表摘要和首 pass。实际生成超过 49,152 B 或闭包身份不同即构建失败。

| 家族 | BOOT GLSL | 原单程序 GLSL |
| --- | ---: | ---: |
| RX | 22,318 B | 146,301 B |
| DM | 22,353 B | 146,316 B |

## 实际创建与证据

- RX 从原研究页、XComponent 到原 `ConfigureResearch` / `Transport::Create`。DM 的 ARM64 `g9_dm_differential` 从原实际 EGL pbuffer 调同一个包内 `libgrs_dm_t647.so` 的 `Transport::Create`。DM HAP 是该 DSO 的编译和签包容器，原生入口单独交付。
- Build 前核实际源码 SHA 和字节门。使用 T475 的 `G9_BOOT` 角色、精确源码、各族布局及 roleSetSHA。真实 program 检查 16 个必需 uniform 的类型、元素数和 location、64×1×1 局部规模、三处必需 SSBO 与可选 EventPath 原 binding。沿用原 profile、seed 校验、缓冲分配和 512 B Config 上传。
- 两次真实 dispatch 各用原 SSBO 与 buffer-update barrier，实际建立 GLsync 并保原 `glFinish` 等待。只有真实 fence 返回有效、从 `WORK work[7].begin`（字节偏移）映射 8 B、unmap 成功、GL/EGL 无错、原 error=0 与 seal=0x641647 同时满足，才通过原 Prepare 并让缓存具备复用资格。
- `BOOT_AUDIT_OK` 是诊断终态（0x666）。Create 返回该态前由原 owner 释放本次资源并先清程序跟踪，transport 不授予 root 或 cold 权限。RX 页以该态结束配置等待，DM 入口写终态和退出码，均未构建产品 READY、SOURCE 或有效仿真帧。
- `G9Boot` hilog 记录 family、BOOT、PID/TID、run/owner/context、进程映射 library BuildId、source/roleSet SHA、编译和反射阶段、完成等待、map/unmap、首个 GL/EGL 原值与两字原尾。DM 同步把同一原事件写到 stderr，`execution.jsonl` 在 Create 前明确刷新。缓存是否真正 PERSISTED 由原文件线程的 T475 记录独立确认。

## 工件与接力

真实 CMake 生成源、链接与 HAP 内 ARM64 ELF 的 SHA、BuildId、字节及 GLSL 静态反射见 `test/reports/t734/compile.json`。RX 工程为 `.codex/local-test/t734-rx`，DM 为 `.codex/local-test/t734-dm`。两份 HAP 均为 `entry/build/default/outputs/default/entry-default-unsigned.hap`。DM 同时提供 `entry/build/default/intermediates/cmake/default/obj/arm64-v8a/g9_dm_differential`，运行时加载同目录回执核对过的 `libgrs_dm_t647.so`。

RX 冷启在既有页面只配置一次，取 `G9RxConfigure` 与同 run/context 的 `G9Boot`。DM 使用既有 T670 DM K192 fixture（`config.u32`、`settings.u32`、`identity.u32`、`seed.u32`），以 `g9_dm_differential INPUT_DIRECTORY NEW_OUTPUT_DIRECTORY [CACHE_BASE_DIRECTORY]` 启动，保原 stderr、`execution.jsonl`，并分别核实际加载 executable 和配套 DSO。每臂留实际 PID/start、artifact SHA/BuildId 及本次真实完成等待和原 error/seal。冷轮成功并核缓存落盘后，再做同输入且新进程的暖轮。若未返回则记录最后实际入点及系统进程原件。

开发侧未签包、未使用真机。GLSL 静态编译、Hvigor/ARM64/ArkTS 和包内源码一致性已核。两族默认单程序也分别构建通过，包内仍为批准的原单程序字节。设备 Create、BOOT_AUDIT_OK 及暖命中仍待各族真机取证。SOURCE 放行由 T736 消费各族独立设备证据。
