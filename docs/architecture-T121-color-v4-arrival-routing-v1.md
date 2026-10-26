# T121：T126色彩V4到件增量分流 V1

2026-10-26；architect；原卡 `6adf3c1ce4b017ab3404b7cc`。消费T126 `d455b67`、`docs/tester-T126-color-evidence-v4.md`、`test/p013_native/t126_identity_capture_v4.cpp`、`test/p126_color_evidence_v4.cjs`及`test/reports/t126/`。本文更新T121 V3裁定；旧文保持历史，不将旧缺项继续列为V4前置。不使用真机，不重跑测试；只读磁盘raw/哈希/源码核对不是设备复测。

## 1. 已关闭的旧缺项

**V4完成真实raw落盘、标准SHA和生产/MRT同SRGB附件等价补证，旧等待条件解除。** 环境为Windows ANGLE 2.1.18846、GLES3.0、EGL1.5；属于离线native GL探针证据，不是真机或XComponent窗口证据。13/13是当前driver断言通过，消费范围如下，不自动扩展为色彩全矩阵或严格数值产品通过。

| 项目 | V4可接受事实 |
| --- | --- |
| 真实同目标生产/MRT | 实际GL_SRGB8_ALPHA8附件、COMPLETE；生产MaterialPass与生产数学MRT变体，在同目标下raw全图diff=0，采样247/219/157 |
| 三份raw | 每份184320B，已从磁盘重算标准SHA；生产/MRT均`dccbe278d1b5d96f17b109115e5137f33092ee0dfea6aa749eae5ee0f0a29008`；常量对照`cf4dad8d2b2c978343ef79d1236977de90a8906fc956a86d8e5eb7929b6c2cf8` |
| 常量对照 | 单独控制路径；与生产全图差168676B，不能作生产等价MRT的第二名称 |
| SHA修订 | Native恢复W[i]；以上三raw与标准库一致，六palette float的24B hash也匹配；不再称这些输出为旧tinysha假摘要 |
| stage | integer base173/grain102/strata127/k1实际执行；float明确NOT_RUN，T101独立f32证据保持不同run身份 |
| 常量更正 | EOTF(145/255)=.2831487404299921，F64 LE `e73903e51b1fd23f`；OETF=.7788681486941016，LE `003151e57cece83f`；历史错公式=.21716935993161873，LE `3abe4ca134cccb3f`，仅保留错误身份 |

上述raw/SHA/同SRGB生产MRT不再索取；不能因为后续分支问题把这些已成立证据全盘撤回。D01 known-answer是Node标准库对参考值，不是直接调用native SHA跑全部向量；三份真实内容的native/标准库一致已有实质价值，不据此要求再交同一组三raw。

## 2. 新发现的窄范围工程问题

### C01实际sample链路尚未执行

源码D段确实向default framebuffer绘制常量并readPixels，故“完全没画三角形”的V3结论不再沿用。但后半段：triTex分配传nullptr，未上传triRead；default FBO仍绑定时调用glFramebufferTexture2D，目标非法；随后绑定r8Fb并用同一常量shader再绘制.5，未使用纹理采样shader/triTex。因此sampleByte=128代表第二次LINEAR附件常量绘制，不代表原写入字节经纹理采样硬件EOTF。末尾先glGetError清空再取triErr会吞掉前面的错误，NONE不能证明整个链路无错误。

归因为**探针工程缺陷**，不是已证产品色彩错误/设计方案失败。developer只修D段：分别保存default绘制/read错误、已知SRGB纹理上传原字节、真正sampler shader采样到明确LINEAR FBO、查询目标encoding/completeness、各阶段先记录错误再处理。tester不使用真机，仅在同ANGLE环境补该roundtrip，保留字节/目标/源码/manifest和逐阶段错误。无需重做A/B已通过的同SRGB生产MRT证明。default attachment报告SRGB但read128的观察保留，不从这个有缺陷sample链路推出跨目标转换结论。

### fixtureBinSha256截短输入

`sha2::sha256(data.data(), data.size())`把float元素数当字节数。磁盘原fixture6144B标准SHA为`32870e766b0d501e17d0093edd00336969354dd2ebae141183b9233d3cccc353`；仅前1536B的标准SHA恰为V4字段`e86423b1369f4b7afe49b218f15188425a109368ec0cbbd1a320672625a15f53`。这是hash覆盖长度错误，**不影响三个RGBA8 raw的摘要正确性**。

developer将typed payload字节数明确乘sizeof(element)，并归档完整fixture/长度与hash身份；tester可先离线独立核对，不因摘要字段错误重跑整套设备采集。不能把旧prefix hash静默更名为完整fixture hash；修订manifest注明旧字段作废原因。源文件/vertex/helper/exe/DLL等尚未全部作为manifest文件归档，若后续T105/严格重演消费需随该次构建补全身份，不能冒充V4已交全量构建包。

## 3. 执行过的分支与尚未执行的矩阵

E段确实执行GL_ONE/ONE_MINUS_SRC_ALPHA分数alpha：dst=EOTF(128/255)、src线性.25、alpha=.25（即premultiplied F=1），DITHER在绘制前关闭，back128/front172；接受此单分支ANGLE观察。default dither查询为1、关闭后查询为0，不代表分别渲染了默认/关闭两支；不能把状态查询当成配对图像证据。scissor源码显式关闭；JSON常量标志不是独立状态查询。

F段正确常量值/bit已生成，但未执行六常量×各target×alpha×dither完整GPU矩阵；floatStage=NOT_RUN不作缺失占位PASS。剩余矩阵是明确未执行的测试覆盖，登记NOT_RUN/NOT_IMPLEMENTED，不据此误派产品缺陷返修；后续确需此覆盖，由manager据既有目标安排tester定点扩展，不新增manager验收门。

## 4. 交接与完成范围

本轮T121已完成**增量核对和分流**：已交证据解除旧前置，剩余C01链路/fixture摘要为窄范围工程修订，矩阵未执行单列；不把整个V4继续挂“等raw/SHA”。本结论不能替代T105条带实施、完整color matrix、数值或产品真机通过。manager据此安排后续任务，不由本执行会话修改全清单或建卡。

下游prompt：developer不使用真机，仅修独立测试探针及driver/manifest，先按本文工程合同，不改生产MaterialPass/数值阈值。tester不使用真机，沿原ANGLE离线环境只采指定缺项；未来另有产品设备卡须打真机并串行，不能用ANGLE替代真机结论；失败回architect。所有渲染含背景前景只走新XComponent+shader，禁止Canvas上屏/失败回退；失败FAULT或明确STALE的上一有效新管线帧。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅自己行状态有限例外），不改滴答同名清单、不回退共享修改。
