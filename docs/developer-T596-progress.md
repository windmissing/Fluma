# T596 开发回执：契约冲突，整卡未完成

2026-11-18。不测试、不使用真机。本人行登记看评论，不声明FULL可用或协议PASS。

本轮独立收口：共享DSO libgrs_cold_slots.so统一M2/M3/G9实际TraceManager的两slot、单builder/reader、全进程及各route原上限。分配前预约；缓存/reader/RETIRING不因逻辑取消退账，Trim的RAII票据随detached block移动、delete[]后退款。实际生成链复制并链接公共实现，不以各DSO静态对象冒进程单例。这是native cold信用，不冒尚未完成的FULL GPU预约票据。

writer唯一cold_capture.cpp定义；摘要byte→word及T535虚拟零化。V2 header/descriptor/rootRecord校验拒绝错版本/范围/摘要/保留位/假NOT_EXECUTED。SOURCE/physical/provenance frame取accepted state的真实arena/有效行数，不取无关buffer/容量。全量校验移至owner锁外。GL/map/unmap/参数/上下文/in-flight/缺锚点/META_INCOMPLETE分别具名。历史迁移脚本显式退役，不能覆盖canonical；正式生成仍复制canonical。G9 cold_trace的V2 receipt helper与T599为同批依赖。

上一提交FULL布局、BASE原件和双流copy保留；FULL早期能力门仍关闭，writer元数据不完整返回META_INCOMPLETE，不发布TRACE_READY/COMPLETE。编译通过不代表完整FULL实现。

具体阻断：T552要求在v5BeginScope之后、首事务之前捕获原入口cursor32，但lift_writer.glsl::v5InputEnergy在同次SCOPE_INIT invocation内先BeginScope，再credit(0,1)、WriteAux、SealCompound。host只能整dispatch后copy，入口aux游标已变。GL copy不能插入invocation；禁止新增dispatch/热map，不能冒尾为入口、反算一笔或自行占reserved word。

已尝试排除：dispatch后copy不保真；反算违背原件；新增dispatch/热读回违反合同。原卡评论6afd53cce4b0d2e2f7786bb2已有源码切点。所需architect裁定：既有cursor32显式入口字段/usedMask修订，或同预算零新增dispatch切点。developer据新合同继续封存实现。

未完成仍属本卡developer责任，不冒只缺测试：逐根INPUT/completion/quota/rootRecord、SOURCE跳过证据；原入口/尾cursor→公开Meta32和共同sealed/forensic；FULL首根前GPU/native联合预约及GPU退休RAII；完整V2 writer/header/descriptor身份填充与collector/reader闭环。FULL公开证据按裁定切点共同收口，当前不启用半实现路径。裁定后仍需developer完成，不转嫁tester。

新生成 .codex/local-test/t596-broker-final native/ArkTS/HAP成功（60.083s），日志 .codex/local-test/t596-broker-final-build.txt。llvm-nm确认一个CaptureColdV5全局定义；llvm-readelf确认M0/M1均NEEDED同一libgrs_cold_slots.so。test/reports/t596/link-symbols.txt保留符号收据。未运行测试/reader/GPU/真机。
