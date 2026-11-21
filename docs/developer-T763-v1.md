# T763 · RX SOURCE 同锁时钟与首拒诊断

RX 研究变体在 protocol mutex 内先取已发布 PagePulse、队首、scope 和 permit，再读取真实 `CLOCK_MONOTONIC`。Begin 在原 scene fence 检查之后取样。未来 receipt 记时序失败，只有确认真正超过 250 ms 的队首才走原过期取消。PagePulse 的原 NAPI receipt 决定 freshness，publish time 单列。Target 不刷新 PagePulse 或姿态时间。

Begin/Pump/Proof 的 owner 检查短暂持锁取样，EGL 和 GLES 调用及等待留在锁外。提交预检和晋升走已持有 protocol mutex 的独立入口，提交前再次采样，没有重新进入锁。真实 context/owner、原 typed proof、FIFO、只消费一个 SOURCE、33,333,333 ns freshness 和冻结 250 ms deadline 继续生效。

`保存原件` 会先保存 `g9rx-source-*.json`，成功时随后保存原冷档。时序 JSON 有独立首拒、最近成功、原码和最新游标。control、transport、pulse 分别保留 160、384、96 个条目，报溢出数量并保留最新条目。阶段记录包含 Configure APPLIED、安全 extent、SetActive、入队、首 pulse、owner 唤醒、Claim/Freeze、上传、逐批 Pump、copy/dispatch/barrier、flush、执行 fence、WORK map/unmap、提交、Stop、退役和可用时的 XComponent Swap。原始时间、对象与 API 返回按相应阶段解读。退役等待参数区分沿用执行 fence 与新建清理 fence。

## 工件与设备续证

以[编译收据](../test/reports/t763/compile.json)中的未签名 HAP 签包，核包内和实际映射的 ARM64 BuildId。启动原 RX 研究页，点击一次「启用研究」。同一 ticket 保留 `G9RxConfigure`、`G9Boot`、`G9RxSource`、PID/start 和角色身份。页面出现终态后点「保存原件」。发生失败时照样取时序文件；若清理仍在进行，待同 scope 退役后再保存一份。成功再核原 1024 B 尾、209,715 Q20、typed commit、FIFO、冷档和实际 Swap。第一拒绝的根因按新原件判断。

隔离重建使用 `node tools/g9rx/prepare_build.cjs .codex/local-test/t763-rebuild --boot-source`，随后在隔离项目运行 `hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`。本轮未签包、未用真机。 [T759 工程合同](architecture-T759-v1.md)
