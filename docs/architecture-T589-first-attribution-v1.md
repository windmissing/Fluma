# T589：G9真实拨动producer、呈现映射与cold闭包

2026-11-18，architect，不使用真机。输入T572准确adapter/overlay/tree、T543/T552 R2与真实签包；证据 `analysis-T589-source-evidence-v1.json`。typed端点存在、合成注入成功或INPUT_CLOCK_UNVERIFIED被诚实返回，都不等于真实手势DEMO已完成。

## 实际缺口

G9研究页没有g9TouchBegin/Update/End调用，仍是legacy N2研究参数/人工target/model；native UP只生成部分记录并回UP_READY_NOT_CONSUMED，没有INPUT128构造/队列/Offer/消费回执。g9ColdRecordsV2只导kind/字节元数据，不是完整证据字节。故当前至少是**真实产品接线NOT_IMPLEMENTED**，不能交T544直接验证效果。

更严重的是g9PresentedMappingV2在token字符串相等后直接`AuthorizeClock(m)`：其内部仅proof=m.current，**没有平台clock proof producer**。映射资格不能授时钟资格；当前拒绝不能证明这条错误授权分支以后仍会拒。该endpoint还以调用次数++generation并将它冒commit，重复读可撤掉stroke；把旧g9PresentedToken和当前新scope/viewport拼接、读viewport未同锁、reset未统一，均有信任/生命周期缺口。

TouchEvidence256结构实际只有64U32，base写在prefix3200，而T543的两256U32 record应在3072与3328；其kind覆盖原record word0中已写的runLo，当前也没有BASE消费晋升/完整Config64/SEED_META64消费到真正cold prefix。不能仅因命名含256就称字段齐备。Begin仅部分safeinteger，Update/End还直接double→uint64转换，负数/NaN/非整数路径必须先拒。

## 唯一真实输入producer与时钟资格

保T543 `OHOS_TOUCH_MONOTONIC_NS_R1`，不改物理/配方。ArkTS DOWN/UP用同一changedTouches点和原BaseEvent.timestamp；MOVE零力/只审计取消。timestamp单位/来源/位型原样保存，须Number.isSafeInteger且>0、≤2^53−1，立即拆U64。x/y按同事件component-local vp及同UIContext.vp2px映射，完整window/display原值作证据，不跨硬件点和resampled点拼时序。超精度具名拒绝，不能用Date.now、frame time、native now替样本或猜单位。DOWN/UP同pointer/source/proof generation，UP active touches可0，dt与age沿T543 checked边界。

native callback入口自己用已核 `CLOCK_MONOTONIC` ns写receipt，不信UI传receipt；根freeze/claim也用同域。新增只读 `g9ClockCapabilitiesV1()`披露producerId/SDK+platform build/source SHA/unit/epoch/proofGeneration/state。授予由native冷 `InstallVerifiedProducerProof(descriptor)`在**真实平台路径已证**后完成，不提供JS可写verified=true接口，也不由成功Swap/token/两次延迟拟合授权。descriptor必须绑定实际OS/build、采样API→producer→转换→CLOCK_MONOTONIC路径和本native时钟实现；SDK“since boot”一句或上游源码存在不等于当前vendor路径已验证。

开发交完整proof producer/读取/撤销路径和真实平台资格输入要求；如果当前构建不能提供该证明，保持INPUT_CLOCK_UNVERIFIED并保缺哪一环/原API/支持域，交独立tester/平台证据补齐。不得合成一份看起来同域的timestamp让T544放行。Stop/permission/context/page/run/inputEpoch切换统一撤proof+stroke+mapping，恢复需新证明generation，CLOCK_SYNTHETIC仍独立标记研究注入，不能宣REAL_TOUCH。

## PresentedMapping与并发

映射由GL owner在新XComponent+shader完整FBO、成功Swap及present授权全部通过后原子发布，来自**该lease**的完整scope、真实commit/publication、参数/seed/adapter SHA、viewport/uBox及UI左上原点变换。App mutex内复制同一不可变快照后释放，再进touch锁，统一锁序，禁止touchMutex→appMutex与反序混用。失败/late/context换代清资格，STALE帧不得制造新映射。

页面只能查询已发布native映射，不能提交任意矩形/token授资格；相同token/viewport/proof不变时查询幂等，generation只随真实映射身份变化，不按API调用次数增长。UP仍需与DOWN同映射/geometry，不能按当前新scope包装旧token。commit必须是真实lease commit，不用generation代替。clock和mapping独立两张资格，共同满足才DOWN_ACTIVE。

## Config、UP入队及真正cold

研究页显式REAL_TOUCH_R2模式实际调用与绑定表一致的Config64版本入口（旧N2/旧32word入口保legacy/SYNTHETIC身份），typed校验/安全JSON消费沿T581；R2合法S40 seed、column9=B、parameterRevision3/seedRevision2及上传SHA沿T552，不把旧g16Q空床N2配置当BALANCED。新注册/声明必须有真实页面调用与构建绑定矩阵，不再交只有端点的半条链。

Down/Move/Up/Cancel都接真实.onTouch；UP只一次组T543 INPUT128_V2原body、原UP capture、RNE J/cap、实际scope/参数SHA，再经现有16槽队列及Controller实际Offer→Claim→completion。QUEUED不是ACCEPTED；失败不刷新UP年龄/不重试旧stroke。保manual pose与真实command两种来源区分，不能把人工target冒sensor/shake。GPU CONFIG_INPUT_GATE复算配方仍唯一物理权威。

真正cold prefix两record各**256U32=1024B**：CURRENT_FROZEN_TOUCH[3072,3328)、BASE_LAST_CONSUMED_TOUCH[3328,3584)，Config64[3584,3648)，SEED_META64[3648,3712)，余保留0，按正式逐word字典填kind/身份/raw/capsule，不让kind覆盖run。当前record对应本根真实冻结命令，BASE record在BASE冻结时保上次已消费原件，只有接受回执（含全部未施加但已消费）才晋升，BLOCKED/FAULT不晋升；NONE/重复也明确绑定，不只留最近UI字段。构造时同scope锁定两record，真正cold writer/collector/reader导完整原字节及hash，元数据getter不能代替。

复用T543已声明22,528B native信用，原预算已含两份完整1024B证据及有界队列/clock/mapping，不以现有64word错误结构的较小sizeof作为新预算。prefix32768B/INPUT128/4SSBO/原dispatch/根1024B completion及250ms/GAP不变；无新增CPU物理、无限stroke历史或逐帧库存map。若实际布局需增配须先具名回architect，不静默占spare。

## T544最小真实前置

须同时有真实页面Config64/R2合法初态、真实Down/Move/UP→队列→GPU接受、独立有效clock proof、成功Swap同lease映射、完整cold双record/参数/seed读链。缺任一仍NOT_IMPLEMENTED或资格UNKNOWN，不能仅凭native端点编译过就开始真实拨动效果结论。实现交齐后，签名/部署/真实操作/clock证据及P05/P06/P07效果由tester/effector沿已有设施直接做，不另加manager验收门，也不把真机测试塞进开发关闭门。

所有执行接力完整遵循 [共同约束](architecture-T581-T589-handoff-policy-v1.md)。本卡未调用端点/运行设备测试；只是把缺口和正确producer/信任边界/完整接线落成可执行工程合同。
