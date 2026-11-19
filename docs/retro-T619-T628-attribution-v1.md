# T619–T626、T628 架构归因复盘

2026-11-19；architect；不使用真机，不执行产品测试、构建、签名部署；九卡当前约定架构范围均完成。每卡正式稿为architecture-T编号-v1.md，原件SHA在对应evidence-v1.json；有限合同/原件连接检查见analysis-T619-T628-contract-audit-v1.py及JSON。不是产品PASS。

| 卡 | 完成交付与接力 |
|---|---|
| T619 | 公开rootCursor64逐word、2920B token前像及1024word配置胶囊，T527/T535/T552/T614同步。原T596续实现；不另建同一开发卡或设备门。 |
| T620 | 原三臂有真实GL sink且末尾各有411未EXIT，不能排除driver；独立修CREATE_ONLY握手/测试屏障及900s总限采证，tester先行。 |
| T621 | 原生raw length=UI元素数×4，匹配官方导出接口字节单位差异。绑定provider能力后产品reader单位适配，tester原T590续测；不盲除4。 |
| T622 | 原进程死亡不拼接；新M0收据不是M2加载证明。已有无App锁Create只读接口先用于持续外部采证，tester先行；真正通道缺口再最小产品seam。 |
| T623 | 无同代Down或非空SOURCE；给真实触摸路径和分段观察seam，tester/必要developer，不预断只能真人，不用合成冒真实。 |
| T624 | clang/lld等待无栈证据，R-string根因未证；全新目录、命令/对象/响应文件指纹及有限超时最小复现，tester设施先行。 |
| T625 | 原始API错缺在报告而不必缺在产品；已有cold API records先提取，两个漏记录catch最小具名补口，保disabled/严格PNG。 |
| T626 | 商用平台proof仍缺三项；厂商bundle/可控系统/新时间语义三路线明确责任。无平台输入不让T616空转，不自降门。 |
| T628 | F02/F05覆盖、F03/F06/F07参数公式、F11缺alias hook、F12离散账修订；公共17/math bitset和exp总门明定。tester与必要developer接力，不阻T609/T610可测域。 |

这轮最重要的改进是先核原件字段和实际构建源码：callId不能误读为callSeq，UI元素数不能冒原生length，安装包SHA不能冒进程加载，通用reason不能替代API原错。下一轮报告需逐项给同身份连接及原始计数；少一环即标准确缺证，不推导广义平台或driver结论。

T619对未知编码先正式定义再实施，保守拒绝是正确行为；新增固定metadata连pending/accepted共7实例计账，不能只算5保留root。T628新语义SHA显式升级，不追改历史包/报告。全部后续继续GPU驻留、CPU停投、XComponent+shader唯一上屏，目标10暂停、水线13→14→12、旧湿M1暂停。仅本人九行状态回写和原卡评论，不维护manager全表/滴答清单、不创建或关闭其它卡。
