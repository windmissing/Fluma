# T621：M3/G9 typedarray 长度单位归因与适配合同

2026-11-19；architect；不使用真机。结论：当前实际reader将原始length无条件当元素数；原件呈4倍关系，最强候选是平台导出接口的字节单位。需要绑定真实provider语义后修产品reader，不能泛化成typedarray不兼容。T582安全诊断PASS保留，未运行测试。

## 实际拒绝点

输入 test-report-T590-m3-g9-firstreject-v1.md、test/reports/t491/t590a四个first/last及 `.codex/local-test/t582-g9-final` 实际绑定与GpuInputAttempt.ets。必须解析外层snapshot字符串，再核call identity；不是仅看UI长度。

| route | API | 预期元素/字节 | 原生原始length | kind/offset | 首拒绝 |
|---|---|---|---|---|---|
| M3A | m3Configure | 16/64 | 64 | uint32(6)/0 | ELEMENT_COUNT |
| G9 | g9ResearchConfig64 | 1/4 | 4 | uint32(6)/0 | ELEMENT_COUNT |

cbInfo/isTypedarray/typedarrayInfo均成功，typed=true、dataPresent=true；detached/backingBytes/语义检查尚未执行，不能称其已通过。G9的N1是实际seedRecipe请求[2]，API名字含64不代表当前参数应改N64。旧M3 schema/words语义也不能通过改变N来绕过。

华为官方接口说明区分标准/native接口的元素数与HarmonyOS导出接口的字节长度；这直接解释可能的4倍关系，但原件尚缺实际解析provider/interop形态证据，不能由文档推断当前二进制必然使用该形态。[官方 napi_get_typedarray_info 差异](https://developer.huawei.com/consumer/cn/doc/doccenter-references/api/napi/)

## 产品工程修订

在实际打包绑定共用一个有明确SDK/运行时/provider profile的只读decode适配层。profile固定lengthUnit=ELEMENTS或BYTES并有来源收据（编译interop模式、SDK版本、解析provider模块/符号或同provider能力探针）；未知为UNKNOWN明确拒绝，不能每个请求“length等于N或4N都收”。BYTES先checked验证整除sizeof(uint32)，再normalizedCount=raw/4；ELEMENTS直接使用raw。profile只在真实boot/provider变化后失效重建，不让请求自报单位。

顺序：argc/类型→typed kind严格uint32→取rawLength/data/offset→按已资格profile规范化→normalizedCount==N→非detached→真实backing范围与offset/指针一致→复制N×4字节一次→原revision/features/hash/reserved/模式等语义。现有合同offset0/整buffer精确N×4继续严格；禁止多收尾字节、拿byteLength当word数、提前解引用、异常吞掉后用零配置。

诊断V2新增rawReturnedLength、lengthUnit、normalizedElementCount、providerProfileId/证据SHA、backingBytes和逐API原status；未执行字段仍NOT_EVALUATED，首因锁存。历史V1 elementLength原数值保留，不重写成16/1。first/last每次同api/bootStamp/diagGeneration/attemptId自动四件仍沿T582，safe export不可因修解码退化。

developer最小范围：共用decode及实际生成绑定/诊断schema，维护生成器同源，编译通过交付。不要由此顺带改INPUT64物理协议或M3/G9参数。G13若复制同reader模式需静态审计并按同provider合同接入，但不能宣称未测G13已复现相同拒绝，更不把批量迁移设为T590续测门。

## 精确tester接力

签名部署/运行时provider和HAP/实际ELF闭环由tester取证；用同provider Uint8/Uint16/Uint32/Float32的1和3元素能力夹具记录rawLength、backingBytes和偏移，支持Sendable时另具名，验证绑定profile。错误kind/短长/非零offset/detached能力若可构造均保原拒绝，不用假输入冒实测。profile不能确认时交回具体模块/符号/SDK缺证，不盲除4。

随后T590原卡用真实请求自动first/last四件核首拒绝是否进入后续语义；配置接受只是接口层通过，真实compute/守恒/效果仍独立测试。T582安全诊断无需重判PASS；不要求developer先跑设备。

续作prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。阻断写未完成项、原始错误/复现/证据/尝试、不能继续原因、所需前置/接手角色并原卡评论。developer不使用真机，测试者真机串行。GPU驻留优先、CPU停投、XComponent+shader唯一上屏，故障FAULT或显式STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12，旧湿M1暂停。
