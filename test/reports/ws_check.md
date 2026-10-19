# WaterSand objective check

| Check | Status | Measured | Threshold | Evidence |
|---|---|---|---|---:|
| rate | PASS | `{"activeFrames": 3, "dormantFrames": 0, "dormantSource": "field", "frames": 3, "maxSandTopSpeedObserved": 24.0, "maxSubstepBudgetRate": 24.0, "vSandTopMax": 28.0}` | `{"activeFrames": ">= 1", "paramsVersion": "v2.2", "substepBudgetRate": "<= 28", "substepSpeed": "<= 30", "vSandTopMax": 28}` | 3 |
| rateChecker_skipsDormantFrames | PASS | `{"activeFrames": 1, "dormantFrames": 1, "dormantSource": "field", "frames": 2, "maxSandTopSpeedObserved": 18.0, "maxSubstepBudgetRate": 18.0, "vSandTopMax": 28.0}` | `{"activeFrames": 1, "dormantFrames": 1, "rate": "PASS"}` | 2 |
| rateChecker_legacyDormantInference | PASS | `{"activeFrames": 2, "dormantFrames": 1, "dormantSource": "event", "frames": 3, "maxSandTopSpeedObserved": 18.0, "maxSubstepBudgetRate": 18.0, "vSandTopMax": 28.0}` | `{"activeFrames": 2, "dormantFrames": 1, "dormantSource": "event"}` | 3 |
