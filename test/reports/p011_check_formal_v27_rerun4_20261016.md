# P-011 objective check

- Route: `bottom-creep-v2.7`
- Overall: **BLOCKED(missing_hook)**

| Case | Measured | Score |
|---|---|---:|
| tcP01101 | `{"bottomCreepResponseCount": 0, "bottomDirectionError": 898, "bottomDirectionSamples": 898, "bottomDirectionSignSwitches": 0, "directionAccuracy": 1.0, "directionError": 0, "score": 0, "t90P95Ms": null}` | 0 |
| tcP01102 | `{"feH": 0.21213652069158473, "feL": 0.29010526662369407, "lagHMs": -155.0, "lagLMs": -144.0, "pearsonH": 0.71812659490429, "pearsonL": 0.5043645671934598, "score": 0}` | 0 |
| tcP01103 | `{"maxSubstepBh": 1.705176385448027, "maxSubstepBl": 14.167695303383717, "maxUncommandedTiltStep": 7.920000000000002, "reverseDirectionError": 0, "score": 0, "zeroResidual": 0}` | 0 |
| tcP01104 | `{"crashCount": 2, "driftHeavy": 3.1579677144893343e-16, "driftLight": 6.315935428978669e-16, "driftWater": 3.590110664893141e-15, "modelErrorCount": 0, "nonFiniteCount": 0, "outOfRangeCount": 0}` | - |

## High-pressure total-clip segments

Independent segments require `totalMotionBudgetClipped > 0` bounded by `totalMotionBudgetClipped = 0` or separate command phases.

- Segment count: 21
- 1: `{"clipSampleCount": 53, "endTime": 1792136549387, "maxClipped": 3.9460951446298984, "maxTotalUsed": 2.390445148067597, "startTime": 1792136547584}`
- 2: `{"clipSampleCount": 72, "endTime": 1792136552696, "maxClipped": 4.0117193541894265, "maxTotalUsed": 2.6459466415464767, "startTime": 1792136550656}`
- 3: `{"clipSampleCount": 1, "endTime": 1792136552726, "maxClipped": 0.1914277805734233, "maxTotalUsed": 1.313094970184814, "startTime": 1792136552726}`
- 4: `{"clipSampleCount": 95, "endTime": 1792136555548, "maxClipped": 4.20379176168228, "maxTotalUsed": 2.6919684161497486, "startTime": 1792136553677}`
- 5: `{"clipSampleCount": 1, "endTime": 1792136555580, "maxClipped": 0.09975905346619307, "maxTotalUsed": 1.392081650949897, "startTime": 1792136555580}`
- 6: `{"clipSampleCount": 63, "endTime": 1792136558719, "maxClipped": 4.156376990223851, "maxTotalUsed": 2.588530886813489, "startTime": 1792136557401}`
- 7: `{"clipSampleCount": 97, "endTime": 1792136561724, "maxClipped": 4.5771595153354765, "maxTotalUsed": 2.978746977068186, "startTime": 1792136559820}`
- 8: `{"clipSampleCount": 91, "endTime": 1792136565030, "maxClipped": 4.065848073013419, "maxTotalUsed": 2.6923628155572663, "startTime": 1792136563428}`
- 9: `{"clipSampleCount": 4, "endTime": 1792136565081, "maxClipped": 0.1776361294445077, "maxTotalUsed": 1.304394314213034, "startTime": 1792136565062}`
- 10: `{"clipSampleCount": 1, "endTime": 1792136565112, "maxClipped": 0.08721102775913872, "maxTotalUsed": 1.306015406372586, "startTime": 1792136565112}`
- 11: `{"clipSampleCount": 102, "endTime": 1792136640820, "maxClipped": 4.1146375078823505, "maxTotalUsed": 3.01064034284931, "startTime": 1792136639817}`
- 12: `{"clipSampleCount": 4, "endTime": 1792136640870, "maxClipped": 0.570057410401858, "maxTotalUsed": 1.5452534038162231, "startTime": 1792136640852}`
- 13: `{"clipSampleCount": 1, "endTime": 1792136640902, "maxClipped": 0.03268772108520175, "maxTotalUsed": 1.3970818936242824, "startTime": 1792136640902}`
- 14: `{"clipSampleCount": 127, "endTime": 1792136642957, "maxClipped": 4.345121667654766, "maxTotalUsed": 2.7176648432064323, "startTime": 1792136641670}`
- 15: `{"clipSampleCount": 1, "endTime": 1792136642990, "maxClipped": 0.013047478999002795, "maxTotalUsed": 1.44269918702545, "startTime": 1792136642990}`
- 16: `{"clipSampleCount": 115, "endTime": 1792136644743, "maxClipped": 4.520141318931443, "maxTotalUsed": 2.903056595212346, "startTime": 1792136643573}`
- 17: `{"clipSampleCount": 125, "endTime": 1792136646713, "maxClipped": 4.337875867473031, "maxTotalUsed": 2.7733405321780493, "startTime": 1792136645460}`
- 18: `{"clipSampleCount": 1, "endTime": 1792136646745, "maxClipped": 0.05634191461188473, "maxTotalUsed": 1.4423665000188999, "startTime": 1792136646745}`
- 19: `{"clipSampleCount": 117, "endTime": 1792136648516, "maxClipped": 4.73424244070565, "maxTotalUsed": 2.928452763310732, "startTime": 1792136647346}`
- 20: `{"clipSampleCount": 120, "endTime": 1792136650469, "maxClipped": 4.36002064334792, "maxTotalUsed": 2.762581692925258, "startTime": 1792136649249}`
- 21: `{"clipSampleCount": 4, "endTime": 1792136650519, "maxClipped": 0.29378839621435676, "maxTotalUsed": 1.4893033392495496, "startTime": 1792136650501}`

## MUST-PASS

- driftBelow0_1Percent: PASS
- nonFiniteCountZero: PASS
- outOfRangeCountZero: PASS
- modelErrorZero: PASS
- crashZero: FAIL
- maxSubstepBelow5: FAIL
- maxUncommandedTiltStepBelow2: FAIL
- zeroResidualBelow1: PASS
- reverseDirectionErrorZero: PASS
- bottomBudgetViolationZero: PASS
- totalBudgetViolationZero: PASS
- directionCorrectAll: PASS
- bottomDirectionCorrectAll: FAIL
- independentBudgetIsolation: PASS
- highPressureEvidenceCount: PASS
