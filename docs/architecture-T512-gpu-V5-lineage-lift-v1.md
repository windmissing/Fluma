# T512 · V5起沙、来源层及循环标签工程闭包

2026-11-14，architect，P5，不使用真机。消费T506正式算法/manifest/fixtures、T497/T498 V4工程及T503有界冷协议，冻结独立V5实现责任和机器资源。[工程manifest](architecture-T512-v5-engineering-manifest-v1.json)保完整设计字典、writer矩阵、stage与存储范围，[检查索引](analysis-T509-T512-checks-v1.json)保原件指纹。本卡为工程交付，不实施业务、不假称运行已具备V5。

## 1. 支持域、版本与真实前置

独立`gpu_resident/m3b/`（或同职责V5模块），wire/layout/algorithm/source/scene5、features31/channel15、medium3/numeric1/time1/angle1/motion1/water2/optics2。与V4/M1/M2不同能力/输入/reader，不原地导入无来源旧床/旧活包。一个GPU owner模块内route选择，共享既有store worker，按实际shader/ABI/布局字节生成新cache key；不另加载第二DSO共享同cache目录的无协调writer。

默认 **liftMode=OFF**；SENSOR_RESEARCH及SYNTHETIC_TEST仅显式冷配置/新空run，参数版本/SHA完整且合设计域，否则CALIBRATION_CONFIG_REQUIRED。OFF也必须维护V5谱系和U/A/certificate，不能退回V4不带来源的LAND；它只使请求0/E不推进（新run E0=0）。不得因工程交付自动启产品sensor、T22或旧50%目标。

实施真实前置分别列出：

| 前置 | 当前工程结论 |
|---|---|
| T497 V4数学与T498银行/stage部分 | 已有，按版本依赖复用；V5每writer覆盖清单见下 |
| T508 blocked时间账/再入政策→T498修订gate | 尚待正式交付；V5继承其物理时间/丢弃/暂停框架，**不自行补accepted/overload/revoked** |
| 可复跑完整V4 GPU实现/同源工具 | V5实施前需实际实现或同卡先完成批准V4基础；仅manifest不是现成V4产品 |
| T503/T507冷能力 | 合同已交，实施/签包独立推进；V5大冷档用另payload descriptor，不冒M2上限兼容 |
| sensor时钟/参数/效果证据 | 模型默认OFF，真实sensor模式须§6核验；不阻止本工程稿或显式合成非机检查交付 |

T512当前约定工程稿可完成；完整产品实施放行以这些真实依赖为准，不因为本卡结束就宣布T498时间缺口消失。

## 2. 权威state与七处同步扩展

每个完整state追加：cohort4[128×64]（131072B）、depth128（512B）、certificate16[K]（64K B）、aux4[K]（16K B），合 **Bext=131584+80K**。cohort底→顶定序，每层正mass、相邻同origin/sourceId强制合并，未用层0且和=S。depth≤64；新run全0。外源origin/sourceId只能引用已接受普通沙kind0或bulk沙kind2，不能借相同sourceId跨origin/namespace混合。

七个存储域全部扩：CURRENT/ACTUAL/SPARE三个state、SELECTED/BRANCH两个combined state、SOURCE WORK spare与QUANTUM WORK spare。每域自己的cohort/depth/cert/aux均不别名；重建布局按半开段生成所有新offset，扩中间spare后后缀必须随之重定位，不能只append一份全局sidecar让多个role共用。冷header/WORK index/回放范围/compare范围/trace descriptor与shader常量同源生成，checked word→byte。

certificate16按设计完整16word，ID/generation/origin/sourceId/birthKind/birthCommit/ordinal/mass/column/parentRank/parentMass/rev1/保留0；外源parent字段0，lift记录真正床顶父层。不把后续merge质量写回birthMass。aux4含cycleTag/birthKind/certificateRev/0；free/退役时cert/aux全0，slot generation按原合同保留。每个活slot证书必须匹配本incarnation，旧代和未来commit引用拒绝（同根提案在未接受scope按设计限域允许）。

U=Σaccepted medium=WET live mass、A=P−U，与几何末态重新分类不是同一个定义。column4升U_H，5=0，8=P；T=S+P、eta=T+W、R=eta+J不变。publication列15为U，header65..70为三个U64的U/A/live cycleTag；V5 scene按同token读真实包和原medium，禁止cloud贴图额外造库存。

## 3. writer封闭矩阵

所有writer同时更新candidate、所依赖派生与谱系；非成功root不外显。下表必须落到实际source/motion/bed/finalSand/pub/replay调用点，不仅新增孤立lift kernel。

| writer/cut | cohort/certificate/tag/U原子要求 |
|---|---|
| 外源birth（点源/bulk seed） | P增，cert由外源原始ID区间/提案commit/ordinal重建，aux tag0；按真实medium增U或A；不动床cohort/E/cumulative循环 |
| SOURCE merge（可含lift包与同origin外源包） | 保a原cert/ID，tag_a+=tag_b，清b cert/aux；核tag≤新mass、保generation身份，U/A按旧/新标签和mass重算，不靠birthKind猜循环量 |
| SOURCE失败trial/dedup/metadata拒绝 | 各trial恢复七域中的本角色快照/所有U缓存/证书/标签/原有效来源前缀，branchEpoch继续；E和Lifted/Returned不随source推进 |
| MEDIUM/WET→AIR/finalSand释放支撑 | P及tag/cert不变，U/A按真实MEDIUM切点同量转移；不在末尾静默重新分类全池 |
| 沙CROSS | 搬P及U/A，origin/cert/tag随slot；目标occupied/J净空与预算保持，不改床层 |
| LAND（含partial/full） | 先预检receiver顶层合并/空层信用，再按实际q追加同origin床层、S+=q/P−=q；回床tag=floor(q×tag/m)，full返全部，Returned+=该tag；full清cert/aux，partial保出生证书并减tag/mass |
| bed三色 | q先受receiver可表达来源片前缀约束，donor top-first拆≤64片，逐片同origin减/追加/合并；库存/guard/total/local只按真实总q扣一次，不每片重复扣 |
| LIFT_BIRTH | 最低free/ID/generation及全部信用成立才顶cohort−q/可pop，S−q/P+q、U/A按新medium、cycleTag=q、真实lift cert，Lifted+=q、budget/channel减q；T/eta/R/W/J不变 |
| water/ARRIVE | 不伪改沙medium/证书；原水守恒与occupied面保持，U只由规定标签writer更新 |
| pub/cold/accept | 不补造cohort/证书或重分类；核全部新域与per-origin/cycle后打包，late/Stop/FAULT不发布candidate |

LAND顶层不同且64满时真实q0/LAYER_CAPACITY_WAIT，包/预算保留；不能吞成无来源床。bed接收若只有有限可表达片，grant为该前缀与原预算/库存/几何之min，不能先扣完床后发现层满。lift不跨cohort、不在其loop merge；pool/ID正常拒留q0 reason，generation/来源/溢出/credit错fatal，不挑下一个slot逃错。

## 4. GPU阶段、四binding与planned/sealed双前缀

V4五role每个core插四dispatch，SOURCE仍72；QUANTUM569/639/614/684、G1+F4=2348/2628/2528/2808。每role顺序如下，未执行role仍uniform no-op，计数及18族审核同V5字典：

```text
scope init/full V5 copy
V4 prepare/preflight → INPUT_E_REQUEST（冻结E0/s，算E1及入口S0请求）
GLOBAL_WATER_LEVEL → frozen medium → 水/沙motion/LAND → bed3色
LIFT_LOOP（升column、单顶层、余时0）
原finalSand → ARRIVE/fold → LINEAGE_U_AUDIT
完整spare copy → 交织物理/附属replay
POST_REPLAY_LINEAGE_COMPARE → scope finalize
```

REQUESTED/CLAMPED/OLD/AUTHORITATIVE/CONSISTENCY都从相同E0/input和完整base开始；SOURCE八trial零物理时间，不跑这四lift stage。E1只有唯一成功QUANTUM接受推进一次，不能五角色累加五遍。

四SSBO实现选择：将**物理EVENT＋附属PROVENANCE32同slot**打包在一个GL对象，descriptor分别标byteStart/stride/validRows。frame0/1/isolation scratch各包含K+4e5行物理段与4Ea行128B附属段；BRANCH/SELECTED combined含完整V5state＋e5物理＋Ea附属；SPARE期望物理/附属在QUANTUM WORK独占扩展。binding3永远是一整个event pair对象或pub/completion，不偷偷加第五binding。combined/source/quantum整段bindBase并用显式offset，range binding若需要须满足实际alignment；padding另账。

core/replay binding0 CURRENT、1 WRITE_BANK/SPARE、2 PACKED_WORK、3 EVENT_PAIR；select/finalize用BRANCH combined、SELECTED combined、WORK、control；权威compare沿T498在WORK期望段复用前比较完整实际semantic＋附属流，最终再比较consistency/selected/ACTUAL状态。不能只比新域hash，或在AUTHORITATIVE省掉谱系切点。

SOURCE/QUANTUM各有256B rootCursor控制段（设计512B合计），明确plannedPhysical/plannedAux、sealedPhysical/sealedAux、pendingCompoundOrdinal/credit/firstFailure。先预留本复合操作的全部信用、写未封body，物理和全部来源片成功后共同封行并推进连续前缀。planned≠可读取长度；failed/late时pending body只能明确诊断，不算接受/重放成功prefix。每色bed先原prefix对edge固定分配区间，64lane每lane至多一边、最多43edge；fragmentOrdinal0..63全组同步，最小(fragment,edge,operand)错误归约后才共同写本轮。禁止atomic竞速决定首错，inactive lane也经过全部barrier。末色统一提交共同sealed cursor，失败本色不伪封部分BED。

床片从global cohort/定容WORK逐轮读取，不把43×64个cohort整批放shared；shared只放64lane的错误/credit/prefix归约，沿V4≤4096B预算并在实际shader编译反射/设备能力核对。任何实现选择超该界先回architect修资源合同，不能为省事默增shared或跨workgroup自旋。

附属kind27..31与物理kind26沿manifest逐word used-mask/保留0/seal-last，含root/role及physicalOrdinal，逐scope的branchEpoch/frame/arenaGeneration由descriptor绑定；0/1/2逻辑arena号本身不授读权限。ENERGY/REQUEST physicalOrdinal0，其余必须引用同scope真实物理事件；重放在该物理cut消费相应片，不能先跑完P最终态再用最终数据“补来源”。每origin独立U64和外源admitted对齐、全沙/水、Lifted−Returned=Σtag、tag≤mass、证书/generation、预算/Hq、U/A都在scope audit及replay通过，requiredAuditMask=0x3ffff。

## 5. 输入64word、参数和native接受

新增V5专用native envelope入口和controller/adapter版本，保旧三参`setGravitySample`在旧route的旧行为；V5不得把该旧调用当起沙资格。input64原raw/filtered/alpha/threshold binary64位型和actual sensor capture timestamp保真，layout0..63按T506完整表；native自行写receipt/freeze、绑定当前input/session/run/owner/context/parameterRev/SHA，客户端伪造这些字段必须拒。freeze后一个plan的F≤4实际quantum共用同一envelope，sensor callback只更新待冻结输入，不在callback发Q或积分E。

原加速度残差adapter不证明剪切力，alpha/阈值只标候选未标定。旧adapter用callback ACTIVE ms且不输出filteredXYZ，不能直接复用三参结果冒新证据；新adapter保旧滤波算法revision和原始输入/三filtered分量/阈值，在V5专用类型传递实际timestamp，失败不改成shake0/角0继续。参数缺值、source STALE、非finite、量化前超[0,1]、seq倒退/耗尽、错input代、时域未验证均在本tick准入前拒，暂停新源/物理、不删除现库存，不推进E。此暂停如何归时间账**继承T508最终控制政策**，不能由本稿偷偷多加一种时间消费。

GPU解码s=floor(shake×65536)，E请求用E0，ramp/decay、n/(120×65536²)、两邻边guard、global channel以及反重力signed宽RNE全部按T506公式，不用F32替代。q=0不写物质；新包本Q余时0，不立刻再LAND。宽U64 add/mul/div溢出首错fatal，降低λ或关sensor不能掩盖审计错误。

native正常只map256U32=1024B尾，核revision5/features31/channel15、header161..182→completion189..210、passCount/audit及原root/permit/full identity后原子切bank/metadata。SOURCE成功保持E/cumulative不变，QUANTUM成功一次推进E1和循环账，BLOCKED/无效样本/late/fatal都不能作为成功步。header老字段和V4planner gate不重解释；不从单个LIFT_BIRTH或新pub计数提前更新外部I/sourceCommit。

## 6. SDK时钟核验与受限sensor支持

本机SDK26的`@ohos.sensor.d.ts::Response.timestamp`及[官方sensor Response](https://github.com/openharmony/docs/blob/master/en/application-dev/reference/apis-sensor-service-kit/js-apis-sensor.md#response)说明报告时间为启动后纳秒。SDK `systemDateTime.TimeType.STARTUP`包含深睡、ACTIVE不含；getUptime可返回number纳秒。原GPU页当前用ACTIVE callback时间代替sensor报告时刻，不能由同名“uptime”认定与native steady_clock同域。该官方接口说明**没有给当前真机sensor时钟和C++ steady_clock的精确同域证明**。

V5新增独立input clockDomain，禁止改现root/pulse clock。首实现为`SENSOR_STARTUP_NS_VERIFIED`（编号1），capture保原timestamp；receipt/freeze由native明确`clock_gettime(CLOCK_BOOTTIME)`读取（本地NDK有定义），不调用无单位的now替代。仅当目标SDK/设备实现证据证明sensor报告与该clock同起点/单位/深睡语义，并完成开run/恢复的桥接验证时，native才置mapping flag bit1；页面不被授权自设verified。无证明或探针错误返回SENSOR_CLOCK_UNVERIFIED，SENSOR_RESEARCH不准入，不用猜offset/减callback延迟“校准”。该限制是设计明确支持域，OFF和SYNTHETIC可独立开发测试。

精确验证接口为冷`queryLiftClockCapability`/`verifyLiftClockEpoch`，返回sdk/device/build/clockId、boot标识（可取得时）、inputEpoch、native STARTUP/BOOT/steady域的bracket读数、sensor原timestamp和映射证据revision。实际callback即时native receipt观测须capture≤receipt且age≤250ms，root冻结同域再核capture≤freeze且age≤250ms，等号允许；差值checked。bracket只验证bridge数值/读序，不把“若干样本很接近”当硬件时钟语义证明。无法证明同域则准确unsupported，不扩大容差或用callback采样时间伪造capture。

同plan多root保持原sample/parameter身份与E输入，不以中途sensor callback换样。即使envelope已经在plan时冻结，native在每root准入仍须以当时的input-domain stamp检查capture age；不得用旧freezeNs冒本root刚验证新鲜。该检查原值写入有界root冷回执，超龄按既定暂停/时间政策，不延长sample或root期限。GPU不需要为检查新鲜度读回候选；scope内五角色仍共享该root同一冻结资格和E0。

JS number只接受非负safe integer timestamp，拆lo=ns mod2^32/hi=floor(ns/2^32)，原roundtrip精确；超过2^53−1已丢精度，不能事后BigInt补回。若SDK有可用sensor原生int64数据入口，可另实现同源adapter；本轮契约允许明确TIMESTAMP_PRECISION_UNSUPPORTED，不把超过精确域截低32位。旧NAPI三参不变，envelope保完整U64及parameter/input身份。SYNTHETIC_TEST域编号2由显式host测试clock证书建立，sourceKind2/label不得伪报SENSOR；OFF NONE域0、signal/time字段0，重力角仍走已批准V4控制。

hide/show、重启、sensor重新订阅、context/reset/input代变化使mapping资格失效；重新验证、首tick0及重新DOWN后才可继续。测试分别注入深睡时间域分叉、native clock错误/倒序、capture未来/receipt旧代、age250ms/250ms+1ns、Number.MAX_SAFE_INTEGER边界。设备验证是独立真实前置，不把本轮只读SDK盘点称传感器已获生产资格。

## 7. 四档资源、冷档与容量限制

| K/Kw | Bext/单域 | Ea/scope | 常驻payload B | Dq5 / G1+F4 |
|---|---:|---:|---:|---|
|192/64|146,944|8,577|79,311,824|569 / 2348|
|192/128|146,944|8,577|83,283,664|639 / 2628|
|512/64|172,544|8,897|148,615,376|614 / 2528|
|512/128|172,544|8,897|152,587,216|684 / 2808|

payload=V4保守基线＋7Bext＋物理frame扩393216＋三scope物理扩98304＋三份4Ea×128附属frame＋三份Ea×128scope附属＋source/quantum各4096×U64 origin scratch＋两256B root control＋input64 256B。不得将U scalar又加一仓质量。Bext/cohort/cert/aux为16B对齐，combined物理/附属段按实际SSBO alignment记padding，逐对象checked capability和同时分配另测。

full branch在V4 fullTrace基础再加20×128×256＋20Ea×128及input原件，合成V5 full总量/每block大小机器稿列清；其中descriptor/留存及padding明确计，不能拆多个GL对象就忘累计总量。T503给冷reader并发/TTL/End/预算机制可复用，**其55,410,688B及M2 Tprofile不是V5完整payload预算**；V5新capability/payload revision必须按全部state/物理/附属/control/input有效容量核启动前信用，未足WOULD_BLOCK，不能半根截谱系仍报COMPLETE。cold reader明示role/trial/branchEpoch/物理ordinal/aux sealed前缀与scope/token；旧reader对V5拒未知。

K512总活动上界119.4658203125单位，对5120床初态最大2.3333168029785156%；K192为44.7996826171875，约0.8749938011169434%。若仅新lift包且不merge，每包≤122333，上界各半。旧50%峰值及固定残余门不变，V5不能达该目标不是软件泄漏/测试阈值可调问题；manager另决定是否扩大库存表示/模型，不提高glyph或云浓度冒库存。固定8秒全回床也不承诺，层满/轨迹/预算真实WAIT必须报告。

## 8. 实施及独立验证接力

developer，不使用真机。前置V4实现和T508/T498时间gate齐后实施**完整writer矩阵**及所有stage/四binding/同源schema/accept/scene/cold/input adapter，默认OFF不切旧路由。不得交孤立lift函数或半个cohort patch即停；原生产endpoint/旧三参不改义。版本化生成器须逐TU统一，GPU shader/manifest/reader/cache字节hash/签包及ELF核对。必要非机检查完成即可交开发，不加独立真机门。

实际helper和真实GPU必要检查：七存储域offset无重叠；SOURCE普通/bulk八trialrollback；证书birth/merge/partial/full LAND退役；同总S不同顶origin、depth64/相邻合并/跨64片、同色多edge最小首错；所有q限制0/正数边界、lowest free/MAX-ID/gen fatal、T/eta/W/J不变；U/A按MEDIUM token、cycle标签精确、E0请求/E1一次、五role不得累加；复合操作第二片/最后片故障只留pending未封诊断；replay乱序/缺片/错physicalOrdinal/未来证书拒；成功SOURCE/Q1后Q2失败保前缀；wrong input/time/参数/老三参/假sensor资格拒。四档资源/readonly prewarm/错误清理/lease/timeout/late/Swap沿V4复用对应范围。

T513独立反例与整数预期可先消费T506、本工程manifest及既有oracle，不新增CPU优化/业务实现；产品未实现标NOT_IMPLEMENTED，设计算术PASS不记产品PASS。缺真实V4/时钟/资源时精确记录前置，各可独立项先完成。设备tester带“真机”标签串行核准确签包、真实sensor原timestamp/mapping、offered/request/grant/reason、预算/库存及实际root期限/30fps/系统像素，效果参数另由designer/manager裁决；失败先architect分流。

共用prompt：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用已有oracle。背景前景全部XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。33,333,333ns GAP/250,000,000ns root期限/120Hz quota/30fps目标不变。成功回原卡且本人行结束待验收；失败/关键缺证写未完项、复现原错、原件、尝试、不能继续原因、前置与接手角色，本人行看评论。

本卡工程约定范围完成、依赖和未标定/未验证域明确列出；未编码或跑GL/真机，不把依赖未实现、sensor未知或容量不足冒产品支持。
