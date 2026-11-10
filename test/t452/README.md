# T452 · GPU M2 V3 常驻事务交付

实现位于 `entry/src/main/cpp/gpu_resident/m2`，应用接线由本目录 `generate.cjs` 从冻结基线 `0228a7d` 生成独立 `m2route`。旧 M1 V2 接受、审计与 reader 保持原协议。入口明确为 GPU M2 V3 冷空场；不迁移旧库存，不使用真机，不使用 Canvas。

SOURCE 是 13 个 dispatch、4 个 SSBO binding、单个尾 fence 和 1024B completion map。最低空槽出生、满池确定选对、宽整数 RNE 合并、pending origin、Bmax/ID/列容量/NO_PAIR 正常前缀停止均在 GPU 执行。spare 从 BASE 重建独立索引并重新计算逐行和 terminal/dedup/sourceEvent 尾部，逐 word 比较；schema/order/mutation/prefix 首错保存阶段、行号、word、expected/actual。fatal 整根拒绝，candidate 和未接受 append 不能成为下一根权威前缀。

V3 QUANTUM 延用既有 GPU 运动与 guard/预算数学，增加有效来源区间、U64 每源累计和合并库存审核，保持最近 SOURCE 摘要。SOURCE128 的 scratch 只在接受后换入逻辑 arena；QUANTUM64 起始前缀为0，连续 QUANTUM 继续自己的64字前缀。短接受保留 permit/tombstone/绝对期限与发布 lease。scene token 绑定独立 V3 root token，沿 XComponent+shader Swap；失败沿既有 FAULT/STALE 处理。

冷档从显式按钮预约，在 GL owner 根空闲点冻结并锁存不可变 completion/state/events。分页读取不运行 GL；页面写出逐文件 bytes/SHA256 收据。`cold_reader.py` 拒绝 V2、错误 stride、错页/缺页、旧身份、错误 token/哈希和 event 保留位；SOURCE128 和 QUANTUM64 分开解析。它是结构 reader，不代替 T453 的独立物理 oracle。

## 复跑

```powershell
python -X utf8 entry/src/main/cpp/gpu_resident/m2/generate_layout.py --check
cmd /c test\t452\build_host.cmd
cmd /c test\t452\build_source.cmd
node test/t452/generate.cjs .codex/local-test/t452-replay
# 在该隔离工程执行 hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon
node test/t452/build_route_host.cjs .codex/local-test/t452-replay
python -X utf8 test/t452/check_traces.py --project .codex/local-test/t452-replay
```

主机 GLES 复用仓库已有 ANGLE SwiftShader、SDK shim、MSVC 和 `t236-source-host` GL 转发，不含 CPU 仿真实现。首次 JIT 以及 reset 后重建 program 可能需要数分钟；软件 GL 时长不用于设备期限或 fps 结论。生成目录必须是尚不存在的新路径，实际提交只包含生成器、产品模块、自测和报告。

四档 `(K,Kw)=(192,64)/(192,128)/(512,64)/(512,128)` 每档20个完整 GPU root，共80个。覆盖 BIRTH_LIMIT 部分准入、去重、满池 merge+birth、DIRECT W/J、payload conflict、零量、完整 QUANTUM、四类行/状态污染23–26、NO_PAIR、列满、末 MAX_ID 和 generation 在已写部分候选后的 fatal 回滚。每个可接受尾还验证错摘要被短接受拒绝。输出原始 GPU 冷字到 `.codex/local-test/t452-host/raw-*.bin`，供结构 reader 自验复跑。

生成应用的 native host 复用实际 bridge/run/transport/scene：源准入、松手后 J→W、真实 pbuffer Swap、冷档和 reset 新 run。开发自验通过不等于 T453 独立差分或真机效果/性能通过。最终检查、kernel/布局哈希、签包及原始日志摘要见 `test/reports/t452/delivery.json`；签包验签记录见 `test/reports/t257/T452-final-complete.json`。
