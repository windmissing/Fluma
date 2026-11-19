# T638 developer 交付

2026-11-19；按T633完整配置/初始化生命周期修复，沿已交T629严格provider decoder及T632数学源码。只开发编译，不使用真机，不执行测试。

- 初始UNCONFIGURED；decode失败单独保存首错、实际typed kind/原始长度/归一化长度/offset/data/backing/API状态/provider profile/输入128word，不创建owner配置ticket。pulse/其它命令不覆盖首配置错。
- 配置实际入队创建ConfigTicket：boot/attempt/queueSerial/proposed Identity/config原件SHA/seed receipt SHA/cancelGeneration。首次owner处理固定proposed identity，deferred保同ticket与scope；WAIT_WINDOW不冒GL失败。新配置取消旧代，旧初始化fence先退休，再处理新配置，不能把新ticket套到旧tail。
- 独立ServiceLifecycle推进原有配置重试、初始化非阻塞Poll和退休，不依赖ACTIVE或业务pulse；实际Ready后才提交App有效scope和generation。SetActive返回具名CONFIG_NOT_READY/STALE_SCOPE；Controller与AcceptPulse核Configured、Ready、非退休及完整有效scope，未配置不进账。不改GAP/配额/算法门。
- G13InitDiagnosticV1固定32768B：4×1024B decode、64×256B stage、2×4096B原始log、4096B控制/summary；first+last、dropped/truncated显式。实际EGL调用ENTER/EXIT/API布尔结果、program factory route13 shader compile/link前后hook、已有capability/reflect/allocate/64/65/fence/tail/seal原字切点。无新增GL、glGetError、map或glFinish；没有既有原错采样的EGL/GL状态保-1 NOT_OBSERVED，不用0冒成功。
- g13Status与g13InitDiagnostics(attemptId,page)直接读固定快照，不向GL owner排队；诊断锁不跨外部API。Status≤16KiB（超额cold正文指向具名cold endpoint），诊断4页，每页8192B原件base64≤16KiB JSON。每页sampleSeq供采集端拒绝跨采样拼装；payload不是冻结GPU档，不宣同一瞬时四页。ARM64字段布局/flags/字节序详见developer-T638-init-abi-v1.json。
- 页面独立250ms查询初始化，Ready前激活disabled；firstConfigFailure单列，pulse不覆盖。surface callback仅入队不等待驱动。研究默认OFF，渲染边界和原物理管线保持。

完整隔离生成`.codex/local-test/t638-delivery`，首次全量hvigor45.796s；最终native/ArkTS/未签名HAP BUILD SUCCESSFUL **9.972s**。实际ARM64 record-layout编译确认InitStage256、InitDecode1024、ConfigTicket176、InitControl888、InitPayload32768。mutex及owner既有命令/ticket元数据另列，不冒包含在32KiB payload。报告test/reports/t638/development.json含最终源/合同/包/ELF与T632 pipeline SHA，旧T608产物不回写。

未执行reader、shader、测试或真机。初始化驱动返回、实际Ready、物理与显示结果NOT_MEASURED，签名部署和独立执行交tester/effector；不将其作为开发关闭门。
