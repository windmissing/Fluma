# T697 独立 Lab driver 交接

2026-11-20，tester。复用 T668 driver 与真实 Store/Lab。用编译期 test fault 接口在 Store 锁内发布 WAIT_SPACE 或 WAIT_FENCE；协调线程在夹具锁外发取消，去掉猜测阶段的 30 ms sleep。发布等待有界。Windows host 四模式通过，保 enter/insert/pop/return、实际 deadline 与 OS 偏差，128 项完成后才允许 full_queue_pop 成功。FilesV2 既有离线校验 7 项通过。ARM64 NDK 编译与链接通过。

交 T698 的实际 ELF 为 .codex/local-test/t697-ndk/t668_driver。test/reports/t697/native.json 记录 SHA、BuildId、对象与实际 SDK libc++。不使用 T668 初版 ELF 替代这一版。用独立进程串行执行，带 DIR 的模式使用空的绝对目录。保存 ELF/加载库身份、实际 argv、stdout/stderr 原件、实测墙钟和退出码。普通 30s，TTL 80s。外部超时记 INCOMPLETE。

- full_queue_pop、full_queue_cancel、fence_cancel、deadline 各单进程。核 WAIT_SPACE/WAIT_FENCE 再联核真实返回结果。
- barrier_cancel DIR、barrier_release DIR 先保真实 BARRIER_ENTER 原件。保存实际取消/重试/错 job 原返回、观察到的生命周期及终态再 Poll，逐字段检查封存结果不变。未采到中间瞬态按未观察登记。
- files DIR 保存实际 Begin/Busy/错 End、完整目录和全部分片、End/过期 ID、三条独立拒绝与清理。使用 python test/t668/files.py <原 stdout> 复核完整流。
- files_ttl DIR 在同 Begin 后约 10s 读一次，原 Begin 后 61s 检查 EXPIRED。files_mutation DIR、files_mutation_truncate DIR、files_mutation_replace DIR、files_mutation_delete DIR 每种隔离执行，记录原件与变更 SHA/缺失、FILE_CHANGED 和旧 reader 恢复。
- injected_clock DIR、injected_fault DIR、catalog_contention DIR 复用 Lab 的 TTL_EDGE_INJECTED、CACHE_FAULT、TWO_SLOT_PIN_REPLACE。保独立标签和原结果。其余未执行的注入边界逐项记 NOT_RUN。

设备 Lab/TTL/变体均待 T698 执行。此交付仅为离线设施、编译与 host/解析验证。
