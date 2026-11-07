# P-013 P1 材质配方与坡度核确认 v1

日期：2026-10-24。责任：designer。任务：[6adc92ba…272e](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adc92bae4b03273ad5d272e)。
状态：**ALGORITHM_RECIPE_SIGNED / READY_FOR_MANAGER_ACCEPTANCE / NOT_IMPLEMENTED / NOT_CALIBRATED / NOT_VALIDATED**。

签收的是下列可实施算法语义；不代表G0已通过、G1已解冻、色彩/效果已验证。本卡纯离线，只新增designer文档与数学参考数据；不修改工程稿、P0、代码、测试或冻结锚点。manager验收后由architect写入P1工程稿rev2，tester用例/RED资产及原门序继续生效。

## 1. 唯一配方、版本及依据

**确认P1采用：DISPLAY_4X3之后只对S做横向Gaussian σ=6列/R=18列，再求显示坡度；材质以低端palette为基色，B/C1/C3扣各自无光照基线后在Q空间相加，统一乘逐像素Lmat，最后一次ToneMap。** 本稿补齐取整、M基线、C3块值、采样/测量定义，不把旧Canvas画面冒充无光照材质。

| 身份 | 本稿签收的确切值 |
|---|---|
| materialRecipeVersion | `P1_ALBEDO_DELTA_V1` |
| slopeKernelVersion | `E2_GAUSS_COL6_R18_P1_V2` |
| zOriginVersion | `1` |
| T_dataLayoutVersion | `2`（沿工程稿128×3布局，不改通道） |
| colorPipelineVersion | `LIN_SRGB_REINHARD_V1` |
| textureSamplingVersion | `P1_TEXEL_CANVAS_PX_V1` |
| strataSamplingVersion | `CONTINUOUS_COLUMN_WARP_V1` |
| 本确认稿/数学参考 | `P013_P1_RECIPE_CONFIRM_V1` / `P013_P1_RECIPE_ANALYTIC_V1` |

这为尚未实现的P1初版定清语义，不给P0同名换义；P0保持layout1/E2_APPROX_P0/zOrigin1。后续改变核、基线提取、C3形状或Q/ToneMap须另版配方/核并重采基线。

唯一输入：[P1工程稿](architecture-P013-p1-material-composite-v1.md) §4.2/§5.2/§5.3/§9/§10；[P013效果目标](design-P013-E2-effect-target-acceptance-v1.md)；[E2开题](algorithm-P012-E2-top-light-opening-v1.md)；现行 `entry/src/main/ets/render/P012WaterSandRenderer.ets` 的palette/位图/带色/显示profile等实现。P0 adapter/reference/fragment只用于核对P0差异，不另起算法来源。源码行号仅定位，以函数/常量及实际hash为准。

## 2. Gaussian σ6：列域、半径、归一化与边界（工作1/2）

### 2.1 输入与核

N=128，列索引c=0…127。先逐字沿现 `buildDisplayProfiles`：raw bh、bh+bl、S、waterTop各做四遍同步三点平均，每遍用旧数组左右钳位、scratch再set；不可原地从左扫右污染同一遍。记结果为H_D、M_D、S_D、W_D，作为layout2的显示几何。**额外Gaussian只读S_D，不写回上述四条profile或物理数组。**

```text
g[j] = exp(-j*j/72), j=-18..18
w[j] = g[j] / sum_{i=-18..18} g[i]
S_G[c] = sum_{j=-18..18} w[j] * S_D[clamp(c+j,0,127)]
dxCanvas = boxWidthCanvasPx / 128
syCanvas = boxHeightCanvasPx / capacity
pb64[c] = (S_G[min(127,c+1)]-S_G[max(0,c-1)]) * syCanvas / (2*dxCanvas)
pbUpload[c] = Float32(pb64[c])
n(x) = normalize(vec3(-interpolate(pbUpload,x),0,1))
```

核生成、卷积及差分用Float64，j从-18递增至18；上传最后一次Float32量化。shader只按原连续列sampler插值pb，不对fragment重做卷积、不插值已归一化normal代替pb后归一化。无可见层或几何非法沿工程拒绝路径处理，不静默换核。

| 维度/边界 | 确切规则 |
|---|---|
| 横向列空间 | 标准差σx=6 **列索引单位**，截断半径Rx=18列，37项、全核一次归一化。对应Canvas宽面尺度6×boxW/128 px，随显示列距换算；不是6px固定屏幕尺度 |
| 高度/深度方向 | **没有Gaussian卷积**：σz=0、Rz=0（恒等算子）。z的exp衰减由ellLight独立控制，不能把σ6当ell或z方向模糊 |
| 左右0/127 | 对每个核tap复制端列；不环绕、不镜像、不删tap后边缘重归一化。差分端点仍除2dx，不临时改成单边dx，因此端点坡度会衰减 |
| 深度边界 | 只有原沙mask内求值。表面极限z→0+；底部h=0仍按本列S算z；h=S本身透明。无深度核、无深度补边，不在沙外夹出一个假表面像素 |

σ6是**光学宽面横向标准差**，不是bh的物理扩散、不是沉积层厚度、不是C层纹周期。四遍三点滤波已先产生显示平滑，故“σ6”仅命名额外的Gaussian，不能把总有效核也称为单独σ6。内域显示四遍核为 `[1,4,10,16,19,16,10,4,1]/81`，额外Gaussian实际截断方差约35.2406243列²，总内域方差再加8/3列²；这些是数学描述，不是新的标定参数。

### 2.2 与两条旧路径逐项对照

| 项 | P0 `E2_APPROX_P0`现状 | Canvas toplight原型现状 | P1本稿 |
|---|---|---|---|
| 输入 | DISPLAY_4X3的S，layout1 | 同renderer显示S | 同一显示S，layout2 |
| 额外核 | 无；sigmaCandidate只是metadata，不进shader | e2EnsureLightMap实际单遍半径round(6)=6的13tap盒平均；“盒×2/材料深度σ”注释不等于实际代码 | 37tap离散Gaussian，σ6列/R18列 |
| 坡度 | shader在x±dx取连续S差分；CPU reference同法，显示px/px | low相邻列差/2，尚未采用同一显示比例换算 | CPU逐列对S_G差分×sy/dx，再上传pb32、片元插值 |
| 深度 | zOrigin1、本列S、逐fragment exp | 三带中点、capacity偏移与LUT近似，不是P1金标准 | zOrigin1、本列未额外Gaussian的显示S，逐fragment exp |
| 材质照明 | 诊断灰阶/透明OFF，未具B/C完整耦合 | 旧底色光照与C/B source-over | 同Rmat、同Lmat、一次ToneMap |
| 版本/可比性 | 保持旧版，不能给P1签效果等价 | 只作原型对照 | 新OFF基线/CPU参考/黄金图全部重出 |

补充：P1在128个列点先差分、再插值，与P0在连续采样位置取x±dx差分在最后一列钳位区不必相同；不能仅因去掉Gaussian就宣称两版逐fragment完全等价。P0源码中没有“σ6深度高斯”可继承。

## 3. CPU可对拍判据与z/C连续性（工作3/4）

### 3.1 输入充分性与参考数据

**只给bh不够确定法线**，因为光面取S=bh+bl+mh+ml。下列fixture同时固定其它层；若bh变而bl反变使S不变，则pb/n必须不变，只有材质mask可能变。这是防止误把重沙内部起伏当沙面光照的反例。

数学输入、权重及选点结果见 [reference-P013-p1-material-recipe-v1.json](reference-P013-p1-material-recipe-v1.json)。这是设计计算参考，不是已跑tester/产品实现测试。统一N128，boxW=128Canvaspx、boxH=capacity=256，所以dx=sy=1；waterTop=256，非倒置、同源无光照材质参数。

| fixture | 已知raw层/或明确阶段输入 | 期望输出 |
|---|---|---|
| 常数 | bh=20、bl=20、mh=ml=12→S=64 | 所有列pb=0，n=(0,0,1)，无额外左右光照分区 |
| 线性坡面 | bh[c]=20+0.2c，bl20、mh=ml12→S=64+0.2c | c=23…104内域pb64数学值0.2；pb32=0.20000000298023224，n(pb32)≈(-0.196116137948,0,0.980580675129) |
| 同坡面边缘 | 同上，完整四遍+Gaussian+端点差分 | c0/127 pb64≈0.04738579465644；c1/126≈0.101346621479；c22/105≈0.199999086249。不能把端点硬改成0.2 |
| raw脉冲 | bh[c]=40+10·1[c=64]、bl20、mh=ml20→S=100+10δ64 | pb64[63]≈+0.01618533041694、[64]=0数学值、[65]≈−0.01618533041694；normal左右相反、中心竖直 |
| Gaussian单元隔离 | **已显示**S_D=100+10δ64，不再做四遍 | pb64[63]=5(w0−w2)≈0.01800228755410，[64]=0，[65]为负；用来区分漏做/重复做DISPLAY |
| 内部层重排 | bh=40+2δ64、bl=20−2δ64、mh=ml20→S恒100 | pb全部0，不能由bh峰产生宽面法线 |

脉冲的独立解析对拍：内域组合核v=w*displayKernel，`pb[64+d]=5·(v[d+1]−v[d−1])`，常数部分消去；这比只比较两份共用实现更能暴露错误。正/负坡镜像时pb反号、nz不变；保持S不变只变waterTop不得改pb。整体同倍缩放Canvas宽高保持sy/dx，pb不变；容量/比例变更按公式重新换算，非等比Canvas→native映射按工程稿拒绝效果采集。

参考文件的pb为Float64计算、pb32为上传值，`normal`明确从哪种pb算；CPU shader参考应消费**实际上传的Float32包**、同坐标、同mask、同fH/warp，避免CPU64对GPU32假称逐位等价。打印小数不是新增“±某误差”PASS线，Float32量化、shader算术、读回/截图各自误差由tester分层测量，manager冻结容差。

### 3.2 zOrigin1与C1

```text
h(y) = (1 - yLocalCanvas/boxHCanvas) * capacity
zMaterial = interpolate(S_D,x) - h(y)        # 不是S_G，不是bh，不是capacity-h
zPx = zMaterial * boxHeightNativePx/capacity
ellLightPx = ellLightMaterial * boxHeightNativePx/capacity
Lmat = lightEnabled ? 1+exp(-zPx/ellLightPx)*(Lface-Lref) : 1
```

额外Gaussian只影响宽面法线；不改H_D/M_D/S_D几何、不重画mask、不改C位图、λ、warp、bandMinY/MaxY或层纹相位。C1仍按工程稿的CONTINUOUS_COLUMN_WARP_V1与64×256字节采样；不是把C图也用σ6横向/纵向模糊。Lmat随真实z连续，并在H/L/M内部沿用同光场，不能按三带中点各算一档。C1 warp texel取整带来的相位台阶仍要实测，不能把“连续插值warp”写成绝无台阶的证明。

在B/C OFF诊断臂，Rbase固定且Lface−Lref符号固定的同材质带内，Lmat向深处连续趋1，输出Y向该像素OFF值收敛；亮面可能向下减亮、暗面可能向下回亮，不能一律要求Y随深度递减。H/L/M真实颜色边界仍可跳变。C3另有样式块边界，见§5：必须单列，不能当成Gaussian或z核的连续性证明，也不能靠模糊C1压掉它。

## 4. Rbase与B基线：源码路径和确定取色（工作5/6）

### 4.1 Rbase确认与不等价处

**确认H/L取palette低端基色，不调用现bandColor输出取色。** palette函数在同renderer内 `p012DefaultPalette()`：H=(145,168,111)、L=(216,196,150)字节；高端Hi分别(188,205,158)/(238,226,192)，是旧伪法向路径，不进入本版Rbase。

令k=0/1/2对应H/L/M；fH沿layout2由raw mh/(mh+ml)、空混合取0.5，再按约定连续列sampler插值；不把M基色固定成0.5混合，也不先在Q/linear空间混H/L：

```text
P0H=(145,168,111); P0L=(216,196,150)        # 不含Hi/旧shade
P_k = P0H if H; P0L if L; (1-fH)*P0L+fH*P0H if M
s[c] = sparkTile[(c%64)*64+(c%64)]        # 现buildSparkTile、同seed规则
gain[c] = 1 if spark消融 else (s[c]>0 ? 1+0.5*s[c]*0.3 : 1)
g(x) = continuous_column_sample(gain,x)   # layout2现有sparkGain
CbaseByte[channel] = round_nonnegative(clamp(P_k[channel]*g(x),0,248))
Cbase_srgb = CbaseByte / 255
Rbase = Q(Cbase_srgb)                     # 逐RGB分量
```

`round_nonnegative(v)=floor(v+0.5)`，与现 `rgb()` 非负输入Math.round一致；先乘gain再clamp248再round，不能把gain乘在Q空间或light之后。gain/fH保持layout2既定插值，不增新通道。H/L/M空带不画，不能以空带0.5配色渗出mask。实际palette/seed常量须进manifest，不用上面说明值绕过实际配置hash。

源码现状差异须保留：`bandColor(kind,fH,shade,col)`先由低端→Hi按shade插值，再用列级gain/clamp248/`rgb`取整；caller `drawSandColumns`还会按游程起列c0取色。P1去掉Hi/旧shade并改逐fragment fH/gain，所以OFF不是旧Canvas E底色的逐位复制。不能将旧亮色读回后当Cbase再乘一次Lmat。

**另一个已核实的跨栈差异：** 旧 `drawSandColumns` 在三带底色之后还画二维8px tile稀疏spark（rgba(255,252,240)、三档alpha），与`bandColor`的列级gain是两条不同代码路径。P1工程§2/§4.1及本次签收的Rbase公式明确承载的是**列级sparkGain**，不是这层二维overlay。本版P1_ALBEDO_DELTA_V1不暗补旧二维overlay，也不让Canvas在GLES后再画沙面亮粒；这项风格差异须由manager在本稿验收时知悉、architect在rev2跨栈差异清单登记，并按§8重采B/E基线。若需保留二维overlay的效果，另明确资产/基色配方与版本，不能developer临时后叠一层破坏同光场。这不改变工程稿绘制责任或宣称旧EAB重新验收。

### 4.2 grainBaseSrgb和B增量

`buildGrainBitmaps`/`buildGrainBitmapsFor`分别使用默认σ或activeAppearance.grainSigma，调用`makeGrainBitmap*`；每texel RGB为 `clamp255(rawGeneratorBase+σ·grainTile[i])`，alpha255。原始无光照字节应在putImageData前同源导出，不读Canvas合成结果。

| k/层 | 位图生成器raw基色 | **grainBaseSrgb[k]定义为零噪声时的实际字节/255** |
|---|---|---|
| H=0 | palette.H | (145,168,111)/255 |
| L=1 | palette.L | (216,196,150)/255 |
| M=2 | (palette.H+palette.L)/2=(180.5,182,130.5) | **(181,182,131)/255**，沿clamp255的round；不是runtime fH色 |

这是对§5.2减数的取整补全：manifest的grainBaseSrgb存**经clamp255量化的零噪声基线**，生成器加噪仍用raw半整数，不能先把rawM四舍五入再生成整张图，否则改变旧资产字节。raw平均值可由palette重建/另记metadata，不把两种值混名。这样零噪声的byte sample等于减数，δB严格为0；若减180.5却采181，零噪声仍会有微小非零偏置。

```text
δB = aB * (Q(BsampleByte/255)-Q(grainBaseSrgb[k]))
aB = 0.75（现P012_B_GRAIN_ALPHA）
```

不减当前Cbase（其受fH/spark影响），不减Hi，不把aB再烘进字节alpha。B消融置δB=0，资产/seed/坐标不变；aB是Q空间增量权重，不再冒称Canvas source-over覆盖率等价。M即使fH=0/1也仍采M seed/减M资产基线，不能跳H/L贴图以追色。Bsample RGB分量可能因clamp不同而非纯灰差，必须逐RGB处理。

## 5. C1/C3配方与统一组合（工作6）

### 5.1 C1

确认现 `makeStratBitmap` 的不透明灰字节 `gray=clamp255(128+env·amp·stratNoiseAt(noise,phase))`；neutral为**字节128/255**，不是线性0.5/编码127.5。amp/λ/envelope/三seed沿当前appearance生成器，不另定默认。C采样严格沿工程§4.3的Canvas坐标/warp/正mod/strataOrigin定义。

`δC1=0.5·[Q(Csample_srgb)−Q(vec3(128/255))]`；同一Q作用到各RGB。Csample为128时增量严格0，不把128底灰再加到Rbase。纹理内部的env和amp已烘入，不二次乘strataStrengthScale或alpha。

### 5.2 C3：旧固定样式采样，非第二光场

每材质带用现bandTopY/bandBottomY遍历全列取minY/maxY，坐标是pre-invert Canvas绝对y；H范围0→H_D、L为H_D→M_D、M为M_D→S_D，再经现`heightToY`。沿旧空带判据maxY−minY≤0.5Canvaspx不启用C3；mask层序保持工程§5.3。这里0.5是既有绘制判据，不是本稿新增效果容差。

```text
Nbands=2; subH=(maxY-minY)/2
block b=floor((yCanvas-minY)/subH), b=0/1（仅带范围内）
shadeIdx[b]=round((1-(b+0.5)/2)*64) = [48,16]
x=shadeIdx/64
shade=(2.2*x/(1+2.2*x))^(1/2.2)           # 现buildShadeLut公式
C3Byte[b]=clamp255(255*shade) = [206,159]
δC3=0.10·[Q(vec3(C3Byte[b]/255))-Q(vec3(128/255))]
```

源码LUT实际为**65项(0…64)**，不是注释中的“8档LUT”；这里采两个固定索引，与pb、Lface、E2开关、实际深度exp没有关系。旧公式含tone/gamma近似，但在P1这里只用于生成固定**样式灰字节**，后续仍按唯一EOTF/Q/T色彩链；不能把旧shadeLut再当最终照明或最终输出gamma套一次。

两个δC3约为(0.13370916946,0.02554165571)，三通道同值。它们不是零均值/中性两块，而是相对128基线的两档正增量。保留这点供OFF风格审查，不擅自去均值、反号或改块值。C消融同时令δC1=δC3=0；内部测量可单列C1-only/C3-only诊断，但不新增产品面板状态。

旧C3在带高度一半处有真**样式阶跃**，不是H/L材料边界。不得从全材质`C_band`报告中删去这一位置伪装连续；光照专用B/C OFF臂可隔离它，完整C开启臂必须另报C3边界阶跃。若效果不接受该阶跃，须升级materialRecipeVersion后改连续样式，并重测C1；本稿确认的是本版唯一配方，不预签该效果通过。

### 5.3 全式与必须保持的不变量

```text
Rpre = Rbase + (δC1+δC3) + δB
Rmat = max(vec3(0),Rpre)
Lface=clamp(La+Ld*max(0,dot(normalize(-pb,0,1),normalizedLightDir)),Lmin,Lmax)
Lmat= lightEnabled ? 1+exp(-zPx/ellLightPx)*(Lface-Lref) : 1
outLinear=T(Rmat*Lmat); T(t)=t/(1+t)
# 沙alpha1/非沙全0、sRGB framebuffer硬件OETF一次，沿工程§5.1/§5.3
```

只有最后一次下界max，不把每项负增量先clamp掉；无额外Rmat上界、曝光或分材质第二光场。OFF与ON的Rpre/Rmat、所有材质资产/phase/seed/消融一致，只有lightEnabled不同。B/C均中性且C3关闭时应严格退化Rmat=Rbase；T(Q(c))应回到经上界保护的EOTF(c)。旧源覆盖 `(base/C1/C3/B)` 不与本式等价，尤其Q凸性会使对称编码噪声也不再统计零均值；不暗做均值补偿或调曝光“匹配”旧图。

## 6. Q的8bit适用性、精度与局部风险报告（工作7）

逐分量c=byte/255，沿已冻结色彩链：EOTF(c)在c≤0.04045取c/12.92，否则((c+0.055)/1.055)^2.4；`L=min(EOTF(c),1−1/4096)`；`Q=L/(1−L)`，上界4095。**确认8bit下有限可用，但不能推断风格/暗面纹理自动安全。**

| 输入字节 | EOTF线性值 | Q参考值 |
|---|---:|---:|
| 0 | 0 | 0 |
| 128 | 0.215860500114 | 0.275283288427 |
| 216 | 0.686685312435 | 2.191679291427 |
| 248 | 0.938685728458 | 15.309416630893 |
| 254 | 0.991102097114 | 111.386032168802 |
| 255 | 原1，保护为0.999755859375 | 4095 |

在不经滤波的8bit输入下，仅255触发上界；palette经248上限本身不触发，B轻沙+噪声却可到255。254→255使Q跃迁约3983.614，远大于普通低端1码差；B系数0.75仍会放大亮粒。Float32在4095量级ULP约0.00024414，Q表/CPU64/高精度shader差异须独立量测，不能由打印小数直接冻结误差界。byte255逆变换线性值应为保护值，最终sRGB约254.9726码、round仍255，**字节round-trip相同不证明没有触发保护**。

256项Q表若优化，只能由同分段公式生成并最后量化Float32、记录实现hash；base/fH/gain取整、上传纹理和手动EOTF不变。采用其它gamma、改变255保护或截断亮粒须另版，不能为风格修复偷偷改§5.1。

每个H/L/M及迎/背光ROI，在固定四场景/同snapshot的OFF/ON/消融臂均报告：

- **Q上限触发率**：对每次Q运算的输入按Rbase/Bsample/Bbaseline/C1/C3/neutral分别计 `EOTF(c)≥1−1/4096` 的component数/有效component总数；另报任一channel触发的沙像素率及各资产唯一texel率，不能把重复采样次数和资产占比混为一项。
- **Rmat下界触发率**：Rpre<0的component数/总component数、任一channel被裁像素率；另列等于0的数量、Rpre最小值/分位值，不能把本来0与被负裁0混同。分材质/暗面统计，不只全帧平均。
- **暗面幅度**：§7定义的dark B高通/C带纹绝对RMS、未归一显示码幅度与归一保留率、CPU预期；另报ToneMap后近饱和值分布，不自行设“近白”PASS阈值。

Q触发率OFF/ON应相同（输入材质同源），Rpre负裁率亦相同；否则可能纹理/phase/配方在光开关间变动。统计一致是结构性核查，不是要求触发率=0。限幅比例、最小暗面幅度、CPU误差容差均须tester重复性/manager冻结；不能凭全帧SSIM给局部暗面丢纹理开绿灯。

## 7. 五项可测定义与采样方法（工作8）

### 7.1 共同方法身份/输入

测量methodId建议 `P013_P1_MATERIAL_COUPLING_YCODE_V1`。显示主测 `Ycode=0.2126*R8+0.7152*G8+0.0722*B8`，单位8bit sRGB显示码（0…255），**不对截图再EOTF后仍叫同一个Y**。CPU先按唯一色彩链输出sRGB参考，再用相同Ycode；Ylinear单独作色彩链诊断，不混入主测分母。

同一physicalSnapshotHash/FrameToken/seed/assetGeneration/palette/appearance/light参数/geometry/原生尺寸/色彩profile，保存P1 OFF/ON×B开关×C开关诊断格（B/C默认是同资产delta系数消融，不重烘、不变seed）。M配比及材料深度匹配；纯沙ROI排除前景雾/高光/粒子/边框、真实材质边界和滤波外域。遮挡mask统一，各臂用**同一支持集**，不得ON结果出来后挑好看的暗面。迎背光由几何n·ℓ与固定光向指定，不从图像亮度排序。记录每ROI材质、z分布、像素数、边界图、B/C/光开关与maskRevision。

静态四类黄金场景各取稳定同帧身份、至少3次独立渲染/读回（沿上游要求），不以时间均值磨掉颗粒；动态倾斜/倒立/重载用脚本预定frameId/snapshotHash逐帧输出，再报时间分布P50/P95，不在ON曲线上选峰帧。曝光/尺寸/色彩状态变动即断配对。PNG原生无损主测，JPEG单列敏感性。CPU中间输出、GL读回、最终三层合成截图分别保存，不把compositor误差归成算法误差。

记F为固定状态图，t∈{OFF,ON}表示光，X∈{B,C}表示纹理；`I_X^t=Ycode(F,t,X开,其它纹理固定)−Ycode(F,t,X关,其它纹理固定)`，输入同源。对B，`H_B=I−G_B*I`，G_B为归一离散Gaussian，σB=active grainFeaturePx×Canvas到native等比尺度，R=ceil(3σB) nativepx，两方向同核；在原生像素不重采样，ROI内缩支持半径后才取RMS，禁止跨mask补零造高频。σB是**测量频带定义**，与σ6列核独立；配置改变重登记对应测量频带。对C，不用B高通代替层纹：沿固定同材质(x,z)支持的行均值剖面，减去预注册最小二乘常数+一次深度趋势后计算RMS，另保留原C1层纹prominence/周期/row-mean指标。

分母为0、ROI空、趋势拟合秩不足或未匹配时输出 `EMPTY_ROI / UNDEFINED_RATIO / INSUFFICIENT_SUPPORT`与原始量，不用EPS造比值/默认1。近零信号是否低于可测性由tester读回重复性标定，不由本稿新设SNR线。

### 7.2 输出定义与判定方向

上游记号 `R_B,dark` 的dark原为ROI下标。本稿将“R_B保留率”和“dark绝对幅度”拆成两个输出族，以满足任务五项定义；没有增加一条新的效果通过线。

| 量 | 精确输入/输出与单位 | 方向/判定属性 |
|---|---|---|
| **R_B(Ω)**，含R_B,dark | `A_B^t=sqrt(mean_Ω(H_B(I_B^t)^2))`，码值RMS；底材 `b^t=median_Ω Ycode(F,t,B关,C固定)`。归一幅度`N_B^t=A_B^t/b^t`；`R_B=N_B^ON/N_B^OFF`无量纲。亮/暗面分别报，不用全沙区一个均值 | 纹理必须可测、相对同配方CPU参考一致，不能凭接近1直接通过。[0.8,1.2]仅上游扫描域，未冻结 |
| **dark**绝对幅度族 | 固定Ωdark输出`darkBOn=A_B^ON`、`darkBOff=A_B^OFF`及CPU值；C用上述去趋势行均值RMS输出darkCOn/Off，单位显示码RMS；另报C1 prominence等原指标 | 用来暴露“比值还好但绝对暗面纹理消失”；不是越低越好。非零/最小可辨幅度及误差带待重复性/manager冻结 |
| **Q_couple,X**，X=B/C | 对材质/z匹配的Ωlit/Ωdark，`Q_X^t=A_X^t(Ωlit)/A_X^t(Ωdark)`；并报无量纲`D_X=Q_X^ON/Q_X^OFF`、各A绝对值及同输入CPU预期。B用H_B RMS，C用去趋势层纹RMS，不把两者频带混用 | 比较实测Q/D与CPU预测方向/误差，而非强求Q>1。后叠贴纸可能D≈1，但正确模型也可能在某些材质上接近1；须用有可辨光场差的成对场景，不凭一比值推论结构 |
| **C_band**连续性向量 | B/C OFF光照隔离臂，在固定x、同材质内部扫z，报Y_ON−Y_OFF与CPU曲线、相邻nativepx梯度、梯度残差。对x横扫，在物理列边界±1nativepx与列中央匹配支持计算`g_edge=P95(abs(ΔxY))`、`g_mid`及比值；真H/L边界单列排除，C3样式边界仅在全材质臂标记另报、不可删去 | 隔离臂应按Lmat规律连续向OFF收敛、无CPU之外列锁定阶跃；完整C臂同时保持C1可辨。阶跃事件计数若需阈值由tester先定，不在本稿预设降幅/比值PASS线 |
| **ΔY_face** | 对同材质/z/遮挡支持的迎背光ROI，`ΔY_face^t=median(Y_t,Ωlit)−median(Y_t,Ωdark)`，显示码；另报`ΔΔY=ΔY_face^ON−ΔY_face^OFF`、CPU预期/参考目标距离/ROI材质构成及像素数 | 光向镜像的新增面级差应按CPU方向交换；平沙不新增伪左右差。8–20/255仍是旧扫描域，不能本稿转PASS线；不是单纯“越亮越好” |

注意：`T'(LR)`对材质增量的局部响应为 `L/(1+LR)^2`，其后还有OETF非线性，所以“暗面纹理绝对幅度一定下降”或“正确R_B必须等于1”都不成立。方向性只在具体fixture/CPU预测下判断，不能用直觉替代配方参考。

**可先立的结构/形状条件：** OFF严格Lmat=1、相同资产/seed、同一Lmat乘全部材质、neutral增量0、flat/mirror法线关系、无额外深度/层纹模糊、同相同坐标CPU公式及mask规则。**必须等实测才立的数值线：** GPU/CPU/截图误差、颜色ΔE、R_B及dark幅度、Q_couple比值误差、C_band阶跃事件阈值/列比、ΔY_face可读幅度及跨帧波动。tester在G0/G3/G5重复性与参考中出分布，designer据此审效果，manager正式冻结；未冻结只报 `MEASURED_NOT_CALIBRATED`，不记VALIDATED。

## 8. OFF跨栈基线重出与门序（工作9）

P1 OFF是完整GLES材质pass、Lmat=1，不是P0透明OFF，也不是Canvas旧OFF。低端palette/去旧Hi光照、量化M基线、Q增量、非source-over、C连续warp、无旧二维sparkoverlay及sRGB/合成栈都会改变输出；不得从Canvas EAB CLOSED、s133 E3 PASS或P0热图继承P1效果分数。

须在同四类场景、相同尺寸/参数/快照重新采：

1. P1 OFF及P1 ON的H/L/M颜色/亮度分布、色块CPU/GL/屏幕profile；Q上限/负裁统计与二维spark差异对照。
2. B暗/亮面绝对高频/RMS、同ROI OFF→ON保留率、旧B lumStd/HF适用锚点；C1层纹prominence/row-mean/周期/有效层厚、C3边界，OFF也是新分母。
3. ΔY_face、C_band、Q_couple各臂与CPU/Canvas旧OFF/Canvas原型对照；不能把OFF变暗导致的ON/OFF提升冒充新光照收益。
4. 三层合成的水雾/界面/非沙ROI回归；GLES身份下重新采E3 J95/J90/std等原核心锚点的分子分母，阈值本身不改，适用性由既有流程核对。Canvas水雾公式保留不等于系统三层合成必定不变。
5. 同温同尺度的Canvas/P0/P1 OFF/ON CPU/GPU/上传/呈现/资源恢复基线，不能从P0小包性能推P1近1MB包、不能把效果签收当90Hz已达成。

旧报告原PASS/CLOSED保留原renderer/build/默认配置身份；新栈结果未测即NOT_IMPLEMENTED/NOT_VALIDATED、空ROI如实报。全帧SSIM只能旁证，同图ROI局部纹理和色彩链证据不可省。

交接：manager验收本算法确认→architect将§2–§7的定稿/差异写入P1 rev2→tester登记CPU fixture/色彩/材质耦合用例与RED→按既有§9 G0前置probe/G1资产CPU参考/G2/G3/G4/G5门序授权实施。**本稿不自行解冻G1、不跳过G0、G0/G5真机由effector队列独占。** 若二维spark差异或C3/Q导致风格不通过，回designer配方迭代，不由developer临场调gamma、删颗粒或模糊层纹。

## 9. 设计自检、候选取舍与变更记录

已用独立离线算术复核权重归一、常数/坡/脉冲/边缘参考、Q表与C3两索引字节；数据在参考JSON。没有跑P1实现、没有真机色彩结论或新数值容差。复杂度为Gaussian O(128×37)、pb O(128)、固定长度临时数组；材质单fragment各取两纹理样本及常量C3/Rbase，没有深度卷积。帧时需G5实测，不根据操作数预签。

候选对比：延续P0 raw差分不满足宽面尺度；沿Canvas盒核与错误单位不满足精确定义；**选列域Gaussian**，拒绝bh/纹理深度模糊。材质选独立Q空间基线增量；拒绝直接加两张带底色RGB、读回旧受光Canvas、保留Hi后再照光及先ToneMap后乘L。C3保留固定样式并显式披露阶跃，未偷偷换成连续渐变；如以后改须新版。复盘见 [retro-P013-p1-material-recipe-v1.md](retro-P013-p1-material-recipe-v1.md)。

- 2026-10-24，v1：完成九项确认及确切版本号。补充σ6列向/行向恒等边界、分阶段CPU对拍、palette/round/clamp248路径、M零噪声字节基线、65项LUT的C3固定块、Q精度/统计、五量可测定义和OFF重采纪律。只新增文档，既有工程冻结责任/layout2/唯一色彩链/mask/门序不变；manager验收与architect rev2登记之前不声称实施已解冻。
