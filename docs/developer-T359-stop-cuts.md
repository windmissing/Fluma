# T359 停源步进切点诊断交付

2026-11-05，developer。不使用真机。开发、离线自测、ArkTS/native编译、打包、签名及verify-app均已完成。设备冻结的首次分歧仍由T360补证、回T353 architect归因；本卡没有猜测或修订物理行为。

实现位于 `test/t359/`。生成器在T351当前源码闭包上复制独立工程，逐接点唯一匹配后记录patch，不覆盖共享树WaterSandField/FrameBudget或其它在途修改。公共生产入口未改。前景背景继续使用原XComponent+shader；原渲染失败/STALE语义保留。

`StopCuts`跟随本owner的现有budget，三稳定ID、256行×48F64固定环及DataView编码器冷创建；热路不导出全池、不JSON、不新增日志。环覆盖明确累计overflow，导出只包含最近窗口，不能据此声称保存了完整8秒序列。导出同时包含原T351帧/子步budget，使用原attempt/substep、Q和回滚delta关联。

原物理入口和每个子步入口记录canonical-before；原motion plan记录predicted（不存在该ID计划时present=0）；成功transfer后读canonical；publication准备前、实际切active bank后、consumer slot完成payload复制后分别记录。ID消失只在canonical-before判定退休，独立retired行后才选新AIR ID，不把相同slot的新ID当旧粒子。中间切点的缺席只标present=0。每行pY/pVy为两个U32 raw words，负零保留。

字段含run/epoch/commit/page frame、sourceEpoch/admission sequence、held/active、previous/current raw ns、clipped dt、accepted elapsed、attempt/substep/acceptedTilt、motion/request/apply/write计数、原失败回滚原因及step回执。elapsed按原回执成功子步数×substepDt求和，含部分成功原step，derived fault不计；不由commit差推模拟秒。field失败rollback阶段读恢复后的canonical。T351原完整计时/子步rollback delta在同一冷档，不能把本诊断field失败次数当所有preview恢复次数。

stage枚举：0 before、1 predicted、2 afterApply、3 prepublication、4 bank、5 slot、6 retired、7 failed-substep rollback、8 public step receipt。stage8的ID=0，是命令回执而非粒子；48词中fields数组指定前37词，其余预留。step最终累计elapsed/Q在stage8读取，中间切点的elapsed为当时已结算public step前缀。requested/actual token在原绘制返回后回填**同一page frame**的有界行；早退未发绘制请求时保持空token，不能当CURRENT。一个page frame可含源和多个子步提交，所以每行cut commit仍独立，不能用帧最终token替代切点commit。

页面“导出帧预算”按钮冷保存 `{cuts,budget}` 到原 DiagnosticColdFiles 目录。正常热路零新增短行，满足每秒≤1限制。源停止不停止step；source clock和dt原语义均不变。源run/reset后应按run/epoch分组消费，不跨run拼接轨迹。记录容量不足时保留overflow，不重建被覆盖原件。

离线7项：180步ON/OFF完整checkpoint/acc/cache原字节对照；全部六切点出现；环覆盖；ID换槽复用退休；同frame requested token；注入原audit=5后的首错/恢复checkpoint对照；负零raw words；真实model DOWN→停源→31次step成功elapsed/Q回执（前两项合为一测试项）。时钟固定仅用于离线剔除耗时差，真实包未改时钟。没有声称复现设备冻结。

复现：`node test/t359/generate.cjs .codex/local-test/t359-NEW`（新目录）；`node test/t359/units.cjs <新工程>/entry/src/main/ets`；在新工程用现有DevEco hvigor执行 `--mode module -p product=default -p module=entry@default default@CompileArkTS default@BuildNativeWithNinja default@PackageHap`。按已授权T257本地签名入口 `python test/t257/sign.py T359-NEW <新工程>`，不更改签名材料、不用机。

实际工程 `.codex/local-test/t359-handoff`，签包/source/所有生成源文件hash及构建日志见 `test/reports/t359/delivery.json`，7/0与签名原回执分别见units/sign。与T350旧包SHA完全不同，T360须登记此新包身份，不冒旧窗重建。只交付开发诊断，不添加manager验收门或真机前置。
