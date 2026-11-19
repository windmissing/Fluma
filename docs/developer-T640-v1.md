# T640 developer 交付

2026-11-19，消费T635；不使用真机，不执行测试。完成源码与编译，运行行为未测。

- V3 RUNNING/CANCELLING/DRAINING/FINISHED；唯一worker在RAII资源真实退出后封存terminalNs、outcome、firstFailure及recordsSHA。Cancel有REQUESTED/ALREADY_REQUESTED/ALREADY_TERMINAL/STALE_JOB和原生调用时刻，完成后不改旧records/hash/outcome；worker终态锁决定竞态。Poll只读且附pollSampleNs/terminalNs及pendingResources。
- Build填充、哈希4096word、页、诊断阶段、cache提交和reader屏障均有取消检查。CHECKPOINT_BARRIER提供原生BARRIER_ENTER原件，Release/Cancel真实唤醒，不增加生产强BUSY。清理期间保DRAINING；不unsafe detach、不因5s超时声称物理I/O已退出。
- Lab所有排空改接test-only DrainForTest：同一绝对5s deadline覆盖同锁等位、插入和fence等待；Cancel经共享Store注册与同Store mutex通知，回调持shared completion至执行。记录queued/fence sequence及enter/insert/pop/return原生时刻。退出明确RetireForTest真实join，之后才能FINISHED；生产Enqueue/Drain和128队列容量保持。
- FilesV2 base64使用String，4096B页/16KiB JSON不变。Begin部分open回滚RAII、错id End不清其它reader、新job拒活跃export并在退休后清catalog。checked index/fileId，O_NOFOLLOW固定allowlist打开；每页前后对pin fd及路径核dev/inode/size/mtime/ctime纳秒和完整文件SHA。超时不续；异常/IO失败/JSON溢出统一close。collector可按冻结index SHA复核完整文件。
- 页面500ms刷新并显示UI采样和native poll/terminal原件；研究合成域不冒物理或GL产品通过。

固定metadata编译计账：ARM64 Lifecycle176B、atomic bool1B、Store共享引用16B、DrainCompletion16B、DrainObservation48B、Store sequence8B，加64B有界共享控制/回调信用共329B（≤1024）。sizeof由实际ARM64 clang record-layout编译确认；64B为分配控制/回调预留信用，不冒对象sizeof。文件目录独立192×32=6144B（≤32×256B），records仍256×256B，原58993832B payload包络不变；cache128MiB、线程栈/分配器不冒包含。机器报告列出逐项sizeof与信用。

完整隔离生成 `.codex/local-test/t640-delivery`，最终hvigor native/ArkTS/unsigned assembleHap BUILD SUCCESSFUL，13.877s。首次全量54.017s。日志与源/包/ELF SHA见test/reports/t640/development.json。编译不证明取消竞态、5s、TTL或文件突变测试通过，交T587 tester独立harness继续；本卡不以设备为关闭门。
