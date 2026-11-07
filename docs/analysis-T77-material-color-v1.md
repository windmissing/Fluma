# T77：默认 pbuffer 与 SRGB8 FBO 色彩差异归因 v1

日期：2026-10-26。角色：designer。原卡 `6adeedbce4b06ee3573c1957`。技术路线：`P013_COLOR_TARGET_ATTRIBUTION_V1`。状态：**归因与补证／条件修复任务书交付，数值和设备呈现未验证通过**。不使用真机，不改产品、shader、配方、CPU主参考、测试判据或原始报告。

## 1. 裁决

73.7203/255 与 1.9563/255 **不能作为同一种材质误差解释**。主差异具有“默认宿主目标读出的码值接近线性量、CPU参考输出为sRGB编码量”的数值特征；FBO剩余误差另有可定位的取整精度反例。先分离目标链与数学链，不加gamma、不改材质配方或误差阈值。

| 层级 | 本轮可确认 | 还不能确认／路由 |
|---|---|---|
| 默认pbuffer目标链 | 已有报表的OFF无增量色块：CPU145，GPU72，接近255×EOTF(145/255)=72.20293。所有非空fixture的“报表最坏点”按此域变换，残差最大约0.500833码 | 是写转换未实际发生、宿主读回／默认表面实现差异，还是未记录的target/state问题，尚未分离；交tester常量shader矩阵 |
| SRGB8 FBO数学链 | 同一简单OFF fixture最大差约5.68e-14码；全35组最大1.956265码。最坏像素附近存在F64/F32的baseByte取整分叉，能解释其量级 | 尚无GPU中间量证据，不能把全部残差定性为舍入／dither或准许通过；交tester分阶段样本 |
| API/attachment | 原执行器对默认GL_BACK做GL_SRGB检查并运行成功；宿主日志列出ANGLE GLES3/Intel D3D11环境 | 查询结果不是功能／屏幕证明；实际EGL query、逐目标read/draw绑定、write-control状态、FBO实际编码数值没有完整归档 |
| 平台／产品 | 这是Windows ANGLE离屏自测；不是Harmony XComponent窗口或最终显示链 | 不宣称ANGLE已被确认有bug，不宣称设备必错或必对。T57必须同帧A/B/C；T56结构集成可继续 |

T55的structuralStatus=PASS只覆盖其二值alpha、沙外零、OFF光向不变等结构检查；两目标numericStatus都是MEASURED_NOT_CALIBRATED，**本轮不升级成数值PASS**。T55最新bridge的ENVIRONMENT_BLOCKED由T76另行处理，不与已成功运行的真实GLES材质执行器混为一项失败。

## 2. 输入身份与复算范围

- T55交付 `docs/developer-T55-material-pass.md`，提交 `b4561a233a4ddc0e41c7fc709754921572c5aec7`。
- `test/reports/t55/gpu-reference-default-pbuffer.json`：35组、最大73.72034040029385码；`gpu-reference-srgb-fbo.json`：35组、最大1.9562652821043685码。单位都是8bit显示码绝对差，不是百分比／ΔE／线性辐射误差。
- 两报表的fixture名称、资产hash和版本相同。真实资产 grain=`cb8aa193b4c859c6f790f9703b6411fd59437ed23e91b6f0e9c32aed5f484fec`，strata=`97bec08be2be9391e09ef132ee35f949cfeebf84186d875603bedc503df22b8a`；另有明确SYNTHETIC_Q_BOUNDARY压力资产，不能冒充产品颜色。
- 将当前六文件与b4561a2的文本统一CRLF→LF后逐项相同：material_pass.cpp、p1_material.vert/.frag、p1_material_functions.glsl、material_gpu_test.cpp、p055_material_fixtures.cjs。原文件字节因换行不同，不能写成“raw逐字节一致”。组合vertex+fragment/helper SHA256=`204c7fe5f78005eee50954ff2f3f19b368dd8d88968fade9fc344eba0d68b2e6`，与T55归档shader-manifest相同。
- 执行器用真实MaterialPass/MaterialGlGpu和ANGLE GLES3，报告环境为ANGLE2.1.18846、Intel UHD630、D3D11。本文只读取既有执行输出，没有重新运行GL、设备、编译或独立客观测试。
- 当前 `.codex/local-test/t55-fixtures/` 的35个RGBA均与FBO报表hash相同，只有4个同时匹配default报表（空／同值内容可能同hash）。两目标原脚本共用输出目录，会覆盖上一目标的RGBA；不能据当前文件伪造default全图。default的数值推断只使用其已归档JSON点／摘要，不冒充全图线性重算。
- 本轮资产与源码身份、35组最坏点域变换、FBO具体像素重算和原始输入SHA记录于 `docs/analysis-T77-material-color-evidence-v1.json`。scratch资产是只读分析输入，正式补证应另存不可变目录。

## 3. 编码与数学的唯一比较口径

保持签收链：`SRGB8资产/底色 → EOTF一次 → Q及材质增量 → 总和max0 → Lmat一次 → Reinhard一次 → 线性显示输出 → 目标硬件OETF一次`。原材质数组是RGBA8字节载体，手动EOTF；新fullframe纹理是SRGB8_ALPHA8，采样硬件解码，不能再手动EOTF。这两类纹理不可混用。

设 E 为EOTF、O为OETF，CPU主参考输出 `C=255×O(Ldisplay)`。若目标实际读回接近 `255×Ldisplay`，直接拿它与C比较，会产生本次约73码的巨大差异。反向套E只是一项诊断假设，不是修复原始读回／改主测域的操作。

依据 [EGL_KHR_gl_colorspace](https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_gl_colorspace.txt)，请求sRGB与相应GL attachment编码有明确定义；依据 [GLES3规范§4.1.8、§4.3.2](https://registry.khronos.org/OpenGL/specs/es/3.0/es_spec_3.0.withchanges.pdf)，写入转换、所选read buffer和ReadPixels是不同阶段，不能把读回当作texture采样而自动套一次EOTF。[EXT_sRGB_write_control](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt)存在时还要核实该开关；没有扩展不能盲目调用0x8DB9，也不能套用桌面OpenGL“必须enable”的惯例解释GLES结果。

任何目标域解释都必须附实际绑定和功能色块；A属性、B数值、C截图分别报告。离屏pbuffer没有compositor／显示证明。

## 4. 主差异：默认目标更像线性码，而不是材质配方整体错

### 4.1 两个可复算的点

| fixture／点（GL读回坐标，左下原点） | CPU sRGB码 | default GPU码 | 255×E(CPU/255) | 解码域残差 |
|---|---:|---:|---:|---:|
| flat_0_0_0_0_0_0_0，(16,16)，R | 145.00000000000003 | 72 | 72.20292880964801 | 0.20292880964801 |
| pulse_1_1_1_1_0_0_0，(62,33)，R | 135.72034040029385 | 62 | 62.50083306663493 | 0.50083306663493 |

第一项关闭Lmat/B/C/spark，只需Q与ToneMap互逆回到EOTF(palette)。同一FBO fixture全图最大差约5.68e-14码，原图(16,16)确为RGBA=[145,168,111,255]，default却差73码。故先改B/C、坡度核、光照或噪声不合理：在这些因素关闭时主差异已出现。

31个有非空worstSample的fixture做相同诊断变换，残差最大0.500833码；这不是全图最大残差，也不是新容差。每个fixture的worstSample是在原sRGB比较域选出来的，不一定是变换后最坏点。

### 4.2 尚不能给出的唯一底层根因

最小假设 H1：**该宿主default-pbuffer的fragment写入→储存→ReadPixels链与“可直接比较sRGB显示码”的假设不符**。现有数据强支持其读回具有线性码特征；但无法单独定位写入端、读回端或实际状态／backend语义。

执行器确实请求EGL_GL_COLORSPACE_SRGB_KHR，并对默认GL_BACK编码CHECK==GL_SRGB。源码没有eglQuerySurface实际colorspace记录；FBO建为GL_SRGB8_ALPHA8并检查complete，未分别记录该FBO的attachment编码查询；两种目标也没有逐次记录read/draw framebuffer和read buffer、write-control状态、dither状态。因此不能用“attachment报告SRGB”反驳数值异常，也不能仅靠异常点断言“某行应用漏了enable”。

原日志的GL扩展列表没有GL_EXT_sRGB_write_control，执行器仅在该扩展存在时enable；这条条件分支本身符合GLES合同。需先核实实际上下文／状态，禁止去掉扩展门后无条件enable，禁止把代码改为shader OETF来适配这一台宿主。

两个执行分支跑在不同启动中、仅共享源码与夹具；没有同进程交替目标、常量输出和独立存储观察，故不宣称宿主driver已被确诊。程序未调用eglSwapBuffers；sampleStage=PRE_SWAP_READBACK表示离屏绘制后、未swap的读回位置，不是已经观察到设备窗口交换链。

## 5. FBO约1.956码残差：存在明确的取整分叉候选

FBO最坏fixture `flat_0_1_0_1_0_0_0`（OFF，B开，C关，spark开），GL像素(88,78)，blue通道：CPU155.04373471789563、GPU157。读取该fixture现存原图，SHA与FBO报表匹配，实际RGBA=[247,219,157,255]。

按现存Float32 T_data和同字节grain重算该点：物理高度62.5，L带，grain texel=(78,200)、RGB=[168,148,102]，palette=[216,196,150]，插值gain=1.149999976158142。这里已用实际上传Float32 profile，不是把原高精度profile误当GPU输入。

- CPU参考以JS double计算：150×gain=172.4999964237213，floor(value+0.5)=172。
- 按binary32乘法的示例运算，乘积可舍到172.5，再floor(+0.5)=173。CPU已经把输入转Float32，并不等于后续每个乘法也以Float32执行；shader还经历palette/255→uniform→×255。
- 固定其它输入与材质数学，仅把baseBlue172换173：参考blue从155.04373471789563变156.5341593505633，量化到157，吻合该原图blue。

这是**可计算的精度分支反例和强候选**，尚未直接读出GPU的baseByte=173；实际GLSL中间精度／优化和其它点仍待tester探针。不能据此把所有1.956码都解释成同一原因，也不能称其只是“8bit输出的半码量化误差”。base的1码在Q增量／ToneMap组合后可以放大为更大的显示码差；pow近似、其它取整边界、相位texel选择和dither还需分别排除。

### 5.1 比较参考分层，禁止替换主参考

1. 保留冻结数学参考（F64）及其sRGB主测，继续报告原差异。
2. 新增**只作诊断**的操作顺序／binary32影子参考，分列baseByte、texel索引、Q/Rpre/Lmat/ToneMap，不用它覆盖主参考或作为新放宽阈值。
3. 探针若证明只有离散baseByte不一致，再决定“契约的数学取整是否要求在CPU预定字节，或显式定义确定的GPU取整表示”。前者涉及per-fragment fH/gain和上传接口，不能只上传列中心字节；后者涉及数值表示版本。均须先由designer/architect写明局部接口与旧版本关系，不能偷偷改公式、加epsilon、改gain或直接换掉CPU期望。
4. 不在没有探针前为该残差新开物理／配方返修，不改既有Q255上界保护或C3阶跃。

## 6. 给tester的精确补证任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md` 其它内容，仅最新有限例外可改自己行状态；不改滴答同名清单。不改生产shader／配方／主测阈值，测试探针使用独立标识。所有渲染仅新GL/XComponent+shader；无Canvas上屏或回退，包括背景前景；失败FAULT或标明STALE的新管线旧帧。多人共享文件，不覆盖T56/T75改动。由manager登记／派工，本会话不建卡或改他人状态。

### 6.1 H1目标链最小矩阵

新建独立输出目录，每个target／run分别存raw RGBA、完整API日志、fixture和shader/exe/DLL SHA256、比较脚本版本与manifest，不覆写T55历史。优先在同一上下文交替绑定目标，排除两启动状态差异；记录actual driver/version，新增backend仅在已有可用环境中进行，不绕过系统保护。

| 目标 | 已知构造／应记录 | 比较域 |
|---|---|---|
| 显式RGBA8 FBO | internalFormat=RGBA8、实际encoding=LINEAR、complete | fragment输出L→约255L；不对该目标套O后声称原值正确 |
| 显式SRGB8_ALPHA8 FBO | 格式、actual encoding=SRGB、complete、write-control支持／状态 | fragment输出L→约255O(L) |
| EGL LINEAR default pbuffer | 请求与eglQuerySurface实际colorspace、config／GL_BACK实际编码 | 与实际查询及功能结果分别核对，不预设所有default一致 |
| EGL SRGB default pbuffer | 同上，draw/read绑定=0及GL_BACK、各次错误 | 同样输出L，检查是否仍呈线性码特征 |

每target用独立常量fragment，输出L=0、0.0031308、0.18、0.5、1和实际H/L/M palette的EOTF值；记录shader输出定义。0.18：linear约45.9码、sRGB约117.6458码；0.5：linear127.5码、sRGB约187.516码。已编码0.5先E再输出应约127.5码；把0.5直接当L输出则约187.516，是重复编码反例。精确量化／实现重复性按原合同测量，不在本文冻结容差。

必须显式设置／记录：GL_DRAW_FRAMEBUFFER_BINDING、GL_READ_FRAMEBUFFER_BINDING、draw buffer/read buffer、viewport/scissor、samples、color mask、GL_BLEND及方程／因子、GL_DITHER、PACK状态/PBO=0、ReadPixels格式类型、上下文线程和epoch、eglGetCurrentSurface、EGL/GL每阶段错误。默认查询用GL_BACK，FBO用COLOR_ATTACHMENT0，不能查错attachment后抄标签。扩展按token精确匹配；无write-control则记NOT_SUPPORTED并按GLES规则，不虚构enable状态。

先disabled-blend常量fragment，再alpha1、alpha0和独立fractional-alpha合成探针；clear色块只作对照，不能用clear成功替代fragment写转换。原MaterialPass在二值alpha1时的over理论上不应造成73码域变化，但仍需用状态对照验证，不凭理论排除backend实现差异。

区分写／读：显式sRGB纹理FBO使用已知sRGB字节的受控上传→直接ReadPixels，与shader线性写→ReadPixels及sample-to-LINEAR-FBO形成三角对照。采样路径会硬件解码，不能把它与直接ReadPixels混称同一种读回。默认pbuffer若有可验证的独立存储／共享纹理观测路径再加交叉证据，记录该路径自己的格式／转换；不可用重复一次同API读回宣称已经排除ReadPixels问题。不可得就保持WRITE_VS_READ_UNRESOLVED。

### 6.2 FBO残差最小探针

固定本报告(88,78)及邻近跨gain半整数点：T_data及资产hash、fH/gain、H/L/M带、grain/strata整数texel、baseByte、Rpre、Lmat、ToneMap逐级输出。shader debug variant单独hash，不代替生产hash；整数base／索引可用整数编码或整数attachment，连续量用支持且实际complete的float target，若不支持如实报告，不能低精度RGBA8反推高精度中间值。

比较四层：冻结F64→操作次序F32诊断→真实GPU中间量→最终sRGB字节。先OFF且B/C/spark全关，再只spark，再B+spark（本反例），再ON、C、镜像和Q0/255压力夹具。dither先记录生产默认，再明确disabled的测试分支；分开报告，不能改生产开关后把旧对照覆盖。

同shader在两个目标的base/linear输出应分别核实。若只有输出target差异，归H1；若base或texel已不同，归数值表示／采样边界；若base相同而后续不同，再定位Q/pow/ToneMap，不靠肉眼调整配方。

### 6.3 交付与T76边界

输出独立色彩矩阵报告、raw、最小反例、实际状态和未决项，不负责整体归因。T76原卡负责上传/身份/收尾/被动诊断与bridge环境；本色彩矩阵是新增精确补证需求，manager决定另卡或追加清楚范围，不能默认T76已覆盖色彩GPU测试。

## 7. 若补证确认缺陷：developer最小hunk路由

| 证据结果 | 最小责任文件／hunk | 禁止扩大 |
|---|---|---|
| 目标绑定／read buffer或日志标签错误 | tester执行器/读回采集；产品若也复现则修scene target绑定／读取诊断hunk | 不改材质shader以补一个采集错误；测试修订保留旧报告 |
| 有write-control支持且被实际错误关闭 | egl_context.cpp或scene target切换处的显式状态建立与回执，按已证阶段恢复正确状态 | 不无条件启用无支持token，不把启用代码塞进材质数学helper |
| 宿主default仍查询sRGB却功能不符，显式FBO正常 | 记录该backend的能力未通过；若需换backend／修平台创建须针对该实现最小复现 | 不以FBO替default签PASS，不全平台加shader OETF／gamma，不把宿主故障外推Harmony |
| GPU baseByte／texel分叉确认 | p1_material.frag与preparation的局部表示／取整契约；必要T_data/types按明确新revision扩展，designer/architect先落接口 | 不用epsilon／调palette/gain／模糊纹理求贴合，不覆盖T56/T75接口或冻结主参考 |
| fullframe采样／present二次变换确认 | T56 scene_compositor/fullframe_targets/present shader及实际纹理格式/采样职责 | 材质RGBA8手动EOTF与fullframe sRGB硬件解码分清；不得两边都加转换 |
| B数值正确但C截图偏差 | 设备surface/dataspace、XComponent／系统合成色彩配置，按实际证据定位 | 不以C去逆调Rmat或Lmat；截图也不等于物理面板校准 |

以上是条件任务书，没有证明相应缺陷前不授权盲改。开发卡完成仍以开发／编译／适当离线自测报告为交付，设备复测独立管理，不加manager验收门。任何不支持／失败按新合同FAULT或STALE上一有效新管线帧，无Canvas。

## 8. T56可继续，T57必须采A/B/C

T56可以继续完整scene结构：统一FrameToken/资产代、背景→沙/真实粒子→前景、candidate/LastGood、成功swap后lease提交、context/resize恢复和资源计数。本次没有证据要求把完整集成等待在宿主pbuffer归因上；但不能把“SRGB8 FBO差小”写成产品色彩通过，也不能仅满足attachment查询就签颜色闭环。

按T68的既定链：各pass输出显示线性预乘；candidate/LastGood为sRGB纹理则写时硬件编码，present采样硬件解码后输出linear，再由真实sRGB window写编码。present不加手动OETF或EOTF。FBO到present使用全帧复制shader的恒等色块对照，不能用未经定义的blit转换替换合同。

T57设备A/B/C最少数据：

- **A**：真实EGL window请求／实际colorspace，所选config通道，default及candidate/LastGood逐attachment实际encoding，write-control支持／状态，texture内部格式，context/surface epoch，shader/source与资产hash；不能继承旧G0为本完整scene作证明。
- **B**：同静态FrameToken、同代资产、同参数，常量fragment／OFF/ON／B/C/spark消融；分别采candidate写后、present前的线性诊断路径和window swap前readback，记录每阶段绑定与坐标／行方向。主测Ycode与线性诊断分列，snapshot只做测试入口，不每显示帧readback。
- **C**：同内容新管线marker、最终无损原生PNG／原始hash／可得的ICC或dataspace、系统显示色彩/HDR设置，图像ROI和对应frame身份；同帧关联能力不足明确UNPROVEN。背景/前景也在shader内，不启动Canvas作现场参考。旧Canvas图只作离线只读历史对照。
- 补fractional-alpha雾/高光/边框的线性预乘over，区分局部沙外RGBA0与最终完整背景alpha1；保留context恢复/resize/LastGood再呈现的A/B/C关联。不能从不透明材质35组外推全scene半透明正确。

T57沿已有冻结四黄金场景、原质量/耦合/性能判据及颜色方法身份。尚未冻结的数值容差只记MEASURED_NOT_CALIBRATED并提交原流程裁决，不现场用1.956或73.72自动定阈。指标失败按原标准失败；本报告不降低任何门槛。

## 9. 交付结论与限制

已完成目标链／数学链分流、主差异的码域反例、残差的精度反例、精确tester需求、条件developer hunk与T56/T57边界；另有证据JSON与独立复盘。T77设计交付完成，宿主底层WRITE/READ原因与设备最终颜色仍待后续证据，未标VALIDATED。本轮只改T77本行状态并回原卡，manager登记与派工，不重开T55、不改他人队列或滴答同名清单。
