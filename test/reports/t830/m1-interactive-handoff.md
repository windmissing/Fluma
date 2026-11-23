# T830 Windows M1 first interactive root handoff

The Windows x64 Release entry point is `hello_world_pc.exe --m1-window SOURCE_SPV CANDIDATE_SPV COMMIT_SPV`. Add `--m1-cold FILE` for an explicit cold capture and `--once` when the verifier should close after the first Present. Use the three module digests in `m1-bounded-shaders.json`, generated with `pc/vulkan/check_m1_spirv.py --split-quantum`. Compilation and module reflection cover this handoff. PC runtime verification for the new entry point is pending.

T840 can rerun the original diagnostic using `.codex/local-test/t830-window-final.exe --once --present-m1-source .codex/local-test/t830-m1-20261123/full_source.spv`. The preserved original executable and all three regenerated SPIR-V files match the SHA-256 values in the original receipts. The interactive executable path and its digest are in [the new build receipt](m1-interactive-build.json).

| Boundary | Producer and evidence |
| --- | --- |
| Pointer and SOURCE permit | Wait for `M1_POINTER_READY`, then start a new in-box Down. Two ordered QPC pulses form the first offer at the existing 8 units/s candidate rate. Move updates the in-box position. Up, cancellation, focus or capture loss, and geometry change revoke the old hold. Keep the raw `WIN32_POINTER` rows and SOURCE receipt. |
| QUANTUM | Uses the accepted SOURCE bank, original default dry parameters, angle table, GPU source ledger and the same owner. Vulkan and GLES share the original pass plan, including REPLAY and tail order. A new frame starts a new event stream. A same-frame quantum carries the existing prefix. Keep the QUANTUM completion, raw fence result and nonce. |
| Scene and Present | The dry scene consumes the accepted QUANTUM publication from one lease. The shader composes background, bed, active sand and foreground with a single sRGB conversion. Keep lease generation, image index, GPU fence, Queue Present, per-image result and retirement. The older fixed-source probe retains its green-pixel oracle. |
| Cold original | Optional capture copies the accepted state and bounded original event ranges after acceptance to a host-visible Vulkan staging buffer, then writes an immutable file. It includes the SOURCE and QUANTUM originals. The physical render path maps only the fixed-size tail plus the WSI diagnostic pixel. |

## Cold file

`M1PCOLD1` is an eight-byte PC envelope tag. It is followed by eight little-endian uint32 section lengths, the 32-byte payload SHA-256 and the concatenated payload. Section order is QUANTUM Completion256, QUANTUM bank, QUANTUM events, QUANTUM publication, SOURCE bank, SOURCE events, SOURCE Completion256 and SOURCE command32. The payload retains the original little-endian GPU words. The Windows identity and presentation receipts are in the process JSONL.

This entry point covers the first pointer-sourced dry SOURCE and first QUANTUM in one window generation. Repeated admissions, steady 120 Hz scheduling and presentation, the water/optics scene, and independent positive and negative PC receipts remain open. The cold envelope requires a reader adapter for cross-platform validation.
