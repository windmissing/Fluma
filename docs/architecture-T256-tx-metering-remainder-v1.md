# T256 同包事务计量剩余工程分流

2026-10-29，architect；原卡 `6ae2a419e4b0ab67702dea7f`。消费 T250 报告、`test/t250/generate.cjs`、T205 manifest 20 cut 与 T245。修订 T245 工程边界。不使用真机、不重跑既过 64root 矩阵、不修改业务代码。

T250 已编译同包三 backend、COMMON_FREEZE_OFF/ON 六路、root before/after/receipt、统一64槽词典，离线12组终态一致；B birth完成、M成功/尝试、A角色进入、source λ循环已有局部计数。GPU 离线 selftest 明标 CPU double，不能作设备 GPU 性能证据。20 cut faultPlan、step planner λ、FIFO服务、JS/native/driver总资源仍 NOT_IMPLEMENTED/NOT_MEASURED。原 `rootRows` 的 rollback 字符串只说明有归档 before/after，不等于逐 cut 故障已注入或回滚已逐字节比对。

## T250 可继续的工程表

| 原 cut 顺序（T205 manifest ID） | 必须新增的隔离 faultPlan/trace 证据 |
| --- | --- |
| 1 SOURCE_IDENTITY；2 SOURCE_CAPACITY_ETA；3 BEGIN_HOT；4 COLLECT_HEADER；5 COLLECT_AB | 各入口/出口固定 root、parent、λ/seed/substep、cutId、before bytes、原 firstCode/receipt；在对应原检查点注入，分别核源准入与未写状态 |
| 6 REQUEST_VALIDATE；7 SINK_RESERVE_GUARD；8 PATCH_PREPARE；9 FIND；10 PATCH_PUBLISH | 逐 cut 注入前后，记录候选/slot/ID/预算、原错误位置与内部写域；失败回滚实际权威字节、已成功前缀保留 |
| 11 AUDIT_SINK；12 TICKET_CLOSE_EVENTS；13 MOTION_APPLY；14 AUDIT_TAIL；15 AUDIT_FINISH | 审计 execute/cert hit、event/undo/ticket credit 与首错；嵌套子成功父失败、尾部失败均核回滚及证据封存 |
| 16 PREVIEW_RESTORE；17 AUTHORITATIVE；18 FIELD_AUDIT；19 SOURCE_PUBLISH；20 OBSERVE_SEAL | branch/authority/field/发布/观察各原 cut 顺序与封存可见性；IO/迟到仅在对应版本已实现时执行，未实现写 NOT_IMPLEMENTED |

每条 faultPlan 必须带 `fixtureId/buildSHA/backend/freezeArm/PID/rootId/cutId/edge(BEFORE|AFTER)/ordinal`，逐 case 冷克隆同输入；保存原代码/阶段/ordinal、offered/admitted/accepted、before/after authority+receipt+events+lineage 原字节与首次失败域。可注入点与原 code 映射不能凭人工文本猜；若某 cut 无可控故障入口，标 NOT_IMPLEMENTED 并精确回报接口缺口。撤销注入后再次冷跑作对照；绝不在生产默认挂 faultPlan。

工作量统一原 public root/call-set 分母：`B_enter/B_completed/B_commit` 分开，含失败与回滚工作；`M_attempt/M_find/{found,noPair,error,active,cancel}` 分开；`A_logical=A_execute+A_certHit`，记录每次 cut/read-set 失效；source λ 与 step planner λ 各有独立 `attempt/accepted/rejected`，不得以 source 循环代 step；每作用域记 enter/leave/fail/cancel、exclusive CPU 和子作用域关系，缺 trace 记 INCOMPLETE 而非0。资源需列 typed arrays、native slots/buffers、wrapper、trace/rootRows 实际 UTF8 bytes、冻结副本、external copies 的可见容量/峰值；driver/RSS 未可测项明标 NOT_MEASURED，不造总量。T205 的 candidate FIFO/异步新行为若未实施，登记 NOT_IMPLEMENTED，不作为旧同步实现的产品缺陷；但已有同包同步 root 的排队/服务时间入口与定义应先补。

**T250 developer 续工，不使用真机**：按上表补 faultPlan 与逐 cut trace/checker 接口、step planner λ及范围内资源词典，编译及离线自测；已有六路和局部矩阵不重跑。若具体算法合同无法决定的 FIFO/取消/新状态才提 designer/manager，不把明确工程埋点推走。**T251 tester 真机**：在有效签包后按同 binary/PID 顺序执行 20 cut、同 offered 两 freeze 臂、满池 synthetic，采真实服务 p50/p95、全链 prepare/worker/fence/map/archive/UI完整新 XComponent 帧与设备资源；driver 不可见总量标 NOT_MEASURED，故障回 architect。旧 pair 设备排在 GPU 驻留取证后，T250 开发卡关闭不以真机或 driver 不可得计量为门。签包沿 T253；全部前/背景只新 XComponent+shader，FAULT 或显式完整 STALE，禁 Canvas。
