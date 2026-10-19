# P-011 objective check

- Route: `bottom-creep-v2.7`
- Overall: **BLOCKED(missing_hook)**

| Case | Measured | Score |
|---|---|---:|
| tcP01101 | `{"bottomCreepResponseCount": 0, "bottomDirectionError": 240, "bottomDirectionSamples": 240, "bottomDirectionSignSwitches": 0, "directionAccuracy": 1.0, "directionError": 0, "score": 0, "t90P95Ms": null}` | 0 |
| tcP01102 | `{"feH": 0.07762306976663658, "feL": 0.4619952457358667, "lagHMs": 11.0, "lagLMs": -95.0, "pearsonH": 0.7716015189083835, "pearsonL": 0.24926578448213862, "score": 0}` | 0 |
| tcP01103 | `{"maxSubstepBh": 0.2501437560487716, "maxSubstepBl": 3.43434165656474, "maxUncommandedTiltStep": 6.48, "reverseDirectionError": 0, "score": 2, "zeroResidual": 88}` | 2 |
| tcP01104 | `{"crashCount": 1, "driftHeavy": 3.1579677144893343e-16, "driftLight": 4.736951571734001e-16, "driftWater": 1.1967035549643792e-15, "modelErrorCount": 0, "nonFiniteCount": 0, "outOfRangeCount": 0}` | - |

## High-pressure total-clip segments

Independent segments require `totalMotionBudgetClipped > 0` bounded by `totalMotionBudgetClipped = 0` or separate command phases.

- Segment count: 7
- 1: `{"clipSampleCount": 37, "endTime": 1792136394601, "maxClipped": 3.9488470939894347, "maxTotalUsed": 2.7899307816568193, "startTime": 1792136393648}`
- 2: `{"clipSampleCount": 8, "endTime": 1792136394700, "maxClipped": 0.3501703155338312, "maxTotalUsed": 2.8800352518497894, "startTime": 1792136394633}`
- 3: `{"clipSampleCount": 148, "endTime": 1792136398574, "maxClipped": 4.689577621519179, "maxTotalUsed": 2.8068375069334826, "startTime": 1792136395501}`
- 4: `{"clipSampleCount": 48, "endTime": 1792136400577, "maxClipped": 4.384181973422698, "maxTotalUsed": 2.7517882059874843, "startTime": 1792136399441}`
- 5: `{"clipSampleCount": 1, "endTime": 1792136400609, "maxClipped": 0.09696096980452368, "maxTotalUsed": 1.435679019436403, "startTime": 1792136400609}`
- 6: `{"clipSampleCount": 32, "endTime": 1792136401730, "maxClipped": 4.52709443167604, "maxTotalUsed": 2.4499780982315436, "startTime": 1792136401210}`
- 7: `{"clipSampleCount": 31, "endTime": 1792136403751, "maxClipped": 4.266926981926571, "maxTotalUsed": 2.464977106992015, "startTime": 1792136403448}`

## MUST-PASS

- driftBelow0_1Percent: PASS
- nonFiniteCountZero: PASS
- outOfRangeCountZero: PASS
- modelErrorZero: PASS
- crashZero: FAIL
- maxSubstepBelow5: PASS
- maxUncommandedTiltStepBelow2: FAIL
- zeroResidualBelow1: FAIL
- reverseDirectionErrorZero: PASS
- bottomBudgetViolationZero: PASS
- totalBudgetViolationZero: PASS
- directionCorrectAll: PASS
- bottomDirectionCorrectAll: FAIL
- independentBudgetIsolation: PASS
- highPressureEvidenceCount: PASS
