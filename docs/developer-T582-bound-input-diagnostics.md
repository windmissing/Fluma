# T582 开发交付：实际绑定入口与同 attempt 原件

2026-11-18，developer。不使用真机，不运行测试/reader/模拟，不签名或部署。按 T581 已交合同完成源码、生成链和编译交付；NAPI 真机谓词、物理及期限结果由 T590 独立取证，本报告不宣产品 PASS。

实际绑定：T522/T529/T517 的真实生成链在原属性名上绑定统一 Invoke wrapper；原实现仅为 wrapper 内部实现，不新增 UI 未使用的替代端点。test/t582/overlay.cjs 的同一矩阵生成 api/route/expectedN 和 t582-bound-input-matrix.json，并断言 property table 指向真实 wrapper。M3 配置 N16、M5 配置 N32/G9 研究 N2/配置 N24、R2 请求 N1/Config64 N64 各自独立。保既有严格 uint32、准确参数数和元素数、offset0、detached/backing API 成功、base==data、完整 backingBytes 及同步 memcpy；无 number[] fallback/单位猜测。

ReadWords 在每个退出分支保留实际 API status、输出资格和原 copy header；失败也入账，不再只记录成功，且同一调用不二次分配 attempt。配置语义的 revision/features/profile/full/cold/hash/flags 等逐序记录首拒，失败不变更 App。原普通拒绝由实际 wrapper 返回完整 GPU_NAPI_ERROR_V1，带 api/route、bootStamp/diagGeneration/attemptId U64 字符串、stage/reason/napiStatus/qualification；原成功 JSON 保 revision 和字段，用 inputCall 关联身份。原 configure 成功状态改为 GPU_NAPI_SUCCESS_V1/state 的 JSON，页面仅核其具体 REQUESTED；原 pulse 等状态成功 0 保留。JSON 页上界仍为 16384，错误和诊断不超过 4096。pending exception 不清除，字符串返回失败不使用未初始化 napi_value。

native lastAttempt/firstFailure 为各不超过512B的二进制快照，三route共不超过3072B payload，由 static_assert 编译约束；不在512B里塞可能截断的 JSON。小锁内只复制/登记，4096B 序列化在锁外。boot 由真实 CLOCK_MONOTONIC 建立，attempt 单调不复用、耗尽拒绝；仅已提交 REQUESTED 的 configure 才换 generation。失败 configure 不清空首拒。查询指定 id 同时匹配 last/first；UI 传四参数 route/id/boot/generation 精确定位，旧二参数读法仍受理。真正缺失才 SNAPSHOT_EVICTED_OR_NOT_FOUND，不借其它 last 解释该 id。

UI 共用 GpuInputAttempt：调用前保存同步原词、Uint32Array 类型、length/byteOffset/byteLength/backingBytes、UI 请求序号和 schema。同一次 native 返回后、异步 IO 之前，立即用完整身份取 selected 和首拒原 JSON，核对身份后再交有界 writer；调用/parse/诊断/IO 出错均具名，普通错误不会再次变成未捕获 JSON.parse。M5 研究参数不再重复调用后拿第二次返回解释第一次。三个页面先撤旧帧与等待资格，session revision/18词身份合法后才创建 frame；M5 resume 同样等待 session，页面离开取消旧等待。清理故障另列，不以二次清理覆盖首因。

冷文件位于应用 filesDir/gpu-input-{M3A|M3B|G9}-{last|first}.json；.part 是未完成写入。每route只保两份记录，pending只保最新两份，不积累 Promise/历史文件。每attempt UTF8≤16384，UI请求≤2048，selected/first各≤4096；超限输出完整 INCOMPLETE，不截 JSON。first 文件明确绑定 currentCall，即使 NO_FIRST_FAILURE 也不冒另一 generation 的首拒。共用 DiagnosticColdFiles 在编码前预约原 2MiB writer/4MiB total 信用并计入两份 retained payload，异步 open/write/fsync/close/rename，完整落盘后返回 SHA256。没有在 UI 等 GL 或在 App/native 诊断锁内做 IO；运行时/分配器额外内存不冒成这里的 payload 额度。

构建与身份：G9 tree .codex/local-test/t582-g9-final 经 test/t599/generate.cjs；M5 tree .codex/local-test/t582-m5-final 经 test/t517/generate.cjs 并执行既有 G11/G12 头生成。两工程都包含同源 M3A。完整首次编译及最后同步文件后的 native/ArkTS/HAP 构建均已完成；最终日志 t582-g9-handoff-build.txt / t582-m5-handoff-build.txt。build/ELF/HAP/矩阵及准确源码 SHA 见 test/reports/t582/development.json。G11/G12生成仅补既有编译依赖，不增加仿真实现。

生成链向旧 T550 注入器显式传 repo root，不依赖工作区另有的 T550 路径修正；该文件他人既有未提交改动未纳入本卡。未修改物理/期限/守恒/配置接受条件。所有渲染仍为 XComponent+shader，无 Canvas/软件 GL 回退。T590 可按现有签名流程使用上述准确产物独立部署，核实际包/ELF/PID/boot 后复测 N16/N2 和同 attempt 原文件，不以本轮编译替现场结论。
