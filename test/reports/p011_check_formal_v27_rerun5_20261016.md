# P-011 objective check

- Route: `bottom-creep-v2.7`
- Overall: **BLOCKED(missing_hook)**

| Case | Measured | Score |
|---|---|---:|
| tcP01101 | `{"bottomCreepResponseCount": 0, "bottomDirectionError": 519, "bottomDirectionSamples": 519, "bottomDirectionSignSwitches": 0, "directionAccuracy": 1.0, "directionError": 0, "score": 0, "t90P95Ms": null}` | 0 |
| tcP01102 | `{"feH": 0.08082364044524554, "feL": 0.4366067866386125, "lagHMs": -178.0, "lagLMs": -177.0, "pearsonH": 0.41127482592132925, "pearsonL": 0.30197589591416657, "score": 0}` | 0 |
| tcP01103 | `{"maxSubstepBh": 0.0667263592037699, "maxSubstepBl": 1.2395759236863597, "maxUncommandedTiltStep": 6.1200000000000045, "reverseDirectionError": 0, "score": 6, "zeroResidual": 0}` | 6 |
| tcP01104 | `{"crashCount": 0, "driftHeavy": 3.1579677144893343e-16, "driftLight": 4.736951571734001e-16, "driftWater": 1.495879443705474e-15, "modelErrorCount": 0, "nonFiniteCount": 0, "outOfRangeCount": 0}` | - |

## High-pressure total-clip segments

Independent segments require `totalMotionBudgetClipped > 0` bounded by `totalMotionBudgetClipped = 0` or separate command phases.

- Segment count: 11
- 1: `{"clipSampleCount": 113, "endTime": 1792136849053, "maxClipped": 4.136583631872759, "maxTotalUsed": 3.160925369379179, "startTime": 1792136847934}`
- 2: `{"clipSampleCount": 46, "endTime": 1792136851074, "maxClipped": 4.485777557966883, "maxTotalUsed": 2.708525898863331, "startTime": 1792136849805}`
- 3: `{"clipSampleCount": 1, "endTime": 1792136851106, "maxClipped": 0.0691641493270913, "maxTotalUsed": 1.424666230113646, "startTime": 1792136851106}`
- 4: `{"clipSampleCount": 111, "endTime": 1792136852810, "maxClipped": 4.517658711803911, "maxTotalUsed": 2.924527035451686, "startTime": 1792136851740}`
- 5: `{"clipSampleCount": 4, "endTime": 1792136852860, "maxClipped": 0.08926496694441116, "maxTotalUsed": 1.5024178384632134, "startTime": 1792136852842}`
- 6: `{"clipSampleCount": 121, "endTime": 1792136854780, "maxClipped": 4.338757949287943, "maxTotalUsed": 2.767023436682166, "startTime": 1792136853593}`
- 7: `{"clipSampleCount": 3, "endTime": 1792136854829, "maxClipped": 0.26574657894923387, "maxTotalUsed": 1.494489188624783, "startTime": 1792136854812}`
- 8: `{"clipSampleCount": 111, "endTime": 1792136856566, "maxClipped": 4.546983962332272, "maxTotalUsed": 2.8957408306975467, "startTime": 1792136855446}`
- 9: `{"clipSampleCount": 2, "endTime": 1792136856599, "maxClipped": 0.12961540143998265, "maxTotalUsed": 1.4341076898217686, "startTime": 1792136856598}`
- 10: `{"clipSampleCount": 126, "endTime": 1792136858569, "maxClipped": 4.350161526600378, "maxTotalUsed": 2.7561689102942784, "startTime": 1792136857316}`
- 11: `{"clipSampleCount": 5, "endTime": 1792136858636, "maxClipped": 0.3621085211456567, "maxTotalUsed": 1.472994712391543, "startTime": 1792136858601}`

## MUST-PASS

- driftBelow0_1Percent: PASS
- nonFiniteCountZero: PASS
- outOfRangeCountZero: PASS
- modelErrorZero: PASS
- crashZero: PASS
- maxSubstepBelow5: PASS
- maxUncommandedTiltStepBelow2: FAIL
- zeroResidualBelow1: PASS
- reverseDirectionErrorZero: PASS
- bottomBudgetViolationZero: PASS
- totalBudgetViolationZero: PASS
- directionCorrectAll: PASS
- bottomDirectionCorrectAll: FAIL
- independentBudgetIsolation: PASS
- highPressureEvidenceCount: PASS
