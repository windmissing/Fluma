# T596 开发进度（未完成）

2026-11-18。开发/编译，不测试、不使用真机。

已修：删除transport.cpp的CaptureColdV5重复stub，唯一实现来自cold_capture.cpp；byte offset摘要寻址改begin/4；prefix使用T535固定自身SHA掩码。增加FULL保留对象布局（20份physical+aux、5个BASE state/context、40960B控制和256B输入保守额度），在根Begin前复制真实BASE及SOURCE control/dedup/ledger/rawBulk/meta，scope入口/结束复制原cursor32，physical/provenance成对保留，不再按slot奇偶漏一半。FULL正式61段目录不改。

尚未完成：完整逐根INPUT/completion/quota/rootRecord和SOURCE控制封存、公开CaptureScopeMeta32转换/共同sealed授权、真实plan跳过记录、请求前native预约、跨route全进程ColdSlotBroker、V2 descriptor/header/page/reader/collector全链。现有FULL入口能力门仍保留，不把新增对象代码冒已可运行的完整FULL；未启用不完整模式。

安全行为：writer无法证明完整根元数据时不发布TRACE_READY/COMPLETE，返回MISSING_REQUIRED_ANCHOR。这个返回是开发缺口的诚实拒绝，不是本卡完成或正式功能。它尚非完整first-error分类，需在后续writer闭包同时收口；不把所有故障冒真实缺锚点实测。现有首次错误GL/EGL原值仍保留。

完整隔离工程t596-progress最终hvigor native/ArkTS/HAP编译通过；日志.codex/local-test/t596-last-build.txt，33tasks/14.306s。不运行测试/实际采集，不宣物理或协议通过。这里是进行中进度，不虚构外部前置阻塞，剩余属于本卡开发责任。
