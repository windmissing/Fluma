# T45 裁决：P1 材质 G2/G3 实施门序与仿真线 GAP

日期：2026-10-25。任务：`6addc1e3e4b0e6ce82b10a78`。角色：designer。结论状态：**G0 五项探针通过；允许隔离推进 G2/G3 实施；不代表 P1 材质产品启用、效果通过或真机验收通过。** 不使用真机；未修改业务代码、测试、冻结判据或清单。

## 1. 依据与结论边界

- T42 回执 `docs/task-T42-g0-retest-receipt-v1.md`：VYG-AL00 的 s141 探针构建中，帧提交、sRGB、TEXTURE 路径 z 序、二值透明、显式输入透传五项 G0 均 PASS；T41 的 T_data 字节长度修复获设备验证。
- T24 `docs/developer-T24-material-preparation.md`：layout2 准备模块、CPU参考、测试夹具和离线资产装配脚手架已完成；T09 CPU 58/58 MATCH。六张生产材质图尚未从现 Canvas 生成器导出，GPU shader 函数尚未加入构建、编译或运行。故 T24 是准备证据，不是产品材质实现证据。
- P1 工程契约 `docs/architecture-P013-p1-material-composite-v1.md`：G2 为上传器/bridge 的版本校验、双代、限额pump与恢复；G3 为完整材质pass、OFF/ON及CPU/GPU一致性。
- T48 `6addc614e4b0e6ce82b14735` 已负责真实 Canvas 资产导出；T43 `6addc1e3e4b01278327e57bb` 已负责仿真设备采集链与入口装配。后续实施须复用现有卡，避免重复实现和文件冲突。

**裁决：**T42 解除了此前因 G0 探针失败造成的门禁。允许继续 G1 的离线准备/资产导出（T24准备模块与既有T48真实导出卡），并允许 G2/G3 进入分阶段隔离实施；不得将 G1 状态改写成六图已生产、GPU 已验证或 P1 已上线。G2 先交付可独立验证的上传与生命周期；G3 在 G2 接口冻结后实现完整材质pass。两者均保持非默认、不可由产品页面启用的隔离路径，直至 G4 集成与 G5 真机双测完成。

G0 的适用范围是 T42 指定设备、构建、探针布局及 TEXTURE profile。SURFACE 的单臂可见不推翻已选 TEXTURE 生产路径。TEXTURE 的 Default 行为与 T29 SURFACE 观察不同；生产配置继续显式使用 `hitTestBehavior.None`。T42 的 `requestComposition=PENDING`、长日志读回被截断、空 `probePoints` 三项仍须在后续证据链中披露；它们不否决 G2 开工，但数组级读回若作为材质颜色证据，需先补齐短字段/分片与探针坐标。

## 2. G2/G3/G4/G5 门序与实施任务书

以下为给 manager 的派工建议；本 designer 裁决不创建滴答任务。每张实施卡应写清角色标签、优先级、输入输出、依赖、文件边界，并写明“不使用真机”。

### T：G2 测试先行（tester，未建卡）

在开发前登记并冻结客观用例，保留现有 T09 58 个 CPU fixture 与所有冻结版本/阈值。覆盖：缺字段/未知版本、长度与hash错误、alpha错误、TypedArray byteOffset/byteLength、pending替换、active不受stage失败影响、≤65,536B/tick且仅完整行上传、上下文epoch不匹配、GL上传失败回滚、上下文重建后从CPU缓存恢复。输出测试用例与可重复分析器；不重定算法、纹理尺寸、物理路线或判线。

### G2：上传与接口（developer，未建卡；T48并行）

- 依赖：T42 G0 回执；T33 架构契约；T24 准备模块；T48 可并行提供真实六图导出契约。G2接口先冻结，资产到货前可用明确标注的合成测试字节做上传器测试，不能冒充生产资产。
- 文件所有权建议：新增 `entry/src/main/cpp/p013renderer/material_uploader.{h,cpp}`；新增 ETS 类型/controller（如 `P013MaterialTypes.ets`、`P013MaterialController.ets`）；在与 T43 完成冲突排查后，单独扩展 `napi_bridge.cpp` 与 `CMakeLists.txt`。不改 `SandCanvas.ets`、`EntryAbility.ets`、`WaterSandField.ets`、`P012WaterSandRenderer.ets`，不复制或改写 T48 生成器，不接产品页面。
- 接口按 T33：`stageMaterialBundle(desc, grainRgba, strataRgba)`、`pumpMaterialUploads(byteBudget)`、`canActivate(generation, contextEpoch)`、`activateMaterial(generation, frameToken)`、`discardMaterialStage(generation)`。Stage 校验七版本、配置修订、身份、精确字节数/范围、SHA256、alpha及context epoch；同步复制到自有CPU缓存；失败不动active。最多一代active与一代pending；替换pending须报告DISCARDED。Pump每tick不超过65,536B，只提交完整行；只有全部上传成功才READY。激活在单帧边界原子切换整代，不允许混合纹理、palette、uniform或fog revision；旧active待旧帧结束后释放。上传/校验失败保留旧active。
- 交付：G2客观用例通过记录、bridge/native API契约、generation/epoch/byte telemetry、上下文销毁重建的缓存恢复证明。编译通过只说明可构建，不说明GPU上传或颜色效果通过。

### G3：完整材质pass（developer，未建卡；串行接G2）

- 依赖：G2接口冻结且上传用例结果可消费；T48六图生产导出可提供真实字节；使用T24 layout2与版本身份。不修改已批准的物理/渲染算法语义。
- 文件所有权建议：新增独立 `material_pass.{h,cpp}`、`p1_material.vert`/`p1_material.frag`，复用T24 `p1_material_functions.glsl`；通过G2已约定的native模块扩展 `drawMaterialFrame`。与G2的 `napi_bridge.cpp`/CMake改动串行提交，避免同文件并行覆盖。不得改P0 `drawFrame` 语义，不接主页面，不替代Canvas。
- 客观证据：完整H/L/M沙区、B/C材质增量共用一次Lmat与ToneMap；P1 OFF 是完整材质pass且 `Lmat=1`；ON和B/C/spark消融；σ6列域坡度、纹理边界/mirror、空列、负/最大Q输入；CPU reference逐fixture对照、shader编译/实际GL执行、T42读回缺口补齐后的色块/像素读回。旧P0图不是P1 OFF基线，不要求逐位等价；按冻结的P1判据记录差异，不能改阈值迎合结果。
- G3通过后仍只代表隔离的材质pass和受测输入成立。不得宣称材质已标定、视觉效果通过、页面集成完成或长时性能合格。

### G4：页面共存与原子切换（developer/tester，未建卡）

依赖G2/G3交付，T43对 `SandCanvas`/入口改动完成并交接后再触碰相同页面文件。整合约束：同一物理快照只prepare一次；背景Canvas、完整GLES沙区、前景Canvas共享FrameToken；材质/调色板/uniform/fog配置同代原子生效。未READY或draw/profile/surface/swap失败时，当tick绘制完整旧Canvas并隐藏整块GLES，防止空白/双层残影；context/profile变化先从CPU缓存重传再恢复；不得改物理路由或提交语义。Tester覆盖旧Canvas回退、切换帧、pending替换和失效恢复。

### G5：真机双测（tester客观 + effector主观，未建卡；须标“真机”）

依赖G4在目标设备可达且T42/T44设备队列串行安排。设备卡必须打“真机”标签并明确设备前置、构建身份、串行排队。用真实T48生产资产与完整页面路径测P1 OFF/ON、B/C/spark消融、色块与A/B/C颜色证据、四个冻结黄金场景、shader/CPU差异、前后台/context恢复、内存、CPU/GPU耗时、swap和长时稳定性；按现有判据逐项报告 PASS/FAIL/未测，不新造阈值。Tester给出可复核数据，effector给出效果判断；二者报告完成即可登记完成，不增加manager验收门。

## 3. Stage/Pump/Activate与回退规则

1. **Stage**：验证descriptor身份、版本、精确长度、byteOffset范围、SHA256及alpha后，将资产和同代配置复制到唯一pending槽。Stage失败不得触碰当前active。pending被新代取代时发明确丢弃回执。
2. **Pump**：在GL owner线程按帧预算分块，硬上限65,536B/tick，按完整行上传；active generation继续绘制。只有全部资源和metadata通过后才进入READY；目标surface/context epoch失配则废弃pending并等待重新stage。
3. **Activate**：仅在 `canActivate` 成功后，于一帧边界同时切换纹理、palette、uniform、fog与配置revision，并绑定同一FrameToken。保留旧代直到在途旧帧结束，之后回收。禁止按资源逐张激活导致跨代画面。
4. **失败与恢复**：Stage/上传失败保持旧代；draw/profile/swap失败时同tick回到完整Canvas旧路径并关闭整块GLES；context丢失时保留CPU资产并重新上传，确认READY后再切回。回退只影响绘制，不改physics、采样身份或事务提交。

## 4. 风险与反例

- **G0探针不等于完整产品 pass**：T42只验证基本帧、色彩、层序、透明和输入，不包含六图、P1 shader、跨Canvas动态同帧或P1色彩公式。
- **T24 synthetic fixture 不等于生产资产**：若把恒色/合成纹理通过上传器的结果写成“真实材质通过”，会形成假阳性。真实图必须来自T48同一Canvas生成器 putImageData 前的字节。
- **部分代激活反例**：grain新代已上传但strata或palette未就绪就切换，会出现跨版本画面；必须原子激活并保留旧active。
- **空窗/双绘反例**：swap失败后仍假设透明clear成功，或仅隐藏一层，会造成空画面或Canvas/GLES重叠；必须整块回退。
- **采样边界风险**：连续列warp后再离散C纹理取样可能产生相位台阶；P1列域σ6与P0连续差分不等价；按架构列为专项观测，不得偷偷改成模糊或改版本。
- **资源/帧预算风险**：每代六图GPU约983,040B；active+pending约1.88MiB GPU，CPU镜像另有相同量级，尚不含T_data、Canvas前后层和驱动开销。65KB/tick是上限，不是帧时保证；需记录实际上传耗时、峰值内存与p50/p95。
- **屏幕颜色误读风险**：T42 request时点收据仍PENDING，日志大数组有截断，探针点为空；若只凭请求回执或一张截图宣称A/B离屏数组齐全，证据不成立。

## 5. 仿真线当前最大 GAP 评估

这里按全局仿真目标 `docs/goals.md` / `docs/PROJECT_GOALS.md` 评估，而非按当前 T49/T51“高度场 + 粒子”DEMO目标评估。当前优先目标是 **G2+G3**：水中重沙/轻沙三相堆积，以及两场景共享的旋转、倒立、摇晃动态；G1+G3 是另行开题的油水双相支线。

**全局最大 GAP：G2 的目标能力仍与当前获准实施范围不一致——全局目标要求水、重沙、轻沙三相和可见分选，但 2026-10-23 的范围裁决当前只推进“水 + 一种沙”，双沙/三相分选仍为 DEFERRED。** 因此最核心的“重沙先沉并集中于下部/内部、轻沙后沉并覆盖上部/外围；倒置后相序反转”尚未进入当前阶段的实现和验收闭环。即使单沙沉降、堆积或当前高度场/粒子DEMO成功，也不能据此宣称G2达成。这是目标覆盖缺口，优先级高于采集链是否已能产出完整run。

全局差距按目标距离排序如下：

1. **G2核心物态与分选未覆盖（最大）。** 完整目标是水+重沙+轻沙三相，各相漂移 <0.1%，沙堆上/下1/3重沙占比比值≤0.5，摇晃后≤8秒恢复稳定，并能在倒置时完成动态分选反转。现行阶段裁决暂缓第二种沙；不能用单沙水沙模型、粒子沉积演示或局部`P=0`的守恒证据代替三相分选。
2. **G2+G3动态能力尚未形成完整通过证据。** 全局要求静态倾角、±89°连续变化、经审计`invert180()`跨越、倒置/翻回相序、快速旋转、摇晃悬浮与静置再沉降都维持守恒与稳定。现有目标用例仍注明连续重力扫描/倒置交换完成度/事件语义/长时组合矩阵待补；`goals.md`记录G2为NOT_STARTED、G3为NOT_VERIFIED，但该状态栏上次更新于10月18日，早于本周后续实现和取证，只能说明缺少最新的目标级判定，不能解读为代码完全不存在。当前某个静态或单路径设备run不能外推到这组动态行为。
3. **目标级客观与主观闭环不足。** 全局门槛要求可执行客观用例和主观效果用例同时成立。`docs/gap.md`仍把P-003/P-005列为单沙+水进行中、三相延期；P-004全场景回归仍OPEN。P-003/P-005的报告文件包含较早的RED/NOT_IMPLEMENTED基线，不能当作当前构建的最终失败证明，也不能代替新的合格双测；最新目标级报告状态应由tester/effector按已交付代码刷新。
4. **跨场景完整性仍有空白。** G1油水不互溶场景是独立支线，目标仍NOT_STARTED；它包含双相守恒、翻转2–4秒交换和≤4秒水平恢复。由于当前优先级明确G2+G3在前，G1是全局产品目标差距，但不是当前主线首要缺口。

T43/T44所处理的endpoint/peak、原始账本、停摇通道与真实粒子生命周期，是G2+G3当前一条高度场/粒子产品路径的**重要验证与接线子缺口**，不是整个仿真目标最大的GAP。其证据仍值得完成：T11记录sampler未装配、store worker未入包、运行时无法停摇、粒子证据为`STRUCTURAL_ZERO_OFF`；T43修复接线，T44再验证该路径。T46的planner code5同样是更局部的单点行为，需要继续按T47补证。它们都不能替代“当前范围何时恢复双沙三相目标、如何验证完整G2+G3”的全局裁决。

**结论与推进建议：**保持G2+G3为主线。当前先完成获准的水+单沙基线和共用动态能力验证，同时明确该阶段仅是G2子集；另把双沙/三相分选作为完整G2不可省略的后续阶段，启动条件与测量判据需由manager按目标/范围流程明确，不能因DEMO完成自动解锁或删除。补齐G2动态用例和全矩阵回归，与功能推进并行准备；按用户既定一级体验顺序先验证倒置响应与相序交换，再推进摇晃再沉淀。完成对应产品路径后由tester/effector形成最新客观、主观报告，不能用T42渲染G0、P011单项设备指标或遥测链路修复代替目标通过。

## 6. 本轮登记

2026-10-25；变更原因：T42已报告G0五项PASS，触发T45裁决；同步评估仿真线最大证据缺口。核验范围：T42、T24、P1 rev2架构、T11设备回执及现有T43/T44/T48任务依赖。结论只基于上述文档与任务，不外推为未检查任务已完成。T45交付后关闭；后续工作清单及其滴答同名页由manager按项目约定另行刷新，本设计稿不代改清单。
