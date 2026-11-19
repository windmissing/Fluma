# T668 离线设施与交接报告

2026-11-19；tester；不使用真机。构建/离线设施交付，不表示设备、物理或性能 PASS。开发编译基线 8fe7c2e65565350b2013dd466bb64dce189b2351；本卡新增工作区按随附源/对象 SHA 收据绑定，提交时一并入库。

完成独立NDK fault-build Store/屏障 driver及FilesV2判定脚本，链接实际T640 Store/Lab，不复制其实现为mock。test/t668/CMakeLists.txt 实际ARM64 NDK编译通过，源码、对象、工具/argv及独立ELF见 native.json；driver原件 .codex/local-test/t668-ndk/t668_driver。test/t668/build_host.cmd 同一Store host检查通过：满128队列后第129个CACHE_QUEUE_FULL、等位取消、插入fence后取消、固定单次5s deadline不pop、pop后fence覆盖全部128工作。原生 enter/insert/pop/return/fence sequence 见 store-host.txt；实际墙钟调度值原样保留，不用5.5秒替代5秒合同，也不将host延迟叫设备PASS。

ARM64 driver另提供 barrier_cancel（真实BARRIER_ENTER后两次Cancel/FINISHED后Cancel、RAII退休）、files（BUSY/错id End保原reader、完整index与所有4096B页、错index/offset/fileId）、files_ttl（真实等待61s）、files_mutation（同大小改原件后FILE_CHANGED）。这些设备执行 NOT_RUN，交effector，不伪造通过。files.py 严格base64、连续offset、各chunkSHA、总字节/原fileSHA，宽exportId不转浮点；7项离线正反控通过。

原t640-delivery当前unsigned与历史收据不符，先拒绝复用；新建完整隔离源码/缓存，只接实际Lab+唯一broker及专用T668LabAbility，避免无关旧Stats501编译接口和旧DSO残留。fresh t668-lab-clean 全量编译44.182s、34任务；只包含Lab和c++共享库，verifyApp PASS。签包及完整来源见 test/reports/t668-final/handoff.json / test/reports/t257/T668-final.json，不能使用先前缓存工程T668签包。可复跑 prepare.py <fresh> 后正常assembleHap；保旧污染输出和原错，未改SDK、全局cache或任意去重flags。

交接T587 effector：独立NDK驱动及对应libc++部署到专用可写测试目录；逐次串行运行 full_queue_cancel / fence_cancel / full_queue_pop / deadline；barrier_cancel <absolute-files-dir>、files <dir>、files_ttl <dir>、files_mutation <dir>。每个使用新目录和单进程；普通driver总预算30s、TTL80s，外部超时只能记录INCOMPLETE并保stderr/退出码；5s返回不意味着内核I/O强制终止。收stdout/stderr原件后 python test/t668/files.py files.ndjson 联核，不替换生产Enqueue/Drain。所有均合成资源设施，不冒物理或GL产品PASS。
