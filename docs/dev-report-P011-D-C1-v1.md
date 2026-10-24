# P-011/P-012 D0 / D-C1 developer 交付记录

日期：2026-10-24。原因：对齐已验收 gesture-sand-inject-phys §3 与 dual-field-particle §0.2 的期望基线，解除持续注入误冻结。

- `WaterSandField.ets`：保留并验证工作区已有 D-C1 草稿，audit(true) 直接复用 getDrift()。heavy/light 期望为 initial+sourceInjected，water 为 initialWater+waterTransactionNet。getMetrics 已调用 getDrift，无需改动。DRIFT_LIMIT=0.001、比较符号 >=、库存/容量检查、λ/η、渲染均未改动。
- `run_p011_gesture_inject_units.cjs`：原 10 例不变，新增越界准入后 120 步成功、三相受控误差与 audit/metrics 一致（0.0005 接受、0.0015 拒绝）、无源 120 步对历史公式逐位比较。无源比较只固定 Date.now，消除耗时遥测差异；物理状态、step 返回和 metrics 均严格相等。
- `probe_p011_audit_drift_gate.cjs`：原探针保留纳入版本管理，未修改。

验证：

| 通道 | PASS | FAIL |
|---|---:|---:|
| gesture_inject | 13 | 0 |
| planning | 106 | 0 |
| s3_moisture | 14 | 0 |
| s4_deep_creep | 10 | 0 |
| input | 124 | 0 |
| advection（额外回归） | 8 | 0 |
| 合计 | 275 | 0 |

旧公式负对照：通过本地 Node loader 将 audit 漂移计算恢复为 HEAD 原公式，不改工作区产品代码。原 10 例及无源比较通过，新增持续注入与口径一致性两例失败，持续注入重现 errorCode=5、ok=false。修复版 13/13 通过。

探针：initialHeavy/Light=12800，累计准入约48；first step ok=true、err=null，second step ok=true、err=null、stepped=true；heavy/light drift=0。探针输出保存在 `.codex/local-test/dc1-probe.log`。

构建：DevEco bundled hvigorw，`--mode module -p module=entry@default -p properties.enableSignTask=false assembleHap --no-daemon`；JAVA_HOME=jbr 并将其 bin 加入 PATH，DEVECO_SDK_HOME=DevEco/sdk。BUILD SUCCESSFUL（13.868s），零错误，有既有编译警告。首次缺 Java PATH 导致 spawn java ENOENT，修正环境后通过。日志 `.codex/local-test/dc1-build.log`。

运行验证分工（2026-10-24 用户明确）：developer 负责开发和不依赖真机的测试自验；其他智能体负责真机端到端验证。Node 已实际执行产品 ETS 物理路径，离线自验及构建完成；未安装或操作真机，不声称完成真机启动/页面冒烟或长时注入验收。真机验证作为下游交接事项，不再等待 developer 占用设备。

仅提交本卡相关文件，不包含其他在途文档/渲染修改；D1 仍须 manager 验收后解锁。
