# P013 色彩工程归因与责任修订 v1（T84）

日期：2026-10-26。architect。原卡 `6adef4bee4b0127832ada18c`。版本 `P013_COLOR_ROUTING_ENGINEERING_V1`。不使用真机；只读既有证据，不运行GPU测试。

## 1. 当前结论

消费 `docs/analysis-T77-material-color-v1.md`及其evidence JSON、`docs/developer-T55-material-pass.md`、T80原卡 `6adef380e4b017ab33fdad3b` 的§6.1–6.3补证范围、T68新管线合同。T77历史设计归因保留，最新测试失败先回architect分流，不重开已完成T55/T77。

| 现象 | 已有证据 | 分类/当前动作 |
|---|---|---|
| default-pbuffer最大73.7203404码 | OFF无增量点CPU145/default72具有线性码特征；default查询SRGB | **工程目标链候选＋测试采集候选，WRITE_VS_READ_UNRESOLVED**。不能认定唯一根因是漏写转换或driver bug；T80原矩阵继续。 |
| SRGB8 FBO最大1.9562653码 | (88,78) blue：CPU155.0437/GPU157；gain×150的F64/F32取整存在172/173候选分叉 | **数值表示设计候选**，尚无GPU baseByte直证；T80取中间量，未确认前不转实施。 |
| 最新bridge程序未能执行 | T55记录OS环境阻断，材质真实GPU测试另已运行 | 环境/桥证据归T76/T81，不与73码色彩差异合并。 |
| 完整scene/真实window/最终屏幕颜色缺证 | T55隔离MaterialPass不是T56完整scene交易 | 尚未实现/未验证的预测试按实际标NOT_IMPLEMENTED或UNPROVEN；不从缺功能预测试派颜色缺陷修复。 |

structural PASS不等于numeric PASS；两target仍MEASURED_NOT_CALIBRATED。1.956与73码都不是新容差。T84完成首次工程分流，数值/设备呈现尚未通过。

## 2. 已确定的工程职责修订

本文件作为 `architecture-P013-new-pipeline-only-revision-v1.md` 与 `architecture-P013-p1-g0-composition-revision-v1.md` 的色彩责任补充，不改P1签收数学/七版本。相对T55仅查询attachment及两run覆盖同目录的证据不足，明确以下落点；这是可实施的工程合同，**不代表已证明产品违反这些条款**。

| 文件/hunk所有权 | 接口/时序合同 | 失败处置 |
|---|---|---|
| `cpp/p013renderer/egl_context.cpp` surface创建/恢复（developer依manager派工） | 请求与实际EGL颜色空间、config通道及当前context/surface身份分别记录；扩展精确token匹配，仅支持时处理write-control；epoch改变重新建立状态 | API错误/能力未成立明确FAULT；不盲enable无支持token，不以shader gamma补。 |
| T56 `fullframe_targets.*` / `scene_gpu.*` | 创建candidate/LastGood时实际format/encoding/completeness与尺寸、epochs绑定；target切换显式建立draw/read绑定和viewport、blend等所有权，记录支持的write-control状态 | 创建失败不替换active/LastGood；context丢失旧GPU句柄失效；不降级RGBA8再冒称sRGB。 |
| T56 `scene_compositor.*` / `scene_layers.*` / present shader | material输出显示线性预乘；SRGB8_ALPHA8写时硬件编码一次；fullframe采样硬件解码一次，present输出linear，window写时硬件编码一次。与材质RGBA8字节数组手动EOTF严格区分 | 禁止再对fullframe手动EOTF/OETF、禁未定义blit替代；未证能力不能报颜色通过。 |
| `render/P013SceneTypes.ets`/SceneController及native scene receipt | 复用T68 requested/rendered/lastGood身份；新增若必要的色彩能力/诊断对象须单独revision，包含target/stage/encoding实际来源、context/surface epoch | 查不到值记UNKNOWN/UNPROVEN，不把请求值写成实际值；色彩能力未证与真实draw错误分列。 |
| tester `test/p013_native/material_gpu_test.cpp`及`test/run_p055_material_gpu.ps1`或独立T80执行器 | 归tester所有，生产路径不改。每target/run独立raw/hash/manifest、显式read/draw/stage与坐标/行方向；原T55JSON/raw不可覆盖 | 缺raw保留MISSING；测试采集错误修执行器，留旧报告及版本，不改生产数学。 |

T56可继续结构集成、shader背景前景、同token同代、lease与LastGood，不等T80；上述target职责已由T68给出，本稿只细化证据/颜色责任，不增加批准门。T84不越权实现或改T56共享文件。若生产有实际错误，再由architect按证据写具体最小patch范围，manager派developer。

## 3. T80继续既定补证，不重复建卡

严格沿T77§6.1–6.3：同上下文四目标（RGBA8 FBO、SRGB8_ALPHA8 FBO、LINEAR和SRGB default pbuffer）常量fragment矩阵，查询EGL实际colorspace、read/draw buffer及绑定、actual attachment、write-control支持/状态、dither/blend/pack/samples/errors、hash和运行身份。clear只作对照，不替代fragment写转换；直接ReadPixels与texture采样解码不能混称同阶段。sRGB受控字节上传、线性shader写和sample-to-LINEAR目标三角对照用于区分写/读，默认存储若无独立路径则保留WRITE_VS_READ_UNRESOLVED。

取整证据锁定FBO反例(88,78)与邻域，保留真实Float32 T_data/资产hash、fH/gain、palette路径、grain/strata texel、baseByte、Rpre/Lmat/ToneMap，分列F64主参考/F32操作顺序诊断/GPU实际中间量/最终sRGB。debug shader独立hash，整数索引/字节精确编码，连续量用实际支持且complete的float目标；不支持就报告未证，不能用8bit输出倒推高精度相等。dither生产默认与明确disabled对照分开归档。

不要求T80追加全平台driver调查或设备性能；不可得backend只报缺口，不绕过系统保护。T77已说明两target共用raw目录导致default图被覆盖：不能拿现存FBO raw补成历史default全图。T80必须独立保存，不修改历史最坏点。

## 4. 补证回architect后的分流与最小hunk

| 补证结果 | 责任与动作 |
|---|---|
| 实际read/draw绑定、attachment或报表目标标签错误 | 测试采集问题→tester修该执行器/manifest，旧结果保持。若独立证明产品同错，architect修订target工程hunk后developer修复。 |
| 有支持的write-control在生产实际错误关闭，或颜色空间创建状态错误 | 工程问题→egl_context/target bind状态建立及回执的最小修订，恢复时同样建立；不动p1数学helper。 |
| default查询SRGB但常量功能仍不符，显式FBO正常，写/读无法分开 | backend/目标链未决→记录能力未通过与具体最小反例，保留未知；不宣称ANGLE唯一故障或全平台加gamma。 |
| GPU baseByte/texel已与冻结参考不同 | 先排除传入值/索引/资产/坐标错误。若工程输入偏差，architect给preparation/uniform/采样hunk；若相同输入的精度/表示/离散取整分叉，转designer制定局部表示revision与证明。 |
| base相同，差异始于Q/pow/Lmat/ToneMap | 对照实际生产shader与冻结操作顺序/公式；实现不符走工程修订→developer；公式/表示需变交designer，不以“浮点误差”概括。 |
| fullframe到present出现额外转换 | 工程问题→fullframe texture格式/采样/present shader的单一转换职责，按§2修订，不在material shader反向抵消。 |
| B数值正确、C截图不同 | 先核同帧/色彩域/PNG元数据、系统合成环境；证据回architect。不能用截图偏差逆调配方，也不能把截图当面板校准。 |
| 指标/比较域或用例本身有争议 | 带旧合同与可复现反例交manager裁决，未经裁定不换主参考、不放宽阈值。 |

确定设计分叉后给designer的任务书：输入相同实际Float32 profile/资产和GPU中间量；明确数学半整数round与binary32边界的约束，比较确定字节准备/明确GPU表示等方案，考虑fH为每fragment变化，不能仅上传列中心baseByte代替整像素；定义接口/版本/误差证明与冻结参考关系。禁止epsilon、改gain/palette/配方、纹理模糊或悄悄把F32影子变主参考。设计交付再回architect落工程接口，不由本稿预选算法。

## 5. 修复及独立回归边界

若确定工程修复：保留T55两目标原数值，新增同fixture/同hash前后证据；OFF/ON、B/C/spark、空沙/沙外RGBA0、镜像、Q0/255和相位/texel边界仍测。全scene另测fractional-alpha雾/高光/边框的线性预乘over、candidate→present恒等色块、context/resize/LastGood恢复；不能从二值材质alpha外推半透明合成。

设备T57继续独立A实际属性/B同帧数值/C最终截图：记录实际window与fullframe编码、source/frameToken/generation/epochs/shader资产hash和sample stage。same frame关联不足明确UNPROVEN；swap回执不是displayPresentVerified。开发交付仅开发/编译/离线自测，设备单独管理，不加manager验收门。

所有渲染只走XComponent+shader，包括背景/前景；禁止Canvas上屏或失败回退。失败FAULT或标明STALE的上一有效新管线帧；保留requested与rendered身份，不以旧画面冒充当前成功。旧Canvas图只读历史对照，不作为现场参考管线。

## 6. 交付状态

T84完成工程责任文档与工程/设计/采集/用例/预期未实现分流。现有材料不允许唯一底层根因裁决，待T80已有任务补证后回architect；T56结构继续。未改业务、shader、配方、测试阈值或原报告，未运行测试/使用真机。仅自己行结束待验收，原卡回报，manager负责登记与后续派工。
