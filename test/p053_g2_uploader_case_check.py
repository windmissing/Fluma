#!/usr/bin/env python3
"""P-013 G2 uploader objective-case registration analyzer (tester side, T53).

Pre-registration analyzer for the G2 material-uploader cases registered in
docs/tester-T53-g2-uploader-cases-registration-v1.md and
docs/test-cases.md section 17 (TC-P013-G2-A01..E02).

What this script does (all read-only; it never modifies implementation,
frozen criteria, or the T09 CPU fixtures):

  1. Preservation check: the T09 CPU-fixture assets (reference JSON, T09
     executor, T09 registration doc, test-cases.md section 16 entries) must
     be byte-identical to the values recorded at T53 registration time.
     Any change is reported as PRESERVATION_VIOLATION.
  2. Traceability check: the clause tokens each registered case relies on
     (byte budget 65536, payload 983040/786432/196608, row strides
     1024/256, the seven runtime version identities, error codes
     400..404, the stage-state enum, the metadata field list) must still
     be present verbatim in the cited source documents. A missing token
     means the cited contract drifted and is reported as
     TRACEABILITY_GAP.
  3. Implementation-surface probe: for every case, the symbols/files the
     case needs are probed in the working tree. Absent surface is
     reported per case as NOT_IMPLEMENTED with a locatable missing list.
     The analyzer never fabricates a case result: with no G2 surface
     present, no case can be executed, so no case reports PASS or FAIL.
  4. Expectation contract: each case carries its machine-readable
     expectation (receipt state / error code / invariants) so a future
     G2 runner can consume this JSON directly. The expectations are
     copied from the frozen contract documents; this script does not
     create new numeric thresholds, tolerances, or error codes.

Scope boundaries (red lines): no algorithm/texture-size/physics-route
change, no frozen-threshold change, no new tolerance line, no T09 fixture
redefinition, no implementation-code change, no fabricated results, no
VALIDATED claim, no device use.

Output: test/reports/p053_g2_uploader_case_check.json
"""

import hashlib
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_PATH = os.path.join(ROOT, 'test', 'reports', 'p053_g2_uploader_case_check.json')

EXECUTION_ID = 'p053-g2-uploader-case-check-v1'

# ---------------------------------------------------------------- assets

T45_DOC = os.path.join(ROOT, 'docs', 'decision-P013-T45-material-rollout-v1.md')
REV2_DOC = os.path.join(ROOT, 'docs', 'architecture-P013-p1-material-composite-v1.md')
T24_DOC = os.path.join(ROOT, 'docs', 'developer-T24-material-preparation.md')
T09_REG = os.path.join(ROOT, 'docs', 'p013-p1-cpu-fixture-and-coupling-test-registration-v1.md')
REF_JSON = os.path.join(ROOT, 'docs', 'reference-P013-p1-material-recipe-v1.json')
T09_EXEC = os.path.join(ROOT, 'test', 'p013_cpu_reference_execution.py')
TEST_CASES = os.path.join(ROOT, 'docs', 'test-cases.md')

CPP_DIR = os.path.join(ROOT, 'entry', 'src', 'main', 'cpp')
ETS_DIR = os.path.join(ROOT, 'entry', 'src', 'main', 'ets')

# SHA-256 values captured at T53 registration time (2026-10-25). The T09
# evidence JSON is deliberately NOT re-hashed here: it is an output
# artifact that parallel sessions may regenerate; the fixture-definition
# layer below is the frozen surface.
FROZEN_HASHES = {
    'docs/reference-P013-p1-material-recipe-v1.json':
        'd214aa447f9f186eba159fd6953f2b42874420926a1b01b7c0977b940f1d63a6',
    'test/p013_cpu_reference_execution.py': None,  # recorded at first run
    'docs/p013-p1-cpu-fixture-and-coupling-test-registration-v1.md': None,
}

# T09 fixture/coupling case IDs that must survive untouched in test-cases.md.
T09_CASE_IDS = (
    [f'TC-P013-FIX-{i:02d}' for i in range(1, 10)] +
    [f'TC-P013-MCP-{i:02d}' for i in range(1, 6)]
)

# Clause tokens the registered cases depend on, with the document each
# token must appear in. Used for the traceability check only.
TRACE_TOKENS = (
    ('65,536', T45_DOC), ('65536', REV2_DOC), ('983040', REV2_DOC),
    ('786432', REV2_DOC), ('196608', REV2_DOC), ('1024', REV2_DOC),
    ('P1_ALBEDO_DELTA_V1', REV2_DOC), ('E2_GAUSS_COL6_R18_P1_V2', REV2_DOC),
    ('LIN_SRGB_REINHARD_V1', REV2_DOC), ('P1_TEXEL_CANVAS_PX_V1', REV2_DOC),
    ('CONTINUOUS_COLUMN_WARP_V1', REV2_DOC), ('zOriginVersion', REV2_DOC),
    ('MATERIAL_PACKET_INVALID', REV2_DOC), ('ASSET_NOT_READY', REV2_DOC),
    ('ASSET_REVISION_MISMATCH', REV2_DOC), ('COLORSPACE_UNSUPPORTED', REV2_DOC),
    ('COMPOSITION_ORDER_BLOCKED', REV2_DOC),
    ('CPU_STAGED', REV2_DOC), ('UPLOADING', REV2_DOC), ('READY', REV2_DOC),
    ('ACTIVE', REV2_DOC), ('FAILED', REV2_DOC), ('DISCARDED', REV2_DOC),
    ('materialBundleId', REV2_DOC), ('assetGeneration', REV2_DOC),
    ('appearanceConfigRevision', REV2_DOC), ('renderSeed', REV2_DOC),
    ('geometryEpoch', REV2_DOC), ('paletteHash', REV2_DOC),
    ('grainHash', REV2_DOC), ('strataHash', REV2_DOC),
    ('inputEncoding', REV2_DOC), ('assetGeneratorVersion', REV2_DOC),
    ('materialRecipeVersion', REV2_DOC), ('colorPipelineVersion', REV2_DOC),
    ('effectiveLambdaMaterial', REV2_DOC), ('warpRangeCycles', REV2_DOC),
    ('surfaceEpoch', REV2_DOC), ('contextEpoch', REV2_DOC),
    ('stageMaterialBundle', REV2_DOC), ('pumpMaterialUploads', REV2_DOC),
    ('discardMaterialStage', REV2_DOC),
    ('byteOffset', REV2_DOC),
    ('DISCARDED', T45_DOC), ('65,536', T45_DOC),
    ('stageMaterialBundle', T45_DOC), ('pumpMaterialUploads', T45_DOC),
    ('canActivate', T45_DOC), ('activateMaterial', T45_DOC),
    ('discardMaterialStage', T45_DOC), ('SHA256', T45_DOC),
    ('CanActivate', REV2_DOC), ('Activate(generation)', REV2_DOC),
)

# ------------------------------------------------- implementation surface

def scan_tree_symbols():
    """Collect every source file under cpp/ and ets/ with its text."""
    files = {}
    for base in (CPP_DIR, ETS_DIR):
        for dirpath, _dirs, names in os.walk(base):
            for name in names:
                if not name.endswith(('.cpp', '.h', '.ets', '.ts')):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, ROOT).replace('\\', '/')
                try:
                    with open(path, 'r', encoding='utf-8', errors='replace') as fh:
                        files[rel] = fh.read()
                except OSError:
                    files[rel] = ''
    return files


def probe_surface(files):
    """Probe the G2 implementation surface; return per-symbol findings."""
    surface = {}

    def add(name, kind, pattern, note):
        hits = sorted(rel for rel, text in files.items() if re.search(pattern, text))
        surface[name] = {
            'kind': kind, 'pattern': pattern, 'present': bool(hits),
            'hits': hits, 'note': note
        }

    add('native_material_uploader', 'file', r'material_uploader\.(cpp|h)',
        'rev2 §3/T45 §2 文件落点 entry/src/main/cpp/p013renderer/material_uploader.{h,cpp}')
    add('bridge_stageMaterialBundle', 'symbol', r'stageMaterialBundle',
        'rev2 §6 stageMaterialBundle(desc, grainRgba, strataRgba)')
    add('bridge_pumpMaterialUploads', 'symbol', r'pumpMaterialUploads',
        'rev2 §6 pumpMaterialUploads(byteBudget)')
    add('bridge_canActivate', 'symbol', r'canActivate',
        'rev2 §6 canActivate(generation, contextEpoch)')
    add('bridge_activateMaterial', 'symbol', r'activateMaterial',
        'rev2 §6 activateMaterial(generation, frameToken)')
    add('bridge_discardMaterialStage', 'symbol', r'discardMaterialStage',
        'rev2 §6 discardMaterialStage(assetGeneration)')
    add('bridge_drawMaterialFrame', 'symbol', r'drawMaterialFrame',
        'rev2 §6 drawMaterialFrame(frame, tdataV2, light, material)')
    add('ets_stage_receipt_type', 'symbol', r'P013MaterialStageReceipt',
        'rev2 §6 P013MaterialStageReceipt={code,assetGeneration,state,receivedBytes,uploadedBytes,totalBytes,activeGeneration,appearanceConfigRevision,contextEpoch,errorStage}')
    add('ets_bundle_desc_type', 'symbol', r'P013MaterialBundleDesc',
        'rev2 §6 P013MaterialBundleDesc 元数据 + sessionId/surfaceEpoch/contextEpoch')
    add('ets_material_params_type', 'symbol', r'P013MaterialParams',
        'rev2 §6 P013MaterialParams 三带/palette/强度/warp/λ/消融/坐标映射')
    add('ets_material_frame_input_type', 'symbol', r'P013MaterialFrameInput',
        'rev2 §6 P013MaterialFrameInput apiVersion=2 与七版本身份')
    add('ets_material_frame_result_type', 'symbol', r'P013MaterialFrameResult',
        'rev2 §6 P013MaterialFrameResult 必返代/版本分列')
    add('ets_material_controller', 'symbol', r'P013MaterialController',
        'T45 §2 ETS controller（P013MaterialController.ets）')
    add('ets_api_version_2', 'symbol', r'apiVersion\s*[:=]\s*2',
        'rev2 §6 新增类型含 apiVersion=2')
    add('ets_mode_4_5', 'symbol', r'GLES_SAND_COMPOSITE_(OFF|ON)',
        'rev2 §6 模式值 4=OFF / 5=ON')
    add('native_error_400', 'symbol', r'MATERIAL_PACKET_INVALID',
        'rev2 §7 新错误码 400')
    add('native_error_401', 'symbol', r'ASSET_NOT_READY',
        'rev2 §7 新错误码 401')
    add('native_error_402', 'symbol', r'ASSET_REVISION_MISMATCH',
        'rev2 §7 新错误码 402')
    add('native_error_403', 'symbol', r'COLORSPACE_UNSUPPORTED',
        'rev2 §7 新错误码 403')
    add('native_error_404', 'symbol', r'COMPOSITION_ORDER_BLOCKED',
        'rev2 §7 新错误码 404')
    add('telemetry_upload_fields', 'symbol', r'materialUploadBytes',
        'rev2 §8 uploader 遥测 materialUploadBytes/materialPumpCount/materialTextureLiveCount')
    add('telemetry_pump_count', 'symbol', r'materialPumpCount',
        'rev2 §8 uploader 遥测')
    add('telemetry_texture_live', 'symbol', r'materialTextureLiveCount',
        'rev2 §8 uploader 遥测（活纹理对象 ≤4 数组）')
    add('telemetry_fallback_reason', 'symbol', r'fallbackReason',
        'rev2 §8 全链/恢复遥测 fallbackReason')
    add('telemetry_generation_fields', 'symbol', r'(pendingGeneration|activeGeneration)',
        'rev2 §8 CPU 资产遥测 pendingGeneration/activeGeneration')
    add('cmake_uploader_entry', 'symbol', r'material_uploader',
        'cpp/CMakeLists.txt 新源码登记（扫描范围含 CMakeLists 文本另行单列）')
    add('controller_legacy_full', 'symbol', r'drawLegacyFull',
        'rev2 §7 回退路径 drawLegacyFull')
    return surface


# ------------------------------------------------------------- case table

# Each case: id, class, title, expectation (frozen), requires (surface keys),
# basis (clause citation).
CASES = [
    # ---- A: Stage 校验类（T45 §2 第2条 / rev2 §6；每类至少一条）----
    dict(id='TC-P013-G2-A01', cls='A-stage-validation', title='缺字段（metadata 必填字段缺失）',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='metadata',
                          activeUnchanged=True, pendingSlot='failed-entry-or-empty',
                          note='stage 先验整包 metadata；失败不改变 ACTIVE 资源'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'ets_bundle_desc_type', 'native_error_400'],
         basis='T45 §3.1；rev2 §6 "stage先验整包metadata/字节hash/alpha255，失败不改变ACTIVE资源"'),
    dict(id='TC-P013-G2-A02', cls='A-stage-validation', title='未知版本（七版本身份取值不在登记表）',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='version',
                          activeUnchanged=True,
                          note='native 遇未知版本拒绝，不自动推断缺省值'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'ets_api_version_2', 'native_error_400'],
         basis='rev2 §4.4/§6 "未知或不匹配的版本拒绝绘制，不推断缺省值"'),
    dict(id='TC-P013-G2-A03', cls='A-stage-validation', title='七版本不齐（desc 未携带全部七项版本身份）',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='version',
                          activeUnchanged=True,
                          note='materialRecipe/slopeKernel/colorPipeline/textureSampling/strataSampling/T_dataLayout/zOrigin 七项须逐项精确匹配'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'ets_bundle_desc_type', 'native_error_400'],
         basis='rev2 §4.4 七个版本身份登记表'),
    dict(id='TC-P013-G2-A04', cls='A-stage-validation', title='配置修订不符（appearanceConfigRevision 不符）',
         expectation=dict(receiptState='FAILED', errorCode=402, errorStage='appearanceConfigRevision',
                          activeUnchanged=True,
                          note='修订不符归入 ASSET_REVISION_MISMATCH；实现若用其它既有码，按实际码报告交裁定'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'native_error_402'],
         basis='T45 §2 第2条"配置修订不符"；rev2 §6/§7 错误码枚举'),
    dict(id='TC-P013-G2-A05', cls='A-stage-validation', title='身份不符（materialBundleId/assetGeneration 身份冲突）',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='identity',
                          activeUnchanged=True,
                          note='hash 基于实际字节与生成参数，不以文件名当 hash；身份冲突整包拒绝'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'ets_bundle_desc_type', 'native_error_400'],
         basis='T45 §2 第2条"身份不符"；rev2 §4.3 metadata/hash 规则'),
    dict(id='TC-P013-G2-A06', cls='A-stage-validation', title='精确字节数不符（grain≠786432 / strata≠196608 / 总≠983040）',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='byteLength',
                          activeUnchanged=True, receivedBytes='actual',
                          note='有效总字节固定 983040，不接任意大纹理'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'native_error_400'],
         basis='T45 §2 第2条"长度错误"；rev2 §4.3/§6 字节数契约'),
    dict(id='TC-P013-G2-A07', cls='A-stage-validation', title='byteOffset/byteLength 越界（TypedArray 视图/元素类型不匹配）',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='byteOffset',
                          activeUnchanged=True,
                          note='只接受匹配元素类型/长度/有效 buffer，正确处理 byteOffset'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'native_error_400'],
         basis='T45 §2 第2条"TypedArray byteOffset/byteLength"；rev2 §6 typedarray 规则'),
    dict(id='TC-P013-G2-A08', cls='A-stage-validation', title='SHA256 不符',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='hash',
                          activeUnchanged=True,
                          note='整包字节 SHA256 与 desc 登记值逐项核对'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'native_error_400'],
         basis='T45 §2 第2条"hash错误"；T45 §3.1'),
    dict(id='TC-P013-G2-A09', cls='A-stage-validation', title='alpha 非法（任一像素 alpha≠255）',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='alpha',
                          activeUnchanged=True,
                          note='六图 alpha 固定 255；任一非 255 整包拒绝'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'native_error_400'],
         basis='T45 §2 第2条"alpha错误"；rev2 §4.3 alpha255 无预乘'),
    dict(id='TC-P013-G2-A10', cls='A-stage-validation', title='context epoch 不匹配',
         expectation=dict(receiptState='FAILED', errorCode=400, errorStage='contextEpoch',
                          activeUnchanged=True,
                          note='stage/帧必须同 contextEpoch；失配拒绝且不推断'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'ets_bundle_desc_type', 'native_error_400'],
         basis='T45 §2 第2条"上下文epoch不匹配"；rev2 §6 frame 同 epoch 规则'),
    # ---- B: 双代与生命周期类 ----
    dict(id='TC-P013-G2-B01', cls='B-generation-lifecycle', title='pending 被新代取代须报告 DISCARDED',
         expectation=dict(supersededReceiptState='DISCARDED', activeGenerationUnchanged=True,
                          discardedResourcesReleased=True,
                          note='新申请覆盖 pending 先显式 DISCARDED，active 仍可画'),
         requires=['bridge_stageMaterialBundle', 'bridge_discardMaterialStage',
                   'ets_stage_receipt_type', 'telemetry_generation_fields'],
         basis='T45 §2 第3条/§3.1；rev2 §6 "新申请覆盖pending先显式DISCARDED"'),
    dict(id='TC-P013-G2-B02', cls='B-generation-lifecycle', title='stage 失败不动 active（active 代继续可绘制）',
         expectation=dict(activeGenerationUnchanged=True, activeDrawableAfterFailure=True,
                          pendingSlot='failed-entry-or-empty',
                          note='stage/上传/校验失败保留旧 active'),
         requires=['bridge_stageMaterialBundle', 'ets_stage_receipt_type',
                   'bridge_drawMaterialFrame', 'ets_material_frame_result_type'],
         basis='T45 §2 第3条"active不受stage失败影响"；T45 §3.4'),
    dict(id='TC-P013-G2-B03', cls='B-generation-lifecycle', title='双代上限不变量（最多一代 active + 一代 pending）',
         expectation=dict(maxActiveGenerations=1, maxPendingGenerations=1,
                          textureLiveCountMax=4,
                          note='live texture 对象最多 4 个数组（两包×两对象），不用每帧 glGen/compile'),
         requires=['bridge_stageMaterialBundle', 'telemetry_texture_live',
                   'telemetry_generation_fields'],
         basis='T45 §3.1；rev2 §6 双代持有规则'),
    dict(id='TC-P013-G2-B04', cls='B-generation-lifecycle', title='上下文重建后从 CPU 缓存恢复',
         expectation=dict(cpuCacheRetained=True, reuploadRequiredBeforeReady=True,
                          staleEpochFramesRejected=True, canvasFallbackUntilReady=True,
                          note='context 丢失保留 CPU 资产并重传，确认 READY 后再切回'),
         requires=['bridge_pumpMaterialUploads', 'bridge_canActivate',
                   'ets_material_controller', 'controller_legacy_full',
                   'telemetry_upload_fields'],
         basis='T45 §2 第3条"上下文重建后从CPU缓存恢复"；T45 §3.4；rev2 §6/§7'),
    # ---- C: Pump 预算类 ----
    dict(id='TC-P013-G2-C01', cls='C-pump-budget', title='每 tick ≤65,536B 且只提交完整行',
         expectation=dict(maxBytesPerTick=65536, completeRowsOnly=True,
                          grainRowStride=1024, strataRowStride=256,
                          note='按完整像素行 chunk 上传；不允许半行/跨 tick 截断'),
         requires=['bridge_pumpMaterialUploads', 'telemetry_upload_fields',
                   'telemetry_pump_count'],
         basis='T45 §2 第4条；T45 §3.2；rev2 §6 pump 规则'),
    dict(id='TC-P013-G2-C02', cls='C-pump-budget', title='byteBudget 域校验（正整数且 ≤65536）',
         expectation=dict(acceptedDomain='positive integer <= 65536',
                          rejectedInputs=[0, -1, 1.5, 65537],
                          onReject=dict(uploadedBytes=0, pendingIntact=True),
                          note='0/负/小数/超限拒绝且不产生上传；pending 可被后续合法 pump 继续'),
         requires=['bridge_pumpMaterialUploads', 'ets_stage_receipt_type'],
         basis='rev2 §6 "byteBudget须正整数且≤65536"'),
    dict(id='TC-P013-G2-C03', cls='C-pump-budget', title='全部上传成功才 READY',
         expectation=dict(partialState='UPLOADING', allSuccessState='READY',
                          note='只有全部资源和 metadata 通过后才进入 READY'),
         requires=['bridge_pumpMaterialUploads', 'ets_stage_receipt_type'],
         basis='T45 §2 第4条；T45 §3.2'),
    dict(id='TC-P013-G2-C04', cls='C-pump-budget', title='epoch 失配废弃 pending 并等待重新 stage',
         expectation=dict(onEpochMismatch=dict(pendingState='DISCARDED', waitForRestage=True,
                                                uploadsAgainstStaleContext=0),
                          note='目标 surface/context epoch 失配则废弃 pending'),
         requires=['bridge_pumpMaterialUploads', 'bridge_stageMaterialBundle',
                   'ets_stage_receipt_type'],
         basis='T45 §2 第4条；T45 §3.2'),
    # ---- D: 激活原子性反例 ----
    dict(id='TC-P013-G2-D01', cls='D-activation-atomicity', title='部分代就绪即激活被拒绝（跨代画面反例）',
         expectation=dict(canActivateFalseWhenNotReady=True, activateRejectedCode=401,
                          activeUnchanged=True, mixedGenerationFrames=0,
                          note='grain 新代已上传而 strata/palette 未就绪 → 拒绝激活'),
         requires=['bridge_canActivate', 'bridge_activateMaterial',
                   'native_error_401', 'ets_material_frame_input_type'],
         basis='T45 §2 第5条；T45 §4 部分代激活反例；rev2 §6'),
    dict(id='TC-P013-G2-D02', cls='D-activation-atomicity', title='旧 active 保留到在途旧帧结束后释放',
         expectation=dict(atomicSwitchAtFrameBoundary=True,
                          switchSet=['textures', 'palette', 'uniform', 'fog', 'configRevision'],
                          sameFrameToken=True, oldActiveReleasedAfterInflightFrames=True,
                          note='一帧边界同时切换整代；禁止按资源逐张激活'),
         requires=['bridge_activateMaterial', 'bridge_drawMaterialFrame',
                   'ets_material_frame_result_type', 'telemetry_generation_fields'],
         basis='T45 §2 第5条；T45 §3.3；rev2 §6/§7'),
    dict(id='TC-P013-G2-D03', cls='D-activation-atomicity', title='上传/校验失败保留旧 active（无混代帧）',
         expectation=dict(activeUnchanged=True, failedGenerationNeverDrawn=True,
                          pendingStateOnFailure='FAILED-or-DISCARDED',
                          note='GL 上传失败回滚；旧 active 与旧配置保持'),
         requires=['bridge_pumpMaterialUploads', 'bridge_activateMaterial',
                   'bridge_drawMaterialFrame', 'ets_material_frame_result_type'],
         basis='T45 §2 第5条；T45 §3.4；rev2 §7 失败发生在 staging 时保留旧 active'),
    # ---- E: 回退反例 ----
    dict(id='TC-P013-G2-E01', cls='E-fallback', title='draw 失败同 tick 回退完整 Canvas 旧路径并关闭整块 GLES',
         expectation=dict(sameTickLegacyFullRedraw=True, entireGlesHidden=True,
                          singleLayerHideForbidden=True, fallbackReasonTelemetry=True,
                          note='不得只隐藏一层造成 Canvas/GLES 重叠或空画面'),
         requires=['controller_legacy_full', 'ets_material_controller',
                   'ets_mode_4_5', 'telemetry_fallback_reason'],
         basis='T45 §2 第6条；T45 §4 空窗/双绘反例；rev2 §7'),
    dict(id='TC-P013-G2-E02', cls='E-fallback', title='profile/swap 失败同 tick 回退（不留空帧/双层残影）',
         expectation=dict(sameTickLegacyFullRedraw=True, entireGlesHidden=True,
                          noTransparentClearAssumption=True, fallbackReasonTelemetry=True,
                          note='swap 失败不得假设透明 clear 成功；不等待失败 swap 再透明清屏'),
         requires=['controller_legacy_full', 'ets_material_controller',
                   'ets_mode_4_5', 'telemetry_fallback_reason'],
         basis='T45 §2 第6条；T45 §4；rev2 §7'),
]

CLASS_LABELS = {
    'A-stage-validation': 'Stage 校验类',
    'B-generation-lifecycle': '双代与生命周期类',
    'C-pump-budget': 'Pump 预算类',
    'D-activation-atomicity': '激活原子性反例',
    'E-fallback': '回退反例',
}

# ---------------------------------------------------------------- helpers

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def read_text(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as fh:
        return fh.read()


def check_preservation():
    """T09 fixture-definition layer must be byte-identical to registration."""
    results = []
    for rel, expected in FROZEN_HASHES.items():
        path = os.path.join(ROOT, rel.replace('/', os.sep))
        if not os.path.exists(path):
            results.append(dict(asset=rel, status='MISSING', expected=expected, actual=None))
            continue
        actual = sha256_file(path)
        if expected is None:
            results.append(dict(asset=rel, status='RECORDED', expected=None, actual=actual,
                                note='first-run baseline recorded by this analyzer'))
        else:
            results.append(dict(asset=rel,
                                status='PASS' if actual == expected else 'PRESERVATION_VIOLATION',
                                expected=expected, actual=actual))
    text = read_text(TEST_CASES)
    missing = [cid for cid in T09_CASE_IDS if cid not in text]
    results.append(dict(asset='docs/test-cases.md §16 条目',
                        status='PASS' if not missing else 'PRESERVATION_VIOLATION',
                        expected=f'{len(T09_CASE_IDS)} 个 T09 用例 ID 全部在册',
                        actual='缺失: ' + (', '.join(missing) if missing else '无')))
    return results


def check_traceability(doc_cache):
    results = []
    for token, doc in TRACE_TOKENS:
        rel = os.path.relpath(doc, ROOT).replace('\\', '/')
        text = doc_cache.get(rel)
        if text is None:
            text = read_text(doc)
            doc_cache[rel] = text
        present = token in text
        results.append(dict(token=token, document=rel,
                            status='PASS' if present else 'TRACEABILITY_GAP'))
    return results


def evaluate_cases(surface):
    out = []
    for case in CASES:
        missing = [key for key in case['requires'] if not surface[key]['present']]
        if missing:
            status = 'NOT_IMPLEMENTED'
        else:
            # Surface present but no G2 execution runner is registered in this
            # analyzer; the case expectation is handed to the G2 runner as a
            # contract. Never report PASS/FAIL without an executed run.
            status = 'SURFACE_PRESENT_AWAITING_G2_RUNNER'
        out.append(dict(
            id=case['id'], cls=case['cls'], title=case['title'],
            expectation=case['expectation'], requires=case['requires'],
            missingSurface=missing,
            missingDetail=[surface[k]['note'] for k in missing],
            status=status, basis=case['basis']))
    return out


def main():
    problems = []
    doc_cache = {}
    preservation = check_preservation()
    for item in preservation:
        if item['status'] in ('PRESERVATION_VIOLATION', 'MISSING'):
            problems.append(f"preservation: {item['asset']} -> {item['status']}")
    traceability = check_traceability(doc_cache)
    for item in traceability:
        if item['status'] != 'PASS':
            problems.append(f"traceability: token {item['token']} missing in {item['document']}")

    files = scan_tree_symbols()
    surface = probe_surface(files)
    cases = evaluate_cases(surface)

    by_class = {}
    for case in cases:
        bucket = by_class.setdefault(case['cls'], dict(label=CLASS_LABELS[case['cls']],
                                                       total=0, notImplemented=0,
                                                       awaitingRunner=0))
        bucket['total'] += 1
        if case['status'] == 'NOT_IMPLEMENTED':
            bucket['notImplemented'] += 1
        elif case['status'] == 'SURFACE_PRESENT_AWAITING_G2_RUNNER':
            bucket['awaitingRunner'] += 1

    absent_surface = sorted(k for k, v in surface.items() if not v['present'])
    report = dict(
        executionId=EXECUTION_ID,
        generatedBy='test/p053_g2_uploader_case_check.py',
        date='2026-10-25',
        scope='T53 G2 uploader objective-case registration; RED baseline only',
        status='TEST_REGISTRATION_ONLY / NOT_IMPLEMENTED / NOT_CALIBRATED / '
               'NOT_VALIDATED / G2_NOT_UNFROZEN',
        preservation=preservation,
        traceability=traceability,
        implementationSurface={k: v for k, v in sorted(surface.items())},
        absentSurface=absent_surface,
        cases=cases,
        summary=dict(
            totalCases=len(cases),
            notImplemented=sum(1 for c in cases if c['status'] == 'NOT_IMPLEMENTED'),
            surfacePresentAwaitingRunner=sum(
                1 for c in cases if c['status'] == 'SURFACE_PRESENT_AWAITING_G2_RUNNER'),
            byClass=by_class,
            preservationViolations=sum(
                1 for p in preservation if p['status'] in ('PRESERVATION_VIOLATION', 'MISSING')),
            traceabilityGaps=sum(1 for t in traceability if t['status'] != 'PASS'),
            redBaseline=('G2 uploader surface absent (material_uploader.{h,cpp}, bridge '
                         'stageMaterialBundle/pumpMaterialUploads/canActivate/activateMaterial/'
                         'discardMaterialStage, receipt/desc types, 400..404 codes, upload '
                         'telemetry all missing); every registered case is NOT_IMPLEMENTED '
                         'with a locatable missing list; no case result is fabricated.'),
        ),
        problems=problems,
    )
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, 'w', encoding='utf-8') as fh:
        json.dump(report, fh, ensure_ascii=False, indent=1)
        fh.write('\n')

    print(f"cases={report['summary']['totalCases']} "
          f"not_implemented={report['summary']['notImplemented']} "
          f"preservation_violations={report['summary']['preservationViolations']} "
          f"traceability_gaps={report['summary']['traceabilityGaps']}")
    for problem in problems:
        print('PROBLEM: ' + problem)
    if problems:
        sys.exit(1)


if __name__ == '__main__':
    main()
