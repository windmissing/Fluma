# P-011 formal rerun - bottom-creep-v2.7 - 2026-10-17

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- Date: 2026-10-17 (Asia/Shanghai)
- Signed HAP: `entry/build/default/outputs/default/entry-default-p011-20261017-signed.hap`
- Signed HAP SHA256: `1A83DE9B83FA8DF549B09CB648543E66754A885A6ADA1624C8C7360224664746`
- Install result: `install bundle successfully.`
- Suite JSON: `test/artifacts/p011-formal-prep-20261016/p011-suite.json`
- Suite Markdown: `test/artifacts/p011-formal-prep-20261016/p011-suite.md`

## Pre-test script corrections

1. The suite now opens hilog logs with `FileShare::ReadWrite`, eliminating the capture-race false failure without changing any acceptance threshold.
2. The suite now performs `aa force-stop` before starting each hilog capture, preventing the prior run from appearing as `runWithoutReset`. This correction also does not change any acceptance threshold.
3. TC02 was not run in this task because the requested mode was `PressureSegments`; if reviewer requires TC02, rerun `-Mode All`.

## New schema identity and aggregation

| Log | runId | Reset markers | Sequence range | FrameId range | SubstepId range | Metric count | Bottom count |
|---|---|---:|---:|---:|---:|---:|---:|
| full-load | `p011-run-1792201100696-3` | 1 | 1-2077 | 1-1250 | 1-2077 | 2077 | 2077 |
| clip-segment-a-positive | `p011-run-1792201122436-3` | 1 | 1-2021 | 1-1211 | 1-2021 | 2021 | 2021 |
| clip-segment-b-negative | `p011-run-1792201143593-3` | 1 | 1-2029 | 1-1211 | 1-2029 | 2029 | 2029 |
| clip-segment-c-positive-repeat | `p011-run-1792201164886-3` | 1 | 1-2015 | 1-1208 | 1-2015 | 2015 | 2015 |

All four logs had `missingHooks=[]`, 0 parse errors, 0 timestamp-order errors, `routeMismatch=false`, no identity mismatch, and no sequence/frameId/substepId regression. Suite `allLogsComplete` therefore passed.

## Per-segment objective results

| Log | Overall | Direction accuracy | Direction error | Bottom-direction samples | Bottom-direction error | Max substep BH | Max substep BL | Max uncommanded tilt step | Total-clip segments | TC01 score | TC02 score | TC03 score | High-pressure evidence |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| full-load | FAIL | 1.0 | 0 | 2076 | 0 | 0.0133143973 | 0.2508220340 | 0 | 1 | 0 | 0 | 10 | FAIL |
| clip-segment-a-positive | FAIL | 1.0 | 0 | 2020 | 0 | 0.0133197182 | 0.2503476837 | 0 | 2 | 0 | 0 | 10 | FAIL |
| clip-segment-b-negative | FAIL | 1.0 | 0 | 2028 | 0 | 0.0134895173 | 0.2719763704 | 0 | 1 | 0 | 0 | 10 | FAIL |
| clip-segment-c-positive-repeat | FAIL | 1.0 | 0 | 2014 | 0 | 0.0133273340 | 0.2516600096 | 0 | 1 | 0 | 0 | 10 | FAIL |

The high-pressure acceptance requires at least three total-clip segments. No pressure segment met that count, so this is the sole per-log hard-gate failure.

## Hard gates and invariants

| Metric | Result | Evidence |
|---|---|---|
| Parse/order/route completeness | PASS | 0 parse errors, 0 timestamp-order errors, route mismatch false in every log |
| Sequence/frameId/substepId regression | PASS | Regression counts were 0 in every log |
| Reset-boundary identity | PASS | Each observed run had exactly one reset marker |
| Non-finite values | PASS | 0 in every log |
| Out-of-range values | PASS | 0 in every log |
| Model errors | PASS | 0 in every log |
| Crash/ANR | PASS | 0 in every log |
| Water drift < 0.1% | PASS | Maximum `3.290934776152043e-15` |
| Heavy drift < 0.1% | PASS | `3.1579677144893343e-16` |
| Light drift < 0.1% | PASS | Maximum `2.052679014418067e-15` |
| Bottom/total motion budget violations | PASS | Violation rate 0 in every log |
| Full-load budget limit errors | PASS | Error rate 0 in every log |
| High-pressure total-clip segments >= 3 | FAIL | Only 1/2/1/1 segments across the four logs |
| Suite sign-switch coverage | FAIL | 0 positive-to-negative and 0 negative-to-positive switches were observed within individual runs |

## Suite decision

Overall: **FAIL**

- Suite MUST-PASS: `allLogsComplete=PASS`; `allLogsPass=FAIL`; `suiteSignSwitchCoverage=FAIL`.
- The decisive functional hard-gate failure is insufficient high-pressure total-clip segmentation in all four pressure runs.
- No threshold was widened and no FAIL was converted to PASS.

## Failure analysis and routing

- Route high-pressure total-clip segmentation failure to **designer**. The current implementation produces 1 or 2 contiguous clip segments per pressure run, below the required 3.
- Route suite sign-switch aggregation failure to **designer** for a follow-up behavior/design decision: each PressureSegments log contains one pressure sign per run, and the current suite does not observe the required opposite-sign transitions within those runs. Any checker aggregation change must be validated as a test-design correction, not a functional threshold relaxation.

## Reproducibility

- Raw logs: `test/artifacts/p011-formal-prep-20261016/p011-{full-load,clip-segment-a-positive,clip-segment-b-negative,clip-segment-c-positive-repeat}.log`
- Per-log checker outputs: `*.check.json` and `*.check.md` under the same directory.
- Suite command: `powershell -ExecutionPolicy Bypass -File .\test\p011_tc02_formal_suite.ps1 -Mode PressureSegments`

---

# P-011 formal rerun with TC02 - bottom-creep-v2.7 - 2026-10-17

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- Date: 2026-10-17 (Asia/Shanghai)
- Unsigned HAP: `entry/build/default/outputs/default/entry-default-unsigned.hap`
- Signed HAP: `entry/build/default/outputs/default/entry-default-p011-20261017-formal-signed.hap`
- Signed HAP SHA256: `6C627A346B754E5BACA9BC1B6137107405CBDD9040D45FD500E9951F70E9118D`
- Install result: `install bundle successfully.`
- Suite command: `powershell -ExecutionPolicy Bypass -File .\test\p011_tc02_formal_suite.ps1 -Mode All`
- Suite JSON: `test/artifacts/p011-formal-prep-20261016/p011-suite.json`
- Suite Markdown: `test/artifacts/p011-formal-prep-20261016/p011-suite.md`
- TC02 schedule validation JSON: `test/artifacts/p011-formal-prep-20261016/p011-tc02-formal.tc02-validation.json`

The unsigned HAP was signed manually with the existing `p001-sign-cert.cer`, `entry-default.p7b`, and SDK `OpenHarmony.p12` key material. No signing material was modified. All five suite logs were passed to `test/p011_check.py` in one checker invocation.

## Identity and completeness

| Log | runId | Reset count | Sequence range | FrameId range | SubstepId range | Target phases | Metrics |
|---|---|---:|---:|---:|---:|---:|---:|
| TC02 | `p011-run-1792206096783-3` | 1 | 1-14807 | 1-8806 | 1-14807 | 531 | 14807 |
| full-load | `p011-run-1792206245338-3` | 1 | 1-2038 | 1-1216 | 1-2038 | 1 | 2038 |
| clip-segment-a-positive | `p011-run-1792206266732-3` | 1 | 1-2030 | 1-1210 | 1-2030 | 1 | 2030 |
| clip-segment-b-negative | `p011-run-1792206287966-3` | 1 | 1-2022 | 1-1208 | 1-2022 | 1 | 2022 |
| clip-segment-c-positive-repeat | `p011-run-1792206309213-3` | 1 | 1-2028 | 1-1212 | 1-2028 | 1 | 2028 |

All five logs had no missing hooks, 0 parse errors, 0 timestamp-order errors, no route mismatch, no identity mismatch, and 0 sequence/frameId/substepId regressions. TC02 schedule validation passed with 531/531 script events and `maxAdjacentAngleStepDeg=1.0`.

## Hard gates and invariants

| Metric | Result | Evidence |
|---|---|---|
| Parse/order/route completeness | PASS | 0 errors/mismatch in all 5 logs |
| Identity/reset observability | PASS | Each of 5 runs had exactly 1 reset marker |
| Sequence/frameId/substepId regression | PASS | 0 in all 5 logs |
| Non-finite values | PASS | 0 in all 5 logs |
| Out-of-range values | PASS | 0 in all 5 logs |
| Model errors | PASS | 0 in all 5 logs |
| Crash/ANR | PASS | 0 in all 5 logs |
| Water drift < 0.1% | PASS | Maximum `1.7501789491354047e-14` |
| Heavy drift < 0.1% | PASS | Maximum `3.1579677144893343e-16` |
| Light drift < 0.1% | PASS | Maximum `9.15810637201907e-15` |
| Bottom budget violations = 0 | PASS | 0 in all 5 logs |
| Total budget violations = 0 | PASS | 0 in all 5 logs |
| Direction correctness | PASS | 100% direction accuracy; 0 direction errors; TC02 had 12,915 bottom-direction samples with 0 error |
| TC02 full-load isolation | FAIL | TC02 log reports `fullLoadSamples=0`, so checker `independentBudgetIsolation=false`; full-load log itself has 2,033 full-load samples and limit error rate 0 |
| Pressure total clip segments >= 3 suite-wide | PASS | 4 pressure logs, 5 total clip segments (`1/1/1/2`) |
| Sign-switch coverage in same run | FAIL | 0 positive-to-negative and 0 negative-to-positive switches within any run |

## Tiered metric results

| Log | TC01 score | TC02 score | TC03 score | Minimum rule |
|---|---:|---:|---:|---|
| TC02 | 0 | 0 | 10 | FAIL: minimum score < 6 |
| full-load | 0 | 0 | 10 | FAIL: minimum score < 6 |
| clip-segment-a-positive | 0 | 0 | 10 | FAIL: minimum score < 6 |
| clip-segment-b-negative | 0 | 0 | 10 | FAIL: minimum score < 6 |
| clip-segment-c-positive-repeat | 0 | 2 | 10 | FAIL: minimum score < 6 |

Direction was objective and correct, but the documented TC01/TC02 quality scores did not reach the tiered pass line.

## Suite decision

Overall: **FAIL**

- Suite MUST-PASS: `suiteInputComplete=PASS`; `allLogsComplete=PASS`; `allLogsPass=FAIL`; `highPressureEvidenceCount=PASS`; `suiteSignSwitchCoverage=FAIL`.
- The TC02 per-log hard gate failed on checker `independentBudgetIsolation` because `fullLoadSamples=0` in the TC02 log.
- The pressure logs passed suite-wide high-pressure segmentation, but all sign-switch counts remained zero.
- Tiered TC01/TC02 scores remained below the pass line.

## Failure analysis and routing

1. Route the TC02 full-load isolation gate to **designer**: the gate requires `fullLoadSamples > 0` even for a log that is explicitly the TC02 variable-tilt schedule, not a full-load pressure segment. The physical full-load evidence itself passed. This is an aggregation/test-design conflict, not a full-load budget behavior failure.
2. Route zero in-run sign-switch coverage and TC01/TC02 quality scores to **designer** for behavior/design follow-up. The measurement contains TC02 positive and negative command phases, but no opposite-sign bottom-direction transition was recorded.
3. No developer change is authorized by this report until designer returns the design decision. No acceptance threshold was widened and no FAIL was converted to PASS.

## Reproducibility

- Raw logs: `test/artifacts/p011-formal-prep-20261016/p011-{tc02-formal,full-load,clip-segment-a-positive,clip-segment-b-negative,clip-segment-c-positive-repeat}.log`
- Per-log checker outputs: `*.check.json` and `*.check.md` under the same directory.
- Signing log: `test/artifacts/p011-sign-final-20261017.log`
- Install log: `test/artifacts/p011-install-tc02-formal-20261017.log`
- Suite console log: `test/artifacts/p011-20261017-formal-mode-all-console.log`

---

# P-011 checker correction rerun - bottom-creep-v2.7.1 test-caliber - 2026-10-18

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- Date: 2026-10-18 (Asia/Shanghai)
- Suite command: `powershell -ExecutionPolicy Bypass -File .\test\p011_tc02_formal_suite.ps1 -Mode All`
- Suite JSON: `test/artifacts/p011-formal-prep-20261016/p011-suite.json`
- Suite Markdown: `test/artifacts/p011-formal-prep-20261016/p011-suite.md`
- TC02 schedule validation: PASS, 531/531 events

## Checker correction verification

| Metric | Result | Evidence |
|---|---|---|
| Suite input/roles complete | PASS | 5 logs, complete role matrix |
| TC02 full-load isolation applicability | PASS | TC02 reports `fullLoadSamples=0` and no longer fails the full-load-only gate; full-load log provides 2,026 isolation samples with 0 limit errors |
| Suite independent budget coverage | PASS | full-load and pressure logs all have 0 limit errors |
| Pressure total clip segments suite-wide | PASS | 4 pressure logs contribute `1/1/1/2`, total 5 |
| In-run sign-switch coverage | PASS | 5 positive-to-negative and 4 negative-to-positive switches within valid runs |
| TC01/TC02 role applicability | PASS | Full-load and pressure logs report `NOT_APPLICABLE`; TC02 behavior log is scored |
| TC02 behavior tier minimum | FAIL | TC01=10, TC02=0, TC03=10; suite `behaviorTierMinimum=false` |

## TC02 behavior evidence

- Activity samples: 400
- Pearson correlation H: not computed (`null`)
- Pearson correlation L: not computed (`null`)
- Feature entropy H: `0.011526726999992187`
- Feature entropy L: `0.16123915572144681`
- Direction accuracy: 100%
- Direction error: 0

The behavior tier still fails because the TC02 score is 0: both Pearson correlations are not computed and feature entropy L exceeds the documented 0.15 limit. No hard gate regressed.

## Suite decision

Overall: **FAIL**

Checker correction is objectively verified. The remaining failure is TC02 behavior quality, not suite aggregation or isolation applicability.

## Failure routing

Route the TC02 behavior score failure to **designer** for the next behavior/design iteration. The designer correction in `bottom-creep-v2.7.1 test-caliber` is accepted for its stated scope; no acceptance threshold was widened and no FAIL was converted to PASS.
