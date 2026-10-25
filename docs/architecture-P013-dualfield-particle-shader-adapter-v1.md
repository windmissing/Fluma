# P-013 双场到 GLES 同帧接口 v1（T37）

日期：2026-10-25。责任：architect。任务：6adda18ae4b0e6ce82af0d63。
状态：**DESIGN_COMPLETE / DEVELOPER_ACTION_REQUIRED / NOT_IMPLEMENTED / NOT_DEVICE_VALIDATED**。
完成即交原T20，无manager验收前置。仅新增本文；不改产品/测试代码，不占设备。最新AGENTS职责裁定优先于历史“manager验收/完整P1先行”说明。

## 1. 输入事实与本次替代范围

依据：T02实际交付adbcc67、`WaterSandField.ets`/`P011ParticleM2Field.ets`/`P011ParticleField.ets`、[历史视觉稿](architecture-P011-particle-visual-wiring-v1.md)、[T33合成合同](architecture-P013-p1-g0-composition-revision-v1.md)、[P1材质工程稿](architecture-P013-p1-material-composite-v1.md)。本次只替代历史视觉稿§2“首选Canvas、GLES须等完整P1”的后端门序；物理只读、state2仍是真库存、LAND只扣granted、tracer互斥等规则保留。

当前源码（本次只读，后续行号随并发变更）：

- WaterSandSnapshot有schemaVersion与可选particleState；schema4的`copyStateTo`可同调用复制高度场与完整粒子SoA，schema2/3不能冒充真实粒子证据。`snapshot()`分配新对象/数组，不应每显示帧调用。
- P011ParticleSnapshot的x/y/vx/vy/mass均为Float64Array；phase/source/state为Uint8Array、id/sourceId为Uint32Array，数组长度为nmax，**活粒子不保证前liveCount连续**。`copyStateTo`复制所有槽，state0是洞。
- 历史稿的copyParticleRenderSnapshot、particleRevision等是设计建议，并非当前已实现API。现快照也没有可直接当“已提交身份”的physicsCommitId；不能用页面snapshotId或pId补造。
- 页面目前先复制field快照，再Canvas绘制，再提交P013；P0 adapter只上传128×1 RGBA32F（512 floats），native drawFrame不支持粒子。已有P0热图不能作为产品粒子pass。
- 外源注入可以在物理子步之间成功改变池/源账本；invert/reset/restore亦改变状态。因此仅用“最近substepId”不能证明当前高度场与粒子同态。

## 2. 唯一数据入口、发布身份与帧包

唯一生产后端为 **T33 TEXTURE内先沉积pass、再真实粒子pass**。CPU只复制/打包；不逐帧从Canvas截图取纹理，不在GPU积分、不由receipt另生一批视觉粒子。Canvas路径只作完整同态故障退路/旧基线，不是T20的GLES完成出口。

新增接口（以下均待developer落地）：

```text
WaterSandField.copyPublishedRenderStateInto(out: P013DualFieldStateSlot): RenderCopyStatus
P013DualFieldAdapter.packInto(stateSlot, frameContext, uploadSlot): PackStatus
P013NativeRenderer.drawDualFieldFrame(frame, tdata, particleXY,
                                    particleMeta, visualParams): P013DualFieldReceipt
```

`out`由adapter创建复用，包含schema4场快照、预分配粒子快照及以下标量；物理模块不import render，实际公共类型置于physics独立只读类型模块，由render引用。不得向renderer暴露可变pool对象。

| 字段 | 类型/含义与唯一责任 |
|---|---|
| runId | 原物理run身份，复制；不能renderer自造 |
| stateEpoch | safe integer，reset/场重建/跨run恢复时改变的只读发布代号 |
| physicsCommitId | safe integer，**渲染发布序号**；每次成功原子状态发布递增一次，含STEP、外源准入、invert、restore、route变更；拒绝/rollback不增，不用原endpoint ordinal代替 |
| publicationKind | STEP/EXTERNAL_ADMISSION/RESET/INVERT/RESTORE/ROUTE_CHANGE，解释上述发布来源 |
| ledgerCommitKey | 原四元提交键或null；只有存在对应成功ledger提交时复制，非STEP发布不伪造endpoint成功子步 |
| routeId/particleRouteActive | 从物理唯一门复制；页面want只能控显示，不重算active |
| field.schemaVersion | 4；缺真实粒子快照时明确不可用，不将P补0 |
| particleCapacity/liveCount | 实际nmax/活数；不得把512候选常量当运行时真值 |
| field+particleState | 同一次不可交错copy，原material-volume/连续列/高度坐标不变 |

发布token为 `(runId,stateEpoch,physicsCommitId)`，通过同一个packet传递，field/particle不得各发一个token。physicsCommitId只是渲染coherence标识，不是新物理事务/端点采样，不改变dt、积分、预算、守恒或账本版本。外源一次成功准入只产生一次发布（请求/重试不增）；一次含多个事务的STEP只产生一次最终发布。reset初态发布一次；restore发布新stateEpoch，不能复用旧epoch的纹理。接口实现前枚举所有状态变更入口并自测，无未编号写入。

copy只在完整物理调用返回、事务锁释放后执行。当前单UI调用栈内同调用复制原数组/标量即可；不得在copy里调用step/collect或重演注入。若未来跨线程，writer发布不可变页，reader需pin/refcount或双缓冲所有权；version-before/after校验也须配合内存同步，不能仅靠两个数字读数宣称无数据竞争。页仍在用则保留上一完整帧或跳本帧并记backpressure，不能覆写进行中的页。

FrameToken另含renderFrameId、geometryEpoch、continuityEpoch、appearanceConfigRevision、surfaceEpoch/contextEpoch、visualInverted；同一物理token可多次显示，不假装新物理提交。前景Canvas与GLES逻辑同token不等于硬件同present，动态证据仍归T31/T35。

## 3. SoA打包、GPU格式与定容资源

### 3.1 活槽压实规则

adapter扫描slot=0..nmax−1，仅state1/2进入输出前count项。保留原稳定particleId与phase/state，顺序固定为源槽升序，不按速度/质量挑选；state0不画。检查压实count=liveCount、正有限mass、有限x/y、合法phase/state/ID；异常整包拒绝，不能删坏粒求通过。质量仅作诊断/一致性，不能以粒子个数替代P或映射出新的物理粒径。

| 上传字段 | 格式/长度 | 使用 |
|---|---|---|
| particleXY | Float32Array，长度2×nmax，前2×count有效，xy为连续列/材料高度 | 从Float64明确量化；不修改物理原值 |
| particleMeta | Uint32Array，长度2×nmax，前2×count有效 | 每实例两个uint：phaseState=`phase | (state<<8)`，particleId原uint；保留高16位为0 |
| T_data | Float32Array，基础版128×1 RGBA32F | 沿DISPLAY_4X3_V1：sandTop/waterTop/bh/(bh+bl)，与同packet场快照生成 |
| visualParams | 强类型只读参数 | 既有heavy/light显示palette、候选半径、alpha二值、视觉revision；无数值签收则标候选，不冻结新阈值 |

native同步校验typed-array类型/长度/byteOffset/非detached/各版本/count≤capacity，随后拷贝**有效前缀**到自己的定容staging，再返回；不能跨调用保存ArkTS backing pointer。数组对象定容，NAPI传的是全容量视图，count指定前缀；native不要求每帧slice生成精确长度数组，也不把上帧尾部当活粒子。拒绝类型/长度/非有限/非法meta等错误，记录精确errorStage，不自动转换任意对象。

native有两个定容VBO：XY为8×capacity bytes、meta为8×capacity bytes；integer meta用整型attribute；`glVertexAttribDivisor(...,1)`，以六顶点quad实例化，每帧一次particle draw（count0则0次）。中心坐标用attribute，quad由gl_VertexID固定生成；不依赖驱动point-size范围。CPU上传有效前缀16×count bytes，capacity变更只在停帧/新资源代一次重建，不逐帧glBufferData。物理不受buffer容量不足影响；不足拒包并fallback，不能偷偷降低Nmax或漏画。

双ArkTS upload slots合计32×capacity bytes，native staging与VBO各16×capacity bytes；以capacity=512举例，单套粒子有效上限8192B、双上传页16384B，另计schema4复制页、T_data、GL驱动分配、textures和系统TEXTURE buffer。数字只是payload预算，不是总显存/零分配证明。schema4完整页包含更多Float64/缓存数组，不能把16N写成全部内存。report上传字节/分配/活资源和高count耗时。

### 3.2 版本与旧桥隔离

新增bridge方法`drawDualFieldFrame`与`dualFieldPacketRevision=P013_DUALFIELD_PACKET_V1`、`particleLayoutVersion=PARTICLE_XY_META_SOA_V1`，保留旧drawFrame/P0参数与enum0–3语义；不把双场数据塞进旧512-float校验器。基础沉积pass沿layout1，完整P1未来沿layout2，各有明确pipelineId分派；不以升级layout1同名含义偷渡三行数据。

## 4. 坐标、遮挡、state2与入床连续性

原坐标：粒子x为连续列（0..cols），y为从容器底向上的材料高度；不是数组槽号或“列中点”。映射先在未倒立容器中：

```text
u=x/cols; v=1-y/capacity
xCanvas=boxX+u*boxW; yCanvas=boxY+v*boxH
visualInverted 时：u'=1-u; v'=1-v（只一次，绕容器中心）
xNative=xCanvas*canvasToSurfaceScaleX; yNative=yCanvas*canvasToSurfaceScaleY
clipX=2*xNative/surfaceWidth-1; clipY=1-2*yNative/surfaceHeight
```

实际页面若box偏移有额外父坐标，统一frameContext提供完整映射，禁止native再猜DPI。均匀缩放仍需记录scale；遵循T33/P1既有非等比限制。物理镜像`x′=cols−1−x`是原物理invert规则，**本稿不改**；渲染只映射当前快照及visualInverted，不再在CPU重新镜像SoA。quad半径由Canvas逻辑px转native px，不能将native px再乘DPI一次。

- 沉积pass覆盖同态D=bh+bl+mh+ml，profile按原DISPLAY平滑，不把U/P高度伪填进D。部分LAND同一次publication的床增加grant、粒子保留remainder；全额LAND后该ID不再进入实例包，床仍由该提交后场重画。
- FLYING（state1）按快照中心绘制小型不透明相色quad，fragment二值圆mask、盒外透明；需要床遮挡时读本帧同T_data，禁止上一帧sandTop缓存。粒子不受沙体E2光场自动作用，见§5的基础显示身份。
- CONTACT_WAIT（state2）仍在粒子pass绘制，中心仍是实际坐标；在沉积pass之后覆盖显示，不按“触床”自行隐藏、不按平滑显示沙面删除。圆片可与床边界重叠，这是位置忠实的显示标记，不代表该质量已经沉积；本版不额外把中心吸附/抬高到显示沙面。若视觉需接触抬升/半透明/尺寸按mass等新算法，交designer，不自行修坐标。
- 合法几何clip可使盒外点无可见像素，分列offscreen/clippedCount；非有限/非法坐标不得静默计入clip。不能以像素/显示点数证明质量/守恒/落床；诊断账本仍原checker负责。
- 不从速度积分位置、不平滑物理落点、不用sourceId/pId推age、不做凭空拖尾/淡出；渲染开关或surface重建不clear/reset粒子池。

## 5. 基础GLES与完整材质分层交付

**基础出口现在即可实施**，不等完整材质G1解冻：T33的TEXTURE/profile能力+基础沉积相色pass+真实粒子pass，`renderPipelineId=DUALFIELD_BASIC_V1`。基础显示复用已有相色/候选显示参数，输入颜色是显示sRGB值，shader先EOTF后交sRGB attachment编码一次，不把P0热图当产品粒子。基础沉积只表现同态分相床高/相色，无B/C/E2材质提升声明；没有完整P1资产就标未实现。

基础pass不增加材质配方：粒子是既有相色位置标记，不自动吃Lmat或新的质量到半径曲线；固定二值alpha保持T33合同。若没有既有可复用半径/相色参数，作为明确命名候选配置登记并交designer定标，不把任意常量写为冻结效果标准。基础输出不是P1七版本材质结果，不填LIN_SRGB_REINHARD_V1冒充ToneMap已执行；基础profile独立记`BASIC_SRGB_DISPLAY_V1`，只复用T33一次编码/actual attachment能力，完整P1保持原colorPipelineVersion和数学。

完整出口未来为 `renderPipelineId=P1_MATERIAL_DUALFIELD_V1`，沉积pass按已签收Rmat→Lmat→ToneMap+layout2/六图/七版本，粒子仍后画、前景Canvas再覆盖。完整材质G1–G5与效果验证继续独立门序；基础粒子链不因它未完成而阻塞。不得为了基础pass暗改P1核/配方、Canvas光场或冻结E3锚点。

基础TEXTURE层内部顺序：透明清屏→沉积pass（不透明D）→粒子pass（不透明二值sprite）；Canvas只画背景水体、前景雾/边框/UI，**不再画第二套沙床或C粒子**。基础并非“在旧整画面上只叠几个粒子”，否则真实床会被两次着色。GL_BLEND/DEPTH/MSAA保持关闭，沙外clear RGBA零。实际profile/alpha仍按T33的API查询+GL数值+compositor三层取证；能力不足走完整同态Canvas退路，不加gamma兜底。

## 6. tracer互斥、错误与线程/生命周期

| 物理/视觉条件 | 唯一显示责任 |
|---|---|
| C active，GLES成功 | GLES真实粒子，Canvas C粒子与B tracer feed/draw均关 |
| C active且count0 | 粒子两路都空，不以tracer补画已落床库存 |
| C active且视觉ablation关闭 | 不显示粒子但不卸物理route，B仍关 |
| C inactive | B旧显示分支可用，不画C粒子；不得把C账本库存转成B |
| C active且GLES失败 | 先停/隐藏本次GLES整层，再用同一完整snapshot走Canvas真实粒子+床旧臂（若已实现）；无有效Canvas C后端则保留上一完整帧并报FAULT/STALE或明确空故障，绝不启B代替真实库存 |

切入C只清B视觉缓存（不影响物理），route/capacity变更的显示责任在frame boundary原子切换；B代码永久删除仍需原既定退出裁决，不由T37删除历史基准。任何frame token/route/epoch错配整帧拒绝，不只画新床配旧粒子；失败不更新“lastPresentedCommit”。

当前同步NAPI在同一受控调用/GL owner线程上传、draw、swap，不新增并发访问pool。未来worker必须转移有所有权的整包并在GL线程销毁资源；不得把borrowed NativeWindow跨生命周期保存。surface destroy标记epoch失效、阻止新draw、owner线程释放；恢复复用CPU最新完整包、重建shader/VBO/T_data并全量上传，不能恢复旧id列表配新场。page hidden只停止显示提交，不修改物理dt/积分/池状态。

## 7. T20任务书、文件所有权与T34串行交接

| 文件 | 责任/范围 |
|---|---|
| 新`physics/P011PublishedRenderState.ets`、WaterSandField.ets | T20：复用定容schema4复制页、发布token及所有成功mutation挂点；只增加只读发布元数据，不改物理事务或T36时间算法 |
| 新`render/P013DualFieldAdapter.ets` | T20独占：同token压实、Float32量化、SoA定容包与validate，不每帧snapshot()/slice()/JSON |
| P013RenderTypes.ets及native类型声明 | T20增独立dual方法/packet/receipt；T34的composition profile接口先交接，旧P0保持 |
| P013RenderController.ets、SandCanvas.ets | **T34先**完成TEXTURE/profile/背景前景责任与生命周期hunk并给commit，**T20后**添加dual submit及route互斥；等待期间T20可先做独占adapter/native文件，不能同时改共享hunk |
| P012WaterSandRenderer/Adapter/RenderTypes.ets | T20：兼容Canvas真实粒子fallback/旧臂及共享同帧context；T34拆分层责任的hunk先交接，不重复prepare |
| 新native `dualfield_pass.*`、`particle_uploader.*`、独立GLSL | T20独占：基础沉积与实例化粒子pass、typed-array拷贝、定容VBO/receipt |
| `napi_bridge.cpp`/`CMakeLists.txt`/egl_context.* | T34先交profile与桥入口，再T20追加dual方法；不重写T32已修桥/首次绑定 |
| 测试文件 | developer负责本接口非真机自测/编译；客观集成归T30，视觉设备归T31（profile平台证据接T35） |

可执行步骤：①先建立schema4同态token与定容copy，覆盖注入发生在step之间；②独立adapter打包/拒包自测；③实现native typed bridge+定容资源+基础两pass；④接T34提交后串行合并controller/page，关闭重复沙层/tracer；⑤构建+非真机回归+精选提交，报snapshot/packet/版本/剩余设备项。无需manager验收，开发交付即完成；T30/T31设备结果不反向作为开发关闭前置。

最小developer自测矩阵：schema2/3缺粒子不造0；活槽有洞压实；count0/满池/超容量拒包；Float64→Float32非有限、byteOffset/类型/长度错误；同帧新床旧粒子拒绝；外源两substep之间发新token；rollback不发新token；部分LAND/full LAND/state2保留；MERGE/DEGRADE只映射现存池；invert/DPI/box边缘/restore epoch；route切换ablation/tracer互斥；context loss重建/资源计数；旧P0接口及物理九通道回归。视觉质量/平台alpha/sRGB、实际帧时和端到端仍归独立测试，不用host数值PASS代替真机。

## 8. 交付结论与红线

本次方案已完成，直接交原T20与T34串行文件交接；无新增manager验收门。只新增本文、不改历史Canvas稿或T33已签收合同。GLES基础双场链现在可开发，完整材质独立推进；守恒、state2库存、镜像公式、Nmax/速度/源准入/预算、endpoint/peak协议及冻结指标均不变。没有设备或实现通过声明；算法/效果参数需要变化时交designer，不通过工程接口暗改数学。

修订记录：2026-10-25，v1（T37），从T02实际schema4稀疏SoA与现P0桥出发，登记同态发布token、定容双SoA上传、基础/完整双出口、state2与tracer责任、线程/恢复和T20/T34串行任务书。旧稿历史Canvas首选由本文明确替代后端门序。
