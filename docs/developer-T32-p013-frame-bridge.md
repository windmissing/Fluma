# T32 帧桥与绑定时序开发交付

日期：2026-10-25。原卡：6add77dfe4b03273ad6591a2。范围为开发、工作区编译和离线自测；真机恢复验证交 T35。

## 实现与核对

- 生产 P013RenderController 原先在字段初始化中读取 native default export，早于 SandCanvas 的 XComponent.onLoad。现改为可空引用，在 onLoad 读取，release 清空；构造、加载前控制/证据导出均不访问 native。静态 import 保留，未声称延后模块加载。WAIT_WINDOW 时不 init/draw，后续窗口就绪再初始化。
- ParseFrame 的 19 个 number、2 个 boolean，ParseLight 的 10 个 number，与 P013FrameInput/P013LightParams 及 G0 探针逐项相符。真实版本为 E2_APPROX_P0；回执中的 E2_APROX_P0 拼写推测不成立。
- 保持 Float32Array、512 元素、非 detached、buffer 范围/偏移/指针校验；保持 Validate、身份优先级、坏帧消费 ID、状态码及 GPU 前拒绝规则。没有 JSON 热路径兜底，也没有放宽校验。
- drawFrame 结果和既有 telemetry.errorStage 暴露具体 FRAME_FIELD_* / LIGHT_FIELD_*、版本、TDATA_* 拒因；合法解析后数值校验仍报 FRAME_VALIDATE/DIAG_RANGE。解析失败时保留首个具体字段拒因，避免用未解析光照默认值覆盖成 DIAG_RANGE。
- P013 基础 renderer/controller/adapter 及 host harness 在开工目录中尚未入库；本提交纳入运行与测试所需依赖、CMake renderer target。未修改 shader 配方、叠层/色彩方案；未纳入 hello/SandCanvas 或物理任务的他人暂存修改。

## 1742 帧结论边界

历史证据为 code100、FRAME_REJECTED、hash0/upload0，说明在上传前被拒；原始日志没有字段/typed-array 子拒因，不能从该记录唯一反推哪个 NAPI 读取失败。离线实际 bridge 接受合法包与非零偏移 Float32Array；G0 几何（1280×2598，box 352/649/576/779）、96/172/0/0 平直剖面通过生产 Validate。host 使用 NAPI/EGL/GPU 边界桩，不能替代 ArkVM/设备验证，不能宣称 1742 帧真机故障已经消失。已修复可复现的生产绑定时序缺陷，并为 T35 明确缺失的拒因观测。

## 验证

- RED：新增 default-export getter 计数测试，旧控制器构造即读取，1 != 0，复现失败。
- GREEN：node test/run_p013_p0_units.cjs，20/20；含加载前无 native 访问、onLoad 绑定、WAIT_WINDOW 重试及旧 controller/adapter 回归。证据 test/reports/t32/controller.json。
- PowerShell test/run_p013_host.ps1：生命周期与实际生产 napi_bridge.cpp 回归全通过；新增 frame 数值字段（session/frame 身份保留原优先级）、2 boolean、全部 light 字段、版本缺失/错拼、typed-array length/type/detached/buffer-range 拒因断言及 G0 有效包校验。既有 NaN/旧帧/错线程/无半上传/OFF/swap/ring/release 回归通过。证据 test/reports/t32/native-host.log。
- hvigorw --mode module -p product=default -p module=entry@default default@CompileArkTS default@BuildNativeWithNinja：ArkTS 与 arm64 native 工作区编译通过，零错误，保留 SDK/旧代码警告；证据 test/reports/t32/build.log。未进行签名安装或真机端到端。

## T35 接力

复跑 G0 并保存 drawFrame.errorStage/telemetry.errorStage、code、T_dataHash、uploadedFrameId、swapSucceeded；失败时按具体阶段回报。叠层、alpha、sRGB 仍按 T33/T34/T35 独立完成；本开发交付不解冻 G1，不代表 manager 验收。
