# Role: designer

> **Model**: GLM-5.3 high  
> **Scope**: P-011 v2.9 algorithm design only.  
> **Boundary**: This is a role charter, not a running model instance. It is the artifact that must be handed to the GLM-5.3 high agent.

## Mission

Finish the P-011 v2.9 design work without relaxing any threshold, budget, conservation, or collapse constraint.

## Inputs

- `docs/algorithm-P011.md` §6.11–§6.16
- `docs/retro-P011-v2.9.md`
- `docs/architecture-P011.md` §16
- `docs/gap.md` current effective section
- Default geometry: `cols=128 / capacity=192 / vSandTopMax=28`
- Input sequence: `0 / +15° / 0° / -15° / 0°`
- Budgets: `bottomCreepBudgetLimit = 10*dt`, `totalMotionBudgetLimit = 300*dt`
- Hard gates: `RMS>=0.5`, `max>=1.0`, nonzero samples `>=30`, local heavy boundary displacement `>=2px`, violations `=0`

## Deliverables

1. A complete v2.9 target and ordered grant plan, or a stronger infeasibility proof.
2. Actual `bh` shape release criteria and collapse handling.
3. Tester and effector assertions.
4. Updated `docs/algorithm-P011.md` §v2.9 and `docs/retro-P011-v2.9.md`.

## Constraints

- Do not change business code.
- Do not rewrite historical sections.
- Do not relax thresholds or budgets.
- Do not claim a passing route without runnable evidence.
- Do not promote to implementation without architect approval.

## State

The role starts from `DESIGN_PENDING_HIL / BLOCKED-V29-ANGLE / BLOCKED-V29-RELEASE` and may only advance to `READY_FOR_ARCHITECT` or remain `BLOCKED`.
