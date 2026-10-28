# T236 foundation slice — M0 is not implemented

This directory holds generated T233 word indexes, explicit U32/U64/U128/Q20
primitives, a cold high precision angle table, resource planning, and native
control-plane tests. It does **not** contain the source/motion/DDA/LAND/bed/audit/
replay/publication step shaders or a compute-to-XComponent renderer integration.
`GetFoundationInfo()` therefore returns `implementedFeatures=0`,
`stepShaderImplemented=false`, `connectedToXComponent=false`.

C++ arithmetic is a standalone primitive implementation for developer differential
testing and future cold/control uses. No CPU simulation backend is called and no
old pair or CPU route is replaced. The GLSL file is a library, not a dispatched
product shader. Building this static library proves native compilation only.

`CompletionPolicy` has zero defaults and requires explicit wire codes; the host
fixture's codes are **test policy**, not newly frozen V1 wire. `RootOwner` tests
consume mock FenceState/completions, not actual GL fences or GPU state. Resource
rows describe the T233 payload/planned padding, not successful GL allocations or
RSS. The K192 sort storage and event/outcome wire questions are filed on T236 and
in `test/reports/t236/handoff-questions.json` for architect clarification.

Reproduce offline:

- `python test/gpu_resident/generate_t236_contract.py --check`
- `python test/gpu_resident/check_t236_spec.py`
- `test\gpu_resident\build_t236_host.cmd`
- `python test/gpu_resident/check_t236_integer.py`

Do not enable M0, claim GR01–GR09 product PASS, or silently fill missing effects
with CPU computation from this foundation's unit results. M1–M3 remain unsupported.
