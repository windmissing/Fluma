# T853 PC M1 SOURCE descriptor allocation

Both SOURCE bank routes use a common count of 16 descriptor sets. The pool reserves 64 storage and 16 uniform descriptors. A successful Vulkan allocation and all 16 non-null handles are checked before the first descriptor update. Compile-time controls cover eight sets, one missing handle, allocator failure and a revoked attempt.

The Windows x64 Release executable is `.codex/local-test/t853-final.exe`. Use the frozen modules from `.codex/local-test/t830-m1-20261123`. The executable, three modules and embedded angle table are pinned in [the build and initialization receipt](build.json).

```powershell
$spv = '.codex/local-test/t830-m1-20261123'
$pc = '.codex/local-test/t853-final.exe'
& $pc --probe-m1-source "$spv/full_source.spv"
& $pc --once --m1-window "$spv/full_source.spv" "$spv/quantum_candidate.spv" "$spv/quantum_commit.spv"
```

The fixed SOURCE smoke admitted 131,072 Q20. Interactive initialization emitted the successful angle-table fence, allocation return with 16 non-null sets, completion of all 16 updates and a single pointer readiness record in order. The harness then closed its own window, so the terminal outcome is `ATTEMPT_REVOKED`. Closing a separate child during BOOT produced `ATTEMPT_REVOKED` with zero readiness and zero completed SOURCE updates. The allocator-failure and short-count controls are compile-time checks of the production guard.

T855 independently exercises real Down and held input, QUANTUM and REPLAY, same-root pixels and Present, release, retirement and optional cold capture for T848.
