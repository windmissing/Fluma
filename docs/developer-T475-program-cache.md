# T475 GPU program binary 持久化开发交付

2026-11-11，developer，按 `architecture-T462-program-binary-persistence-v1.md` 完成首版。不使用真机。完整证据见 `test/reports/t475/handoff.json`，自动原件位于同目录的 `m1/`、`m2/`。

交付为受版本控制的 ProgramFactory、ProgramArtifactStore 和 T475 生成器。生成器沿 T468→T452 生成来源，叠加 T456 boot、T467 recovery，再把 M1 和 M2route 的三个实际 compute 创建入口改接工厂。交付 HAP 使用这套生成应用。生成器同时处理两条合并冲突，保留 T467 的 lease/恢复和 Flush 观测，将 M2 的 NonceOf/ProgressClockV2 引用解析到实际命名空间，并去除页面重复 import。工作区默认工程沿用原来源，使用本功能须通过 T475 生成器构建。

| 实际 program | M1 | M2route | 导出条件 |
| --- | --- | --- | --- |
| SOURCE | 已接入 | 已接入 | 原 reflection、资源分配与 prepare 成功 |
| INTERMEDIATE_QUANTUM | 已接入 | 已接入 | 原 reflection、资源分配成功，原路径没有独立 prepare/warmup |
| FINAL_QUANTUM | 已接入 | 已接入 | 原 reflection、资源分配与 prepare 成功，保留后续原 T341 warmup |
| 场景 vertex/fragment | 首版未接入 | 首版未接入 | 按架构可独立派卡 |

key 使用实际 GLSL 精确字节、生成 layout ABI、固定 compute link 配置和实际 GL/EGL 环境的 SHA256。真实 context/config/version、驱动四字符串与排序后的扩展参与指纹。包 SHA 为 Want 声明来源，收据标明 DECLARED_SHA256，缺失时 UNDECLARED。epoch/run/session 与包签名 SHA 均不进入 key。跨 context 创建全新 GLuint，重新执行原 uniform 反射及 SSBO/资源初始化。

文件线程启动预载并校验 `filesDir/gpu-program-cache/v1`。GL owner 处理 GL、不可变字节和短内存锁交接，文件线程执行读写、摘要、LRU、同步和同目录原子替换。单 binary 上限 32MiB，内存 artifact 总 payload 上限 128MiB，条数上限 32。文件总量计算包括 binary header、收据和原子写临时空间。单 key JSONL 历史上限 128KiB，超限移除最早完整事件。队列上限 128，临时 wire/日志也各有固定上限。读者/待写 artifact 被 pin 时保留字节和预算，相同 key 并发提交共享不可变版本。

未知格式或加载失败使该条失效，本次只回相同 GLSL 一次。磁盘坏 hash、截断、schema/key/长度失败留下具体拒绝原因。导出不可用在本次启动按 key 去重。已失败的 shader 只记录诊断。缓存事件带 GL 原错/缓存错、原始日志 hex、reported/copied/truncated、各阶段 native stamp、支持格式及 binary 长度/格式/SHA。COMPILE/LINK/LOAD/EXPORT 调用进入前提交自动收据。文件 worker 按实际完成状态写 PERSISTED 或 WRITE_FAILED。内存 artifact 的原子状态区分 EXPORTED_IN_MEMORY、WRITE_PENDING、PERSISTED、WRITE_FAILED。`programCacheState()` 无 GL 调用，可读配置状态、条数及收据落盘失败计数。收据本身无法写入时通过该计数报告。

开发检查全部通过。Windows/MSVC 上真实文件层覆盖重启、损坏/截断、并发同 key、pin/LRU、128MiB payload 拒绝、日志滚动、配置与写入失败。ENOSPC、部分临时写入、sync/replace 失败由只在自测启用的宏注入，验证同一目标旧文件逐字节完整。真实 ANGLE ES3.1 工厂测试覆盖跨进程 compile→PERSISTED→load、同 key 不同包来源、代码/layout/link/环境变更、未知格式、驱动拒绝损坏 binary、坏 hash/截断后一次源码回退、原有 GL 错误传播、无效 shader 日志和不同存活 context 拒绝导出。无格式、空/超限导出使用明确标记的 GL 查询注入。

生成应用的实际 M2 App 创建入口冷、热两轮 Create code=0，三个 program 均 PERSISTED 后跨进程 BINARY_LOAD，热阶段源码编译次数为零，反射/prepare/warmup 收据合格。M1 检查通过同一个离线 App fixture 将 namespace 切回 M1，并编译执行生成的实际 M1 transport 源码，冷、热两轮也全部通过。该 namespace fixture 是离线适配。它与真实驱动执行、故障注入已在 handoff.json 分列。

最终隔离应用 ArkTS/native/PackageHap 构建成功，30 tasks，保留既有编译警告。签名及 verifyApp PASS。签包路径和完整 SHA256 见 `test/reports/t475/signing.json`。HAP SHA256 为 `29ecdbdecf506d277762fc36ced88686fef179a3be8cf8e8b0875fb3241785ec`，包内 arm64-v8a/libgrsm1.so SHA256 为 `6bc31490472d99ca369899a2f3d6b261b9f6f2932ee1d1827c5084a606ddb49b`。签包位于 `.codex/local-test/t257-sign/T475-handoff/T475-handoff-signed.hap`。凭据由已有签包脚本在内存读取。

离线复跑从仓库根目录执行，生成目录及文件自测目录须为新路径。

```powershell
node test/t475/generate.cjs .codex/local-test/t475-replay-new
cmd /c test\t475\build_store.cmd .codex/local-test/t475-store-new
cmd /c test\t475\build_gl.cmd .codex/local-test/t475-gl-new
node test/t475/build_app.cjs .codex/local-test/t475-replay-new .codex/local-test/t475-app-new
node test/t475/build_app.cjs .codex/local-test/t475-replay-new .codex/local-test/t475-m1-new m1
```

在新生成工程中使用 DevEco hvigorw `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`，将 JAVA_HOME 指到现有 DevEco jbr，并将其 bin 加入该进程 PATH。签包沿用 `python test/t257/sign.py <独立输出名> <生成目录>`。`collect.py` 固定核对本次交付路径，校验六 program 的自动收据与 binary header/长度/hash、冷热来源、热 compile=0、原创建诊断、最终源码/生成 manifest 一致性和签包内 ELF。

独立真机接续按设备队列串行执行，接手角色 tester/effector。先核 signing.json 中 HAP/ELF SHA、验签及 Want 的 grsm1/gpusha/gpuelfsha。清数据后首次启动进入 M2 空场，读取私有目录自动 JSONL/binary，核三 program 编译、导出与实际 PERSISTED。保留数据强停后重启，核同 key 三次 BINARY_LOAD、compile=0、反射/prepare/warmup、READY/首 root/Draw 的真实时间。再以新空场重建 context 检查全新资源与同环境 binary 复用。重签包保留数据时核 key 不变并更新来源声明。清数据/卸载作为独立冷缓存条件登记。

负控按受控改代码/环境、坏 hash、截断与未知格式，核拒绝和同 GLSL 一次编译。记录实际 binaryFormats、能力门、恢复/lease、真实 GAP/250ms、光学与交互。所有前景背景继续走 XComponent+shader，失败沿既有 FAULT 或标 STALE 的旧有效帧路径。真机设备支持、首次失败修复、最终效果与启动收益均尚待独立验证，离线 ANGLE 证据仅证明本次非机执行结果。若真机格式数为零或 load 仍慢，携原始能力/日志/时长回 architect 归因。
