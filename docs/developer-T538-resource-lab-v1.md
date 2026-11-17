# T538 开发交付

2026-11-17。不使用真机；遵用户指令不运行测试、oracle、仿真或端到端验证。

`node test/t538/generate.cjs .codex/local-test/<fresh> m1|m2` 沿 T507→T536→T537 实际来源生成，增加 `T531_RESOURCE_LAB_V1` 独立诊断构建模块及 `pages/GpuResourceLab`。该页面只导入 lab 模块，不创建 XComponent、物理 owner、SOURCE 或 Swap；研究页固定面板也保留生产只读快照和内联 lab 入口。仅开发设施，无 Canvas/软件 GL 回退。

生产采样 `GpuResourceSnapshotCold` 直接消费原 `resourceState/cacheStorageState`。native ledger/cache/组合编码器改用预先分配的有界 streambuf，每原件最多 1MiB，溢出返回完整 INCOMPLETE；原 JSON 字段保持。原 trace 两槽容量/reservation/state/reader/builder 一并输出。采样 begin/end 独立 gate 以实际 scope 校验，begin/end native STEADY_NS、两次原 scope、原 JSON、UTF8 长度、fsync 后文件 SHA 均进入 `T531_RESOURCE_SAMPLE_V1`。总额含 manifest 最多 2MiB。finally 仅释放本次 gate。scope 任一变化即 INCOMPLETE。U64 原字符串不转 Number；签包输入声明与外部测量分别标识，未取得安装/ELF证据明确 NOT_MEASURED，driver 永远 DRIVER_NOT_MEASURED。

`libgrs_resource_lab.so` 独立链接生产 TraceManager/Writer/Gate/Capture/ledger/ProgramArtifactStore；仅此目标有 `GRS_PROGRAM_CACHE_TEST_FAULTS`，生产 grsm1 无该宏。实例、ID、目录、fixture 全隔离。目录先 canonical 验证包含关系，使用 pid/steadyNs/session 新路径，不清理生产缓存。无递归删除。native session 最多一个，控制记录 256×256B，命令直接有界执行、无待办队列；只 worker 执行大工作。reader 屏障场景最多 builder+reader 两线程；缓存场景 builder+store 文件 worker 两线程。reader 在 native 内持 Read，RAII 归还；不把产品 reader permit 暴露给 JS。

可达 case：

- TRACE_MAX_SOURCE / TRACE_MAX_Q：四 profile，在真实槽 span 原位填 recipe，真实 Layout/Finish/Page/Writer，保原始 block SHA256、recipe、编码 SHA、页数/最大字节、容量。合成内容明确不宣真实物理 trace 通过。
- TWO_SLOT_PIN_REPLACE：先失败候选保旧 published，再 reader pin、成功替换、第二替换拒绝、End/RETIRING、显式释放屏障、FREE retained capacity 和最终 trim。错误/取消也 join reader 后释放。
- TTL_WALL：真实 native 创建时刻起60s，worker≤1s等待/查询，Acquire/Release不续期；实际发现延迟原值保留。TTL_EDGE_INJECTED 另测边界、倒序、ID耗尽并标注注入时钟。
- DIAGNOSTIC_CREDIT：真实 Capture 和生产 bounded writer，一份副本；完整档实际大小/少1、2MiB clamp、最坏 escape fallback。四 scope 原 firstCause/faultRoot 完整文件保存。DIAGNOSTIC_BUSY_FAIL 调用真 gate 的 BUSY，并对分配/NAPI/I/O/取消/最终 manifest 故障出口注入 fallback；这些是故障注入原件，不称设备 allocator/系统调用真的失败。
- CACHE_FAULT：独立 store 实际 ENOSPC/sync/remove 开关、真实 tmp/files/bytes、目录暂不可用的 scan、128 队列、32 key pin、32MiB entry、128MiB pinned 与恢复。每阶段生产 Diagnostics502 原件 write/fsync/hash。生成文件仅在 lab session 内。

lab start/poll/release/cancel/export 接口不等待60s、不操作生产 App。导出 `T531_RESOURCE_LAB_RECORD_V1` 的 scope/sessionDirectory、逐阶段记录与文件 SHA；`FILE`/紧随的 `FILE_SHA256` 关联对应原件，tester应取回 session 目录文件，不能只凭 manifest 自证。原件故障不改写成 PASS。known payload 58,993,832B 之外，helper固定对象、cache pin/编码临时量、worker栈、allocator 明列排除，不能宣全进程硬界。

两路线最终 hvigor/HAP 与签名收据见 `test/reports/t538/delivery.json`。本轮只编译/签包；全部 case、真60s TTL、实际设备资源、压力、driver、物理功能均 NOT_RUN/NOT_MEASURED，交 tester 接力，不以编译替代通过。
