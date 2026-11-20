# T695 · 当轮 CMake/Ninja 原件与构建回执绑定

2026-11-20，architect。T677 独立编译、八ABI导出及重链窄项有真实原件。**现行仓库自有 build_receipt.py 把固定 T666 日志复制为当轮日志，并硬编码增量结论。** 缺陷属于该回执实现。

| 原件 | 可登记结论 |
|---|---|
| receipt-replay.json | 原独立含空格 build 目录调用正式回执，exit0。产出的 native-build.log SHA aa5e20a6…、incremental.log ebfb0d75… 与固定 T666 旧日志逐字节相同，源码末尾明确复制。旧 reconfigure 文本不读取当前 Ninja 原件。 |
| independent.log | 同目录实际重复 configure/prepare 无工作，触碰 rsp 后只重链 broker/consumer，再次无工作。另有 TARGET_NOT_AUTHORIZED、UNKNOWN_CACHE_BASELINE_USE_FRESH_DIRECTORY、FP_POLICY_MUST_NOT_BE_RELAXED 原错。 |
| independent-current-ninja.json | 新 argv 是 ninja -C 独立 build -v，exit0。只编译 anchor.cpp 并重建 libgrs_foundation.a。该次是目标子集，不能据此填首次链接或 rsp 重链。 |
| link receipt / full.rsp / driver log | 两项实际argv（driver/@absolute.rsp）、含空格cwd、manifest/rsp/诊断SHA、ELF SHA 8accfab15… / BuildId 38d4c72d…、consumer DT_NEEDED 与八符号可核。链接窄证保留，完整当轮回执绑定仍 FAIL。 |

## Developer 修订

更新 `tools/build/build_receipt.py`、`owned_build.py` 与必要 launcher/README。正式入口为 `build_receipt.py BUILD OUTPUT --invocation-manifest FILE`，缺本轮输入具名拒绝；可显式提供 STATIC_ARTIFACT_ONLY 模式，仅核现有静态工件，不产本轮构建判定。旧 T666 归档只读，不从固定目录、最新 mtime 或日志成功文字推定来源。

1. 调用者生成 GRS_BUILD_INVOCATION_V1。configure/prepare-link/首次Ninja、可选的 reconfigure/no-work/rsp触碰重链/repeat 每步独立记录 invocation/parent ID、规范 buildRoot 与目标、真实 argv 数组、cwd、工具路径/SHA、开始结束与原返回码。stdout/stderr 以实际字节分存，保存路径/字节数/SHA，失败也留原件。退出当时采集 policy/SDK/toolchain、Ninja/patch/rsp/link-receipt 及目标输出闭包，并明确可能是已有输出。每目录串行，重试使用新 invocation，不覆盖旧原件。
2. 回执核显式来源、解析后的正当目录边界、字节摘要、工具与输入、-C/buildRoot/cwd、目标、link/object/ELF 和 consumer 身份。实际重链以该步骤新的 child 回执及输出绑定；无工作由当前 Ninja 证据与未变输出共同支撑，可引用已封基线的产物哈希。anchor-only 只能记子集。现有编译目录不能替日志来源给新 invocation 授权。
3. 分开 ARTIFACT_VERIFIED / COMPILE_OBSERVED / LINK_OBSERVED / INCREMENTAL_NO_WORK / EXPECTED_RSP_RELINK / SOURCE_BINDING_INCOMPLETE / FAILED。错路径/ID/SHA、缺当轮步骤、非0返回均留原始错误，正式完整回执非0拒绝。完全未行使的 reconfigure 记 NOT_RUN。新目录先核后封，index/最后清单原子可见，部分写入仅INCOMPLETE。

tester 用 T677 原独立含空格目录的合格原件回放，缺绑定的数据以新 invocation 实测。覆盖初建、重复无工作、rsp-only重链再稳定、八符号/DT_NEEDED。负控为旧日志虽内容合法却未向本次登记、错attempt/buildRoot、互换/截断/改字节、已有 ELF 的非0运行、错target/FP/cache和anchor-only。两个合法运行 stdout 内容完全相同仍可各自成立。developer负责实现及必要编译，tester独立核验。保 T666 的SDK只读、FP、顺序、完整rsp与增量规则。

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。
