# P-013 P1 工程设计 rev2：纹理耦合、完整沙区合成与色彩管理

日期：2026-10-24。责任：architect。任务：6adc861c…d612，P4（渲染线）。

状态：**ENGINEERING_DESIGN_REV2 / PENDING_MANAGER_ACCEPTANCE / DESIGNER_RECIPE_SIGNED / NOT_IMPLEMENTED / NOT_CALIBRATED / NOT_VALIDATED**。

P0主体已由manager验收，本稿rev2是下一步设计，不代发P1实施许可。只交文档，不写代码、不占真机、不改冻结锚点/判线。designer已在[材质配方与坡度核确认 v1](design-P013-p1-material-recipe-slope-confirmation-v1.md) §1–§8 签收材质与坡度算法语义，manager于2026-10-24 21:55验收通过；rev2将该定稿变成开发契约并补齐跨栈差异与门序。本文的确切参数是**设计契约**，不是已完成真机定标或效果签字；G1是否解冻仍由manager验收本rev2后决定。

## 1. 输入、P0基线与P1出口

依据：[立项书v1.1](proposal-P013-incorporation-v1.md)（P1完整材质合成，P2水雾，P3旧栈退役）、[designer效果目标](design-P013-E2-effect-target-acceptance-v1.md)（Rmat先合成，Lmat逐像素作用，ToneMap最后）、[E2算法](algorithm-P012-E2-top-light-opening-v1.md)、[P0工程契约](architecture-P013-p0-skeleton-predesign-v1.md)、[工具链路标](p013-p0a-toolchain-notes-v1.md)、[seg3最新报告](fps-seg3-P012-v0.md)和priority.md的P0主体验收。

已读实现：native `napi_bridge/egl_context/shader_pass/tdata_uploader/render_contract.h`，ArkTS `P013FrameAdapter/RenderController/RenderTypes`，`P012WaterSandRenderer`的材质位图、motionCache、分层顺序与appearance staging，以及SandCanvas的ticker/Stack。P0现状：T_data=128×1 RGBA32F、layout1、E2_APPROX_P0、zOrigin1；只有诊断灰阶/透明OFF；EGL未指定sRGB surface；shader关闭blend；controller只在旧Canvas render之后提交。**这些是需要扩展的基线，不是P1已具备能力。**

P1出口需同时具备：B/C与底材在同一线性材质空间相加→同一个Lmat→同一次ToneMap；完整沙区由GLES负责；水雾/界面/粒子显示仍按原责任排序；色彩和版本身份可复现；CPU参考/黄金图/生命周期/性能证据齐备。M1须由designer对正式报告判定；P0热图PASS不能继承为M1。M2的90Hz呈现独立验证，换栈不解决物理负载、不突破90Hz面板、不宣称G2.7≥100达成。

## 2. 方案选择与绘制责任

不采用跨Surface乘法、每帧读回完整Canvas再上传、将旧已受光画面当反照率纹理三种做法。主线采用 **CPU保留现有确定性材质资产生成 + GLES完整沙区单pass + Canvas水雾/前景双层共存**。

```text
ArkUI Stack：
  Canvas background：水体底色 + 水体雾（旧步骤1/2）
  XComponent：完整沙体底材 + C/C3 + B + E2（一个沙区fragment pass）
  Canvas foreground：tracer/活动粒子显示 + 液面高光 + interface雾 + 容器边框
  原ArkUI按钮/参数面板/手势
```

| 原renderer责任 | P1 composite模式 | 基线/P0模式 |
|---|---|---|
| prepare/profiles/motion/appearance | 同snapshot每逻辑帧只执行一次，产只读FrameToken | 原完整render路径保留 |
| 沙体三带、B、C、C3、列级spark | GLES独占；Canvas不再画这些层 | Canvas完整旧链 |
| water底色、水体雾 | background Canvas保留原公式与缓存 | Canvas完整旧链 |
| tracer/未来路线C真实粒子显示 | foreground消费既有唯一显示路由；不改变物理门 | 原显示互斥纪律 |
| 高光、interface雾、边框 | foreground按旧步骤3.75→4→5→6顺序 | 原顺序 |

现interface雾会覆盖沙侧，故不能留在GLES下方；tracer/粒子不能被完整沙层意外遮掉。P1不迁移雾噪声生成或改变alpha cap。前景Canvas增加一层是明确成本，遥测/内存必须包含；不能把“双Canvas+Surface”写成天然性能收益。

同tick提交只保证**逻辑输入一致**，不能保证三Surface由系统同次呈现；Canvas没有与EGL共享的已证实呈现fence。静态黄金图须等稳定并核对frame key；动态呈现需trace/截图身份核实。若设备z序/透明合成不支持或出现不可接受错帧，标COMPOSITION_ORDER_BLOCKED/FRAME_ALIGNMENT_UNPROVEN并保留Canvas，交manager/architect改方案，不把静态PASS当动态共存已过。物理/输入/外观存储schema不因本方案改动。

## 3. 文件与模块落点

以下均为待授权实施，工程目录为 `entry/src/main/`：

| 文件/模块 | 责任 |
|---|---|
| `ets/render/P013RenderTypes.ets` | 追加P1显式帧/资产/版本/receipt类型；保留现P0 API与枚举值0–3 |
| `ets/render/P013FrameAdapter.ets` | P1三行T_data、混合成分、CPU坡度与warp导出；P0 pack不改布局 |
| `ets/render/P013MaterialAssets.ets`（新增） | 从同一材质生成器导出六张无光照RGBA8像素与manifest；固定缓冲/失效矩阵 |
| `ets/render/P013MaterialReference.ets`（新增） | EOTF/Q/ToneMap/材质组合/采样与Lmat的CPU参考；不读取显示截图倒推参数 |
| `ets/render/P012WaterSandRenderer.ets` | 提供prepareOnce/drawBackground/drawForeground/drawLegacyFull与只读材质导出；旧render仍为原语义包装，不重复推进motion |
| `ets/render/P013RenderController.ets` | P1模式、资源准备/原子激活、Canvas层责任、异常恢复、帧/资源遥测 |
| `ets/pages/SandCanvas.ets` | foreground Canvas与同ticker接线，显式模式切换，canonicalEvidence强制旧臂；不新增物理门/调试捷径 |
| `cpp/p013renderer/material_uploader.cpp/.h`（新增） | 两个纹理数组、bounded上传队列、双代资源、恢复与计数 |
| `cpp/p013renderer/shader_pass.cpp/.h` | 独立P1 program/必需uniform验证；P0诊断program保留 |
| `cpp/p013renderer/glsl/sand_material.vert/.frag`（新增） | 完整沙区材质、统一Lmat、ToneMap；无Canvas读回 |
| `cpp/p013renderer/render_contract.h,napi_bridge.cpp,egl_context.cpp,tdata_uploader.cpp` | P1版本分派/桥/色彩profile/三行上传；P0仍可独立运行 |
| `cpp/CMakeLists.txt` | 新shader嵌入、实际源码hash、文件清单；沿用已打通的库实名与工具链 |

共享生成器若抽为纯函数模块，应让Canvas与GLES导出同用，不手抄两套种子/噪声/λ计算；不为了本设计顺手重写P012。无ImageBitmap原生handle可直接传给NAPI的现成契约，因此P1传明确字节；不使用每帧getImageData/截图读取现有bitmap。

## 4. 纹理/参数/版本契约

### 4.1 T_dataLayoutVersion=2（128×3 RGBA32F）

row-major，`offset=(row*128+c)*4+channel`，1536 float32、6144B。行列取样仍用P0的texelFetch+mix、x/colW及右缘127钳位，不依赖float线性过滤扩展。

| 行 | R | G | B | A |
|---|---|---|---|---|
| 0 | sandTop | waterTop | bh | mixStart=bh+bl |
| 1 | pb（Canvas显示px/px） | fH_mix | warpH（cycles） | warpL（cycles） |
| 2 | warpM（cycles） | sparkGain | 保留0 | 保留0 |

row0复用DISPLAY_4X3_V1，材料高度与层序保持P0校验。fH_mix为同snapshot的mh/(mh+ml)，分母为0取0.5（空混合带时不形成可见材质），不得写物理数组；fH∈[0,1]，线性插值后再用于混合色。warp按现 `warpAt` 与acceptedUV从CPU同源计算三带表，P1首版选择逐列插值，**不复用Canvas分段矩形的近似边界**；`strataSamplingVersion=CONTINUOUS_COLUMN_WARP_V1`，这是显式新栈采样差异，需重出层理基线，不宣称逐位旧图等价。sparkGain为现seed/列索引规则产出的无光照材质亮粒增益；消融spark时为1。

每帧另传 `grainOffsetCanvasPx[3]`（现meanDeltaXPx的H/L及均值M，按现规则round）；`bandMinYCanvasPx[3]/bandMaxYCanvasPx[3]`（由同profile全列极值）；`strataOriginYCanvasPx[k]=floor(bandMinY[k])`；`strataLambdaEffectivePx`、`canvasToSurfaceScaleX/Y`、Canvas坐标box与原生surface坐标box。数组长度全部固定，所有值有限；reserved通道非0拒绝，不能静默解释为将来其它数据。

### 4.2 P1坡度核与深度

已签收 `slopeKernelVersion=E2_GAUSS_COL6_R18_P1_V2`：输入既有 `DISPLAY_4X3_V1` 完成后的128列 `S_D`，只沿列索引做 σx=6列、截断半径 Rx=18列、37 taps 的Gaussian；`g[j]=exp(−j²/72)`，对完整37项一次归一化 `w[j]=g[j]/Σ(i=-18..18)g[i]`。列端点复制 `S_D[clamp(c+j,0,127)]`，不环绕、不镜像、不删tap后二次归一化。**深度方向 σz=0、Rz=0，恒等处理**；只在沙mask内求值，不在沙外造边界值。

实现顺序与精度：固定 `j=-18…18` 顺序，以Float64生成核、卷积 `S_G[c]=Σw[j]S_D[clamp(c+j,0,127)]` 及列点差分；`pb64[c]=(S_G[min(127,c+1)]−S_G[max(0,c−1)])×(boxHCanvas/capacity)/(2×boxWCanvas/128)`，端点仍除 `2dx`；最后上传前量化为Float32。shader只对上传pb做连续列插值，不在fragment重卷积，不插值已归一化normal代替pb后归一化。

**跨P0不可宣称逐fragment等价**：P1先在128个列点差分再插值；P0 `E2_APPROX_P0` 在连续采样点取 `x±dx` 差分，尤其末列钳位区结果不必相同。P0不因P1签收而改核或升版。数学fixture及期望值见[reference-P013-p1-material-recipe-v1.json](reference-P013-p1-material-recipe-v1.json)；该文件是分析参考，不是产品/GPU测试通过证据。

`zOriginVersion=1`继续：`zMaterial=interpolate(S_D,x)−h(y)`，深度面使用 `S_D`（不是额外平滑的`S_G`、不是`S`的物理写回值，也不是`S_G`）；shader使用原生显示px，`ellLightPx=ellLightMaterial×boxHeightPx/capacity`。若计算pb用Canvas px，同等比例转换下方向相同，非等比缩放则拒绝P1效果采集（避免坡度单位漂移）。光向沿P0 `CONTAINER_PRE_INVERT_V1`，镜像q一次，不再CPU反转纹理或给XComponent再rotate。更换深度单位或屏幕固定光向须另版。

### 4.3 六张材质资产与绑定

固定layer顺序H=0/L=1/M=2。CPU导出两份紧密连续Uint8Array，均row0为图像顶行、RGBA、alpha255、无预乘、无E2/阴影/ToneMap烘焙：

| 资源 | 尺寸/格式 | GL绑定 | CPU与GPU字节数（各） |
|---|---|---|---:|
| T_grain | 256×256×3层，GL_TEXTURE_2D_ARRAY/GL_RGBA8 | unit1，sampler2DArray | 786432 |
| T_strata | 64×256×3层，GL_TEXTURE_2D_ARRAY/GL_RGBA8 | unit2，sampler2DArray | 196608 |
| T_data | 128×3，GL_TEXTURE_2D/GL_RGBA32F | unit0 | 6144 |

B像素由现grainTile×grainSigma加该位图固定基色并clamp255，C像素由现stratNoise/amp/effectiveλ/envelope生成128+delta灰阶；导出**putImageData之前的同一字节**，不得从已经受光的Canvas沙面读取。保留现alpha255/强度独立规则，避免旧s94的把alpha烘入纹理问题。GL_RGBA8在这里存的是sRGB编码颜色数据，**不**使用GL_SRGB8_ALPHA8自动解码，shader手动EOTF一次。

材质采样首版固定texelFetch（无硬件重采样/无mipmap）：B的整数texel由pre-invert Canvas绝对坐标 `floor(xCanvas-round(off_k)),floor(yCanvas)` 正mod256；C的x由 `round(warp_k/WARP_RANGE*64)` 正mod64，y由 `floor(yCanvas-floor(bandMinY_k))` 正mod256。WARP_RANGE取生成器现值，manifest提供，不能开发者另设；wrap公式 `(i%n+n)%n`。真实Canvas logical px→surface native px的映射写入帧身份，不能拿DPI重复放大粒径。P1 C列warp连续插值但取列texel仍可能出现纹理相位台阶，列锁定梯度/纹理prominence必须实测；若改成手动线性过滤需另采样版本，不悄悄模糊压台阶。

材质包metadata固定包含 `materialBundleId,assetGeneration,appearanceConfigRevision,renderSeed,geometryEpoch,paletteHash,grainHash,strataHash,grainWidth=256,grainHeight=256,strataWidth=64,strataHeight=256,layers=3,inputEncoding=SRGB8_STRAIGHT_OPAQUE,assetGeneratorVersion,materialRecipeVersion,colorPipelineVersion,effectiveLambdaMaterial,effectiveLambdaCanvasPx,grainBaseSrgb[3],warpRangeCycles`；hash基于实际字节与生成参数，不以文件名当hash。颜色三通道分别保存，M grainBase是现H/L平均基色，不是运行期fH基色；后续增量提取须扣正确的资产基色。

### 4.4 七个版本身份登记

P1帧、材质bundle、CPU fixture和报告必须携带并逐项精确匹配以下身份；未知或不匹配的版本拒绝绘制，不推断缺省值：

| 字段 | P1 rev2 固定值 | 作用 |
|---|---|---|
| `materialRecipeVersion` | `P1_ALBEDO_DELTA_V1` | palette、B/C增量和组合公式 |
| `slopeKernelVersion` | `E2_GAUSS_COL6_R18_P1_V2` | §4.2列域核、边界、差分及精度 |
| `colorPipelineVersion` | `LIN_SRGB_REINHARD_V1` | 唯一EOTF/Q/合成/ToneMap/OETF链 |
| `textureSamplingVersion` | `P1_TEXEL_CANVAS_PX_V1` | B/C texel取样坐标和正mod规则 |
| `strataSamplingVersion` | `CONTINUOUS_COLUMN_WARP_V1` | 连续列warp及离散纹理取样身份 |
| `T_dataLayoutVersion` | `2` | P1的128×3布局 |
| `zOriginVersion` | `1` | `S_D−h(y)`深度原点定义 |

P0仍固定 `T_dataLayoutVersion=1 / slopeKernelVersion=E2_APPROX_P0 / zOriginVersion=1`；不为P0同名换义，不把三行layout2 buffer交给旧layout1校验器。配方确认/数学fixture自身另记 `P013_P1_RECIPE_CONFIRM_V1 / P013_P1_RECIPE_ANALYTIC_V1`，不是替换上述七个运行时版本字段。

## 5. 色彩管理与材质合成冻结表

### 5.1 必须唯一的计算链

```text
无光照palette/纹理sRGB字节
  → 一次sRGB EOTF解码 → Q映射至参考材质辐射量
  → Rbase + δC（含C3） + δB，最后下界0
  → 乘同一个Lmat（OFF严格1）
  → 一次ToneMap T(x)=x/(1+x)
  → sRGB default framebuffer硬件OETF一次
  → 系统compositor（另核实际屏幕色彩/alpha，不代称已验证）
```

这里Rmat是风格化材质辐射代理，不声称测得物理反照率。现B位图已有基色、C灰阶中性值128，直接把两张RGB加到底色会重复加基色；下面明确扣基线，防“耦合”实现仍是两张贴纸。

| 项 | 本版唯一约定 | 禁止 |
|---|---|---|
| sRGB→linear | 分量c∈[0,1]：c≤0.04045取c/12.92，否则((c+0.055)/1.055)^2.4 | 用pow(c,2.2)冒称同版本、重复解码 |
| Q材质量化映射 | `Q(c)=L/(1-L)`，L=min(EOTF(c),1-1/4096)；上界4095 | 字节255映射无穷；每帧自动曝光 |
| ToneMap | `T(x)=x/(1+x)`，曝光1，逐RGB分量，输入max(Rmat,0)×Lmat | 先T(Rmat)再乘Lmat、再套旧shadeLut一次 |
| 输入B/C | GL_RGBA8编码字节，手动EOTF/Q；alpha固定1，aB/aC独立 | 自动sRGB纹理解码后再手动解码 |
| 输出 | EGL_KHR_gl_colorspace的SRGB surface，default attachment实际编码必须GL_SRGB；fragment输出linear T结果 | 默认EGL_LINEAR再任意gamma“补偿” |
| sRGB写控制 | 如有GL_EXT_sRGB_write_control，显式enable FRAMEBUFFER_SRGB_EXT；无扩展依GLES3 sRGB写规则，不调用不支持token | 对sRGB framebuffer输出已OETF编码颜色造成双编码 |
| Alpha/混合 | 沙像素alpha1、非沙(0,0,0,0)，关闭GL_BLEND/深度/MSAA；二值mask符合预乘特例 | 负RGB、非沙白底、半透明gamma域预乘含糊处理 |
| 颜色域 | SDR sRGB；Canvas水雾原设置/截图格式与系统色彩状态入manifest | 以P3/HDR截图混测，拿色彩漂移算效果提升 |

EGL创建须检查扩展字符串、传 `{EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE}`、query surface与GL_BACK attachment color encoding；失败或实际不是GL_SRGB返回COLORSPACE_UNSUPPORTED，降级Canvas，**没有**本版未登记的“自动shader gamma兜底”。规范依据：[EGL_KHR_gl_colorspace](https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_gl_colorspace.txt)、[EXT_sRGB_write_control](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt)。本地SDK头有token仅证明可编译，不证明真机扩展/系统标色能力。扩展只约束GL写入编码，不能推断最终屏幕compositor已按预期解释，仍须色块/截图取证。

P0诊断的DIAG_GRAY_V1原写出编码标量；不允许在P0现有surface上直接改sRGB模式后假称灰阶不变。P0保留其旧profile；切入P1重建surface/profile并递增surfaceEpoch/contextEpoch。若在P1 surface显示热图，需用独立 `DIAG_GRAY_SRGB_TARGET_V2`（fragment先EOTF(0.5Lmat)再交硬件OETF），显式新身份，不能污染P0黄金图。

### 5.2 已签收的Rbase/B/C/C3确定配方

下列算法语义由designer确认稿 v1 §4–§6签收，P1实现不得再由developer选择基色、取整或减数：

1. **Rbase：**材质`k∈{H,L,M}`。低端palette字节固定H=`(145,168,111)`、L=`(216,196,150)`；M按同一layout2连续`fH`作编码字节域插值 `P_M=(1−fH)P_L+fH P_H`。不得用Hi palette、旧shadeLut、旧已受光`bandColor`作为P1基色。spark按现列seed规则生成`gain[c]=1`（消融）或现公式（启用）；先将palette基色乘连续列插值`gain(x)`，再clamp到248，再按非负`round_nonnegative(v)=floor(v+0.5)`取整，除255得`sRGB`，最后逐分量`Rbase=Q(Cbase_srgb)`。顺序不得换成先Q后gain或先ToneMap。
2. **B grain基线：**材质纹理三层均使用Alpha255的sRGB字节。`grainBaseSrgb[H]=(145,168,111)/255`，`[L]=(216,196,150)/255`，`[M]=(181,182,131)/255`。M层原始生成器底色仍为H/L平均`(180.5,182,130.5)`，噪声加到该raw值后按现`clamp255`/Math.round生成；基线是该raw值的**零噪声量化字节**，不可先round raw再生成整图。故零噪声样本与减数严格相同，`δB=0`。`δB=aB·(Q(BsampleByte/255)−Q(grainBaseSrgb[k]))`；aB沿当前`P012_B_GRAIN_ALPHA`，实际值入manifest。
3. **C1：**输入灰阶字节`gray=clamp255(128+env·amp·stratNoiseAt(...))`，中性值是编码字节`128/255`，不是线性0.5或127.5；`δC1=aC·(Q(CsampleSrgb)−Q(vec3(128/255)))`。字节128时增量严格0。纹理采样沿§4.3和`CONTINUOUS_COLUMN_WARP_V1`；C位图**不做σ6横向或纵向模糊**。
4. **C3：**保持每个材质带两个纵向样式块。块索引`b=floor((yCanvas−minY)/subH)`，`subH=(maxY−minY)/2`；`shadeIdx=[48,16]`，对应`x=shadeIdx/64=[0.75,0.25]`。沿旧生成式`shade=(2.2*x/(1+2.2*x))^(1/2.2)`，按`rgb()`的非负`Math.round`量化`clamp255(255*shade)`，得到灰字节`[206,159]`。`δC3=aC3·(Q(C3Byte/255)−Q(128/255))`。C3是固定无光照样式，不由坡度法线、`Lmat`或σ6核重新生成；其中点分块边界是**样式阶跃**，须单列报告。
5. **合成顺序：**`Rpre=Rbase+(δC1+δC3)+δB`，然后仅一次`Rmat=max(vec3(0),Rpre)`；不逐项clamp负增量，不设额外Rmat上界、不做source-over，也不将C3/纹理与光照拆成第二光场。随后§5.1唯一链中乘同一个Lmat并ToneMap。B/C消融只把相应delta置零；seed、资产和物理均不变。

```text
CbaseByte = round_nonnegative(clamp(P_k * sparkGain(x), 0, 248))
Rbase = Q(CbaseByte / 255)
δB = aB  * (Q(BsampleByte / 255) - Q(grainBaseSrgb[k]))
δC1 = aC  * (Q(CsampleByte / 255) - Q(vec3(128 / 255)))
δC3 = aC3 * (Q(C3Byte / 255) - Q(vec3(128 / 255)))
Rmat = max(vec3(0), Rbase + (δC1 + δC3) + δB)
Lface = clamp(La + Ld * max(0, dot(normalize(-pb,0,1), lightDir)), Lmin, Lmax)
Lmat = lightEnabled ? 1 + exp(-zPx / ellLightPx) * (Lface - Lref) : 1
outLinear = T(Rmat * Lmat); alpha = 1
```

**跨栈含义：**本配方不声称旧display-space source-over等价于线性增量合成。P1 OFF严格是完整GLES材质pass、`Lmat=1`，不是Canvas旧Hi/shade底色的逐位复制；需独立重出OFF基线。Q对高亮输入可能放大饱和值差，Q上限/负裁/暗面幅度按§6局部风险报告，不能只看全帧SSIM或改gamma消差。

### 5.3 mask、颜色校准与读回

mask/坐标沿P0连续sampler与容器逆映射；H:0≤h<bh，L:bh≤h<mixStart，M:mixStart≤h<S，h≥S透明，不能将P-011飞沙/接触等待计入沙丘mask。P1不重建水/雾密度mask。二值mask不引入新抗锯齿混合语义，边缘质量与旧连续/逐列差异单列评估；若要coverage alpha必须另冻结display-space预乘/合成版本，不临时打开MSAA平滑过线。

色块取证覆盖黑、近黑、18%linear灰、0.5linear灰、近白、RGB主色及实际H/L palette；分别存CPU预期linear、期望sRGB8、GL读回、最终无损屏幕截图与Canvas对照。glReadPixels的返回编码/格式用实测probe登记，不从GL_SRGB枚举推断截图像素；关闭任何读回阶段的二次gamma。比对同一静态frame identity及颜色profile，原生PNG主测，JPEG只敏感性分列。容差由tester重复性测量/manager冻结，本文不自创ΔE、SSIM或误差阈值。

### 5.4 材质耦合五量的测量契约

固定方法身份 `methodId=P013_P1_MATERIAL_COUPLING_YCODE_V1`。显示主测`Ycode=0.2126·R8+0.7152·G8+0.0722·B8`，单位为8bit sRGB显示码；截图不得再次EOTF后还称同一个Ycode。CPU参考先沿§5.1完整色彩链输出sRGB，再用相同Ycode；Ylinear另作色彩链诊断，不混入主测分母。

所有对照固定`physicalSnapshotHash/FrameToken/seed/assetGeneration/palette/appearance/light/geometry/原生尺寸/色彩profile`。保存P1 OFF/ON × B开关 × C开关诊断格；消融不重烘资产、不改变seed。材质与深度匹配；纯沙ROI排除雾、高光、粒子、边框、真实材质边界和滤波外域。所有臂使用完全相同支持集，不可看过ON后再选ROI。迎/背光按几何`n·ℓ`与固定光向指定。记录每ROI材质、z分布、像素数、边界图、纹理/光开关及maskRevision。静态场景至少3次独立渲染/读回；动态逐预注册frameId/snapshotHash采样并报告时间分布，不在ON曲线上择峰。原生PNG主测，JPEG单列；CPU中间输出、GL读回、最终三层截图分开保存。

纹理影响定义`I_X^t=Ycode(F,t,X开,其它纹理固定)−Ycode(F,t,X关,其它纹理固定)`，其中`t∈{OFF,ON}, X∈{B,C}`。B带宽分量为`H_B=I−G_B*I`，`G_B`是归一离散Gaussian，`σB=active grainFeaturePx×Canvas/native等比尺度`，`R=ceil(3σB)`原生px；两方向同核。不得重采样原生图；ROI向内缩完整滤波支持后才算RMS，不跨mask补零。σB是测量频带，与渲染用的σ6列核不同。C不用B高通，取固定同材质(x,z)支持的行均值剖面，减预注册最小二乘常数+一次深度趋势后算RMS；仍保留C1 prominence/周期/row-mean既有指标。

| 量 | 可实施定义和输出 | 状态解释 |
|---|---|---|
| `R_B(Ω)`（含R_B,dark） | `A_B^t=sqrt(meanΩ(H_B(I_B^t)^2))`码值RMS；`b^t=medianΩ(Ycode(F,t,B关,C固定))`；`N_B^t=A_B^t/b^t`；`R_B=N_B^ON/N_B^OFF`。亮/暗ROI分开 | 与同配方CPU预测配对；`[0.8,1.2]`只是旧扫描域，不能作为冻结PASS线 |
| `dark`绝对幅度 | 固定Ωdark报`darkBOn/Off=A_B^ON/OFF`及CPU值；C以去趋势层纹RMS报`darkCOn/Off`显示码RMS，附C1指标 | 检查“比值正常但暗面纹理消失”；不定义最小幅度PASS线 |
| `Q_couple,X`（X=B/C） | 匹配Ωlit/Ωdark，`Q_X^t=A_X^t(Ωlit)/A_X^t(Ωdark)`，并报`D_X=Q_X^ON/Q_X^OFF`、各A及CPU预测。B用高通RMS，C用去趋势层纹RMS | 对照CPU方向与误差；不规定Q必须大于1，也不由单比值推断结构 |
| `C_band`连续性向量 | B/C OFF光照隔离臂，固定x同材质内扫z，报`Y_ON−Y_OFF`、CPU曲线、相邻原生px梯度/残差；x横扫比较列边界±1px和列中央支持的`g_edge=P95(abs(ΔxY))`、`g_mid`及比值；C3边界另标 | C3阶跃不得从完整C臂删除；阶跃阈值/列比尚未冻结 |
| `ΔY_face` | 匹配材质/z/mask的迎背光ROI：`ΔY_face^t=median(Y_t,Ωlit)−median(Y_t,Ωdark)`，另报`ΔΔY=ON−OFF`、CPU预测、材质构成和像素数 | 检查镜像光向与平沙伪差；8–20/255仍为旧扫描域，不升级为PASS线 |

ROI空、支持集不足、趋势拟合秩不足、分母为0或样本未匹配时分别登记`EMPTY_ROI / INSUFFICIENT_SUPPORT / UNDEFINED_RATIO`并附原始量；不加EPS、不默认比值为1。近零信号可测性由tester基于读回重复性校准，不由本稿另立SNR门槛。

**可先固定的结构条件：**OFF严格`Lmat=1`；ON/OFF共用资产/seed；同一Lmat作用全部材质；neutral增量为0；flat/mirror法线关系；无额外深度/层纹模糊；同相同坐标CPU公式与mask。**只有实测后才能提出数值线：**GPU/CPU/截图误差、颜色ΔE、R_B与dark幅度、Q_couple误差、C_band阶跃阈值/列比、ΔY_face可读幅度和跨帧波动。tester先在G0/G3/G5建立重复性与分布，designer据此审效果，manager再冻结；未冻结一律标`MEASURED_NOT_CALIBRATED`，不记VALIDATED。

## 6. napi/API、资产生命周期与原子配置

P0 `drawFrame(frame,tdata,light)`不改签名；P1新增入口避免旧512-float校验歧义：

```ts
stageMaterialBundle(desc: P013MaterialBundleDesc,
  grainRgba: Uint8Array, strataRgba: Uint8Array): P013MaterialStageReceipt;
pumpMaterialUploads(byteBudget: number): P013MaterialStageReceipt;
drawMaterialFrame(frame: P013MaterialFrameInput, tdataV2: Float32Array,
  light: P013LightParams, material: P013MaterialParams): P013MaterialFrameResult;
discardMaterialStage(assetGeneration: number): P013ControlResult;
```

新增模式值4=`GLES_SAND_COMPOSITE_OFF`、5=`GLES_SAND_COMPOSITE_ON`；OFF是**完整同材质pass但Lmat=1**，与P0“透明OFF”不同。模式值0–3维持旧意义；新增类型含 `apiVersion=2` 与§4所有版本，native遇未知版本拒绝，不自动推断。

`P013MaterialBundleDesc`字段精确按§4.3 metadata，另含 `sessionId,surfaceEpoch,contextEpoch`。`P013MaterialStageReceipt={code,assetGeneration,state,receivedBytes,uploadedBytes,totalBytes,activeGeneration,appearanceConfigRevision,contextEpoch,errorStage}`，state=`CPU_STAGED|UPLOADING|READY|ACTIVE|FAILED|DISCARDED`。`P013MaterialFrameInput`继承P0帧身份/几何，增加 `apiVersion,assetGeneration,materialRecipeVersion,colorPipelineVersion,textureSamplingVersion,strataSamplingVersion,lightEnabled`；tdataV2精确1536float32。`P013MaterialParams`为§4.1 fixed-length三带数组、palette基色/资产grainBase、强度aB/aC/aC3、warpRange、有效λ、tier、grain/strat/spark消融、Canvas/原生坐标映射；不给JS任意GL纹理handle。

typedarray只接受匹配元素类型/长度/有效buffer，正确处理byteOffset；桥同步复制到native定容staging，返回后JS可复用。stage先验整包metadata/字节hash/alpha255，失败不改变ACTIVE资源。有效总字节固定983040，不接任意大纹理。frame必须精确引用ACTIVE或已READY的generation、同appearance revision、同contextEpoch；不允许新uniform+旧纹理混帧。draw成功与swap成功沿P0结果语义，不命名“已呈现”。

`P013MaterialFrameResult`扩展P0 FrameResult，必返 `apiVersion=2,assetGeneration,appearanceConfigRevision,materialRecipeVersion,colorPipelineVersion,T_dataLayoutVersion,slopeKernelVersion,zOriginVersion`（错误帧保留requestedGeneration与实际activeGeneration分列）。generation不匹配不得报告requested代已swap。P0/P1共享同session的单调frameId消费者；切profile重建surface须取新epoch、重发全包，不因切函数把frameId归零。P1入口不读取P0 diagnosticEnabled来决定是否画沙，显示开关只来自P1 composite模式与lightEnabled，避免OFF误画透明。

native内部 `MaterialUploader::Stage(desc,grain,strata)`、`Pump(maxBytes)`、`CanActivate(generation,contextEpoch)`、`Activate(generation)`、`BindActive()`、`Reset(contextCurrent)`；stage至多一个pending代，active一代。新申请覆盖pending先显式DISCARDED，active仍可画；live texture对象最多4个数组（两包×两对象），不用每帧glGen/compile。GL调用仍只在P0 NAPI owner，lifecycle mutex/解绑/恢复上限沿P0，不引入worker抢current。

pump每tick最多65536B（byteBudget须正整数且≤65536），按完整像素行chunk上传：B stride1024、C stride256，纹理unit1/2分派层；PBO解绑，UNPACK_ALIGNMENT4、rowLength/skip清零，glTexSubImage3D，不mipmap。983040B理想15个chunk，实际跨layer切段可更多；只承诺有界字节，不承诺<0.1ms。stage整包CPU复制约0.94MiB单列耗时，安排在外观应用/初始化而非每tick反复调用。上下文丢失时重建两数组并用保留CPU活包重传，READY前退Canvas，不“纹理丢了下一帧随便采”。

appearance仍走现草稿→合法域校验→applyAppearance→单一configRevision/持久化，不加第二套参数面板schema。P1协调器在旧/新材质CPU资产和native目标代均READY后，在**同一个FrameToken**激活palette、texture generation、uniform、foreground fog配置；期间使用上一代完整配置，UI显示pending而不假称已应用。若激活/上传失败撤销pending，旧active与旧配置保持；不能用现stage中已覆写的bitmap引用假称旧代仍在。这个双代持有是P1需要新增的工程行为，不能直接套旧consumeStagedAppearance就认为已原子。

| 输入变化 | T_data/参数 | B资产 | C资产 | 其它责任 |
|---|---|---|---|---|
| 物理step/profile/成分 | 每帧row0/fH/pb | 不重烘 | 不重烘 | motion/warp跟同FrameToken |
| 接受位移/phase | 更新offset/warp | 不重烘 | 不重烘 | 不因雾相位变化重建材质包 |
| grainFeaturePx/grainSigma | revision变 | 同seed重烘B | 复用C字节 | 两包原子激活 |
| strataλ/strength | effectiveλ与revision变 | 复用B | 重烘C | 延续整数周期对齐，报告有效λ |
| geometry/DPI | 更新几何/pb/坐标 | 原Canvas逻辑尺度明确后复用 | effectiveλ变化时重烘 | surface以eglQuerySurface为权威 |
| fogDensity/clearMottle | 原Canvas雾更新 | 不重烘 | 不重烘 | 同配置revision一起激活 |
| light/ON-OFF | uniform变 | 不重烘 | 不重烘 | seed/纹理/快照固定 |
| reset/invert/continuity | 明确epoch/重新配对 | seed未变可复用 | 参数未变可复用 | 不把视觉assetGeneration冒充物理epoch |

## 7. 帧时序、切换与失败恢复

renderer接口：`prepareFrame(snapshot,context):P013PreparedFrame`、`drawBackground(token,ctx)`、`drawForeground(token,ctxFg)`、`drawLegacyFull(token,ctx)`、`exportMaterialAssets(token,out)`；token包含snapshotId/frameSerial、配置、profiles、motion、种子、所有纹理参数和contextEpoch，仅本tick有效。prepare拒绝重复推进；draw只读token不再次update motion/consume config，不同ctx不能各建一份sampler历史。

```text
同一postFrameCallback
  既有物理step/copyStateTo
  prepareFrame一次，确认canonicalEvidence/route身份
  资源pump（有pending才做；限额）
  P1 ready? 否→drawLegacyFull+清foreground+移除/隐藏GLES
    是→clear并drawBackground(token)
       →drawMaterialFrame(token的T_data/参数/active资产)
       →成功则clear并drawForeground(token)，保留GLES可见
       →失败则立即drawLegacyFull(token)全量清重画、clear foreground、隐藏/移除GLES
  导出同frameId遥测；ticker仍唯一
```

初次切入：先保持Canvas完整画面，隐藏GLES完成sRGB profile/资源预热；收到首次有效swap后由controller在下一帧边界切责任，首过渡帧标TRANSITION、不作为黄金图。切回：同tick完整重画Canvas、清foreground、移除GLES；不等待失败swap再“透明清屏应该成功”。后台/销毁停驱动，前台完整重建/重传才恢复。canonicalEvidence强制原Canvas臂，P1 OFF/ON不是物理route，不能改变P-011事务或粒子库存。

新错误码在P0既有值后追加：400=`MATERIAL_PACKET_INVALID`、401=`ASSET_NOT_READY`、402=`ASSET_REVISION_MISMATCH`、403=`COLORSPACE_UNSUPPORTED`、404=`COMPOSITION_ORDER_BLOCKED`；shader/GL/EGL错误继续原300/301/302/200系列并附stage。失败发生在材质staging时保留旧active；发生在draw/present/lifecycle时完整Canvas回退并保存故障证据。不为“降级”关闭B/C或改默认外观后宣称P1仍成功。

## 8. 帧时/资源风险与遥测

沿P0 P013_FRAME/固定环/同帧关联，新增 `schemaVersion=2` 行，不改P-012 seg3原字段。CPU阶段和GPU/系统呈现分开；GPU timer不支持时null，swapSucceeded不是已呈现；不同钟只比较持续时间。背景/前景Canvas提交、材质包stage/pump均计总开销，不只报shader draw。

| 模块 | 新增字段与口径 |
|---|---|
| prepare/adapter | `prepareMs,profileMs,slopeBuildMs,warpPackMs,tdataBytes=6144,pairValid,profileHash,slopeHash,frameTokenId`，每帧；冷建另分组 |
| CPU资产 | `assetBakeMs,assetCopyMs,assetGeneration,pendingGeneration,activeGeneration,grainHash,strataHash,assetInvalidationReason,assetCpuBytes`，每次实际重烘/stage事件 |
| uploader | `materialUploadCpuMs,materialUploadBytes,materialPendingBytes,materialPumpCount,materialTextureLiveCount,materialTexturePayloadBytes,materialUploadErrorStage`，每chunk/每帧合计 |
| shader | `materialUniformMs,materialDrawCpuMs,materialGpuMs,gpuTimerValid,gpuDisjoint,shaderHash,materialRecipeVersion,colorPipelineVersion`；GPU异步query回填原frameId |
| 颜色/材质诊断 | `eglColorSpaceRequested,eglColorSpaceActual,attachmentColorEncoding,srgbWriteControlAvailable,srgbWriteEnabled,inputEncoding,outputEncoding,qCapPixelFraction,negativeAlbedoPixelFraction`；像素比例只诊断readback run计算，不在热路径全帧CPU遍历 |
| Canvas共存 | `canvasBackgroundMs,canvasForegroundMs,canvasLegacyMs,foregroundDrawCount,canvasSandDrawCount,logicalFrameId,compositionMode,presentCorrelationValid,transitionFrame`；P1 canvasSandDrawCount必须0 |
| 全链/恢复 | `totalAddedCpuMs,swapMs,frameIntervalMs,presentedIntervalMs,presentedSource,contextEpoch,materialReuploadCount,fallbackReason,telemetryDropped`；呈现源缺失必须null |

内存下界：单代六图payload983040B，活/待GPU两代1966080B；CPU活/待两代同量，另加P0/新版T_data、定容staging/遥测与profile缓冲。每个全幅RGBA8 Canvas backing至少4WH字节，新foreground增加至少4WH，Surface swapchain/驱动多缓冲另计，不以纹理payload当进程总内存。统计native owned、RSS、GPU texture/buffer计数、surface世代以及release归零；峰值/恢复/失败路径不漏。没有实测证明30min无增长或90Hz。

风险优先序：三Surface合成与色彩能力→Q/底材风格变化与C采样差异→shader纹理带宽/分辨率→上传stall与前景Canvas新增成本→跨栈锚点可比性。先用最小probe解除前两项再投入全套迁移；若失败退回manager，不靠改阈值、全局模糊、缩分辨率或物理降频过线。P1不继承P0单次<0.1ms为近1MB包的约束；6144B逐帧上传和静态包pump分别量测，正式P1帧时预算由tester基线/manager确认，原30fps与已解决硬门槛不得回退。

EOTF/Q含pow与除法，不能因“单pass”忽略每fragment计算成本。常量palette、grainBase、128中性值、C3两块参考的Q在CPU算好作为uniform，采样字节的Q仍按本版公式求值；若采用256项精确字节Q查表优化，表值必须由同参考公式Float32量化，并登记实现hash/证明所有256输入等价，不允许近似gamma或改变配方。GPU耗时实测后才判断是否需要该优化，不能预称单pass<1ms。

## 9. 验证矩阵与门序

所有以下为待实现测试清单，不是本稿已跑的结果；tester先落实可执行用例与RED基线，再由manager放行实现。数值容差不由architect新冻。材质耦合结果必须按§5.4记录实际支持集及五量原始输入/输出；不完整分母不填占位分数。

| 组 | 输入/对照 | 必须证明 |
|---|---|---|
| 色彩probe | 标准色块/实际palette，CPU/GL/屏幕/Canvas三端 | 编码一次、无双gamma、透明非沙、真实色彩profile与截图身份 |
| 资产 | 六图同seed/同参数导出，byteOffset/长短/alpha/坏hash/混代 | CPU字节同源，错误整包拒绝，旧代不污染，context后重传 |
| CPU数值 | 平沙/双坡/边缘/空混合/深度/镜像/180°/非全屏box | 同核/同mask/同坐标，Lmat关闭严格1，T(Q(c))校验与饱和值策略可复现 |
| 材质耦合 | 同physicalSnapshotHash/FrameToken/seed/assetGeneration/palette/appearance/light/geometry/原生尺寸/profile；OFF/ON×B×C诊断格；同maskRevision/支持集 | methodId=`P013_P1_MATERIAL_COUPLING_YCODE_V1`、Ycode同§5.4；按五量定义产出R_B(亮/暗)、dark绝对幅度、Q_couple(B/C)、C_band、ΔY_face及CPU对照；登记空ROI/零分母/支持不足状态；数值容差未冻结时标`MEASURED_NOT_CALIBRATED` |
| Canvas共存 | 水雾ROI、界面沙侧、tracer/粒子、边框、模式切换 | z序/alpha实际可用、Canvas无重复沙层、prepare只一次、失败完整回退 |
| 外观原子性 | 六参数单项/组合/恢复推荐、连续快速改参、上传失败 | active包/配置同revision，不半新半旧，不新增面板schema |
| 资源/性能 | settle/tilt/heavy，Canvas基线、P0诊断、P1同材质OFF/ON，同温同尺度 | CPU/GPU/呈现与上传/恢复分别P50/P95/max，30min资源曲线、上下文恢复，不能安静尾部掩盖重载 |
| 黄金图/跨栈 | designer四场景静置/倾斜/倒立/重载，至少三次独立静态采集 | 包/源码/shader/纹理/profile/光向/版本/色彩/ROI/mask身份齐全；s133门槛不改、重新采分母分子，不直接继承Canvas PASS |

E2效果候选的8–20/255、R_B,dark的[0.8,1.2]等仍是designer扫描域，不自动升级为冻结PASS线。E3/baseline已解决门槛继续维持；未定标/不可比/EMPTY ROI如实标记。P1原材质OFF与P1 ON用同一色彩链/资产/核，Canvas OFF是跨栈对照臂，不能因两臂色彩不同把亮度提升当光照收益。完整WS/SLIP物理口径与P-011终点协议均不由本设计放行。

实施切片建议（manager分卡并经授权后执行）：

1. **G0前置probe**：sRGB surface/profile与三层z序/透明/输入透传，明确实际呈现证据能力；未过不改产品默认。
2. **G1资产/CPU参考**：进入实施的解冻前置为**本rev2经manager验收 + tester登记CPU fixture/材质耦合用例及RED基线**；designer配方和坡度核已签收，不再列为待签收。G0 surface/sRGB/三层z序/透明/输入透传probe仍必须先通过（需真机、effector队列独占），不得用rev2文档完成代替G0实测。G1交付六图共享字节导出、shader色彩/材质CPU参考、Gaussian/pb及七版本身份；tester用例与RED可并行准备，G0未过不启动依赖真实surface的实施。
3. **G2上传/桥**：新API、两代包、bounded pump、版本校验/恢复；先隐藏Surface验证，不动旧视觉。
4. **G3完整沙pass**：P1 OFF/ON、材质消融与色块；shader/CPU参考一致性与黄金图链。
5. **G4共存接线**：prepare一次、background/sand/foreground责任、fallback与配置原子激活；Canvas旧臂回归。
6. **G5真机双报告**：按effector串行队列采集色彩、效果、帧时、长稳、生命周期；designer验证M1，manager裁定P2/后续排期。

G0/G5需要真机时由manager建卡排队，不能architect自行安装HAP占设备。P0主体已验收并不豁免P1的测试先行、平台probe或效果签字。rev2配方签收仅解除算法定义缺口；本rev2 manager验收和tester CPU/材质耦合RED资产是G1解冻前置，G0真机probe仍须先过。本文给出可直接开发的默认选型与拒绝/回退规则，但不以设计完成宣称G1已解冻、P1已开工或M1达成。

## 10. 跨栈差异清单与待签收状态

### 10.1 跨栈差异（编号冻结，变更须另版配方/版本）

| ID | 旧Canvas与P1差异 | rev2处理 | 需要另版配方？若要保留旧效果 |
|---|---|---|---|
| X1 | 旧`drawSandColumns`在三带底色之后另画8px稀疏二维spark tile，颜色`rgba(255,252,240)`、三档alpha；它与列级`sparkGain`是不同代码路径 | `P1_ALBEDO_DELTA_V1`只含列级sparkGain；二维overlay不进P1。禁止暗补overlay，也禁止Canvas在GLES后叠画沙面亮粒 | 本版不需另配方；如需保留二维spark效果，须另定资产/基色提取/组合规则并升级`materialRecipeVersion`及版本清单 |
| X2 | Canvas旧路径使用Hi palette与坡度/shadeLut伪光照；P1低端palette为Rbase并由统一Lmat照明 | P1去旧Hi/shade第二光场；P1 OFF虽`Lmat=1`，也**不是**旧Canvas E底色逐位复制 | 恢复旧高端基色/伪光照须另定资产与基色配方、核身份及版本；不得给当前配方临时加回shadeLut |
| X3 | Canvas EAB CLOSED、s133 E3 PASS、P0诊断热图都不是P1完整GLES合成栈的结果；P1的palette/Q/C warp/色彩surface/三层合成均改变比较身份 | OFF跨栈基线重出；旧结果留在原renderer/build/config身份，不能继承P1效果分数、PASS/CLOSED或适用范围 | 本版新基线不需另配方；若目标是保留旧E效果，必须另立资产/基色配方和版本并重采，不能借旧报告放行 |

X1–X3依据designer确认稿 §4.1与§8。**designer §8 五项必须重采：**①四场景下P1 OFF/ON的H/L/M色彩分布、CPU/GL/屏幕profile、Q上限/负裁统计及二维spark差异；②B亮/暗面绝对高频/RMS、OFF→ON保留率及旧B锚点适用性，C1 prominence/row-mean/周期/有效层厚和C3边界（OFF作新分母）；③`ΔY_face`、`C_band`、`Q_couple`各臂与CPU/Canvas对照，排除OFF变暗造成的伪提升；④三层合成水雾/界面/非沙ROI回归，并以GLES身份重采E3 J95/J90/std分子分母（阈值不变、适用性另审）；⑤同温同尺度Canvas/P0/P1 OFF/ON的CPU/GPU/上传/呈现/资源恢复基线。四类场景、尺寸/参数/快照固定；旧结果保留原身份，PNG原生主测、全帧SSIM只作旁证。

### 10.2 签收、解冻和待实测状态

| 项 | 当前状态 | 依据/下一门 |
|---|---|---|
| designer算法配方/σ6核 | **已签收** | `design-P013-p1-material-recipe-slope-confirmation-v1.md` v1 §1–§8；manager 2026-10-24 21:55验收。对应本稿§4.2、§5.2、§5.4、§10.1 |
| P1工程稿rev2 | **待manager验收** | 当前修订把签收语义落入工程契约；验收前不解冻G1 |
| G1 CPU fixture/耦合用例与RED | **tester待登记** | 前置包含本rev2完成；tester登记可执行fixture、五量测量/状态分流及RED基线 |
| G0 surface/color/z-order probe | **仍须先过、待真机** | sRGB attachment、三层z序/透明/输入透传；由effector按共享设备队列执行，设计稿不能替代 |
| 数值容差、效果与色彩 | **未冻结/未验证** | tester先测重复性和分布，designer复核，manager冻结；此前报`MEASURED_NOT_CALIBRATED` |
| P1实现/G5 | **未授权/未执行** | manager验收后依G0、G1→G2→G3→G4→G5次序另行放行；本稿不解冻、不占真机 |

工程契约索引：§2绘制责任/顺序；§4 layout2、坡度核、六图采样与七版本；§5唯一色彩链/材质配方/mask/五量测量；§6桥/双代/配置原子性；§7时序/模式/错误；§8遥测；§9验证矩阵与G0–G5门序；§10跨栈差异和签收状态；§11修订索引。

## 11. rev2修订索引与变更记录

| # | 位置 | rev2写入内容 | 唯一依据 |
|---|---|---|---|
| R1 | 页首/§1 | 标为工程设计rev2；designer算法签收已完成，P1 manager验收待办 | designer确认稿v1 §1–§8、任务6adcb885 |
| R2 | §4.2 | σx=6列/R18/37项/一次归一化；σz=0；复制边界、mask、Float64到Float32；P1列点差分与P0连续差分非逐fragment等价 | designer确认稿 §2.1–§2.2、参考JSON |
| R3 | §5.2 | 低端palette/Rbase取色和顺序；M基线181/182/131；C1 128/255；C3索引/样式值；Rmat仅最后下界max与统一Lmat | designer确认稿 §4–§6 |
| R4 | §4.4/§5.2/§5.4 | 七运行版本、S_D非S_G的z、C不模糊、C3阶跃单列；methodId与Ycode、ROI/支持集/分母错误分流、五量及实测/校准状态 | designer确认稿 §1、§3.2、§5、§7 |
| R5 | §9/§10 | X1–X3跨栈差异和“是否另版配方”决策；明确五项重采内容；tester/G0/G1门序及签收状态 | designer确认稿 §4.1/§8；本任务卡 |

全稿自检：P0仍layout1/E2_APPROX_P0/zOrigin1；§2/§4.1 layout2、§5.1唯一色彩链、§5.3 mask与绘制职责未更改；E3 J95/J90/std及E/A/B CLOSED状态和已批准终点协议均未变；8–20/255、[0.8,1.2]仍只是旧扫描域；未新建数值容差线；G1未自行解冻；未宣称实现或验证。

- 2026-10-24，rev2（任务6adcb885e4b0127832670739）：将designer签收稿v1 §1–§8和数学参考写入§4/§5；增加X1–X3跨栈差异与五项OFF基线重采清单；将五量方法/ROI/异常分流写入§5.4与§9；更新签收、G0/G1门序和待签状态。纯文书零代码；未用真机；待manager验收，不自行解冻G1。
- 2026-10-24，rev1（原v1，任务6adc861c…d612）：依据P0验收与P1任务，交付六图上传/同一材质光场/完整沙区合成、唯一色彩链、版本/接口/生命周期/配置/绘制责任、帧时资源风险与遥测、测试矩阵及G0–G5。最初登记上游算法待designer确认；rev2由签收稿更新该状态。
