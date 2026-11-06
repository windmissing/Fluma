# T381 · CPU 干态跨列受阻时继续原列纵向运动

已实现并完成离线检查。仅在隔离 CPU 参考路径显式启用 `P011_CPU_DRY_CROSS_VERTICAL_V1` 时生效。签包与 SHA256 见[签包回执](../t257/T381-handoff.json)。签包入口为 P051 Derived V2 隔离页，按钮「导出跨列收据」写出当前已发布子步和独立失败回执。生产默认关闭，未使用真机；T367 同类设备观测由后续 tester 执行。

- 零获准的真实 T4 保留在源列，使用独立于目标床的 `blockedYFree`。有效 ticket 仍按原规则关闭和退款；全额获准仍用原 T4。触床先受行时床约束，最后一次 lift 和砂顶更新后、audit 前以源列终床复核。尾部只修改位置、速度和 state，由下一子步 T3 处理沉积；非零剩余质量仍保留。
- 链头预演绑定被接受的候选计划身份、父事务、输入/角度/步长、before bank 和初始预算；正式结果逐项复核，尾部另核 slot/generation/ID。通过 canonical writer 与 TxD1 写入，audit 后、发布准备前再核验真实最终位型。没有在 renderer、publish prepare 或 M2 finish 里写粒子。
- 外层失败保留原 Hot/Field 首错、阶段和回滚结果；恢复池、床、派生状态、预算票据和事件字节，撤销未发布 seal，保留 LastGood 和成功前缀。导出 finalF64Hex 保留有符号零。旧/新 checkpoint 交叉恢复拒绝。
- 稀疏干床出现无 donor/receiver 的空 creep 请求时，启用路径跳过该空行，避免 `-1` 污染预算。非零请求及现有容量、预算、守恒阈值原样。

离线结果见[新路夹具](units.jsonl)和[旧路差分](legacy.jsonl)。12 条自然受阻行观察到源列推进，近床夹具随后有 3 条获准沉积事件和 2 个最终退役；T369 两档参考复算为 y=30.65877735511109、vy=-44.4155562087579。覆盖左右、双端 local/total/容量和全额获准、目标高床、终床升降与端点、同角不同候选、before bank 篡改、±0、缓冲别名/拆离、真实 lift 同时保留两批尾项、slot generation 篡改、9 个故障点及第二子步失败时保留首段。旧路差分对 `1b30b69` 做 30 步和两次外源准入，核对 dry checkpoint、Hot 事件、湿场及 dry field 的原始数据。

现有 Hot 11、patch 12、sink 17、M2 13、M2 transaction 14、planning 106、gesture 18、input 124、S3 14、S4 10，均零失败；T216 exact70 3208 例及 tuple、publication、owner、checkpoint、legacy differential 通过。隔离 ArkTS/native Hvigor 构建成功（30 个任务），验签 PASS。

Nmax=512 时的新增定容 typed bytes：尾行及基础核验 313,856；plan 七个实际实例 81,936；八候选加 scratch 的前后数据与证明 1,660,032；尾的额外 generation 和发布位型 18,944；链头预算证明输入 2,072；父级预算/票据/事件恢复 1,417,676。共 3,494,516 字节，不含对象头与冷导出。尾行布局为 8×U32 + 20×F64，旧事件环格式不变。

复跑：`node test/t381/units.cjs`；从 `1b30b69` 用 `git archive` 提取 `entry/src/main/ets`，再执行 `node test/t381/legacy.cjs <baseline-ets-root>`。签包工程由 `node test/t381/generate.cjs <fresh-local-dir>` 生成。
