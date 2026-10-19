## P-006 liquid-mode objective check — 2026-10-15T11:38:32+08:00

- Case: `ALL`
- Commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Overall: **PASS**
- Self-check: `True`
- Parse errors: 0

### TC-11 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-STATIC-300-FRAMES` | MUST-PASS | `3620` | **PASS** | - |
| `HG-UPRIGHT-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-OBSERVATION-DURATION` | MUST-PASS | `30.992` | **PASS** | - |

实测汇总：`{"finalPhaseOrder": true, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "longestStaticFrameRun": 3620, "maxDualShareDeviationPct": 0.0, "maxInterfaceAmplitudeCells": 1.0, "observationSeconds": 30.992, "particleDriftPct": 0.0, "pixelAreaDriftPct": 0.0}`

### TC-12 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-SHAKE-EVENTS` | MUST-PASS | `{"start": 1, "stop": 1}` | **PASS** | - |
| `HG-RECOVERY-DURATION` | MUST-PASS | `30.0` | **PASS** | - |
| `HG-MIXED-RISE` | MUST-PASS / TIERED | `0.32999999999999996` | **PASS** | 10 |
| `HG-RECOVERY-RESIDUAL` | MUST-PASS / TIERED | `0.005000000000000001` | **PASS** | 8 |
| `HG-RECOVERY-SLOPE` | MUST-PASS | `-0.012035998242392397` | **PASS** | - |
| `HG-FINAL-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-FINAL-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |

实测汇总：`{"baselineMixedFraction": 0.02, "finalInterfaceAmplitudeCells": 1.0, "finalPhaseOrder": true, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "maxDualShareDeviationPct": 0.0, "mixedRise": 0.32999999999999996, "particleDriftPct": 0.0, "peakMixedFraction": 0.35, "pixelAreaDriftPct": 0.0, "recoveryResidual": 0.005000000000000001, "recoverySlopePerSecond": -0.012035998242392397}`

### TC-13 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-TRANSITION-EVENTS` | MUST-PASS | `2` | **PASS** | - |
| `HG-REARRANGE-WINDOW-1` | MUST-PASS / TIERED | `2.525` | **PASS** | 10 |
| `HG-PHASE-ORDER-1` | MUST-PASS | `true` | **PASS** | - |
| `HG-REARRANGE-WINDOW-2` | MUST-PASS / TIERED | `2.5250000000000004` | **PASS** | 10 |
| `HG-PHASE-ORDER-2` | MUST-PASS | `true` | **PASS** | - |
| `HG-NO-TELEPORT` | MUST-PASS | `0.5` | **PASS** | - |
| `HG-POST-EVENT-OBSERVATION` | MUST-PASS | `4.992000000000001` | **PASS** | - |

实测汇总：`{"initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "maxDualShareDeviationPct": 0.0, "maxInterfaceStepCells": 0.5, "particleDriftPct": 0.0, "pixelAreaDriftPct": 0.0, "postLastEventSeconds": 4.992000000000001, "transitions": [{"event": "invert_on", "eventTime": 1.0, "rearrangeSeconds": 2.525, "stablePhaseOrder": true, "target": "inverted"}, {"event": "invert_off", "eventTime": 5.0, "rearrangeSeconds": 2.5250000000000004, "stablePhaseOrder": true, "target": "upright"}]}`

## P-006 liquid-mode objective check — 2026-10-15T16:22:18+08:00

- Case: `ALL`
- Commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Overall: **FAIL**
- Self-check: `True`
- Frame inputs: `0`
- Parse errors: 0

### TC-11 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-STATIC-300-FRAMES` | MUST-PASS | `3620` | **PASS** | - |
| `HG-UPRIGHT-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-OBSERVATION-DURATION` | MUST-PASS | `30.992` | **PASS** | - |

实测汇总：`{"finalPhaseOrder": true, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "longestStaticFrameRun": 3620, "maxDualShareDeviationPct": 0.0, "maxInterfaceAmplitudeCells": 1.0, "observationSeconds": 30.992, "particleDriftPct": 0.0, "pixelAreaDriftPct": 0.0}`

### TC-12 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-SHAKE-EVENTS` | MUST-PASS | `{"start": 1, "stop": 1}` | **PASS** | - |
| `HG-RECOVERY-DURATION` | MUST-PASS | `30.0` | **PASS** | - |
| `HG-MIXED-RISE` | MUST-PASS / TIERED | `0.32999999999999996` | **PASS** | 10 |
| `HG-RECOVERY-RESIDUAL` | MUST-PASS / TIERED | `0.005000000000000001` | **PASS** | 8 |
| `HG-RECOVERY-SLOPE` | MUST-PASS | `-0.0025647254615758816` | **PASS** | - |
| `HG-RECOVERY-TIME` | MUST-PASS / TIERED | `2.308` | **PASS** | 8 |
| `TIERED-PEAK-MIXED-SHARE` | TIERED | `0.35` | **PASS** | 10 |
| `TIERED-RECOVERY-TIME` | TIERED | `2.308` | **PASS** | 8 |
| `TIERED-FINAL-MIXED-SHARE` | TIERED | `0.025` | **PASS** | 6 |
| `TIERED-TC12-MINIMUM` | TIERED | `6` | **PASS** | 6 |
| `HG-FINAL-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-FINAL-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |

实测汇总：`{"baselineMixedFraction": 0.02, "finalInterfaceAmplitudeCells": 1.0, "finalPhaseOrder": true, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "maxDualShareDeviationPct": 0.0, "mixedRise": 0.32999999999999996, "particleDriftPct": 0.0, "peakMixedFraction": 0.35, "pixelAreaDriftPct": 0.0, "recoveryDurationSeconds": 30.0, "recoveryResidual": 0.005000000000000001, "recoverySlopePerSecond": -0.0025647254615758816, "recoveryStableMetric": {"mixedShare": 0.025, "time": 10.308}, "recoveryTimeSeconds": 2.308, "tierScores": {"finalMixedShare": 6, "minimum": 6, "peakMixedShare": 10, "recoveryTime": 8}}`

### TC-13 — FAIL

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-TRANSITION-EVENTS` | MUST-PASS | `2` | **PASS** | - |
| `HG-REARRANGE-WINDOW-1` | MUST-PASS / TIERED | `1.508` | **FAIL** | 4 |
| `HG-PHASE-ORDER-1` | MUST-PASS | `true` | **PASS** | - |
| `HG-REARRANGE-WINDOW-2` | MUST-PASS / TIERED | `1.508` | **FAIL** | 4 |
| `HG-PHASE-ORDER-2` | MUST-PASS | `true` | **PASS** | - |
| `HG-NO-TELEPORT` | MUST-PASS | `0.5` | **PASS** | - |
| `HG-CENTROID-DISPLACEMENT` | MUST-PASS | `0.1670000000000016` | **PASS** | - |
| `TIERED-PROCESS-SMOOTHNESS` | TIERED | `0.1670000000000016` | **PASS** | 10 |
| `HG-PIXEL-CLASSIFICATION-CONTINUITY` | MUST-PASS | `0.0` | **PASS** | - |
| `TIERED-TC13-MINIMUM` | TIERED | `4` | **FAIL** | 4 |
| `HG-POST-EVENT-OBSERVATION` | MUST-PASS | `4.992000000000001` | **PASS** | - |

实测汇总：`{"centroidDisplacementSources": {"frames": 0.0, "metric": 0.1670000000000016}, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "maxAdjacentCentroidDisplacementCells": 0.1670000000000016, "maxAdjacentPixelClassificationChange": 0.0, "maxDualShareDeviationPct": 0.0, "maxInterfaceStepCells": 0.5, "particleDriftPct": 0.0, "pixelAreaDriftPct": 0.0, "pixelClassificationSources": ["LIQUID_METRIC changedCells/totalCells"], "postLastEventSeconds": 4.992000000000001, "tierScores": {"minimum": 4, "rearrange": [4, 4], "smoothness": 10}, "transitions": [{"event": "invert_on", "eventTime": 1.0, "rearrangeSeconds": 1.508, "stablePhaseOrder": true, "target": "inverted"}, {"event": "invert_off", "eventTime": 5.0, "rearrangeSeconds": 1.508, "stablePhaseOrder": true, "target": "upright"}]}`

## P-006 liquid-mode objective check — 2026-10-15T16:23:00+08:00

- Case: `ALL`
- Commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Overall: **PASS**
- Self-check: `True`
- Frame inputs: `0`
- Parse errors: 0

### TC-11 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-STATIC-300-FRAMES` | MUST-PASS | `3620` | **PASS** | - |
| `HG-UPRIGHT-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-OBSERVATION-DURATION` | MUST-PASS | `30.992` | **PASS** | - |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `3720` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"finalPhaseOrder": true, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "longestStaticFrameRun": 3620, "maxDualShareDeviationPct": 0.0, "maxInterfaceAmplitudeCells": 1.0, "observationSeconds": 30.992, "particleDriftPct": 0.0, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "validFrameCount": 3720, "validFrameRatio": 1.0}`

### TC-12 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-SHAKE-EVENTS` | MUST-PASS | `{"start": 1, "stop": 1}` | **PASS** | - |
| `HG-RECOVERY-DURATION` | MUST-PASS | `30.0` | **PASS** | - |
| `HG-MIXED-RISE` | MUST-PASS / TIERED | `0.32999999999999996` | **PASS** | 10 |
| `HG-RECOVERY-RESIDUAL` | MUST-PASS / TIERED | `0.005000000000000001` | **PASS** | 8 |
| `HG-RECOVERY-SLOPE` | MUST-PASS | `-0.0025647254615758816` | **PASS** | - |
| `HG-RECOVERY-TIME` | MUST-PASS / TIERED | `3.308` | **PASS** | 6 |
| `TIERED-PEAK-MIXED-SHARE` | TIERED | `0.35` | **PASS** | 10 |
| `TIERED-RECOVERY-TIME` | TIERED | `3.308` | **PASS** | 6 |
| `TIERED-FINAL-MIXED-SHARE` | TIERED | `0.025` | **PASS** | 6 |
| `TIERED-TC12-MINIMUM` | TIERED | `6` | **PASS** | 6 |
| `HG-FINAL-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-FINAL-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `4441` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"baselineMixedFraction": 0.02, "finalInterfaceAmplitudeCells": 1.0, "finalPhaseOrder": true, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "maxDualShareDeviationPct": 0.0, "mixedRise": 0.32999999999999996, "particleDriftPct": 0.0, "peakMixedFraction": 0.35, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "recoveryDurationSeconds": 30.0, "recoveryResidual": 0.005000000000000001, "recoverySlopePerSecond": -0.0025647254615758816, "recoveryStableMetric": {"mixedShare": 0.025, "time": 10.308}, "recoveryTimeSeconds": 3.308, "tierScores": {"finalMixedShare": 6, "minimum": 6, "peakMixedShare": 10, "recoveryTime": 6}, "validFrameCount": 4441, "validFrameRatio": 1.0}`

### TC-13 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-TRANSITION-EVENTS` | MUST-PASS | `2` | **PASS** | - |
| `HG-REARRANGE-WINDOW-1` | MUST-PASS / TIERED | `2.625` | **PASS** | 10 |
| `HG-PHASE-ORDER-1` | MUST-PASS | `true` | **PASS** | - |
| `HG-REARRANGE-WINDOW-2` | MUST-PASS / TIERED | `2.625` | **PASS** | 10 |
| `HG-PHASE-ORDER-2` | MUST-PASS | `true` | **PASS** | - |
| `HG-NO-TELEPORT` | MUST-PASS | `0.5` | **PASS** | - |
| `HG-CENTROID-DISPLACEMENT` | MUST-PASS | `0.3329999999999984` | **PASS** | - |
| `TIERED-PROCESS-SMOOTHNESS` | TIERED | `0.3329999999999984` | **PASS** | 10 |
| `HG-PIXEL-CLASSIFICATION-CONTINUITY` | MUST-PASS | `0.0` | **PASS** | - |
| `TIERED-TC13-MINIMUM` | TIERED | `10` | **PASS** | 10 |
| `HG-POST-EVENT-OBSERVATION` | MUST-PASS | `4.992000000000001` | **PASS** | - |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `1200` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"centroidDisplacementSources": {"frames": 0.0, "metric": 0.3329999999999984}, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "maxAdjacentCentroidDisplacementCells": 0.3329999999999984, "maxAdjacentPixelClassificationChange": 0.0, "maxDualShareDeviationPct": 0.0, "maxInterfaceStepCells": 0.5, "particleDriftPct": 0.0, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "pixelClassificationSources": ["LIQUID_METRIC changedCells/totalCells"], "postLastEventSeconds": 4.992000000000001, "tierScores": {"minimum": 10, "rearrange": [10, 10], "smoothness": 10}, "transitions": [{"event": "invert_on", "eventTime": 1.0, "rearrangeSeconds": 2.625, "stablePhaseOrder": true, "target": "inverted"}, {"event": "invert_off", "eventTime": 5.0, "rearrangeSeconds": 2.625, "stablePhaseOrder": true, "target": "upright"}], "validFrameCount": 1200, "validFrameRatio": 1.0}`

## P-006 liquid-mode objective check — 2026-10-15T16:24:49+08:00

- Case: `ALL`
- Commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Overall: **PASS**
- Self-check: `True`
- Frame inputs: `0`
- Parse errors: 0

### TC-11 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-STATIC-300-FRAMES` | MUST-PASS | `3620` | **PASS** | - |
| `HG-UPRIGHT-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-OBSERVATION-DURATION` | MUST-PASS | `30.992` | **PASS** | - |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `3720` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"finalPhaseOrder": true, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "longestStaticFrameRun": 3620, "maxDualShareDeviationPct": 0.0, "maxInterfaceAmplitudeCells": 1.0, "observationSeconds": 30.992, "particleDriftPct": 0.0, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "validFrameCount": 3720, "validFrameRatio": 1.0}`

### TC-12 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-SHAKE-EVENTS` | MUST-PASS | `{"start": 1, "stop": 1}` | **PASS** | - |
| `HG-RECOVERY-DURATION` | MUST-PASS | `30.0` | **PASS** | - |
| `HG-MIXED-RISE` | MUST-PASS / TIERED | `0.32999999999999996` | **PASS** | 10 |
| `HG-RECOVERY-RESIDUAL` | MUST-PASS / TIERED | `0.005000000000000001` | **PASS** | 8 |
| `HG-RECOVERY-SLOPE` | MUST-PASS | `-0.0025647254615758816` | **PASS** | - |
| `HG-RECOVERY-TIME` | MUST-PASS / TIERED | `3.308` | **PASS** | 6 |
| `TIERED-PEAK-MIXED-SHARE` | TIERED | `0.35` | **PASS** | 10 |
| `TIERED-RECOVERY-TIME` | TIERED | `3.308` | **PASS** | 6 |
| `TIERED-FINAL-MIXED-SHARE` | TIERED | `0.025` | **PASS** | 6 |
| `TIERED-TC12-MINIMUM` | TIERED | `6` | **PASS** | 6 |
| `HG-FINAL-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-FINAL-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `4441` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"baselineMixedFraction": 0.02, "finalInterfaceAmplitudeCells": 1.0, "finalPhaseOrder": true, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "maxDualShareDeviationPct": 0.0, "mixedRise": 0.32999999999999996, "particleDriftPct": 0.0, "peakMixedFraction": 0.35, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "recoveryDurationSeconds": 30.0, "recoveryResidual": 0.005000000000000001, "recoverySlopePerSecond": -0.0025647254615758816, "recoveryStableMetric": {"mixedShare": 0.025, "time": 10.308}, "recoveryTimeSeconds": 3.308, "tierScores": {"finalMixedShare": 6, "minimum": 6, "peakMixedShare": 10, "recoveryTime": 6}, "validFrameCount": 4441, "validFrameRatio": 1.0}`

### TC-13 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-TRANSITION-EVENTS` | MUST-PASS | `2` | **PASS** | - |
| `HG-REARRANGE-WINDOW-1` | MUST-PASS / TIERED | `2.625` | **PASS** | 10 |
| `HG-PHASE-ORDER-1` | MUST-PASS | `true` | **PASS** | - |
| `HG-REARRANGE-WINDOW-2` | MUST-PASS / TIERED | `2.625` | **PASS** | 10 |
| `HG-PHASE-ORDER-2` | MUST-PASS | `true` | **PASS** | - |
| `HG-NO-TELEPORT` | MUST-PASS | `0.5` | **PASS** | - |
| `HG-CENTROID-DISPLACEMENT` | MUST-PASS | `0.3329999999999984` | **PASS** | - |
| `TIERED-PROCESS-SMOOTHNESS` | TIERED | `0.3329999999999984` | **PASS** | 10 |
| `HG-PIXEL-CLASSIFICATION-CONTINUITY` | MUST-PASS | `0.0` | **PASS** | - |
| `TIERED-TC13-MINIMUM` | TIERED | `10` | **PASS** | 10 |
| `HG-POST-EVENT-OBSERVATION` | MUST-PASS | `4.992000000000001` | **PASS** | - |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `1200` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"centroidDisplacementSources": {"frames": 0.0, "metric": 0.3329999999999984}, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "maxAdjacentCentroidDisplacementCells": 0.3329999999999984, "maxAdjacentPixelClassificationChange": 0.0, "maxDualShareDeviationPct": 0.0, "maxInterfaceStepCells": 0.5, "particleDriftPct": 0.0, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "pixelClassificationSources": ["LIQUID_METRIC changedCells/totalCells"], "postLastEventSeconds": 4.992000000000001, "tierScores": {"minimum": 10, "rearrange": [10, 10], "smoothness": 10}, "transitions": [{"event": "invert_on", "eventTime": 1.0, "rearrangeSeconds": 2.625, "stablePhaseOrder": true, "target": "inverted"}, {"event": "invert_off", "eventTime": 5.0, "rearrangeSeconds": 2.625, "stablePhaseOrder": true, "target": "upright"}], "validFrameCount": 1200, "validFrameRatio": 1.0}`

## P-006 liquid-mode objective check — 2026-10-15T16:44:54+08:00

- Case: `ALL`
- Commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Overall: **PASS**
- Self-check: `True`
- Frame inputs: `0`
- Parse errors: 0

### TC-11 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-STATIC-300-FRAMES` | MUST-PASS | `3620` | **PASS** | - |
| `HG-UPRIGHT-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-OBSERVATION-DURATION` | MUST-PASS | `30.992` | **PASS** | - |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `3720` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"finalPhaseOrder": true, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "longestStaticFrameRun": 3620, "maxDualShareDeviationPct": 0.0, "maxInterfaceAmplitudeCells": 1.0, "observationSeconds": 30.992, "particleDriftPct": 0.0, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "validFrameCount": 3720, "validFrameRatio": 1.0}`

### TC-12 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-SHAKE-EVENTS` | MUST-PASS | `{"start": 1, "stop": 1}` | **PASS** | - |
| `HG-RECOVERY-DURATION` | MUST-PASS | `30.0` | **PASS** | - |
| `HG-MIXED-RISE` | MUST-PASS / TIERED | `0.32999999999999996` | **PASS** | 10 |
| `HG-RECOVERY-RESIDUAL` | MUST-PASS / TIERED | `0.005000000000000001` | **PASS** | 8 |
| `HG-RECOVERY-SLOPE` | MUST-PASS | `-0.0025647254615758816` | **PASS** | - |
| `HG-RECOVERY-TIME` | MUST-PASS / TIERED | `3.308` | **PASS** | 6 |
| `TIERED-PEAK-MIXED-SHARE` | TIERED | `0.35` | **PASS** | 10 |
| `TIERED-RECOVERY-TIME` | TIERED | `3.308` | **PASS** | 6 |
| `TIERED-FINAL-MIXED-SHARE` | TIERED | `0.025` | **PASS** | 6 |
| `TIERED-TC12-MINIMUM` | TIERED | `6` | **PASS** | 6 |
| `HG-FINAL-PHASE-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-FINAL-INTERFACE-AMPLITUDE` | MUST-PASS / TIERED | `1.0` | **PASS** | 10 |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `4441` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"baselineMixedFraction": 0.02, "finalInterfaceAmplitudeCells": 1.0, "finalPhaseOrder": true, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "maxDualShareDeviationPct": 0.0, "mixedRise": 0.32999999999999996, "particleDriftPct": 0.0, "peakMixedFraction": 0.35, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "recoveryDurationSeconds": 30.0, "recoveryResidual": 0.005000000000000001, "recoverySlopePerSecond": -0.0025647254615758816, "recoveryStableMetric": {"mixedShare": 0.025, "time": 10.308}, "recoveryTimeSeconds": 3.308, "tierScores": {"finalMixedShare": 6, "minimum": 6, "peakMixedShare": 10, "recoveryTime": 6}, "validFrameCount": 4441, "validFrameRatio": 1.0}`

### TC-13 — PASS

| 指标 | 类别 | 实测 | 判定 | 分档 |
|---|---|---:|---|---:|
| `HG-COUNT-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `HG-PIXEL-AREA-CONSERVATION` | MUST-PASS | `0.0` | **PASS** | - |
| `VV-01-DUAL-SHARE` | MUST-PASS / TIERED | `0.0` | **PASS** | 10 |
| `HG-TIMESTAMP-ORDER` | MUST-PASS | `true` | **PASS** | - |
| `HG-TRANSITION-EVENTS` | MUST-PASS | `2` | **PASS** | - |
| `HG-REARRANGE-WINDOW-1` | MUST-PASS / TIERED | `2.625` | **PASS** | 10 |
| `HG-PHASE-ORDER-1` | MUST-PASS | `true` | **PASS** | - |
| `HG-REARRANGE-WINDOW-2` | MUST-PASS / TIERED | `2.625` | **PASS** | 10 |
| `HG-PHASE-ORDER-2` | MUST-PASS | `true` | **PASS** | - |
| `HG-NO-TELEPORT` | MUST-PASS | `0.5` | **PASS** | - |
| `HG-CENTROID-DISPLACEMENT` | MUST-PASS | `0.3329999999999984` | **PASS** | - |
| `TIERED-PROCESS-SMOOTHNESS` | TIERED | `0.3329999999999984` | **PASS** | 10 |
| `HG-PIXEL-CLASSIFICATION-CONTINUITY` | MUST-PASS | `0.0` | **PASS** | - |
| `TIERED-TC13-MINIMUM` | TIERED | `10` | **PASS** | 10 |
| `HG-POST-EVENT-OBSERVATION` | MUST-PASS | `4.992000000000001` | **PASS** | - |
| `HG-VALID-FRAME-COUNT` | MUST-PASS | `1200` | **PASS** | - |
| `HG-VALID-FRAME-RATIO` | MUST-PASS | `1.0` | **PASS** | - |
| `HG-FPS-P95` | MUST-PASS | `119.99999999999999` | **PASS** | - |
| `HG-PHYSICS-TIME-P95` | MUST-PASS | `1.5` | **PASS** | - |
| `HG-BACKPRESSURE-FRAME-FIELDS` | MUST-PASS | `0` | **PASS** | - |

实测汇总：`{"centroidDisplacementSources": {"frames": 0.0, "metric": 0.3329999999999984}, "fpsP95": 119.99999999999999, "initialParticleShares": {"particleHeavy": 0.5833333333333334, "particleLight": 0.4166666666666667}, "invalidFramesWithClippedTime": 0, "maxAdjacentCentroidDisplacementCells": 0.3329999999999984, "maxAdjacentPixelClassificationChange": 0.0, "maxDualShareDeviationPct": 0.0, "maxInterfaceStepCells": 0.5, "particleDriftPct": 0.0, "physicsTimeMsP95": 1.5, "pixelAreaDriftPct": 0.0, "pixelClassificationSources": ["LIQUID_METRIC changedCells/totalCells"], "postLastEventSeconds": 4.992000000000001, "tierScores": {"minimum": 10, "rearrange": [10, 10], "smoothness": 10}, "transitions": [{"event": "invert_on", "eventTime": 1.0, "rearrangeSeconds": 2.625, "stablePhaseOrder": true, "target": "inverted"}, {"event": "invert_off", "eventTime": 5.0, "rearrangeSeconds": 2.625, "stablePhaseOrder": true, "target": "upright"}], "validFrameCount": 1200, "validFrameRatio": 1.0}`

