# T535：V5 cold prefix 摘要合同修订（V2）

> 2026-11-19 T619补齐prefix内公开身份/配置：按 [T619](architecture-T619-v1.md) 及其JSON先冻结capsule和root token，再执行本稿固定mask摘要。mask[1116,1148)、32768B范围、header2/policy1及预算不变；token包含胶囊原SHA而不含prefix SHA，不产生循环。reader不得用补零接受旧未定义身份。

> 2026-11-17 T552补充生效：本文“非prefix沿T527”原先未给数值的部分现已由 `architecture-T552-v5-segment-wire-v2.json` 及 `architecture-T552-v5-wire-and-g9-seed-v2.md` 唯一补齐；按新逐段身份/计数/捕获元数据实施。T535固定自身SHA mask、版本及预算规则不变。

日期：2026-11-17。角色：architect。不使用真机。状态：ENGINEERING_READY_NOT_IMPLEMENTED。

本稿消费 T517 `7a02d77`、`developer-T517-v5-implementation.md` 和 T527 正式表。T527 的 prefix descriptor 覆盖自身 SHA 而未定义 canonical 输入，是工程合同遗漏；T517 拒绝自行选择算法、以 `PREFIX_DIGEST_POLICY_REQUIRED` 暂停完整 cold 是正确处理。容量 helper 和局部编译均不等于完整 capture。本稿解除该合同前置，不声称 T517 已完成。

## 1. 唯一版本与唯一摘要输入

更新后的 `architecture-T527-v5-cold-manifest-v1.json` 是原路径下的 **ENGINEERING_V2**，文件名保留以免悬空旧引用；本稿优先于 T527 的旧文字。magic 仍为 LE `V5C1`，header[1]=2、header[3]=0xC0DE0502、header[15]=1（`SELF_SHA_ZERO32_V1`）。capability=`T527_V5_BOUNDED_COLD_V2`，payload=`GRS_M3B_LINEAGE_COLD_V5_2`，page=`T527_SEGMENT_PAGE_V2`。物理 wire5、原 INPUT64、事件、state、root record、descriptor32 布局及非 prefix segmentRevision 不变；descriptor seal 仍 0xC0DE0501。

prefix 必须是 descriptor **0**，固定 descriptor 起点1024B、长度128B，所述数据范围恰为 `[0,32768)`、strideWords=1、segmentRevision=2，valid/planned/sealedRows 均8192，SEALED_VALID。它的 SHA256 words23..30 恰占归档字节 **[1116,1148)**。不准根据不可信 descriptor 内容动态选择 mask，不准移动、重复或别名化 prefix descriptor。

prefix descriptor kind=1，其rootIndex/role/trial/branchEpoch/arena/generation/physicalFirst/Last/auxFirst/Last全0，表示全档prefix而非某root事件；flags只能SEALED_VALID=2。所有ROOT/FULL均如此，不能根据role/trial换mask。非prefix种类/role仍沿T527对应目录和语义。

唯一 canonical 摘要：`C = SHA256(P[0:1116] || 32个0字节 || P[1148:32768])`，P 是最终待发布的32768B原始 LE 字节。只虚拟归零这32B，不排除整个 descriptor、不清空 seal、不清空其它 SHA、不依据 host endian 重解释。digest 的连续32字节按四字节 LE 装入 descriptor words23..30；打印十六进制为原 digest 字节序。其它所有 descriptor 的 SHA 都是 **所述完整 byteLength 原始字节** 的 SHA256，含段内规范零填充和 NOT_EXECUTED 零段，不能只 hash validRows。未使用 descriptor 的32word全0。

此 C 是 `canonicalPrefixSha256`，不是实际 prefix 文件摘要。`rawPrefixSha256=SHA256(P)` 必须另算、另命名。G9 T529 的“不为 prefix 建自 descriptor”是另一独立协议，不移植、不混用。

## 2. 构造、封存与发布顺序

1. 沿原 T527 请求前信用、scope、两 slot/单 builder/reader 和 FULL 单 GPU 保留集预约。ROOT 等真实 owner 安全点；FULL 在完整 plan 前预约，保五 BASE_STATE + 五同代 BASE_SOURCE_CONTEXT、五 control、20 paired 流及最终 state/context，NOT_EXECUTED 显式保零。不造旧锚点。
2. 候选 block 全部初始化为0；写完整段数据，checked 检查 offset/length/stride、planned/valid/sealed 和 role/trial/root/ordinal。FORENSIC_INCOMPLETE 与 COMPLETE 分开，不把 pending 封成成功。
3. 对非 prefix 段计算原始 SHA，填 descriptor，填固定 descriptor seals。冻结 header、root records、完整 cold config、实际页数、scope、声明包/ELF/layout/shader/参数 SHA、结果 flags 等所有 metadata。header seal 是常量，不是另一递归 checksum。
4. 写 prefix descriptor 的固定元数据与 seal，header rev2/policy1/seal；自身 SHA32B暂为0。全 block 此后物理内容不可变。计算 C 并且只写自身 SHA32B一次。再计算 rawPrefixSha256，核验 canonical 复算、完整结构及各段 SHA。
5. owner 再校原 scope/撤销/TTL/GL restore/unmap 结果后，才以 release 语义发布 TRACE_READY；reader acquire 后只读。发布状态、pin、内部引用计数在 payload 外，不再回填 payload。保存失败/超时销毁候选并保旧 READY；不得为导出重写业务 firstCause。

## 3. ROOT/FULL 页与外部文件身份

page0 有界摘要必须同时写 capability、payload、pageRevision、headerRevision2、policy1、mode、K/Kw、trace/request、完整 scope、actualBytes、segmentCount、actualDataPages、canonicalPrefixSha256、rawPrefixSha256。二者来源于同一个不可变 slot。随后的 prefix 普通数据页包含实际 SHA 字节，不发送归零后的 prefix。

完整数据页仍 segment 内整行≤1024U32；每页绑定相同 trace/request/scope/mode/profile/revision，含 segmentIndex、byteOffset、stride、firstRow、rowCount 和精确页序。ROOT 为7段、FULL为61段（都含prefix）；不能把ROOT改个mode当FULL。reader依据 profile机器表生成唯一页目录，不接受缺页、重页、乱序、跨段、半行、checked乘加溢出或页面身份漂移。

归档采用有界逐页 UTF8 JSONL：每条 page（含末尾 LF）≤16384B；最后单独一条外部 manifest（含LF）≤16384B，schema=`T535_V5_ARCHIVE_MANIFEST_V2`。manifest 不放入32768B prefix 或任何被它描述的段。它包含上述同一身份、精确页数/原始payload字节数、canonicalPrefixSha256、rawPrefixSha256、`payloadRawSha256`（按原始offset顺序拼接真实LE段字节，含prefix自身SHA）、`pagesFileSha256`（只覆盖最终manifest之前逐页落盘的全部原始UTF8字节含LF）、实际页文件字节数、COMPLETE/FORENSIC_INCOMPLETE及签包收据引用。所有摘要使用64字节ASCII小写hex。不能把每页hash列表塞进有界末条manifest；用逐页连续hash实现同等文件完整性检查。

`finalFileSha256` 仅放文件以外的导出回执/签包关联记录，覆盖最终manifest也落盘后的整文件；不把它嵌入自己覆盖的文件制造第二个循环。collector 先临时文件逐页write/hash，完成所有页、reader结构检查及末条manifest后fsync/close，才rename并出完整回执；I/O、NAPI、最后manifest或rename失败保 INCOMPLETE，finally End/unpin。慢读不续60s TTL。SHA仅完整性定位，不代替真实签包/设备加载来源或物理正确性。

## 4. reader 顺序和拒旧

先核版本组合/精确mode/profile和有界目录，再核 reserved0、固定prefix descriptor位置/范围/计数/seal、各段不重叠且恰覆盖原块、所有whole-row边界；未知先拒，不能先大分配。以固定mask流式复算 C，与descriptor所存SHA和page0/末条manifest三方一致。随后比实际raw prefix SHA、所有非prefix段原始SHA、payloadRaw、页文件SHA、页数和外部回执身份；任何一项不同拒收，不能把 canonical C 当 rawPrefixSha256。

V5_1/header1/缺policy/policy非1/伪升版而保旧页串/错误mask一律 `UNSUPPORTED_V5_DIGEST_POLICY` 或相应完整性错误，不自动补零、猜测或降级。V3/V4线上适配和离线reader独立保留；不把V5_2解释为旧版。旧共享脚本 `analysis-T524-T527-engineering-checks-v1.py` 是历史V1生成器，不能再用其覆盖本次修订表；本轮检查只读修订表。

## 5. 不变预算、有限检查与接力

四profile的ROOT/FULL offset、容量、61/7段、descriptor64槽、32768B prefix、SOURCE_CONTEXT 10,224,000B均不变。最大两FULL slot 524,375,552B、已知并存payload 894,457,592B、归档上界1,049,837,568B、page16KiB、两个slot/单builder/单reader/单FULL保留集和60s TTL不变；不借诊断4MiB信用。新字段占原word15，归档末条沿原一个manifest信用，无新增整档副本。hash流式固定32B状态/小块，不能再复制整block或整档到JS。250ms root、33,333,333ns GAP、120Hz配额、显示30fps与CPU停投不变。

工程有限正反例及四profile表一致性见 `analysis-T535-T543-contract-checks-v1.py` / `analysis-T535-contract-checks-v1.json`。它们检查摘要数学、结构及兼容拒收，不运行产品测试、不证明真机性能。

交 developer（T517原卡接续）可执行prompt：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅用户约定的本人行状态例外），不更新滴答同名清单。按T535与修订T527实现完整ROOT/FULL、五BASE/SOURCE_CONTEXT锚点、collector/reader/诊断adapter及跨route共享credit；更新既有generator避免旧脚本覆写。保四SSBO与新XComponent+shader唯一上屏，失败FAULT或标记STALE上一有效完整帧，禁Canvas回退。开发仅开发/编译/提交，不执行测试、不使用真机；不把tester/effector作为开发关闭前置。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部分支受阻先完成其它可做部分；具体阻断写未完成项、原始错误/复现/证据/已尝试/不能自行继续原因/所需输入与接手角色，原卡评论并本人行看评论；成功结束待验收。manager登记后直接推进，不新增manager门。
