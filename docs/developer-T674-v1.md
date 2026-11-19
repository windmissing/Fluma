# T674 · 新RX捕获作用域与canonical源身份交付

2026-11-19；developer，仅开发/编译，不使用真机，不执行测试、产品、shader、签名或部署。按T665工程及contract完成；原T645后续设备编译/输入/物理/呈现域由effector继续，不把静态编译当产品PASS。

- RX生成器核冻结父raw SHA、T665原函数LF SHA与唯一g9Capture花括号范围；只在清contacts循环、显式复合扫描loop、ordinal内选中包绑定范围转换clearSlot/scanSlot/selectedSlot。记录转换1次、前后函数SHA、前后精确binding范围及源行映射。函数外不改；冻结父和默认DM/SPF/G11未修改，质量/动量公式、排序/比较/循环/continue/break、事件索引、Config64、T647数学/接受门不改。
- pipeline.comp、pipeline_source.h literal同一LF UTF8原件，SHA不含C终止NUL；无BOM、内嵌NUL、CR或#line。sourceManifest/implementation/bundle/cache ABI/reader身份包含hygiene版本和实际全调用链输入。新hooks源独立生成并留独立身份/prod diff，没有用旧harness冒生产资格。
- CMake生成/依赖闭包和每次build前verify_assets核源输入、comp/header/contract/全部生成物及bundle；缺/旧/不一致fatal。等价../依赖REALPATH后去重，未删输入。首轮Ninja重复输出原错保留build-first-failed.log，修复后全新目录成功。
- RX交共享工厂前用已有CPU字节SHA/strlen核真正PipelineSource、生成header和当前program contract；失败SOURCE_IDENTITY_MISMATCH(0x665)，保firstFailure并具名stage。无额外GL查询/glGetShaderSource、等待、线程、热IO或物理CPU实现；新cache ABI绑定实际implementation。

完整native/ArkTS/unsigned HAP构建41.773s、最终增量13.396s，零错误。最终生产同一canonical shader通过glslang静态编译；工具绝对路径/二进制SHA、版本、完整flags、无宏、ES3.10目标、原始stdout/stderr及退出码在compile-receipt.json/glsl-static.log。未执行shader。

完整compile_commands/build.ninja/rules.ninja、生成原件/reader/inputs、实际源码SHA、首次失败/最终成功日志在 `test/reports/t674/`。

复现：`node tools/g9rx/prepare_build.cjs .codex/local-test/<全新目录>`，在该目录用当前SDK/JBR执行`hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`；随后`python tools/g9rx/build_receipt.py <目录> <成功构建log> <receipt路径> --validator <glslang.exe>`。

这是开发编译交付，不承诺设备编译必过；若有新驱动拒绝，沿新源、同attempt真实firstFailure重新归因，不盲换变量。未修改门禁、协议payload或渲染路径，前景背景仍仅XComponent+shader。

## Actual artifact identity

Canonical source: 145458B / 2144 lines; SHA `4faf48aa0ebeafe575349f30a36ec686ac9a63ed6fece05e2002d4032706f526`. Linked and packaged ELF source bytes match exactly. Capture mapping: lines1723-1750; selectedSlot at1728.

- unsigned HAP: `3d4042bce6af2b65908480f98917f78bfd5a2bf2b221163e37d3c05408b90c0c`
- linked native: `9c32020bb5dbeb4452917e30011146583d17a25491b1ca1fac5ff90f63734b42`
- packaged native: `72d3b7214d7fbf64e5f5aaf3d3a2828f34e8c0625648670aaddf126c6f91f69c`
- BuildId: `e989ed1053e9a6e207065806bb15c42d8469e939`

Protocol payload:40960B; actual native App:91392B; host/driver/FBO excluded from payload.
