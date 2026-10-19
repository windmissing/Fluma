# P-011 objective check

- Route: `bottom-creep-v2.7`
- Overall: **PASS**

| Case | Measured | Score |
|---|---|---:|
| tcP01101 | `{"bottomCreepResponseCount": 2, "directionAccuracy": 1.0, "directionError": 0, "score": 10, "t90P95Ms": 0}` | 10 |
| tcP01102 | `{"feH": 0.0174350459074043, "feL": 0.017328384450088885, "lagHMs": 0.0, "lagLMs": 0.0, "pearsonH": 0.9935582784143583, "pearsonL": 0.9934078917831574, "score": 10}` | 10 |
| tcP01103 | `{"maxSubstepBh": 0.4031999999999982, "maxSubstepBl": 0.4031999999999982, "maxUncommandedTiltStep": 0.0, "reverseDirectionError": 0, "score": 10, "zeroResidual": 0.0}` | 10 |
| tcP01104 | `{"crashCount": 0, "driftHeavy": 0.0, "driftLight": 0.0, "driftWater": 0.0, "modelErrorCount": 0, "nonFiniteCount": 0, "outOfRangeCount": 0}` | - |

## MUST-PASS

- driftBelow0_1Percent: PASS
- nonFiniteCountZero: PASS
- outOfRangeCountZero: PASS
- modelErrorZero: PASS
- crashZero: PASS
- maxSubstepBelow5: PASS
- maxUncommandedTiltStepBelow2: PASS
- zeroResidualBelow1: PASS
- reverseDirectionErrorZero: PASS
- bottomBudgetViolationZero: PASS
- totalBudgetViolationZero: PASS
- directionCorrectAll: PASS
