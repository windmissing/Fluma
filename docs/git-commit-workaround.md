# git commit method note (2026-10-19)

## Conclusion

No standalone git.exe on PATH (verified), but the project has a .git repo; historical commits (latest 130a52c) were made via external tool - DevEco Studio built-in Git. Agent sessions cannot run git directly; all commits must go through DevEco Studio.

## Uncommitted work (everything after v2.9)

Reviewed and ready to commit:

- entry/src/main/ets/physics/: P011NaturalObservation/Store/Worker (observation contract, ledger schema v3, per-edge transition telemetry, P011RelaxHysteresisObservation top-level record), P011TiltWant, P011TiltPlanningPass, WaterSandField, SandCanvas (zero-intrusion hooks, sliding state, planning pass skeleton)
- entry/build-profile.json5: observation worker registration
- tools/run_p011_v210_natural_observation.ps1: 7-phase schedule, mandatory params, calibration/holdout separation
- test/: p011 checkers and 37 unit tests
- docs/: all v2.9-v2.14 documents (algorithm, architecture, retro, observation, test-report, priority, gap)

## How to commit

Use DevEco Studio Git panel (Commit/Push), or install portable git for agent-side commits. Reviewer-verified: 37 unit tests OK, build passes, safe to commit.
