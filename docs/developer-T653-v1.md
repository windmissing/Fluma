# T653 开发交付 v1

2026-11-19。按 T650/T619 实现本卡完整范围。结论 **COMPILE_PASS_NOT_PRODUCT_PASS**；不使用真机，不运行产品/单测/物理/reader，不签名、不安装、不部署。

## 初始化原件与实际接线

`m3b/init_diagnostics.h/.cpp` 固定 payload16384B：control1024、attempt4×512、stage32×256、log2×2048、reserved1024。Stage互斥SHA/Pulse32B位于208，known240；容器offsetof为1024/3072/11264/15360，C++ static_assert 与实际 AArch64 ELF ABI一致。实际 Store16424B=payload16384+mutex40；program watch、页复制2048、base64/NAPI短暂返回空间另计，不拿初始化信用占用cold broker或伪称栈/驱动零开销。

- requestTicket为Store原生单调序号，绑定boot、实际配置API的InputCall attempt、成功后的diagGeneration、Config32原件和全128B SHA、源manifest及原cancel/epoch/window。后续session/cold API attempt不替换配置ticket。
- snapshotRevision覆盖写入；不wrap，耗尽具名 DIAGNOSTIC_SEQ_EXHAUSTED/INCOMPLETE。stage保最初16+最近16并计lost，attempt四槽轮换、首失败/真实最后进展、日志reported/copied/截断分别记录；过期槽返回STALE_TICKET。
- 真实GL owner Configure、20个实际program角色、Scene程序/targets、同ticket+epoch/window发布、session、输入offer、Pulse同步ControlResult及后续owner真正读取accepted pulse序列均已接。普通shader SHA与Pulse四U64互斥，Pulse原始scope单独保留；ownerTid来自实际owner绑定，不用UI线程Tid冒owner。
- program日志只接既有program-cache/graphics receipt原字节，不额外查询日志、错误或状态；未采样GL/EGL/API返回保持-1和known关闭。布尔与枚举原返回各保原值，未执行的创建/场景切点不虚构成功。没有新增GL dispatch/map/fence/glFinish/glGetError/热IO。
- `m5InitDiagnostics(ticket:string,page:number)`直接短锁拷贝独立Store；编码/NAPI在诊断锁外，不排GL owner、不获取App/controller锁。8×2048B，ASCII/base64回包小于4096B；每页ticket/boot/revision/offset/采样时间独立。混revision不会冒完整档。
- 新配置/窗口变代/停用撤旧ticket；只有当前ticket与原epoch/window仍一致且实际Configure/Scene成功才发布controller及WAIT_PAGE_PULSE。保原GAP/Fresh/物理时钟，不套G13时钟，不产生假pulse/Ready。

## 页面、生成与FULL

新增 `GpuM5Init.ets`：有限450次/15秒初始化观察、同ticket session及原生终态/首失败、取消代检查；取得完整非零scope前不发输入。超时明确 native state unknown。真实 FRAME_ATTACH/首次回调保在客户端回执，不伪作native阶段；退出保存原始分页并标注revision是否一致。未触碰M3/G9公共waitSession行为。

已更新规范bridge inc、实际T517 App overlay/NAPI注册及d.ts、新helper/页面、CMake依赖。`tools/v5/prepare_init_build.cjs`要求全新目录，消费实际生成链，并保留T501生成的artifact-store扩展，只替换本卡需要的program observer/factory；全native/ETS文件生成manifest，配置阶段校验任何源失配即fatal。源SHA header排除自身循环，实际App/ABC入本次收据；不是只复制m3b CPP或借旧App。

明确移除的是遗留word15==0门：Config32严格0/1、完整参数摘要/Valid保留；research request的FULL选择同步到Config32并重算摘要，页面有ROOT/FULL研究入口。实际T619 fullBranchTrace、GPU arena预约/Arm、ROOT入口cursor64、SOURCE+Q0..Q3、完整4096B capsule（full标志11）、2920B token、Meta2/双slot/共享broker、真实释放及有界reader保持同合同；不以删除门替代FULL实现、不把未封样本报COMPLETE。

T517生成器同步T614机器表原字节SHA，生成h/glsl仅身份stamp变化；revision2/usedMask/布局未变。本卡**不声称该FULL flag1遗留门是此前flag0 NOT_READY根因**；本次观测供原T594取真实首失败。

## 编译与原件

完整新生成 App 的native/ArkTS/unsigned HAP构建通过，34/34执行63.418秒；最后原件修订后增量复编译 **22 s 317 ms [39m，零错误**。ABI从实际包内ELF静态提取：`[1, 16384, 256, 512, 2048, 16424, 40, 1024, 3072, 11264, 15360]`。没有加载/运行库。

- unsigned HAP：`.codex\local-test\t653-build-final\entry\build\default\outputs\default\entry-default-unsigned.hap`，SHA `60e28a676acb5f6574c28b049eea79c7b88d48d64b888a2aa6233b23bb58ee9c`。
- 包内 `libgrsm1.so`：3201504B，SHA `35ac238c0eb000010736eaf5d5e70917e8e28a7884d460d5832016429669b7f0`，BuildId `7f31c1de559a8e10a3ee7ba7dcc8837fb732a51b`。
- 未剥离调试ELF：SHA `815cc9e694f4e647e46ba87e16adcf452db7aa1cfe894ac2e7a27615659dc946`；同BuildId/ABI，不能拿这个SHA替代包内ELF。
- `modules.abc`：2990464B，SHA `4595e19217df3a18c269ca6f4dbdf3e8a5259e5aa6faf5b4164798bb628d37f6`。
- 编译源manifest SHA：`3e93a1c35f75e1b7dbedfce130328903567f2dbe0d2595b364ee4efc1f3aa511`，包含实际生成App/native/ETS，CMake硬校验；source header不自哈希。

原始日志、ABI/正式输入/全部实际源指纹见 `test/reports/t653/compile-receipt.json`、`generated-app-source-manifest.json`、`hvigor-build.log`。native中loadedModuleSHA/processStart仍明确UNKNOWN、known关闭；编译包SHA只列编译原件，不冒运行进程已加载证据。

本卡可交原T594续取同ticket配置/首失败及真实ROOT/FULL四档；开发不加设备门，产品/物理/捕获及reader运行结果仍由tester/effector负责。
