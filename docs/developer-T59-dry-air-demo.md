# T59 真干空容器与空气触点准入

2026-10-26，developer。按 T58 `P051_DRY_AIR_SOURCE_V1` 实施，不使用真机。T62 共享页面 hunk 已交接，本卡保留 SURFACE_PX_V1、同节点触摸和 COLUMN_PRESERVING_F32_RENDER_V1，不重做拒绘或材质。

## 实现与边界

- 新 `P051DryParticleAdmission.ets` 定义独立请求/回执/版本/纯预检。`initializeDryParticleDemo` 先预检与冷建 binding，再成组创建全零 water/H/L/P/U/source/count/epoch 的新 run；普通旧 initialize 仍为湿场默认。`injectDryGestureSand` 要求显式 dry route、源身份/单调成功 sequence/时间、合法触点、正/零请求及非 busy/frozen。
- 使用 `DRY_AIR_V1`：waterTop 为 S+U+P 派生占据包络，实际 getTotals().water=0，干场 U=0；冷启 waterTop 逐列0。新增 `getDryDiagnostics` 明确 freeWaterTotal / occupiedVolumeTotal / EMPTY_BASELINE，观测 audit 同名字段保留旧 waterTotalBefore/After（旧定义是包络总和，不可冒充自由水）。不改正式 endpoint/peak 协议的适用域。
- 一触点两相统一 λ，容量 C−O，最多8次/λmin1e-4；预检完整分包+非零尾量、free slots、ID、source/commit 安全范围。无外源合并、无向床直接加源、无悬沙兜底。原 injectGestureSand 的 NO_WATER / η / λ 数学保持。
- M2 `beginDryBatch/finishDryBatch` 用原 HOT begin/apply/finish/rollback，H/L 仅一次事务；DRY_POINT_V1 每粒精确保留 x/y、外源初速0、原phase/source/ID。第二相失败回滚 field/pool/cache/free stack/nextId/source/publication；成功后 IH/IL、epoch、独立 admission count 各按一笔更新并只发布一次 EXTERNAL_ADMISSION。成功源后另一步失败只回滚该步；导出 sink 故障记 INCOMPLETE 不重注入。
- dry SPAWN/CROSSING 容量使用 C−O，publish patch 后、audit 前用实际 cache 更新候选包络；LAND 仍原票据/guard/退款/state2，同候选床增与池减。collect/planner/live 床容量共享 dry helper，mode 进入固定 projection 比较，HOT frozen input 在本步校验 mode 不变。dry collect 跨列出生 slot 预检避免 pool 满降级，lift 保床/退款，悬沙降级被 dry 审计拒绝。
- dry 空气动力 `DRY_AIR_GRAVITY_V1` 与场共享 accepted tilt；vx 阻尼+9.8sinθdt、vy−9.8cosθdt，H/L共用。HOT 双 collect 与冷参考同版；wet 空气/水中原方程保持，未把 settleSpeed 重新解释成已标定速度。
- snapshot/publication 保存 medium/airDynamics/birthPlacement；dry restore 在写 live 前校验源身份/基线/count/缓存/空槽/ID/容量/包络/质量。缺介质却带 dry 动力标识、篡改包络等拒绝。历史缺字段视为 wet。dry 水事务、legacy inject、其它不兼容路由/含水配置、显式 invert 在写入前拒绝；旧 wet 路径保留。
- P051EmptySandDemo reset 真干场、T36 时钟后一次同步 dry 请求；触点不钳水面或床面，非法/拒绝停止触摸；Up/Cancel/后台停止与 reset 清债。页面仅补真实 dry 诊断，复用 T50 映射（NOT_CALIBRATED）和 T49/T62 basic 渲染。

新增 HOT 数值数组为零；medium 为标量字符串。M2 projection 每个槽+1 Float64，共8槽+scratch=72B冷分配。dry run reset 冷创建原固定 workspace，不在粒子/子步扩容；外源请求及 schema/证据导出沿既有冷路径。没有增加子步、阈值或预算。干场显式倒置仍 DRY_INVERT_UNSUPPORTED，未声称完整倒置物理实现。

## 自测与交付

`test/run_p059_dry_admission_units.cjs` 19/19：冷启/空分母、旧 NO_WATER、精确两相一次commit、第二相故障全回滚、统一容量λ/单slot双色拒绝、非法输入/NO_OP、持续LAND零水、零预算CONTACT_WAIT/重试、满接收列CROSSING拒绝、export失败、后续step失败不撤销源、重复序列/后台无追赶、分相期望、accepted倾角/冷参考双HOT一致、满池摇晃保床不入U、snapshot完整性、模式门和同commit适配。

九个既有物理通道 338/338（gesture18/planning106/input124/S3 14/S4 10/advection9/M1 33/HOT11/M2 13）；额外 hot sink17、patch12、hotS4 6、refund11 均全绿；既有 DEMO5/5、重力6/6、packet11/11、T62 repair6/6。HOT 36,000步探针 ZERO_ALLOCATION（显式源码分配口径；Node GC有观测，不等于ArkTS设备零分配/帧时证明）。原测试与判据不改。最后 hvigor CompileArkTS/BuildNativeWithNinja/PackageHap BUILD SUCCESSFUL，零错误；证据 test/reports/t59，命令原始输出保留。

开发交付不代表屏幕产品成功、性能通过或重力/材料已标定。真机/端到端由 tester、effector 后续独立执行；需使用本次构建核对 W=0/包络区别、精确触点、落床累积、stop/reset/后台、同commit屏幕及手持方向，不用历史隐藏水或旧 T52 拒绘结论代替新 run。
