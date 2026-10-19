# P-011 formal rerun - bottom-creep-v2.7.2 - 2026-10-18

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- Date: 2026-10-18 (Asia/Shanghai)
- Signed HAP: `entry/build/default/outputs/default/entry-default-p011-20261018-v272-signed.hap`
- Signed HAP SHA256: `4A4763C7D6AD6C0ED9298AC6411A3BF7AEE651AE26A6C03764C9ACD245F67123`
- Install result: `install bundle successfully.`
- Suite command: `powershell -ExecutionPolicy Bypass -File .\test\p011_tc02_formal_suite.ps1 -Mode All`
- Suite JSON: `test/artifacts/p011-formal-prep-20261016/p011-suite.json`
- Suite Markdown: `test/artifacts/p011-formal-prep-20261016/p011-suite.md`
- TC02 schedule validation: PASS, 531/531 events

## Identity and completeness

- Route observed: `bottom-creep-v2.7.2`
- Parse errors: 0
- Timestamp-order errors: 0
- Route mismatch: false
- Identity missing/mismatch: 0/0
- Sequence/frameId/substepId regressions: 0/0/0
- Run without reset: 0

## Hard gates

| Metric | Result | Evidence |
|---|---|---|
| Parse/order/route completeness | PASS | 0 errors/mismatch |
| Reset/identity observability | PASS | Each run had a reset marker and valid runId |
| Sequence/frameId/substepId regression | PASS | 0 regressions |
| Non-finite / out-of-range / model error / crash / ANR | PASS | 0 in all logs |
| Full-load and pressure budget coverage | PASS | 0 limit errors |
| Pressure total clip segments suite-wide | PASS | 4 pressure logs, 4 total clip segments (`1/1/1/1`), meets suite-wide minimum 3 |
| In-run sign-switch coverage | PASS | 5 positive-to-negative and 4 negative-to-positive switches |
| TC02 full-load isolation applicability | PASS | Suite-level isolation coverage passes; no per-role applicability conflict |

## Tiered behavior result

| Log role | TC01 | TC02 | TC03 | Minimum rule |
|---|---:|---:|---:|---|
| TC02 behavior | 10 | 0 | 10 | FAIL: minimum < 6 |

TC02 behavior evidence:

- Activity samples: 400
- Pearson correlation H: not computed (`null`)
- Pearson correlation L: not computed (`null`)
- Feature entropy H: `0.023496788213169695`
- Feature entropy L: `0.20549138771581632`
- Lag H / L: `0ms` / `0ms`
- Direction accuracy: 100%
- Direction error: 0

The remaining failure is the TC02 tiered behavior score. It is not a hard-gate failure and not a suite aggregation failure.

## Suite decision

Overall: **FAIL**

`allLogsPass=true`, `behaviorTierMinimum=false`. The hard gates and suite aggregation corrections are stable. The sole remaining issue is TC02 behavior quality under the frozen thresholds.

## Failure routing

Route the TC02 behavior score failure to **designer** for the next bottom-creep route iteration. No threshold was widened and no FAIL was converted to PASS.
