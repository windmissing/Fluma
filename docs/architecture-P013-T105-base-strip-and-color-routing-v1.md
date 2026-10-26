# T105 原生像素三带整数条带与T101到件分流 v1

日期：2026-10-26。architect。原卡 `6adf3250e4b017ab34041a9b`。不使用真机。消费T99全稿/证据/复盘、T101 067fd84报告/源码/raw/manifest和T87/T93。**条带工程方案交付；未实现，numeric仍MEASURED_NOT_CALIBRATED**。

## 1. T101原始数据与实际目标

本轮直接核读生产/debug raw各184320 bytes，全图diff=0，二者SHA256 `2da8baa864dc3ea298db7a8c2cefe14a0b42ebf06183679abe36f6c8459ac1db`。float raw ToneMap(88,78)约(0.933986247,0.709221125,0.334963888)，对应linear×255的238/181/85；接受为这些独立执行变体的观测。gain/Qbase/Rpre/Lmat/ToneMap五阶段现已到件，不再声称连续量完全缺失。

但“同一SRGB8_ALPHA8 pbuffer目标”与源码不符：pass1生产MaterialPass在默认pbuffer绘制；pass2另建FBO，attachment0明写**GL_RGBA8**，attachment1为RGBA32UI；pass3再建RGBA32F FBO。三者同context，**不是同target/encoding**。diff0只能证明这两次输出字节相等，不能证明所有插桩无影响、相同sRGB目标等价或uniform必然无误。sRGB pbuffer是EGL颜色空间属性，不是`GL_SRGB8_ALPHA8`纹理内部格式名。

environment实际 `queryOK=1 colorspace=12425=0x3089=EGL_GL_COLORSPACE_SRGB`，不是LINEAR(12426)。这直接不支持“缺KHR→请求静默降级”的解释；EGL1.5核心路径及T80/T55/T88已存在的显式sRGB FBO编码观测保留。不能写“本host无真正编码路径”。GL扩展从GL串取得属到件，但缺逐target actual attachment、read/draw/buffer等状态时，默认pbuffer为什么读线性特征仍WRITE_VS_READ_UNRESOLVED。

manifest的production/debug/stage `*ShaderHash`分别长7214/6948/7280字符：源码调用hexOf输出**源码字节十六进制文本，不是64位SHA256摘要**。可解码还原源码并另算hash核身份，不能把这些字段直接当摘要相等证明。粮/层资产hash64字符有效不自动补齐源码/exe/DLL/fixture与目标状态manifest。多阶段debug Rpre表达式/操作结合与生产helper也须逐行diff，不能从单点最终diff0外推所有阶段操作图同一。

结论：局部base表示设计仍可实施，T101提供生产输出和阶段直观测；目标链/插桩等价/完整数值闭环未完成。测试执行完成登记保留，不重开T101、不改旧raw或阈值。

## 2. 冻结F64 evaluator与palette来源

新增纯ETS `render/preparation/P013BaseByteEvaluator.ets`，版本 `P1_FROZEN_F64_EVAL_V1`；输出表示 `P013_NATIVE_X_F64_BASEBYTE_V2`。用ArkTS number binary64固定语句/临时变量次序实现，不另用native float或shader重算；禁止FMA/重结合、a+(b−a)t、Math.round或先量化/插值byte。

已核T55 `test/p055_material_fixtures.cjs` sample实际为 `a*(1-t)+b*t`，M为`L*(1-fH)+H*fH`，base为`floor(clamp(P*gain,0,248)+.5)`，与T99所定RN操作图一致。T55仅boxWidth128的夹具，任意原生宽/offset的推广严格按T99几何，不宣称旧测试已覆盖所有W。半整数nextDown(.5)的加.5舍入效应继承F64 evaluator，不换成理想half-up。

accepted配置新增明确原编码域 `paletteEncodedF64[6]` 及 `paletteInputRevision=P1_ENCODED_PALETTE_F64_V1`；工厂H=(145,168,111)、L=(216,196,150)取原常量来源。normalized palette继续由这些数按RN64除255生成，bind时同时校验一致并将原位型纳入digest。自定义配置若只有旧Float32 uniform或normalized字段、缺原编码来源，返回BASE_PALETTE_PROVENANCE_MISSING，不以×255/round猜原值。M grainBase(181,182,131)与动态底色分开，七版本及原Q/增量/光照不换义。

prepare冻结**实际上传layout2 Float32字节**，转number后按既定RN插值；不用未上传高精度profile。对每j原生viewport中心 `pX=vX+j+.5`，qx0=pX−boxXPx，validX先判0≤qx0<boxWidthPx，mirror只在这里做一次qx=boxWidthPx−qx0；r=clamp(qx/(boxWidthPx/128),0,127)、i=min(floor(r),126)、t=r−i。fH=row1.g，gain=spark?row2.g:1。H/L/M逐带逐RGB先混palette，再gain、clamp248、floor(RN64(+.5))；同列不同native像素不合并，条带索引绝不按floor(modelX)。

此求值使用**native box px**与viewport，不把scene的canvasBox vp混进base几何；B/C等仍按原各自canvas采样合同。viewport为整数轴对齐、单采样；旋转/斜切/Y相关gain/MSAA/sample shading缺乏可分离证明，返回BASE_REPRESENTATION_UNSUPPORTED，不用低分辨率近似。

## 3. 条带格式、类型与同代身份

`P013BaseByteStripDescriptor`：apiRevision=1、representation/evaluator/paletteInput版本、七旧版本、session、stripSerial、frameToken、run/state/physicsCommit、configBindingId/configDigest、geometry/context/surface epoch、viewportOriginX/Y/width/height、boxF64Bits、mirror、sparkEnabled、profileSha256、paletteBits/hash、payloadSha256、byteLength=12W。descriptor不可变，SHA使用实际有效byteOffset/length；不把整底层buffer或FNVa checksum冒充SHA。

载荷三个layer H/L/M，每层W×1、RGBA8UI，线性地址`((k*W+j)*4)`；RGB∈0..248，A=validX?1:0；invalidX时RGB统一0。native GL_TEXTURE_2D_ARRAY `GL_RGBA8UI`，upload=`GL_RGBA_INTEGER/GL_UNSIGNED_BYTE`，NEAREST、mip0、三层，shader为highp usampler2DArray+texelFetch(j,0,k)。禁止UNORM/sRGB decode/filter；采出整数转换float精确，不再gain/混色/clamp/round第二次。

新scene packet显式 `P013_SCENE_PACKET_V2_BASE_STRIP`（只对申请新表示的P1路径）；缺版本不是默认新表示。basic fullscene不需要该strip；原P1_GPU_FLOAT_BASEBYTE_V1仅作明确旧身份/历史对照，V2失败不能偷偷退旧表示并声称V2成功。七旧版本保持原义，两个新版本及palette provenance加入accepted配置摘要与完整fingerprint，不复用只覆盖grain/strata的老digest解释新配置。

建议新frame API `stageBaseByteStrip(desc, Uint8Array)→receipt`、`pumpBaseByteStripUploads(byteBudget)→receipt`、`drawNewPipelineFrameV2(frame,...,stripSerial)`；NAPI严格型别/长度/offset/detach、安全整数、版本、实际字节域/hash/几何/配置身份。静态material assetGeneration不随每物理帧增长，stripSerial是独立有界单调frame资源身份，requested/actual stripSerial/profileHash/payloadHash须进receipt；同assetGeneration不证明条带可复用。

缓存key含全版本、profile实际字节、palette编码位型、spark、viewport/box/mirror/geometry/config。OFF/ON光向/Lmat、B/C不会改变base字节，但每帧仍需显式将命中内容绑定当前token；不可直接借旧descriptor冒新token。profile变化或spark/mirror/geometry变化须匹配新payload。用于下一帧准备的scratch不得覆盖已stage/lease数据。

## 4. 资源、pump与scene原子性

新增native `base_strip_uploader.{h,cpp}`，与static MaterialUploader分开但复用同session owner/mutex/epoch规则。两GPU槽（ACTIVE/PENDING，分别三层一张texture）＋两native CPU不可变槽，合计48W bytes；NAPI复制架构还需**一个ETS可复用输出scratch=12W**，实际稳态总载荷上限60W bytes，不能漏算桥复制。W1178单strip14136B、全部70680B；W3840单strip46080B、全部230400B，另列metadata/driver开销。这是对T99建议48W的显式工程资源细化，不改变表示数学。CPU缓存只记key，不重复保存payload；同步stage返回后ETS scratch才可复用。无无界内容缓存、无每帧glGen/compile，尺寸变化在prepare冷路径按失效次序释放旧geometry strip槽再重建，不能新旧各两套藏超额。查询GL_MAX_TEXTURE_SIZE/ARRAY_TEXTURE_LAYERS；超过能力或整数溢出拒绝，不缩W、降模型列数或静默分页。

总frame上传调度上限65536 bytes（static material与base strip合计），分配固定策略：每个显示tick base优先完成不超过其剩余额，剩余给static；记录两类实际bytes，不能两次各65536后冒总量合规。单层可按连续j区段glTexSubImage3D，不要求完整4W一行才进度：每次块长≤floor(budget/4)，预算不足4合法无进度，非法budget（非有限/非整数/≤0/>65536）400且0工作/游标状态不变。保留static G2原合法预算语义，不通过strip接口扩大它。

state=CPU_STAGED/UPLOADING/READY/ACTIVE/DISCARDED，native拷贝校验完成才替PENDING。最多一个PENDING，未lease可被较新请求明确丢弃并记录discarded serial，不热扩容；长宽超大/反复新profile可能导致READY饥饿，必须报告NOT_READY/STALE与丢弃计数，不能用旧profile画新物理帧。先采用同步O(W) ETS准备，无异步worker新竞态；未来异步结果必须检查完整token/key，迟到丢弃。

scene绘制前在同owner内同时Acquire材料lease与strip lease；frame的T_data/profile hash、配置、viewport/box/mirror/spark和strip完全匹配才画所有pass。任何错配在candidate开始前拒绝；validX预检须与实际覆盖域一致，不能让fragment透明返回掩盖缺条带。所有pass＋present＋swap成功后，一次无分配CommitSceneLeaseAfterSwap同时提交strip ACTIVE、material ACTIVE、LastGood source；任一失败Abort两lease，active不部分替换。

LastGood是已完成的全帧texture，重呈不依赖把旧strip绑定新frame；保持T68兼容条件/STALE身份，requested/rendered/lastGood分别记录。context丢失旧GPU句柄失效，CPU immutable数据保留但须按新context/surface/geometry重验证或重算并重传；恢复READY前FAULT/NOT_READY，不拿旧handle成功。resize先使旧geometry strip不匹配，LastGood是否可重呈按原兼容条件，不因条带仍有bytes而放行。

独立回执计数 `baseStripTextureLiveCount≤2/baseStripGpuBytes/baseStripCpuBytes/pendingBytes/uploadedBytes/discardedSerial/restoreCount`；不挤占或隐藏原materialTextureLiveCount≤4和fullframe两buffer计数。release清所有strip/session绑定；失败路径同样无泄漏。资源异常/GL错误返回stage明确，禁止Canvas或gamma回退。

## 5. 最小文件/hunk与分阶段developer任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身行状态有限例外；不改滴答同名清单、不回退共享代码。画面仅XComponent+shader含背景前景，无Canvas回退，失败FAULT或标明STALE上一有效新管线帧。

| 阶段/所有权文件 | 实施要求与前置 |
|---|---|
| A `P013BaseByteEvaluator.ets`/新BaseStripTypes | 冻结F64操作图、配置编码palette来源、pixel中心/X三带、定容CPU staging、bytes/hash。先有独立base字节oracle测试，再开发；不改主参考公式。 |
| B `base_strip_uploader.*`/CMake/native bridge/ETS native类型 | 严格stage/pump/版本/epoch/容量/状态、GL整数texture和回执，host整数上传读回；旧static uploader身份/预算不改。 |
| C `P013SceneController/Preparation/Types`与scene_compositor/gpu | packet V2/profileHash/stripSerial同packet绑定、总上传调度、双lease/原子swap/恢复/计数；共享hunk串行交接，不整页覆盖。 |
| D `material_pass.*`/P1 shader＋嵌入hash | 新usampler与viewportOrigin整数，按j/k读base，删V2 palette/gain/round计算，保留Q/δB/δC/Lmat/ToneMap/输出线性及mask/k。旧七版本不换义，新shader hash显式更新。 |

类型与digest/接口revision先落，不能先改shader猜缺字段或将空strip默认为V1。新表示有明确配置选择，开发完成/编译/离线自测交付，设备另管。若现行生产palette配置无法给出编码域原值，先补provenance生产者，不逆推Float32；其它动态几何导致X可分离条件不成立则退designer具体输入域，不由developer造近似。

## 6. T101精确tester续证（剩余项，不重复已交连续量）

同context显式画**同一个SRGB8_ALPHA8 texture FBO**作生产与debug对照，分别正确建立/记录draw/read/buffer/viewport/blend/dither/write-control等；integer stage保持单独attachment，生产只对color attachment写。再做独立RGBA8组，记录每组actual encoding，默认pbuffer另做GL_BACK actual encoding及受控常量写/读/采样三角对照，不拿query的SRGB替功能结果。保留T101旧diff0/raw，不改标签冒称修正后的结果。

新manifest把源码hex字段改名sourceHex，另外输出真正SHA256（decoded源码、拼装vertex/frag/helper、exe/DLL、fixture/bin、uniform原值/上传位型、所有raw），自动验证长度与来源。五阶段已交可复用，仅补同目标/操作次序验证，不重做无关全部场景。T87的正确第六常量、default query、四目标context身份、每常量raw/逐阶段error及T93半透明量化dst/dither分支尚无全交证明，按具体缺项补，结果回architect。

“没有GL_EXT_sRGB_write_control”只表示不能用扩展开关，不表示无sRGB编码；“没有EGL_KHR_gl_colorspace”在EGL1.5不等于不支持；T101 actual query为SRGB必须如实记录。最终numeric不改容差/主测域、不加gamma，宿主不外推Harmony。

## 7. 独立base回归与产品边界

tester独立验证逐j×三k条带与冻结F64 **零字节差**，T88例点V2=172、旧V1=173保留；W128/256/1178及非整数box原点/宽、r=0/127、边外valid、mirror一次、M同列多native像素反例、spark开关、OFF/ON/B/C base不变、0/248及half邻域位型。禁止以1码容差通过离散字节测试。RGBA8UI实际GPU整数读回、版本/hash/profile/spark/几何/迟到/offset/非法budget/partial upload/lease/resize/context/失败回滚/计数与释放独立覆盖。

base逐字节一致不证明GPU mask/k、纹理索引、Q/pow或完整颜色相等；最终色继续原F64/实际目标对比与MEASURED_NOT_CALIBRATED。T57设备A/B/C、完整scene半透明和p50/p95/内存/上传带宽仍独立，稳定性实质前置沿T92/T97，不因本条带设计交付解除。性能代价实测，不承诺O(W)就自动达到帧率。

T105完成工程接口/资源/事务和色彩到件分流；本轮仅文档、现存raw核读，无业务/shader/测试/阈值修改，无GPU测试或真机。原卡报告完成与剩余tester缺项，自己的行结束待验收。
