# P-011 objective report - bottom-creep-v2.8-heavy-interface

- Date: 2026-10-18 (Asia/Shanghai)
- Route ID: `bottom-creep-v2.8-heavy-interface`
- Checker schema: `v2`
- Overall verdict: **PASS**
- Next assignment: **effector only** (subjective review)

## Test identity

| Item | Value |
|---|---|
| Source commit | `fdc819045f66bdc666b0f9b1fa8e9d65c3760934` |
| Signed HAP | `.ws-p011-v28-heavy-interface-signed.hap` |
| HAP SHA256 | `0C1C138AB0A0C0AD96AFC1C80182ACF55A2EA4E7E65BB5813E5E8B52176C5DF9` |
| Device | `62T0225B10010020` |
| OS | `VYG-AL00 7.1.0.42(SP22C00E111R3P1log)` |
| API version | `26` |
| Route isolation | v2.8 logs only; no v2.7.x log was mixed |

The historical TC02 schedule precheck failure remains unchanged. This report is the new strict-protocol rerun and does not reinterpret or relax that historical result.

## Green-ROI preflight

Result: **OBSERVATION_READY**

| Direction | Green centroid delta (px) | Heavy flux non-zero samples | Heavy shape RMS | Heavy shape max delta |
|---|---:|---:|---:|---:|
| Positive | `+1.4317007095943382` | `203` | `0.506902408780808` | `1.1370544408866294` |
| Negative | `+1.1010656206868816` | `201` | `0.6058908148419208` | `1.35868851336582` |

Required minimums: heavy flux `>= 30`, shape RMS `>= 0.5`, shape max delta `>= 1.0`. All preflight observation gates PASS.

## TC02 schedule precheck

Result: **PASS**

| Metric | Measured | Threshold |
|---|---:|---:|
| Script event count | `531` | `== 531` |
| Expected event count | `531` | `== 531` |
| Script run ID count | `1` | `== 1` |
| Script sequence | `1..531`, valid | continuous, no duplicate |
| Target sequence | exact schedule match | exact match |
| Accepted-time gap violations | `0` | `== 0` |
| Minimum accepted-time gap | `116 ms` | `>= 100 ms` |
| `input_state` events | `0` | excluded from command metrics |
| Unknown telemetry events | `0` | `== 0` |

Behavior scoring, pressure segments, and suite aggregation were only run after this strict precheck passed.

## Formal suite summary

Overall: **PASS**. The final suite has one behavior log, one budget-isolation/full-load log, and three independent total-clip pressure logs. There are `5` positive-to-negative and `4` negative-to-positive sign switches. The checker reports all MUST-PASS gates as PASS:

- `suiteInputComplete`: PASS
- `suiteRolesComplete`: PASS
- `allLogsComplete`: PASS
- `allLogsPass`: PASS
- `behaviorTierMinimum`: PASS
- `highPressureEvidenceCount`: PASS
- `independentBudgetCoverage`: PASS
- `suiteSignSwitchCoverage`: PASS

### Per-log objective metrics

| Log | Role / target | Run ID | Reset count | Per-log gate | TC01 / TC02 / TC03 score | Budget violation rates (bottom / total) | v2.8 heavy observations |
|---|---|---|---:|---|---:|---:|---|
| `p011-tc02-formal.log` | behavior / 531 target phases | `p011-run-1792333199903-3` | 1 | PASS | `10 / 8 / 10` | `0 / 0` | All 10 phases: non-zero heavy samples `1594-1610`, RMS `3.5368-5.0881`, max delta `7.9587-11.3950`, direction correct `10/10` |
| `p011-full-load.log` | budget isolation + pressure clip / `+88` | `p011-run-1792333375230-3` | 1 | PASS | `N/A / N/A / 10` | `0 / 0` | Samples `2026`, RMS `3.8074623103311405`, max delta `10.489187552859661`, direction correct |
| `p011-clip-segment-a-positive.log` | pressure clip / `+88` | `p011-run-1792333396594-3` | 1 | PASS | `N/A / N/A / 10` | `0 / 0` | Samples `2014`, RMS `3.789357641125169`, max delta `10.4376110409859`, direction correct |
| `p011-clip-segment-b-negative.log` | pressure clip / `-88` | `p011-run-1792333417895-3` | 1 | PASS | `N/A / N/A / 10` | `0 / 0` | Samples `2011`, RMS `5.155207616393775`, max delta `11.902705491400567`, direction correct |
| `p011-clip-segment-c-positive-repeat.log` | pressure clip / `+88` | `p011-run-1792333439114-3` | 1 | PASS | `N/A / N/A / 10` | `0 / 0` | Samples `2007`, RMS `3.785347739351235`, max delta `10.429379011370898`, direction correct |

### Behavior-log detail

| Metric | Measured | Gate |
|---|---:|---|
| TC01 `t90P95Ms` | `0` | PASS |
| TC01 direction error | `0` | PASS |
| TC01 bottom direction error | `0` | PASS |
| TC02 `pearsonH` | `0.8514094900592801` | PASS |
| TC02 `pearsonL` | `0.8485187224569213` | PASS |
| TC02 `lagHMs` / `lagLMs` | `0 / 0` | PASS |
| TC02 `feH` | `0.041381892330365266` | PASS |
| TC02 `feL` | `0.027691286957740813` | PASS |
| TC02 score | `8` | `>= 6` PASS |
| TC03 `maxUncommandedTiltStep` | `0` | PASS |
| TC03 reverse direction error | `0` | PASS |
| TC03 zero residual | `0` | PASS |
| TC04 water/heavy/light drift | `5.3253e-14 / 1.3579e-14 / 1.4842e-14` | PASS |
| Non-finite / out-of-range / model error / crash | `0 / 0 / 0 / 0` | PASS |
| `variance(C_H)` | `2394.6446424133055` | `> 1e-12` PASS |
| `variance(C_L)` | `1293.4300571929791` | `> 1e-12` PASS |

Pressure logs report zero non-finite values, zero out-of-range values, zero model errors, zero crashes, and zero budget violations.

## Harness adjustment

The final TC02 capture used a `200ms` device shell pause between Want commands. The previous harness used `150ms`. This only makes command spacing stricter; the accepted-gap threshold remained exactly `100ms`. No threshold was widened, no sample was smoothed, resampled, or deleted, and no known historical failure was reinterpreted.

## Evidence and SHA256

| Evidence | SHA256 |
|---|---|
| `test/artifacts/p011-v28-heavy-interface/p011-tc02-formal.log` | `E4A1C6BCAF51266EAB21A1A2DC04664689EDE7B8A5ABC8449665F3B58EAEEC74` |
| `test/artifacts/p011-v28-heavy-interface/p011-full-load.log` | `D15B946C0D7B716638AE0D3450A84D52A1943EBBD37FA15A5401238E0CFEDDD8` |
| `test/artifacts/p011-v28-heavy-interface/p011-clip-segment-a-positive.log` | `C244C35C334FDA3B1CD8964B120F62BD21B34E661B1ED35A6C5523D69E5A99EA` |
| `test/artifacts/p011-v28-heavy-interface/p011-clip-segment-b-negative.log` | `BAC148C15C6C4C50554EB9309A13994A809153C7A3F48C51309EEDE2BC0576DD` |
| `test/artifacts/p011-v28-heavy-interface/p011-clip-segment-c-positive-repeat.log` | `1BB9C7A9FABFC2DBC6CE3E9BEB6C8E8033F694393230B47329C4B02A533BE996` |
| `demo/effect/P011-v28-heavy-interface/preflight/hilog.txt` | `07E165F3EC8DBF474D11EAE29A31428412C808776295A1D611BEA5BDE6F1FFD9` |
| `demo/effect/P011-v28-heavy-interface/preflight/preflight.json` | `A7C686DF541116BCBE3A1C8404A6662FDD5E1CC5F1A4918AFBD118CC5770C10D` |
| `test/artifacts/p011-v28-heavy-interface/p011-suite.json` | `8CA652476D74304B6E93FCD08365EF644546D8E19263CB8D1401B04D06DDBE5E` |
| `test/artifacts/p011-v28-heavy-interface/p011-suite.md` | `26535DF23D2DA021F11583E180237C321C146CE828A5C19161C64CA733104352` |
| `test/artifacts/p011-v28-heavy-interface/p011-tc02-formal.tc02-validation.json` | `853D55C6C1A8916B0115EEC9FA3B5BA9CF30FE7110BB8AC0C99C17B49C4EBF0F` |

Supplementary evidence: `demo/effect/P011-v28-heavy-interface/preflight/manifest.json`, three preflight JPEG captures, the generated schedule CSV, and the device command script. Test cases are recorded in `docs/test-cases.md` section 9.

## Objective conclusion

All v2.8 observation gates, all route-specific tiered minimums, all budget/conservation/stability hard gates, and all suite-level hard gates pass. The objective verdict is **PASS**. No algorithm or architecture document, historical report, or business code was modified by this validation task.

## 2026-10-18 双模式输入测试先行 RED 基线（新增，旧 PASS 保留）

原因：用户要求程序模拟姿态的测试模式，以及真实传感器驱动的体验模式。此段仅记录新增输入契约的实现前基线，不撤销或追认旧物理测试证据。

- 被测 HEAD：`fdc819045f66bdc666b0f9b1fa8e9d65c3760934`。工作树含未提交修改，完整被读三文件SHA256记录于原始JSON；不是clean commit或匹配新HAP的声明。本轮无构建、安装或设备操作。
- 用例：`docs/test-cases.md` §10，TC-P011-MODE-01~07，均MUST-PASS。物理指标/质量分档及及格线不变。
- 已执行：`python test/test_p011_input_modes.py --output test/artifacts/p011-v28-canonical-20261018/input-modes-red-baseline.json`，进程退出0表示库存脚本成功，验收结论为 **RED_BASELINE_NOT_ACCEPTED**。
- 原始结果：源码库存6项均present=true：test-only parser guard、forceInvert guard、fresh sequence guard、加速度计订阅、测试模式排除sensor路由、script目标保持。这是静态存在性证据，不是单元或runtime PASS。
- 正式用例实测：MODE-01源互斥、02切换清残留、03真实回调、04可复现、05首Want/531、06生命周期、07共用链与回归均 **BLOCKED**：最终双模式契约、实现及对应运行证据待交付；不产生伪造运行数值或质量评分。
- 静态风险：现有toggleTestMode退出分支清forceInvert与gesture hold，但未见该分支清p011ScriptTargetTilt、pending Want及sensorGx/Gy滤波历史；source=sensor路径在lastSensorCommandTime缺失时可回退当前时间，不能据此证明真实回调到达。必须通过新契约/实现及运行测试验证，不凭文本推断运行失败。
- 现有P011TiltWant解析模块已读取，未发现仓内已登记的对应独立主机测试入口；本轮未将ArkTS源文件当Node可直接执行的JS。后续由developer提供可运行adapter/state-transition单元测试，tester重测。
- 原始证据：`test/artifacts/p011-v28-canonical-20261018/input-modes-red-baseline.json`；脚本：`test/test_p011_input_modes.py`。源哈希及时间均由该次运行生成。

交接：designer/architect冻结入口、规范姿态、时间/滤波/清理语义及复现数值契约；developer实现后经reviewer提交并提供匹配HAP，tester才运行canonical及设备客观测试。脚本模拟不能替代真实传感器回调接线证据，静止真实回调不能证明人体动态体验、噪声或标定表现。

## 2026-10-18 23:18 新双模式签名包正式客观复测：FAIL（默认可见场景）

**最新总体结论：FAIL，green heavy-interface硬闸门未通过。canonical TC02与压力/full suite为BLOCKED（未执行），不是这些未执行测试自身FAIL。** 旧PASS及RED基线保留，不将旧10列压力场景预检外推为默认场景通过。

### 身份与源码冻结

- reviewer commit：`130a52c74e8964fb024677b672cad1965f5cf1db`；sourceCommitType=`reviewed P011 baseline + recorded dirty source snapshot`。
- 实际构建时基线：`fdc819045f66bdc666b0f9b1fa8e9d65c3760934`；HAP：`test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap`，SHA256=`C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`。
- 开发构建时48文件manifest已核验；本次独立`source-manifest-before.json`/`source-manifest-after.json`比较相关文件哈希变化为0。EntryAbility/SandCanvas/LiquidCAWant及P001交叉依赖保持dirty，不能声称该commit单独clean复现HAP。
- 设备`62T0225B10010020`，沿用开发安装的同SHA包；本次独占设备、冷启动，采集结束已停止hilog。OS/API依开发交付及本机既有身份记录，本次没有另行查询，不虚构新查询证据。

### 双模式目标与主机回归：独立通过

重新执行实际产品ETS helper/WaterSand测试：`& 'C:/Program Files/Huawei/DevEco Studio/tools/node/node.exe' test/run_p011_input_units.cjs test/artifacts/p011-v28-canonical-20261018/units-retest.json`，**39 PASS / 0 FAIL**。包含MODE01~07纯helper/固定dt物理轨迹、双向10轮转换、100次sensor干扰、fresh callback恢复、首fresh Want/同模式保持、input reset不改变材料/run及既有物理回归。此结果属于主机行为测试，不伪称设备全量MODE E2E。

`python -m unittest discover -s test -p test_p011_capture.py`为19 tests OK；runner mock回归为PASS，原始输出保存在`host-runner-retest.log`。这些通过与本次green物理失败分别记录。

设备模式集成证据引用开发同包`runtime-hilog.txt`/`panel-hilog.txt`及layout：冷/热TEST首命令保留、Want与面板双向转换、EXPERIENCE真实ACCELEROMETER callback带callbackTimestamp与有限值。是静止真机接线/隔离smoke，**不证明真实手持动态、噪声、标定或人体体验**。本次正式green另外验证单run首条命令不丢失、连续5条script准确消费。

### 新包green正式采集及硬闸门

精确可复现采集命令：`powershell -ExecutionPolicy Bypass -File test/artifacts/p011-v28-canonical-20261018/collect_green.ps1`。脚本记录完整Want：rendermode=4、testmode=true、forceinvert=false、seed=20261016、wspreset=thick、wssnapshot=true，无wscols/wscapacity/wssandtopmax覆盖。姿态0/+15/0/-15/0，等待3/4/2/4/2秒后各截图，另有命令与传输开销；manifest记录host命令和截图时刻。

实际WS_EVENT init为cols=128、capacity=192、vSandTopMax=28；runId=`p011-run-1792336720208-3`，单次scene reset。P011_EVENT input/source=script序列1..5与0/+15/0/-15/0精确一致。acceptedTime分别1792336720209、1792336723981、1792336729103、1792336732175、1792336737333；两个非零目标保持5122/5158ms，输入确实被接受，不是传感器抢占或未消费Want。

| 方向 | heavy非零样本（门槛>=30） | heavyShapeRms（>=0.5） | heavyShapeMaxDelta（>=1.0） | green centroid delta px | 判定 |
|---|---:|---:|---:|---:|---|
| +15 |124 PASS|1.0555411480769294e-7 FAIL|1.7731429124978604e-7 FAIL|0.006113994371276021|FAIL|
| -15 |154 PASS|5.367150030523121e-7 FAIL|1.6498952533083866e-6 FAIL|0.0012020007311548397|FAIL|

以上为原checker按manifest窗口输出；全log正/负相位heavyShapeRms最大也仅2.146400665866335e-7 / 5.690154386543986e-7，不能用窗口差异解释失败。相反lightShapeRms正相位最大1.7190898876551233，负相位0.35841109277184596。heavyFlux非零、累计位移非零，说明输运计数不等于中央80% heavy界面可观察变化；局部边界输运而中央heavy静止是待designer进一步归因的推断，不在本报告擅改物理。

历史旧preflight原始WS_EVENT init明确cols=10、capacity=1000、vSandTopMax=30（water/heavy/light总量380/720/720）；新默认场景4864/9216/9216。两者preset/route相同，几何不同；旧采集完整命令及seed未在现存旧manifest记录，不能声称初始化完全一致。**不将本次换为10列stress来隐藏默认场景失败。**

checker工具风险：`p011_v28_preflight_check.py`目前仅ROI有绿色即输出OBSERVATION_READY/exit0，未在exit码实现既有heavy样本/RMS/max硬阈值。此次按用例逐项核验判FAIL；工具状态不能覆盖硬门槛，不改旧JSON。

### 证据与后续

独立诊断：`test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json`，含完整实测、几何对比、全部green文件SHA256和sourceHashChanges=[]。新原始hilog SHA256=`20326cd2c5e1f1141dc42831f3f71cb687f93cbc2fba21b06c7dfbaf7f5c67f2`。截图、manifest、原preflight输出保留于同目录`green-new-hap/`，未覆盖旧PASS/FAIL。

本轮不进行质量分档/TC02物理评分，不放宽阈值。canonical531、冻结审计和压力suite因前置green失败标BLOCKED；主观正式评估未放行。建议designer归因默认几何heavy观测失败，测试工具硬闸门遗漏另列测试资产整改，双模式39项通过保留为已完成目标。

## 2026-10-18 green预检工具硬闸门最小整改（host-only）

本段仅修正测试工具exit/status，不改变上述新包物理FAIL及历史冻结输出。基于reviewer基线130a52c74e8964fb024677b672cad1965f5cf1db工作树，先新增`test/test_p011_preflight_gates.py`，运行RED：真实新包shape失败反例仍exit0，断言失败；原始`preflight-gates-red.log`保留。

随后仅在`test/p011_v28_preflight_check.py`增加现有三个heavy门槛的有限值/阈值判断，正负phase分别输出physicalGates；新增roiReady与physicalGatePassed，保留旧诊断字段。ROI不足仍BLOCKED且非零；ROI存在而heavy失败为FAIL(heavyObservation)且exit2；两者通过才OBSERVATION_READY/exit0。

回归实测：`python -m unittest discover -s test -p test_p011_preflight_gates.py`，**5 tests OK**，原始日志`test/artifacts/p011-v28-canonical-20261018/preflight-gates-green-final.log`。覆盖冻结新包失败反例非零、旧真实10列green fixture仍通过、ROI缺失但物理通过仍阻断、精确阈值边界通过、每项低于阈值及NaN/Inf拒绝。真实fixture只读，CLI结果存临时文件，旧JSON/图像/log未修改。无设备操作、无产品/算法改动，交reviewer审查提交。
