# P-011 subjective report — bottom-creep-v2.8-heavy-interface

- Date: 2026-10-18 (Asia/Shanghai)
- Route ID: `bottom-creep-v2.8-heavy-interface`
- Source commit: `fdc819045f66bdc666b0f9b1fa8e9d65c3760934`
- Signed HAP: `.ws-p011-v28-heavy-interface-signed.hap`
- HAP SHA256: `0C1C138AB0A0C0AD96AFC1C80182ACF55A2EA4E7E65BB5813E5E8B52176C5DF9` (rechecked)
- Device: `62T0225B10010020`
- Device OS: `VYG-AL00 7.1.0.42(SP22C00E111R3P1log)`
- Device API: `26`
- Overall subjective verdict: **BLOCKED**

This report is route-isolated. It does not reuse v2.7.x evidence or compare this route with another route. The objective v2.8 PASS remains separate and is not treated as subjective PASS.

## BLOCKED reason

The required continuous video capture prerequisite is unavailable on the connected device. `uitest` provides `screenCap` and UI-operation recording but no screen-video recorder. The device capability output is retained at `demo/effect/P011-v28-heavy-interface/subjective/native-capture-capability.txt`.

Because EC-TT-05 and hard gates HG-TT-01, HG-TT-02, HG-TT-03, HG-TT-05, and HG-TT-06 require review of continuity, teleport, tearing, freezing, persistent jitter, and smoothness, still screenshots cannot prove absence or presence of those defects. TIERED dimensions are therefore **not scored**. No score was invented.

## Environment and input protocol

- Mode: `rendermode=4`, `testmode=true`, `forceinvert=false`, `seed=20261018`, `wspreset=thick`, `wssnapshot=true`.
- Input: scripted `wstilt` commands through EntryAbility; not human-held sensor input.
- Formal scenario runner: `demo/effect/P011-v28-heavy-interface/subjective/run_v28_subjective.ps1`.
- Capture status: exploratory only because continuous video was unavailable. The pre-execution guard in `demo/effect/P011-v28-heavy-interface/subjective/PREPARATION.md` explicitly distinguishes screenshot exploration from formal evidence.

## ROI marking

Reference image: `demo/effect/P011-v28-heavy-interface/subjective/roi-reference.png`.
ROI crops: `demo/effect/P011-v28-heavy-interface/subjective/roi-crops.png`.

| ROI | Full-resolution box on the reference frame |
|---|---|
| Water | `(174, 938, 1131, 1160)` |
| Yellow light | `(174, 1160, 1131, 1566)` |
| Green heavy | `(174, 1566, 1131, 1971)` |

## MUST-PASS gates

| Gate | Result | Reason |
|---|---|---|
| HG-TT-01 | **NOT EVALUABLE / BLOCKED** | Video is required to determine freezing longer than one second. |
| HG-TT-02 | **NOT EVALUABLE / BLOCKED** | Video/frame sequence is required to assess tearing, broken surfaces, and persistent flicker. |
| HG-TT-03 | **NOT EVALUABLE / BLOCKED** | Continuous frames are required to rule out single-frame teleportation. |
| HG-TT-04 | **N/A / BLOCKED** | The route declares heavy response and objective preflight observed heavy movement, but subjective formal confirmation on continuous capture is unavailable. |
| HG-TT-05 | **NOT EVALUABLE / BLOCKED** | At least one second of continuous observation is required to assess non-input jitter. |
| HG-TT-06 | **NOT EVALUABLE / BLOCKED** | Subjective persistent stutter requires continuous observation; screenshots cannot establish it. |

## Scenario results

All exploratory rounds used two rounds for EC-TT-01 through EC-TT-04 and a single EC-TT-05 recheck pass. Each round contains timestamped JPEG frames, `env.json`, and a hilog snapshot. These captures demonstrate that the harness ran and the app remained reachable, but they are not accepted as formal subjective evidence for scoring.

| Scenario | Formal result | Exploratory evidence |
|---|---|---|
| EC-TT-01 | **BLOCKED / NOT SCORED** | `demo/effect/P011-v28-heavy-interface/subjective/EC-TT-01/run01/` and `.../run02/` |
| EC-TT-02 | **BLOCKED / NOT SCORED** | `demo/effect/P011-v28-heavy-interface/subjective/EC-TT-02/run01/` and `.../run02/` |
| EC-TT-03 | **BLOCKED / NOT SCORED** | `demo/effect/P011-v28-heavy-interface/subjective/EC-TT-03/run01/` and `.../run02/` |
| EC-TT-04 | **BLOCKED / NOT SCORED** | `demo/effect/P011-v28-heavy-interface/subjective/EC-TT-04/run01/` and `.../run02/` |
| EC-TT-05 | **BLOCKED / NOT SCORED** | `demo/effect/P011-v28-heavy-interface/subjective/EC-TT-05/review/`; formal slow-speed video review was not possible |

Each scenario/round folder includes `contact-sheet.png` for frame review and `roi-diff-contact-sheet.png` for screenshot-pair difference review. The latter is exploratory and cannot replace video because screenshots are sparse and some captures may repeat identical display state.

## Evidence index

- Capability check: `demo/effect/P011-v28-heavy-interface/subjective/native-capture-capability.txt`
- Capture guard: `demo/effect/P011-v28-heavy-interface/subjective/PREPARATION.md`
- Formal runner: `demo/effect/P011-v28-heavy-interface/subjective/run_v28_subjective.ps1`
- Round metadata: `env.json` in every scenario/round folder
- Round hilog snapshots: `logs/hilog.txt` in every scenario/round folder
- Frame timestamp source: `timestamps` array in every `env.json`
- ROI reference: `demo/effect/P011-v28-heavy-interface/subjective/roi-reference.png`
- ROI crops: `demo/effect/P011-v28-heavy-interface/subjective/roi-crops.png`

No app crash, app fatal, or route-mixing was observed in the exploratory hilogs. This observation does not substitute for the blocked hard-gate evaluation.

## Exact unlock conditions

To convert this report from BLOCKED to a scored run, all of the following are required:

1. Enable or provide a reliable device-side screen-video recorder or external video capture that records the full screen at the display frame rate without color/geometry alteration.
2. Verify a short recording can start before app reset, stop after the final settle, and export/read back as a playable file; retain file SHA256, device time alignment, and source/device/route metadata.
3. Execute the canonical EC-TT-01 through EC-TT-04 schedules with recording started before setup, retaining one recording per round and synchronized command CSV/hilog.
4. Review EC-TT-01 through EC-TT-04 recordings at 1x, 0.5x, and 0.25x for EC-TT-05, including at least one reverse transition per scenario and per-frame checks around suspected anomalies.
5. Export required key frames, ROI crops, timestamp mapping to nearest input sequence, and contact sheets or video clips.
6. Complete both formal rounds for EC-TT-01 through EC-TT-04; add a third round if any TIERED dimension differs by at least one point.

## Verdict

**BLOCKED**. No formal TIERED score is assigned, no hard gate is declared PASS, and the route does not receive subjective acceptance. Objective evidence remains PASS independently. Once the capture prerequisite is unlocked, rerun this version under the same route-isolated evidence root and append a new dated section or create a new version-specific report.

## 2026-10-18 双模式协议 RED 基线（追加，不覆盖历史）

- Source commit: fdc819045f66bdc666b0f9b1fa8e9d65c3760934（当前基线）；新双模式实现开发中，未交付审计签名HAP。
- Route: bottom-creep-v2.8-heavy-interface。适用用例：effect-cases.md 新增自动姿态/双模式协议、EC-TT-M01/M02。
- 立即基线判定：**BLOCKED(NOT_IMPLEMENTED)**。M01/M02、EC-TT-01~05 正式双模式轮均未执行，全部分档 N/A（未实现/缺正式证据）；HG-TT-01~06 未判定，不能宣称零缺陷或通过。
- 用户已授权自动姿态替代必须人工手持的前置；≥6 分及 HG-TT 硬门槛、完整原生录像要求保持。合成姿态不能证明真实硬件噪声/标定/人体手持体验，未覆盖项如实记录。
- 与同路线上次相比：仅验收输入协议按用户要求更新；无新的可比较分数，不推断改善/恶化，不跨路线比较。
- 录屏能力探测：系统 com.huawei.hmos.screenrecorder 可由控制中心自动启动/停止；root探测视频位于 /storage/media/100/local/files/Photo/2/VID_1792335687_002.mp4（mtime 2026-10-18 23:00，752215 bytes），已导出 subjective/recorder-capability/root-20261018-2300-probe.mp4。OpenCV 可解码，868×1920、538帧、平均20.474fps、约26.3秒；仅工具能力，不是正式业务证据或帧率结论。
- 正式执行前置：新HAP身份固定、reviewer通过、canonical客观冻结审计PASS，设备串行释放，实际输入时间/完整视频/遥测对齐。达成后重测；当前不因该RED基线要求算法归因。

能力录像 SHA256：`CA454B1A6D84A4AE305DA678580B6F5819341CF8EDA381A4686F6F9473697FF1`。此录像仅为root录屏能力探测，不能用于 EC-TT 双模式正式轮评分。

## 2026-10-18 新双模式包默认画面正式主观前置结论

- 结论：**BLOCKED(green_default_scene_gate_failed)**。本段只接收tester已核实的前置失败，不冒充effector正式录像体验评分。
- Source commit：`130a52c74e8964fb024677b672cad1965f5cf1db`；route：`bottom-creep-v2.8-heavy-interface`；输入修订：`dual-input-v1`；包：`test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap`，SHA256 `C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`；device `62T0225B10010020`。
- tester前置证据：`test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json`。默认实际参数 `cols=128/capacity=192/vSandTopMax=28`；5条script序列1..5、单run，±15°实际保持约5.1秒（root/tester已核对）。不是历史10列stress场景。
- 正/负heavy非零样本124/154，但heavyShapeRms=`1.0555411480769294e-7 / 5.367150030523121e-7`，heavyShapeMaxDelta=`1.7731429124978604e-7 / 1.6498952533083866e-6`；均未达到原`RMS≥0.5/max≥1.0`前置。绿色质心位移仅`0.006113994371276021 / 0.0012020007311548397 px`。light响应不替代heavy门槛。
- 注意原始 `green-new-hap/preflight.json` 标签为OBSERVATION_READY，但正式 `formal-gate-result.json` 已按原阈值明确FAIL；本报告依正式门槛结果阻断，不接受错误就绪标签放行。
- 原始hilog：`test/artifacts/p011-v28-canonical-20261018/green-new-hap/hilog.txt`，SHA256 `20326CD2C5E1F1141DC42831F3F71CB687F93CBC2FBA21B06C7DFBAF7F5C67F2`。相关baseline/positive/negative截图与manifest同目录，身份和文件哈希见正式gate JSON。
- 因前置失败，canonical/pressure与effector EC-TT-01~04各两轮、05回放及M01/M02正式录像均未执行；分档全部N/A，HG各项没有新的主观正式判定。未进行设备操作，未补造评分。
- 历史10cols/capacity1000/vSandTopMax30测试不能外推默认用户画面；保留旧报告及v2.7.3原判，不混入本route新包评分。阈值、场景与默认分辨率不修改。
- 移交：designer归因并改善默认分辨率下的heavy运动；新实现经reviewer及tester前置/完整客观PASS后再恢复正式自动主观。真实硬件噪声、标定、人体手持感仍列未覆盖，用户已授权的自动姿态验收不依赖人工手持。
- host工具已准备：`subjective/run_dual_input_formal.ps1`、`prepare_v28_subjective.ps1`、`native_recording.ps1`、`analyze_video.py`；仅静态语法检查，未对当前新包执行正式采集。工具不会自动评分，EC05只回放。
