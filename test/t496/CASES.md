# T496 offline cases

No device. Build the frozen single-source class-layout gate and main fixture with build.cmd. build_shadow.cmd checks bounded CPU-observed shader/object/buffer/uniform state. build_budget.cmd NEW_ATTEMPT checks persistent evidence-budget refusal without GL. check_actions.py --selftest covers complete action input and missing/extra/order/seq/payload/layout rejection. run_bounded.py runs one explicitly named process under a shared 420s total limit.

check_capture.py REPORT checks actual busy reservation, safe-point arm, full identity/hash-bound pages and FROZEN/BEGIN/END. compare_roots.py OFF_REPORT ON_REPORT requires complete nonempty actual mapped completion256 and accepted ledger/token sequences; never accepts a prefix or empty equality.

Current full D1 is blocked by 120s business / 32MiB full-call evidence limits. Narrow C0/C1/C2 and D2 success is not whole-card product PASS. See docs/test-report-T496-t657-offline-v1.md and t657-v4-blocker.json.
