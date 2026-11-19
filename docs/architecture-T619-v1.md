# T619：V5 公开身份和完整配置编码闭包

2026-11-19；architect；不使用真机。结论：ENGINEERING_READY，交原 T596 续实现；未运行产品、reader、构建或设备测试。消费 T596 b127521、原评论 6afd8837e4b0dc337e97c48d、developer-T596-progress.md 和 development-t614.json。T596 拒绝用零填未知身份是正确处理。

## 唯一正式表与版本

`architecture-T619-v5-public-identity-v1.json` 是本次 rootCursor64、rootTokenSha 和 fullColdConfigCapsule 的逐 word 权威；T527/T552/T614 表均显式引用它，T535 的摘要算法继续生效。仍为 V5_2/header2/policy1；这是原先未定义区域的首次有效编码，不把历史不完整文件追认为 COMPLETE。reader 必须要求 cursor/capsule 各自 magic/revision/seal、保留零和 token 重算一致；缺字段拒为 IDENTITY_CONTRACT_MISSING，未知版本 UNSUPPORTED_IDENTITY_REVISION，不猜测、不兼容补零。

prefix 仍 32768B。cursor5x64 在字节[20736,22016)，5×256B；root records 在[24192,26752)，5×512B；capsule 在[26752,30848)，4096B；[30848,32768)全零。ROOT 使用一个实际根，其余未使用槽清零；FULL 仍 SOURCE+Q0..Q3 的五槽。NOT_EXECUTED 的 root 按 T552 flag=1 保留具名状态，cursor/input/quota/completion 无样本槽按既定零规则，token=0 仅代表未执行；FORENSIC 缺真实身份不能冒 COMPLETE。

## rootCursor64 的 producer 与切点

它是**冻结的公开 root permit/入口 owner 快照**，不是 T614 私有 scope cursor32，也不是该私有数组前64word。完整64word、Lo/Hi顺序、usedMask 两个U32、每字段producer见机器表。所有整数LE32，U64低word在前。入口需保存实际 RootPermitV5 和 RootOwnerV5；在同代 owner 授权校验后、首个该root业务 dispatch 提交前捕获 ownerBefore，在首dispatch确实提交后才设置 ROOT_ENTERED。若提交前被取消，不能用该候选当已进入root；保未进入或 forensic 原因。

`sourcePrefix/dedupPrefix/eventEnd/currentBank/actualBank/eventFrame/lastGood/stale/publicationSlot` 均来自 ownerBefore，lastGood/stale是boolean U32，不能拿完成后的 owner 反算。工程接线在ControllerV5::Advance持mutex且BeginRootLocked成功后、离开锁交TransportV5::Begin前保存owner字段与ColdScopeLocked原身份；最终Authorize再次校验同代才允许提交。内部SetCaptureRootIdentity类setter可新增，不改外部endpoint。保存位置属于已有 CaptureRoot 的固定metadata，须计入T596已单列的fixedCaptureMetadataBytesExcludedFromPayload，不默默占业务/诊断信用。实际为5 retained+pending+accepted共7实例，每份cursor64+原header身份18word，原字节增2296B，允许结构对齐增量上限2352B，按真实sizeof差额计账；不得只算5份。配置变代清空未封存候选，不能拿新config填旧acceptedRoot。

快照不新建完整bank副本，不增加GPU读回、dispatch、fence或hot map。T614 每scope入口/终点配对40次、5120B复制规则不变；它负责scope消费身份，本表负责root入口授权身份，二者不互相替代。

同一root完成后，将其 cursor、原始 INPUT64、quota256、completion256 和实际 RootRecord0..29 合并封存，再计算 token。根中断时可以输出FORENSIC及真实已捕获区，缺任一区则 token不能假冒完整承诺。ROOT快照若未保留入口cursor，不能从末尾control恢复，必须报告 FORENSIC_INCOMPLETE。

## rootTokenSha 字节前像

精确长度2920B，730个LE32：domain magic `0x31545235`、revision1、header[24:42]的18个原scope身份word、rootCursor64、INPUT64、quota256、completion256、RootRecord[0:30]、parameterSha8、shaderSha8、layoutSha8、angleSha8、configCapsuleRawSha8。最后8word为完整4096B胶囊原字节SHA256的LE装载。按此顺序逐字节流式SHA256；不含C++padding、JSON、字符串终止符、host endian、traceId、导出时刻、段offset、自身token或prefix SHA。摘要原32字节每4字节LE写RootRecord58..65，hex打印按原摘要字节序。所有来源都是同一被冻结root的原件；header身份必须取根保存时的pid/epoch/run/context/surface/pageSession/resetSerial/permission/inputEpoch，不得使用导出时当前scope。shader/layout/angle SHA取permit，parameterSha取冻结lift参数身份并与record42..49/capsule核一致；token同时绑定全配置而不只绑定lift.sha。

这是封存证据的承诺摘要，不是授权密钥、签包证明或物理PASS；篡改input/count/完成结果/入口scope中的任一位均须改变token。它绑定结果，因此不同真实完成切点不能强求相同token。writer顺序：冻结原件→填root/capsule→token→非prefix SHA/descriptor→T535固定mask canonicalPrefixSHA→rawPrefixSHA→不可变发布。reader按同样前像重算，不能从token倒推出未保存身份。

## fullColdConfigCapsule

1024U32全部确定，word70..1023保留零。物理参数保存**实际配置整数**，不选新默认、不倒推uniform、不把模型Q20称为SI标定。omega为角表index/s；waterRate为Q20/s；gravity Q20/s²；settle/emit/liftSpeed为Q20/s；kAir/kFlight/kWet为整数s^-1；soft为Q20列高差阈值；flux为每边固体搬移上限Q20/s；liftRate/ramp/decay为Q16/s；band为Q20高度；liftCap为Q20/s。依据T237/T497/T506/T534及实际uniform绑定，emit实际供SOURCE的uM3EmitQ消费。

word69记录effectiveWaterKappa：当前TransportV5未绑定uKappa，实际既有策略为自有新program的link初始化0，不把T534候选24写成已生效。writer须由自有program参数初始化/绑定账明确保证该0策略；未知或代码改为别的策略须拒配置身份而非猜零。本卡仅登记现状，不启用新水仿真或替换物理参数；后续调参需按正式设计版本化。

胶囊另存实际K/Kw、wire/features、参数revision/mode、omegaProvided/fullBranchTrace、固定120Hz和65536tick、GAP33333333ns/root250000000ns，以及layout/shader/angle/parameter SHA。parameterSha沿现有lift.sha的命名含义，不伪称所有物理数值的SHA；全胶囊字节由prefix canonical摘要覆盖。相同lift.sha但其它配置不同仍保真实数值，不能因此宣称同配置。word0..68字段范围/有效性与机器表一致，actual config.Valid 为必要条件；不可从UI当前值补过期root配置。

## 续作与复用界

原T596 developer：按此表补实际入口快照、完整writer/reader同合同、strict拒旧/保留零、完整capsule和流式token，消除 PREFIX_IDENTITY_CONTRACT_REQUIRED；保持既有共享broker/真实释放退信用/跨DSO信用/两slot单builder单reader、原容量与TTL，更新生成链不覆盖本合同。开发完成编译通过即可，不要求另卡设备/独立测试作为关闭门。产品COMPLETE能力需实现后才能声明，当前只解除编码合同前置。

G13可复用SHA流式工具、冻结/发布/credit生命周期；不能复制V5 prefix32768/64cursor/rootToken前像到G13独立65536 prefix、MAC字段或其摘要合同。旧湿M1仍暂停，水线13→14→12，CPU停投，全部上屏仅XComponent+shader；失败FAULT或显式STALE，禁止Canvas/软件GL产品回退。

执行prompt共同约束：不使用真机。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不得更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部分支受阻先完成其它可做部分；阻断回执须含未完成项、原始错误/复现/证据/尝试、不能自行继续原因、所需输入和接手角色，原卡评论并本人行看评论；成功结束待验收。
