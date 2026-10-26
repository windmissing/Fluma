# P-013 纯新管线工程合同 v1（T68）

日期：2026-10-26。角色：architect。任务：6adeac6ee4b017ab33f636c4。
状态：ENGINEERING_REVISION_COMPLETE / IMPLEMENTATION_REQUIRED / NOT_DEVICE_VALIDATED。
不使用真机。仅方案，不代开发、不重跑测试、不宣称效果通过。清单仅维护自己状态；整体清单/滴答同名清单/派工与完成登记归manager。

## 1. 用户强制边界与本revision身份

**所有后续场景上屏均为新XComponent.TEXTURE+GLES shader管线；禁止Canvas渲染或失败回退，包括背景、前景、水体、雾、容器、沙床、真实粒子。** 不能通过Canvas截图/bitmap复制到新surface伪装迁移。失败只允许有据的上一有效新管线帧（明确STALE身份）或FAULT；不能伪装当前提交/呈现成功。

新工程身份：`renderContractRevision=P013_NEWPIPE_ONLY_V1`、`compositionRevision=P013_FULLFRAME_SHADER_V1`。只改变绘制责任/合成与故障契约；沙材质仍使用签收的七算法版本、Rmat→统一Lmat→一次ToneMap和原P1 OFF定义；高度场/粒子同提交token、坐标映射、state2库存、源/预算/冻结阈值不变。新全场合成需重新取得视觉/颜色证据，不继承旧Canvas或G0探针PASS。

基础双场/真正干空容器先采用已定基础shader；完整P1材质G2–G5独立继续。不以全材质未就绪为由回Canvas，也不把基础相色pass当完整P1已达成。非场景的ArkUI按钮/文本/面板可保留交互，不能用它们绘制场景图层；FAULT可有明确系统文字提示，不伪装为场景。

## 2. 版本化替代索引（原文保留，新增合同显式覆盖）

| 历史资产/条款 | 本revision替代内容 | 保留部分 |
|---|---|---|
| [P1工程rev2](architecture-P013-p1-material-composite-v1.md) §2/§3/§6/§7/§9的Canvas背景前景、READY前退Canvas、整块旧臂回退 | 全场GLES pass与§5/§6状态机；不再调用drawLegacyFull/renderBackground/renderForeground | §4/§5已签收材质/核/采样/七版本/五量、资产大小与source身份不变 |
| [T33](architecture-P013-p1-g0-composition-revision-v1.md) §2/§3/§5的bg Canvas→沙TEXTURE→fg Canvas、失效回Canvas | TEXTURE内完整场景顺序；分层shader alpha与全帧暂存 | TEXTURE、输入None、真实surface px、sRGB A/B/C三层证据原则 |
| [T37](architecture-P013-dualfield-particle-shader-adapter-v1.md) §5/§6/§7的CPU背景前景与Canvas真实粒子退路 | 基础/完整场景均全shader；故障显式STALE/FAULT，不启B tracer补质量 | 同发布token、稀疏SoA/定容上传、state2、镜像/DPI与物理只读 |
| [T45](decision-P013-T45-material-rollout-v1.md) §2 G4/§3.4与§4空窗反例中的旧Canvas退路 | 新管线最后有效帧/FAULT；G4变成纯新管线装配 | 分阶段G2/G3/G4/G5及已定算法范围，不把旧测试执行完成当通过 |
| [T53](tester-T53-g2-uploader-cases-registration-v1.md) B04末句、E01/E02 | §8逐条版本映射；新用例禁止Canvas，不静默删旧条目 | A04→402、A05→400、C02零上传/状态不损及其它上传/原子门槛 |
| [T54历史报告](developer-T54-material-uploader.md)/P013MaterialFallback | drawLegacyFull/clearForeground/hideEntireGles不再是新产品failure API | 上传器/typed bridge/CPU双代/READY条件；已完成卡不重开 |
| [T64修复任务书](analysis-P011-T64-t44-failure-routing-v1.md) §3.1/§7的schema4 Canvas修复方向 | T66禁止修Canvas回退；只迁移新管线入口与准确故障身份 | 实际surface px、不可造首拒因、提交/导出身份、幂等收尾/CLOSED严格计数等原方案 |

这些是用户新边界授权的工程/渲染测试版本变更，不改endpoint/peak协议、质量判线或历史结论。原文件和旧测试保留历史身份；本稿不将其既有“Canvas成功”结果重命名成新管线成功。若后续发现噪声/颜色数学需要改变，designer登记新算法revision，不由developer暗改。

## 3. 场景包、背景/沉积/粒子/前景唯一顺序

新增生产入口（接口为待实现合同，不冒称当前已有）：

```text
prepareNewPipelineFrame(physicalPublishedState, displayState, acceptedConfiguration)
  -> P013ScenePacket（使用复用定容slot）
drawNewPipelineFrame(packet) -> P013SceneReceipt
```

P013ScenePacket一次复制并绑定：原run/stateEpoch/physicsCommitId及ledgerCommitKey、renderFrameId/请求帧、surface/context/geometry/continuity身份、native实际像素尺寸/box、route active/池count、同态T_data与SoA、背景/水/雾/容器参数、appearance/light revision、材质generation/七版本与所引用资产hash。只prepare一次，不分别在背景/沙/前景消费staged配置或推进motion/noise时间。GPU只读原快照，不另积分粒子/改床高。

同一个TEXTURE、同一个候选完整framebuffer、同一ScenePacket下按固定顺序执行：

| 顺序 | shader责任与来源 | 约束 |
|---|---|---|
| 0 | 场景背景/容器底色 | 全scene rect覆盖，来自现背景参数；不露底层Canvas。明确干场无水时跳过水pass |
| 1 | 水体底色与水域雾 | 复用当前waterTop/同frame mask与已登记噪声/包络参数；不得从当前Canvas截取像素 |
| 2 | 沙床沉积 | 基础相色或完整P1材质；D只含bh/bl/mh/ml，不把U/P填成沙床；完整OFF仍是完整材质且Lmat=1 |
| 3 | 真实粒子 | 同token的FLYING/CONTACT_WAIT；按T37坐标与state2规则，C active时B tracer不feed/draw；无算法授权不加新拖尾/淡出 |
| 4 | 液面高光 | 当前waterTop/原宽度与alpha候选，干场不伪造水面 |
| 5 | interface雾 | 沿既有sandTop两侧包络/FOG及E3消融；覆盖沙侧的原顺序保留，不并入Rmat再照明 |
| 6 | 容器边框/场景前景标记 | shader几何；原box/clip/invert一次映射，DYN等消融语义明确保留 |
| 7 | 全帧present pass | 只在完整候选通过后向实际surface提交；失败不能只展示背景或某一新层 |

Canvas旧函数仅是迁移时只读的数学/参数出处；有真实asset生成器时可复用putImageData之前的原始无光照字节和纯CPU数值/离线导出。禁止运行时drawImage(Canvas)、getImageData、截图贴图、隐藏Canvas背景前景来绕过全shader要求。现P012Adapter若兼任采样/几何数值，只能抽为无Canvas依赖只读helper，不能调用其render/renderBackground/renderForeground。

P1 shader数学不改；水/雾/高光/边框从当前纯参数/已定包络移植，不以“迁移”为由改强度、noise seed、光向、ROI或冻结E3指标。若旧Canvas的滤波/栅格化细节无解析定义，明确列sampling difference和待designer确认项，不能保证逐像素等价或自行调参数贴图。

## 4. 合成域、alpha、全帧缓冲与资源

### 4.1 唯一线性显示域over

工程合成身份为`LINEAR_DISPLAY_PREMULT_OVER_V1`；这与沙体的材质辐射域不同。沙先按原Rmat/Lmat/ToneMap得到线性显示值，**其它背景/雾/边框不再次经过沙的ToneMap或Lmat**。旧palette/颜色字节EOTF一次得到显示线性颜色，覆盖alpha按原定义取值，不做EOTF。

沙/真实粒子维持二值coverage。原来已有的半透明雾/高光/边框使用预乘显示线性颜色`(a*cLinear,a)`，over为`Cout=Cs+(1-as)*Cd, aout=as+(1-as)*ad`。开启这些pass的GL_BLEND时用`ONE, ONE_MINUS_SRC_ALPHA`（RGB/alpha分别一致），背景与不透明沙覆盖写可关闭blend；不把非预乘RGBA误当预乘，不在编码值上重复乘alpha。最终背景覆盖的scene rect通常alpha=1，**局部沙pass沙外alpha=0不等于完整画面沙外必须透明**；透明测试需分别检查局部pass与最终背景。

新全shader over与旧Canvas的底层合成域/量化未证明逐位等价，明确为新的工程合成身份；既有沙材质数学不变，效果和E3需按原阈值重新取证。不能把这项差异隐瞒为已验证跨栈一致；若designer判定必须保持另一合成模型，再出显式修订，不暗调本合同。

### 4.2 可恢复的候选与LastGood

使用两张自有全帧纹理（candidate与LastGood）及FBO。首选明确`GL_SRGB8_ALPHA8`、1层、无mipmap；逐个查询实际attachment encoding并检查FRAMEBUFFER_COMPLETE/GL错误，按T33的sRGB write-control规则执行。所有pass输出线性预乘值，硬件负责写sRGB；采样此sRGB fullframe纹理由硬件解码，present shader不再手动EOTF/OETF。present target仍须真实sRGB surface与实际encoding核查；不支持时FAULT，不能临时加gamma或降Canvas。

原材质RGBA8纹理仍按签收稿手动EOTF一次；其处理不可与新的sRGB fullframe纹理混同。A=API/attachment、B=GL数值读回、C=最终无损截图继续分开；FBO通过不等于surface或屏幕已通过。readonly历史Canvas截图仅离线对照，不能作新管线输入。

候选frame绘制完所有pass并检查GL错误后，才present+swap。LastGood纹理只在该次成功swap后与candidate交换所有权；任何失败不得把未完成的candidate覆盖LastGood。不能依赖swap后backbuffer内容未定义的缓存，也不能从Canvas重建LastGood。首帧无LastGood时缺任何必需pass都FAULT/NOT_READY，不显示“背景已成功=全帧成功”。

两张全帧RGBA8载荷约`2*widthPx*heightPx*4`，不含驱动/系统TEXTURE/深度等开销；例如1080×2340约20,217,600B。单独报告FBO/frame textures与原materialTextureLiveCount≤4（两代各两数组）；新增全帧纹理不计入材质数组限制，也不能隐藏总GPU增量。定容资源在尺寸/context代变化时重建，不每帧glGen/compile；超限分配失败FAULT，不缩物理Nmax/尺寸求通过。sample/readback仅测试入口，不每显示帧读取到CPU。

## 5. 呈现状态机、失败身份与不伪报成功

状态唯一为NO_VALID_FRAME、CURRENT、STALE、FAULT（资源上传状态另沿G2枚举）。`drawNewPipelineFrame`返回：

| receipt字段 | 精确定义 |
|---|---|
| requestedFrameToken / requestedGeneration / requestedConfigRevision | 此次想画的源，不等于实际成功输出 |
| renderedFrameToken / renderedGeneration / renderedConfigRevision | candidate所有pass完成且有效的源；失败前未完成则null |
| lastGoodFrameToken / lastGoodGeneration / lastGoodConfigRevision | 仅从上次成功newpipe全帧+swap取得；旧Canvas绝不能填这里 |
| presentationState / faultReason / firstFailureStage | CURRENT/STALE/FAULT等；首拒字段、pack/prepare/draw/profile/swap阶段完整保留 |
| currentFrameSucceeded | 仅本次请求完整有效并成功swap时true；STALE永远false |
| requestedSwapSucceeded / staleSwapSucceeded | 新候选和重放旧帧分开；不能让一次旧帧swap替代请求swap |
| lastGoodSourceEpochs / targetSurfaceEpoch / targetContextEpoch | stale的来源与此次目标分列，不覆盖旧身份假称新frame |
| activeGeneration / pendingGeneration / actualPresentationGeneration | 资源状态、待激活与实际画面分列；三者不能只用requested代冒充 |

swap成功只证明buffer成功交换，未取得display present fence时不得声称物理面板已呈现。CURRENT是本合同“请求完成且交换”的状态名，设备实际显示仍需C层证据；`displayPresentVerified`缺证据保持false/UNKNOWN。

| 情况 | 唯一行为 |
|---|---|
| 第一次合法完整newpipe帧 | 成功swap后建LastGood，CURRENT；资源/配置同代 |
| 新帧pack/源域/epoch拒绝，有兼容LastGood | 不改LastGood；允许复用旧纹理重放，STALE并显示/记录原token及当前请求错误；不再跑旧物理积分 |
| candidate部分pass/GL失败 | 丢本候选，LastGood保留；不得只让新背景/旧沙/新fg混出一帧 |
| profile/swap失败 | 当前请求失败；只有上下文/目标仍合法时才能下一次单独重放LastGood，记录staleSwap结果；不承诺“同tick一定能救回” |
| 没有有效LastGood或重放也失败 | FAULT，有明确错误；可保留系统已有画面但视为UNKNOWN/STALE，不能报告本次成功 |
| 当前物理继续变化而画面保持旧帧 | 保留旧发布token，真实physics状态不回退；不得回滚池/床、生成tracer填画或伪造新commit |

LastGood只兼容同context内有效GL对象、已核profile、实际尺寸/几何映射；不同尺寸/方向/profile不能把旧纹理伸缩后冒称同帧。若新映射可以重建旧源，仍是显式stale重绘并带两组身份，不静默复用旧receipt。

## 6. 双代与surface恢复：不混代、不借Canvas

1. Stage/Pump失败：旧active资源与其完整旧配置保留，按旧配置可继续**新管线**绘制当前合法物理源；此时实际generation为旧代，desired新revision仍pending/failed，UI不得谎报新外观已应用。
2. Pending未READY：不画未传完纹理；旧active可供完整场景所有配置使用。无旧active时保持LastGood或FAULT/NOT_READY。完整材质候选不能暗降基础相色并仍标P1成功；基础模式须用户/产品明确选择独立pipelineId。
3. 新代呈现采用一个受owner/mutex保护的frame transaction：canActivate预检当前目标binding（T67）→pin READY pending及整代配置→用它生成完整candidate→present+swap→只在成功后提交可见配置与LastGood、执行不分配内存的active指针提交→回收无在途引用旧代。检查与提交不可夹入另一个JS stage/bind。
4. READY pending可以作为同一事务内candidate绘制源，不能在上传中采样。既有G2独立Activate测试仍按旧API合同保留；**新产品路径**不在draw前从JS单独activate再消费可变UI配置。native新wrapper将已预检的Activate commit置于成功swap后的同一次串行调用，逻辑frame boundary上原子生效。失败不进行该commit，旧active/pending按具体错误保留或FAILED，不出现半新uniform/旧纹理。
5. active+pending最多两代；LastGood全帧纹理是已完成画面，不是第三份材质代。旧active删除时遵循在途GPU引用安全；不得为实现事务偷占第三材质代或提前覆写旧CPU镜像。
6. surface/window/context丢失：标旧GL对象不可用，不能重用旧handle或声称LastGood仍可采样。CPU完整资产/上次完整源包可保留；新actual surface px/profile/epoch就绪→Stage/Restore/Pump到READY→完整重绘再swap→建立新LastGood。重传前FAULT/NOT_READY，不回Canvas。
7. 同context只有surface重建时，也先验证尺寸/profile/geometry与目标epoch；若复用仍合法的fullframe纹理，仅可明确STALE重放。销毁/恢复的资源计数与故障首例记录，不拿其它session的swap当本次恢复成功。

native新增唯一内部租约接口：`AcquireSceneLease(generation,bindingId,frameToken,surfaceEpoch,contextEpoch)`只允许完整READY pending或既有active，返回固定代的只读纹理/配置引用；`CommitSceneLeaseAfterSwap(lease)`在同锁同次调用中于成功swap后无分配地提交代指针，`AbortSceneLease(lease,reason)`保全旧active并按错误处置candidate。租约不向JS泄露GL handle，不能调用只返回active的BindActive后假称画了pending；frame transaction期间禁止替换/回收已pin代。所需引用与回收记账必须在draw前准备好，不能swap成功后再发现无法提交配置。

G2/T67绑定、A04/A05/C02码值语义由其独立合同决定，本节不重定数字错误码；同一故障的资源receipt与scene presentationState分别披露，不将STALE当一个新的材质errorCode。

## 7. T55/T56/T66接口任务书与文件所有权

| 责任任务/文件 | 可执行范围 |
|---|---|
| T55：material_pass/GLSL及材质typed frame | 保持完整H/L/M、B/C共享一次Lmat/ToneMap；输出candidate沙层所需线性显示值，暴露readback/版本身份，不调用Canvas；与T67绑定接口适配分hunk交接 |
| T56：新增scene_compositor/fullframe_targets/background_pass/foreground_pass（native及GLSL） | 全场顺序、双全帧FBO、完整frame transaction、LastGood/STALE/FAULT、所有层同packet配置；严禁运行时Canvas底图 |
| T56：P013MaterialController/Types、P013RenderController/Types、native桥/CMake | 新生产scene入口和状态回执；替代P013MaterialFallback，旧forceBaseline/fail不得再选CANVAS_BASELINE；桥/资源实际所有权由native owner，JS无纹理handle |
| T56：SandCanvas/EmptySandDemo及场景组件 | 名字可以历史保留，但实际不创建上屏Canvas节点/2D ctx；统一TEXTURE scene；输入手势独立透明命中容器，控件不被None误伤；干场不伪造水层 |
| T66：SandCanvas产品双场入口/日志hunk | 直接接T62真实surface px+准备生命周期+schema4发布；失败只报新管线状态，移除Canvas分支，不能修P012 schema4作为退路；与T56同函数串行交接 |
| T66：观测/store/worker/身份/finish hunk | 沿T64/T65修复无hook/getter身份、实际route、单用途finish、两流written/fsync/CLOSED fence；不因渲染失败丢采样或伪造成功身份 |
| T48/资产模块 | 继续提供同源无光照原始字节/metadata；可以复用离线生成器，不能runtime Canvas raster再上传 |
| tester/effector后续卡 | 登记版本化新测试/复测原失败；tester报告执行与证据，designer负责差异/根因分流；设备由manager打真机标签串行安排 |

串行所有权：T55先交material_pass与drawMaterialFrame相关hunk→T56接scene wrapper和完整配置；T66先做其独占身份/store修复并消费必要T65反例；page/controller同函数在T56/T66间明确前者提供scene API、后者接真实双场/采集入口，后拿文件者基于前者commit最小追加，不覆盖整页。架构完成不授权绕过尚缺的真实实现前置，不由执行session移动他人队列。

若T66当前修复出口还缺T56的全背景/前景能力，T66只可报告其已完成身份/新入口hunk与真实剩余依赖；不能以只做部分准备把整卡宣称通过，也不能为了“有沙”临时回Canvas。是否拆分开发卡由manager处理，不附加manager验收。

## 8. 测试版本映射与反例（保留旧测试，不改物理判线）

| 历史测试/条款 | 新revision对应要求 | 版本处理 |
|---|---|---|
| T53 B04 READY前退Canvas | CPU镜像/重传/旧epoch拒绝原样；READY前LastGood或FAULT | 新增TC-P013-NEWPIPE-B04；旧B04原文/报告保留，不能让旧Canvas子项PASS冒充新要求 |
| T53 E01 draw失败全量Canvas | candidate失败不改LastGood；请求失败/实际STALE身份一致，Canvas调用次数=0 | 新增TC-P013-NEWPIPE-E01，不删除原E01 |
| T53 E02 profile/swap失败整块回退 | 无兼容资源时FAULT；可重放时只算staleSwap，不伪造请求swap成功，Canvas调用0 | 新增TC-P013-NEWPIPE-E02，旧E02只适用历史revision |
| T54 controller fallback回调测试 | 注入旧drawLegacyFull应立即暴露违规；新版只消费LastGood/FAULT接口 | 新测试使用新contract identity，旧contract runner独立归档，不静默改历史JSON或把旧FAIL转PASS |
| T65 schema4 Canvas拒绝 | 保留作为旧路径事实，不修它做上屏；新路径验证schema4同token输入与真实拒绘原因 | 新增NEWPIPE_SOURCE_COHERENCE测试；旧证据原封保留 |
| T33三层/透明/颜色probe | 背景/沙/前景全shadermarker同frame；局部沙透明和最终完整背景分开；实际sRGB/FBO/屏幕各取证 | 新compositionRevision，旧G0适用范围不外推完整scene |

新报告含contractRevision、旧caseId映射、SUPERSEDED_BY_USER_REQUIREMENT原因、原expectation保留、newExpectation与实际结果分列；这些是**用户授权的渲染合同替换**，不新改冻结质量/效果阈值，不修改T09 CPU fixture、T53 A/D材质数学或已发布endpoint/peak估计对象。tester维护新版本测试资产，architect本文不编辑原测试或重新评分。

必须覆盖：首帧无LastGood、各pass中途失败、swap失败、pending部分上传/替换、绑定被更新、同代全配置、context loss、resize/profile变更、过期token、只背景完成、LastGood被candidate覆盖、黑屏误报透明、B tracer双画、UI参数新旧混用、无水背景、UI触摸透传；后台恢复不得把旧stale帧当最新source。旧Canvas函数可用测试spy：任何production frame/failure分支调用都失败；静态import扫描只作辅助，实际调用/节点树也要证明。

效果与色彩迁移差异交designer，缺数据由manager派tester补证，不将整体归因压给tester。现场首拒因/真实present缺证据仍明确UNKNOWN；不把宿主GL桩或截图近似当设备完整通过。

## 9. 完成边界、资料与变更记录

本稿完成T68工程版本修订；只方案，无实现/编译/设备PASS声明。下游适配点已给既有T55/T56/T66，原T54保持完成，三项上传器合同交T67单独处理。不创建重复任务、不改他人状态；manager据完成通知检查后续真实依赖并派工。

规范参考：[Khronos EXT_sRGB_write_control](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt)说明sRGB目标的线性混合/写转换；[EGL_KHR_gl_colorspace](https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_gl_colorspace.txt)为实际surface颜色配置依据。这里只据规范选择工程路径，设备仍需查询FBO/attachment、错误状态与A/B/C证据，不能因文档有token声称能力已测通。

红线自检：无Canvas上屏/回退/背景前景；无Canvas复制帧；无假current/swap/代身份；不改物理、signed材质数学、预算、阈值或历史结论；旧测试明确版本化而非静默删除；不使用设备；执行session只改自己清单状态。

修订记录：2026-10-26，T68 v1，用户纯新管线约束替代旧Canvas合同，登记全场shader顺序、线性显示域预乘over、候选/LastGood全帧资源、可见代frame transaction、STALE/FAULT和surface恢复、T55/T56/T66任务书及旧用例映射。实现/视觉差异仍须各角色独立报告，不宣称产品通过。
