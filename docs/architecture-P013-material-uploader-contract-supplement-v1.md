# P-013 上传器合同补充 v1（T67）

日期：2026-10-26。角色：architect。任务：6adea928e4b06ee357347c76。
状态：CONTRACT_DESIGN_COMPLETE / DEVELOPER_ADAPTATION_REQUIRED / NOT_IMPLEMENTED / NOT_DEVICE_VALIDATED。
不使用真机。仅工程裁定，不改产品代码、T53冻结期望或历史执行结论。执行session只维护自己清单状态，不改队列或滴答同名清单；完成登记归manager，无新增验收门。

## 1. 适用范围与权威关系

依据：[T53登记](tester-T53-g2-uploader-cases-registration-v1.md) §2 A04/A05、§4 C02、§10缺口；[T54历史部分报告](developer-T54-material-uploader.md)（8a07901）；[P1工程rev2](architecture-P013-p1-material-composite-v1.md) §4/§6/§7；[T45裁决](decision-P013-T45-material-rollout-v1.md) §3。本稿只将未具体化的基准/身份/码值变成唯一工程合同，不改400/401/402等既有枚举和冻结expectation。

用户已通知T54完成，原卡保持完成；历史部分报告的“整卡未完成/等三项才能开G3”只描述当时时点。本次设计交接供既有T55/T56与manager安排独立适配hunk，不重开T54、不重复创建上传器卡、不撤回已放行的已定材质实施。

本稿只裁定G2三个接口点。显示失败/背景前景/Canvas历史条款由独立 [纯新管线合同T68](architecture-P013-new-pipeline-only-revision-v1.md) 替代：后续所有上屏仅XComponent+shader，无Canvas上屏或故障退路。T67不另定义其渲染状态机。

当前只读源码：stage只校验descriptor自报revision合法整数，未与外部权威绑定；MaterialDesc.metadata是bundleId/paletteHash/generatorVersion简拼，不能表示完整内容身份；Stage未实现冲突；C++ Pump非法budget抛invalid_argument，NAPI/ETS抛CONTRACT_PENDING。这些是待适配事实，不是本稿已实施结果。

## 2. A04唯一配置基准：已绑定的目标配置，不是旧active

### 2.1 配置绑定控制面

新增唯一控制入口（现五个Stage/Pump/CanActivate/Activate/Discard签名保持）：

```text
bindMaterialConfiguration(binding: P013MaterialConfigBinding): P013MaterialConfigReceipt

P013MaterialConfigBinding:
  sessionId: safe integer
  appearanceConfigRevision: nonnegative safe integer
  configurationDigest: SHA256 of the accepted immutable desired configuration
P013MaterialConfigReceipt:
  code, sessionId, bindingId, appearanceConfigRevision, configurationDigest, errorStage
```

绑定由既有appearance合法域校验完成后、资产生成/stage之前的controller配置协调器显式调用；native owner/mutex下自有复制并分配session内单调bindingId。digest来自用户接受的完整目标配置（含palette/uniform/fog等），不是由待校验desc回填的“expectedRevision”。具体完整配置结构复用已有单一appearance schema，不另造参数面板；digest须按确定性完整字段序列化，不能hash对象地址/文件名。

- **Stage唯一比较：**`desc.appearanceConfigRevision == boundTarget.appearanceConfigRevision`。旧active.revision可以不同；pending、descriptor自己的副本、UI草稿和最后一次draw都不是基准。
- 未绑定时Stage返回401 ASSET_NOT_READY、state=FAILED、errorStage=configBinding；绑定缺字段/非法类型返回400，原绑定不动。绑定本身不上传、不激活、不改变当前画面/UI active配置。
- 同revision同digest重复绑定幂等；同revision不同digest返回402/errorStage=appearanceConfigRevision，原绑定保留；不同新revision可建立新目标。revision是协调器已接受的身份，不允许为了通过校验临时改desc或从desc自动绑定。
- rebind只改变desired目标，不销毁active，也不立即破坏已经准备的pending。旧pending可保留供诊断，但其bindingId与当前目标不匹配时canActivate=false、activate返回402；新stage才按正常覆盖规则替换pending。
- 每个成功stage捕获bindingId/revision/digest；从绑定到激活间不能改其配置对象。active代可以在新目标上传时继续用自己的完整旧配置绘制，不能把新UI参数塞进旧代。

### 2.2 A04验收场景

active revision=10；先显式bind目标11；stage(desc revision=11)合法，不因与active10不同报错。仍绑定11时stage revision=10或12，返回**402 ASSET_REVISION_MISMATCH / FAILED / appearanceConfigRevision**，uploadedBytes=0，active与现pending内容/进度不变。

T53 A04 fixture须先绑定其权威目标，再仅改descriptor的revision作反例。缺绑定是独立未就绪场景，不可用“缺基准401”假称A04→402已覆盖。测试中的binding值必须来自独立输入fixture，不得在被测Stage内由desc抄入。

cold/init/release创建新session会清绑定；同session context重建保留CPU目标绑定但必须重新验证实际epoch后恢复资源。配置取消由协调器绑定正确的新目标，不偷偷把目标换回active来凑码。需要回到历史外观时由协调器分配新的合法配置revision，避免同revision异内容。

## 3. A05唯一身份冲突与重试/恢复规则

### 3.1 身份域与内容指纹

上传器资源主键为 `(sessionId, assetGeneration)`。generation是session内正安全整数，新代必须大于已成功接纳的generation高水位；Stage校验失败不消耗generation。materialBundleId是该generation绑定的非空资产标签，不是文件名hash，也不单独充当全局永不重复的数据库主键。

每个驻留代（active、pending、允许恢复的CPU镜像）保存materialBundleId与完整contentFingerprint。指纹由重新核验的grain/strata SHA256及**所有影响内容/配置的不可变metadata**生成：七版本、尺寸/层数/编码、appearanceConfigRevision、paletteHash、grainBaseSrgb、renderSeed、geometryEpoch、生成器/生成参数、两域有效lambda、warpRange等。不包括可变surface/context target与frameToken；这些另按epoch/帧检查。绑定digest/bindingId也随代保存，不用当前简拼metadata字符串冒充全配置身份。

规范化必须按固定字段顺序和类型编码，数组逐项、数值有限且固定二进制/规定数字编码，字符串长度明确；不能依赖属性遍历顺序或用无转义冒号连接。fingerprint用于一致性，不替代对真实字节的SHA256与alpha/长度检查。

### 3.2 唯一判定表

| 已知状态/请求 | 唯一处理 |
|---|---|
| 驻留同generation，但materialBundleId不同 | **400 MATERIAL_PACKET_INVALID / FAILED / identity**；不替换任何资源 |
| 驻留同generation同bundleId，但实际字节hash/完整metadata指纹不同 | **400 / FAILED / identity**（若连字节与自报hash不符先报hash400）；不能把同代内容覆盖当新资产 |
| 驻留同generation、同bundleId、同完整内容、同target/binding | 成功幂等：返回该代实际CPU_STAGED/UPLOADING/READY/ACTIVE；不重新上传、不得降低进度、不分配新纹理或丢弃pending |
| 新generation>高水位、descriptor与当前绑定一致 | 正常新stage；同bundleId可复用逻辑标签，但仍是独立新代，不能复用旧mutable配置指针；须全量校验 |
| generation≤高水位但代已退役且没有驻留CPU身份 | 400/identity，拒绝复用旧generation；调用方用新generation；不建立无限历史身份表 |
| context重建、驻留CPU代内容不变 | 通过原Restore控制流程按新surface/context重建pending、重传到READY；允许同generation同内容恢复，不当作A05冲突；不能沿用旧GL handle |
| context重建时擅改同代bytes/metadata | 400/identity；恢复不是改同代内容的后门 |
| surface/context不符，即便内容相同 | 按A10/C04各自原期望处理；不能拿“幂等”绕过epoch |

常驻身份记录只需active+pending/其原CPU镜像与高水位，遵守双代上限；不新增无界bundleId历史缓存。release新session清身份表和高水位；sessionId不同不可混用，不借新context reset generation计数。

成功新stage覆盖pending时，旧pending明确DISCARDED，资源释放；必须先完成新包所有校验和自有复制才替换。任何Stage失败保留旧active与旧pending，不通过污染旧配置来实现FAILED回执。回执assetGeneration是本次请求，activeGeneration/pendingGeneration仍是实际驻留身份。

## 4. C02非法byteBudget的唯一结构化回执

唯一合法域：JS number类型的有限正整数，1≤byteBudget≤65536；缺参/额外参数、NaN/Infinity、负/零/小数/65537、字符串/bool均非法。整型转换只能发生在验证之后，不能先size_t截断再通过。

非法域返回：**code=400 MATERIAL_PACKET_INVALID，errorStage=byteBudget，uploadedBytes=0，materialUploadBytes=0**。不再抛CONTRACT_PENDING/RangeError或C++异常给普通参数错误；这是既有400包/调用域校验的具体化，不新增状态码，不修改T53已冻结的“拒绝/零上传/状态不损”期望。

| 项 | 必须保持 |
|---|---|
| receipt.state | 本次被观察的pending原状态；无pending时active原状态；两者皆无时沿空槽DISCARDED。不是把真实pending置FAILED/DISCARDED |
| 身份/进度 | active/pending generation、appearance revision、uploaded cursor/剩余字节、已上传纹理内容不变；receivedBytes/pendingBytes等反映原实际状态 |
| 计数/资源 | materialPumpCount、纹理live/CPU bytes/配置绑定均不变；无分配/GL上传/GL解绑/资源丢弃 |
| 无对象时 | generation可为既有空槽0，不伪造active；随后合法调用按既有401 ASSET_NOT_READY处理 |
| 之后合法budget | 从原pending进度继续，不能因前次参数错永久FAILED |

参数域校验先于owner/epoch/GL动作；因此“非法budget且无pending/epoch已变”仍先回400/byteBudget，且本次不做epoch丢弃。随后**合法**pump才执行C04的epoch失配DISCARDED。owner/thread错误不授权读写共享live状态；实现可用受锁的既有只读receipt快照填状态，不能为非法调用抢GL current。

合法但小于当前一行（例如budget=1且当前grain行1024B）不是非法：code0、uploadedBytes0，游标不变，之后budget1024可推进；不上传半行，不放大预算。是否记录一次合法pump调用沿原materialPumpCount定义，不把这个0字节合法调用与非法域混同。

## 5. 校验顺序、激活与适配所有权

Stage按单一顺序：typed descriptor/必填/七版本400→session与surface/context A10校验400（owner系列保留原码）→绑定存在401与revision mismatch402→真实byte range/长度/hash/alpha400→身份冲突400→自有复制→替换pending。只要求独立反例命中其指定项；混合坏输入有唯一首错，不能靠改优先序让错例虚PASS。

CanActivate必须同时满足：pending READY、generation与当前context、捕获bindingId/配置revision仍为协调器desired目标。Activate原子绑定同FrameToken的纹理/完整配置；未READY→401；READY但目标配置已换→402；old epoch沿现epoch合同，不把“canActivate=false”统一包装为成功。metadata类型错误仍400，不因含revision字段就都算402。

| 实施文件 | 最小适配 |
|---|---|
| P013MaterialTypes.ets / lib native声明 | 增bindMaterialConfiguration强类型控制面；receipt保留原字段并补requested/bound身份诊断，无any绕过 |
| P013MaterialController.ets | 显式绑定已接受目标配置，记录binder返回；非法pump返回结构化receipt；不从desc自动绑定、不执行Canvas fallback |
| napi_bridge.cpp | 独立绑定入口+参数解析+owner/mutex；A04传外部bound目标到core，C02无GL参数拒绝 |
| material_uploader.h/.cpp | 完整fingerprint/驻留身份+高水位；幂等stage与受控Restore；Pump非法域不抛异常；失败无资源污染 |
| T55/T56配置协调器 | 消费已绑定目标并同代激活，G3已定shader工作继续；背景前景/fault契约交T68，不重做T54上传器 |
| developer定向测试 | A04独立绑定基准、A05同代异ID/异内容/完全幂等/已退役重用、C02非法域与后续合法pump、restore与旧active保全 |

反例清单：目标11/active10/desc10→402；同generation异bundleId→400；同identity异自洽字节→400/identity；同字节但未改hash→400/hash；完全重stage进度不退；重复C02错误不累计pump；budget1合法0上传；合法budget+epoch失配仍DISCARDED；恢复CPU不可带出旧纹理。原T09、T53 A01..A10/B01..B04/C01..C04/D01..D03期望保持；E类及B04 Canvas部分由T68版本化替代，不由T67偷偷改checker。

## 6. 交付边界与修订

这是唯一工程合同，不是实现或测试通过。所裁定三项都能从既有码/作用域落地，无需改变材质数学；若实现发现与签收算法/固定测试正文真正冲突，保留证据反馈designer/manager，不按本稿擅改判线。用户已完成通知的T54保持完成；后续适配工作由manager沿已有T55/T56或单独hunk派工，不另加验收门。

本次仅新增本文。未改产品/测试/历史报告，不运行T54执行器覆盖其报告，不使用设备。绑定/身份/预算三项精确合同与T68纯新管线显式引用，各自独立，不重定T68的FAIL/STALE语义。

修订记录：2026-10-26，T67 v1，明确A04已绑定目标revision→402、A05session内generation/完整内容冲突→400、C02非法budget结构化400/零上传/原状态不损；保留冻结期望，不以凑码替代真实输入对照。
