# P-011 v2.9 blocking proof report

- Date: 2026-10-19 (Asia/Shanghai)
- Route under audit: `bottom-creep-v2.9-central-dune-heavy-interface`
- Scope: `BLOCKED_PROOF_ONLY`; this is not a v2.9 implementation test or formal preflight.
- Verdict: **PASS as blocking proof**. The former candidate remains rejected.

## Evidence

| Item | Value |
|---|---|
| Test file | `test/test_p011_v29_blocking.py` |
| Test file SHA256 | `3B214CD993AFD52A360CCC49D7145A427F4CEA64E6B88891CCE82D612A66A3B0` |
| Command | `python -m unittest -v test.test_p011_v29_blocking` |
| Result | `2/2 OK` |
| Device/HAP used | None |
| Production code changed | None |
| Threshold relaxed | None |

## Angle conflict

For the former target `[(-2.7), (-2.7), (+2.7), (+2.7)]`:

```text
relaxAngle limit at 30 degrees = tan(30) = 0.5773502691896258
target slope at +15 degrees    = tan(15) = 0.2679491924311227
central adjacent deviation     = |2.7 - (-2.7) - 0.2679491924311227|
                              = 5.132050807568877
```

This exceeds the 0.5773502691896258 invariant. Therefore `BLOCKED-V29-ANGLE` is reproduced even under the conservative assumption that only heavy changes.

## Release defect

The isolated release fixture sets the signed cumulative flux `X` exactly equal to the former required flux profile `F`. The flux residual is therefore `0`, but a subsequent avalanche of `0.5` from the high central column to the adjacent low column changes actual `bh` away from the target while `F-X` remains zero:

```text
fluxResidual   = 0.0
shapeResidual  = 2 * 0.5 = 1.0
```

Thus `BLOCKED-V29-RELEASE` is reproduced: `F-X` completion cannot certify the actual heavy shape or authorize budget release.

## Boundary

1. No device was operated and no signed HAP was built.
2. No business code, checker threshold, budget constant, angle constant, or algorithm history was changed.
3. This report does not upgrade v2.9 to `READY_FOR_DEVELOPER`, `READY_FOR_TESTER`, or any physical PASS state.
4. Designer must still resolve the angle-feasible target, actual-shape release semantics, budget accounting, phase lifecycle, and the pending green-ROI measurement interpretation.
