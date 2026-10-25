# T43 设备采集链装配开发交付

日期：2026-10-25。卡号6addc1e3e4b01278327e57bb。按任务要求不使用真机；本报告仅为开发、编译、离线装配/打包核对，不宣布endpoint/peak协议生效或设备GLES通过。后续清单及滴答同名清单未修改，交manager维护。

## 缺口与最小实现

1. **首子步采样身份**：新增P011DeviceRunAssembly薄装配函数，复用P011EndpointCaptureSession与P011EndpointStartupClock（systemDateTime.getUptime(STARTUP,false)，真实单调ms、设备启动减run起点，resolution未测仍null）。页面initialize之后、初始shake及首step之前创建session，session调用既有setEndpointSampling及capture mode。每次实际接受命令后/step前从field.endpointCurrentPhaseIdentity取得真实commandSequence/commandPhaseId/segmentId绑定；occurrence字符串只组合这些实际身份，不猜phaseD、不制造sequence/tick。reset重建时新run/clock重新绑定。capture期间schedule start/tick走同一session clock；非capture旧臂不改。
2. **worker打包与路径**：源移至ets/workers/P011NaturalObservationWorker.ets，build-profile.sourceOption.workers与ThreadWorker('entry/ets/workers/P011NaturalObservationWorker.ets')一致。createStore等待worker真实ready后才公布路径；错误不假报ready。
   当前工程是ESModule构建，HAP仅有ets/modules.abc，不能声称额外生成独立P011NaturalObservationWorker.abc。已从实际unsigned HAP抽取modules.abc并用API26 ark_disasm反汇编：存在独立worker record、func_main_0及writeComplete函数，旧physics worker record消失。test/check_p011_worker_package.py可复跑，manifest含HAP/modules/source SHA256，workerPackaging=ES_MODULE_RECORD_IN_MODULES_ABC、standaloneWorkerAbc=false。修复的是实际模块入口及打包可达性；设备加载仍由T44证明。
3. **完整raw侧流**：同worker写两份NDJSON：原逐子步ledger，以及同run成功准入原始-admissions.ndjson。setupNaturalObservation在首次准入前setExternalAdmissionEventSink，ready前先缓存原始记录，ready后全量转发；不从epoch/metric重建raw，不改变原ledger schema或物理计数。sidecar保留runId/admissionTransactionId/sourceId/sequence/inputTime/admittedH/L/cumulativeCount，跨文件靠原身份关联。
   worker在后台UTF8编码并循环处理短写，逐行fsync；0/异常写入标错而不截断JSON。close等队列处理完成并校验两流写入条数与发送条数，成功后P011_OBS_STORE_CLOSED标COMPLETE；失配不报完整。ledger写入失败只使证据invalid，不向物理step抛错；准入sink同步失败沿既有T39 exportFailures/INCOMPLETE披露。页面离开detach probe/sink、关闭旧store，陈旧异步factory失败不拆掉新run。
4. **同run停摇**：已有onNewWant→liquidWantCommand→页面通道仍是唯一入口。新增shake-only envelope分支，单发shake参数不经过默认rendermode/testmode/seed/invert覆盖；页面先识别该分支，仅调既有面板setter→既有field.setShakeInput。初始/更新/停止均沿该单一物理输入；采样session取真实SHAKE_OFF事件effectiveElapsedMs，输出既有shakeOffBoundary（off+8000、末窗off+7000..8000），不重置run/初值、不自动启停积分。显式摇晃控制后，thick预设的原隐式tilt→shake派生不再覆盖已接受停止命令；未显式输入时保留旧行为。
5. **真实双场入口**：显式冷启动p011particle=1，调用既有defaultP011ParticleParams→setParticleRoute→setNaturalObservationRouteId，默认off。S3/S4/advection/显式其他schedule route冲突先拒绝并报ROUTE_CONFLICT，不覆盖其他路线；仅倾角命令且未要求切route时不意外卸掉已武装C。schema4/真实池仍由T20/M2产出，不重做仿真、shader、镜像或守恒预算。

## T44 操作入口（设备执行归effector）

- 纯采样旧臂可沿wssnapshot=true（该旧证据开关按既有规则强制Canvas）。
- 要同时观察基础GLES双场并采样，冷启动参数至少rendermode=4、testmode=true、p011particle=1、p011capture=1，wssnapshot=false；shake=1可启动既有摇晃输入。p011capture是独立采集opt-in，不改renderer canonical开关。
- 运行中通过同Ability onNewWant单发shake=0，不能用重新冷启动构造“同run静置”。看WS_EVENT shake_effective，边界内含相同run的SHAKE_OFF实际elapsed和8s/末1s窗。保持同run运行至少8s；phase/clock/key与raw ledger按记录读，不从截图猜测。
- P011_OBS_STORE只有worker ready后才输出，读取filePath与admissionFilePath；两者均为应用cache下NDJSON，可在运行中取已fsync的前缀。正式结束宜离开页面并等P011_OBS_STORE_CLOSED完整计数回执，再拉取/核对；直接强杀而无closed不能冒称完整。
- worker加载、缓存路径权限、同run停摇、实际raw完整性、真实粒子/GLES/帧时仍须T44设备验证。已知满池动态code5及G0平台后续任务独立处理，不由本卡放宽判定。

## 验证

- hvigor default@CompileArkTS default@BuildNativeWithNinja default@PackageHap：BUILD SUCCESSFUL、零错误，保留旧SDK/ArkTS警告。未签名/安装/连接设备。日志test/reports/t43/build.txt。
- 装配11/11：首POST_COMMIT真实key/clock/ordinal/elapsed、真实phase、同run停摇后480子步保持初值/源epoch、shake-only零值/无效值及实际Ability含ohos.*平台元数据入口、提取实际页面consumer证明不进reset/mode分支、冷启动装配顺序、真实schema4/路由冲突、UTF8长行/短写、真实成功准入侧流独立回算、ready/0写/关闭丢数拒绝及写错不冻结物理。
- endpoint9/9、peak8/8、D3 8/8、dual packet9/9、gesture clock8/8、九物理通道338/338。D3旧静态断言限定“Want分支直接出现field.setShakeInput”已随薄装配路径更新为逐跳检查，并新增运行实际consumer/实际field的测试；未改数值判线/协议。
- worker-package检查PASS，真实HAP module record与函数入口存在；不是host mock string检查，更不是设备启动PASS。见test/reports/t43/worker-package.json。

本次源代码改动只有页面/Ability装配、复用helper、store/worker和配置/测试。WaterSandField与M1/M2数学、冻结阈值、endpoint/peak协议正文、shader均未改。报告完成后原卡回执与关闭，T44的开发前置可解除；排机与清单由manager处理。

## 修改文件与用途

| 文件 | 用途 |
|---|---|
| entry/src/main/ets/entryability/EntryAbility.ets | 显式capture/particle冷启动请求、shake-only现有Want信封 |
| entry/src/main/ets/pages/SandCanvas.ets | reset后首步前采样装配、真实phase、raw sink、同run停摇、C入口 |
| entry/src/main/ets/physics/P011DeviceRunAssembly.ets | 复用现有session/clock/route/input的薄装配，Node可测 |
| entry/src/main/ets/physics/LiquidCAWant.ets | 同一命令类型的可选shakeOnly标识 |
| entry/src/main/ets/physics/P011NaturalObservationStore.ets | 等ready、两流发送/关闭计数、错误披露 |
| entry/src/main/ets/workers/P011NaturalObservationWorker.ets | 从physics迁移；后台完整UTF8写入、双文件/fsync |
| entry/build-profile.json5 | worker源目录声明与ThreadWorker入口一致 |
| test/run_p011_device_capture_units.cjs、test/check_p011_worker_package.py | 实际装配/consumer/worker及HAP字节码入口离线核对 |
| test/run_p012_d3_wiring_units.cjs | 保留工作区既有追加用例并适配T43委托调用，数值判线未动 |
| test/reports/t43/* | 编译/打包哈希与各通道离线证据 |
