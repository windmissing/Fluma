# T87 色彩矩阵到件工程分流 v1

日期：2026-10-26。architect。原卡 `6adf041ee4b0127832b2c3fc`。不使用真机。状态：**到件分流完成；测试采集缺口明确，产品底层色彩根因未闭合**。

## 1. 消费范围与裁定

消费T80提交53a88be的报告 `test-report-T80-h1-target-matrix-v1.md`、canonical/raw JSON、environment.txt、三个raw RGBA及执行器源码；消费T55旧执行器/日志、T77和T84文档。本轮只读证据/代码并查规范，不重跑测试，不改变原报告或用例。此稿补充 `architecture-P013-color-routing-revision-v1.md`，有冲突时本次到件解释优先，原raw不改。

T1未请求colorspace，读到线性码；T3 RGBA8结果与之相同；T4 SRGB8_ALPHA8有编码特征，这些**观测保留**。T2是执行器主动SKIP，**没有创建失败证据**。不能把T1（未请求）外推成T55（显式请求）的静默降级，更不能认定宿主不支持sRGB pbuffer。T55主测仍MEASURED_NOT_CALIBRATED，WRITE_VS_READ_UNRESOLVED；T4剩余误差和GPU baseByte分叉仍待证。

本轮直接读取归档raw：三文件均16384 bytes，中心(32,32)分别为T1/T3 `[55,55,55,255]`、T4 `[128,128,128,255]`，对应最后一个常量。T1/T3 SHA256均 `3b85797717f19b519ee47d98fe1b8b8348e38e1d56d007216e5fe523b5484273`；T4为 `d8d553546719ecc9637714ba865b13783ef9f55aa28c0b79b0dfe2e7042cc38a`。这是既有raw核读，不是新GPU执行或全矩阵复测。

## 2. 采集与解释逐项核对

| 项 | 源码/原输出事实 | architect分流 |
|---|---|---|
| EGL扩展缺失 | environment为EGL **1.5**；执行器仅按EGL_KHR_gl_colorspace子串决定T2，未创建便SKIP | EGL1.5核心已有colorspace，缺扩展名不等于缺核心能力；T2属于测试门控遗漏，不是已测UNSUPPORTED。 |
| T1实际query | `colorspace=12426=0x308A=EGL_GL_COLORSPACE_LINEAR`；T1属性未请求颜色空间 | 与默认linear一致，不是SRGB请求被降级的证据。T55旧程序另有SRGB请求和GL_BACK actual encoding检查成功，两者不能混成同一surface。 |
| default attachment | 源码用glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT)，输出`GL_BACK_impl_read_format=0x32993` | 不是attachment encoding查询；32993是十进制，实际十六进制0x80E1，不能把前缀0x加十进制文本当枚举。旧T55用glGetFramebufferAttachmentParameteriv(...GL_BACK,...COLOR_ENCODING)才是对应查询。 |
| GL write-control | runCurrent在EGL extension串查GL_EXT_sRGB_write_control；T3/T4只留空后缀 | 此字段不能证明GL扩展不存在。旧T55 GL日志可以描述其旧上下文，不能替当前context查询。缺扩展时不调用0x8DB9；存在则显式查询/建立并记录状态。 |
| 第六个常量 | 源码`1.055*pow(145/255,2.4)-0.055`，结果约0.21576；canonical Lvalues末项null | **既非EOTF也非OETF**，公式/标签有采集缺陷。观测55/128只对应实际错误公式产生的值，不是palette145的正确基准。正确EOTF高段是`pow((s+0.055)/1.055,2.4)`，约0.28314874；OETF高段指数1/2.4。 |
| 187 vs188 | L=.5的T4观察187，记录dither=1，只有该常量的单点回读 | 不能从一个点断言“截断”或与baseByte同因；保留原观察、浮点输入和量化残差，缺关闭dither及重复空间分布/中间量。 |
| context身份 | T1用context1，T2若执行才复用context1；T3/T4在新context2 | 是同进程，**不是四target同一context**；不能声称已经排除context状态差异。FBO调用把surf参数传NO_SURFACE，但实际有current pbuffer，日志no-surface指未查，不是没有绑定。 |
| 错误与raw | drawErr/readErr追加后被`r.errLog=qErr("state")`覆盖；query在其后；每target raw仅保存最后一个常量全图 | err=NONE不是全阶段无错误证明；raw不能还原前五常量全图。原矩阵JSON末项有尾逗号，canonical另行转录，必须保留二者身份，不能冒充同字节合法JSON。 |

这些属于**测试采集/能力判定工程缺口**，交tester修执行器与版本化补证；没有产品漏enable、ANGLE静默降级或材质算法错误的确定证据。不是测试阈值争议，不需为纠正错误query/公式放宽指标；若要改主测域/判据才交manager裁决。

## 3. 规范依据及生产边界

[EGL1.5规范§3.5.1–3.5.2](https://registry.khronos.org/EGL/specs/eglspec.1.5.pdf)定义核心EGL_GL_COLORSPACE及pbuffer属性，默认LINEAR；SRGB与LINEAR对应GL attachment编码。应使用运行时eglInitialize major/minor（并记录version串）识别核心路径，较旧版本再核对应扩展；成功创建、实际query、GL actual attachment和功能色块仍须逐项验证，核心版本并不保证所选目标功能一定成功。

[EXT_sRGB_write_control](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt)是GL能力，查询当前GL上下文扩展列表，token完整匹配；EGL串不替代GL串。生产 `egl_context.cpp` 已有EGL核心版本/扩展门、实际surface及GL_BACK编码检查，并从GL串查write-control的工程路径；本次证据不授权删除这些门或加全平台shader OETF/gamma。

所有画面只走XComponent+shader，包括背景/前景；禁止Canvas上屏或失败回退。真实失败明确FAULT，或符合T68兼容条件时标明STALE的上一有效新管线帧。UNKNOWN/未测颜色能力不伪造成编码正确或当前成功。

## 4. 给既有T88的精确补充，不重建矩阵任务

T88 `6adf041ee4b06ee3573ec0b4`继续T77既定fractional-alpha与baseByte探针；manager可将下列**同一执行器的采集纠正和缺失目标补齐**并入其范围，不重复已有T1/T3/T4有效观测，不重开T80，不增加manager验收门：

1. 使用运行时EGL版本或精确扩展选择colorspace路径；同一current context切换T1/T2/T3/T4，T2真正尝试核心SRGB创建，记录属性、config、return/errors与实际query。失败保留准确API错误；不支持才明确UNSUPPORTED。上下文必须变化时分组标记，不称完全同context。
2. default用正确GL_BACK attachment encoding查询，FBO用COLOR_ATTACHMENT0；read-format单独字段、枚举同时输出十六/十进制。记录actual current draw/read surfaces、context、read/draw framebuffer和buffer、viewport/scissor/mask/blend/samples、PACK/PBO与逐阶段错误；不覆盖早先错误。GL扩展从当前GL取，记录write-control支持及实际状态，不凭空enable。
3. 保留原错误第六常量作为historical id；新增明确EOTF(palette)/OETF对照，归档实际double输入及上传float位型，canonical不能用null省略值。每常量/target独立raw或明确只有sample证据，自动从原始数据生成合法JSON与SHA manifest，避免人工转录/目录覆盖。
4. 187/188只作观测；生产dither与disabled测试分支、重复像素/多次采样分列，不能把单点差认截断。fractional-alpha和受控字节上传/采样三角对照继续；真实baseByte/索引/gain/Rpre/Lmat/ToneMap按T77§6.2，F64主参考与F32影子仍分开。

测试修正责任文件：`test/p013_native/t80_target_matrix_test.cpp`或独立T88版、构建/manifest脚本与新报告。保留53a88be原源码/raw/JSON，新增明确revision（例如T88_COLOR_CAPTURE_V2），不改生产shader/配方/CPU主参考/阈值。补证不整体归因，结果回architect。不开新的大规模backend调查；确有能力缺失再记录具体target和错误，不外推所有ANGLE或Harmony。

## 5. 产品工程条件任务书与设备交接

目前**不派产品修复**。若新证据证明真实产品绑定/状态/颜色空间错误，先修订T84目标工程hunk，再由developer只改egl_context创建恢复、target bind或scene present对应阶段；若GPU输入相同但baseByte离散表示分叉，再交designer定义表示revision。采集错误只修tester执行器；阈值/主测域争议交manager；未实现功能预测试记NOT_IMPLEMENTED，不冒称颜色PASS。

T56结构继续，不等本归因。T57设备A/B/C须采：A真实EGL核心版本与EGL/GL各自扩展、创建请求/实际query/default与fullframe编码、write-control状态和epoch/hash；B同frameToken/资产代的常量与材质/半透明、candidate与window明确采样阶段及绑定；C同帧原生PNG/ROI/可得dataspace与系统设置。swap不是物理显示证明，离屏结果不推断Harmony。设备卡带真机标签且同设备串行，不把设备结果加为开发关闭前置。

## 6. 交付

已交付证据收窄、采集工程修订与T88/T57精确路由；底层write/read和数值表示继续未决。未实施产品/测试修改、未重跑测试，未使用真机。仅自身行结束待验收并在原卡报告，清单全局/后续派工留manager。
