# T581：M3/G9旧绑定入口与首拒消费链

2026-11-18，architect，不使用真机。输入T545-m3-entry-v3、T542-g9-entry-v3及T579准确签包/生成树；报告将本问题写成T580的地方只保历史，T580仍属于G12资源。证据 `analysis-T581-source-evidence-v1.json`。

## 已证与未证

T579-m3diag来自t562-m3-diagnostic，G9来自t562-g9-diagnostic，实际HAP和包内ELF已与delivery.json逐字SHA核准。原件/报告支持重复点击后进程存活、UI受控显示拒绝；这是原崩退窄域保留，不能推断NAPI具体API/typedarray/同因/物理或四档期限已证。

生成app_bridge的property table确实将m3Configure/g9ResearchConfig指向本TU旧函数；两拒绝仍`text(...,"M3_CONFIG_REJECTED")`/`G9_RESEARCH_CONFIG_REJECTED`裸token。共享ReadWords及gpuInputDiagnosticsV1已经链接/注册，但页面只展示“可读诊断”指引，并未取得同attempt的UI原数组→native原快照→首拒文件。故为**已证工程接线缺口**，不是让tester再猜typedarray原因。

## T582实施合同

修改真源 `test/t522/bridge_overlay.cjs`、`test/t529/bridge_overlay.cjs`、`test/t550_words_contract_overlay.cjs` 及实际canonical bridge/页面生成模块；以生成后的property→函数→ReadWords→Record→返回路径做绑定矩阵。不要只改未绑定的napi.inc，也不增加第三套带后缀但UI不调用的endpoint。M3A/M3B/G9的实际属性名/预期N和route必须同表生成，旧研究N2、M3 N16和后续Config64各自独立，不拿一种长度推另一种。

既有入口每个普通失败均返回 `GPU_NAPI_ERROR_V1`完整对象，含api/route/bootStamp/diagGeneration/attemptId（U64字符串）/stage/reason/napiStatus/VALID或API_FAILED或NOT_EVALUATED。ReadWords成功后的revision/features/profile/full/cold等语义拒绝也须加入该attempt，不能只记录NAPI成功后又裸退。成功对象保原revision、实际状态及同attempt关联；session未建立禁止启动frame循环。保严格uint32/元素数/offset0/同步copy/真实detached和backing检查，不改NumberArray fallback或忽略API错误。

`gpuInputDiagnosticsV1(route,id)`精确查询lastAttempt或firstFailure中**同完整身份**的原件；当前代码显式id只找firstFailure，需同时匹配last，确实已被两槽替换才SNAPSHOT_EVICTED_OR_NOT_FOUND。不得将另一个last当同id返回。重置计数后仍用boot/diagGeneration区分，route串不互用。

UI在调用前保同步不可变U32原词、length/byteOffset/backingBytes、请求序号和预期schema；拿返回attemptId后立即调用已有诊断端点，保原返回体和首拒，异步写冷文件。普通调用/parse/诊断/文件失败都具名，不能让导出异常再次崩页。每attempt文件UTF8≤16384B（UI词≤2048、两native快照各≤4096、meta≤2048，余量用于编码；实际writer超限完整INCOMPLETE），last+first两记录不无限历史，计入既有2MiB writer/4MiB总信用。原生诊断仍固定小数组，不全buffer读回。

先复制完整调用数据再排异步IO，UI线程不等GL；旧模块mutex内只复制快照，序列化/写盘在锁外。保记录中的API阶段NOT_EVALUATED，不能用填零结果冒实际查询。

## 接力

T582按上述已绑定真源完成产品接线/编译/提交；测试任务再由tester用已有签名流程获取新包，保实际调用N16/N2/同scope raw请求、同attempt诊断与文件SHA，才分NAPI API/值/平台契约。未有该原件前仍UNKNOWN，不合并M3/G9同因。两份旧包的“不崩退”窄结论保留，期限/根成本NOT_REACHED。

所有执行prompt包含并遵循 [共同约束](architecture-T581-T589-handoff-policy-v1.md)，尤其developer不测试、不用机，不将签名/执行推回开发，不增manager验收门。
