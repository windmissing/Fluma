# T121 色彩V3常量、摘要与归档工程分流 v1

日期：2026-10-26。architect。原卡 `6adf3c1ce4b017ab3404b7cc`。不使用真机。消费T115 459bd4a报告、`t115.json/p115_results.json`、probe/driver/build脚本及T87/T105。**部分交付：目标编码点观察接受；真实SHA、生产/debug等价与raw归档缺关键证据，原卡看评论**。

## 1. 已到件与不应扩大部分

probe实际建GL_SRGB8_ALPHA8 texture FBO、查询COLOR_ATTACHMENT0=SRGB且complete；另建RGBA8查LINEAR，default GL_BACK查SRGB；EGL query12425=0x3089=SRGB。生产点247/219/157与同目标**常量**shader播种ToneMap值输出同点一致，有编码路径正观察，不能再写“host无真正编码路径/缺KHR等于静默降级”。

debug是常量uniform色，不是生产frag加整数MRT的等价变体；全图diff168676正是不同内容，不能用于T101/T105所需生产/debug同目标全图等价。该点验证的是受控输出编码特征，不替代所有生产数学链/纹理/光照/半透明/最终屏幕。numeric仍MEASURED_NOT_CALIBRATED，设备A/B/C未测，无gamma/新容差授权，T114条带基础实施继续。

## 2. 常量值：代码已纠正，报告和断言没有核数值

T115源码OETF函数是正确分段形式，实际JSON `oetfOf145=0.778868`，LE hex `003151e57cece83f`解码约0.7788681486941016，**并非报告/F01标题所称0.215760**。本轮只读独立算术：

- EOTF(145/255)=`((145/255+.055)/1.055)^2.4`≈0.2831487404299921。
- OETF(145/255)=`1.055*(145/255)^(1/2.4)−.055`≈0.7788681486941016。
- T80错误公式`1.055*(145/255)^2.4−.055`≈0.21716935993161873；历史报告的0.215760标签也不精确，不能再次作为期望。

F01只检查typeof number和hex长度16，完全不比较函数expected/hex对应值，也未画六常量矩阵。故“正确第六常量已测试通过”过度。后续应分开正确EOTF线性输入/正确OETF诊断值/历史错公式id，保留原raw，不改主测域或私定容差。

## 3. 核心工程缺陷：所谓SHA256不是正确摘要

probe自带tinysha压缩轮的`t1=h+S1+ch+K[i]`**漏了消息字`w[i]`**，虽然扩展了消息schedule却从未在轮中使用。该算法的摘要依赖长度而不正确依赖消息内容，64字十六进制不是SHA256证明；同长度生产/debug raw明明diff168676却给出完全相同prodRawSha256/dbgRawSha256正与此缺陷吻合。driver D01只用正则验长度，未独立验摘要，不能签“真实SHA”。

本轮Node标准crypto对JSON内**相同uniformBits原字节**独立核算SHA256=`49cdf069439d14a3d8efa02677e7204ca037ce3b5b5ae1c6fc0e0d8c326cdf7e`，报告却为`da5698be17b9b46962335799779fbeca8ce5d491c0d26243bafef9ea1837a9d8`。源码中stageShaderSha256实际哈希占位串`stage-pass-not-rebuilt-here`的25字节，并非任何执行stage shader；uniformBits仅pal六float，不含完整实际uniform集，不能声称所有上传位型身份闭合。

现存probe debug源码（按R字符串LF字节取出）标准SHA=`4a404e64fb5c44d5e9a939e0b13dbee1707a5f8905c5e3970c8350a05df8b27e`，不等JSON所报；行尾/构建源码实际字节仍须独立manifest固定，不拿本地文本核读倒填历史运行。错误tinysha所有字段均需视为**不可信摘要**，不是只改raw两个字段。

## 4. 其它“recorded”字段与归档缺口

defaultBack.triangle.writeRead=recorded只是硬编码字符串；源码tri[0]仅写hasFloat标记，没有受控字节上传/直接读回/采样三角执行，因此C01检查“字段存在”不能证明写读差异已排除。fractionalAlpha.backEncoded=128、front描述、blend因子描述均是硬编码/沿用T88说明；本probe没有画该合成矩阵，只有EOTF(128/255)算术和查询当时dither，不能冒称新实际back/front/alpha合成采样。也未独立disabled-dither分支。

stageErrors是**一个字符串**，driver Object.values对字符串枚举单字符、every(typeof string)仍通过，不是逐API阶段错误证据。缺draw/read前后绑定/buffer/error、实际readback状态、source/exe/DLL资产清单及每常量raw完整归档。probe只写t115.json，生产/debug全图缓冲仅内存hash，没有dump文件；hash即使正确也不能替代原始图复核，更何况本次算法错误。不能由manager“另派归档卡”把已经不存在的内存文件凭空补成旧run证据。

## 5. 已确定的工程修订与最小tester任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己行状态有限例外；不改滴答同名清单、不回退共享修改。所有渲染仅新XComponent+shader含背景前景，无Canvas回退，失败FAULT或显式STALE旧有效新管线帧。失败/关键证据不足原卡评论并自己行看评论。

修订责任是**测试采集工程**：`t115_identity_probe.cpp`、`p115_color_identity.cjs`及新独立run目录/manifest脚本，不改产品material/shader/配方/阈值。明确下一版至少如下：

1. native先dump实际prod/debug RGBA、整数stage/float raw（未执行则NOT_RUN）、完整shader源/vertex/helper与实际uniform bytes，return成功前校验文件写完。runner用标准crypto或系统SHA256逐文件算真实摘要并交叉native结果；不能只修tinysha一行继续信未经known-answer校验的实现。至少空串/abc/多block标准向量及同长度不同内容反例真实断言。
2. schema每条manifest含relativePath/byteLength/SHA256/kind/run/target/sampleStage，所有字段指向存在文件；stage未跑明记NOT_RUN，无占位hash。源码hex如需要只是sourceHex，不冒摘要。执行前隔离输出目录/清晰build-exit失败，不从上次遗留t115.json读出“新PASS”；当前driver捕获build失败仍继续读旧JSON有stale证据风险。
3. 用真正生产frag＋独立MRT整数/float probe，同context/同SRGB8_ALPHA8 attachment运行完整相同参数对照，另设RGBA8组。保留常量shader作为编码功能对照独立case，不能替代生产debug。所需stage指T101真实gain/base/Q/Rpre/Lmat/ToneMap与operation diff，缺则明确标记。
4. 六常量按正确EOTF/OETF及实际uploaded Float32 bits画出，F64原位型/端序标明；断言公式和位型一致，报告数值从自动结果生成，不复制错标题。主测误差阈值仍沿原合同，未定数值只报告观测，不能现场定1码容差。
5. default query与受控写/读/采样三角**实际执行**，每阶段实际target/format/encoding/read/draw/buffer/GL-error分立；半透明实测back/front字节、解码dst、alpha/source预乘位型、blend/dither/scissor，生产默认/disabled分别留raw与原公式，不把硬编码128当观测。

T115历史raw/JSON/10项结果保留，新增明确revision（如T115_IDENTITY_CAPTURE_V4），能归档旧文件就保留；旧全图未dump则标UNAVAILABLE并**新run取证**，不伪称补齐原run。最小目标是纠正身份/原始数据与实际未跑项，不重复其它无关测试。补证回architect，tester不负责整体归因。

## 6. 下游范围与交付状态

T114的逐像素F64整数条带/表示接口不等待这份目标采集纠正；它自己的base字节零差与整数上传回归照T105/T113执行。完整numeric/目标/设备闭环仍未通过，不能从T115点247/219/157宣布已修所有残差。真实host有编码路径保留，不外推Harmony；T57仍按同页稳定性与设备A/B/C独立依赖。

本卡已完成代码/数据到件核对及可执行测试修订书，但“真实SHA/原始归档/同内容等价”的关键证据不足，按最新规则原卡报告并本行看评论。无生产代码/测试执行/阈值变更，无GPU新运行或真机，未绕权限。
