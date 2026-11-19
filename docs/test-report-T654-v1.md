# T654 v1 · 目标 13 错误 29 完整能力与四页原件精准补证（部分域 + 阻断根因固定）

日期：2026-11-19，tester，真机（011124264R000337）。
设施：T654Probe.ets（t638 树 tester 页：g13Status 完整读取 + g13InitDiagnostics
4 页 raw + 同进程 router 到 G13 页产生 attempt）——6 轮签名迭代
（probe→probe7，signed 9ec69010）。
证据：`test/reports/t491/t654a/`（t654-capture1/2.txt 完整 g13Status JSON、
configure QUEUED 回执、STALE 拒读原件、v1-partial.json）。

## 已采原件

| 域 | 实测 |
|---|---|
| g13Status 完整 | route 13/schema 1、fieldToken G13F1:…、coldToken G13R1:…076a27c7…、consumedSourceSeq=0、lastRoot{clock=CLOCK_MONOTONIC,nonce=0,plan=0,deadline=0,nativeOutcome=0}、state=UNCONFIGURED、firstError=0、acceptedQuantum=0 |
| g13Configure 回执 | QUEUED error=0 attempt=2、typedarray diagnosticRevision=2 rawReturnedLength=512 |
| g13InitDiagnostics | attempt 110/2 全 4 页 `STALE_ATTEMPT_OR_PAGE`（33B 拒读原件） |
| G13 UI 诊断 | UNCONFIGURED（router 后实例）/ CONFIG_FAILED ERROR_29 attempt 110/209（直启实例） |

## 阻断根因（精确，回 architect）

attempt 232 原进程早在 T643 会话结束即死——诊断 raw 页（8192×4，decode/stages/
logs/control 分段）是**进程内存态、无持久化**，原 attempt 已不可回读（
differentAttempt=true 唯一路径）。

新 attempt raw 页采集要求：GL 线程消化 g13Configure（观察 Begin）→ 但 GL 线程
绑定 G13 页 XComponent 生命周期，**probe 页与 GL 页无法在同一 Ability 内共存**——
router 切页后 G13 页新实例 XComponent 不挂载、configure 排队永不消化
（UNCONFIGURED 恒持），Page() 恒 STALE。

这正是 T622 已预判的情形：需要**最小产品 seam**——外部可读三槽快照或独立非 GL
诊断线程。同 seam 等待族=T591/T594/T654。

## NOT_REACHED 如实

能力阈值（blocks/bindings/invocations/precision/laneX/shared/maxBlock/rawGL）
均未进 Build 域（CONFIGURE_ERROR_29 在 CAPABILITY:EXIT 后）——阈值观察
NOT_REACHED；不假 PASS、不从 CAPABILITY:EXIT 推资源通过。

## 已试与建议

已试：want 路由（EntryAbility/G13ResearchAbility 双 Ability）、probe 页内
同链 g13Configure、同进程 router 切页、三轮 want 参数变体。建议接手：
architect 按合同回执确认 seam 授权后 developer 落（T622 fallback 原文），随后
本 probe 设施直接续采（脚本就绪）。
