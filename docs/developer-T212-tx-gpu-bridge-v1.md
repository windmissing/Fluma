# T212 GPU 整体事务隔离实验桥开发交付

日期：2026-10-28。developer，不使用真机。依据 T205 §10/D1G，生产默认路由、原 native/wire、原 T171/T191 全局接口未修改。仅新 XComponent+shader 渲染；禁止 Canvas 上屏或失败回退。本卡独立入口无渲染依赖。

## 实现与边界

`test/p013_compute/ets/tx/` 包含每 owner 独立的 compat work、固定 metadata、双 13buffer 冻结、backend、fixture、Worker、launcher。LEGACY_SCAN 与 GPU_PAIR_V1 同入口字节和原 CPU F64 writer；TX_ADJ_AVL 的窄接口及条件编译适配器已到位，等待下一张 T211 核心，当前调用明确 NOT_IMPLEMENTED，未用旧索引冒充树。

真实 root/run/step/attempt/substep/role/ordinal/pool slot 映射到独立 compat branch；每次请求保守增加 content revision，回滚相同字节也不复用旧控制身份。CPU 消费前复核真实身份、镜像、13 buffer 身份/长度/逐字节值。role=1seed/2collect/3preview/4authoritative/5explicit；native owner/context/request、完整回复原档保留。未知 V2 policy 明确 UNSUPPORTED。GPU ERROR/STALE 走原失败回滚，不自动改用 CPU backend。configure/消费/清理失败保留首因与 QUARANTINED；只有 reset 确认零 liveBuffers 才 RETIRED。

一个 Worker 一次 fixture，禁止重用，后续必须新 Worker；并发 owner 必须各自 Worker/NAPI env/EGL。没有宣称同 context reset 能恢复 owner 耗尽。UI 仅启动和收封存通知，不借渲染 context。

64 列 find ledger 保留 native prepare/pack/SHA/bridge/upload/dispatch/fence/consume 分项、真实/兼容身份、adapter freeze 和预期 SHA 消息字节/块、实际 reply dispatch 与预期 dispatch 分列。未到件的计时为 NaN，不伪零。原 audit、sink 的 inclusive clock/count 与 CPU merge 独立记录，不将相互嵌套耗时相加冒充 exclusive 根总时。Worker 另报完整物理 service、JSON/UTF8 编码、write/fsync、物理结果与归档结果，编码失败不把已成功物理结果改失败。

资源报告逐项列 8-view metadata、双冻结 bank、trace、CPU counters、12288B/native identity capacity。每 GPU request 的 26 个 bounded Uint8 view wrapper 明确计入，非零分配声明。runtime/native vectors/driver、字符串对象等 NOT_TOTALLED；外部快照 UNBOUNDED_EXTERNAL，不给虚假总 MB。

## 可复现与自验

生成：`node test/p013_compute/tools/generate_tx_gpu_project.cjs .codex/local-test/t212-h1-final STRICT_O2`。隔离生成器恢复当前原物理输入并逐 hunk 记录，旧 generator 不改。native wireV1/canonical LE V2 原源码和三 shader 保持原 SHA。Tree adapter 只在 T211 模块存在后编译，不拿未实现空壳作完成。

自验：`node test/p013_compute/run_t212_bridge_units.cjs .codex/local-test/t212-h1-final/entry/src/main/ets/physics`：9 PASS / 0 FAIL。包括双 owner 交错、same-byte rollback、取消/标量/缓冲区/镜像迟到、GPU 错误与清理失败不回退、未知版本拒绝、SHA 身份、N192/512 完整原物理状态与 pair 流一致、失败保留第一次成功源前缀、backend 冷绑定。nativeTransport=HOST_DOUBLE_NOT_GPU，GPU 正确性/收益/设备争用未验证。

真实 PackageHap、源与产物身份、四 TU 实际 strict 编译参数/宏、链接命令、object/packaged ELF buildId/section、unsigned HAP 与完整复现材料见 `test/reports/t212`。不构建签名包、不安装、不做 E2E。所有数值 fold/原错误阈值不变。T213 接收独立验证；本报告不宣整事务复杂度降低或生产 GPU 已接入。
