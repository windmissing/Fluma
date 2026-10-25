# T53 G2 上传器客观用例登记与冻结 v1

日期：2026-10-25。责任：tester。任务：[6addcb9d…99981](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6addcb9de4b06ee357299981)。状态：**TEST_REGISTRATION_ONLY / NOT_IMPLEMENTED / NOT_CALIBRATED / NOT_VALIDATED / G2_NOT_UNFROZEN**。

本稿在 G2 上传器开发**之前**登记并冻结客观用例，为 G2（上传器/bridge）提供可执行判据；不实现任何产品代码、不修改任何冻结口径、不新冻数值容差、不宣布 G1/G2 通过（宣布权在 manager）。G2 实施卡（T54）由 manager 另行派工，本稿不创建 G2 任务。不使用真机；全部判定为离线契约核对与符号面探测。

## 0. 依据与身份

| 项 | 值 |
|---|---|
| 门序裁决 | `docs/decision-P013-T45-material-rollout-v1.md` §2「T：G2 测试先行」/§3 Stage·Pump·Activate 与回退规则/§4 风险与反例 |
| 工程契约 | `docs/architecture-P013-p1-material-composite-v1.md` rev2（§4.3 六图与 metadata、§4.4 七版本、§6 napi/API/双代/原子配置、§7 时序/错误码、§8 遥测、§9 验证矩阵） |
| T33 工程定案 | rev2 §6 接口清单（`stageMaterialBundle`/`pumpMaterialUploads`/`canActivate`/`activateMaterial`/`discardMaterialStage`） |
| 准备模块 | `docs/developer-T24-material-preparation.md`（layout2、CPU 参考、测试夹具；T24 自检 11/11） |
| 上游 fixture | `docs/p013-p1-cpu-fixture-and-coupling-test-registration-v1.md`（T09 九个 CPU fixture + 五量用例；**一条不得删改**） |
| 数学参考 | `docs/reference-P013-p1-material-recipe-v1.json`（SHA-256 `d214aa447f9f186eba159fd6953f2b42874420926a1b01b7c0977b940f1d63a6`，本稿运行期复核 PASS） |
| 执行证据 | `test/reports/p053_g2_uploader_case_check.json`（执行器 `test/p053_g2_uploader_case_check.py`；23 例全部 NOT_IMPLEMENTED；保全 0 违例；追溯 0 缺口） |
| 错误码枚举 | rev2 §7：400 `MATERIAL_PACKET_INVALID` / 401 `ASSET_NOT_READY` / 402 `ASSET_REVISION_MISMATCH` / 403 `COLORSPACE_UNSUPPORTED` / 404 `COMPOSITION_ORDER_BLOCKED`；GL/EGL/shader 继续 300/301/302/200 系列 |
| 版本身份 | 七项：`materialRecipeVersion=P1_ALBEDO_DELTA_V1`、`slopeKernelVersion=E2_GAUSS_COL6_R18_P1_V2`、`colorPipelineVersion=LIN_SRGB_REINHARD_V1`、`textureSamplingVersion=P1_TEXEL_CANVAS_PX_V1`、`strataSamplingVersion=CONTINUOUS_COLUMN_WARP_V1`、`T_dataLayoutVersion=2`、`zOriginVersion=1` |

**期望状态码的指派口径**：每个用例的期望状态码从 rev2 §7 已冻结枚举中按语义指派（包级校验失败→400；修订/身份类中的配置修订→402；资产未就绪→401），不新建错误码、不新建数值容差。实现若使用其它既有码，该用例按 FAIL 报告并附实际码，交 manager/architect 裁定编号语义，tester 不自行改判据。

## 1. 登记范围与 T09 保全声明

- **保全**：T09 的九个 CPU fixture（TC-P013-FIX-01..09）与五个材质耦合用例（TC-P013-MCP-01..05）、参考 JSON、T09 执行器与登记稿**一律不改**；本稿用例为独立追加条目（TC-P013-G2-*），与 T09 条目无ID重叠、无判据互引。
- **保全核验**（执行器每次运行复核）：参考 JSON 哈希等于冻结值；T09 执行器与登记稿哈希按首跑值登记；`docs/test-cases.md` §16 十四个用例 ID 全部在册。任何变化报 `PRESERVATION_VIOLATION` 并使执行器退出非零。
- **追溯核验**：每个用例依据条款中的关键 token（65536/983040/786432/196608/1024/256、七版本字符串、400–404 错误码、stage 状态枚举、metadata 字段表、五接口名、byteOffset/SHA256/alpha）在执行器中被断言仍逐字存在于被引文档；token 消失报 `TRACEABILITY_GAP`（说明契约漂移，不是用例失败）。
- **不重定**：本稿不重定算法、纹理尺寸、物理路线或判线；不新建数值容差线；不重定义 T09 fixture；不改实现代码。

## 2. Stage 校验类用例（TC-P013-G2-A01..A10）

T45 §2 第 2 条枚举的十类失败各至少一条。共同期望：receipt.state=FAILED、当前 active 代与其配置零改动、pending 槽或为空或保留该失败条目；失败定位信息（errorStage）可指向具体校验环节。

| 用例 | 失败类 | 期望（receipt.state / errorCode / errorStage） | 依据 |
|---|---|---|---|
| TC-P013-G2-A01 | 缺字段（metadata 必填字段缺失） | FAILED / 400 / metadata | T45 §3.1；rev2 §6 |
| TC-P013-G2-A02 | 未知版本（七版本之一取值不在登记表） | FAILED / 400 / version | rev2 §4.4/§6「未知版本拒绝，不推断缺省值」 |
| TC-P013-G2-A03 | 七版本不齐 | FAILED / 400 / version | rev2 §4.4 七版本身份登记表 |
| TC-P013-G2-A04 | 配置修订不符（appearanceConfigRevision） | FAILED / 402 / appearanceConfigRevision | T45 §2；rev2 §6/§7 |
| TC-P013-G2-A05 | 身份不符（materialBundleId/assetGeneration 冲突） | FAILED / 400 / identity | T45 §2；rev2 §4.3「不以文件名当 hash」 |
| TC-P013-G2-A06 | 精确字节数不符（≠786432/196608/总≠983040） | FAILED / 400 / byteLength | T45 §2；rev2 §4.3/§6 |
| TC-P013-G2-A07 | byteOffset/byteLength 越界或元素类型不匹配 | FAILED / 400 / byteOffset | T45 §2；rev2 §6 typedarray 规则 |
| TC-P013-G2-A08 | SHA256 不符 | FAILED / 400 / hash | T45 §2/§3.1 |
| TC-P013-G2-A09 | alpha 非法（任一 ≠255） | FAILED / 400 / alpha | T45 §2；rev2 §4.3 alpha255 无预乘 |
| TC-P013-G2-A10 | context epoch 不匹配 | FAILED / 400 / contextEpoch | T45 §2；rev2 §6 同 epoch 规则 |

## 3. 双代与生命周期类用例（TC-P013-G2-B01..B04）

| 用例 | 场景 | 期望 | 依据 |
|---|---|---|---|
| TC-P013-G2-B01 | 新 stage 覆盖已存在 pending | 被取代代 receipt.state=DISCARDED；activeGeneration 不变；被取代代资源释放；active 仍可画 | T45 §2/§3.1；rev2 §6「覆盖 pending 先显式 DISCARDED」 |
| TC-P013-G2-B02 | 已激活代存在时执行一次失败 stage | activeGeneration 不变；失败后 active 代仍可绘制（draw 成功）；pending 槽为空或保留失败条目 | T45 §2/§3.4 |
| TC-P013-G2-B03 | 双代上限不变量 | 最多一代 active + 一代 pending；遥测 materialTextureLiveCount ≤ 4（两包×两对象）；无每帧 glGen/compile | T45 §3.1；rev2 §6 |
| TC-P013-G2-B04 | 上下文销毁重建（contextEpoch 递增） | CPU 缓存保留（assetCpuBytes 不归零）；重传后确认 READY 才切回；旧 epoch 帧被拒绝；READY 前退 Canvas 旧路径 | T45 §2/§3.4；rev2 §6/§7 |

## 4. Pump 预算类用例（TC-P013-G2-C01..C04）

| 用例 | 场景 | 期望 | 依据 |
|---|---|---|---|
| TC-P013-G2-C01 | 常规 pump（byteBudget=65536） | 单次 uploadedBytes ≤ 65536；只提交完整行（grain 行 1024B / strata 行 256B 的整数倍）；不允许半行或跨 tick 截断；上传期间 active 代继续绘制 | T45 §2/§3.2；rev2 §6 |
| TC-P013-G2-C02 | byteBudget 域校验 | 接受域=正整数且 ≤65536；0/负/小数/65537 拒绝：uploadedBytes=0、pending 状态不损、后续合法 pump 可继续 | rev2 §6「byteBudget 须正整数且 ≤65536」 |
| TC-P013-G2-C03 | 分块进度 | 未传完时 state=UPLOADING；全部资源与 metadata 通过后才 READY | T45 §2/§3.2 |
| TC-P013-G2-C04 | pump 时 surface/context epoch 已变 | pending 废弃（state=DISCARDED）并等待重新 stage；不对失配上下文产生上传 | T45 §2/§3.2 |

## 5. 激活原子性反例（TC-P013-G2-D01..D03）

| 用例 | 反例场景 | 期望 | 依据 |
|---|---|---|---|
| TC-P013-G2-D01 | grain 新代已上传、strata/palette 未就绪即请求激活 | canActivate 返回 false；activate 被拒绝（401 ASSET_NOT_READY）；active 代不变；不产生混代帧 | T45 §2/§4 部分代激活反例；rev2 §6 |
| TC-P013-G2-D02 | 正常激活 | 一帧边界原子切换纹理/palette/uniform/fog/配置 revision 并绑同一 FrameToken；旧 active 保留至在途旧帧结束后回收；帧只能引用单一 generation | T45 §2/§3.3；rev2 §6/§7 |
| TC-P013-G2-D03 | 上传/校验中途失败（GL 300 系列附 stage） | 旧 active 与旧配置保持；失败代不得被任何帧引用；pending 转 FAILED/DISCARDED；不得用已覆写 bitmap 引用假称旧代仍在 | T45 §2/§3.4；rev2 §7 |

## 6. 回退反例（TC-P013-G2-E01..E02）

| 用例 | 反例场景 | 期望 | 依据 |
|---|---|---|---|
| TC-P013-G2-E01 | draw 失败 | 同 tick 完整重画 Canvas 旧路径（drawLegacyFull）并**关闭整块 GLES**；只隐藏一层（造成 Canvas/GLES 重叠或空画面）判 FAIL；fallbackReason 遥测记录 | T45 §2/§4 空窗双绘反例；rev2 §7 |
| TC-P013-G2-E02 | profile/swap 失败 | 同 tick 整块回退；不得假设透明 clear 成功、不得等待失败 swap 再「透明清屏」；不留空帧/双层残影；fallbackReason 遥测记录 | T45 §2/§4；rev2 §7 |

E 类期望在控制器接线（G4 页面前）由桥接层失败语义 + 控制器回退路径共同满足；本稿冻结期望值，不计入 G2 上传器实现面（执行器按符号面分别标注缺失项）。

## 7. RED 基线与实现面缺失清单

**RED 结论：G2 上传器实现面在当前工作树完全缺失，23 个登记用例全部 NOT_IMPLEMENTED。** 执行器符号面探测（cpp/ets 全树 + CMake 文本）确认以下 25 个必需符号/文件一个都不存在：

- 文件：`entry/src/main/cpp/p013renderer/material_uploader.{h,cpp}`（rev2 §3/T45 §2 落点）——缺失；
- bridge 符号：`stageMaterialBundle`、`pumpMaterialUploads`、`canActivate`、`activateMaterial`、`discardMaterialStage`、`drawMaterialFrame`——全部缺失；
- ETS 类型：`P013MaterialStageReceipt`、`P013MaterialBundleDesc`、`P013MaterialParams`、`P013MaterialFrameInput`、`P013MaterialFrameResult`、`P013MaterialController`——全部缺失；
- 版本/模式：`apiVersion=2`、`GLES_SAND_COMPOSITE_OFF/ON`——缺失；
- 错误码：`MATERIAL_PACKET_INVALID`(400)、`ASSET_NOT_READY`(401)、`ASSET_REVISION_MISMATCH`(402)、`COLORSPACE_UNSUPPORTED`(403)、`COMPOSITION_ORDER_BLOCKED`(404)——全部缺失；
- 遥测：`materialUploadBytes`、`materialPumpCount`、`materialTextureLiveCount`、`fallbackReason`、`pendingGeneration/activeGeneration`——全部缺失；
- 回退路径：`drawLegacyFull`——缺失。

逐例缺失项（可定位到符号与条款）以执行器 JSON 的 `cases[].missingSurface/missingDetail` 为权威，本稿不重复抄录。**没有任何用例被报告 PASS/FAIL**：无实现面无执行，编造结果违反红线。

## 8. 可重复分析器与执行证据

- 执行器：`test/p053_g2_uploader_case_check.py`（纯 Python 标准库，只读；不写任何产品/测试判据文件，唯一输出为 `test/reports/p053_g2_uploader_case_check.json`）。
- 运行方式：`python test/p053_g2_uploader_case_check.py`；退出码 0=登记资产完好（RED 基线属预期），非 0=保全违例或追溯缺口（契约漂移，需先修文档再谈用例）。
- 输出结构：`preservation`（T09 资产哈希/在册核对）、`traceability`（条款 token 逐字核对）、`implementationSurface`（25 个符号面探测明细）、`cases`（23 例期望契约 + 状态 + 缺失清单 + 依据条款）、`summary`、`problems`。
- **复跑约束**：执行器不重跑 T09 CPU 执行器（避免与并行会话争用 `test/reports/p013_cpu_reference_execution.json`）；fixture 定义层保全以哈希登记 + 结构核对代替。G2 交付后，G2 用例运行器应直接消费本 JSON 的 `cases[].expectation`，并在同一通道复跑 T09 执行器前确认无并行写入。
- 每个数字可追溯：RED 计数（23/23）、缺失符号数（25）、保全违例数（0）、追溯缺口数（0）均可在 JSON 中按字段复核。

## 9. 不涉及项（本节条目一律不触碰）

- 不改算法、纹理尺寸、物理路线或判线；不改冻结阈值与七版本身份；不新建数值容差线；
- 不重定 T09 既有 fixture；五量用例（TC-P013-MCP-*）仍归其原登记稿，本稿只声明保全；
- 不改实现代码；不伪造运行结果；不宣称 VALIDATED；不宣布 G1/G2 通过；G1 是否解冻仍由 manager 按门序裁定；
- 不使用真机（真机取证归 G5/effector 串行队列）；
- P-011 全部协议/用例不在本节范围。

## 10. 交接与缺口

- **T54（G2 上传与接口）**：按 §2–§6 期望契约实现；交付时运行本执行器应保持退出码 0 且 `cases[].status` 由 NOT_IMPLEMENTED 转为可执行态；E 类回退路径随控制器接线补齐。
- **manager**：G2 实施授权与排期；A04/A05 期望码指派（400/402 语义边界）如需调整，须以修订版登记，不允许实现侧单方面改判。
- **architect/designer**：若实现语义与 rev2 §6/§7 枚举冲突（如身份不符归入 402），以缺陷形式回 architect，不在本稿口径内调和。
- **剩余缺口**：① byteBudget 拒绝时的 receipt.code 精确值（本稿只冻结「拒绝 + uploadedBytes=0 + pending 不损」，码值语义交上述裁定通道）；② E 类回退的遥测字段名以 rev2 §8 `fallbackReason` 为锚，G4 遥测落地后复核；③ GL 上传失败反例的 300 系列附 stage 细则随 native 实现面到位后由 G2 运行器补精确期望。

## 11. 变更记录

- 2026-10-25，v1：首次发布。登记 23 个 G2 上传器客观用例（Stage 校验 10 + 双代生命周期 4 + Pump 预算 4 + 激活原子性反例 3 + 回退反例 2），全部期望可追溯到 T45 裁决、rev2 契约与参考 JSON；T09 资产保全核验通过；RED 基线 23/23 NOT_IMPLEMENTED（25 个实现面符号全缺）。纯文书 + tester 只读执行器；未动冻结口径、未用真机、未宣布 G1/G2 通过。
