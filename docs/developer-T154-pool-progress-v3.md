# T154 POOL_PROGRESS V3 开发回执

2026-10-27，developer；不使用真机，原卡 `6adff62ce4b0127832d27965`。
**诊断实现已入库；旧静态分配门禁未通过，不能交付为全部验证完成。** 按失败规则本地行“看评论”，原卡交 architect/T155 继续处理门禁接入证据。

实现消费 T116 V2 §4，保留 T149 已入库 hunk及原 audit/排序/事务次数：

- `P011PoolProgressDiagnostic` 冷定容 32 条最早记录、32 条最近环、16 个在途 span、24 个计数；环满增加 dropped。begin/end 以同一 span 记录单次 seed/collect attempt/STEP 和内部阶段的真实 delta；RUNNING 的 end/error/delta 输出 null，首次超长的部分计数明确 `RUNNING_PARTIAL`，不冒完成。
- audit 从实际 SINK_ROW/APPLY_TAIL/FINISH/OTHER 调用点计数；collect ID sort 与 audit sort 分开。排序比较/sift 迭代、index sort/比较/sift/tree/range/rebuild、birth B、Hot transfer 和 field checkpoint read/restore 分源。`poolCopies` 为 Hot transfer + field checkpoint read/restore；导出 snapshot 与诊断原值拷贝明确 NOT_CAPTURED，不把它叫全部内存拷贝。
- root/end/parent/child/count/epoch 在循环入口、阶段和数值游标更新，禁止逐比较日志。每 128 次游标更新检查一次首超长窗口，阶段退出也检查；之后不再产生新超长 dump。
- run/state/config/substep/attempt/candidate/λ/preview、seed phase/source/列/质量/位置/速度、energy 前后、实际 Nmax/live/free、pool owner/Hot token 同记录关联。未开启 candidate trace 时角色标 NOT_CAPTURED，不猜 role。
- 页面只注入 `systemDateTime.getUptime(ACTIVE,false)` 单调毫秒时钟，物理模块保持无 SDK/渲染依赖。未注入/抛异常/时钟倒退显式标识，耗时 null，未使用 Date.now 冒单调阶段时间。
- 首超长时一次保存当时可取得的 13 组 pool 原字节及 live/free/nextId，另导出 config/seed F64 原 bytes；FNV32 明确仅相关性 hash。捕获点标 `AT_THRESHOLD_DETECTION_NOT_REPLAY_START`，没有完整场/票据历史，`exactFullTransactionReplay=false`。源码与无签名包 SHA256 见 build_manifest.json；不把合成池或本机结果当设备精确重放。

离线实测（均为测试输入与确定性测试时钟，非设备成本）：

| 夹具 | seed / B | audit sink / tail / finish | index rebuild | Hot transfer / field read / restore |
| --- | --- | --- | --- | --- |
| N=16，单次 .35 / 包 .1 | 1 / 4 | 1 / 1 / 1 | 0 | 10 / 2 / 0 |
| N=192，满池可合并 | 1 / 1 | 1 / 1 / 1 | 1 | 10 / 2 / 0 |
| N=16，满池不匹配、8 次 λ | 8 / 8 | 8 / 8 / 0 | 8 | 80 / 2 / 9 |

此表是整个夹具区间累计；每 seed 的独立 delta 在 selftest.json 的 seedRow/lastSeed，不能拿累计当单 seed。由真实路径可见失败重试也可能经过 sink/tail audit，但未到 finish，不能一律乘三。

`test/run_t154_pool_progress_units.cjs` 8/8 PASS，含阶段分源、计数乘数、λ/rollback、首32与ring、冻结原池字节、RUNNING/未知时钟、非法ID、开关/抛异常时钟的物理等价。Hot 11/11、M2 13/13、patch 12/12、sink 17/17、T149 13/13、T85 7/7、T123 155 例零失败。

编译：BuildNativeWithNinja、CompileArkTS、PackageHap 通过；assembleHap 仍在既有 SignHap storePassword/keyPassword 少于32位配置失败。日志 `test/reports/t154_build.txt`，未改签名。

剩余失败：`node test/probe_p011_particle_hot_graph.cjs` 返回 NOT_ZERO_ALLOCATION。修改前 HEAD 6ee7e19 已有2个未解析调用（T149 captureFailure、T123 mergeIndex.find）；当前另有受开关保护的诊断方法调用未纳入该分析器模块清单/控制流。完整当前列表及原有 cold totals 对象位点见 `test/reports/t154/static-gate.json`。保持失败原样，未删门禁/加纯函数白名单冒通过，亦不由 developer 据此断言真实分配泄漏。需 architect 明确诊断 OFF 的控制流合同及工程分流，tester/T155 对真实当前图补接并执行。该证据未闭合前，不宣称生产 Hot 零分配已由本卡证明。

代码没有删 audit、跨 mutation 复用证书、批量改变 seed 或跨帧拆事务；新增计时包含诊断开销。真实长窗口及设备收益仍需后续串行设备取证。
