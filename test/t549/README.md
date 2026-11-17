# T549 GPU 执行设施

开发只生成/编译，不执行。`node test/t549/generate.cjs .codex/local-test/<fresh>` 沿T529原生产链构建，CMake额外输出arm64原生 `g9_differential`。该程序使用实际EGL ES3 pbuffer及生产Transport/Pump/CompletionGate/CaptureSpan；无mock、CPU物理实现、预期oracle或渲染产品回退。无可用真实EGL即明确失败。pbuffer是隔离执行设施，无产品上屏/效果资格；原产品仍仅XComponent+shader。

tester在其独立受控GPU环境调用 `g9_differential INPUT_DIRECTORY NEW_OUTPUT_DIRECTORY`。开发本轮不安装/启动程序、不执行GPU。OHOS使用构建对应libc++/EGL/GLES动态依赖；可执行文件位于工程`entry/.cxx/default/default/debug/arm64-v8a/g9_differential`，不是HAP启动入口。所有输入按LE U32文件，明确拒长度/保留位；不引用或导入tester期望。

- config.u32：16word：K,g,muStart,muStop,band,entrainRate,shear,settleTicks,revision,research(1),synthetic(1),fullTrialTrace(0/1),后4word0。K192或512，参数合法性和参数SHA由生产helper处理。此设施运行原T529 V1，不能冒T548真实手势。
- identity.u32：18word，9个U64低高：run,owner,context,inputEpoch,pageSession,permission,window,config,surface。
- seed.u32：精确profile.stateBytes原件，由tester提供真实待测状态，不在设施内构造有利夹具；原生产seed/hash/初始化路径照旧。T529既有seed hash时序问题仍属产品事实，设施不修饰。
- commands.u32：最多4096行，每行192word；0..127是原始INPUT128，不自动重写nonce/capture/J/cap；128..129 nonce，130 plan，132 frame，134 control，136 geometry，138 kind，139 ordinal，140 arena，141..142 arenaGeneration，143=0，144..151 layoutSha，152..191=0。permit current bank/commit/quanta由实际上步typed提交更新。测试可逐文件精确操纵SOURCE与IMPULSE/重送/坏正文/角度/时刻；过期原值会被拒，不伪刷新capture。命令输入时序由tester定义，真实deadline开始于Begin前，绝不延长250ms。
- quota.u32：256word原始配额原件，仅随冷档作为外部输入声明保留，不冒设施经过生产clock授权的quota。若研究需要完整clock/Claim语义，另用原Controller/页面，不以本执行设施代替。

SOURCE仍12 dispatch，QUANTUM仍2024。执行原Fence/Pump与typed Accepted/Blocked；未获typed接受不提交。BLOCKED和错误仍写原completion/证据。每根写原request/input/completion、结果/dispatch/barrier/fence/deadline/GL/EGL；退休后经生产CaptureSpan输出完整raw cold（BASE/CURRENT/selected/public、FULL六trial arena，事件/PATH、来源、动量能量、去重胶囊、事务replay/控制summary原字均保留）。cold失败明确记录原GL/EGL，不能当有原件。读取/完整性与物理判据由tester实现，设施不输出物理PASS。错误退出非零，旧原件保留；无安全退休则明确RETIRE_TIMEOUT，不能继续复用GPU工作区。

资源收据分别列生产profile resident/full arena逻辑payload，实际renderer原件；这些不是driver/RSS测量。builder只保一份caller-span冷块；逐根输出后释放，ROOT/FULL按原profile容量，未在JS聚合。原始文件SHA可用独立`python test/t549/receipts.py <directory> <new-receipt.json>`流式生成；脚本仅hash文件，不执行物理/判定。开发交付报告绑定实际全部源SHA、pipeline/generated-header SHA、exe/ELF/HAP和编译日志。运行结果仍NOT_RUN，差分及预期由T541 tester独立执行。

最终generator在T529生成后按git e429ea7恢复完整g9spf目录，冻结生产源码，避免并行T548修改污染差分基线；最终签包以T549-frozen收据为准。仅做编译与签名，没有执行harness。
