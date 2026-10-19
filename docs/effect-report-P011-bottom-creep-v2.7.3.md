# P-011 subjective effect report — bottom-creep-v2.7.3

## Report #1 — 2026-10-18, status: FAIL

### 1. Test object and environment

| Item | Value |
|---|---|
| Problem | P-011 tilt-follow / bottom-layer motion response |
| Technical route | `bottom-creep-v2.7.3` |
| Git commit | `fdc819045f66bdc666b0f9b1fa8e9d65c3760934` |
| Signed HAP | `entry/build/default/outputs/default/entry-default-p011-20261018-v273-signed.hap` |
| HAP SHA256 | `190EEF4BA5AF3C50AC72A0E19D776FE557CD1150CA254FD0604EE2EDA46AC6B6` |
| Device | HUAWEI `VYG-AL00`, `62T0225B10010020` |
| OS / API | `7.1.0.42(SP22C00E111R3P1log)` / API 26, Beta1 |
| Entry | `rendermode=4`, `testmode=true`, `wspreset=thick`, `wssnapshot=true` |
| Effective input | scripted `wstilt` Want sequence |
| Evidence root | `demo/effect/P011-v273/` |
| Evidence manifest | `demo/effect/P011-v273/evidence_manifest.json` |

Capture method was a fixed `snapshot_display` frame sequence per scenario rather than a continuous video. The 0.5×/0.25× replay requested by EC-TT-05 could therefore not be performed. Frame review, contact sheets, and pixel-difference maps were used instead. This limitation is recorded, not compensated.

### 2. Evidence inventory

| Scenario | Round | Frames | Contact sheet | Log |
|---|---|---:|---|---|
| EC-TT-01 | run01/run02 | 7/7 | `demo/effect/P011-v273/EC-TT-01/<round>/contact_sheet.jpeg` | `logs/hilog.txt` |
| EC-TT-02 | run01/run02 | 10/10 | `demo/effect/P011-v273/EC-TT-02/<round>/contact_sheet.jpeg` | `logs/hilog.txt` |
| EC-TT-03 | run01/run02 | 11/11 | `demo/effect/P011-v273/EC-TT-03/<round>/contact_sheet.jpeg` | `logs/hilog.txt` |
| EC-TT-04 | run01/run02 | 5/5 | `demo/effect/P011-v273/EC-TT-04/<round>/contact_sheet.jpeg` | `logs/hilog.txt` |

The manifest also records per-frame SHA256 values. Pixel-difference maps are available as `diff_start_right.jpeg` and `diff_right_left.jpeg` in the first round directories.

### 3. Subjective review

The blue water band and yellow light-sand band responded visibly to the scripted tilt and changed direction consistently across both rounds. They returned toward a level configuration after reset and did not show tearing, flicker, frozen UI, or an unambiguous jump during the sampled frames.

The green bottom layer, however, remained visually static throughout all sampled frames and both rounds of every scenario. The diff maps show changes mainly in the blue water band and the yellow light-sand boundary; the green bottom region shows only a negligible edge artifact and no user-visible continuous bottom response. Given the case definition, this is not acceptable bottom-layer following. The objective suite result is not used to alter or excuse this subjective observation.

### 4. TIERED dimension scores

Scale: 0–10; passing threshold is 6 per applicable dimension. There is no prior `bottom-creep-v2.7.3` subjective report, so no same-route version comparison is made.

| Scenario | Following immediacy | Continuity | Return/reversal | Bottom-surface coordination | Motion realism | Stable smoothness | Scenario verdict |
|---|---:|---:|---:|---:|---:|---::|---|
| EC-TT-01 | 1 | 2 | 2 | 2 | 2 | 7 | FAIL |
| EC-TT-02 | 1 | 2 | 2 | 2 | 2 | 7 | FAIL |
| EC-TT-03 | 1 | 2 | 2 | 2 | 2 | 7 | FAIL |
| EC-TT-04 | 1 | 2 | 2 | 2 | 2 | 7 | FAIL |
| EC-TT-05 review | 1 | 2 | 2 | 2 | 2 | 7 | FAIL |

Scores for EC-TT-05 reflect the same evidence review; no new subjective input was added. The water/light-sand response is clear, but because the evaluated subject is bottom-layer motion, the bottom-motion dimensions remain below the passing threshold.

### 5. MUST-PASS hard gates

| Gate | Result | Notes |
|---|---|---|
| HG-TT-01 crash / white-black screen / freeze >1s | PASS | None observed in sampled frames or UI state. |
| HG-TT-02 tearing / broken surface / sustained flicker | PASS | Not observed. |
| HG-TT-03 bottom-layer teleportation | PASS | No discontinuous bottom-layer jump observed. |
| HG-TT-04 complete bottom-layer immobility under valid input | **FAIL** | The green bottom layer was visually static under all scripted valid tilt sequences lasting longer than 2 seconds. |
| HG-TT-05 sustained abnormal jitter | PASS | Not observed. |
| HG-TT-06 sustained subjective fluidity failure | PASS | No sustained freezing or subjectively continuous stutter; on-screen FPS varied but did not show the prohibited persistent failure. |

### 6. Overall subjective verdict

**FAIL.**

The route does not meet the subjective acceptance criteria because HG-TT-04 is triggered and the bottom-motion dimensions are below the passing threshold. The visible water/light-sand motion is stable and coherent, but the route cannot be accepted as a bottom-layer response implementation on this evidence.

### 7. Change record

| Date | Route | Summary |
|---|---|---|
| 2026-10-18 | `bottom-creep-v2.7.3` | Added the first standalone subjective report. Executed EC-TT-01~EC-TT-05 via scripted `wstilt` input and frame evidence. Overall verdict: FAIL due to HG-TT-04. |
