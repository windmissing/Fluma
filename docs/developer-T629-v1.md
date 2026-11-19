# T629 developer 交付

2026-11-19。按 T621 完成开发和编译，不运行测试/能力探针/真机，不签名部署。运行 provider 资格和实际配置能否接受均由 T630 取证；本报告不宣称已确认 BYTES，也不把 G13 静态接入说成同因复现。

共用 `napi_provider_profile.h` 在实际进程/环境首次需要时，通过当前导入的 napi_get_typedarray_info provider 做独立冷资格：记录 dladdr 模块/符号/地址、原 Native SDK header SHA、单调 boot、清理钩子/handle scope 状态及 Uint8/Uint16/Uint32/Float32 各1、3元素的八条原始记录。创建长度按本 SDK 的明确元素数合同，核实际kind/data/backing/offset和固定构造范围；只有整组一致区分 ELEMENTS/BYTES 才封 profile 和证据SHA。请求自身 N/4N 不参与选单位。固定八环境槽，环境清理撤 profile；不能资格、缺SDK/provider身份/资源/探针API失败均 UNKNOWN 安全拒绝。无 JS 自报单位入口；检测到 Sendable 时拒为未资格，不把普通 interop 资格套用它。

`ReadTypedValue` 为唯一 uint32 值解码；M3A/M3B/G9 的既有实际 ReadWords 绑定委托它，G13静态审计后同样接入，保持自己的诊断槽而不借G9槽。顺序保 argc/type/kind→原rawLength→provider固定单位checked规范化→exact N→offset0/data→非detached→真实backing精确N×4/base==data→一次定长copy。BYTES不可整除4拒绝，N×4静态溢出检查；错kind/偏移/短长/尾字节/detached不放宽。物理协议、revision/features/hash/reserved/模式/阈值不改。默认bridge/optics等其它老接口不在本卡迁移范围。

诊断 `GPU_INPUT_DIAGNOSTIC_V2` 新增 rawReturnedLength、lengthUnit、normalizedElementCount（未评估null）、providerProfileId/证据SHA/原资格JSON、各API原status与backingBytes。历史 elementLength 保原值；未调用字段仍 NOT_EVALUATED，不把0当未调用成功。原api/bootStamp/diagGeneration/attemptId和首因/last锁存不改；G13独立回复含同类typedarray诊断。快照512B、JSON4096B与安全异常导出门保留。`gpuInputDiagnosticsV1` 端点名称兼容，返回V2，自动first/last文件消费者显式接受V1/V2，不重写历史文件。

构建接线在两个实际 CMake 目标绑定 SDK header 指纹；t550同源overlay复制新provider头/CMake，T517/T522/T529现有生成绑定继续使用共同decoder，T582继续复制新V2消费者。工作区该overlay早已有root/d.ts两项修改，本提交只精选本卡provider复制与CMake接线hunk，不带他人修改。

编译：M3/M5包、实际G9生成包、G13静态接入包均native/ArkTS/unsigned HAP成功；源码/SDK/ELF/HAP/日志SHA见 `test/reports/t629/development.json`。仅Node --check检查生成工具语法，未执行测试或运行能力探针。G13此编译沿T608随包旧math/algorithm身份，未提前把T628新数学合同标给旧核；接续T632另立新身份。旧T608/T596历史构建报告不回写。

T630需对实际签包/ELF与provider、interop形态回环，取八类能力原件及真实M3/G9请求自动first/last；Sendable若平台支持单列验证。接口接受不代替compute/守恒/效果通过，UNKNOWN不构成开发关闭新门。
