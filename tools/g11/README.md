# G11 实际独立产品构建

开发与编译工具，不签名、安装、运行产品或测试。

1. `node tools/g11/generate_contract.cjs --check` 检查正式 T660 rev2 生成物。
2. `node tools/g11/prepare_build.cjs .codex/local-test/<全新目录>` 准备实际 G11 owner/bridge/page/Ability/native 链。缺源/旧源/正式输入大小或SHA不一致即失败。
3. 在隔离目录运行当前 SDK 的 `hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`。产物是 `entry/build/default/outputs/default/entry-default-unsigned.hap`。
4. `python tools/g11/build_receipt.py <构建目录> <证据目录> <glslang.exe>` 只做静态GLSL编译、包内ELF/ABI/BuildId/SHA和源一致性检查，不加载库或执行shader。

正式开发回执见 `docs/developer-T639-v2.md`，实际字典为 `docs/architecture-T660-g11-manifest-v2.json`。`prepare_compile.cjs` 保留为第2步的兼容命令名，已不生成旧编译壳。

研究默认OFF；冷选R1/DM、K192/512、V2048/8192、ROOT/FULL。等待bootstrap raw ready，可先导出seed原件；封存时实际释放bootstrap witness后，再开始会话/激活。手动测试源标为SYNTHETIC_TEST，倾角为NATIVE_CONTROL，不提供REAL_TOUCH资格。页面仅XComponent+shader，失败显示FAULT或stale的上一有效新管线帧。

ROOT/FULL、bootstrap和singlePhase witness的U32原件都由专用worker流式写入/核SHA。不得把编译、原件归档或digest称为产品/物理/像素PASS。
