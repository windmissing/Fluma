# T34 合成开发交付（2026-10-25）

依据：architecture-P013-p1-g0-composition-revision-v1.md。范围为生产可复用profile/层级适配与独立G0 probe；不宣称G0真机通过，也不实现被门序隔离的P1完整材质。

## 实现

- SandCanvas同Stack声明顺序及zIndex明确为背景0→TEXTURE10→装饰Canvas前景20→控件30；XComponent透明背景、opacity=1、None命中，前景None，手势仍由唯一公共父层拥有。生产默认Canvas/canonical旧臂不变，未引入PROBE-ONLY生产门。
- P012Renderer新增prepareOnce/background/foreground与legacy包装。一次adapter.update及运动缓存更新；背景负责水/雾/现有沙材质，前景负责真实粒子或tracer、高光、界面雾和边框。P0诊断仍是原DIAG_GRAY_V1；失败后恢复完整Canvas，不留下仅背景的半臂。准备token与fieldEpoch/frameSerial/physicsCommitId核对，未重采物理。
- 独立P013CompositionProfile及receipt；controller.requestComposition/submitPreparedFrame/disableComposition，native.requestComposition/queryCompositionProfile/captureProbe。明确profile identity，不使用any或JSON桥接兜底。默认probeOnly=false，完整P1材质尚未实现时返回P1_MATERIAL_NOT_IMPLEMENTED并回Canvas，不能拿色块冒充产品材质。
- EglContext显式sRGB profile与旧profile隔离：完整扩展token/EGL1.5检查、window surface属性请求sRGB、实际surface查询、RGBA8及无MSAA配置、default draw/read framebuffer=0与GL_BACK编码GL_SRGB查询、支持EXT_sRGB_write_control才enable与核实。未知/失败为COLORSPACE_UNSUPPORTED(304)，没有手动gamma兜底。销毁/resize/后台重建不复用旧回执；release回到P0 profile。
- 新composition_probe shader仅输出线性0/0.0031308/0.18/0.5/1条带，二值mask沙内alpha1/沙外RGBA0，禁混合/深度/覆盖采样。硬件OETF只由sRGB attachment执行，未改旧shader、palette、坡度核或七算法身份。
- 仅明确probe profile进行读回：swap前default GL_BACK、独立linear RGBA8 FBO、sRGB FBO各6点RGBA；分开记录，不能互相替代。captureProbe只返回已经draw+swap成功、同session/frame/snapshot/surface/context token的缓存，点坐标也须完全一致；不读交换后的不确定buffer，不制造/推进物理快照。生产P0无每帧readback。记录pack alignment、坐标方向、viewport、API版本/扩展/错误、shader FNV1A64及三组原始RGBA；物理hash在纯probe中明确UNPROVEN_PROBE_HAS_NO_PHYSICS。CPU OETF期望仅作可复算对照，不冻结容差。
- Probe页面改为正确bg→X→fg→controls顺序，棋盘背景、Canvas已编码参考条、跨沙区洋红前景标记；want p013g0=1,p013g0lib=renderer,p013g0profile=texture进入新profile，surface为旧P0修正顺序对照。失败移除XComponent、保留Canvas；旧hello对照仍可选。色彩/alpha/前景的屏幕证据归T35。
- Probe telemetry使用G0_COLOR_PROBE_ON/OFF、G0_LINEAR_COLOR_BLOCKS_V1、独立compositionRevision/source hash；不沿用P0 shader hash伪报。额外probe program/VAO计入现有资源统计，正常release在current时删除，生命周期失效按旧规则销毁context并记录abandon。

## 验证与证据

- ArkTS+arm64 native：hvigor default@CompileArkTS default@BuildNativeWithNinja BUILD SUCCESSFUL，零错误；保留既有SDK/旧代码警告。test/reports/t34/build.txt。
- Node composition 5/5：legacy draw命令与分层顺序等价、adapter/motion仅更新一次、线性/单次OETF/重复编码反例、unsupported完整Canvas回退、坏token不发帧、默认不启probe。test/reports/t34/composition.json。
- native host：实际EglContext与实际napi_bridge.cpp，profile有效/错误字段/未知扩展/alpha不足/surface编码/attachment编码/write-control拒绝、token/点/epoch拒绝、重复请求不换代、release恢复P0、旧帧桥与8种生命周期故障回归均通过。host仅NAPI/EGL/GPU边界桩，probe shader的GPU执行和真实读回仍未证明。test/reports/t34/native-host.txt。
- 旧通道：P012 renderer82/82、tracer9/9、D3 8/8、P0 20/20；9个P011通道338/338。原冻结断言未放宽。详见test/reports/t34/各JSON及p011-regression-summary.json。T20 9/9与T36 8/8证据在各自reports目录。

## 留给T35/T31

T35采集API/attachment、三组swap前readback与同内容无损截图三层证据，日志p013_g0_composition/p013_g0_color_readback；同token抽样查询，量化容差仍归tester，当前标MEASURED_NOT_CALIBRATED。目标设备是否支持TEXTURE+sRGB及透明、前景、输入/动态错帧、性能、资源长稳均未宣称PASS。未使用真机、不安装HAP；T31负责真实粒子/速率主观端到端。T37新GLES数据通路不在此卡实施，完整P1材质/G1门序不因开发提交自动解冻。

提交仅选T34代码/自测/证据；CMake只带renderer probe source，保留hello等其他任务暂存。manager依用户最新职责只登记完成及裁决，无额外验收前置。

Host入口已去除重复独立编译：bridge_host_test本身include并执行同一lifecycle_test_main，仍逐条运行全部生命周期断言。最终统一host与完整工作区编译通过。T37(6089f98)已在实施中更新T20产品出口；T34交本提交接口后由本会话继续T20基础GLES适配，旧Canvas完成回执已撤回。
