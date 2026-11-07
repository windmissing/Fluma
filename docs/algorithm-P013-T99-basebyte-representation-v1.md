# T99：原生像素X三带F64离散条带表示 v1

2026-10-26，designer；原卡 `6adf27a0e4b017ab340363c9`。路线 `P013_NATIVE_X_F64_BASEBYTE_V2`。状态 **DESIGN_READY_FOR_ARCHITECT / NOT_IMPLEMENTED / NOT_VALIDATED**。不使用真机，不改业务、配方、原始报告或冻结阈值。所有画面仅新XComponent+shader；禁止Canvas含背景前景上屏／回退，失败FAULT或显式STALE上一有效新管线帧。

## 1. 范围与唯一推荐

architect `architecture-T93-basebyte-evidence-routing-v1.md` §1–2已将局部离散表示问题交designer：冻结F64在例点得到baseBlue172，独立GPU debug整数为173。本案只解决 `编码域palette×gain→clamp248→round` 的确定结果与传递；不宣称全部1.956码误差、取样／Q／光照／目标编码或设备颜色已解释。

推荐**CPU按冻结F64参考，为每个原生像素X准备H/L/M三个baseByte，作为整数条带上传；fragment按当前像素X及原带k取整数**。不在GPU重新执行易分叉的palette/gain乘法取整，不以F32影子替换CPU主参考。

关键可分离性：当前配方的fH与sparkGain都只依赖横向列profile采样；在同一个原生像素X和材质带k内，baseByte不随Y变化。Y决定像素是否在沙内和选H/L/M哪个k，B/C纹理及Lmat仍按既有fragment坐标计算。故准备3×W条目与准备W×H全像素base完全等价，且成本为O(W)，**不是128个模型列中心byte的近似**。

这是一项显式新的离散承载方式；签收的底色／混色域、增量／光照／ToneMap公式保持。方案交architect落实prep/type/shader/scene资源接口，developer不提前自行加epsilon或改主参考。

## 2. 已有证据和局部边界

- T88 `t88-stage-probe.json`，debug shader hash `897954e22d96ae53aba1da38d52fcb2bc9716efd11be494cb3f799481e293f68`，(88,78)整数baseB173、grainB102、strataB127、k1。它是整数直证，不是由最终颜色反推。
- 已核fixture邻列gain：Float32(1.3)和1，F64插值中点为1.149999976158142；150×gain=172.4999964237213，F64 floor(+0.5)=172；binary32乘法可到172.5→173。
- T93已限定：debug attachment0为RGBA8，不是SRGB8目标；其238/181/85不能与生产SRGB8图247/219/157直接比较。uniform／bin执行时完整身份、连续量、同目标生产/debug对照由T101补。本设计不替代该补证，也不重新判T88整体颜色PASS。
- 本轮只读算术与SHA、half邻域及同模型列两个原生像素M带不同值的反例，存于 `design-T99-representation-evidence-v1.json`；没有运行GPU／产品测试。旧T77/T88结论不覆写。

## 3. 确定的数值对象：继承冻结F64求值，不换成F32数学

输入必须是同一candidate packet的：完整七版本、实际上传的layout2 Float32位型、accepted palette的原编码域数值、spark开关、几何／镜像、run/stateEpoch/physicsCommitId、accepted配置digest。F64主参考由这些实际可复现输入计算，不能用原始高精度profile代替已上传Float32，也不能从GPU四舍后的uniform重建palette。

### 3.1 逐原生像素X采样

约定原生viewport的X原点vX、W为整数像素；输出索引j=0..W−1，中心pX=vX+j+0.5。以本packet几何的F64值计算：

```text
qx0 = RN64(pX − boxX)
validX = 0 ≤ qx0 < boxWidth
qx = visualInverted ? RN64(boxWidth − qx0) : qx0
r = clamp(RN64(qx / RN64(boxWidth/128)), 0, 127)
i = min(floor(r),126)
t = RN64(r − i)
sample(row,channel) = RN64(RN64(a_i * RN64(1−t)) + RN64(a_i+1 * t))
fH = sample(row1,g); gain = sparkOn ? sample(row2,g) : 1
```

RN64表示参考操作次序中的每一步binary64最近偶数舍入。不得FMA／重结合、改成a+(b−a)t、texture线性过滤或先插值byte；即使这些形式实数代数等价，边界结果可能不同。最终操作顺序应与冻结CPU evaluator逐行对照，architect若发现现有主参考使用另一等价式，先把精确evaluator作为版本合同固定，不能自行选更易实现的式子。

palette的H/L原数值来自同一accepted配置的编码域字节值（可为合同允许的编码数值，不额外量化palette）。M每个channel按冻结顺序：`P_M=RN64(RN64(P_L*RN64(1−fH))+RN64(P_H*fH))`。H/L直接用各自palette，M的grain基准(181,182,131)不改成这个动态底色，也不对H/L先Q再混色。

如果配置只保留归一化sRGB而缺失冻结参考对应的编码数值来源，architect需明确并绑定其原F64转换顺序；**不得**从Float32 uniform×255猜原值、四舍回原palette或悄悄改色。缺输入身份时不生成可激活条带。

### 3.2 clamp／round的边界含义

延续T55冻结CPU的实际求值：

`v=RN64(P_k[channel]*gain)`；`a=clamp(v,0,248)`；`baseByte=floor(RN64(a+0.5))`。

特别披露：实数数学的half-up、JS Math.round与 `floor(RN64(x+0.5))` 在极靠half的浮点数上不保证相同。例 `x=nextDown(0.5)`，最后加0.5可舍到1，floor得到1，而精确数学half-up为0。本revision**继承冻结F64 evaluator的结果，不偷偷把它改成另一种round**。若产品希望另定精确half-up，需另一个明确数学revision及原流程裁决，不能借“修GPU表示”顺手改冻结参考。

因此所有half测试须留乘法前后和加0.5前后的F64位型，不能只写“half下一点应向下”而忽略实际操作图。clamp248在加0.5之前；0/248和外侧有限值同原公式。NaN/Inf、不合法gain/fH/几何仍明确拒绝，不用clamp掩盖非法输入。

## 4. 条带格式、读取与等价证明

建议 `BaseByteStripV2`：宽W、高1、array layers=3，texel为RGBA8UI，RGB存0..248整数，A=validX?1:0；按layer H/L/M、原生j升序存。CPU字节数恰12W。W、vX、surface高度/viewport、box/geometryEpoch、mirror、profile/palette/source配置身份均入metadata和content hash。

native内部格式 `GL_RGBA8UI`，上传 `GL_RGBA_INTEGER/GL_UNSIGNED_BYTE`；shader用highp `usampler2DArray`＋texelFetch整数坐标，0 mip、NEAREST、无sRGB decode/UNORM filtering。GLES整数采样接口依据 [Khronos GLES3规范](https://registry.khronos.org/OpenGL/specs/es/3.0/es_spec_3.0.withchanges.pdf)；这是数据纹理，不是把shader画面换到CPU或Canvas。

shader在既有mask/k判定之后：

```text
j = floor(gl_FragCoord.x) − viewportOriginX
require(j in [0,W) and matching strip identity)
q = texelFetch(baseByteStrip, (j,0,k),0)
require(q.a == 1 and q.rgb in [0,248])
baseByte = vec3(q.rgb)       // 0..248整数在highp float内精确表示
continue existing Q / δB / δC / Lmat / ToneMap / linear output
```

不要用q.x的模型列索引读条带；j是原生像素索引。不要再乘gain、混palette、clamp248或round第二次。validX不匹配须使candidate失败并保留FAULT/STALE身份；不能用返回透明来伪装完整scene成功。工程可在CPU/native预检共享几何和绘制范围避免逐fragment故障表达，但必须留可检查的same-domain不变量。

对任意覆盖内像素(j,y)和既有选中的k，冻结F64主参考base定义为B64(j,k)。CPU严格计算并存入strip[j,k]，上传／整数texelFetch不改变值，所以fragment得到的base与主参考**逐字节相等**。这是本revision的证明目标；不依赖GPU乘法精度、half位置或M带是否呈整列常值。

证明条件：当前profile和底色依赖仅X及k，几何是既有轴对齐盒＋一次180°镜像，viewport为单采样原生像素格，条带与同packet绑定。若以后出现Y相关gain、斜切／旋转使profile依赖Y、sample shading/MSAA多采样或投影warp，本压缩条件不成立，必须拒绝不支持的表示或另版2D／每sample表示；不能继续用X条带近似。

本条带为三个k都准备值，**不代替**H/L/M边界／mask选择。若GPU的k／几何边界与F64主参考不一致，须另有整数探针定位；本证明只保证对应k的base，不把剩余取样／边界问题统称已修。Q/pow/光照/ToneMap/目标编码仍可能有误差，T101与设备A/B/C继续。

## 5. 为什么不是模型列中心准备

设计算术反例：Wbox=256，模型128列，原生像素j144和j145均落模型列72，但采样fraction分别0.25/0.75。相邻fH=.2/.8、gain=Float32(1.3)/1：

| native j | 连续fH | gain | M的F64 base RGB |
|---|---:|---:|---|
| 144 | 0.35000000000000003 | 1.2249999642372131 | [234,228,167] |
| 145 | 0.6500000000000001 | 1.074999988079071 | [183,191,134] |

128列的一组M byte不能表示这两个实际像素；先准备列byte再插值还会产生非原公式的颜色。原生X条带在每个中心先采profile、混M、乘gain、clamp、round，保存每个结果，所以没有该近似。

镜像以原生输出像素j为key，准备时先做与参考相同的一次qx镜像；shader读strip时不再反转j。Y镜像只影响既有band／mask／C3等路径，不重算条带。box端点／r=127的clamp与i126,t1规则保留，不能因mirror出现qx=boxWidth而少采一列或重镜像。

## 6. 版本、配置与事务接口

新增必填 `baseByteRepresentationVersion=P013_NATIVE_X_F64_BASEBYTE_V2` 及 `baseEvaluatorVersion=P1_FROZEN_F64_EVAL_V1`；旧路径标 `P1_GPU_FLOAT_BASEBYTE_V1`，**不能缺字段就默认新版本**。

原七版本分别保持：materialRecipe=P1_ALBEDO_DELTA_V1；slopeKernel=E2_GAUSS_COL6_R18_P1_V2；colorPipeline=LIN_SRGB_REINHARD_V1；textureSampling=P1_TEXEL_CANVAS_PX_V1；strataSampling=CONTINUOUS_COLUMN_WARP_V1；T_dataLayout=2；zOrigin=1。原因是配方、坡度核、Q/ToneMap、B/C采样、原profile布局与原点均未变。新的离散承载身份是**额外必验版本**，进入T75 accepted-configuration digest／完整fingerprint，而非冒充其中一个版本已覆盖。若architect落地实际改变这些旧字段之一，必须随实际合同升级并逐项说明，不能沿本表硬标不变。

建议接口（由architect落工程名）：

`prepareBaseStrip(FrozenFrameInputs) -> {versions, runId,stateEpoch,physicsCommitId,frameToken,geometryEpoch,viewport,boxBits,visualInverted,profileHash,paletteHash,sparkEnabled,configurationDigest,byteLength,contentSha256,bytes}`。

native stage验证元素类型／长度／byteOffset、实际字节hash、RGB域/Aflag、能力及同packet身份；draw绑定stripSerial，回执列requested/actual strip hash/token。新纹理是frame数据，不把每个物理帧强行当新的static material assetGeneration；仍绑定当前grain/strata代与accepted配置。不能用assetGeneration相同证明profile变化后的条带仍有效。

准备缓存key至少含七版本＋两个新版本、布局原点／W／viewport／box位型／mirror／geometryEpoch、layout2实际字节hash、palette原值、spark开关和accepted配置。光向/Lmat OFF/ON、B/C不改变base，可复用同bytes但每frame绑定当前token；spark开关改变gain，须换匹配条带，不能改纹理资产／种子。prepared数据不可引用下一frame会覆写的scratch。

同一scene candidate必须同时具备matching T_data、strip、grain/strata、所有pass的配置与几何，才可Acquire/绘制；异步条带迟到时不以新token使用旧条带。任何prepare/upload/尺寸/版本/能力失败，在完成candidate前丢弃并保留真实FAULT/STALE，不提前CommitSceneLease或伪造swap。LastGood全frame纹理按原T68/T56合同保留，不依赖旧strip冒充当前帧。

## 7. 成本和候选比较

每次profile/几何变更准备3W RGB计算，采样可每j一次共享fH/gain；时间O(W)、staging O(W)，不按W×H重算，不按粒子扫描。shader每个沙fragment增加一次整数texelFetch，删掉palette/gain/round分支；GPU收益／CPU开销均须测，本文不承诺帧率提升。

单strip载荷：W1178→14,136B；W1920→23,040B；W3840→46,080B。若每秒60次全量更新，W1178约848,160B/s，W3840约2,764,800B/s，仅载荷、不含驱动/同步开销；不是实测带宽。建议两个GPU frame槽＋两个CPU staging槽，合计载荷48W，W3840约184,320B，另加metadata／driver开销。显式计入独立baseStripTextureLiveCount/bytes，不藏进原materialTextureLiveCount≤4限制或static G2预算。

GL_MAX_TEXTURE_SIZE／array layer能力须查询；width超过能力时FAULT，或architect定义保持j一一对应的确定分页布局另版工程承载。禁止缩分辨率／改128列byte来“适配”。固定容量资源随surface/context代变化重建，不每帧compile/glGen，无无界缓存。

| 候选 | 数学／成本 | 决定 |
|---|---|---|
| GPU float为主，F32影子替换F64参考 | 可让例点173变“正确”，实际换了主判据 | 拒绝 |
| 加epsilon、调gain/palette或改round阈 | 隐改配方并移动所有half边界 | 拒绝 |
| 128模型列中心byte | 不表达per-fragment fH/gain，M反例已给 | 拒绝 |
| 整屏F64 base atlas | 对每像素可精确，但O(WH)准备／上传，未利用X可分离性 | 可行但不选 |
| shader软件F64／整数多字实现 | 可保持F64语义，但每fragment付出模拟成本，GLSL编译器与次正规/除法舍入证明复杂 | 保留研究备选，不作为本轮最小方案 |
| 原生X×三带F64整数strip | 保持冻结F64，O(W)，GPU只精确取整数，需新frame资源接口 | 唯一推荐 |

## 8. 证明／测试与回architect边界

不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md` 其它内容，仅自身状态有限例外；不改滴答同名清单，不回退他人修改。不使用真机；仅新shader、禁止Canvas含背景前景回退，FAULT/STALE真实。architect先修订material preparation/types/upload/frame/scene工程合同及版本，再developer按其责任hunk实现；本任务不直接改shader或CPU主参考。

必须交独立客观矩阵：

1. T88例点新strip baseB=172，生产与独立整数probe都读取172；旧V1 debug173保留历史。**不要求**最终色立即变为某个旧值：同目标完整uniform、连续量与T101仍需补齐。
2. 任意合法pixel X×三k，strip与冻结F64 evaluator逐字节一致（不是1码容差）；CPU/native若双实现，RN顺序／无FMA跨实现逐例核对，不能native默用float。
3. M连续fH/gain、每列多个native像素、非整数box宽和原点、放大／缩小、r=0/127、左右外边、H/L/M空带／窄带、visual invert一次；给上述j144/145反例。
4. 0/248、m+0.5两侧相邻F64与F32输入、gain插值／palette临界值，记录乘积及加0.5位型；保留floor(RN64(+0.5))与精确half-up区别，不静默修主参考。
5. OFF/ON、B/C开关strip字节应相同；spark开关按其gain正确换表；asset/seed/hash/七版本不变。新增representation/evaluator版本错、profile/geometry/spark错／旧代条带一律拒绝。
6. 实际RGBA8UI上传／偏移／长度／hash及usampler读回整数检查，避免normalized sampler、LINEAR filter、sRGB解码或错layer；重新创建context/resize后强制匹配新几何，不拿旧viewport表成功。
7. 准备失败／异步迟到／upload中断、场景其它pass失败都不能发布半新frame；LastGood/FAULT/STALE与scene lease正确，内存／带宽／p50/p95另测。

数学边界证明目标是base离散值一致，**不是把F32影子升为主参考，也不是把最终颜色误差线放宽**。原numericStatus继续MEASURED_NOT_CALIBRATED；T101目标／连续量及T57设备A/B/C仍独立，设计／开发完成不等于产品颜色通过。若GPU k/texel／输入发生另一差异，回architect按证据分流，不在本局部revision顺手掩盖。
