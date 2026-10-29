# T268 明水可见性与设备 ACK 缺证分流 · 工程修订 v1

2026-10-29，architect；原卡 `6ae2f99ae4b0ab67703581ef`。不使用真机。消费 `test-report-T262-c-long-identity-retest-v1.md`、T262 原卡评论、T226/T258 交付及当前 C V3 源码；T244 仅作离线边界。此稿补充 `architecture-T219-clear-water-C-v1.md` §3/§4 和 `architecture-T222-clear-water-flight-v2.md` §6.1，不改变 V1 源/物理/光学公式、通过阈值或 endpoint 协议。新增水效果仍按 GPU 驻留 M1 路线 T240 优先；本稿只处理现有目标7 C V1 的证据链，不把目标8 Flight V2 当已实现。

## 1. 可确认事实与不可跨越的结论

T262 使用固定 T258 源码重建的 T261 签包（SHA256 `419f36459af356276e3485b712d0b1611a57b72b738c1c7b29d414a52040eaf9`），真实 `pages/ClearWaterDemo` XComponent 页面持续 `CURRENT`，T252 的每帧 `CW_V3_PARSE`/code100 未复现；沙注入后的沙色像素与点位对应，背景色在屏幕。T262 `t262k.jpeg` 为切水后的深色画布，未见独立水色像素。报告只给水按钮 UI 状态变更、没有逐笔水请求/终态 receipt、同 token 的 W/profile/native/逐层像素记录。T244 离线 A-01 的 `ACCEPTED/admitted/transactionId/kindSuccessSeq` 是**沙**请求，不能转用为本次设备水请求 ACK。

源码确认 `ClearWaterSourceController.select(1)` 只 `stop(KIND_SWITCH)` 并改 kind；必须再有新 DOWN。`down()` 会重置源时钟，T244 已观察到两个有效 tick 后才有采样窗口。`ClearWaterDemo.kind` 和 `CURRENT` 分别是 UI 选择与绘制回执，不等于水源提交。`sample()` 内存中会查 terminal receipt 并更新 `feedbackCount`，但当前页面/设备证据没有导出该 receipt 的身份、`disposition/admitted/transactionId/kindSuccessSeq` 或旧源终态。`ledger64[15..20]` 只是最后终态摘要，不含完整请求键、kind、disposition 和 admitted，也不能代替精确 reader。

水源成功时 `P011ClearWaterSource.commitNext()` 将正量写入当前列 `W[c]`，增加 `I_water/waterSuccessCount/sourceCommit/physicsCommit`，终态带 `globalWaterBefore/After`、`columnWaterBefore/After` 和 `actualWaterDelta`。`P011ClearWaterEngine.copyCurrentSceneInto()` 把 canonical `W` 放到 `profile64[8c+1]`，并按列折入 `ledger64[11]`；`ClearWaterSceneSlot.verify()` 可在源侧核 profile/ledger。native C V3 校验和上传后的 `ClearWaterGlGpu::Layer(3/4)` 才分别做水体透射/水面高光；完整六层 GL 调用、Present/Swap 返回成功只证明命令链未报错，不证明每层各自产生非背景像素或设备屏显逐层同 token。

当前冷配置 `rateWater=1`、`sigmaWater=0.025`、`surfaceGain=0.35`、`PENDING_CALIBRATION`；shader 在 `W>0` 且像素落入水深时才有 `1-exp(-sigma*path)` 水体 alpha，水面按真实湿带与像素相交面积计算。即使有正 W，薄水/低覆盖也可能在截屏色深、缩放和压缩下不可辨；**页面文字“参数待定标”只是状态说明，不是根因证据**。不能据此直接调透明度、改物理水量或宣布 GPU 故障。

## 2. 归因分叉及当前状态

| 链路点 | 一锤定音的同 run / 同 token 证据 | 若成立的分流 |
| --- | --- | --- |
| 输入/源终态 | 新 DOWN 后有效采样窗口；逐笔 request key、kind、requested、enqueue 与 terminal query；`ACCEPTED` 或 `PARTIAL_CAPACITY` 且 `admitted>0`、非空 transactionId/kindSuccessSeq；旧沙请求终态保持旧 key，切源后旧代不能新增成功 | 没有正量终态：先查手势/时钟/拒绝码；UI 切换本身不算 ACK。若行为违反既有合同，是工程问题；若只是测试未生成有效窗口，是用例/采集问题，交 manager 裁决 |
| authority 库存 | 与正量水 receipt 对齐的 `sourceCommit`、`W[c]`/全局 W、`I_water` 增量；无中途 reset/新 run/失败后旧帧误配 | 正量已接受但 W 未增加或总账不符：源/状态工程缺陷，修正式工程稿后 developer；若正量 W 后被真实物理移动，需按连续 commit 追踪，不能只比一个列 |
| publication/包 | 同 owner/runGeneration/sourceCommit/publicationSerial/frameToken 的 canonical→slot→native profile W 与 ledger 总量；capture/validate 结果 | authority 正 W 而 slot/native 丢失或取了旧代：publication/桥接工程缺陷；版本/身份拒绝须保留 first stage，不能借 `CURRENT` 推断无拒绝 |
| 水体/水面绘制 | 同 frameToken、同尺寸/镜像/配置的实际 W/eta/S/optics，GL layer3/4 错误和各层前后同 ROI 像素差；与 `ClearWaterTransmissionAlpha`/`ClearWaterSurfaceAlpha` CPU oracle 对齐 | profile 正确但应有可分辨覆盖且绘制结果零差/错误：shader/混合/目标工程问题；公式正确但目标可见性不合体验要求：给 designer 设计，独立 effector 验证，不由 architect 盲调参数 |
| 屏幕采集 | 真实 swap 的 `CURRENT`、requested/rendered token、同 token 诊断 ROI 与定时截屏、背景对照、无 UI 遮挡/压缩误判 | 离屏层有差而屏幕无差：present/色彩空间/采集路径再定位；截图不与 token 配对时只记证据不足 |

截至 T262，前四个水专属结点均未获设备闭环证据；**水不可见的唯一根因未证**。保留 T262 对长身份拒绝未复现和沙粒实际像素的窄结论；撤销从背景+沙粒推得“六层逐层水像素 PASS”的扩大解释。T262 无新的产品 FAIL 归因，亦不能把缺证写成水效果 PASS。

## 3. 给 developer 的最小有界工程接点

先按本稿修订交接，再由 manager 派 developer 实现诊断，不把诊断缺口当新物理效果。现有 `P013ClearWaterController` 有 256×24 trace/32 counters，但没有逐请求字段；保留原 C V3 endpoint、Source row20/56/24 和 `copySourceReceiptInto` 语义。增加**诊断版、默认关闭**的固定环形 `CW_C_DIAG_V1`，条目上限和丢失计数冷配置，每条只抄预分配数值/有限枚举；热路径不 `JSON.stringify`、不打印整个 profile/token、不分配逐帧长字符串。冷导出时附包 SHA、runId、runGeneration、inputGeneration、geometryEpoch、owner、surface/context、frameToken 完整值与导出修订，导出截断必须报告而非默默覆盖关键记录。敏感 request key 原始位型需完整保留在诊断包内，不能只靠 hash；常态不将其逐帧打 hilog。

`P011ClearWaterSourceController` 在 select/stop/down、时钟首次/有效窗口、enqueue、commit、terminal reader 后记录：old/new kind、stop reason、gesture/source/requestSeq、clockWindowId/raw/unit、requested、enqueue status、batch processed、query 枚举、终态 disposition/status/admitted/rejected/transactionId/kindSuccessSeq/sourceCommit/physicsCommitId、global/column W before/after、errorStage；旧代请求只读 query 记录独立标识，不二次 enqueue 或改终态。`P013ClearWaterController` 每个**选定的**诊断 frame 记录 source/physics commit、publicationSerial、`ledger64[11]`、按列的 W 正量计数/总量及触点列 W/S/eta、capture/verify/bridge stage、完整 frameToken 与 native requested/rendered/lastGood token、六层/Present/Swap first failure stage。若选定帧需要完整 128 列 profile，应在冷导出时从已封存同 token slot 拷出，不能对新的 canonical 假称旧帧。

水专属图像分叉只允许 tester 指定的至多三帧诊断模式：在同一候选 FBO、同一 token 与 ROI 内，分别于粒子层后、水体层后、水面层后做**只读**像素采样/摘要，记录纹理格式、ROI、原线性或 sRGB 解释和采样时间；不得改目标内容、混合状态、渲染层序或提交判据。层间零差需与已封存 profile/optics 的理论覆盖共同判断。诊断开销单列计时，正常运行关闭。整帧仍只准 XComponent+shader（背景和前景也在此管线），失败 `FAULT` 或标明旧 token 的完整 `STALE`，绝不 Canvas 上屏/回退。

工程非真机自测至少覆盖：水正量与拒绝各一笔完整字段、同 key 重查零副作用、切源旧代终态和新 DOWN/两 tick、源成功后 step 失败仍可追 W、publication→native 同 token 水 profile、诊断环截断显式计数、诊断开关不改物理/帧返回、离屏层采样不改 Present/Swap。构建/自测通过仅表明可交设备，不是水可见或设备 ACK PASS。

## 4. 给 tester / effector 的定向补证与判线

tester 在独立真机窗口使用可复现固定签包/SHA、诊断 revision，先冷启动并标记 run；切水后重新 DOWN、保持至少两个有效 tick，记录每个请求至 terminal 的全字段和切源前旧代终态。注入点/目标列/有效时长必须记录，避免 UI 按钮 tap 充当源手势。取同一正量水 terminal 的 `sourceCommit`，沿连续帧定位 canonical W、slot/native W、每层 ROI 差及完整 token；用无水基线和真实沙注入位置作几何对照，设备截屏保存原始文件、屏幕时间和对应帧标识。若帧/截图无法严格一对一配对，只能给观测窗口与不确定性，不能写同 token 产品 PASS。设备 readback 如果做不到同 token，应明确 `NOT_MEASURED` 并列工程补口。

判线：设备 ACK 需逐笔 `TERMINAL` + 正 `admitted` + transactionId/kindSuccessSeq/sourceCommit，UI 文本和 `CURRENT` 不代替；水库存需正 W 与守恒 receipt/ledger 对齐；水像素需理论非零覆盖、同 token 层间或屏幕可辨差及独立采集证据。若库存与合法低 alpha 预测一致但主观水质感不足，交 designer 定标并交 effector 效果验证；若层间命令失败、正 W 包遗漏或理论可见区实际零差，带首错 stage 回 architect 做工程修订和 developer 返修；若测试动作没有有效源窗口/采集不能同 token，给 tester 精确补证，不把“参数待定标”当产品故障结论。T240 的 GPU M1 水效果独立推进，不因本卡新走 CPU 水效果。

## 5. T271 原件回传后分流 v2（2026-10-29）

`T270-v4-signed.hap` SHA `599d03e6fc98278a14a1c006187791d8ebd116070f0c134117c114d6b14d793f` 的第三轮 `test/reports/t271/t271_diag3.json` 内四笔 `TERMINAL_QUERY`（row153/158/209/214）均为 water kind、`ACCEPTED`、`admitted>0`、transactionId/kindSuccessSeq/sourceCommit 1→4，W 与 I_water 增量一致；同档 frame200–202 的 `profiles` W 总和、`ledger64[11]` 和 `nativeSamples.nativeWater` 均约0.066748733，完整 frameToken/slot verify/CURRENT 数值链有证。**逐笔正水 ACK 与同 token W→native 数值传递在这一 run 内成立**，原 T262 的“只是切换 UI”缺口已补。该档 `dropped=393/incomplete=true`，分片 END 逐字节齐只说明导出文件传输完整，不说明环内全历史事件无丢；已封存四笔及三帧仍可作窄结论。T271 报告首页 runId `cw-run-1793273652501-1-1` 与第三轮 rows/frameTokens 的 `cw-run-1793274115973-1-1` 不同，需 tester 修报告元数据，不跨 run 混证。旧代请求在 select 时 seq=0，`OLD_KEY_QUERY` 未造场景，继续 `NOT_RUN`，不能把 N/A 泛化为旧代隔离通过。

**水层像素归因的测试 ROI 不成立。**第三轮 frame200–202 `visualInverted=0`，surface/box 宽高为1178×2026、box 原点0；封存 profile 的实质量 W≈0.00005–0.00657 位于模型列78–102，列64的残量约 `2e-16`。native 固定 ROI `(x=64,y=200,w=4,h=4)` 的 x 对应模型列约6–7（`x/1178*128`），与列78–102 完全不相交；`glReadPixels` y=200（底部起）也远离未倒置时 S=0、W<0.007 的容器底部薄层。故三层该 ROI 同为背景只证明**无水位置的采样不变**，不能证明水体/水面 shader 零差，也不能把原因唯一归于 `sigmaWater=0.025`/8-bit 量化。报告用全局 W=0.0667 推单点 path≈0.033 亦与本帧每列最大 W≈0.00657 不同；正确体层在水内的最大 `1-exp(-0.025*0.00657)≈0.000164`，还须核实际像素中心/覆盖与 sRGB/预乘，不能直接拿总量替单列深度。JPEG 全屏“未见水”是观感线索，未与指定真实水像素和同 token 绑定，不能定位唯一根因。

这是**测试点位/判据问题**，交 manager 裁决 T271 水像素部分应重采，而不是先派 designer 调参或 developer 修 shader。现有 T270 诊断通过 `cwdiagX/cwdiagY/cwdiagFrame` 可指定最多三帧 ROI，无已证生产代码故障。tester 补证应从已封存同 token profile 中先选实量 W 列（例如 frame200 的列80–82），结合 `box/visualInverted` 和 `glReadPixels` 底部原点计算 ROI（该帧 x 约736–770px、底部 y≈0–3px；按实际目标/FBO再校），记录每个 ROI 映射的列、像素中心或面积覆盖、`S/W/eta`、单列 oracle alpha、optics16、层2/3/4 raw RGBA 和同 token 屏幕对应位置。至少给一笔**自然有效源窗口**产生可覆盖像素中心的水量作为层链正控，另保留原薄水两 tick 作为低量对照；不得偷偷改物理水量、sigma、shader 或用假水柱，且要记录两个真实 run/receipt。若在应覆盖且理论可分辨的同 token ROI 仍零差/报 GL 错，再回 architect 工程分流；若实现与公式吻合但体验仍不达标，再交 designer 定标并由 effector 验证。旧代隔离可另设切源前仍有待决沙请求的真场景定向补证，不与此次四笔正水 ACK 混为同一 PASS。

本轮 architect 不使用真机、不重跑测试；保留 T271 的 ACK/数值链窄 PASS，撤销其固定 ROI 推出的水层唯一根因和完整水效果 PASS。新增 GPU M1 效果仍按 T279–T281 优先，现有 C 诊断只用于目标7补证；渲染始终只走 XComponent+shader，故障 FAULT 或完整旧 token 显式 STALE。
