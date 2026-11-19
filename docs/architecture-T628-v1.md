# T628：G13 独立基准与数学错误口径裁定

2026-11-19；architect；不使用真机。消费T605正式算法/fixtures，T607四表，T606报告及test/t606原件，T608 dd91018交付及原评论6afdaa64e4b004b3f877fcf4。本次一致性审计不执行求解器/测试，不设T609/T610开工新门。T608已实现，T606报告的旧NOT_IMPLEMENTED仅为当时历史状态；当前未测不等于未实现。

## 独立基准逐项分流（tester修T606/T609）

| case | 正式合同与本次裁定 |
|---|---|
| F02 | 不只核D u=0。uStar由连续梯度给定，离散压力为(k/d_h)P cosπx cosπy，P=1/4；压力RMS≤min(2^-16,Etrunc_RMS/16)、max≤2^-12、空间阶≥1.8。它能证明Poisson/投影压力分量，不能代替完整NS物理压力；禁止直接将解析压力写入待测解。 |
| F03 | 正式nu=1/16、A=1/4、T=1/4、J=1/2/4、网格32/64/128，k=2π；T606的nu=1/64、率π²/16不属于此fixture。独立实现连续衰减和BE离散幅值，保数值误差界M*2^-16/J+gamma_(64M)A、速度1/64/能量1/32、时间阶0.8/空间阶1.8及nu倍增。 |
| F05 | 需要非零闭盒制造解u=a sin²πx sin2πy、v=−a sin2πx sin²πy、pi=P exp(−2t)cosπx cosπy，A=1/4、P=1/8、nu=1/64、T=1/4、J4；REST和RAMP_SPIN两组。T606仅旋转势/静止Coriolis归零属于覆盖缺口，不能替代此例。独立解析导数构造外力并核速度相对RMS1/32/max1/16、压力相对RMS1/16、空间阶0.8。失败后才按真实误差归设计/实现，不在无GPU结果时先判设计错。 |
| F06 | 正式启动级数U*y+(2U/π)Σ[(-1)^n sin(nπy)exp(-nu n²π²t)/n]；T606漏(-1)^n，数学错误交tester修。U=1/4、nu1/64、N128、t≥1/120；尾界U exp(-aN²)/(πaN²)，不能用旧N40/未知参数的3.1e-107。还需实际wall shear与steady/startup正式门。 |
| F07 | 正式f_x1/32、nu1/64、rho*=1、H1，连续流量1/6，不是T606的5.333；保离散半格偏置f_x h²/(8nu)、速度max偏置+2^-12、流量/剪切1/32。驱动力非周期势，不冒压力求解非零结果。 |
| F11 | 正式五负控是normal wall、RHSsum、gauge、pressure sign、periodic alias duplicate。产品现有1 PRESSURE_DISABLED、2 PRESSURE_SIGN_REVERSED、3 RHS_INCOMPATIBLE、4 GAUGE_OFFSET、5 NORMAL_WALL；缺alias，PRESSURE_DISABLED额外有用但不能冒第五项。工程补hook6见下。已有四项和其他case可并行，不等hook6才测试全域。 |
| F12 | 连续式不能多乘一个nu/mu；rho*=1时耗散2nu∫S:S。产品门是离散逐算子/实际midpoint功率、正功率不由负功退额、冻结120个accepted quanta窗口、无未来耗散信用；T606连续恒等式不能替代账本原件。 |

其余F01/F04/F08–F10/F13–F16也按fixture逐case引用原given/expected及T605 gates：每行期望必须绑定fixture输入SHA、网格/J/时刻/变量位置/压力均值规范、RMS/max分母和收敛比较对；不能把“order至少0.8”套到F02/F03全部域。舍入/构造/级数截断界分列，不能从总误差删掉未测构造误差。T605已冻结的数值/成本/资源门原样引用，不因报告说未定而新建宽松门；真实未定仅按具体字段提出designer问题，不能泛称阈值都未定。

## F11 工程补口（developer）

独立test-only构建增加fault="PERIODIC_ALIAS_DUPLICATE"→hook6；生产构建依旧拒test fault。实际注入在正常周期alias同步后、FLOW_AUDIT核验前，只破坏一个应等同的alias面或重复更新一次，使独立alias一致性探测必拒；不得在注入后再enforce悄悄抹平，不影响其它五种hook语义。记录注入scope/micro/面索引、expected/actual原字和故障身份；结果FIELD_RANGE35，FIELD_RANGE audit bit1不满足，且CURRENT/commit/tick不推进。若现有audit不检查alias，补独立相等门，不伪CHECKER_PASS；布局不增bank/dispatch，使用现有FLOW_AUDIT和诊断预算。test-only源码/HAP/shader SHA另立，production不带可由普通用户启用开关。

## 公共17与mathStatus的唯一映射

正式补充表`architecture-T628-math-domain-v1.json`修订T607数学语义，layout.errors既有17 NUMERIC_OVERFLOW保留，不新增公共NUMERIC_DOMAIN码。mathStatus是bitset：1除零，2非有限binary64或F32溢出，4 exp范围/零下溢/相对误差门失败，允许组合。公共17且同故障root的非零mathStatus→详细NUMERIC_DOMAIN；17且0→普通数值溢出；缺mathStatus→MATH_DETAIL_NOT_OBSERVED，不能猜。其他公共首因即使另有聚合math位也保首因，不替换为NUMERIC_DOMAIN。

T608 softDomain→fail(17)、ctrl[751]→micro[783]/cold已有真实来源，基本映射可保；cold当前mathStatus?NUMERIC_DOMAIN:NONE只表达数学观察聚合，不能单独冒首失败原因。补充firstError stage/root/micro与该次math detail同切点锁存，aggregateMathStatus另名，避免后续运算污染先发故障。保留原参数/soft值/舍入F32位、实际scope/微步与mathSHA，预算用既有control/冷原件，不随意占reserved word或改public error enum；具体首次detail如现有槽不足，固定host metadata另列信用而非暗填GPU字段。

精确内部接线冻结为ctrl[750]=firstMathStatus、[751]=aggregateMathStatus、[766]=firstFaultDetailSeal(0xC0DE1311)，均在已有gpuGlobal256块内，非ctrl[960..1023]保留零区。现有fail的原子首错误winner已经写[513]op/[515]scope/[516]J/[517]micro：拆成通用fail(error,detail=0)和failMath(invocationLocalDetail)，数学调用传17与本invocation累积detail，通用数值失败始终detail0；winner写750并封766，同一root尾barrier/fence后才可读。不拿已聚合751作为winner detail。所有重置514的边界同步重置750/766；751保root聚合，micro783仍聚合观察，cold另导firstMathStatus+seal+首因坐标。无需新增slot、bank、dispatch、读回量；本补充表登记内部字段含义，新mathSHA区分修订。raw argument若只能由原config/accepted tick确定，须标DERIVED并附公式/来源，不冒观测原值；实际保留的soft/F32原件按原字段命名。

另一个静态工程缺口：softDiv先判numerator为零再判denominator，故0/0会提前返回零、漏bit1。developer将分母零检查置前，保非零分母的0/b=0；这是源码可证分支问题，未证明正式fixture实际命中过该路径，不据此伪造GPU失败结果。

## exp 精度域裁定

`2^-20`相对门覆盖**每个被接受的exp结果及其实际F32转换**，不是“只要落在F32次正规范围就合格”。exp(-103)真值约1.85e-45，最近F32约1.40e-45，相对误差远超门，必须公共17/detail4；零下溢、溢出、缩放越域同样明确拒绝，不夹0继续计算。T608已做真实转换误差检查，保门，不需为例外放宽。soft算法近似误差与F32转换误差共同计入总2^-20，不能两者各用满一份门。有限检查见本批静态证据，不冒全域精度证明。

更新math语义后mathSHA/algorithmSHA应按T607规范重算并显式新构建身份；常数/coefficients、布局、dispatch、gates未改。旧T608构建保旧身份和开发完成状态，tester记录实际使用哪一版，不在文件上“补写”旧二进制SHA。T607生成器同步读取新补充表，避免重跑覆盖本裁定。

过渡期T609/T610可以继续使用T608 dd91018原包及其随包冻结合同：mathSHA=`ee12f34b54239a8c7c395e5aa30aadcd95683be9d4f72e8f92cc203e92f68829`，algorithmSHA=`9bbb3df8eee70de03e156de8d2ec919e37515c999e177f4bdf63d195d1cfd48d`；用T608收据/原T607版本复核，不能把当前表的新SHA配置注入旧包导致全域BAD_HASH。原包不具备新firstMathStatus封存或hook6的部分只记NOT_OBSERVED/NOT_IMPLEMENTED，其余按未改的T605门验证。新工程实现后单列新包身份验证新增域；不是等待新包才能开始全部验证。

交接：tester修独立公式/fixture参数与覆盖/原件，developer补hook6和首次数学原因绑定并编译（不要求真机）；F05真实失败先architect归因再designer，测试门若不合理才manager裁决。T609/T610按现有可验证域并行，争议/缺hook域记NOT_IMPLEMENTED或NOT_MEASURED，不宣PASS，不新设集中准备/manager验收门。

续作prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）或滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。具体阻断含未完成、原错/复现/证据/尝试、不能继续原因、前置与接手角色。开发只开发编译、不使用真机；tester/effector签名部署/执行采证，真机串行。GPU驻留优先、CPU停投、XComponent+shader唯一上屏，失败FAULT/显式STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12，旧湿M1暂停。
