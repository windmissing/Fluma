# T833 OP17 T854 qualification and private write sets

Windows x64 Release compiled successfully. The standalone motion and MMS modules reproduce the T854 GLSL and SPIR-V bytes. The same Intel UHD 630 also ran the ledger positive control. Creation, submit and fence returned success for all three. Both original OP66 rounds had zero differences and each owner retired normally.

| Diagnostic role | Actual pipeline creation |
| --- | ---: |
| OP17_MOTION_CONTEXT | 58.272 s |
| OP17_MMS_CONTEXT | 9.243 s |
| OP17_LEDGER_ONLY | 76.850 ms |

The instrumented private roles qualified again under one owner with the original OP66. A cold command recorded OP17 MOTION → MMS → LEDGER with the original push words `[17,0,1,0,0,10]`, one 128-byte WORK segment and a single completion fence. The 13,894,400-byte archive retains all four original buffers and the private segment at each stage. The T857 checker consumed the actual PC words:

| Fixture | Result |
| --- | --- |
| ACTIVE | One active logical count, one ledger trace and no math error |
| ENTRY_NOOP | One NOOP count and one trace at entry. The two consumers advance only the private phase |
| MMS_FAULT | First error `[17,17,0,1,0]`, math mask 4. The active ledger and trace still execute and retain the first error |

Reproduce on the Windows PC with the pinned executables. Use a fresh output directory for each supervisor invocation.

```powershell
python pc/vulkan/qualify_role.py .codex/local-test/t833-t854-qualification.exe OUTPUT_QUAL OP17_MOTION_CONTEXT OP17_MMS_CONTEXT OP17_LEDGER_ONLY
python pc/vulkan/qualify_role.py .codex/local-test/t833-t854-owner.exe OUTPUT_PRIVATE --t854-private-writes
python pc/vulkan/check_t854_capture.py OUTPUT_PRIVATE/T854_PRIVATE_WRITES.receipt.json OUTPUT_PRIVATE/T854_PRIVATE_WRITES.jsonl OUTPUT_PRIVATE/T854_PRIVATE_WRITES.bin
```

The [build identities, raw capture location and verdicts](t854-qualification/result.json) carry the executable and archive hashes. Qualification times are individual observations, with motion close to the 60-second budget. The private cases cover one real plan slot. A runnable monolith on matching inputs and the remaining scope, microstep and adversarial cases are still required for full equivalence. The production mapping remains at SOURCE 16 and QUANTUM 820. The 836/850 candidates and trusted business roots remain gated.
