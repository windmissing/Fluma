# T228 GPU M1 沙水 DEMO V4 开发交付

2026-10-30，developer；原卡 `6ae1c075e4b0f11bdf2e808b`。不使用真机。消费 T221/T222 体验/光学/生命周期合同，以 T237/T238 GPU M1 为当前数值/事件/scene 权威，T281 `c127322` 为实际接口前置。旧 T227 CPU-only、F64 profile10/waterProof64 不作本实现生产通道。

## 实施范围

`GpuResidentM1.ets` 从单次实验发射页接成持续沙水页面，入口仍为独立 Ability Want `grsm1=true`，不修改生产默认。空容器由 FullM1 创建；沙/水源切换需新 DOWN，主 pointer 单占，MOVE 更新实际模型坐标，UP/CANCEL/停止只关闭新源。按同一个 UI frame 原始纳秒钟先作差，首 tick 和 dt0 无源量，合法 delta≤33333333ns 才产生 `rate*delta/1e9`；不截窗补量、不积压重试。触点不钳到床/水面，非法/越盒输入拒绝，由 GPU SOURCE 决定 flight/direct/bed rejection。速率8units/s 是待定标输入候选，不是水/沙速度律。

`GpuM1DemoController.ets` 仅管理协议、手势和冷配置。复用一个128B command及视图，revision2/feature3 的 run/input/geometry/config 与 source-control U64 身份按 READY 快照绑定。新增 `GRS_M1_DEMO_UI_V4` 显式拒绝旧 UI reader；物理库存、W/J/侧扩散/湿沙/床和最终接受角继续来自 GPU。排队回执 `ENQUEUED_NOT_ACK`、源终态准入和 scene CURRENT 分开。W/J/接受角/物理提交/资源数字放在可展开诊断中。

native `UiState` 和 `Run::DiscardSources/Suspend/Resume` 提供非终止输入边界；不拿会终止 FullM1Transport 的 `Stop()` 处理普通抬手。UI 源边界变更立即清桥队列；GL owner 清掉未提交源，已提交 SOURCE 只排空一次，成功库存继续存在。源边界检查与真正 Pump/提交同锁，避免 UP/切源恰在检查后提交前的漏入。source-control 使用 gesture U64 域，与 GPU inputGeneration 不混用；关闭边界后的旧 command 拒绝。

后台/页面隐藏及250ms传感器过期会关闭输入、暂停未来 quanta，排空一个既有 root 后保留已成功 W/J/沙与 native owner，状态为完整旧 token 的 STALE。恢复重置时钟首 tick，不补墙钟/不恢复旧 pointer。传感器仅发送观测目标、freshness和 shake 诊断；最终 jAccepted 仍由 GPU 可行角核产生，未移植旧 CPU GravityPort 平滑。显式离线手动模式单独标明。真实 sensor adapter 沿既有采样平滑；摇晃不新增扬沙/水浪力。

reset、容量配置及光学应用均明确创建新空 run。optics16 采用严格 F32 typed array及域检查，冷绑定 scene token；不热改当前库存。提供 sigma、surface gain、像素带宽入口，候选保持 PENDING_CALIBRATION。真实 native window/context/geometry 丢失后明确 OWNER_STATE_LOST，需要用户新空场，不从 completion 假恢复 pool或悄悄冒充续跑。仅前后台且 window仍有效时保留 owner。背景、床、沙、水体、水面、J、前景均沿原 accepted publication 新 XComponent+shader；无 Canvas、CPU 仿真/全池上传回退，未更动物理或 scene shader。

## 非机验证与边界

机器证据：`test/reports/t228/delivery.json`，原输出 `controller.json/native-units.txt/app.txt/m0-bridge.txt/ready.txt/clock.txt`。最终构建与签名指纹也在该目录。

- 实际 ArkTS controller 经 TypeScript 编译运行：22/0；覆盖首窗/dt0、量/原字身份、MOVE/多指、切源/停源、间隙/倒退/异常时钟、背压不补量、geometry/镜像、run/epoch、非法 reader/身份、后台恢复、sensor/manual隔离、冷光学/容量和ID耗尽。
- 实际 native Run/Fixed120Clock/UI gate（transport明确桩）：10/0；覆盖停源前后边界、一个在飞 root、暂停丢弃后续 quanta、首 tick0恢复、freshness/角目标限域、完成摘要字段、冷光学、资源峰值。桩的 terminal Stop 设置断言，不允许普通 UI 误调用。
- 生产 app_bridge+实际 FullM1/Run/scene 在真实 ANGLE SwiftShader GLES3.1：20/0；近面水出生 J→停源→ARRIVE W→下一 quantum WATER_EDGE，随后沙源/湿介质落床，W总量不变；同 run 后台60秒墙钟和传感器stale/恢复；源不补发、错源边界拒绝；真实窗口失效报 OWNER_STATE_LOST；显式冷光学reset新空run、完整 CURRENT。PC pbuffer 替代原生窗口、脚本时钟替代真实调度，绝不作为 HarmonyOS/真机端到端证据。
- 原 M0 bridge16/0、READY18/0、固定时钟14/0；新旧通道隔离。场景核未改，T281四档196/0及像素20/0为历史对应scene证据，不冒称本卡重跑全部矩阵。
- hvigor `default@PackageHap` BUILD SUCCESSFUL；签名与验签 PASS；未安装设备。

## 资源账与交接

status/delivery 记录 FullM1 实际 GL buffer allocatedBytes、完整双帧 `8*w*h` 与二者已知峰值；不是仅引用设计 payload。PC host 另用 GetProcessMemoryInfo 记录 working set、peak working set/private usage，包含软件 Vulkan/JIT，不能当设备 GPU/RSS。controller固定 typed payload192B；JS对象/视图、状态JSON/诊断字符串、driver隐式分配与旧context驱动驻留重叠为 NOT_MEASURED，不伪记零。原生 context 串行退休后才建立新场。所有失败/首次构建资源同样保留真实日志，不提高阈值/删审计/replay。

签包路径、SHA256 及当前构建 fingerprint 以 `test/reports/t228/sign.json` 为准。复跑：`node test/run_t228_demo_units.cjs`；`build_t228_units.py`→本地 build-units.cmd；沿 T281 host环境生成头/构建后 `build_t228_app.py`→本地 build-app.cmd→app.exe；`archive_t228.py`校验结果、构建及签包。只开发自测，T229/T300/T301独立/设备/观感结论另卡，不加作本卡完成门。
