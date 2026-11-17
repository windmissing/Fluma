# T560 / T563 工程复盘与交付

2026-11-17，architect；不使用真机。两卡均完成当前工程范围，不代表新管线已经实现或产品通过。不修改业务代码、历史报告/原件、暂停的目标10或CPU排期。

T560交付 [分层资格与Lab归因](architecture-T560-stage-capability-and-lab-triage-v1.md)、机器合同及原件SHA审计。原Create把compute和packet vertex SSBO要求混在一起：headless必须独立资格；可见packet改用同GPU CURRENT buffer整数实例属性，零额外payload/dispatch，真实scene/present仍需独立设备证据。T541报告的vertex0/lanes/fragment没有独立完整probe stdout，按报告观察保留，不能凭源码打印格式补证。

Lab队满故障第一次拒绝是预期注入，但释放worker后立即Drain会再次争用128容量队列，源码存在具体竞争窗口，旧FAIL不改PASS。缺栈不冒第二抛点实测；新test-only排空需原子等位/入fence、5s/cancel/RAII。原件仅4对FILE/SHA，PIN/SCAN_RESTORE是记录而非文件；“六阶段各有文件”作为证据清点修正报manager，不把缺失文件捏造成原件。V1 Export仅Poll，已冻结V2私域同进程有界文件页传输和实际文件SHA核验；开发完成不代表文件已实际取回。

T563交付 [目标11工程闭包](architecture-T563-g11-accepted-flux-visual-v1.md)、组合manifest、两个完整dispatch表、明确SHA视觉配方和有限检查。两个关键接缝：FLOW_BEGIN必须早于FLOW_ORIGIN_RESERVE，才能保住被scratch覆盖的真实Hamilton份额；SOURCE replay旧map必须取BASE。TOUCH同批recruit→launch须用已封转换求中间质量，再筛真实来源前缀，不能用最终空F把合法S→F→P丢掉。三视觉bank与父物理CURRENT在同一完成合同原子发布，单glyph WORK依真实scene fence背压，不偷添缓冲。

资源/计数：8个父版本/K/V组合逐段核算；V2048/8192分别新增3538944/6094848B。最坏QUANTUM R1=2543、DM=2923、SOURCE=19，四binding不变；最大DM K512 V8192已知并存包络632474752B，单phase witness预约后641920128B，driver/FBO/host等另计。分配/编译/250ms实际可行性仍NOT_MEASURED，不能因表能闭合就推定设备实时。

本角色检查：`python docs/analysis-T560-T563-engineering-checks-v1.py`完成52,650个rank分区/镜像关系、8,192个投影误差关系、成对seed、控制/字段布局、完整父pass去钩子等价、4binding/封印最后与全部资源计算。仅有限规格代数和源码审计，不执行GPU/GL/lab/产品测试，不新增CPU物理oracle或stepper。

接手：developer按工程稿完整开发/编译/提交，不执行测试、不使用真机；真实父R1/DM实施未齐具名报告前置，不停在零碎准备。tester独立取完整capability stdout/分层shader资格，Lab新case outcome和原字节文件，G11 V01–V14、单phase witness及同输入ON/OFF物理不变；effector按同accepted区间量可见流动、停流/反向、重投影与相转化闪跳，不用漂亮录像代替状态正确。测试失败仍先architect归因，manager登记推进，不新增验收门。

执行者只更新T560/T563本人行状态和原卡评论；不更新整份清单、滴答同名清单或其它卡。
