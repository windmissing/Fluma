# T681 · 单沙冷导出有界配对摘要开发交付

2026-11-19；developer；按T680 §4及contract，仅开发和编译，不使用真机，不执行测试、产品、shader、签名或部署。本卡是诊断字段保留，不是native ack/geometry缺陷修复，也不声称产品PASS。

## 生产链和来源

真源 `test/t426/PrepareReadinessV4.ets::report()` 新增 `pairingEvidence`；DTO与纯值提取在 `test/t426/PreparePairingEvidence.ets`。T574真实overlay复制两个模板，T659实际owner glue保留并检查外层UTF8上限，`tools/singlesand/prepare_owner_build.py` 将所有模板带入同一T598/T592固定基线 `9ec800f446c62d45197ec74abc4023e85400756d`，没有只改生成缓存或另一页。旧报告revision、authority原路径、公有report签名保留；新增块revision为 `T680_PREPARE_PAIRING_EVIDENCE_V1`。

report仅调用一次super.report，读取同次owner纯值副本/ownerSerial和原首末错误。从已生成authority、最多256条已有事件提取；不refresh/pack/frame/check/predicate/reserve、不bind owner、不调用native、不造生产时刻或第二套权威。DTO只有具名数据字段，JSON值不提供授权方法。原owner/ticket/reserve/RESERVED后consume、三秒期限、两帧permit、RasterGeometry allocation/change key均不修改。

完整与detailTruncated分支均保：gate session/prepareId/pageGeneration/各serial/startedMonoMs/state/已有reason/diagnosticOverflow；ownerSerial及完整十字段owner或合法null；snapshot window/代与scope、packet geometry/nativeGeometry/nativeReady/viewport/layout；raster标量、layout/draw/frameAnchor及native十项；原顺序三输入、键盘配置/device/windowMode/avoid单位、原顺序四rect、height/animation/history；已有最后同prepareId/pageGeneration的FRAME serial/frame/now/reason及与保存snapshot的serial比较。无已有记录为NOT_EVALUATED，数值为null，不跑predicate补reason。

gate observationSerial、raster observationSerial、recorded FRAME serial分别保存；不拿它们互换。geometry源明确ARKTS_PACKET，viewport源NATIVE_GET_STATE，不称native geometry ack。数字未知/不可精确表达、缺失或截断为null并列路径；合法0/false保持原值。过长身份不以截断前缀参与配对；owner=null不自动判产品身份缺陷。诊断COMPLETE/INCOMPLETE不是门禁PASS，scope未知/截断不具精确join资格。

## 预算和错误

逐次实际序列化后用UTF8字节计数：摘要目标4096B，readiness硬限14336B，外层硬限16384B。不可为追求建议目标丢整组必要数值，超过目标有来源标识；必要的文本裁减逐字段置null并标截断/INCOMPLETE。原firstRawError/lastRawError保持原路径和已界定原对象，不被摘要替代。

authority细节超限时只省去大authority，保配对摘要与首末原错；同时错误/摘要压力极端时明确INCOMPLETE/PREPARE_COLD_UTF8_LIMIT和丢失组/字段，身份损失仍单独列出。无法维持硬上限时拒绝导出超限文件，绝不标成完整配对证据。外层observation被省略时，摘要仍有gate/window/scope，不用page当前branch或新snapshot回填旧FRAME。预算处理仅改新建诊断DTO，不推进准备权威。

## 编译和产物

全新真实生成树 `.codex/local-test/t681-source-release/.codex/local-test/product` 完成native、ArkTS、unsigned HAP全链编译：**BUILD SUCCESSFUL 46.982s，34/34执行、零错误**。最初匿名嵌套对象的ArkTS类型错误已改为具名DTO；初次失败日志、随后成功与最终全新构建日志均保原件。SDK/既有API编译警告不冒设备资格。

`tools/singlesand/build_owner_receipt.py` 仅做构建原件与包内身份检查：600个实际生成文件逐字节核manifest，两份实际renderer模板与canonical模板一致；ABC包含新诊断revision/来源及旧wrapper revision。完整源码manifest、真实consumer副本、全部包内native SHA/BuildId与ABC/HAP SHA在 `test/reports/t681/compile-receipt.json`。没有执行report或语义测试来替代tester。

复现：`python tools/singlesand/prepare_owner_build.py .codex/local-test/<全新source目录>`；在其`.codex/local-test/product`中用当前SDK/JBR执行`hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`；再运行`python tools/singlesand/build_owner_receipt.py <source目录> <成功build.log> <证据目录>`。

后继由T671 tester维护离线parser/用例/签包，由T592 effector执行同scope键盘/prepare/RESERVED/HOLD/严格PNG采证；这些独立工作不是本开发卡关闭前置。前景背景仍仅XComponent+shader；不改endpoint/协议名/generation含义、不猜native ack、不以旧JPEG冒严格PNG。

## Actual artifact identity

- unsigned HAP SHA256: `176461c39e0c548d162424299dcff404c29f366853279b19101510de995a37fc`
- ABC SHA256: `9ee4321898dd09035b8c6e7f04648af050dc8aba6c9f9de00b146eddb2b3b8b4`
- packaged libp013renderer.so SHA256: `c466e6734feddf1db8364a82b47e7a9e6089d5d53377c7843a5a55ff7d8eb631`
- renderer BuildId: `5f30a03d8bbcb2236003bf24601cb5eb1e56403e`
