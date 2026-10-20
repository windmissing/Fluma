# P-012 渲染表现层设计 v1

日期：2026-10-20。作者：designer。技术路线：`p012-render-material-v1`。状态：**DESIGN_ONLY / READY_FOR_ARCHITECT_REVIEW / PERF_NOT_VALIDATED**。

目标：在现有128列物理场及几何基础上，补齐浅色柔光、界面斑驳云雾和粉质颗粒，再增加层理与随流纹理。设计依据为已验收的 [render-gap-analysis-P012-v1.md](render-gap-analysis-P012-v1.md)。第一梯队覆盖约80%第一眼差距是effector的主观预判，不是已测收益或验收通过结论。

**落地顺序固定：E光照/色调 → A云雾 → B颗粒 → C层理 → D流痕。** E是A/B显示前提，不能先在原暗底上堆噪声再用曝光补救。本稿只定义渲染方案，不实现、不改变物理或测试用例。

## 1. 边界、路线隔离与现有能力

### 1.1 两条红线

1. **只读物理场。** 所有颜色、雾密度、纹理坐标、光照和形变缓存均属renderer私有状态，不写回`WaterSandField`/`FluidField`，不调用step/inject/invert180，不改128列、capacity、几何、材料库存、通量、dt或物理随机源。不通过shader/纹理位移伪造全局坡面、重沙≥2px位移或P-011收敛。
2. **外观与物理验收隔离。** P-012数据不进入checker、物理快照schema、P011/WS遥测或物理routeId。`appearanceRouteId=p012-render-material-v1`仅是渲染配置和离线视觉证据manifest标识；原物理routeId保持原值。正式物理checker继续使用其既有canonical渲染/证据路径，不能把改色后的图送入旧绿色阈值checker再调整阈值求通过。P-012验证走SV-05/06/07与基线关键帧，附离线视觉测量，不增改物理checker。

关闭外观路线应恢复原renderer输出；同初态、输入、物理step序列下开关外观不得改变物理快照。必须保留renderer选择入口，模式选择不隐含切换物理路线；禁止用更好看的P-012截图替换历史物理审计图。

### 1.2 本地接口核对

- `entry/src/main/ets/pages/SandCanvas.ets::renderWaterSand(ctx, snapshot)`已有水沙绘制入口；当前复制并平滑profile后按列填色，沙顶3px硬描边会妨碍柔和过渡。P-012在自己的渲染分支替换材质/硬描边，不叠加额外轮廓平滑或抬高sandTop。
- `WaterSandSnapshot`含`cols/capacity/waterTop/bh/bl/mh/ml/suspH/suspL/hs/hl/sandTop`；**没有速度场、粒子坐标或沉积时间历史**。不能在方案里虚构`velocity[c]`字段。
- `renderHybrid(ctx)`读取`FluidField.heavy/light/ero`与`total(c)`。这些不是水沙四层账本，不能把`ero`解释为真实`suspH/suspL`。P-012主要接入watersand；hybrid只经独立只读adapter复用材质功能，使用另一证据标签`p012-render-material-v1-hybrid`，缺失层字段的功能标为N/A，不伪造库存。
- 本地OpenHarmony SDK `26.0.0/ets/component/canvas.d.ts`已声明Canvas2D的`Path2D/clip/createLinearGradient/createPattern/drawImage`、`ImageBitmap`和离屏画布。采用这些既有接口；不依赖浏览器WebGL、DOM、CSS滤镜或未核实的自定义fragment shader。Canvas图像绘制/离屏用法可参照[OpenHarmony官方Canvas文档](https://gitee.com/openharmony/docs/blob/87dfd55a178780683e9b58c0f5742691c773f936/en/application-dev/reference/apis-arkui/arkui-ts/ts-canvasrenderingcontext2d.md?force_mobile=true)。实际重载以项目SDK为准。

## 2. 只读材质输入和渲染管线

| 渲染量 | watersand来源/计算 | 使用项 | 无数据或退化处理 |
|---|---|---|---|
| 表面、液面及分层边界 | `S=sandTop`、`W=waterTop`、`h1=bh`、`h2=bh+bl`、混合层厚`mh+ml` | E/A/B/C/D | 空相不绘制；使用当前renderer坐标映射和既有边界，不新增物理列 |
| 材料配比 | 混合层`fH=mh/max(mh+ml,eps)`；有序层按bh/bl实际区域 | E/B/C | 混合层为空时不取0/0，不生成混合块 |
| 悬浮浓度代理 | `qH=suspH/max(W-S,eps)`、`qL=suspL/max(W-S,eps)`，仅显示映射时clamp到[0,1] | A/E/D | 水深≤eps时雾/悬浮纹理为0；clamp不改库存 |
| 局部坡度/近似法向 | 只读S的中心差分，换算到当前屏幕单位；边界用单边差分 | E/D | 平面给统一漫反射，端点无越界访问 |
| 材料运动估计 | 前后两份只读相库存快照，见§5.2 | B/D及A纹理相位 | 无可信对应关系则冻结纹理位移，不用倾角或时间正弦编造流动 |
| 屏幕比例、倒立状态 | 既有box变换、capacity映射和视觉倒立事件 | 全部 | vp与截图px显式转换；不再调用物理invert |

推荐结构为类型明确的`MaterialRenderInput`、`MaterialPalette`、`MaterialTextureCache`和`MaterialMotionCache`，在ArkTS中使用显式interface/class及预分配TypedArray；不使用`any`、动态属性或逐帧临时对象拼装。

画面合成顺序与功能交付顺序分别定义：交付必须E→A→B→C→D；每帧绘制可按**E底色/漫反射 → C层理底调 → B沙粒（D变换作用于同一纹理）→ A界面/水体光学散射 → E柔和高光与容器边框**。这样雾覆盖颗粒但不被硬沙面描边截断；E给材质的最终颜色曲线贯穿整条管线。

```text
render(readonly snapshot, existing frame context):
    copy required arrays into reusable render buffers
    derive phase masks, 128-column boundaries, concentrations, normals
    update render-only motion coordinates from consecutive valid snapshots
    draw E palette + broad lighting within existing phase masks
    if C enabled: draw layer-local brightness bands
    if B enabled: draw cached grain; if D enabled apply motion anisotropy
    if A enabled: draw mottled interface scattering and water haze
    draw bounded E highlights; restore Canvas state
```

物理场未变时不推进噪声时间；主渲染仍逐帧响应现有快照。截图/录像、诊断统计由外部采集完成，不在这里增加遥测。

## 3. E — 光照与色调映射（第一步）

### 3.1 实现

采用**受限的二维漫反射+低饱和材质调色**，先更换材质基色，不对整张屏幕（含UI）套曝光滤镜。候选基色来自effector目标ROI：重沙浅黄绿`(145,168,111)~(151,175,126)`，水体近薄荷白`(192,202,196)`附近；轻沙维持可辨的暖浅色相，混合层由fH连续混合。不得把重轻两相涂成同色来凑亮度。

以屏幕单位坡度s构造伪法向`n=normalize(-s,0,1)`，固定柔和左上主光`l`，令`L=La+Ld*max(dot(n,l),0)`；候选`La=0.85,Ld=0.15`。这是表面明暗代理，不是真三维法线。广域亮度变化用低频缓存light-map，固定种子、随容器坐标；同一平面不产生逐列明暗棋盘。

材质颜色用预计算LUT实现低饱和与柔和高光肩部，候选压缩函数`T(x)=x/(1+x)`、`x=exposure*linearRadiance`，最后编码为sRGB。纯底色可预离线求目标值；实际热路径采用少量亮度档的预着色纹理/gradient，不每帧全屏读像素做tone mapping。**去掉现行沙色127上限**，保留到约240～248的高光空间；不靠一个白色坏点满足max。

液面高光用沿W的宽软带，沙面用稀疏亮粒与坡度调制；“焦散”仅用固定光照方向的低频浅亮斑模拟，透明度候选≤0.12，不做光线追踪。水体的发白主要来自高调基色和A的散射，不能以全屏白雾覆盖所有材料。

### 3.2 验收与预算

- 锚点：hybrid水体p50 **87→约200**（参考191～210），水体p95−p50由0向22～47展开；沙面p50由97向155～162，max **127→约240**，参考p99达221～248。E单独先验色调与高光；水体纹理方差须在E+A联合验收，避免用一条渐变冒充云雾。
- 亮部不能大片饱和成纯白，水/重沙/轻沙仍可辨；曝光改变不能掩盖瞬移或界面裂缝。
- 估算：`O(128)`法向/档位计算，≤16次材质/高光绘制增量，CPU目标≤0.25ms/帧；LUT与light-map≤0.25MiB。数字是待真机验证的预算，不是已测耗时。

## 4. A — 云雾与界面光学过渡（第二步）

### 4.1 带纹理的过渡，不改材料边界

在沙水界面使用到已有S轮廓的屏幕法向距离d，定义光学混合权重：

```text
t = clamp((d + wFull/2) / wFull, 0, 1)
waterWeight = t*t*(3-2*t)
color = lerp(sandSurfaceColor, waterColor, waterWeight)
```

候选10%～90%亮度过渡宽度为**24～48个归一化截图px**，以完整沙区高度约350px的当前素材为参考，随展示尺寸等比换算；smoothstep的10%～90%宽度约为`0.6084*wFull`，故不能直接把24～48当wFull。近界面亮度差约40～50时，平滑包络本身的最大导数约`1.5*ΔY/wFull`，给maxGrad<8留余量；最终梯度仍以加噪后的截图为准。

这是有限宽的光学散射/遮盖，S仍是相边界，不创建新的沙体轮廓或悬浮质量。带宽受实际水深、可用沙区厚度和容器裁剪约束；浅水时允许显示带变窄，不能为通过宽度锚点画出容器或覆盖整个相。原黄色3px沙顶硬描边在P-012分支移除；不扩大原重轻相物理混合厚度。

### 4.2 斑驳密度

采用固定种子的两尺度低频tile：约12～24px细团、40～80px大团，采用平滑插值产生零均值噪声`n1/n2`。只在renderer中构造：

```text
q = clamp(qH+qL, 0, 1)
tau = tauOptical(d) + kH*qH + kL*qL
tauMottled = max(0, tau * (1 + a1*n1 + a2*n2))
alphaFog = 1-exp(-tauMottled)     // 用LUT或预生成alpha纹理采样
```

`tauOptical`是界面附近有限支撑的材质散射，不代表永久悬沙。水体内的动态雾浓度和颗粒可见度随q衰减；q归零后停止流动雾，仅保留静态、低对比的水介质光照斑驳，不制造持续漂浮粒子。清水的静态明暗变化归E，不能冒充有悬浮沙。

两尺度噪声错开固定相位，A的底层相位随可信材料运动估计移动；静止时不按时钟滚动。最大alpha候选0.55，界面颗粒经包络平滑衰减，禁止均匀水平方向linear-gradient单独充当云雾。A和B必须联合检查，防止颗粒把过渡剖面重新变成硬边。

### 4.3 验收与预算

- effector锚点：过渡宽度**<8px→数十px**；maxGrad **约46→<8**（参考约4.9）；水体内部std **0→>15**，参考云雾带std32.9。宽度/梯度按同一界面剖面与分辨率检查；水体std锚点用于匹配的有雾关键帧，不要求无悬沙清水凭空永远“沸腾”。
- 不应出现均匀彩虹带、重复圆斑印章或静止闪烁。30s静置后的材质浑浊与真实悬浮消退分别判断。
- 估算：128列包络；16组（每组8列）共用缓存噪声裁片与渐变遮罩，至多48次绘制；CPU目标≤0.35ms/帧，纹理/小遮罩≤0.75MiB。遮罩跟随128列原轮廓，纹理分组不减少物理或轮廓采样数。

## 5. B — 粉质颗粒与随材料移动的噪声（第三步）

### 5.1 颗粒分布

初始化生成可平铺的256×256颗粒tile，使用renderer独立固定种子：1～2px细粒为主、3～5px弱团簇作包络，两张不同质地tile混合以减轻平铺感。单位方差、零均值亮度噪声`nG`被截到有限范围，候选亮度标准差幅度18～22级，再稀疏叠加0.5%～2%的柔和亮粒。它作用于E已提亮的材质，不是等概率黑白雪花。

```text
Ygrain = Ymaterial + sigmaG(phase)*nG(U,V) + sparseHighlight(U,V)
sigmaG = phaseBlend(bh/bl region, mh/(mh+ml), palette settings)
```

重沙、轻沙、混合区各有独立种子/对比度，配比只改材质组合；所有grain严格clip到相mask，界面处由A包络衰减。高光经过E肩部压缩，不能过曝成碎白边。近看有颗粒，整屏仍是连续沙体。

### 5.2 当前没有速度字段：使用受限的只读运动估计

首选现有业务若已提供的只读、非遥测材料位移数据；当前WaterSandSnapshot未提供，v1使用**相总库存重分布的渲染位移代理**，不新增物理速度场或读取P011日志反驱动画。

对H/L分别定义`M_H=bh+mh+suspH`、`M_L=bl+ml+suspL`。闭边界、无注入/重置且同一物理场的连续快照满足相总量守恒时，横向净通量的积分代理为：

```text
deltaM_p[c] = M_p,new[c]-M_p,old[c]
F_p[e] = -sum(c=0..e, deltaM_p[c])       // 右向为正，单位=库存
deltaX_p[c] = (F_p[c-1]+F_p[c]) / (2*max(Mbar_p[c], eps))
```

边界外F取0；deltaX单位为列。前后库存总量校验不通过、相库存太稀薄或切换了物理场时，该估计无效并冻结/重建渲染坐标。内部升降/渗滤不改变M，因此不会误当横向流动；纵向纹理位置用当前相层内归一化高度随边界伸缩，而不是凭空生成沉降粒子轨迹。

**能力限制：** 该代理恢复的是同相沉积+悬浮合计的净搬运，不能唯一分解底层、表层、悬浮各自速度，也看不到净零的反向循环。只用于弱纹理跟随和D的候选方向，不声称是真实流速；仅悬沙变化而沉积profile未变时，冻结沙体横向纹理，优先更新雾。没有足够依据则冻结对应纹理，不能以tilt符号持续滚动沙纹。

renderer维护连续UV场，以同一相邻列坐标插值平流：`UVnew(x)=UVold(x-deltaX(x))`，在显示坐标中采样缓存tile。Canvas2D无自定义UV网格shader，v1用16组材质裁片近似局部平移/缩放，各组位移取连续UV插值值，边缘2～4px归一化羽化、权重和为1；相位不得逐列独立随机。组间纹理接缝可见则增加渲染裁片到最多32组并重新验证预算，不能降低128列物理分辨率。

当输入快照相同，deltaX=0、纹理相位不变。长期运行定期将UV减去整tile周期，不改变显示相位；不使用每帧随机seed。切换外观开关不能推进或重置物理状态。

### 5.3 验收与预算

- effector锚点：沙体内部行内G通道std **约3～4.4→≥15**（参考17～23）；水平灰度相邻差HF_h **0→≥0.8**（参考约1.1）；整体颗粒亮度std参考26～32。同时报告G通道与灰度测量，不能混换指标。
- 同场静置不闪烁；运动时颗粒与可见材料同向、没有固定屏幕贴纸感、反向拖影或跨列拼缝。纹理随材质运动的结论必须来自录像，不由一张噪声截图认定。
- 估算：`O(128)`库存差分/UV更新，16组×最多3材质，≤48次grain绘制；CPU目标≤0.40ms/帧，预着色纹理和UV缓存≤1.25MiB。禁止逐帧在全屏逐像素生成颗粒或调用Math.random。

## 6. C — 分层高度映射为层理（第四步）

现有bh/bl/mh/ml只描述当前层高和配比，没有真实沉积年代。将`[0,bh]`、`[bh,bh+bl]`、`[bh+bl,S]`分别定义为有序重层、有序轻层、混合层；用层内深度ζ映射微弱水平条纹：

```text
Ylayer = Ybase + aLayer*bandNoise(zeta/lambda + slowWarp(x))
aLayer = aOrdered * (1-mixFraction) + aMixed * mixFraction
```

候选`aOrdered=4～8`亮度级、混合区减半；lambda对应约12～18显示px，叠加两种非整比频率与宽幅明暗区，避免等间距“百叶窗”。条纹受当前层边界裁剪、随层高/材质坐标变化，不能把虚构的沉积时间写入物理账本。以纯亮度微调为主，保留材料色相。

沿bh、bh+bl与S的不同高差影响条纹密度、亮度及遮罩，不只从全屏y画一张静止横线贴图。薄于一个条纹周期的真实层只画其存在的部分；hybrid没有四层身份时仅可画heavy区域弱深度纹理，不能报告水沙层理映射通过。

- effector锚点：同一归一化沙体ROI的row-mean极值振荡**≥20/区域**，参考22～43；参考极差54～81为E+B+C合成诊断，不能靠C单独加粗亮暗线硬凑。若现有几何不支持参考的2～4明暗块，记录结构差异，不改变沙丘形状。
- 估算：缓存layer纹理，≤16次绘制；CPU目标≤0.15ms/帧，纹理≤0.25MiB。O(128+层数)，不逐行重建大Path。

## 7. D — 流痕与局部前缘毛边（最后）

依赖A/B已经可见且运动坐标稳定。用§5.2中可信材料位移代理构造屏幕方向`t`；强度取实际profile变化与净通量的共同证据，不能仅由容器倾角触发。有效方向上使用各向异性纹理：

```text
stretch = 1 + (sMax-1)*clamp(observedMotion/motionRef, 0, 1)
Nflow = grain(dot(uv,t)/stretch, dot(uv,normal(t)))
```

候选`sMax=2～4`、motionRef仅是视觉映射系数，待同序列录像定标。v1把方向量化到少量预生成atlas方向档并交叉淡化；拉伸B的同一纹理裁片，而不是再画大量粒子。方向无效、材料静止时stretch=1，不播放预设下落动画。

前缘毛边仅在真实S附近1～3显示px的光学带调整alpha/亮度，限于容器和现有相mask附近；不移动S、不用大尺度轮廓噪声制造沙丘。散在水中的可见粒点必须有对应悬浮库存；无悬浮材料不能凭空产生飞溅。

- effector锚点：匹配流动期沙面行灰度阶跃由0向**≥6**改善；流痕方向与可见运动一致，静止后停止。steps阈值/最小间隔由同一离线测量口径冻结，不能把随机噪声交替统计成流束。
- 参考轮廓relief约141px、当前约7px只作差距记录，**不是本渲染路线的达标目标**；二者尺度/物理形态不同，不通过抖动边界伪造大起伏。
- 估算：复用B，额外方向选择和前缘≤16次绘制；CPU目标≤0.20ms/帧，方向atlas≤1.0MiB。缺可信流向时D处于数据不足，E/A/B/C仍可独立交付，但不能标D已验证。

## 8. ArkTS/ArkUI落地与性能约束

### 8.1 缓存方案

采用Canvas2D的Path裁剪、普通source-over透明合成、缓存ImageBitmap/tile和gradient。小纹理在初始化或外观配置变化时由离屏画布生成，`putImageData`仅用于预生成小tile，随后`drawImage/createPattern`复用；不每帧transfer、全屏readback或创建PixelMap。`createPattern`可能返回null，必须有有限`drawImage`平铺fallback，不能默默丢掉颗粒层。

Offscreen/纹理的释放、重建由renderer持有；初始化重任务分批在首次展示前完成并先显示合法底色，不阻塞正在运行的物理帧。预生成纹理可以用包内确定性资源代替，外观相位仍在renderer中控制；不以Worker/新native插件作为v1前置。

纹理缓存像素分辨率与物理128列是两个独立量。低频云雾可用低分辨率tile放大；高频grain按目标截图px/DPI映射采样，避免大块马赛克。取消旧架构“压力时N=96”的降档建议在P-012的适用性：**物理cols始终128，geometry采样不降**。

### 8.2 预算估算与帧率放行

| 功能 | CPU增量设计预算/帧 | 新增绘制调用上限 | 常驻纹理/缓存预算 |
|---|---:|---:|---:|
| E | 0.25ms | 16 | 0.25MiB |
| A | 0.35ms | 48 | 0.75MiB |
| B | 0.40ms | 48 | 1.25MiB |
| C | 0.15ms | 16 | 0.25MiB |
| D | 0.20ms | 16 | 1.00MiB |
| 公共adapter/mask/状态 | 0.15ms | 16 | 0.50MiB |
| 合计 | **≤1.50ms** | **≤160**（加原基础几何） | **≤4MiB**，重建暂态峰值≤8MiB |

CPU表是工程预算，GPU填充/透明overdraw另须测量，不能把调用数直接当帧时间。第一梯队含公共部分目标≤1.15ms；每帧无大数组/图片对象新分配，视觉pass平均覆盖≤3层容器面积，优先只更新有变化的mask/UV，小纹理缓存复用。复杂度为`O(128+P)`、P≤32材质分组；纹理生成一次性O(tilePixels)。

当前素材FPS=52～60，不代表存在1.5ms空闲：60fps帧预算16.67ms、52fps约19.23ms。**新渲染必须在同设备/温控/刷新率/输入下配对验证无下降**：窗口FPS分布与P50/P95帧时相对baseline不退化，最低稳定区间不得低于对应基线；不能把“仍在52～60内”当作从60降至52的免责。预算达标但总帧时下降仍不放行。只靠历史截图上的FPS不能宣称性能已验证。

预算超限时先缓存预着色、裁掉屏外绘制、合并相同裁片和降低低频噪声更新工作；不得减少物理列数、降低物理步频、跳过物理step或隐藏降到30fps。C/D可保持未启用并作为后续阶段，但第一梯队已承诺的E/A/B验收必须同时满足；关闭颗粒/雾后跑快不算该档通过。若现有主线程预算仍容不下E/A/B，则报告PERF_BLOCKED供architect重做渲染组织。

## 9. 生命周期与证据一致性

renderer私有状态包括tile种子、UV、上一份只读相库存、可选边界副本及纹理缓存；不在snapshot或遥测中增加字段。

- run/reset/注入导致总量变化：清除前帧匹配及UV增量，按render seed重建材质映射；纯tile资源可复用。
- TEST/EXPERIENCE或watersand/hybrid切换：清空运动估计，使用当前完整输入初始化，不混用前一物理场库存。模式切换不触发新物理操作。
- 方向反转：按有效快照变化更新UV，不换seed、不整块跳相位；无变化时静止。
- 180°坐标事件：只跟随已有视觉变换，重建前帧配对，避免把镜像当巨量平流；不自行flip物理数组。
- 尺寸/DPI变化：重建像素映射与mask，保留材质归一化坐标或在合法新帧初始化，释放旧bitmap；App后台恢复/帧间断档不追赶一串视觉动画。
- 相为空：相纹理与浓度显示消失；缺帧/非有限读数使用最后合法显示或停止该增强层，不修补物理状态。

## 10. 验证：SV-05/06/07及基线关键帧

沿用 [TEST_PLAN_T1_T4.md §主观评分](../test/TEST_PLAN_T1_T4.md) 的SV-05界面观感、SV-06颗粒与液体质感平衡、SV-07对Video1整体相似度；评分依据完整录像及规定动作，不凭静帧宣布SV通过，不改原评分量表或总分门槛。P-012只改善对应外观维度，不能据这三项替代其它物理/体验用例的通过。

### 10.1 素材与分阶段对照

- 参考：`test/baselines/video1_frame0..7.jpg`和`video1_reference.mp4`；静置看f0/f4，动态流痕重点对照f2/f6，同时保留8帧全览。
- 当前基线：`test/artifacts/p012-cur-ws.jpeg`、`p012-cur-ws15.jpeg`、`p012-cur-hybrid.jpeg`及`p012-hybrid-tilt1/2/3.jpeg`。基线物理为e2f02a1，不能与不同物理版新图直接归因为外观改善；实现复测必须另采同物理build外观OFF/ON配对。
- 对照顺序：OFF → E → E+A → E+A+B → +C → +D，每档相同初态、种子、输入、模式和设备；第一梯队必须联合验收。TEST固定输入用于可重复关键帧，EXPERIENCE检查真实传感器下的无假动/闪烁和流畅性。
- 每档保留0°静置、±15°、匹配基线的慢倾/翻转/回正录像与时间对应关键帧；不为展示流痕改物理输入或强行产生新的坡面。

### 10.2 量化锚点口径

E/A/B/C/D各节数字来自effector差距分析，是P-012效果验收依据，不写入物理checker。由effector在同一离线ROI口径下复核；这些数值不能替代SV的自然度判断。

统一排除UI、容器边框/纯背景；标出水体内部、界面法向带、沙体内部、流动前缘四类ROI。先按容器/沙区尺寸对齐比较尺度，保留原图与变换参数；不使用锐化、后期增亮或不同缩放给某一档加分。max亮度只在材质ROI统计，排除边框；同时看p99，避免单亮点刷max。

effector报告的“G通道行std”和“灰度HF_h”分别保留。若原测量脚本/ROI未附，交付时需冻结测量附表并对参考及OFF同时重算；不能把当前3.0/46等历史近似值冒充新算法同口径结果。建议灰度采用8bit `Y=0.299R+0.587G+0.114B`，HF_h为同一沙体行内相邻Y差绝对值均值；若原effector脚本定义不同则沿用原定义并注明，不悄悄换指标。

界面maxGrad用固定法向剖面的横向聚合亮度逐行差，10%～90%宽度用该剖面的局部沙/水平台归一化；单颗粒的逐像素梯度另报，不与聚合剖面混用。row-mean振荡、流动steps的平滑窗口/最小prominence/间隔必须冻结，并对噪声层消融，防止把B的高频随机点算成C层理/D流痕。

| 交付档 | 主要量化锚点 | 主观重点与防退化 |
|---|---|---|
| E | 水p50≈200；沙max≈240及p99改善 | SV-07色调接近；相身份可辨、不过曝 |
| E+A | 过渡数十px、maxGrad<8；匹配水体ROI std>15 | SV-05柔和界面；SV-06斑驳体积感，非均匀插值 |
| E+A+B | 沙体行std≥15、HF_h≥0.8 | SV-06粉质、SV-07实物感；静态无闪烁、动态无贴纸感 |
| +C | row-mean振荡≥20/匹配区域 | SV-07层理自然，不像规则横线，不改变层厚 |
| +D | 匹配流动行steps向≥6改善，方向随可见材料 | SV-05/06前缘和流束；静止停、无大轮廓假动 |

所有档均附同机配对帧时/FPS和开关外观前后物理状态一致性证据；仅复用既有物理回归/只读快照比较，不给生产checker加入P-012逻辑。结果按appearanceRouteId、物理route/build、renderer配置、素材hash与模式隔离归档，独立记录“效果通过/性能通过/数据不足”，不跨模式沿用通过结论。

## 11. 架构交接与候选取舍

选用缓存tile+Canvas2D多层合成，因为现有ArkUI接口可承接且性能开销可限定。逐粒子CPU绘制/每帧全屏噪声生成不选：成本随屏幕像素或颗粒数增长；全屏实时blur不选：会吞掉B的高频和C层理；shader流体/真实体积光不作为v1前置：超出本次表现层与已核实接口；单一均匀渐变不选：无法补上斑驳与HF缺口。

architect后续需明确：只读adapter与渲染入口、TypedArray/bitmap生命周期、组间羽化及D方向atlas实现、严格route隔离、同物理build视觉采集和性能验证方式。顺序必须E→A→B→C→D；先交第一梯队评审，再让后两项依次接入。实际速度来源不足时如实采用受限代理或将D标记数据不足，不通过新物理字段、遥测或倾角假动填缺口。

设计完成不代表帧率/观感已达标。本稿已核对本地字段、渲染入口、SDK声明、SV定义及目标/当前预览；性能数字和材质系数均为待真机验证的设计预算，正式通过需配对录像、离线锚点和SV评审。
