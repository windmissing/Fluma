# T624：大 shader 构建挂起归因与有界重建

> 2026-11-19 a1/长链接命令归因见`architecture-T655-v1.md`：已使用对象rsp，主要膨胀是SDK链接标志缓存重复62/63组；先tester独立新输出与native argv。T629因果未证，单次1–2s编译不复现不等排除所有compiler问题，SPF旧树与DM主树标本须分名，不阻塞独立正确G9rx包。

2026-11-19；architect；不使用真机。结论：已证构建未产出，未证140KB R-string为根因；先由tester修独立测试构建/等待证据，确证产品生成单元缺陷再交developer。保留T541/T564真实harness，不使用旧frozen或缓存绿替代。

## 能证明与不能证明

输入 test-report-T541-differential-toolchain-v2.md、`.codex/local-test/t564-stage-tree/build.log`、实际CMake/compile_commands、g9dm/g9spf生成链。报告clang CPU约0.05s/38min、随后lld约0.03s/60.5min，只证明观测到不推进；没有线程栈/等待对象/完整link命令/响应文件快照。日志FAILED 4294967295和截断命令不足以判定编译器内部死锁，可能涉及外部终止、父子进程/pipe/文件等待，均未证明。

实际compile_commands中g9spf/kernel_assets.cpp命令830字符（SHA见本批evidence），包含-O0 -g -fno-limit-debug-info -std=gnu++17 -fPIC -fno-fast-math -ffp-contract=off；编译对象属于grs_foundation。kernel_assets.cpp本身小，包含transport/candidate/生成shader头；应核完整预处理TU，而不是只计R-string文件大小。没有可靠原对象/最终ELF指纹就不能宣“新源编过但链接没过”。

## tester 构建处置合同

每次建立唯一全新输出目录，不复用旧T549 object/archive；保留现有源/日志。目录manifest记录源harness、所有生成输入及产物、compile_commands、SDK编译器/ld文件hash与版本、CMake选项、response文件原字节及SHA、工作目录、环境必要非秘密变量、完整argv、输出.o/.a/ELF hash及ELF BuildId。缺一对象来源不能按mtime认新源。冻结生成输入后生成一次；禁止构建中重写shader头。

分阶段最小复现：①同argv预处理并保预处理输出SHA/字节数；②同TU compile-only；③只用本次新对象link-only；④原harness目标。先一次≤120s/阶段、总≤900s墙钟（含dump与清理），超时先留本次PID/PPID/启动时刻、CPU增量、线程等待栈/wait chain、open响应文件/pipe/输出句柄和原始stderr/exit，再仅结束本次拥有的进程树。不能全局杀clang/lld或删除他人输出；无法取栈要记录权限/原始报错。CPU为零不自动判deadlock；核父进程是否等待子进程/输出读者、响应文件是否已写完、文件访问是否异常。不要关闭安全软件或换未知SDK作为默认试法。

先以原flags重现，必要的去-g/去include/仅常量TU作为具名差分实验，不能把差分构建冒最终同配置绿。新增测试fixture由tester维护，不把已有签名/部署/采证退developer。只有最小复现定位产品生成组织/编译接口时，携完整输入/命令/栈/输出身份回architect，按下段工程策略交developer。

## 条件性产品工程规避

若证实特定TU超大调试信息/解析路径触发问题，可将shader纯资产移到独立TU，避免transport等非必要头；使用独立片段数组/多TU，冷创建时按固定顺序有界拼接一次，最终传入GL的字节数、换行和SHA必须与原真实shader一致。相邻C++字符串常会被前端合并，单把R-string切成相邻literal不保证解决根因；不能直接宣已根治。新冷拼接内存纳入现有创建信用/生命周期，根热路径不concat、不新增GPU dispatch/CPU仿真。

可替换已知兼容SDK补丁版本的前提是正式支持的ABI/平台兼容，固定新toolchain身份并重建全对象；不是随意换编译器。实际数值源/FP flags不得为过编译削弱，保量化/确定性合同。只有新源、新对象、新ELF闭环才恢复T541/T569执行；T600仅当命令/TU/等待指纹同一缺陷才复用结论，否则保独立问题，不额外设全队列准备门。

本次只读检查命令/源及证据，不执行复现/产品构建，不声称任何规避已验证。复盘：下一次首个有限超时就采等待证据，避免38+60分钟无信息重复；墙钟/CPU/退出三者独立保真。

续作prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。具体阻断写未完成、原错/复现/证据/尝试、不能继续原因、输入/接手角色。开发不使用真机且只开发编译；tester独立构建/测试，设备另标真机串行。GPU驻留优先、CPU停投、XComponent+shader唯一上屏，失败FAULT/显式STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12，旧湿M1暂停。
