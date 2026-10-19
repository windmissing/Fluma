# P-011 objective check

- Route: `bottom-creep-v2.7`
- Overall: **BLOCKED(missing_hook)**

| Case | Measured | Score |
|---|---|---:|
| tcP01101 | `{"bottomCreepResponseCount": 2, "bottomDirectionError": 41, "bottomDirectionSamples": 43, "bottomDirectionSignSwitches": 1, "directionAccuracy": 1.0, "directionError": 0, "score": 10, "t90P95Ms": 0}` | 10 |
| tcP01102 | `{"feH": 0.17677914574920367, "feL": 0.16186479315258898, "lagHMs": -152.0, "lagLMs": -14.0, "pearsonH": 0.7681016501034623, "pearsonL": 0.6615982813026072, "score": 0}` | 0 |
| tcP01103 | `{"maxSubstepBh": 0.24275469730721966, "maxSubstepBl": 3.9133703598163976, "maxUncommandedTiltStep": 11.52, "reverseDirectionError": 0, "score": 2, "zeroResidual": 0}` | 2 |
| tcP01104 | `{"crashCount": 0, "driftHeavy": 3.1579677144893343e-16, "driftLight": 6.315935428978669e-16, "driftWater": 1.0471156105938318e-15, "modelErrorCount": 0, "nonFiniteCount": 0, "outOfRangeCount": 0}` | - |

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
- directionCorrectAll: FAIL
- bottomDirectionCorrectAll: FAIL
- independentBudgetIsolation: PASS
- highPressureEvidenceCount: FAIL
