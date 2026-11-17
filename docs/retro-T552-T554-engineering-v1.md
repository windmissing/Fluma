# T552 / T553 / T554 architect交付回执

2026-11-17。不使用真机，不运行GL/NAPI/产品测试，不写业务实现/CPU物理stepper。三卡约定的工程补齐、证据归因和独立v2工程设计均已交付；仅本人清单行状态例外，不改manager队列/滴答同名清单或tester报告。

T552：两项是此前architect合同缺项，已明确承担和补齐。`architecture-T552-v5-wire-and-g9-seed-v2.md`及V5机器表逐项展开四profile ROOT7/FULL61段的kind/rev/rootIndex/role/trial、动态branch/冷arena/generation、record静态引用、planned/sealed/未执行规则。正式确认目录内kind=index+1；实际V5 SOURCE/CONTROL rev5与旧generated rev4错配也须开发同源对齐，PHYSICAL64无通用末word seal不可误写。FULL scope原cursor入口/末尾从真实切点保在原8192B/root控制保留尾，额外40次/5120B copy而无新增GPU payload/dispatch/热尾。T535 mask/容量预算不变。

G9 R2明确列9=40Q、parameterRevision3/seedRevision2，全部template/config/parameter/actual SHA已重算到 `architecture-T552-g9-seed-manifest-v2.json`；不删除初始审计，不hash后补缓存冒原SHA。T517/T548两个原卡对应分支可分别接续完整范围，不互等；原partial报告仍真实，T544仍需完整adapter与正确签包。原T527/T535/T543工程MD加修订引用，旧R1机器/脚本和T549冻结基线不覆盖。

T553：`architecture-T553-ime-root-and-lossless-evidence-v1.md`及manifest/证据分三域。IME UNKNOWN是复合谓词，报告对注入不弹IME的推断未作为唯一根因；交tester真实focus→系统键盘avoid/height→既有stopEditing/clearFocus准备，缺同进程请求入口才交developer `showTextInput`窄接口，绝不写假visible/清历史。系统PNG已有SDK UiTest Driver.screenCap精确路径，仍需实际runner/PNG魔数CRC/原件/同token时钟区间，JPEG±1继续非严格PASS。

T537生成链和实际签包ELF已含T536八族，不能再把其未交当前置。T510原件Down→Offer=18,633,594ns、五条保留Offer为ENQUEUED_NOT_ACK；同档ROOT_ACCEPT计5153、末root5121已接受。最终root5122 firstCause为CLOCK_GAP，根开始到撤权41,380,625ns、间隔34,047,344ns而非250ms超时；撤权距Down约311.790s，不能冒输入当刻首错。准确缺口是同intent/enqueue到G1 Claim/SOURCE消费关联，先复用诊断，确缺则定长窄见证交developer。原输入/位置已证保留，SOURCE消费/严格PNG/预约仍未证。

T554：`architecture-T554-g9-dynamic-mobilization-v2.md`及manifest/完整2404项dispatch表冻结独立g9dm/profile/magic/config/合法seed/HEADER106..113/48wordWORK、动态后混合隐式32轮、来源连续前缀WAIT/硬错、中点私有预测与entryP真实一次力/计时、原能量CFL门、两新EVENT64和逐字replay、审核0x3ffff/来源0xffff N/A及冷reader。新resident K192/K512=55,339,648/98,022,528B；两FULL native最大376,119,296B，已知并存小计614,190,208B不含driver/FBO等。最坏875520 Eval/2626560宽除，GPU成本/物理效果未测，不承诺实时。

运行 `python docs/analysis-T552-T554-engineering-checks-v1.py`：T552 311项、T553 31项、T554 74项，PASS_FINITE_ENGINEERING_ONLY。核逐段连续/身份负例/paired游标/原seed前像/新cache及SHA、原包/ELF/原件算术、全部pass/资源式/event mask、32轮抽象前缀及小域Eval/RNE。局部数学检查不是完整stepper或独立产品oracle；未重跑T556或真机。三份证据JSON均保存源指纹和未测域。

manager可直接登记交付、推进T517/T548及T555工程前置并按T553分流，不加manager验收门。developer仅开发/编译/提交、不执行测试、不使用真机，独立验证另由tester/effector；失败先architect。所有后续新XComponent+shader唯一上屏，故障FAULT或标明STALE上一有效完整帧，禁止Canvas/软件GL产品回退；CPU停投、默认OFF、250ms/GAP/120Hz配额/30fps显示和旧产品门均保持。
