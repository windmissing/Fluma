"""T613 architecture inventory. Reads bytes/symbols; never builds or runs a solver.

The JSON is a working-tree snapshot, not a deployed-package receipt or test PASS.
Do not run legacy generators from this inventory: several mutate product trees.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
GPU = 'entry/src/main/cpp/gpu_resident/'


def source(path, symbol):
    return (path, symbol)


def g(path, symbol):
    return source(GPU + path, symbol)


# IDs are stable engineering inventory keys, not new execution cards.
ASSETS = [
    ('I01', 'build', 'PATTERN_ONLY', 'explicit targets and assembled shader identity', [
        source('entry/src/main/cpp/CMakeLists.txt', 'add_library(grsm1 SHARED'),
        g('CMakeLists.txt', 'add_library(grs_foundation STATIC'),
        g('m3b/kernels.cmake', 'm3b_root_sha.h'),
        g('g9spf/kernels.cmake', 'g9spf/transport.cpp')]),
    ('I02', 'build', 'PATTERN_ONLY', 'isolated generators are different binding graphs', [
        source('test/t517/generate.cjs', '/m3b/kernels.cmake'),
        source('test/t529/generate.cjs', '/g9spf/kernels.cmake'),
        source('test/t537/generate.cjs', "['m1','m2']"),
        source('test/t599/generate.cjs', 't576_generate_pipeline.cjs')]),
    ('I03', 'common', 'REUSE_CONDITIONAL', 'compute program persistence and real compile receipts', [
        g('program_cache/program_factory.h', 'inline bool Build('),
        g('program_cache/program_factory.h', 'inline std::string Environment()'),
        g('program_cache/artifact_store.h', 'bool Configure('),
        g('program_cache/artifact_store.h', 'kTotalLimit = 128u * 1024 * 1024')]),
    ('I04', 'common', 'REUSE_CONDITIONAL', 'graphics cache ABI and reflection', [
        g('program_cache/graphics_program_factory.h', 'BuildGraphics('),
        g('uniform_locations.h', 'struct UniformLocations')]),
    ('I05', 'common', 'PATTERN_ONLY', 'strict typed decode with route-specific registration', [
        g('napi_words_contract.h', 'ReadWordsResult<N> ReadWords('),
        g('napi_words_contract.h', 'M3A = 0, M3B = 1, G9 = 2'),
        g('napi_words_contract.cpp', 'bool SnapshotJson('),
        g('bridge_state.h', 'OfferLocked(')]),
    ('I06', 'common', 'PATTERN_ONLY', 'XComponent single EGL owner and revoke-before-retire', [
        g('app_bridge.cpp', 'struct App : BridgeState'),
        g('app_bridge.cpp', 'eglSwapBuffers'),
        g('bridge_state.h', 'InvalidateLocked()'),
        source('entry/src/main/ets/pages/GpuResidentM1.ets', 'class M1DemoFrame')]),
    ('I07', 'common', 'REUSE_CONDITIONAL', 'bounded logical resource ledger, not driver RSS', [
        g('resource_ledger.h', 'class Ledger'),
        g('resource_ledger.h', 'DRIVER_NOT_MEASURED'),
        g('resource_gl.h', 'inline void Bind('),
        g('resource_gl.h', '#define glBufferData')]),
    ('I08', 'common', 'PATTERN_ONLY', 'separate compute scene and present qualification', [
        g('g9spf/stage_capability.h', 'struct StageCapabilityV2'),
        g('g9spf/stage_capability.h', 'QueryCapabilityI('),
        g('m2/capability.h', 'ProbeCurrent(')]),
    ('I09', 'common', 'PATTERN_ONLY', 'root completion and bank/lease atomicity', [
        g('control.h', 'class RootOwner'),
        g('m1/full_transport.h', 'ContinueSubmit()'),
        g('m3b/controller.h', 'AcquireV5('),
        g('g9dm/controller.h', 'AcquireScene()'),
        g('g9dm/transport.h', 'Commit(const Permit&')]),
    ('I10', 'common', 'PATTERN_ONLY', 'monotonic quotas, not a new fluid time discretization', [
        g('m1/progress_clock_v2.h', 'GapNs=33333333,JobNs=250000000'),
        g('g9dm/progress_clock_v3.h', 'GapNs=33333333,RootNs=250000000'),
        g('g9dm/progress_clock_v3.h', 'Balanced()const')]),
    ('I11', 'common', 'REUSE_CONDITIONAL', 'one process-wide cold credit DSO', [
        g('cold_slot_broker.h', 'class DetachedBlock'),
        g('cold_slot_broker.cpp', 'kProcessBytes=524375552'),
        g('cold_slot_broker.cpp', 'GrsColdReserve('),
        g('cold_slot_broker.cmake', 'add_library(grs_cold_slots SHARED')]),
    ('I12', 'M3', 'IN_FLIGHT_PARTIAL', 'V5 retained BASE/cold implementation and ongoing closure', [
        g('m3b/cold_capture.cpp', 'CaptureColdV5'),
        g('m3b/transport.cpp', 'TransportV5::'),
        g('m3b/cold_contract.h', 'namespace'),
        source('docs/developer-T596-progress.md', '尚未完成')]),
    ('I13', 'common', 'PATTERN_ONLY', 'cold collector paging and source-specific reader', [
        source('entry/src/main/ets/physics/GpuBoundedCold.ets', 'export'),
        source('entry/src/main/ets/physics/GpuM3Cold.ets', 'export'),
        source('entry/src/main/ets/physics/GpuG9Cold.ets', 'export'),
        g('m2/cold_reader.py', 'def ')]),
    ('I14', 'scene', 'REUSE_CONDITIONAL', 'two GPU full-frame targets and last-good commit', [
        source('entry/src/main/cpp/p013renderer/fullframe_targets.h', 'class FullFrameTargets'),
        source('entry/src/main/cpp/p013renderer/fullframe_targets.cpp', 'FullFrameTargets::Ensure'),
        g('g9dm/scene.cpp', 'controller.AcquireScene()'),
        g('g9dm/scene.cpp', 'controller.ReleaseScene(')]),
    ('I15', 'M1', 'LEGACY_NUMERICS', 'height transport/packet rules do not supply a field solver', [
        g('m1/water_edge.comp', 'uniform uint parity,kappaW;'),
        g('m1/quantum_motion.glsl', 'void resolvePacket('),
        g('m1/layout_generated.h', 'revision=2,features=3'),
        g('m1/full_transport.h', 'class FullM1Transport'),
        g('m1/scene.frag', 'void main')]),
    ('I16', 'M1', 'SCAFFOLD_DISTINCT_FROM_FULL', 'old source-core profile explicitly has no quantum implementation', [
        g('m1/profile.h', 'ProtocolReadyShaderNotImplemented'),
        g('m1/profile.cpp', 'ConfigureCode::ProtocolReadyShaderNotImplemented')]),
    ('I17', 'M2', 'LEGACY_NUMERICS', 'SOURCE128 merging and legacy stock geometry', [
        g('m2/source_execute.glsl', 'void '),
        g('m2/merge_primitives.glsl', 'uint '),
        g('m2/root_gate.h', 'class RootGate'),
        g('m2/buffer_plan.h', 'ResidentBufferBytes('),
        g('m2/layout_generated.h', 'namespace')]),
    ('I18', 'M3', 'LEGACY_NUMERICS', 'V4/V5 sand lineage and lifting', [
        g('m3a/source_v4.comp', 'void main'),
        g('m3b/lift_writer.glsl', 'void '),
        g('m3b/lineage_schema.h', 'struct LineageProfile'),
        g('m3b/controller.h', 'class ControllerV5')]),
    ('I19', 'G9', 'LEGACY_NUMERICS', 'SPF R1 and DM v2 are sand physics', [
        g('g9spf/schema_generated.h', 'namespace'),
        g('g9spf/pipeline.comp', 'void main'),
        g('g9dm/schema_generated.h', 'namespace'),
        g('g9dm/pipeline.comp', 'void main'),
        g('g9dm/research_config.h', 'namespace')]),
    ('I20', 'G9', 'IN_FLIGHT_PARTIAL', 'clock proof and actual touch/presented mapping still separate', [
        g('g9spf/clock_producer.h', 'CLOCK_MONOTONIC'),
        g('g9spf/touch_adapter.h', 'TouchEvidence256'),
        source('docs/developer-T599-progress.md', '尚未完成')]),
    ('I21', 'G11', 'LEGACY_VISUAL', 'accepted sand material-plan markers are not fluid velocity', [
        g('g11vis/visual_transport.h', 'class VisualTransport'),
        g('g11vis/visual_stage.glsl', 'g11PutMarker('),
        g('g11vis/generated_contract.h', 'kConfigRevision = 1u'),
        g('g11vis/scene.cpp', 'CompositeScene::Draw('),
        source('test/t566_generate_composite.cjs', 'g9dm/pipeline.comp')]),
    ('I22', 'G11', 'METADATA_ONLY', 'cold PAGE receipt has no actual raw word payload', [
        g('g11vis/cold.cpp', 'Cold::Read('),
        g('g11vis/cold.cpp', 'page.Number(available < kPageWords'),
        g('g11vis/kernels.cmake', '../../../test/t566_generate_composite.cjs')]),
    ('I23', 'G12', 'PARTIAL_NOT_IMPLEMENTED', 'old porous coupling and explicit wet writer faults', [
        g('g12water/contract_generated.h', 'G12_POROUS_SPF_Q20_V1'),
        g('g12water/water_core.glsl', 'g12JFlight()'),
        source('test/t576_generate_pipeline.cjs', 'bool g12NotImplemented('),
        source('test/t576_generate_pipeline.cjs', 'bool g12SourceWetGate(){return true;}'),
        g('g12water/transport.h', 'class Transport')]),
    ('I24', 'G12', 'METADATA_ONLY', 'cold PAGE has no immutable snapshot or raw payload', [
        g('g12water/cold.cpp', 'Cold::Read('),
        g('g12water/cold.h', 'struct Slot'),
        g('g12water/scene.cpp', 'G12_POROUS_SCENE_V1|'),
        g('g12water/kernels.cmake', '../../../test/t576_generate_pipeline.cjs')]),
    ('I25', 'common', 'REUSE_CONDITIONAL', 'checked integer primitives only if new numeric policy needs them', [
        g('integer.h', 'namespace grs'),
        g('integer_primitives.glsl', 'grsMul32'),
        g('g9spf/wide_integer.glsl', 'uint ')]),
    ('I26', 'evidence', 'FACILITY_NOT_NEW_QUALIFICATION', 'existing signing and legacy independent evidence are narrow', [
        source('docs/test-report-T579-signing-handoff-v1.md', '不使用真机'),
        source('docs/test-report-T541-device-predicate-v2.md', 'vertex=0'),
        source('docs/test-report-T556-dm2-oracle-v1.md', 'GPU 域全 NOT_RUN'),
        source('docs/test-report-T461-empty-raw-v3.md', 'nativeEndNs')]),
]


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT).decode('utf-8', 'replace').strip()


def main():
    receipts = {}
    assets = []
    for aid, route, disposition, purpose, entries in ASSETS:
        refs = []
        for path, symbol in entries:
            b = (ROOT / path).read_bytes()
            s = b.decode('utf-8-sig')
            assert symbol in s, (aid, path, symbol)
            receipts[path] = dict(bytes=len(b), sha256=hashlib.sha256(b).hexdigest())
            refs.append(dict(path=path, symbol=symbol, line=s[:s.index(symbol)].count('\n')+1))
        assets.append(dict(id=aid, route=route, disposition=disposition, purpose=purpose, refs=refs))
    # Resolve build facts without running cmake/node/hvigor or product tests.
    cmake = (ROOT / GPU / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
    direct = [x for x in ('m3a','m3b','g9spf','g9dm','g11vis','g12water') if '/'+x+'/kernels.cmake' in cmake]
    assert set(direct) == {'m3a','g9dm','g11vis','g12water'}, 'Canonical route graph changed; inspect new includes'
    skipped = []
    for route, script in [('g11vis','t566_generate_composite.cjs'),('g12water','t576_generate_pipeline.cjs')]:
        resolved = (ROOT / GPU / '../../../test' / script).resolve()
        skipped.append(dict(route=route, canonicalConfiguredGenerator=str(resolved), exists=resolved.exists(),
                            actualRepositoryGenerator='test/'+script))
        assert not resolved.exists(), 'Build-path observation changed; re-read actual CMake instead of preserving old finding'
    governing = {}
    for path in ['AGENTS.md', 'docs/goals.md', 'docs/architecture-T584-first-attribution-v1.md',
                 'docs/architecture-T589-first-attribution-v1.md', 'docs/architecture-T567-water-sfp-engineering-v1.md']:
        b = (ROOT / path).read_bytes()
        governing[path] = dict(bytes=len(b), sha256=hashlib.sha256(b).hexdigest())
    for path, receipt in receipts.items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest() == receipt['sha256'], ('concurrent source edit', path)
    wet = (ROOT/'test/t576_generate_pipeline.cjs').read_text(encoding='utf-8-sig')
    missing_writers = [dict(symbol=n, stage=int(s)) for n,s in re.findall(
        r'bool (g12\w+)\(\)\{return g12NotImplemented\((\d+)u\);\}', wet)]
    assert len(missing_writers) == 11, 'Wet implementation changed; inspect actual generator before classifying'
    payload = dict(schema='T613_G13_REUSE_INVENTORY_V1', date='2026-11-18', role='architect',
        snapshotHead=git('rev-parse','HEAD'),
        workingTreeModifications=git('diff','--name-only','--',*receipts).splitlines(),
        scope=['M1','M2','M3','G9','G11','G12'], executionOrder=[13,14,12],
        sourceBytesAreSnapshot=True, deployedIdentityProven=False, deviceUsed=False,
        buildsRun=False, productTestsRun=False, newFluidSolverImplemented=False, productPASS=False,
        assets=assets, receipts=receipts, governingInputs=governing,
        buildFacts=dict(canonicalDirectRoutes=direct, isolatedExtraRoutes=['m3b','g9spf'], conditionalGeneratorPaths=skipped),
        g12ExplicitNotImplementedWriters=missing_writers,
        handoff=dict(design='T605 chooses model/discretization/numerics; no model selected by T613',
                     engineering='T607 consumes T605 plus this inventory; new G13 identity/ABI/layout/caps/resource/cold/scene',
                     implementation='T608 complete product and compile only; no device/test closing prerequisite',
                     verification='T606 independent baselines then T609/T610 GPU quantitative/observable evidence'),
        limits=['symbol/byte existence is not runtime binding or product correctness',
                'legacy schemas, clocks, resource profiles, thresholds and physics do not automatically qualify G13',
                'T596/T599 are in-flight source snapshots, not newly accepted completion',
                'goals1-11 ongoing and goal10 paused; old water tasks deferred without deleting assets'])
    target = ROOT / 'docs/architecture-T613-g13-reuse-manifest-v1.json'
    target.write_text(json.dumps(payload, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(assets=len(assets), uniqueInputs=len(receipts), routes=payload['scope'],
                         directBuildRoutes=direct, buildsRun=False, testsRun=False), ensure_ascii=False))


if __name__ == '__main__':
    main()
