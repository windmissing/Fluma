# T362 · 双态单沙材质工程合同 v1

2026-11-05，architect，不使用真机。消费 `algorithm-T356-single-sand-dual-material-v1.md`、配套模型/JSON/复盘，以及T351后的SceneController/Preparation、NumericAssets、DISPLAY adapter、dualfield pass、scene compositor/GPU/targets、NAPI和EGL源码。T356是设计/合成例图，不是已实现shader；本文是developer可执行合同，非材质/设备PASS。物理冻结T359和性能T361独立，材质不能冒修二者。

## 1. 路线、接口和文件边界

当前57值 `SceneConfiguration::values[0]` 的0/1/2及P1/base-strip协议均保留含义；基础bedFragment的mh分支确为两色平均、H粒子取heavy，仅统一palette不足以实现T356。同沙新路径使用独立typed config/packet及NAPI入口，禁止把新模式塞入旧values数组或以P1身份描述新公式。

固定新增 `P013SingleSandPipeline.SINGLE_SAND_DUALSTATE_SCENE_V1`（序列化值同名字符串，闭集枚举）、packet `P013_SINGLE_SAND_SCENE_PACKET_V1`、config `P013_SINGLE_SAND_CONFIG_V1`、资产 `SINGLE_GRAIN_R8_SIGNED_V1`、recipe `SINGLE_SAND_BOUNDED_LINEAR_V1`。T356其余palette/particleStyle/coverage/sampling/surfaceNormal/composition/measurement版本逐字登记，未知值整包拒绝。新typed类型与旧SceneReceipt并列，不能让旧NAPI bool-v2 parser误接新packet。

| 文件/模块所有权（developer卡） | 精确改动 |
|---|---|
| 新 `render/P013SingleSandTypes.ets` | 配置、geometry、support、frame packet、asset receipt、完整scene receipt；所有数值有限/版本/长度/身份校验 |
| 新 `render/preparation/P013SingleSandAssets.ets` | 独立R8冷生成和hash；不调用旧彩色三层grain生成后反算灰度 |
| 新 `render/P013SingleSandPreparation.ets` | 消费已封签只读publication，复用DISPLAY/投影，准备S_D/pb32和单H支持证据；不调physics step/audit |
| 新 `render/P013SingleSandController.ets` | `prepareSingleSandOptics`、`prepareSingleSandFrame`、`drawSingleSandScene`三步和有界收据；独立controller显式选路 |
| 新 native `single_sand_packet.{h,cpp}` / `single_sand_assets.{h,cpp}` / `single_sand_pass.{h,cpp}` | typed解析/拥有拷贝、配置与资产lease、R8纹理、床/粒子shader；GLSL独立文件/源hash |
| `napi_bridge.cpp`、ArkTS native声明、构建CMake | 新 `registerSingleSandConfiguration(config)`、`stageSingleSandGrain(desc,Uint8Array)`、`drawSingleSandScene(packet,Float32Array profile,Float32Array xy,Uint32Array meta)`；返回独立typed receipt |
| `scene_compositor/scene_gpu/fullframe_targets` | 复用整scene candidate/LastGood时序与EGL owner，增加独立配置/asset lease分支或共享内部抽象；不得仅把新资源塞进旧P1 Material()==true分支 |
| 页面及facade publication只读接点 | 显式材质选路、配置/诊断冷入口；与T359页面/物理hunk串行交接，先以静态sealed fixture开发，不覆盖T359诊断修改 |

新路径必须纳入同一session/EGL owner和surface/context epoch，不能创建第二个抢同XComponent窗口的渲染owner。共享compositor改造需保旧0/1/2及base-strip全部用例和错误语义。执行prompt持续完成本角色开发/编译/非机检查才停止；具体阻断回原卡。仅改本人清单状态，不改其它队列/同名滴答清单。全渲染前背景只允许XComponent+shader，失败FAULT或完整旧token显式STALE。

## 2. Typed配置、支持证据与拒绝

`SingleSandConfigV1`字段：上述版本、materialId=`SINGLE_H_SAND_V1`、phaseSupport=`DRY_H_ONLY_V1`、albedoSrgb8:uint8[3]=(145,168,111)、backgroundSrgb8=(16,16,36)、radiusLogical=2、grainFeatureLogical=2、grainGain=.08、surfaceGain=.10、surfaceDepthLogical=8、lightDirRaw=(-.35,0,.82)、particleCoreGain=.10、particleRimGain=.16、rimStart=.60/rimEnd=.95、seed=20261022、generatorRevision、grainHash、coverage/sampling/composition版本。参数范围及不裁色条件逐项照T356§3–4，默认均标DESIGN_CANDIDATE。lightDir在配置冷态归一一次；零向量拒绝。纹理特征2/3等扫描值形成新配置，不能热改uniform却不换digest。

配置digest使用**固定字段顺序的canonical UTF8二进制说明串**：版本/枚举按ASCII长度前缀，整数无符号十进制，浮点用IEEE754 F64 little-endian hex（保±0，不用JSON对象键顺序/locale）；数组按固定顺序。shader输出只用由该config算得的线性A/background，CPU/NAPI native独立核字段范围并重算digest。生成器hash、R8字节hash、shader源SHA256单独记录；新增字段必须版本化。没有支持的digest算法/未知字段不能静默降级。

支持入口先走现有 `derivedPublicationGateV2` 和当前只读publication seal/identity；新路径首版只接受V2已注册DRY_AIR_V1、128列、真实capacity、particleRouteActive与`dual-field-particle-v1`。在**既有pack对字段和槽的遍历中**同时核：bl/ml/suspL及P_L列缓存全0，活粒phase均H，suspH也0，freeWater为dry合同的0（不能用waterTop==0判定，dry waterTop为床+池包络），H床bh/mh/池真实保留；materialId来自受控配置绑定且对应单材料publication，不由页面随手声明“单H”。不建立第二套库存、不另跑exact70、allocator或守恒审计。空场同样允许并有zero support证据。

`SingleSandSupportV1`由受控prepare对同sealed state私有构造，绑定run/epoch/commit、publicationKind、policy/certificate、source seal serial、原始profile与投影payload hash、各被核数组形状、L非零列/粒数、medium/material登记。它是渲染准备的支持面证据，不冒充新的物理证书。NAPI收到独立自有typed payload后核元数据身份/长度、有限域和全部粒子tag；不得把调用方传入一句supported=true当充分条件，也不对不可见canonical再次物理审计。原始支持与显示profile的hash分开，F32显示损失不能冒称原始F64质量变化。非法物理输入沿原严格失败；合法但L/湿/多材料不在本路径支持面者明报UNSUPPORTED_MATERIAL_SET，不重新标H。

新receipt独立字符串 `reason` 和原GL/EGL/status字段，不占用/重定义旧数值码：`SINGLE_VERSION_UNSUPPORTED`、`SINGLE_PUBLICATION_MISMATCH`、`UNSUPPORTED_MATERIAL_SET`、`SINGLE_TYPED_RANGE`、`SINGLE_GEOMETRY_UNSUPPORTED`、`SINGLE_CONFIG_INVALID`、`COLOR_GAMUT_UNSUPPORTED`、`SINGLE_ASSET_NOT_READY`、`SINGLE_ASSET_HASH`、`SINGLE_CONFIG_ASSET_MISMATCH`、`SINGLE_TARGET_UNSUPPORTED`、`SINGLE_DRAW_FAILED`；第一失败原码/阶段保留，不能用后续STALE覆盖。expected未支持能力登记NOT_IMPLEMENTED/UNSUPPORTED，不冒产品PASS。

## 3. Packet、坐标与显示profile

`SingleSandPacketV1`固定绑定session、surface/context/geometry/continuity epoch、native viewport、native box、logical box、scalePolicy、run/stateEpoch/commit/publicationKind/ledger key、frameSerial/frameToken、config revision/bindingId/digest、R8 generation/hash、shader hashes、projectionRevision、rawSourceHash/displayProfileHash/pbHash、count/capacity/particleEnabled、support evidence引用、真实target身份。无每粒新材质属性；xy/meta沿现行column-preserving F32投影，tag phase/state/id与slot升序、state0跳过保持不变。native parser检查typed类型/byteOffset/长度/溢出，接收后立即拷贝到自有candidate，后续异步上传不持有JS borrowed pointer。typed profile固定 `Float32Array(256)`：前128 S_D、后128 pb32；xy/meta各2×capacity（只消费count），尺寸与hash按显式little-endian字节定义，不用native struct padding。

S_D来自现行DISPLAY_4X3_V1的sandTop显示序列，一次4趟3点滤波/端列复制，和现有显示profile相同；**不能把已显示S_D再次DISPLAY**。pb计算在F64显示序列上使用Gaussian σ6列/R18、37项 `exp(-j*j/72)` 顺序归一、端列复制；差分严格复用 `P013MaterialDataPreparation` 的端点/分母约定，然后乘 `(boxHNative/capacity)/(boxWNative/128)`，最后一次F32上传。新prepare可抽取纯profile算子供旧/新共用，但要求旧输出逐位回归；不为复用被迫生成P1的warp/spark/motion/彩色资产。shader插值pb后构造normalize(-pb,0,1)，不插值已经归一的法线替换公式。

明确端列差分为 `(SG[min(127,c+1)]-SG[max(0,c-1)])*(boxHNative/capacity)/(2*boxWNative/128)`，边界仍除2而非改单边差分；S_D/pb沿旧dual shader相同的列索引和插值约定采样，防止像素中心平移半列。权重/SG scratch不传native，upload只含已验F32 profile。所有shader与独立参考均以该实际字节profile为输入，再另核CPU准备的F64→F32过程。

logical坐标是容器自定义逻辑像素：frame给logicalBoxW/H和nativeBox，sX=Wn/Wl、sY=Hn/Hl；都有限>0且等比。等比检查冻结为 `abs(sX-sY)<=1e-6*max(1,abs(sX),abs(sY))`，通过后s=sX并记录误差；超出拒绝SINGLE_GEOMETRY_UNSUPPORTED，不悄拉伸圆片。native EGL_QUERY_SURFACE/已有surface prepare结果为实际viewport依据，UI的vp只在页面形成native几何时换算一次，controller不能再次vp2px。半径2s、深度8s，grainFeatureLogical不乘第二次s。

像素中心native q=(gl_FragCoord.x,H-gl_FragCoord.y)-boxOrigin，盒边沿现行半开区间；visualInverted仅一次逆映射得到pre-invert容器逻辑(X,Y)，使用同一坐标采S_D/pb和R8。粒子先按现行投影将material x/y转box native，mirror一次；rho沿原quad local，保证圆片coverage不变。床mask `0<=h<S_D(x)`，h=(1-qY/boxHNative)*capacity；薄床无原生像素中心命中报SUBPIXEL_BED，不设最小厚度。FLYING的中心落在同帧显示床内隐藏整圆；CONTACT_WAIT不隐藏/不吸附到平滑面，内缘不外扩。部分LAND保留实际remainder，完整LAND从同publication实例移除，无ghost淡出；像素面积不证明质量。

## 4. R8冷资产与内存所有权

新生成器按当前 `P013SceneNumericAssets.buildGrainTileFine` **raw基色之前**的算子冻结：256²；fineA/B seed+11/+23，envelope seed+37、尺度2×fine；LCG使用Math.imul(1664525,state)+1013904223的int32溢出，取(state>>>8)&0xffffff再除0xffffff；环形box滤波radius=max(1,Math.round(feature/2))、循环y/x/dy/dx顺序；每层去均值/方差及二次去均值和截幅顺序沿原函数；fine=.5A+.5B、env=1+.4E，组合再按原顺序标准化/去均值/clip±2.5。所有Math.round为JS语义，不能换C++ties-even。developer须固定源函数hash并以原函数raw输出做逐F64运算序核对；不能把设计合成tile hash `e9bec0…`冒生产R8 hash。

raw gFine/2.5得到n0，编码 `floor(128+(n0<0?128:127)*n0+.5)`，输出row-major y*256+x的Uint8Array65536，无RGB/alpha；decode b<=128?(b-128)/128:(b-128)/127。GL_R8/RED/UNSIGNED_BYTE、UNPACK_ALIGNMENT=1、NEAREST、mip0、REPEAT或整数positive-mod texelFetch（本版取positive-mod texelFetch），无sRGB解码、无mipmap。像素ix/iy=floor(X/Y)正mod256，纹理一个texel=1logicalPx，feature2来自生成而非采样倍率。每沙床片元一次fetch；particle不采grain，无frame/time/random phase。

seed/feature/generatorRevision→R8 bytes→SHA256由冷态生成并缓存。`stageSingleSandGrain`校验size65536/hash/versions/config绑定、拷贝CPU自有pending再上传完整纹理；generation单调、迟到配置丢弃，有界active/pending各1；asset lease引用只读且绑token/config/epochs。纹理上传或shader编译失败不改active，context loss后只从已保存CPU字节重新上传同hash，不能重生成未知新tile却续旧generation。release在owner线程，lease在swap后提交或失败abort，pending替换不可释放LastGood仍依赖的资源；旧context资源仅失效不跨context GL delete。

正常态账从T356模板333,096B起（5份R8=327,680；pb32六份=3,072；两个F64 scratch=2,048；weights296），逐项核生命周期。新packet若另存S_D两份ETS+两份native+两份GPU则再加3,072B；若复用现有profile纹理/缓冲，必须证明原存储能保candidate/LastGood，不重复扣账。生成峰值原算法fineA/fineB/env/out及white等F64临时数组另记，**不包含在333,096B稳态**；限制冷生成串行，释放scratch后只留R8。NAPI临时copy、descriptor/string/hash、对象/driver、GPU对齐、resize期间双代、shader、测试readback另计真实峰值。已有xy/meta每capacity16B单份、frame targets双SRGB8_ALPHA8约8WH也要记录总占用，不称免费。新profile正常上传1024B/帧+count×16B实例，不传R8每帧。

## 5. 颜色/光学与实际目标链

冷config将albedo/background逐RGB做一次标准sRGB EOTF，给shader统一线性A。不复用旧dual palette branch，不做P1 Q/δC/Reinhard/曝光。激活前校验 `U=max(1+aG+aF,1+aP)`、A_i*U<=1、1-aG-aF>0、1-aP-aR>0；失败COLOR_GAMUT_UNSUPPORTED。shader不靠通道clamp修非法参数。

床在mask内 `d=(S_D-h)*boxHNative/capacity`、light=normalize(-.35,0,.82)，faceDelta=max(0,dot(normalize(-pb,0,1),light))-max(0,light.z)，`gBed=1+aG*n+aF*exp(-d/(depthLogical*s))*faceDelta`。rgba=(A*gBed,1)，沙外=(0,0,0,0)。粒子rho<=1且盒内，t=clamp((rho-.60)/(.95-.60),0,1)，rim=t²(3-2t)，gParticle=1+aP*(1-2rho²)-aR*rim；rgba=(A*gParticle,1)。AIR/state2同公式，默认范围床[.82,1.18]、圆片[.74,1.10]保守界，不以state/ID变色；后画槽覆盖前槽。背景EOTF(16,16,36)后不透明，dry不启用旧水雾/水高光，前景沿原值且同帧，不用整屏ToneMap。

实际生产目标沿当前 `FullFrameTargets`：candidate/LastGood为GL_SRGB8_ALPHA8，attachment需查GL_SRGB；shader写线性，硬件一次编码。层混合ONE,ONE_MINUS_SRC_ALPHA，binary沙边无MSAA/fractional覆盖，不靠alpha补采样。present采sRGB texture硬件解码回线性，恒等shader送已证EGL sRGB窗口再次编码，这是两次**不同存储阶段**的正常encode/decode链，不能误称最终双gamma；禁止在present手写pow。EGL窗口实际query colorspace/attachment、EXT_sRGB_write_control能力与状态及GL原码沿 `EglContext`实证检查，不只看扩展名。目标不支持报SINGLE_TARGET_UNSUPPORTED，无linear未知窗口降级。

诊断SM12另用明确GL_RGBA8线性FBO读回近似线性存储码、sRGB FBO存储码和真实窗口各层证据；CPU参考按相同格式量化，不能将RGBA8当无限精度float或pbuffer当窗口。测试附件可升级浮点但须另记格式/支持/容差，不能静默换产品目标。记录dither/samples/blend/write-control/viewport及GLSL highp实际精度，量化容差按实际target独立登记。正常路径readback=0。

## 6. 同scene原子激活及收据

准备顺序为：验证sealed publication和支持→准备完整profile/xy/meta→验证几何/config/资产generation→native自有copy/identity/hash→取得config+asset immutable lease→candidate所有层（背景、床、粒子、前景）→present→swap成功→无分配/无回调的commit active/config/assets/targets/LastGood。沿既有SceneCompositor先完成可能失败的allocation/身份copy，再触及candidate；新SingleSand不要走旧`Material()`自动选active旧配置配新packet的回退分支。

只有requested token/config/asset/profile/geometry/target完全匹配且整帧swap成功才CURRENT。迟到asset、hash不符、shader/上传/某层失败、swap失败都不得污染LastGood或commit配置；可重放上一有效完整frame且surface/context兼容时STALE，actual保持旧token与旧config/hash；跨context无有效LastGood则FAULT。只背景成功、旧床新粒、换配置未换背景均不能CURRENT。类型支持错误先于GL绘制失败，保first failure，旧路径码不改。

receipt包含requested/rendered/lastGood完整身份与pipeline/config/grain/profile/pb/projection/shader hashes、material support摘要、target实际色彩信息、count/occlusion/clipped/no-support统计、swap/FAULT/STALE/first failure原码、generation lease状态、CPU/GPU bytes/texture live数、prepare/upload/draw次数和有界阶段wall。normal counter不能用hash隐藏全帧读回。CPU物理只读且exact70/endpoint/elapsed/particles合同不变。独立配置消融开新revision、用同snapshot，不改几何或物理来帮助可见。

## 7. 独立验证和交付

开发编译签包、非机覆盖typed parser/version/shape/finite、单H与L/湿拒绝、source token错误、R8对原raw运算序与hash、geometry/alias/slot/state2、lease/config/asset失败原子性和context恢复；旧0/1/2及base-strip回归。静态sealed fixture可在T359物理修复之前实现材质；页面接线等T359共享hunk交接后合并。不是把真机/effector加为developer关闭门。

tester独立实现T356公式/ROI/数据解析，不导入生产shader/helper作oracle，真实新shader分别出native尺寸RGBA/PNG与输入/target manifest、正反控按新packet固定：

| SM | 正控与必须被检出的反控 |
|---|---|
| 01 同色 | 四gain全0，床/AIR/state2中性输出(145,168,111)；旧两色平均应失败 |
| 02 色度/域 | 解增益回A、最暗/最亮有限无裁色；单R高光/重复gamma/ToneMap应失败 |
| 03 背景可见 | 稀疏无床、预注册粒子覆盖ROI，CR分位及样本数；同背景色反控，CR>=4只候选 |
| 04 床颗粒 | 同支持集grain ON/OFF差，完整tile或≥32²logical；grain0严格差0，闪点不能以高RMS过关，RMS .5–8只候选 |
| 05 表面 | grain OFF、flat/正负坡、相同S的bh/mh重分配、深度0/4/8/16logical；错误按列/重复scale/库存态换色需失败 |
| 06 同色重叠 | 同snapshot BED_ONLY vs BED+PARTICLE，逐ID可见支持D；中性全0反控，≥8像素/40% D>=4仅候选，露背景像素不能替床内差 |
| 07 几何alpha | 空/0.15薄床/厚床/state0/1/2/边/重叠；mask不变、alpha二值，halo/最小床厚/软拖尾/漏state2失败 |
| 08 密实数量 | 真实库存只经S改变mask，实际0/1/低/高count；用alpha亮度当质量或增殖假粒失败 |
| 09 尺度 | s1/2/3×四亚像素相位，逐中心/半径/深度/镜像一次；重复scale或按二值面积强求4倍失败 |
| 10 稳定 | 同snapshot/config三次冷热绘制及改frame/time，字节不变；frame-seeded噪声/UP滚纹失败 |
| 11 原子性 | source/config/asset/profile/geometry/context错配及swap失败，验证完整LastGood/实际token；半新帧CURRENT失败 |
| 12 颜色链 | 中性色0/128/145/168/111/255、半透明前景、真实材质，线性FBO/sRGB FBO/窗口分别读；双EOTF/错误预乘/pbuffer冒窗口失败 |

无样本/0分母/全遮挡分别INSUFFICIENT_SUPPORT/UNDEFINED_RATIO/OCCLUDED，不填0冒达标；床亚像素不增厚。数学结构可作硬约束；CR/RMS/D和量化容差按target实测/重复性定标，不能借设计模型数值冻结新产品门。

effector九例规格逐字消费T356 V01–V09：空场、厚床、稀疏落沙、同帧接触、极薄床、无细节消融、错色负控、外halo负控、闪点负控。图由真实新shader重出，设计九格仅合成参考，不能作实机截图；评分同沙色/颗粒/落沙可辨/接触/稳定性分列。动态同run空场→真实触摸→UP→持续下降→LAND→静止和合法倾角另卡，保commit/成功模拟秒/pY/token/frame interval。T359/T360冻结与T361性能未过时如实标动态NOT_EVALUATED，不能用静图签动态PASS。

## 8. 复杂度、风险与分流

正常profile准备O(37×128+128)，支持检查并入既有pack槽遍历，不增加物理audit；投影复用原槽顺序及既有校验（其重复ID检查已有O(N²)，不能宣整条pack严格O(N)）。床片元O(WH)一次R8/exp、粒子6顶点/实例、覆盖最坏O(K*rNative²)含overdraw；实际新增wall/GPU/bytes由独立预算计量，不能承诺1fps自动恢复。shader/asset冷编译生成一次，禁止逐帧glReadPixels/纹理重建/随机点。

本稿无缺失光学公式，无需designer先补才可开发；若真实新shader独立验证表明gain/可辨性不足，失败先回architect判工程/颜色目标/用例，再交designer改光学，不自行改物理或用时间纹理造流动。超支持湿/双沙/AA/输运纹理/质量半径均另版。T362工程交付完成，后续由manager沿真实依赖派developer、独立tester与effector，不新增manager验收门。
