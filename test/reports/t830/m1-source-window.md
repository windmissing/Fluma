# T830 Windows Vulkan dry SOURCE and shader Present

The Windows x64 Release build passed. The Intel UHD 630 diagnostic admitted a dry SOURCE of 131072 Q20 in one event. The same-generation Vulkan submit and fence returned VK_SUCCESS. The original M1 owner accepted the 256-word GPU Completion before publication. The fence arrived 74.598 ms after permit grant, within the frozen 250 ms deadline. The Win32 diagnostic pulse timer supplied six observed pulses.

The acquired publication lease fed the fragment shader directly on the same device. The 784×461 window's acquired image passed shader readback at the diagnostic particle position. Queue Present and per-image results were VK_SUCCESS, and the lease retired normally. The frame's accepted root matches the Completion nonce. The unchanged BOOT entry point also passed on the same executable.

| Evidence | File |
| --- | --- |
| Dry SOURCE, completion words, and window Present | [JSONL](m1-source-window.jsonl) · [process receipt](m1-source-window-receipt.json) |
| BOOT smoke regression | [JSONL](boot-window-regression.jsonl) · [process receipt](boot-window-regression-receipt.json) |
| Static SPIR-V source units and reflected layouts | [manifest](m1-bounded-shaders.json) |

Generate the M1 sources with `test/gpu_resident/prepare_t280_commit.py`, then run `pc/vulkan/check_m1_spirv.py --split-quantum` with the canonical assembled directory and a SPIR-V validation enabled glslang. Build the Windows x64 Release executable with the `pc/vulkan` CMake entry point. With the pinned SOURCE module, run `hello_world_pc.exe --once --present-m1-source <full_source.spv>`. The captured PE SHA-256 is in the receipt.

Scope is a fixed dry diagnostic input with GPU publication and pixel proof. The interactive pointer/hold and production pulse route, M1 QUANTUM execution, complete product rendering, and native accepted-root archive remain open. The two QUANTUM compilation units preserve the pass partition and uniform offsets under static validation. Dispatch and replay equivalence await the PC transport.
