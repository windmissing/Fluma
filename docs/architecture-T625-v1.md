# T625：PREPARE 观察异常的首因与补证合同

> 2026-11-19本次raw jscrash首因闭合见`architecture-T658-v1.md`：JSON owner快照丢失.same后由键盘回调调用触发TypeError，交developer纯字段不可变owner/比较/key最小修复。仅当前版本栈已证，旧v1两轮不自动归同因；原首错冷记录、reserve禁用/严格PNG与PREPARING期限均保留。

2026-11-19；architect；不使用真机。结论：FAILED且reserve disabled符合安全语义；现有报告没有原始API异常，不能确定window/avoid-area/keyboard中哪个失败。先提取现有同attempt冷记录；未覆盖catch的最小观测工程见下。T592尚不具备HOLD/PNG通过证据。

消费test-report-T592-prepare-hold-v1.md、test/reports/t491/t592a布局/日志和 `.codex/local-test/t598-final` 实际单权威页面、PrepareMainWindow、PrepareReadiness/V4。不是旧canonical页面。EntryAbility持主window，generation更换，不采用lastWindow猜对象；单一gate输出reserveEnabled，PREPARING之外停止观察。

## 真实catch范围

readPrepareSnapshot依次涉及refreshRaster、window身份、getWindowProperties、getWindowAvoidArea(TYPE_KEYBOARD)、组件getRectangleById('t463-xcomponent')、singleSand.prepareReadinessCold、isFocused/getWindowStatus及rectangle的size/windowOffset/screenOffset/scale/translate/rotate.angle。空window或代不符会返回unready snapshot，未必抛异常；组件尚未布局/形状字段缺失/窗口失效/API异常都只能列候选，不能当根因。

observePrepare的catch先fail(PREPARE_OBSERVATION_FAILED)，随后prepareApiFailure('window.snapshot',error,...)；后者保存code/name/message/prepareId/pageGeneration/时间。父类record仅要求currentId（active且同id/generation），**不要求PREPARING**，所以先fail不必然丢错误。原始异常可能已在`prepareCold`导出的`t540-prepare-<session>-<generation>-<id>-<state>-<observationSerial>.json`的readiness.report.api；不能在未读它前说缺产品诊断。subscribe catch也有window.subscribe具名记录。bound-reader catch和frame-observation catch仅generic fail，确有观察缺口。

## tester 先做的准确补证

不重试快点Reserve。下一次失败后保持页面/attempt，立即调用现成prepareCold导出，获取readiness.report、API records、frame observation原件及SHA，关联layout/hilog同id/generation/observationSerial。先读已失败两轮遗留冷文件，如未导出则明确NOT_CAPTURED，不能将第三轮错归旧轮。记录真实code/name/message、windowId/引用generation、生命周期时序、触发API/订阅名、keyboard hide请求与事件、UI线程回调时间。无错误条目则指出落在哪个catch与raw异常缺失，不笼统“未实现”。已有build的签名部署/补证由tester负责。

## 必要时产品工程修订（developer）

将snapshot大try分成具名只读stage：MAIN_WINDOW_IDENTITY、WINDOW_PROPERTIES、KEYBOARD_AVOID_AREA、COMPONENT_RECT、NATIVE_READINESS、WINDOW_FOCUS_STATUS、RECT_SHAPE。两个无记录catch也写同一bounded错误条目。首错误锁存后再fail；保raw code/name/message和真实抛错stage，Error取值异常本身具名ERROR_SERIALIZATION_FAILED，不能用字符串化undefined冒原错。

在UI线程读取window/component；回调闭包绑定创建时window引用+generation+prepareId，前后校验同代，过期回调只记discard、不fail新attempt。非UI调度需要显式返回UI，不能把UI API搬worker。错误上下文保window是否存在/实际identity及before/after generation、native scope和observe serial，不从exception文本猜线程。已有API历史须有界：本次补充最多首/末各一条原错，message UTF8各≤1024B、截断显式标记；沿页面冷诊断预算≤16KiB，超限dropped，热回调不写文件/等待IO。不得扩大ready TTL3000ms、伪造keyboard/avoid-ready、无异常时强行READY。

原始错误确认后分流：合法窗口API误用/shape访问→developer按具名合同修；窗口生命周期引用过期→developer；真实平台权限/能力缺口→准确平台前置交manager裁决；测试导出/操作漏采→tester。单有generic reason不足以派具体业务修复。T592继续真实fresh PREPARE→reserve→HOLD→严格PNG与标定，禁JPEG替代/快点绕过/放宽指标；开发关闭不加此设备门。

续作prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）或滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。阻断须含未完成、原错/复现/证据/尝试、不能自行继续原因、输入与接手角色。developer不使用真机，tester/effector签名部署采证且真机串行。GPU驻留优先、CPU停投、XComponent+shader唯一上屏，失败FAULT/显式STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12，旧湿M1暂停。


## 2026-11-19 T592 v3 raster/native 续归因

当前精确分流见[architecture-T592-raster-v1.md](architecture-T592-raster-v1.md)：协议字符串T532_WINDOW_HOST_RASTER_V3与planRevision9非同域版本，截图planRevision与raster.generation均9，不构成协议失配。RASTER_NATIVE_MISMATCH是native十项身份/geometry/尺寸合取，具体首不等项缺当前冷原件；先由effector原T592取已有自动prepareCold，tester T671离线解析readiness.authority.snapshot.raster.native并核detailTruncated。T659本次.same崩溃未复现不等全部负控通过；不猜重做版本/放宽门，不先派developer/designer，不修改T592状态。
