# T44 双场产品与合规run真机复测

日期：2026-10-26（Asia/Shanghai），08:48起执行；角色effector。原卡6addc1e3e4b06ee357290762。

**结论：部分完成，产品可见效果FAIL，合规采集BLOCKED。** R0/R1均仅见背景和控件，无可见沙；两次Back均未收到store关闭回执。依执行案§7“连续两臂同因失败：暂停并报告”，停止R2/R3，不输出虚构的endpoint/peak数值。交付失败录像、无损关键帧、原始账本前缀与准入流、逐项分析；未改业务代码或判据。

## 身份与执行范围

- 隔离工作树 `D:/hmprojects/t44-build`，HEAD `d941e7fd1a94aa1407c757277fcebda4ad673e07`（含T43）；`entry/src` tree `7836d441b643d4678225ac12e10146e51b8f8eff`，tracked产品diff为空。仅local.properties环境修改，未混入T52/后续产品代码。
- HAP `D:/hmprojects/t44-build/entry-default-s142-signed.hap`，SHA256 `E47FA0DE97573019AA5DCF1A59163E219EEE4B59E4D357059DC44E860C4B18D8`，本轮覆盖安装成功。现存build.log显示ArkTS、native及PackageHap完成，SignHap因既有密码长度配置00303116失败；独立签名产物实际安装成功，不把assembleHap全链称成功。
- 真机VYG-AL00，hdc `62T0225B10010020`，API26，屏幕1280×2832竖屏；单设备串行。开始无本应用PID/其它hdc采集命令；结束停止录屏并force-stop释放设备。
- 冷启参数：`rendermode=4,testmode=true,p011particle=1,p011capture=1,wssnapshot=false,shake=0`。真实设备、脚本TEST输入，无真实手持动态声明。
- R0 run `p011-run-1792975718417-3`，R1 run `p011-run-1792975806102-3`；分开核算，不拼接。R1五次全宽触摸滑动(100,1200)→(1200,1200)，600px/s，后静置20秒并Back。**uitest末参是速度，不是旧执行案表中误记的毫秒**，实际命令/时间见actions.txt；本轮不改变产品判据。

## 原卡四项结论

| 项目 | 结论 | 依据与限制 |
|---|---|---|
| route实际激活、池非结构性零 | PASS（已保存前缀内） | route事件ok/active=true；R0 954/954、R1 283/283记录particleRouteActive=true；R1池最大512、质量最大38.84532288330398，消除STRUCTURAL_ZERO_OFF现象 |
| 手势→真实粒子→LAND→可见床累积 | FAIL（可见效果）；证据链部分成立 | 五次触摸确有27笔原始准入、真实粒子质量，但两臂画面无沙；无法验证同帧token、B tracer互斥和可见LAND，不能以物理汇总替代 |
| 停摇endpoint与peak双报合规run | BLOCKED / NOT_RUN | 两臂重复出现可见失败和缺CLOSED后按执行案暂停；R2/R3未执行，无SHAKE_OFF/ON有效窗；新旧口径均N/A，非0、非PASS |
| worker/身份/raw完整性 | 部分PASS，整体BLOCKED | worker ready、cache文件可读、endpoint身份已装配；缺CLOSED计数，R1重复sequence需补证，只能认前缀，不能认两流完整 |

本次首次该隔离路线真机基线，无同路线改善分可比较；拒绘/无沙触发硬阻断，落沙连续性、质感、跟手等0~10维度N/A。录像确认页面仍显示FPS和控件，没有以背景/色块/Canvas替身冒充沙。

## 原始数据诊断（不作为完整合规PASS）

`tools/analyze_t44_capture_20261026.py`严格解析NDJSON，未修补JSON、未去重或删失败数据。输出prefix-analysis.json。原始文件全部保留。

| 指标 | R0 | R1（停进程后第二次拉取） |
|---|---:|---:|
| NDJSON可解析记录 | 954 | 283 |
| JSON失败 | 0 | 0 |
| sequence重复数 | 0 | 6 |
| committed记录 | 954 | 283 |
| 非空endpoint clock/phase身份 | 954 | 283 |
| 可解析WS_METRIC中非空metricCommitKey | 90/90 | 97/97 |
| 独立admissions行数 | 0 | 27 |
| closed回执 | 无 | 无 |
| 前缀内audit五项失败/错误码 | 0/0 | 0/0 |
| H/L最大质量恒等残差 | 6.82e-12 / 1.36e-12 | 4.70e-12 / 1.13e-12 |

WS_METRIC计数来自full/close日志合并，可能含重叠行，仅作身份非空旁证，不是独立样本总数。前缀audit为有限值/范围/容量/库存/守恒；不能证明尾部或完整run通过。最大totalBudgetExcess约5.15e-14保留浮点原值，不将其截成零，也不重新解释冻结预算门槛。已保存记录未见code5，不代表未采集尾部已排除。

R1最后保存sequence277，elapsed52074ms：源H/L各19.731137432098386，粒子H=16.68323467560854、L=19.731137432098397，池512，CONTACT_WAIT80个/H质量3.095640570841695。准入27条求和H/L各19.73113743209839，与源累计相差约3.55e-15，**逐位一致要求未能据此签PASS**，需tester按相同累加次序/提交身份补证。初始有悬浮库存，床质量自然变化不能全部归为外源LAND；不能以总床增量证明触摸沙均已落床。逐粒年龄未分析，不宣称长期不落已排除。

R1第一次边运行/后台取流有1条不完整JSON，保存为R1-store.ndjson；强停后再拉为R1-store-final.ndjson，无JSON失败但仍6个sequence重复。重复是否属于多个同序号提交/导出重发，留给designer决定补证，不自行认定损坏或删去。两流runId一致；无CLOSED则不能核对最终records/admissions计数。

## 最小复现与责任建议

1. **F44-VISIBLE**：安装该HAP→上述冷启→等待worker ready→容器只见背景；五次滑动后仍无沙。R0.png、R1-start/inj/settled.png及连续录像直接支持。交designer区分入口/合成/渲染/观测问题；如缺证据，由manager派tester采实际renderer选择、draw/swap、帧token/物理commit及fallback拒因，明确后developer修复。不能把T52的NONUNIFORM_SCALE/INVALID_PARTICLE直接移作本基线根因，本轮页面未显示这两码。
2. **F44-CLOSE**：R0/R1运行后Back→等待≥4秒→无P011_OBS_STORE_CLOSED；R0追加force-stop也无，R1最终force-stop后同样无回执。已保存原始前缀，不声称Back销毁了页面。交designer裁定生命周期/收尾接口；tester补Back/Home/页面销毁的生命周期与worker close/计数回执，再由developer修复明确缺陷。
3. **F44-IDENTITY**：R1 283条中6个sequence重复；store ready元数据observationRouteId为natural-relax-hysteresis-v1，实际ledger为dual-field-particle-v1；准入逐位累计有上述差异。作为待分流证据缺口，不由effector作整体归因。tester补attempt/frame/substep/sequence唯一键、流顺序、累计逐笔比对。
4. 修复/补证后需另行安排R0/R1复测、R2同run停摇与R3冻结时序peak双报。开发卡仅开发/编译/离线自测，真机复测独立管理，不作为开发卡关闭前置。

T11旧缺口中：sampler/store worker入包、非结构零、metric key已有本轮设备旁证；**同run起停摇未执行验证，raw完整收尾仍未消除**。T42遗留requestComposition=PENDING、长数组日志截断、空probePoints，本轮不作颜色/数组读回证据，不宣称修复。`peakProtocolEffective=false`，endpoint新旧并报要求不变，本轮均无合格窗口数据，不宣布协议生效。

## 证据和交付登记

目录 `test/artifacts/t44-dualfield-product-run-20261026/`：R0-store.ndjson、R0-admissions.ndjson、R1初拉/终拉双份NDJSON、各臂事件/收尾日志、actions.txt、无损PNG、build.log、prefix-analysis.json、video-check.json。

原生系统录屏 `continuous-R0-R1.mp4`，868×1920，完整顺序解码1399帧、约149.807秒，平均9.339fps（录屏平均值，不是应用性能fps）；SHA256 `E740D142A0B5F3726FD0FAB6F1F657AAAC107F9C94CD0A4ED0CBA13F2278A78B`。关键帧人工审看R0、R1注入和静置均无可见沙；原生录像未剪辑、未伪装成功演示。各原始NDJSON/PNG/视频SHA256记录在prefix-analysis.json。

交付为**部分完成/失败与阻塞回执**，不是产品通过；T44本行登记“结束待验收”仅表示本轮交付结束。manager据报告裁决后续责任/依赖，不增manager验收前置。
