# T467 · 显式恢复与首 root 接线

不使用真机。`generate.cjs` 生成独立 regular / experience / t410 / t437 四个来源，保留正式 V1 默认；实验两页在实际 frame 启动前冷选 V2。T410保留T450冷SHA入口；T437再接T434完整/前缀双路和复制账，不热切当前run。公共物理、wire、GAP=33333333ns、root绝对期限250000000ns和120Hz配额不变。

```powershell
node test/t467/generate.cjs .codex/local-test/t467-replay experience
node test/t467/build_controls.cjs .codex/local-test/t467-replay
node test/t467/page_units.cjs .codex/local-test/t467-replay
node test/t467/build_gl.cjs .codex/local-test/t467-replay
```

在生成工程内用 hvigorw `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon` 构建。其余三个来源分别替换最后一个生成参数为 regular、t410、t437；输出目录必须未存在。签包与验签沿现有 `test/t257/sign.py`。软件 GLES host 复用现有 ANGLE/SDK shim；运行验证注入控制时钟，inflight轮询不按poll次数制造8.4ms，不能把软件JIT wall或该注入时钟当设备期限/fps结论。期限和真实GAP另由生成bridge控制桩检查。

`recovery_v3.h` 保存撤权、RESET_REQUESTED、RETIRING_OLD、CREATING_NEW、WAIT_NEW_SESSION、READY。显式新空场同步source OFF、撤旧permit、取消RESERVED、以RUN_RESET冻结ARMED并保存旧账。GL owner选择完整旧context销毁路径，对未完root和每个lease登记tombstone，退出/销毁失败保持退休，返回完成才允许新资源交接。新资源绑定epoch/window/run/context；新frame绑定resetSerial和pageGeneration，资源完成后才新建session、从seq1接受真实回调，首pulse建baseline，后续pulse产quota。旧通知、旧frame、旧token/end不能恢复旧run。窗口/后台变更取消事务，页面显式再请求；没有定时器伪心跳或旧permit续期。

两页newRun、冷clock选择、profile/optics的新空场均重建frame。正常页面生命周期使用相同helper；过期排队回调永久stop。原begin/end接口仍存在，但begin不能绕过资源/退休交接或在READY恢复同run。

启动诊断导出保留T456原首错并附加recovery对象：512条固定事件、溢出incomplete、实际tid/观察角色、原域UI/native时间、anchor旧新值、session/epoch/permission、root/plan/deadline、旧账和Flush enter/exit。inCall独立锁存，满表也能导出未返回调用。读侧无GL，首错后只附加只读证据。

交付报告为 `test/reports/t467/delivery.json`。控制检查23/0，实际两页frame/helper12/0；实际生成App的离线GLES完成V2 READY→SOURCE/QUANTUM→Swap、松手J→W和新run恢复。四档构建零错误、独立签包验签。真机恢复、GAP实际域、250ms设备成本和30fps由T473/T410/T437另测；本卡不声称设备通过。
