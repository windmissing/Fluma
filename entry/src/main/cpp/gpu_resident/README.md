# T236 isolated GPU-resident M0 implementation slice

T241 product wire and padded-sort corrections are consumed. `m0_source.comp`
expands the generated layout, integer primitives, source-root audit/replay and
M0 motion/DDA/LAND/three-color BED code into one ES310 program. Physical writes
execute in this program; no CPU simulation backend or fallback is called.

`SourceGpuTransport` cold-allocates the real three state banks, double event and
publication arenas, dispatches 32/34 source or 59/68 quantum passes, and polls a
real tail fence without waiting. Only a sealed validated 1024-byte completion
can move the current selector. Cancellation retires late work without mapping;
fatal/untrusted completions freeze further submission until explicit retirement.
Cold diagnostic full-state maps require `FreezeDiagnostics` and have separate
byte accounting. Uniform payload and command bytes are counted in root upload
metrics. All calls belong to one native thread and one EGL display/context/draw
surface. `Release` returns BUSY while render fences are pending; retry from the
owner loop, never wait on UI.

`ResidentRun` is the native asynchronous scheduling interface: FIFO16, at most
one source plus four independent quantum roots per active tick, dt0 no writes,
F0 control carry only, no catch-up. `Pump` does one submit or nonblocking poll.
Accepted source/quantum roots survive a later failure; the tick receipt records
unexecuted quanta. Pause clears queued gestures and retires late GPU work.

`ResidentScene` consumes the accepted publication using VBO attributes and a GPU
PBO texture upload (zero vertex SSBO requirement, no CPU pool round trip). All
background/bed/particles/foreground use a full publication identity/token and
shader pipeline. Only complete draw plus successful caller swap commits the
last-good token. Failure is FAULT; no Canvas. Render-release fences protect the
two publication slots. Display presentation is always unverified here.

This slice is not enabled in the app: implementedFeatures=0,
stepShaderImplemented=true, connectedToXComponent=false. The window owner/NAPI/
page integration, final-ID exhaustion protocol, fault-cut matrix and independent
product validation are still pending. M1 water/J/M2/M3 are unsupported. Do not
turn PC SwiftShader checks into hardware GPU, color, device, performance, or
complete GR01¨CGR09 PASS claims. The software driver is an explicit developer
harness only, never an app fallback. Historical foundation evidence is retained;
new evidence is in `test/reports/t236/resume/`.

Reproduce offline with the generator `--check`, `check_t236_spec.py`,
`build_t236_host.cmd`, `check_t236_integer.py --output-dir test/reports/t236/resume`,
`build_t236_shaders.py`, `prepare_t236_host_gl.py`, then
`build_t236_source_host.cmd --swiftshader` (and `--k512`). The harness uses the
installed local ANGLE/SwiftShader DLLs and does not install a runtime. Build the
Harmony PackageHap before delivery. Do not enable the production route solely
from these checks.
