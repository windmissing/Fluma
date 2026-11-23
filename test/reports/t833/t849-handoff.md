# T833 T849 Windows Vulkan qualification and private observations

The Windows x64 Release build passed. The qualified diagnostic executable is `.codex/local-test/t833-t849-qualification.exe`. It reproduces the T849 GLSL and SPIR-V bytes for the four executed narrow roles. The additional reduction-only diagnostic preserves the original Soft128 reducer and has a bounded private readback. Its rebuilt module digest differs from the T849 specimen. That role was unnecessary after the motion arm qualified and was not executed.

| Role | Intel UHD 630, driver 31.0.101.2140 |
| --- | --- |
| OP7 motion | Pipeline created in 59.156 s. Submit, fence and both OP66 masks passed |
| OP7 MMS force | Pipeline created in 9.470 s. Submit, fence and both OP66 masks passed |
| OP17 ledger | Pipeline created in 78.824 ms. Submit, fence and both OP66 masks passed |
| OP17 drive context | Pipeline create ENTER, no return in the 60 s observation window |

The OP17 diagnostic rerun captured the same owner thread's stack bytes and x64 context at the 20 s and 45 s samples. Both observed instruction pointers belong to the loaded Intel `igc64.dll`, at offsets `0xae55e8` and `0x4a17de`. The raw minidumps remain on the PC at the paths and SHA-256 values in the narrow result. Module offsets have been decoded. Symbolic unwind and internal compiler-pass identification remain unavailable. Windows Performance Recorder could not enable its system-profiling policy in this session. [Original role, supervisor and stack receipts](t849-qualification)

The separate `--qualify-t849-writes` diagnostic first qualifies each used module in the same owner and on the same device. It then submits FIELD → MOTION → MMS → OP8 and copies the four fixed buffers into cold staging at each phase. The file is ordered by stage, with `BASE/CANDIDATE/WORK/CTRL` concatenated within each stage. The terminal JSON supplies segment lengths, file SHA, and actual queue and fence results. Each stage keeps the original six push words and the original descriptor object ranges. This is a write-set observation of the isolated modules. The production candidate's cross-dispatch Soft128 context handoff and full equivalent-reference gate remain pending.

The final private executable is `.codex/local-test/t833-t849-private-seeded.exe` (SHA-256 `fd562769aa7a76879143dd4ee0e572a1a0273b9ca11dbd7c853a5a33a068f4e0`). The BOOT-seeded rotating MMS fixture returned Vulkan success. FIELD set the audit mask to 6 and incremented the logical count once. MOTION and MMS left that count unchanged. Only MMS changed CTRL words 642 and 643. OP8 advanced the count to 2. The first-error tuple stayed zero, and BASE, CANDIDATE and WORK stayed byte-identical across the four calls. Two earlier observations with incomplete diagnostic seeds recorded first-error tuples `[7,35]` and `[7,26]`; both were preserved through MOTION, MMS and OP8. Each 5,343,360-byte archive is pinned by SHA in [the private observations](t849-qualification/private-write-result.json).

Original logical SOURCE 16 and QUANTUM 820 and the production module mapping stay frozen. The OP17 drive context lacks qualification, and the 836/850 candidates and production roots were not opened.
