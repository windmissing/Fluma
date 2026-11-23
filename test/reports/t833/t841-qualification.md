# T833 Windows Vulkan T841 role qualification

The Windows x64 Release executable is preserved at `.codex/local-test/t833-t841-qualification.exe` (SHA-256 `ad40c7ce3ba07ac9a782b0a2b2acf5b34ca233f6d778142192bc9fd7dc950b01`). The eight diagnostic modules reproduce the T841 source and SPIR-V digests. The original opcode-to-role table remains frozen. A module in this diagnostic registry has no production dispatch authority.

Run the isolated role qualification with the existing supervisor.

```powershell
python pc/vulkan/qualify_role.py .codex/local-test/t833-t841-qualification.exe OUTPUT OP7_FIELD_ONLY OP7_DRIVE_ONLY OP15 OP17 OP6 OP8 OP12_14 OP16
```

Each arm has its own process, 60-second pipeline observation window, and 120-second process wall limit. Qualification requires real pipeline creation, submit, fence and both sets of OP66 bits. A terminated process has no normal owner retirement. All QPC timestamps and probes are in the [role receipts](t841-qualification).

| Role | Hardware result |
| --- | --- |
| OP7 field | Created in 307.067 ms, OP66 `[0,0]` |
| OP7 drive | Pipeline ENTER only. Supervisor observation 60.003 s |
| OP15 | Created in 330.819 ms, OP66 `[0,0]` |
| OP17 | Pipeline ENTER only. Supervisor observation 60.100 s |
| OP6 | Created in 35.347 ms, OP66 `[0,0]` |
| OP8 | Created in 52.138 ms, OP66 `[0,0]` |
| OP12–14 | Created in 58.748 ms, OP66 `[0,0]` |
| OP16 | Created in 102.199 ms, OP66 `[0,0]` |

Both stalled modules returned from shader-module creation, then entered `vkCreateComputePipelines` with flags 0. The supervisor collected QPC, process and owner CPU samples and terminated only its diagnostic children. No pipeline result, GPU submit, terminal OP66, or normal owner retirement exists for those two arms. The available Windows tools had no user-mode debugger for owner stack capture. Further attribution concerns the reachable `drive` / Soft128 path and the separate `entryLedger` work in OP17. The observations leave the exact driver compiler stage unresolved.

The OP7 split therefore has no paired write-set evidence or business promotion. SOURCE 16 and QUANTUM 820 remain the frozen logical schedules. The proposed 821 physical dispatch candidate and real business roots were not run.
