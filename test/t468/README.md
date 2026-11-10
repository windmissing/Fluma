# T468 · M2 创建阶段原始诊断

按T462，不使用真机。`generate.cjs` 从T452独立M2生成来源派生，接入实际 SOURCE_V3、QUANTUM_INTERMEDIATE、QUANTUM_FINAL_V3 三条创建链。没有删除中间program，没有修改shader数学、wire、Root/READY判定或提供软件GL产品回退。

```powershell
node test/t468/generate.cjs .codex/local-test/t468-replay
node test/t468/build_units.cjs .codex/local-test/t468-replay
node test/t468/page_units.cjs
node test/t468/build_gl.cjs .codex/local-test/t468-replay
```

在生成工程内用hvigorw `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon` 构建。软件GLES复用现有ANGLE与SDK shim，仅是非机参考。实际helper21/0、声明/原字节导出11/0；真实生成App冷Create1/0，核三program shader SHA/字节数、compile/link日志API均实际调用、context实际15项limits与ProbeCurrent原值。空log仍为有效证据，不要求非空。构建零错误，签包验签PASS。

产品helper `gpu_resident/m2/create_diagnostics.h` 使用原owner/thread/context执行冷GL，固定3代快照、96项事件、每program4096B raw日志，总额≤16384B。每次GL调用前写enter，调用后原rawError锁存；首错不可由清理或新代覆盖。读侧只复制/序列化，不调用GL；阻塞compile可见inCall及真实耗时。旧代可用serial读，超过固定保留窗口明确拒绝。

原glGetError决策点通过Consume恢复诊断消费的首错，成功status不能把已有GL错误洗掉。日志查询自身错误单列，不冒充compile错误，也不新造旧路径失败。Reflection保留原128项/96B名字谓词，明确记录n/capacity/index/nameLength，分别测试三对象的compile/link/reflect失败。raw日志在对象删除前读取，reported/copied/truncated/queryError/empty均独立。shader warning不作FAIL。

页面“导出 M2 创建原始诊断”和“导出上一创建诊断”均无需ARMED/trace成功，产生native.json、3份原字节.log及bytes/SHA收据manifest。空日志也保存0字节文件。原字节以hex跨NAPI，不经UTF8转码损失；metadata JSON仅安全转义已定义字段。换代/缺页拒绝组合成功导出。

T472步骤：

1. 使用 `test/reports/t468/handoff.json` 所列最终签包，核HAP SHA、验签收据及压缩包中libgrsm1.so的ELF SHA。按既有tester安装/冷Want流程传入其中 `grsm1/gpusha/gpuelfsha`；这些runtime字段明确是外部声明，必须与host验签收据匹配，不伪称应用自证签包或自哈希ELF。
2. 一次冷启动进入M2空场，即使TRANSPORT_CREATE #6也点创建诊断导出。保留manifest、native JSON、3个raw日志、完整布局，不先要求root/冷trace/ARMED。
3. 明确“新空场”再取一次当前诊断，并可导出上一创建诊断。bootSerial/epoch/run/context/window分别核对，不能把同码当同阶段。目录/日志receipt输出沿现有WS_EVENT文件取证流程。
4. 先读firstFailure的object/stage/rawGl，再看该对象source SHA、bytes、reflection参数和raw日志。reportedLength=0且queried=true是实际空log；queryError非0是查询失败，不能转写“驱动无日志”。numerical support不等于compile support。
5. 带真实driver/limits及上述原件回architect分流。此包是诊断交付，不声称设备SHADER_FAILED已解决，也不提前宣告M2整根/效果/性能通过。
