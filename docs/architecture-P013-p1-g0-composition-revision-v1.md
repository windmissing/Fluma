# P-013 P1-G0 合成工程 revision v1（T33）

日期：2026-10-25（Asia/Shanghai）。责任：architect。任务：6add77dfe4b06ee35724fd94。
状态：**DESIGN_READY / PENDING_MANAGER_ACCEPTANCE / NOT_IMPLEMENTED / NOT_DEVICE_VALIDATED**。
本稿是 [P1工程rev2](architecture-P013-p1-material-composite-v1.md) 的独立工程追加，只替换其跨层合成与G0取证落实方式；不改签收的材质配方、坡度核、Rmat→Lmat→ToneMap顺序、七项算法版本或任何效果判线。不宣布G0通过或G1解冻；T34负责实现/编译/非真机自测，T35负责设备补证，T23由manager裁决。

## 1. 输入事实、证据边界与选型

依据：[T29回执](p013-p1g0-probe-receipt-v1.md)（commit 7f1ceb9）、P1工程rev2、已签收designer配方及T09 CPU fixture；只读核查 `P013P1G0Probe.ets`、`SandCanvas.ets`、`P013RenderController.ets`、native `egl_context/shader_pass/napi_bridge` 与本机API26 SDK声明。

T29记录1742次帧拒绝、当前叠层前景被遮挡、alpha尚未测通、默认profile下8bit截图色块接近，以及None/Transparent可收到触摸。这些是对应设备/构建/布局的证据。探针实际声明顺序为 `Canvas bg → Canvas fg → XComponent → 状态UI`；fg本就位于XComponent下方，未做“把fg放到XComponent之后并反转z序”的对照。故不能由该截图推出SURFACE在所有ArkUI前景之上，更不能把单设备结果泛化为平台永恒限制。原回执作为历史证据保留，不由本稿回写PASS/FAIL。

**本次唯一推荐：P1合成采用 `XComponentType.TEXTURE` + 明确的背景/沙体/前景层级；保留Canvas前景，继续XComponent+EGL/GLES shader。** 选择TEXTURE是为使用ArkUI纹理合成路径，避免把当前DSS独立surface的未知叠层行为当生产依赖；不是宣称此机已实测通过，也不改为Canvas材质乘法。

本地SDK `26.0.0/ets/component/enums.d.ts` 的TEXTURE声明支持EGL/OpenGLES、与原生内容合成，提示GPU合成可能比SURFACE耗电；`xcomponent.d.ts`允许TEXTURE的libraryname和native生命周期接口。此处类型是 `XComponentType.TEXTURE`，**不另造不存在的 `.renderType()` 配置**。官方指南亦区分两种上屏方式，见§8。编译支持不等于当前设备合成和色彩已验证。

| 路径 | 可执行结构/优点 | 代价与本次结论 |
|---|---|---|
| A 修正现有SURFACE声明/z序 | bg→SURFACE→fg→controls，显式zIndex；最小保留原native窗口接线 | 现回执未验证修正后结果；保留为T35诊断对照，不作为自动生产fallback或当前唯一保证 |
| B TEXTURE统一ArkUI层级 | bg→TEXTURE沙pass→fg→controls；同一NAPI/native window生命周期，明确透明背景 | **本次实施选型**；额外GPU合成/显存/时延必须实测，不预称更快；profile不支持则回Canvas |
| C 前景移入GLES | 在沙pass之后另做粒子/雾/边框pass或上传前景纹理 | 会新增纹理/alpha/噪声/显示路由契约，扩大算法及色彩验证范围；当前不实施，不暗迁移Rmat/Lmat之外的前景 |

若TEXTURE在目标设备无法满足本契约，输出明确阻断与证据交architect/manager；不得静默改用C或给shader加gamma。若未来C需要改变雾/alpha或材质作用次序，先交designer签收新算法revision。

## 2. 层级、输入与surface属性契约

P1逻辑结构（仅方案示意，非本轮代码）：

```text
Stack，同一几何rect/clip/visualInverted映射
  Canvas background     zIndex=0  水体底色/水体雾
  XComponent TEXTURE     zIndex=10 完整沙体Rmat→Lmat→ToneMap
  Canvas foreground     zIndex=20 tracer或真实粒子唯一显示路由→高光→interface雾→边框
  原ArkUI controls      zIndex=30 按钮/面板/提示
```

- 声明顺序与zIndex同时一致；不要只交换两个数字而保留互相嵌套的父层。前景Canvas必须真正在XComponent之后，三个视觉节点共享同一个Stack层级。XComponent背景显式透明，组件opacity固定1；不可用整体opacity制造每像素透明。
- XComponent与纯装饰fg显式 `.hitTestBehavior(HitTestMode.None)`；交互手势保留原唯一拥有者/公共父节点的既有route，不让多层分别接同一Pan。`Transparent`仅用于需要本层也响应的独立探针对照，不能生产中双发同一gestureSand。controls保留原命中，不设None。视觉z序和输入命中是两件事。
- TEXTURE仍以现有libraryname=`p013renderer`获取borrowed NativeWindow；保持surface create/change/destroy与context epoch校验、线程归属、解绑恢复契约。T32负责首次native绑定推迟到onLoad及FRAME_REJECTED修复；T34不重复重写桥接，不以JSON序列化丢掉typed array作兜底。
- 尺寸以actual native surface为准，记录Canvas logical px、surface native px及缩放；box/clip/invert只映射一次。保持已签收非等比缩放拒绝规则。不要依赖只对SURFACE有效的surface尺寸自定义接口来控制TEXTURE几何。
- 不改变生产默认模式或canonicalEvidence的Canvas强制旧臂。切换时整代更换组件/profile和surface/context epoch；旧帧/旧readback不能证明新profile。失败显示完整Canvas旧臂，不能只留下背景、黑surface或双画沙层。

## 3. Alpha唯一契约

### 3.1 GPU输出与系统合成分工

沿P1 rev2二值沙mask：沙内 `RGB=T(Rmat*Lmat)`（线性）、alpha=1；沙外及盒外 `RGBA=(0,0,0,0)`。EGL config需实际RGBA8，查询并记录选中config的ALPHA_SIZE≥8；XComponent和中间容器背景透明。每帧清屏alpha=0，禁用GL_BLEND/DEPTH/MSAA，沙pass覆盖写入；系统compositor负责沙surface与Canvas合成。

二值alpha的straight/premultiplied结果一致，**本版不引入半透明沙fragment**。边界不加平滑mask、不把alpha乘进Rmat、不对alpha做EOTF/OETF。未来fractional alpha必须另登预乘域/混合方程，不能在本版以抗锯齿为名暗加。Canvas fg原有雾/粒子alpha仍由其旧Canvas路径负责，不经过沙pass的Lmat/ToneMap。

### 3.2 排除“alpha通道存在即透明通过”

- GL侧：实际绑定default draw/read framebuffer并记查询；swap前readback同一帧盒外/非沙为RGBA零、沙内alpha=255；不读取已交换后的不确定back buffer。
- 画面侧：用高对比棋盘/彩条背景验证沙外显露；诊断开/关的非沙ROI必须相同（同一静态FrameToken、同一无损截图链）。若截图链非无损只能给测量值与未定标结论，不沿JPEG写逐像素零差PASS。
- 边界用一像素ROI查黑边/白边，fg洋红标记穿过沙体区必须可见；黑色沙不等于透明，透明清屏+不透明黑组件背景也不算通过。

## 4. 色彩编码与三层证据（不可互相代替）

### 4.1 维持唯一色彩链

P1继续 `SRGB8字节→EOTF一次→Q→Rbase+δC+δB→max0→乘Lmat→Reinhard ToneMap一次→sRGB attachment硬件OETF一次`。T34不改palette/Q饱和策略、坡度核、C3、B/C采样或七版本身份。TEXTURE只变外部合成profile，新增工程标识 `compositionRevision=P013_P1_TEXTURE_STACK_V1`，不冒充新算法配方。

EGL初始化检查KHR colorspace支持（或明确的EGL核心等价能力），`eglCreateWindowSurface`属性请求 `EGL_GL_COLORSPACE_KHR=EGL_GL_COLORSPACE_SRGB_KHR`，不是塞进eglChooseConfig属性表。随后query实际surface colorspace；makeCurrent后对default framebuffer的 `GL_BACK` 查询 `GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING`，必须是GL_SRGB且无GL错误。有GL_EXT_sRGB_write_control时显式enable其FRAMEBUFFER_SRGB_EXT并记录；无此扩展不调用无支持token，按GLES规则执行。任何未知/不支持/失败返回COLORSPACE_UNSUPPORTED并回Canvas；**没有shader手动gamma自动fallback**。

不能从color config有8位、SDK有token、glClearColor截图接近推导attachment属性。T29“sRGB attachment PASS”仅保留为历史标题，当前可用结论限定为该旧profile内容的屏幕编码近似一致；不能继承为P1显式sRGB查询通过。P0 DIAG_GRAY_V1/旧profile保持原样；P1诊断若需显示同编码灰，采用rev2已登记DIAG_GRAY_SRGB_TARGET_V2，不直接在新sRGB surface输出旧编码值。

### 4.2 三份证据各证明什么

| 证据层 | 必交数据 | 能证明/不能推出 |
|---|---|---|
| A API/attachment | EGL/GL版本扩展、requested/actual colorspace、config通道位数、GL_BACK编码、write-control支持/状态、GL/EGL错误、surface/context epoch、profile | 证明实际创建和写转换配置；不单独证明shader计算正确或compositor解释正确 |
| B GL数值/读回 | swap前同FrameToken的RGBA readback、CPU float参考和量化期望、shader/source hash、viewport/pack alignment/行方向；分别linear RGBA8、sRGB FBO及实际default attachment | 分开验证着色数值、OETF与alpha；离屏FBO通过不等于window attachment通过，更不等于屏幕已通过 |
| C compositor/截图 | 同内容Canvas参照、native窗口色块、前景标记、无损原生截图/格式/ICC或dataspace（可得时）、系统显示色彩/HDR设置与原始文件hash | 验证最终合成路径；8bit/JPEG接近不证明attachment是GL_SRGB，屏幕截图不等于物理面板光度校准 |

default attachment不可查询/读回或API失败时分别写UNPROVEN/具体错误，不能用B的离屏结果补A。截图格式没有可证明色彩元数据则明确未知，不猜sRGB。性能路径禁止每帧读回，readback只在probe/测试入口，报告其额外开销。

### 4.3 可复算色块与反例

使用**fragment输出色块**而非仅glClear，读取发生在swap之前：

- 线性输出0、0.0031308、0.18、0.5、1；按P1已签收OETF计算编码，0.18约118/255、0.5约188/255，alpha独立。
- 已编码输入c=0.5走EOTF再写sRGB attachment应回约128；直接把0.5编码值当linear送入会变约188，为重复编码反例。
- 以线性RGBA8离屏读回约46/128对照线性值0.18/0.5；sRGB attachment应约118/188。不得将两种格式的readback统一当linear数据比较。
- 原色/中性色/实际H/L/M palette、B/C开关、Lmat OFF=1及ON配对；签收CPU fixture保持同Q、同ToneMap顺序，不能为贴近截图改公式。

这些是解析期望/反例，不是新冻结数值容差或真机PASS。tester先登记读回量化/重复性容差及比较域，未冻结时输出MEASURED_NOT_CALIBRATED，designer/manager按原流程确认；不得现场调gamma使色块“看起来相同”。

## 5. 文件所有权、接口与时序（T34开发任务书）

只在T33签收后由manager放行T34；不把此方案完成当G1全体解冻。以下为接口契约示意，开发遵循ArkTS实际类型，无自由选择的并行消费名：

```text
P013CompositionProfile
  compositionRevision: P013_P1_TEXTURE_STACK_V1
  componentType: TEXTURE
  colorPipelineVersion: LIN_SRGB_REINHARD_V1
  alphaMode: BINARY_MASK
  hitMode: NONE

P013CompositionReceipt
  frameToken, physicalSnapshotHash, surfaceEpoch, contextEpoch
  requestedColorSpace, actualColorSpace, attachmentEncoding
  eglAlphaSize, srgbWriteControlSupported, srgbWriteEnabled
  compositionRevision, profileStatus, errorStage

controller.requestComposition(profile): receipt/status
controller.submitPreparedFrame(frameToken): existing frame receipt
controller.disableComposition(reason): restore full Canvas
native.queryCompositionProfile(): immutable receipt
native.captureProbe(frameToken, points): diagnostic RGBA samples/status
```

接口进入当前项目的typed ArkTS/NAPI定义时逐字段有类型/枚举；不通过any字典绕过ParseFrame，避免复发T32判型问题。captureProbe只能读取已经提交/实际draw的同epoch帧，不能自己制造或推进物理snapshot。

| 责任文件 | T34范围 | 避免冲突/交接 |
|---|---|---|
| `ets/pages/SandCanvas.ets` | 真实同Stack bg/TEXTURE/fg/controls次序、透明背景、None、切换回退 | 与T32仅onLoad绑定相关hunk先协调；不改物理/gesture source家族 |
| `ets/render/P013RenderTypes.ets` | 独立profile/receipt接口；七算法版本不变 | T32拥有旧ParseFrame契约修复，不另造替代旧帧 |
| `ets/render/P013RenderController.ets` | profile状态机、同token调度、fallback | T32负责桥绑定和入参诊断；T34等待其实际交付后合并相邻hunk |
| `ets/render/P012WaterSandRenderer.ets` | 按rev2提取prepareOnce/background/foreground/legacy包装 | 沿原绘制函数，不迁移算法；完整材质G1–G4未放行时仅probe接线，禁止伪装完整P1已实现 |
| `cpp/p013renderer/egl_context.*` | profile创建/查询/扩展、epoch恢复 | 不能改变P0默认surface的profile |
| `cpp/p013renderer/shader_pass.*`及独立probe shader | alpha二值输出/色块readback测试入口 | 保留旧DIAG_GRAY_V1；不修改已签收Rmat/Lmat公式 |
| `cpp/p013renderer/napi_bridge.cpp`及native类型声明 | 上述profile/query/probe receipt序列化 | 与T32协同，不重复修Frame bridge；失败字段不填成功默认值 |
| `ets/pages/P013P1G0Probe.ets` | bg/X/fg正确次序、controls标记、SURFACE对照与TEXTURE主臂 | PROBE-ONLY保留；want门/路由不进发布默认 |
| `test/p013_native/`与host测试/构建入口 | 由developer新增profile/错误/恢复/alpha非真机自测 | tester拥有端到端判据/测试报告，effector拥有设备采集脚本与产物 |

时序：surface onLoad→T32绑定ready→profile请求/创建→查询receipt→同快照prepareOnce→bg/沙pass/fg消费同token→成功swap后发布receipt；任一失败保持或恢复完整Canvas旧臂。Canvas/纹理共享逻辑token不等于证明同一次硬件present；动态错帧仍由T35取证。切后台/旋转/尺寸变更/恢复重建profile，资源与回执禁止跨epoch复用。

为避免T34越入G1–G4，本卡T34的最小交付是**生产可复用profile/层级适配 + 独立G0色块/二值mask/前景probe**；完整P1材质shader/六图资产仍沿原G1–G4门序。已具备材质时消费原合同，未具备时报告未实现，不能拿probe替代材质效果PASS。

T34开发完成线：定向变更入库、构建通过、非真机自测通过、失败路径/模式回退/旧profile不变有证据；交修改清单与commit和剩余设备项。T35未跑不反向阻塞开发卡关闭；真机发现具体实现缺陷才返修。

## 6. Tester/Effector 复测矩阵与反例

先由tester明确客观用例与证据schema，T35只在T32/T34交付、T33验收及manager分配设备窗口后执行。无新增阈值；历史冻结门槛不动。

| 场景/对照 | 需证明与失败定位 |
|---|---|
| 原布局SURFACE / 修正顺序SURFACE / TEXTURE明确z序 | 记录完整节点树、声明顺序/zIndex/组件类型；fg洋红条跨沙区、controls在最前。分清布局错误和合成路径，不能用原布局反证所有SURFACE |
| 背景棋盘+透明OFF / 二值沙mask | 非沙背景透出、沙内不透明、fg可见；分开记录GL alpha和屏幕像素；组件不透明黑背景必须能被反例检出 |
| None / Transparent / Default | 每次输入有唯一gesture receipt，装饰层不抢触摸；None生产、Default负对照，Transparent不双发 |
| 故意省colorspace / 双OETF / wrong attachment | A查询与B数值必须识别；C截图近似不能掩盖A未知/错误，禁止补gamma |
| 正确sRGB profile与P0旧profile切换 | 新旧身份/epoch不串；P0原热图不被改义；不支持时完整Canvas fallback |
| 前后台/resize/销毁重建/快速切模式 | native窗口生命周期、帧接受、profile重建、stale epoch拒绝、零黑层残留；日志环形滚落用持续文件证据，不能伪精确计数 |
| 静态/动态同token | 静态等待一致帧；动态trace/帧标记检查错帧，无fence证据不宣称同present |
| 功耗/CPU/GPU/显存/呈现 | 同温同内容同尺度比较TEXTURE和原臂；额外buffer/合成列入统计，不凭方案预称性能收益 |

A/B/C三层证据、原始图像和日志需有buildCommit/sourceTreeHash/packageHash、设备/OS/GPU、窗口native尺寸/DPI、shader与profile版本、FrameToken、物理snapshot、参数/seed、surface/context epoch、原始产物path/hash及采集时刻。JPEG现有素材仅历史旁证，主测色彩/透明尽量用无损PNG；不得将有损差值解释成精确像素PASS。任何一层不可得写BLOCKED/UNPROVEN及原因，保留其余真实证据。

## 7. 红线、自检与下游门序

- 本次仅新增本独立工程revision，不编辑业务代码、shader、测试、原探针回执或已签收算法稿；不使用设备。
- 已比较修正SURFACE、TEXTURE与前景GLES；唯一实施路径为TEXTURE，非静默多路径任选。
- 保留Rmat/Lmat/ToneMap及七算法版本、旧Canvas退路；不引入半透明沙算法或二次gamma。
- 明确输入命中、视觉层级、surface属性和A/B/C三层色彩证据互不等价；未把8bit截图当attachment验证。
- T32拥有桥与绑定修复；T34接本工程合同；T35补证排队；T23/G1仍阻塞直至manager核验全部必要G0项。
- 本文没有把真实P1材质未实现说成完成，也不将T34开发完成线扩大到真机验收。

交接：本文入库回执→manager快速验收T33→沿用T34原卡实施→T32/T34交付后manager排T35→tester/effector提交必要G0证据→manager决定T23。当前未放行T23，未关闭任何产品效果问题。

## 8. 官方资料与本地核查来源

2026-10-25读取，官方页面为可变资料，实际编译以API26本地SDK为准：

- [OpenHarmony 自定义渲染XComponent指南](https://github.com/openharmony/docs/blob/master/zh-cn/application-dev/ui/napi-xcomponent-guidelines.md)：SURFACE/TEXTURE上屏区别、透明内容与组件背景关系。本文据此选择纹理合成路径，不从规范推断本机已通过。
- [OpenHarmony XComponent接口](https://github.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-arkui/arkui-ts/ts-basic-components-xcomponent.md)：native库、生命周期适用类型；本地 `C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/ets/component/{enums,xcomponent}.d.ts`交叉核查。
- [EGL_KHR_gl_colorspace](https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_gl_colorspace.txt)：surface创建属性及实际attachment编码关系，默认LINEAR；EGL属性不等于最终compositor标色。
- [GL_EXT_sRGB_write_control](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt)：扩展存在时写转换控制；不在无扩展环境调用其token。

变更记录：2026-10-25，T33工程v1，新增明确TEXTURE叠层选型、二值alpha/同token/窗口契约、A/B/C色彩证据和反例、文件所有权与T34任务书；保持P1 rev2的已签收数学，不回写历史probe结论。纯方案，待manager签收与后续实测。


## 2026-10-26 用户强制边界追补（T68）

本文原有Canvas背景/前景、Canvas上屏和失败退路自本次要求起不适用于后续产品路径，显式由 [P013_NEWPIPE_ONLY_V1工程合同](architecture-P013-new-pipeline-only-revision-v1.md) §2–§8替代；原文保留作历史记录。后续仅XComponent+shader全场景，失败保持标明旧token的有效新管线帧或FAULT，不得调用Canvas或把旧帧伪装当前成功。本文其余已签收数学/物理只读/坐标/身份语义按新合同映射保留；不回改历史测试结论。
