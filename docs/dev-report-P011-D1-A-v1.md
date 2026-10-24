# P-011 路线 C / D1 A 段开发交付

日期：2026-10-24。原因：用户转交 D1 更新卡，A 段立即开工、B 段待架构 rev1 验收。范围依据 `architecture-P011-dual-field-particle-v1.md` §13.5；不将 A 段自验宣称为完整 D1 或原型出口。

## 修改及理由

- `entry/src/main/ets/physics/P011ParticleField.ets`：独立 M1 SoA、O(1) 空槽栈、逐列分相缓存、粒子/来源 ID、完整独立快照、reset 不复用 ID、ID 耗尽拒绝；纯 `particleRouteEnabled` 双条件谓词，不读写真实场。无 import，无 renderer/页面依赖。
- 同模块的 `retainAfterDeposit` 是池内余量原语，提供给 A 段桩使用：仅减实际 granted，保留原粒 ID/来源，部分授予和零授予均 state=2，只有精确零质量才释放；sub-EPS 余量不删除。它不发布床层写入，生产原子提交留给 B/D2。
- 候选 collect 骨架只读快照与视图，按 ID 生成全序 deposit→crossing 请求，静置粒子显式重试、每粒最多相邻跨界一次、接触候选不再产生同子步 crossing；两次候选计划逐字段一致。只从 identity 读取 dt/接受角，未使用实时场或预算；终端速度来自显式候选参数，不读取 settleSpeed。
- 有界 journal 预分配请求行，复制输入身份/记录，重复 key 或溢出即抛错并封闭 journal，不能 finish 出截断成功计划。A 段冻结计划通过候选请求序列完整序列化比较，未冒充生产哈希或零分配热路径。
- `test/run_p011_particle_field_units.cjs`：实际转译执行产品 ETS，12 项 A 段自验，含 CONS-03 桩床增量+池余量闭合、拒绝零写入、ID 耗尽/复用、快照隔离、双 collect、CFL/顺序、日志溢出/重复/别名隔离、stub grant 六段身份、36,000 次随机双相池转移。

## 验证

| 通道 | PASS | FAIL |
|---|---:|---:|
| particle D1 A | 12 | 0 |
| gesture_inject | 13 | 0 |
| planning | 106 | 0 |
| s3_moisture | 14 | 0 |
| s4_deep_creep | 10 | 0 |
| input | 124 | 0 |
| advection | 8 | 0 |
| 合计 | 287 | 0 |

36,000 次随机测试是按 60Hz 的 600s 步数执行池分配/部分与完全授予，每步分相误差 <1e-9；不是六事务 600s 真实物理仿真，不代替 §8 第1项完整验收。

hvigorw：`--mode module -p module=entry@default -p properties.enableSignTask=false assembleHap --no-daemon`，BUILD SUCCESSFUL（17.798s），零错误，保留既有 warning。独立 ETS 源另复制为临时 TS 用 bundled TypeScript `--strict --noEmit --target ES2020 --lib ES2020` 检查通过。新模块尚未接产品入口；包构建成功不等同真实场粒子路线运行已验证。

日志位于 `.codex/local-test/d1-a-{units,gesture_inject,planning,s3_moisture,s4_deep_creep,input,advection}.json` 和 `d1-a-build.log`。按用户分工不操作真机，端到端由下游智能体执行。

## B 段交接与未完成项

A 的 `collectParticlePlan` 显式接收 params，`applyParticlePlanToStub` 仅接受无 pool 权限的 A 段 sink，均为隔离骨架；B 段按已验收 rev1 收口最终 API 和完整预算/原子池/CommitSink 协议。当前 snapshot/free-flight scratch 为自验使用，尚未实现生产复用/恢复/prepare/publish/restore。

完整 spawn 质量包与出生、lift/deposit/crossing/degrade/merge 原子六事务、真实同公式预算/guard 桩、双跑与真实 shadow/live 可替换性、全部运动预测发布/事务级回滚尚未交付，仍等 B 段授权。WaterSandField setter/真实 guard/床/susp/λ/η/观测账本/页面/renderer 均未改动，属 D2 或后续卡。不用本次 PASS 放行完整 CONS/BUD 组或原型出口。
