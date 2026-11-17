# T535 / T543 architect交付回执

日期：2026-11-17。不使用真机；未运行产品测试、未实现业务代码、未新增CPU物理/oracle。两卡约定工程范围均已完成，运行功能仍由后续developer实现和独立角色验证。

T535消费T517 `7a02d77`原回执及T527合同，归因为architect遗漏摘要自引用规则。已修订原T527 MD/JSON为ENGINEERING_V2，新增 `architecture-T535-v5-prefix-digest-v2.md` 与 `architecture-T535-prefix-digest-manifest-v2.json`：唯一固定mask[1116,1148)、封存/发布次序、canonical/raw独立摘要、ROOT/FULL页身份及有界外部manifest、旧V5_1拒收均已冻结。四profile布局/预算保持原值。外部整文件hash只存在外部回执，避免第二次自引用；后续修改所有摘要合同都应先列摘要覆盖图，确认不存在未定义环。

T517可在原卡接续完整ROOT/FULL、五BASE与SOURCE_CONTEXT锚点、collector/reader/诊断adapter及跨route credit；其局部compile和capacity helper仍非完整capture。architect工程前置已交，不新增manager门；本轮不擅自改变T517状态/任务prompt。

T543消费T534已批准研究规格与T529 `e429ea7`源码，新增 `architecture-T543-g9-candidate-touch-adapter-v1.md` / `architecture-T543-g9-adapter-manifest-v1.json`。明确旧candidate1与BALANCED不同、现有bridge为空seed、现有seed SHA在填身份前计算，不能冒新候选/上传原件身份。已冻结Config64、673792B规范S40单origin模板与实际上传hash次序、one-UP整数配方、真实px/单调时钟proof及gesture/scene/config/token身份、GPU复算、INPUT128 V2未用word0、cold V2双原始证据和信用/dispatch成本。真实UP不会经ModelEnvelope改采样时间，也不自行rearm BLOCKED。模板/参数/adapter SHA和有限向量见机器表。

边界检查发现并明确：UP可有0个active touches；普通同geometry的present提交不应取消整次手势；UP接触x量化后落128Q的右缘小区间须明确拒绝；NaN/Inf、负0、次正规、RNE半偶、时长/250ms等号、2^53精度、mask污染及nonce重送规则有有限例。上游clock代码和SDK只作为工程依据，不能替代实际部署平台clock proof或设备证据。

运行 `python docs/analysis-T535-T543-contract-checks-v1.py`：T535 49项、T543 113项有限工程检查通过。它读取正式设计与源码表，核四profile几何/原预算、canonical prefix及坏字节/旧版拒收、全部T534手势预期数值、输入字段/零位/正文去重、时钟/像素/手势准入有限真值表。结果为PASS_FINITE_CONTRACT_ONLY，非产品/reader/设备/物理/效果PASS；没有重跑tester报告。脚本只生成本次T535/T543工程表/证据，不能用旧V1共享生成器覆盖修订T527。

交manager：可直接登记本两卡交付。T543后的真实adapter独立开发可排，T544仍需实现及正确签包，T541/T542原取证不因此改判；V5仍等T517实现，互不串前置。developer职责严格开发/编译/提交、不执行测试、不使用真机；所有新渲染仅XComponent+shader，故障FAULT或标明STALE上一有效完整帧。root250ms/GAP33333333ns/配额120Hz/显示30fps、研究默认OFF、CPU停投和既有产品门槛均保持。
