# T507 GPU 冷档生命周期与有界序列化开发交付

2026-11-16，developer，不使用真机。按 T503 正式工程稿启用独立 `T503_BOUNDED_COLD_V1`，trace为 `T503_M2_COLD_TRACE_V1`、payload仍 `T445_M2_TRACE_V3`，diagnostic为 `T503_EXECUTION_DIAGNOSTICS_V1`。通过 `test/t507/generate.cjs <新目录> m1|m2` 生成交付工程。基线沿 T502→T501→T492 增强 delivery，生成 manifest 已核 execution_diagnostics.h SHA256 与 T503 冻结的 `4493f69416e8af858a2a932e469094b9e474d6300ac6107d6410e50956411ac5` 一致。物理 wire 与热 SOURCE/QUANTUM、lease/scene、snapshot协议沿原路径。

`cold_protocol.h` 提供 checked UTF8 writer、固定 scope、gate、RAII 和协议常量。`cold_trace.h` 提供每 App 两 slot、单 builder、单 reader、单调 request/trace U64、32条 terminal、request起60s TTL、scope换代撤销及原子替换发布。全manager容量上限55,410,688B，单profile最大27,705,344B。FREE backing仍计capacity，先信用预约、锁外释放旧FREE backing后再分配。owner在真实root安全点 freeze，caller-owned精确block直收completion/state/events，无额外生产Capture vector。失败候选保旧published，读pin跨替换/End/reset/TTL进入RETIRING，回包前重核scope并释放。Map/unmap及restore失败保原阶段/错误，取消builder归来仍先完成恢复。

`cold_diagnostics.h` 在Capture复制前使用单export gate，Capture静态上限1,404,072B，writer capacity等于实际剩余预算、最大2MiB。所有append和escape先检查UTF8信用，无增长stream。超限清构建长度后写完整INCOMPLETE小envelope，四scope的firstCause/faultRoot和原始GL/EGL/stamp保留。RAII覆盖异常/NAPI失败，copy/writer分配失败使用固定小fallback。scope变化明确标旧scope/stale。新API直接从writer span创建NAPI字符串。trace Page与diagnostic有各自单reader/export，不混计为同一个许可。

两页面实际 SnapshotPort/collector 已迁移到新capability和diagnosticAPI。M2新trace逐页保存，不在JS聚合完整state/events。页≤1024U32/16KiB，独立trace归档按profile页数预算；timeline+diagnostic总UTF8≤4MiB、diagnostic≤2MiB、原timeline30s沿用。坏页、scope变化、超预算、I/O、cancel/hide及最后manifest失败都进入finally释放snapshot或trace。旧在线requestM2Trace/readM2Trace/executionDiagnostics明确返回UNSUPPORTED_COLD_REVISION，旧离线V3档案/reader保留。M1 trace明确UNSUPPORTED_ROUTE，M1 diagnostic可用。

实际native helper检查28/0，覆盖slot/pin/替换/失败保旧/TTL相等及慢读/clock倒序/End builder/跨session/ID耗尽/四profile最大counts/容量/坏页/gate/BUSY与分配失败。writer完整诊断在恰好limit时COMPLETE，少1字节时INCOMPLETE，四份firstCause和faultRoot经JSON解析逐字段相同。UTF8、最大U64及最坏escape fallback原件已留。fallback-worst为13,218B，Capture实测1,404,072B。

collector检查17/0，使用实际生成ArkTS类、真实native QUANTUM trace页及既有不可变snapshot原件，覆盖完整流式导出、坏页、I/O、cancel、未知schema、manifest失败、busy/budget/stale/超UTF8/NAPI异常路径及finally释放。文件端口为故障注入fixture，实际生产页面迁移另作接线核对。`check_contract.cjs` 核机器manifest常量、增强来源和旧API拒绝。

真实ANGLE离线M2 App运行使用实际生成owner/transport与生产manager，SOURCE128和QUANTUM64分别封存、分页导出，逐页核16KiB/U32数量、原scope/traceId、总页数/字节预算。caller span与原Capture结果逐U32比较，容量不足、completion超限和map/unmap/restore错误检查位于同一owner安全点。map错误目标、unmap false和解绑context为标明的故障注入。首次fixture在新root已活跃时追加Freeze被BUSY门拒，已将自测移动到ServiceColdTrace的真实安全点。产品协议没有放宽该忙态门。离线GL只是开发自测，设备长测另由T514等独立任务接续。

M1/M2最终生成应用均ArkTS/native/PackageHap构建成功，今日增量复跑也BUILD SUCCESSFUL。签名verifyApp均PASS。完整生成TU/helper/页面SHA、HAP/ELF、原件和用例见 `test/reports/t507/`。M1签包 `.codex/local-test/t257-sign/T507-m1-complete/T507-m1-complete-signed.hap`，SHA256 `a1baa57784e1d883fbe823b12c38aac4abb59c44f0cde4fe29563086bcf08d2b`。M2签包 `.codex/local-test/t257-sign/T507-m2-complete/T507-m2-complete-signed.hap`，SHA256 `b702032f6fb34c2818fe457daf8f7e8c6ed36fc9fd120eb2d0d723e6a19dc1b6`。构建保留既有编译警告。

在仓库根复跑，输出目录必须为新目录。

```powershell
node test/t507/generate.cjs .codex/local-test/t507-new-m2 m2
node test/t507/build_units.cjs .codex/local-test/t507-new-units
```

生成工程用现有DevEco hvigorw执行 `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`，JAVA_HOME用DevEco jbr、PATH加其bin。必须先构建生成真实shader头，再执行GL自测。GL脚本第四参数为空字符串表示无cache seed，第五参数为m2route。

```powershell
node test/t507/build_gl.cjs .codex/local-test/t507-new-m2 .codex/local-test/t507-new-gl '' m2route
node test/t507/page_units.cjs .codex/local-test/t507-new-m2 .codex/local-test/t507-new-gl .codex/local-test/t507-new-pages.json
```

`collect.py` 固定核对本次交付路径并归档来源、签包、native/collector/真实GL证据。单元里的ALLOCATION_FAILED和NAPI异常为明确注入，未声称通过设备allocator/RSS/driver峰值测量。T514独立tester接手后按T503八组合×32cycle冷档子集核所有slot/reader/builder/owner局部capacity及native/JS分账，M1应拒trace且支持diagnostic，M2核SOURCE/QUANTUM及scope/TTL/取消/替换。保存新schema原件及真实device能力/错误，容量计数不能当RSS立即回收证明。foreground/background继续XComponent+shader，失败沿FAULT或标STALE完整旧帧。真机、效果和性能结果尚未验证。
