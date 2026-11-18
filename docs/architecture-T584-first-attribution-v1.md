# T584：V5 FULL真实BASE锚点与完整证据语义

> 2026-11-18 T614补齐同dispatch入口切点：本文默认非prefix revision1须应用FULL CONTROL revision2例外；scope入口/原件/Meta2和具体opcode以 [T614修订](architecture-T614-v5-entry-cursor-v1.md) 为准，不再要求host插入invocation。

2026-11-18，architect，不使用真机。读取T571 1c7946b、现存T527/T535/T552正式机器表、m3b writer/transport/controller/cold manager，证据 `analysis-T584-source-evidence-v1.json`。保T571开发交付已登记；本轮是新缺口分流，不替其重开验收。

## 裁定

**执行过的root缺BASE不能标NOT_EXECUTED，更不能给FULL COMPLETE。** 未进入的root/role仅在有实际plan控制证据表明跳过时才NOT_EXECUTED，空体仍有全长度摘要；进入但未封为FORENSIC_INCOMPLETE，已执行而未保真实锚点为MISSING_REQUIRED_ANCHOR/INCOMPLETE。CURRENT不可逆推出旧BASE/source context，诚实承认缺对象不等合同允许缺实现。

当前工程正式FULL目录和生成header均为**61段**，不是58：prefix1+final2+SOURCE/physical/provenance frame3+control5+BASE pair10+scope pair40。T552固定索引11+2i/12+2i为五BASE pair，21+10q+2(r−1)为四Q×五role的physical，紧接provenance；未执行也保目录，不靠段数量猜执行事实。

## 源码缺口（静态，非本轮运行失败证）

1. FULL创建只留20个physical大小buffer和32768B控制块，未留5(S+D) BASE，writer直接将其标未执行。
2. 冷writer对20个slot按奇偶只选PHYSICAL或PROVENANCE一个名字，实际40段每对只填一半；transport只复制scopeRows×64word，不保完整aux。需每个role同时保两条流及共同sealed cursors，不能按slot奇偶分kind。
3. 仅四Q控制，遗漏SOURCE root0；6×128word复制不等于8192B完整CONTROL128/Meta。native fullMask/pending只说明host走到某提交点，不能独自证明GPU角色进入/完整封存，拒绝前缀/早错不能无声丢mask。
4. 所有容量行数被直接当sealed，缺planned/physical+aux共同前缀、rootRecord/nonce等；V1 header/capability还与正式V5_2冲突。
5. `Digest(out+s.begin,...)`把byte offset用于U32指针，应为checked begin/4；prefix需T535自身SHA掩码而非一般Digest。此处静态有越界/摘要错风险，不能冒现场已复现崩溃。
6. transport.cpp的拒绝stub与cold_capture.cpp真定义同时在STATIC library源码集；可能旧符号使新对象不被链接或产生多定义。须归一唯一实现并交编译/链接符号对象收据，不以两个文件都编过推writer实际绑定。
7. 每Controller持自己的TraceManager，单实例two slots不是全进程跨route two slots证明；SOURCE_FRAME、CONTEXT也必须从真实权威子段搬运，不拿opaque buffer号/容量推它们已完整。

## 工程修订与生命周期

FULL必须在新plan首根之前预约全部GPU/native信用，固定5个root槽：0 SOURCE、1..4 Q。每根`Begin`在任何state/source-context写之前，将真实BASE state和当时完整D快照复制到对应只读保留槽；source上下文按control96/dedup64/ledger16/rawBulk512/sourceMeta32完整字典，不能仅复制当前语义event stream。BASE在后续SOURCE提交/下一Q改写前已封身份，失败root也保此前真实前态，不从后态倒推。根完成时冻结原INPUT64、completion256、quota256、cursor64、ROOT_RECORD128和CONTROL Meta；SOURCE没执行须有实际跳过原因，不伪造其锚点。

每Q/role按T552入口/尾原cursor捕获physical和aux，保role/branchEpoch/nonce、p/a start/planned/sealed/end。scope双stream都完成才标共同prefix有效，pending body不能算valid；根早错保实际entered/incomplete，不把未分配或忘copy改成未执行。导出sidecar搬运字段不重算物理/来源，CURRENT仍只由原root事务发布。

使用正式既有额外GPU公式 **20*(256e+128Ea)+5*(S+D)+5*8192+256**，T552 cursor保留按其既有预算放入已声明控制/元数据区域；不能把现有20×physical大小误当已覆盖aux/BASE。五控制块共40960B，含SOURCE，替代仅四Q的32768B。每槽按机器表offset，所有byte→word转换/乘加先检查。无新增物理dispatch或热读回，buffer copy/屏障/分配必须单列实际成本；预算不足返回FULL_UNSUPPORTED_RESOURCE，不能降锚点或改名COMPLETE。四档原Tfull157537024/163853056/255871744/262187776B和全route双槽上限524375552B保持，max已知并存894457592B保排除项，不重算成RSS/设备资格。

capability/page/collector/reader统一正式T527 V2、payloadV5_2、header2/policy1、非prefix段revision1及每段原内层wire。删stub/唯一符号后实际header/prefix摘要、root索引、每段usedMask/内层seal、bytes与validRows都需写入；ROOT也不能延用旧V1无锚假完整。builder使用已预约caller-owned span，不再new一个同容量vector；只读页≤1024U32/16384UTF8、整行、TTL60s不续。

进程级ColdSlotBroker统一M2/M3/G9的两slot/单builder/reader信用，route/profile各自限额不抬；旧reader/DETACHED/GL-retiring未实际free仍计账，跨route不得各开两块绕过。slot和GPU保留各有RAII票据，逻辑取消先撤资格，owner完成unmap/restore/退休后归还；mutex内不GL/等待/文件IO。保旧published直到候选所有必需段验证成功，故障原件可标FORENSIC，不能覆盖旧可读完整档。

## 接手

明确为**工程实现缺口**，先按本稿修文档/唯一生成及链接路径，再由developer完整做真实锚点/paired流/root控制/descriptor与有界broker；不把缺BASE推tester“验证NOT_EXECUTED即可”。独立tester之后覆盖SOURCE空/非空、Q0..3真执行/跳过/失败、坏mask/缺paired段/早取消、跨route pin/TTL/最后一页/摘要与IO失败；本卡静态发现不等这些测试已跑。

完整遵循 [共同约束](architecture-T581-T589-handoff-policy-v1.md)，developer仅开发编译提交、不测试/不使用真机。
