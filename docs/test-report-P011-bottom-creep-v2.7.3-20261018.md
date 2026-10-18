# P-011 formal rerun - bottom-creep-v2.7.3 - 2026-10-18

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- Date: 2026-10-18 (Asia/Shanghai)
- Signed HAP: `entry/build/default/outputs/default/entry-default-p011-20261018-v273-signed.hap`
- Signed HAP SHA256: `190EEF4BA5AF3C50AC72A0E19D776FE557CD1150CA254FD0604EE2EDA46AC6B6`
- Install result: `install bundle successfully.`
- TC02 schedule validation: PASS, 531/531 events
- Suite JSON: `test/artifacts/p011-formal-prep-20261016/p011-suite.optimized.json`
- Suite Markdown: `test/artifacts/p011-formal-prep-20261016/p011-suite.optimized.md`

The first v2.7.3 suite process hung during aggregation because checker had quadratic lookup paths. The developer corrected those lookups to sorted/indexed/dictionary form without changing any criterion. After optimization, all five logs were passed to one checker invocation and the suite completed in `15.037s`.

## Identity and completeness

- Route observed: `bottom-creep-v2.7.3`
- Parse errors: 0
- Timestamp-order errors: 0
- Route mismatch: false
- Identity missing/mismatch: 0/0
- Sequence/frameId/substepId regressions: 0/0/0
- Run without reset: 0

## Hard gates and suite coverage

| Metric | Result | Evidence |
|---|---|---|
| Parse/order/route completeness | PASS | 0 errors/mismatch |
| Reset/identity observability | PASS | Each run had a reset marker and valid runId |
| Sequence/frameId/substepId regression | PASS | 0 regressions |
| Non-finite / out-of-range / model error / crash / ANR | PASS | 0 in all logs |
| Independent budget coverage | PASS | `independentBudgetCoverage=true` |
| Pressure total clip segments suite-wide | PASS | 4 pressure logs, 5 total clip segments (`1/1/2/1`) |
| In-run sign-switch coverage | PASS | 5 positive-to-negative and 4 negative-to-positive switches |
| TC02 full-load isolation applicability | PASS | Suite-level coverage passes; no per-role applicability conflict |

## Tiered behavior result

| Log role | TC01 | TC02 | TC03 | Minimum rule |
|---|---:|---:|---:|---|
| TC02 behavior | 10 | 8 | 10 | PASS: minimum >= 6 |

TC02 behavior evidence:

- Activity samples: 12,945
- Pearson correlation H: `0.8653415370268619`
- Pearson correlation L: `0.8653415370268619`
- Lag H / L: `0ms` / `0ms`
- Feature entropy H: `0.025611293192860707`
- Feature entropy L: `0.025611293192860707`
- Direction accuracy: 100%
- Direction error: 0
- Bottom-direction samples/errors: 12,945 / 0

## Suite decision

Overall: **PASS**

All documented hard gates, suite aggregation checks, and tiered minimum requirements passed. No threshold was widened and no FAIL was converted to PASS.
