## P-001 liquid-mode objective check — 2026-10-15T15:40:42+08:00

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

