# T648 default DM2 and shared T647 proof ABI

Developer compile artifacts only. No product execution, test run, signing, install, device, G11 owner or RX edits.

Regenerate from the formal tables and byte-bound frozen math reference:

```
python tools/g9dm/generate_assets.py --out .codex/local-test/t648-assets --canonical
node tools/g9dm/prepare_build.cjs .codex/local-test/t648-build-fresh
```

Use the installed hvigorw with `--mode module -p product=default -p module=entry@default assembleHap --no-daemon` in that isolated directory. The unsigned HAP is a compile container. `libgrs_dm_t647.so` links the actual default transport/controller, shared semantic core and both composite parent gates. It does not register a G11 page or substitute the downstream product bridge.

`test/t555_generate_dm_schema.cjs` now delegates to the same generator. T549 frozen sources remain unchanged. Native cache ABI is `DM2_T647_V1|<implementationSHA>`. The parameter capsule is exactly the formal Config64, hash over all 256 LE bytes, with DM8 at WORK Config+112..119. The legacy, unregistered NAPI include exports a revisioned 65-word envelope: Config64 then the independent fullTrialTrace flag 0/1. It cannot accept the old 24-word configuration. This include gets a syntax-only check, separate from the real default library build.

The default `Transport::Create` constructs the canonical S40 template, checks its frozen SHA, stamps actual scope/parameter identity, recalculates actual seed SHA, uploads all five banks and requests a GPU read-only initial audit. An explicitly supplied template must match, never silently replace physics. Failed/old identities require a cold new controller/run; no hot migration from the incorrect DM construction.

## T639 typed seam (code available now)

Headers:

- `entry/src/main/cpp/gpu_resident/acceptance_t647/contract.h`
- `entry/src/main/cpp/gpu_resident/g9spf/completion_gate.h`
- `entry/src/main/cpp/gpu_resident/g9dm/completion_gate.h`

Link `acceptance_t647/contract.cpp` plus both completion_gate.cpp files. Do not link the default DM controller into G11 as its joint bank owner.

Only the actual `grs::g11vis::VisualTransport` friend can seal its active program and mapped observation. Build `ProgramFacts` from its real compiled GL program/current EGL context/owner thread, immutable physical config/seed/layout, actual compiled shader, composite layout/pipeline and actual visual seed/config/recipe. V is 2048 or 8192. ParentKind is explicit 1=R1, 2=DM. Never obtain these identities from the completion tail.

Inside that real owner:

```cpp
const auto active = grs::accept647::ActiveProgramIdentity::ForComposite(facts, parentKind);
auto expected = grs::accept647::ExpectedForActivePipeline(
    active, grs::accept647::PermitView::Of(immutableParentPermit, rootBeginNs), parentKind);
if (!expected) return /* rejected */;
// Claim/upload before dispatch: expected->Serialize(caller224, 224), expected->Hash().
// The view borrows the immutable owner permit; keep active/permit alive and frozen.
// After actual dispatches/barriers, signaled fence, raw GPU mapping and live guard:
const grs::accept647::NativeMappedObservation observation(active, mappedTail256, actualEvidence);
// R1:
auto r1 = grs::g9spf::CompletionGate::AcceptedComposite(*expected, observation);
// DM (choose the matching family only):
auto dm = grs::g9dm::CompletionGate::AcceptedComposite(*expected, observation);
// All-rejected Q uses the corresponding BlockedComposite; hard 19..29 never becomes BLOCKED.
```

`ValidatedCompositeParentR1` / `ValidatedCompositeParentDM` and their blocked counterparts are distinct private-construction types exposing only `Words()`, `ExpectedHash()`, `Producer()`. No cast to a default proof, count rewrite, or arbitrary JS evidence flag exists. The pure semantic core compares all relevant T524 fields, exact row/mask, bounded cursors/trial-prefix/first choice, errors/counters/fence/time/scope/hashes and the GPU-written extension. CPU never supplements audit bits. For SOURCE duplicate/BLOCKED, actual visual proposed counters remain base while the immutable candidate Expected hash stays unchanged.

T639 still owns the real GPU extension upload/writer, material and visual evidence, joint guard/physical+visual bank commit, publication, scene and product bridge. A parent proof alone never authorizes its visual commit. The five-family table is shared: frozen R1 default remains its existing 12/2024 API; RX keeps its independent T644 native factory; this module provides the repaired default DM and the two new G11 trusted parent gates.

## Cold reader

`read_cold.py archive.bin` validates the captured DM2 prefix/tail/config/input/segments and real scope certificates. It creates no live native proof or product PASS. Full controls128 are captured at prefix word1408, the T647 eight controls at words1528..1535. This does not grow the existing 8192-word prefix. K512 root/FULL/two FULL bytes: 48050176/188059648/376119296. Dictionary and build identities are generated beside this README. Cold CaptureSpan still uses the actual mapped GPU spans and two-slot broker; there is no CPU reconstruction of event/physics evidence.
