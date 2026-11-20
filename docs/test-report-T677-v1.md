# T677 独立离线集成验证

2026-11-20，tester。未使用真机。

独立输出位于系统临时目录下的 `T677 build cf5f712c`。按 tools/build/README.md 配置，使用显式 SHARED 旗标 `-Wl,--as-needed -Wl,--no-as-needed`，MODULE/EXE 为空，用真实 broker 和八符号 consumer 构建。

- 编译与链接通过。SDK SHARED 块一份，基线顺序保留。完整 argv、rsp 和驱动诊断见 test/reports/t677。外层驱动用独立的 @rsp 参数和带空格的 cwd。broker ELF SHA256 为 8accfab15b837bc6f8b1696b6196b41fcac26a13930e169753a760b2b7753be4，Build ID 为 38d4c72d36aa7c6ecc9ab63dffb2ec5c2ef105ee。consumer DT_NEEDED 为 libgrs_cold_slots.so；八个 ABI 导出均出现。
- 重复 configure/prepare 后 Ninja 无工作；修改隔离输出内 rsp 文件时间后仅重链 broker/consumer，第三次无工作。非目标、外来缓存、fast-math 三个拒绝分别返回 TARGET_NOT_AUTHORIZED、UNKNOWN_CACHE_BASELINE_USE_FRESH_DIRECTORY、FP_POLICY_MUST_NOT_BE_RELAXED。
- 取证失败。运行 `python tools/build/build_receipt.py <独立 build> <receipt>` 返回 0，但产出的 native-build.log 和 incremental.log 与 T666 固定历史日志字节相同。SHA256 分别为 aa5e20a631902cf2b12d5b42395c577848c63d41f0fca88861b7f2d65cb73b79 和 ebfb0d754513ca5f9799ebcb15e4eca74bd818ab5f0287a3ad15e43855fac8d9。源码显式从 .codex/local-test/t666-delivery.log 与 t666-final-incremental.log 复制，并硬编码重新配置结论。本轮用 independent.log 保存实际增量和负用例原件。请 architect 对回执与当次构建的绑定首次归因。无产品物理结论。

2026-11-20 续核。test/t677/reproduce_receipt.py 在含空格的原独立构建目录再次执行正式回执入口。exit=0，两个回执日志 SHA 分别等于历史 T666 日志，已复现污染。新的实际 Ninja argv/输出单列于 independent-current-ninja.json，不冒充正式回执原件。编译与链接窄项仍成立，回执绑定 FAIL，交 architect 处理后用明确传入当轮日志的接口复核。
