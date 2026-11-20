# T676 M2 与正式 M1 离线采集

2026-11-20，tester。两个域可独立交接。签包路径、SHA256 与逐 ELF 哈希见 test/reports/t676/package-identities.json。

A 域已验证可复用 T590-cursor2 签包，libgrsm1 SHA 与 T663 编译回执相同（7a0bcbaa…，Build ID c23e231c781476010af91f631c60d2239371205c）。原 T590-v3 cursor 原件已经交 T682。T591 collector 源现在用严格 JSON、基线 serial、同 scope、sealed 真值与正的 nativeEndNs 判定 terminal。仍需将修正集成到 T663 同页签包，补齐日志分页、字节预算和 scope 原件。现成 cursor2 可用于既有单 API 路径。

B 域复用 T597-fixed 签包及体验页窗口路径。T593-v4 无窗口 probe 的 `executionDiagnosticsV1('m1', 64)` 抛 SyntaxError。真实 NAPI 用 JSON.parse 读取 scope 字符串，读取 pid、route、source 与完整 control；第二参数是字节预算。T593Probe 改为先读 coldCapabilities 再直接序列化原生 scope，传 8192 字节预算。保留 u64 字符串。无窗口结果只用于负基线。正式采集需要在同一体验页和 owner 上观察 READY、资源发布/退休、Fresh、真实 Down、typed enqueue、同序 Claim/SOURCE 及 Draw/Swap。目前补丁未进入已有包，体验页同 scope 记录及预算验证仍待完成。T593-v4 的 READY 证据不覆盖本次 Down 或 Claim。

两包原件和可复用路径已列明。请 architect 按角色边界接收未闭合的采集设施点，续作仍由 tester 在真实体验页源上完成离线集成。

离线回归复现命令为 node test/t507/page_units.cjs .codex/local-test/t597-fixed test/reports/t507/trace test/reports/t676/t507-baseline.json。诊断夹具运行后在正式 M1 源码断言 assert(s.includes('coldTrace.collect')) 失败。该断言与体验页缺接线需分别核对，保留为未验证，不把 T507 基线冒充本卡同页通过。

2026-11-20 同页补充版已编译并签名。test/t676/prepare.py 从隔离的 T663 和 T597 源树各生成一个版本，保留原 XComponent、Ability 及入口。A 按 bootSerial 读取原诊断和三个对象 log，再读取同 serial 锚。B 在正式 M1 页面按真实 coldCapabilities scope 传 JSON 字符串和独立总字节预算。native Gate::Begin 要求不少于 16384B，8192 只用于写盘分片。B 读取 STATUS（含 Observe 副作用）、RECOVERY、BOOT 和 EXEC_DIAG，末尾再读 scope。真实 Down 沿页面 recordPageTouch / model.down 原入口，保现有 allowed/sourceAttempt 数据。两构建分别 66.260s、67.501s，均 34/34，verifyApp PASS。签包/实际对象/源及命令见 test/reports/t676-same-a/ 与 t676-same-b/，原包仍单独保留。

test/t676/audit_capture.py 对分片偏移、SHA、UTF8、64 query、60s、32KiB索引及1MiB目录限额做离线验收。夹具五项通过。对既有 t676b2.ndjson 逐行严格解析时 5 行均拒绝，原始行 SHA 和 JSON 错误单列，不修写历史。classify_cursor.py 用 T590 原始样本检查 cursor 版本、保留区与同域五字段，再区分 API 返回、原生终态和观察末仍开放。本次 replay 判 OPEN_AT_OBSERVATION_END。

仍缺严格前置。A 为当前 attempt 的手动同页导出，尚缺“创建前 arm、持续发现新 serial”的证据。B 的新 collector 与新编译出的 libgrsm1.so 一起打包，实际 BuildId 59e6c47e…，不同于正式 T597-fixed 原 ELF；A 新包也产生 b31cf133…，不同于原 T663 签包。签名和编译通过只确立各自产物。将这两包标为补充诊断，不能替代正式原 ELF，也不能拼入原运行记录。需 architect 明确允许的同页采集与原 ELF 绑定方案，再完成 A 基线 arm 和 B 正式原包同 scope 的关键输入判定。原包已可独立使用的现有出口照常交接。不使用真机，本卡看评论。
