# T655–T658 首次归因复盘

2026-11-19，architect，不使用真机。本批完成四卡工程归因与精确角色交接，没有执行产品、构建或测试；ELF元数据检查与保存原件/字段表核对不是运行PASS。

T655把长命令拆为SDK缓存flags、Ninja shell和已有rsp三层：broker只有一对象且已rsp，62/63组累积不能归T629 include或对象混入。先独立新输出/正确native argv，不全局删缓存、改SDK或去重有序link flags；历史compiler等待未排除，混树标本分名。只有干净合法目标仍超长才启用完整target driver rsp工程，不自动阻塞独立正确包。

T656按真实指针入参/清理顺序识别测试设施错误：ContextRetired前已Freed，随后非零gA复用在入参门失败，不能改变生产退役语义。38行两个19行片段/21去重字串不冒单run全覆盖；M4累计cap未测。保现有跨DSO窄事实，修测试矩阵和完整out值，无新产品endpoint/设计改动。

T657按完整调用键得到267 ENTER/266 EXIT/唯一开放call266，补保存program4及四bindings的传参，不把缺XYZ/源SHA/退出码的“进程消失”升级成driver硬崩。原C0时钟和shadow仍须完成，缓存source/role/time只有关联而无pid/attempt精确绑定；先tester完整异常/主动kill/退出原件，不延长等待或借旧411。

T658由当前raw JS栈与模板/生成源闭合JSON快照丢方法问题，采用method-free十字段私有值、显式equal和固定tuple key，保持same-ID readiness变化、旧回调discard、当前首错与票据消费安全。修真源/生成链，不只修local-test或吞异常。未把v1旧轮次无证强归同因，也不放宽reserve/PNG/期限。

四卡均未变物理、GAP、容量、FP或CPU停投/XComponent+shader边界，原业务/设备验证未行使部分仍保留。后继按正式合同给tester或developer，无manager/开发设备新验收门。
