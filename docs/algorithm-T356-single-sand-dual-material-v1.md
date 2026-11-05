# T356：双态单沙材质可辨性设计 v1

2026-11-05，designer；原卡`6aec73d8e4b0cb1ec554cb28`。路线 **`single-sand-dual-material-v1`**，状态 **DESIGN_READY_FOR_ARCHITECT / NOT_IMPLEMENTED / NOT_DEVICE_VALIDATED / PENDING_CALIBRATION**。交付单沙床态/粒子态、背景及接触边界的可实施视觉合同、像素正反控、主观例图规格和独立复盘。不使用真机，不改产品代码，不改测试/目标/工程文档；不允许修改`D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态按有限例外修改，不改其它行列/队列或滴答同名清单。完成本卡当前范围和本角色必要检查才停止；manager按交付登记，不新增验收门。

## 1. 输入事实、复用与边界

| 已读输入 | 可消费的事实 | 本稿不据此推导的结论 |
|---|---|---|
| T347正式报告及`s1.jpeg`，签包0195c935，报告HEAD41c5351 | 注入期有pY下降、mh增长；截图右下存在稀疏粒子，床量约1.1547，床层视觉很薄；所核回执CURRENT | 小床量不要求画出厚沙堆；JPEG不作为精确像素金标准；T350已补充松手冻结，不能沿用T347“静置保持”解释全部粒子 |
| T350正式报告及A_default/B2截图，同T347包 | 默认与显式粒子入口不同；B2截图页面已报SCENE_MATERIAL_DATA；C入口56/56 CURRENT，但UP后pY逐位冻结；低fps与89→30脱钩另案 | B2不能作当前完整成功帧；1fps、松手冻结和倾角钳位不是材质原因；不由本卡首次归因或修改物理 |
| T37双场adapter、T68新管线合同、当前dualfield_pass/scene_gpu/scene_layers | 同publication床与真粒子、稀疏槽、state2仍绘制、二值coverage、线性显示预乘over、完整candidate/LastGood可复用 | 历史Canvas背景/前景/回退条文已被T68替代；不能从“同token”推导流畅或材质足够清晰 |
| P013 P1配方、T99/T105底色表示及颜色路由合同 | sRGB输入→线性显示输出/一次编码、Gaussian σ6列/R18显示坡度、量化/目标链分层、同配置代可复用 | P1的Q/δB/δC/三带/条带并非本稿公式；不覆盖其冻结身份或宣称旧C水图是单沙材质证据 |
| 当前P051DerivedDemoV2、P013SceneController/Preparation | 源requestedH>0、requestedL=0；页面默认调用pipeline=0，基础pass；最新T349及调度为CPU仿真优先、GPU仿真备选 | 本卡只设计XComponent+shader材质；不新增GPU compute迁移、不碰T351并发工程 |
| 用户视频及最新澄清 | 连续流动、细颗粒、滞后与渐稳是参考观感；角度用户调节，不以固定45°作材质门 | 不从录屏判断真实材质、惯性/黏度或定时；本卡不会靠材质动画制造该物理效果 |

本轮复用结论：**保留现有发布、投影、合成及故障语义，新增独立的单沙光学配方。** 当前`SceneGlGpu::Layer`给床/粒子同一组palette，但基础bed shader的可动床分支为`mix(light,heavy,.5)`，H粒子取heavy；P1的M也由其自身配方处理。这是可从源码复算的语义差异，不能仅靠同一组uniform证明两态同色，也不据此断言T347图片的全部色差病因。新单沙路径对bh/mh等**同一材料的存储态一律使用同一albedo**；不得把移动床、接触等待当成第二沙种。

## 2. 路线选择、版本与支持面

| 候选 | 取舍 |
|---|---|
| A：只换背景/统一两个palette，保留纯色圆片和纯色床 | 可作最小消融对照；仍难在床上认出同色接触粒子，且没有床的细颗粒密度感，不选为完整目标 |
| B：直接套完整P1 H/L/M的B/C/E2和列级sparkGain | 复用资产多，但引入层纹/三带语义及Q高端放大，不是本次单沙的最小视觉规格；保留旧路径及旧测试，不借本卡改写 |
| **C：统一单沙albedo＋有界灰度颗粒＋受控表面明暗＋粒子内缘明暗** | **推荐**。以亮度和空间结构区分状态，保持同一线性色度；不增加沙种、外扩描边、透明稀释或随机闪点 |

新增身份：`materialRecipe=SINGLE_SAND_BOUNDED_LINEAR_V1`，`renderPipeline=SINGLE_SAND_DUALSTATE_SCENE_V1`，`paletteInput=SINGLE_SAND_SRGB8_V1`，`grainAsset=SINGLE_GRAIN_R8_SIGNED_V1`，`particleStyle=SINGLE_DISC_INNER_RIM_V1`，`coverage=BINARY_NATIVE_CENTER_V1`，`sampling=CONTAINER_LOGICAL_PX_V1`，`surfaceNormal=E2_GAUSS_COL6_R18_P1_V2`（只复用该已定义算子），`composition=LINEAR_DISPLAY_PREMULT_OVER_V1`，`measurement=T356_YCODE_DUALSTATE_V1`。均为新路径身份，不复用P1_ALBEDO_DELTA_V1或LIN_SRGB_REINHARD_V1描述不同公式。

首版支持**DRY_AIR_V1、单H沙**：已发布单材料登记，bl/ml/U_L/P_L与活L粒子为0，非零H的bh/mh/P_H仍保留真实库存；其它合法但未覆盖材质/介质拒绝本路径并明报UNSUPPORTED_MATERIAL_SET，不把L改标H。湿/水下、双沙、自由水透射、发光、金属闪片及粒子真实几何重建不在本卡。工程必须消费已有只读发布证据，不为本材质另跑每像素/每粒子物理审计。

## 3. 坐标、几何及尺度

复用已签收128列/实际capacity、DISPLAY_4X3显示床S_D、原生box、visualInverted一次镜像和现行粒子F32投影版本。任何材质消融不得重滤S_D、改变粒子中心、投影方式、count或state。`h=(1-qY/boxHNative)*capacity`；床mask为盒内`0<=h<S_D(x)`。粒子中心由本publication真实x/y投影；不用UI receipt、年龄、上一帧速度外推。

`logicalPx`是frame提供的容器逻辑坐标单位，不是材料高度、模型列、Android dp或未经声明的vp。`s=boxWNative/boxWLogical=boxHNative/boxHLogical>0`（由已有几何域验证，数值容差沿工程既有规则）；逻辑→native只乘一次s。CPU/GL记录实际宽高、映射及版本，不能猜DPI。本版非等比显示需明确拒绝或另版，不用椭圆粒子/不同向纹理掩盖。

| 参数 | 首版建议值/定义 | 允许的初轮离线扫描范围（非用户滑条或已标定PASS线） |
|---|---|---|
| albedoSrgb8 | **(145,168,111)**，沿现有H低端palette；床、AIR、CONTACT_WAIT共同来源 | 首版不扫描色相；新色需独立配置与全套对比 |
| backgroundSrgb8 | **(16,16,36)**，复用现有盒内背景，盒内常量不叠噪声 | 本卡不要求换背景；其它主题必须重新核对对比度 |
| radiusLogical | **2 logicalPx**，`radiusNative=2s`，与mass无关 | 1.5/2/2.5，仅视觉辨识扫描；不能逐粒自适应扩大 |
| grainFeatureLogical | **2 logicalPx**，256×256 tile每texel 1 logicalPx | 2/3；不把此量写成真实粒径 |
| grainGain aG | **0.08** | 0.04/0.08/0.12；0为消融 |
| surfaceGain aF | **0.10** | 0/0.05/0.10；保持aG+aF<=0.22 |
| surfaceDepthLogical | **8 logicalPx**，native深度8s | 4/8/12；不是σ6列/床厚度 |
| lightDirection | normalize((-0.35,0,0.82))，固定观察面光向 | 首版不按手机重力旋转光向；法线来自当前床面 |
| particleCoreGain aP | **0.10** | 0/0.05/0.10 |
| particleRimGain aR | **0.16** | 0/0.08/0.16；aP+aR<=0.30 |
| rimStart/rimEnd | **0.60 / 0.95**，单位归一圆半径 | 首版固定，不能按是否落床闪烁切换 |

有效配置在准备阶段校验所有有限值、正尺度、上述边界和无clamp色域条件，不在shader中静默修正非法配置。建议值为DESIGN_CANDIDATE，需独立读回与设备观感定标。

## 4. 唯一颜色与纹理公式

### 4.1 颜色域与不裁色条件

逐RGB令`A=EOTF(albedoSrgb8/255)`；EOTF沿现行分段sRGB公式。背景也只EOTF一次。**本版直接输出线性显示颜色**，无Q、δC、Reinhard、额外曝光或第二次EOTF。这不是对P1公式的替换。令`U=max(1+aG+aF,1+aP)`，激活配置须满足每通道`A_i*U<=1`及`1-aG-aF>0、1-aP-aR>0`；失败报COLOR_GAMUT_UNSUPPORTED，不把单通道钳白导致色相漂移。

任意已覆盖沙片元`C=A*g`，g为同一标量，故未量化时`C/sum(C)=A/sum(A)`，两态色度严格一致。显示sRGB编码和8bit量化会使RGB比例/HSV数值变化；**不要求亮暗后的sRGB码相等或HSV hue逐位相等**。中性消融`g=1`时两态必须回到同一albedo。全部输出均有限，alpha始终取coverage的0/1。

### 4.2 床沙：密实覆盖、细纹理与表面

单R8 tile：复用`P013SceneNumericAssets.buildGrainTileFine`的固定seed20261022、fineA/fineB及2倍尺度envelope、标准化/截幅步骤，只提取**加rawRGB基色之前的**`gFine∈[-2.5,2.5]`。新资产字节`n0=gFine/2.5`；`b=floor(128+(n0<0?128:127)*n0+0.5)`。shader解码`n=b<=128?(b-128)/128:(b-128)/127`，neutral128严格为0，0/255对应-1/+1。生成器必须做逐运算序兼容核对、记录新R8 hash；本稿参考模型只用可复算合成tile验证配方，不冒已经导出上述生产R8。

采样pre-invert容器逻辑位置`(X,Y)`，`ix=floor(X) mod 256, iy=floor(Y) mod 256`（正mod），NEAREST、mip0、每片元一次R8 texelFetch，无sRGB解码，不采三层彩色grain再平均。seed/phase无墙钟、frameId、motionCount；同snapshot重复绘制逐像素静止。为避免用纹理运动假装流动，首版纹理锚在容器坐标；需要跟随物质输运时另版由真实通量驱动，本卡不声称已复现视频的完整动态纹理。

法线复用已显示S_D→横向Gaussian σ6列/R18、端列复制、归一核→中心差分×`(boxHNative/capacity)/(boxWNative/128)`→上传pb32→片元插值pb后归一`nFace=normalize(-pb,0,1)`；不再做一次DISPLAY，不模糊材质mask或纹理。令`dNative=(S_D(x)-h)*boxHNative/capacity`，`l=dot(nFace,lightDirection)`，`l0=dot((0,0,1),lightDirection)`：

```text
faceDelta = max(0,l)-max(0,l0)       # ∈[-1,1]，平面为0
gBed = 1 + aG*n + aF*exp(-dNative/(surfaceDepthLogical*s))*faceDelta
Cbed = A*gBed
```

surfaceGain是内部宽面明暗，不加白线、不画沙外像素，不人为增厚薄床。bound为`1-aG-aF<=gBed<=1+aG+aF`；默认[0.82,1.18]为保守界，实际固定光向更窄。内部bh/mh边界不改变albedo、纹理或光照；其真实合计床面决定几何。

“密度感”由**真实连续床mask、alpha1、内部细颗粒和稳定轮廓**共同体现，不能用alpha随床量变化、噪声黑洞、反复叠点或亮度当体积。不足一个原生像素的真实薄床允许没有像素中心命中；报告SUBPIXEL_BED，仍正确绘制，不能扩大至至少1/2px冒充床增长。浓度、面积与质量保持不同量。

### 4.3 落沙：同albedo、有限圆片和内缘

复用实例quad，归一local为`u`，`rho=length(u)`；覆盖判据盒内且`rho<=1`。禁止外扩描边/光晕/拖尾。FLYING中心在同帧显示S_D内部时沿现行center-occlusion隐藏整圆；CONTACT_WAIT不做此隐藏，在真实位置后画，禁止吸到平滑床面。沿现有slot升序绘制次序，不擅改层叠排序。重叠像素由后画的圆片覆盖，不做加亮叠加。

```text
t = clamp((rho-0.60)/(0.95-0.60),0,1)
rim = t*t*(3-2*t)
gParticle = 1 + aP*(1-2*rho*rho) - aR*rim
Cparticle = A*gParticle
```

默认gParticle∈[0.74,1.10]。中心略亮、内缘略暗，全部是同一沙色，不独立放白色spark、不以state/ID/hash随机换色；同一ID在AIR→CONTACT_WAIT外观不跳变。小圆片内缘可帮助与床区分，但**不保证任意遮挡和亚像素中心都可辨**；可见支持不足单列，不移动中心补证。固定半径是位置标记，并非真实粒径/质量投影，不借本卡增加按mass尺寸、粒子分裂或视觉替身。

部分LAND后保留真实remainder粒子，床按本publication增加；全额LAND该ID从实例中消失，同publication床更新，不做额外淡出、不保留一帧ghost。因视觉圆片不按mass缩小，部分LAND的像素面积不等价剩余质量，报告不得用像素面积证明守恒。是否运动及何时LAND由物理负责。

## 5. alpha、背景、同token和失败

盒内背景常量不透明；床和粒子局部pass沙外RGBA=(0,0,0,0)，沙内alpha1。默认干场无水面高光/雾；前景边框沿旧值，但所有像素测量排除边框及UI。不得给整屏再套Lmat/ToneMap。其它原有半透明层仍用线性显示预乘`(a*C,a)`与`ONE,ONE_MINUS_SRC_ALPHA`；本版不新增fractional沙coverage/MSAA，若需抗锯齿另版评审，不把JPEG软边当已支持。

RGB的线性化、混合与编码须按实际target验证，规范依据见[Khronos EXT_sRGB_write_control](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt)；不能只查扩展名或把pbuffer读回当最终窗口色彩证明。shader输出线性，sRGB目标编码一次；present阶段沿实际支持的采样/目标链保持恒等，需线性FBO/sRGB FBO/窗口三阶段证据，禁止为修一台截图加第二次gamma。

同一次完整packet/lease绑定：`run/stateEpoch/physicsCommitId/publicationKind`、FrameToken、实际particle snapshot/projectionRevision/count、S_D/pb32字节hash、box/scale/mirror/geometryEpoch、material/config digest、R8 seed/hash/generation、radius与全部光学参数、surface/context epoch/真实颜色目标、scene/particle shader hash。不得因背景静态就免除其当前配置身份。新uniform配旧资产、旧床配新粒子、迟到准备、未知版本皆整candidate失败。只有全部层画完并成功swap、按原owner合同提交后才CURRENT；上一有效**完整新管线帧**如可重放则明确STALE/actual旧token，无可兼容资源则FAULT。前背景及失败路径均只用XComponent+shader，禁止Canvas上屏或回退。

## 6. 可实施接口、复杂度和资源账

本稿不强行把新字段塞进当前57个values，也不改变pipeline0/1/2枚举含义。architect新增独立typed material descriptor及packet revision（候选`P013_SINGLE_SAND_SCENE_PACKET_V1`），提供`prepareSingleSandOptics(config)`、`prepareSingleSandFrame(publication,geometry)`、`drawSingleSandScene(packet,lease)`的职责映射，实际API命名由工程稿固定。逻辑输入/输出如下：

```text
cold config -> {versions, materialId, phaseSupport, albedoSrgb8, backgroundSrgb8,
  radiusLogical, grainFeatureLogical, aG,aF,depthLogical,lightDir,aP,aR,rimStart,rimEnd,
  scalePolicy, seed, generatorRevision, grainHash, configDigest}
same published snapshot + geometry -> {S_D, pb32, projected particleXY/meta,
  exact source token, snapshot/profile hashes, clipping/counts, immutable config reference}
draw -> {requested/actual full token, config/asset/profile/shader hashes,
  pipeline, CURRENT/STALE/FAULT, original first fault, swap status, payload/bytes/pass counters}
```

不为单沙建立第二套物理库存。profile/pb准备`O(37N+N)`、N=128，不逐片元做Gaussian；上传已有XY/meta为16K上界，不新增每粒材质属性。粒子每实例六顶点，覆盖范围`O(K*rNative²)`最坏且高重叠仍需执行，床片元O(WH)、一次R8采样/exp，不能只按活粒子数承诺帧率。使用现有全帧targets和床/粒子pass，新增正常路径readback=0；不得逐帧glReadPixels、生成tile/编译shader或扫描exact70。

保守新增payload模板：单R8为65,536B；asset active/pending各一份CPU/native镜像和GPU纹理、ETS cold scratch一份，共`5*65536=327,680B`；pb32每槽512B，双ETS+双native+双GPU=3,072B；prepare的S_G/pb64双Float64 scratch共2,048B、37个F64权重296B；合计**333,096B**。已有S_D、XY/meta、全帧双纹理`8WH`等不在新增数内，不能当零成本。若工程增加一份JSON/字节/缓存/额外纹理须增账，若复用旧资源须证明生命周期。descriptor/对象/对齐/driver/RSS/resize并存/截图导出另计，模板不是实际内存或帧时测量。

正常CPU仿真路径保留，与T351的主工程文件串行交接。本设计不要求等待功能修复才能做静态数值/材质例图；动态沉降效果需真实物理推进，T350冻结归architect，不能拿本材质的例图替代其修复或把独立设备卡加入开发关闭门。

## 7. 像素测量、正控和反控

结构检查与像素读回分层：**本稿可冻结的是新配方数学及结构谓词**；下面感知数值为初轮候选测量线，记`PROPOSED_NOT_CALIBRATED`，不覆盖已有冻结阈值。tester独立实现公式，核实际typed bytes、真实shader/目标，建立三次重复性后供designer判断/manager登记。硬结构不以新软线替代。

共同保存PNG/RGBA无损原生尺寸，不重采样；JPEG只作定性。固定同snapshot/粒子中心/seed/geometry/target/config，消融只改指定参数且开新配置身份；ROI在看结果前用几何生成，不能按亮度挑点。`Ycode=.2126R8+.7152G8+.0722B8`；`Ylinear`为实际线性显示RGB同权重，**先逐像素EOTF再求Ylinear**，不把Ycode再当线性。背景对比用`CR=(max(Ylinear_s,Ylinear_bg)+.05)/(min(...)+.05)`，这里只是可复算材质指标，不是声称UI无障碍合规。

| ID | 正控 / 方法 | 故意错误反控与预期 |
|---|---|---|
| SM01同色 | aG/aF/aP/aR=0、覆盖内远离边缘，床与AIR/state2输出同A；无损8bit量化期望(145,168,111)，实际容差分target记录，不猜±2码通用门 | 床按H/L中间色、粒子按H；必须出现与参考不同的中性码，禁止以“状态不同”豁免 |
| SM02色度与域 | 隔离中间线性RGB，除g后对A；neutral/最暗/最亮/镜像，全部finite、无通道裁切，色度与理论一致 | 只给R加高光、调HSV饱和、二次ToneMap或重复gamma；独立参考应区分 |
| SM03背景可见 | 无床、稀疏不重叠、盒边留2r裕量，统计粒子覆盖内的CR P05/P50、Ycode差和像素数；默认A/背景在保守g=.74下CR>=4作为**候选** | albedo=background、中间漏EOTF或全场叠雾，CR显著下降；不把颗粒多当对比高 |
| SM04床纹理 | flat厚床，aF=0，纹理ON-OFF同支持集，报告ΔYcode RMS/P01/P99/最亮码率/重复tile；候选RMS 0.5–8码；完整tile/至少32×32logical ROI | aG=0则差严格0；frame-seeded闪点虽RMS高但SM10失败；全白点不能充当合格颗粒 |
| SM05表面 | grain OFF，flat/正负坡/相同S但bh↔mh重分配；测深度0、4、8、16logical剖面和镜像，核faceDelta与exp；纯flat严格无face增量 | S不变只改bh却改法线、把8logical当8列或重复乘s，应与解析剖面不符 |
| SM06粒子/床重叠 | 独立保存同snapshot的BED_ONLY与BED+PARTICLE（后者为正式输出，前者仅诊断消融），在可见粒子支持P测`D=abs(Ymix−Ybed)`；按每ID暴露像素数/遮挡比例统计，不按全帧稀释。初轮候选：P至少8native像素时，至少40%像素D>=4码；先报每ID再报分位数 | aP=aR=0且床无细节时同色重叠D=0，应识别不可辨；旧床/新粒子即使差更大也由SM11拒绝 |
| SM07几何与alpha | 空场、薄床0.15材料单位、厚床、state0/1/2、盒边、重叠两粒；所有材质消融coverage/mask不变、沙alpha∈{0,1}、沙外RGBA0；报告VISIBLE/BOX_CLIPPED/CENTER_OCCLUDED/NO_SAMPLE_SUPPORT | 外扩白halo、最小床厚、alpha按mass、软拖尾、漏state2、state0补画；任一结构不符即失败 |
| SM08密实与数量 | 同一床增库存只通过S_D改变覆盖，床内alpha恒1；真实0/1/低/高count，只画现有槽；活数/质量来自账本而非像素 | 用亮度或alpha替代床量、密集时复制视觉点/亮度叠加均拒绝；完全遮挡的ID不要求凭空可见 |
| SM09尺度 | s=1/2/3和粒子中心四种半像素相位，实际半径=radiusLogical*s，特征/exp深度同尺度；镜像一次；逐原生像素对参考 | s二次相乘、逻辑px=列/高度、镜像两次；半径不可用二值像素面积严格4倍当判据（栅格采样有离散误差） |
| SM10稳定 | 固定snapshot与config三次冷/热重绘；RGBA/texture hash相同；另一个frameSerial和时间戳不得改颜色，重复token不伪物理推进 | 时间噪声、随机白粒、在UP后移动贴图；静态差分非0即暴露，不用时间平均抹掉 |
| SM11同token/故障 | 床/粒子/config/asset/profile/context mismatch与swap失败逐项注入，requested/actual完整身份及LastGood可复核；CURRENT只能整帧成功 | 背景成功就CURRENT、旧床新粒、半新参数、以STALE算新成功；必须失败且不污染LastGood |
| SM12颜色链 | 中性色块0/128/145/168/111/255、真实新材质与半透明前景对照，分别测线性中间目标、sRGB FBO、最终窗口；save实际attachment/blend/write-control/dither/samples/GL原码 | 重复OETF/EOTF、错误预乘、pbuffer读回混称窗口，应可定位；黑屏不能记作透明PASS |

ROI样本不足、分母0或下层完全遮挡必须分别报INSUFFICIENT_SUPPORT/UNDEFINED_RATIO/OCCLUDED，不填0冒达标。接触圆片中心若深埋或盒外无法辨识，几何事实先记录，不改位置；0.15薄床不会被主观厚床例图替代。量化容差和CR/RMS/D的候选线仅用于后续定标，不在本卡宣称产品PASS。

## 8. 主观例图与动态合同

配套`design-T356-single-sand-reference-v1.png`是**数学配方合成示意图**，非截图、非真实shader/物理/设备输出，tile为明确的合成输入。每格同一albedo/geometry并记录例图hash，不能混入T347/T350图片冒新效果。例图JSON/脚本可复算。正式例图后续由tester在真实新shader按同fixture重出，effector再看完整页面。

| 例图 | 必须包含的内容/主观判据 |
|---|---|
| V01空场 | 暗背景、无假沙、无闪点；负控空场补点一眼可见 |
| V02厚床 | 连续密实单色系沙体，细颗粒低幅纹理、自然表面明暗；不能像白噪声电视雪花、层间换色或塑料纯色块 |
| V03稀疏落沙 | 小型同色系圆片与背景清楚分离，无白色外光圈或长拖尾；半径是显示标记，不按mass解释 |
| V04同帧接触 | AIR/state2与床并存；看出“同一种沙”，又能辨认床外与床面上的暴露圆片；重叠负控不能靠换色过关 |
| V05极薄床 | 按实际高度显示并标注薄床输入；不能为了看清造出厚层；需与V02分开评分 |
| V06无细节消融 | 同底色纯床+纯粒子，展示床上接触区可能消失；作为被否A路线对照，不标产品缺陷截图 |
| V07错色负控 | 床与粒子换成不同色相，虽然容易区分，必须判同沙辨识失败 |
| V08外halo负控 | 超出原圆片mask的描边应被SM07抓到，不采用增强可见性评分抵销 |
| V09闪点负控 | 静态床上独立白点表现出漂浮亮屑，且固定snapshot随frame变化；不得把它当细沙质感 |

真实动态另取同run的空场→DOWN/MOVE→UP→持续下降→LAND/床增长→静止，以及合法倾斜/回正。同时保留物理commit/模拟秒、pY/state、原生frame interval和CURRENT身份，分开评分：同沙色系、床颗粒、落沙辨识、接触边界、稳定性各0–10，沿团队既有及格线6记录，另报原始意见/覆盖范围；未执行就是NOT_EVALUATED，不以九格静态图给动态PASS。松手冻结、1fps、有效角钳位交既有architect分流，不用改光学补偿；设备由后续真机卡串行，开发不等该卡才能关闭。

## 9. 交接、风险及完成判定

architect消费本稿、当前T351实际HEAD及P013合同，新增工程文件明确：新pipeline枚举/descriptor/used-field校验、单材料只读支持证据、R8生成字节和资源所有权、logical/native映射、pb复用、同packet原子激活、shader/actual target记录，以及现有双场投影/state2/occlusion不变项。旧P1六图/条带API不接R8冒旧layout；现有57值不静默追加换义。公共scene/native/page hunk与T351串行，designer不改它们。

developer只按新工程稿实现材质、编译及必要非机自测，不修改CPU物理/源速率/dt/库存/预算/UP语义；tester维护新版本用例并独立读回，effector另卡真实观感。产品失败先architect，公式/效果设计问题再回designer，用例争议交manager；缺证精确补，不重开整体归因。不新增manager验收门。

主要风险：固定容器纹理不能宣称跟随物质运动；二值小圆栅格化会有相位差；同色重叠不能保证每粒都可见；薄床无命中样本是合法几何；标量调制仍有显示量化差；旧颜色目标问题未被换配方自动解决；O(WH)新增采样/exp须计入实际性能，不能假设1fps因此恢复。后续若需AA/动态输运/湿沙/金属闪光另立版本，本卡无此承诺。

本卡成功条件是**设计、例图规格、可复算数学检查、输入身份和复盘完整交付**，不是实现或设备通过。伴随证据`design-T356-single-sand-evidence-v1.json`和模型脚本仅校验新公式、候选指标可计算性及正反控灵敏度，不运行产品测试/设备或重新评分T347/T350。

## 10. 本角色检查回执

Python/numpy/Pillow数学模型已执行，exit0：256个中性色往返、10,001个圆片径向增益采样、20,001个线性色度/色域采样、12个尺度/亚像素相位支持检查、3个消融coverage不变检查及旧两色平均/重复EOTF反例。默认保守最暗粒子与背景的线性CR约5.53266；**合成tile**的flat床纹理差Ycode RMS约2.21617码，均只说明候选线在这些设计输入下可计算且非空。

为隔离背景而另设完全埋入床的state2数值fixture：S_D=70、中心y=66、s=1、半径2logical，9个互不重叠ID各有12个原生覆盖像素；细节版每ID全部像素D>=4码、中位D约8.5708码；中性消融全部D=0。九格总览中的近表面接触中心y=68.8，与此全重叠数值fixture明确分开，不能用露在背景上的圆片增加D来冒床内可辨。图下放大块为4倍最近邻说明图，测量仍用未放大的原生数组。

halo越界、错色、随frame变化白点均被设计模型反控检出；0.15材料单位薄床在s=1时无像素中心命中且未增厚。已检查例图布局/标注、12项SM合同、9项V规格及输入hash。未执行产品、GPU shader、编译、独立tester/effector或真机测试；这些检查不构成材质产品/主观效果PASS。
