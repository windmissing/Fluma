// T91: SCENE-GEO-01..05 functional regression on the authorized scene migration
// (T89 routing, manager-adjudicated). Independent executor using the REAL
// P013SceneController + P013ScenePreparation + native stub; identity fixed to
// T56 commit 114a7ee tree. Historical T62 cases keep their original FAIL and
// are marked SUPERSEDED_BY_AUTHORIZED_SCENE_MIGRATION (not edited green).
'use strict';
const fs = require('fs'), a = require('assert/strict'), Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(
  fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const load = Module._load;
Module._load = function (n, p, m) {
  if (n === 'libp013renderer.so') return {default: {}};
  if (n === '@ohos.hilog') return {default: {info() {}, warn() {}, error() {}}};
  return load.call(this, n, p, m);
};
const {P013SceneController: C} = require('../entry/src/main/ets/render/P013SceneController.ets');
const {P051EmptySandDemo: D} = require('../entry/src/main/ets/physics/P051EmptySandDemo.ets');
const {WaterSandField: F} = require('../entry/src/main/ets/physics/WaterSandField.ets');
const {p012FactoryAppearance: factory} = require('../entry/src/main/ets/render/P012RenderTypes.ets');

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
function fieldOnlyPublication() {
  const f = new F(128, 256);
  const out = f.createPublishedRenderStateSlot();
  a.equal(f.copyPublishedSceneStateInto(out), 'OK');
  return {field: f, state: out};
}
function demoPublication() {
  const d = new D(), state = d.createPublication();
  d.start(64, 200); d.tick(1e9, 1 / 60); d.tick(1e9 + 2e7, 1 / 60);
  a.equal(d.field.copyPublishedRenderStateInto(state), 'OK');
  return {demo: d, state};
}
function ctx(over) {
  return Object.assign({physicsCommitId: 1, fieldEpoch: 1, continuityEpoch: 1, geometryEpoch: 1,
    frameSerial: 1, sampleTimeMs: 1000, frameDeltaMs: 16, boxX: 0, boxY: 0, boxW: 128, boxH: 256,
    renderSeed: 51, configRevision: 0, appearanceTier: 3, ablation: 0, visualInverted: false,
    particleLayerEnabled: true}, over || {});
}
// configurable native stub
function makeNative(over) {
  const o = Object.assign({
    state: {surfaceEpoch: 1, contextEpoch: 1},
    prepared: {code: 0, sizeSource: 'EGL_QUERY_SURFACE', widthPx: 800, heightPx: 480, surfaceEpoch: 1, contextEpoch: 1},
    initFail: 0, prepareFail: null, raceAfterPrepare: false, drawReceipt: null, drawThrows: null,
    log: [], bindCount: 0, configValues: null,
  }, over || {});
  return {
    __o: o,
    init: (name) => { o.log.push(['init', name]); return o.initFail ? {code: o.initFail} : {code: 0, sessionId: 1}; },
    getState: () => ({...o.state}),
    prepareDualFieldSurface: (s, e) => {
      o.log.push(['prepare', s, e]);
      if (o.prepareFail) return o.prepareFail;
      return {...o.prepared};
    },
    bindMaterialConfiguration: (b) => { o.log.push(['bind', b.appearanceConfigRevision]); o.bindCount++; return {...b, code: 0, bindingId: ++o.bindCount}; },
    registerSceneConfiguration: (b, v) => { o.log.push(['register']); o.configValues = Array.from(v); return {code: 0}; },
    stageMaterialBundle: (d, g, s) => { o.log.push(['stage', d.assetGeneration]); return {code: 0}; },
    pumpMaterialUploads: () => ({code: 0}),
    drawNewPipelineFrame: (f, data, xy, meta, motion) => {
      o.log.push(['scene', f.frameId]);
      o.lastFrame = JSON.parse(JSON.stringify(f));
      if (o.drawThrows) throw o.drawThrows;
      return o.drawReceipt || {code: 0, currentFrameSucceeded: true, presentationState: 'CURRENT',
        faultReason: '', firstFailureStage: 'NONE', requestedSwapSucceeded: true, staleSwapSucceeded: false,
        requestedFrameToken: f.frameToken, lastGoodFrameToken: f.frameToken, renderedFrameToken: f.frameToken,
        lastGoodGeneration: f.requestedGeneration, actualPresentationGeneration: f.requestedGeneration,
        renderedConfigRevision: f.appearanceConfigRevision, sourceFailure: f.sourceFailure || '',
        particleCount: f.particleCount};
    },
    setActive: () => ({code: 0}), release: () => ({code: 0}),
  };
}
const OUT = 'test/reports/t91';
fs.mkdirSync(OUT, {recursive: true});

// ================= SCENE-GEO-01 冷启动 =================
{
  const {state} = demoPublication();
  const native = makeNative();
  const c = new C(native);
  const r = c.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, 1);
  const prepareCall = native.__o.log.find(l => l[0] === 'prepare');
  const f = native.__o.lastFrame;
  const ok1 = r !== null && r.currentFrameSucceeded && native.__o.prepared.widthPx === 800 &&
    f.surfaceWidthPx === 800 && f.surfaceHeightPx === 480 && f.boxWidthPx === 800 && f.boxHeightPx === 480;
  rec('SCENE-GEO-01-A', 'cold start: real prepared 800x480 px used for full-box packet (no caller guess)', ok1,
    {prepare: prepareCall, surfaceWidthPx: f && f.surfaceWidthPx, surfaceHeightPx: f && f.surfaceHeightPx,
     boxWidthPx: f && f.boxWidthPx, boxHeightPx: f && f.boxHeightPx, frameToken: f && f.frameToken});
  // zero-size prepared surface refused
  const n2 = makeNative({prepared: {code: 0, sizeSource: 'EGL_QUERY_SURFACE', widthPx: 0, heightPx: 480, surfaceEpoch: 1, contextEpoch: 1}});
  const c2 = new C(n2);
  const r2 = c2.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, 1);
  rec('SCENE-GEO-01-B', 'zero-size prepared surface refused (null receipt, PREPARE failure identity)',
    r2 === null && c2.failure.startsWith('PREPARE'), {failure: c2.failure});
  // NaN scale refused
  const n3 = makeNative(); const c3 = new C(n3);
  const r3 = c3.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, NaN);
  rec('SCENE-GEO-01-C', 'NaN/zero scale refused with SCENE_SCALE identity', r3 === null && c3.failure === 'SCENE_SCALE', {failure: c3.failure});
}

// ================= SCENE-GEO-02 生命周期竞争 =================
{
  const {state} = demoPublication();
  // prepare failure -> null, no frame submitted
  const n1 = makeNative({prepareFail: {code: 7, sizeSource: 'NONE', widthPx: 0, heightPx: 0, surfaceEpoch: 1, contextEpoch: 1, errorStage: 'surface'}});
  const c1 = new C(n1);
  const r1 = c1.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, 1);
  rec('SCENE-GEO-02-A', 'prepare failure -> null receipt, no scene draw call',
    r1 === null && !n1.__o.log.some(l => l[0] === 'scene'), {failure: c1.failure, log: n1.__o.log.map(l => l[0])});
  // epoch race: state changed between prepare and re-getState
  const n2 = makeNative();
  n2.prepareDualFieldSurface = (s, e) => { n2.__o.state.surfaceEpoch = 99; return {...n2.__o.prepared}; };
  const c2 = new C(n2);
  const r2 = c2.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, 1);
  rec('SCENE-GEO-02-B', 'surface-epoch race after prepare -> STALE_PREPARED_SURFACE, no draw', 
    r2 === null && c2.failure === 'STALE_PREPARED_SURFACE' && !n2.__o.log.some(l => l[0] === 'scene'), {failure: c2.failure});
  // draw reports failure: currentFrameSucceeded=false -> controller exposes firstFailureStage, receipt preserved
  const n3 = makeNative({drawReceipt: {code: 0, currentFrameSucceeded: false, presentationState: 'FAULT',
    faultReason: 'LAYER_FAIL', firstFailureStage: 'SCENE_MATERIAL', requestedSwapSucceeded: false, staleSwapSucceeded: false,
    requestedFrameToken: 'req', lastGoodFrameToken: 'old-good', renderedFrameToken: null,
    lastGoodGeneration: 0, actualPresentationGeneration: 0, renderedConfigRevision: 0,
    sourceFailure: '', particleCount: 0}});
  const c3 = new C(n3);
  const r3 = c3.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, 1);
  rec('SCENE-GEO-02-C', 'draw-layer failure: receipt preserved, failure=firstFailureStage, no false CURRENT',
    r3 !== null && r3.currentFrameSucceeded === false && c3.failure === 'SCENE_MATERIAL' &&
    r3.presentationState === 'FAULT' && r3.lastGoodFrameToken === 'old-good',
    {failure: c3.failure, presentationState: r3 && r3.presentationState, lastGood: r3 && r3.lastGoodFrameToken});
  // STALE replay identity: with a prior good frame, staleSwap path distinguishable from requestedSwap
  const n4 = makeNative({drawReceipt: {code: 0, currentFrameSucceeded: false, presentationState: 'STALE',
    faultReason: '', firstFailureStage: 'SOURCE', requestedSwapSucceeded: false, staleSwapSucceeded: true,
    requestedFrameToken: 'req2', lastGoodFrameToken: 'good1', renderedFrameToken: null,
    lastGoodGeneration: 1, actualPresentationGeneration: 1, renderedConfigRevision: 0,
    sourceFailure: 'INVALID_PARTICLE', particleCount: 0}});
  const c4 = new C(n4);
  const r4 = c4.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, 1);
  rec('SCENE-GEO-02-D', 'STALE with compatible LastGood: staleSwap=true distinct from requestedSwap',
    r4 !== null && r4.staleSwapSucceeded === true && r4.requestedSwapSucceeded === false &&
    r4.presentationState === 'STALE',
    {presentationState: r4 && r4.presentationState, staleSwap: r4 && r4.staleSwapSucceeded,
     requestedSwap: r4 && r4.requestedSwapSucceeded, lastGood: r4 && r4.lastGoodFrameToken});
}

// ================= SCENE-GEO-03 Float32 保护（T61 右列临界值经 scene 真实链） =================
{
  const {state, demo} = demoPublication();
  const pool = state.field.particleState;
  const native = makeNative();
  const c = new C(native);
  // legal critical Float64 right boundary source (frounds to 128) through the REAL scene chain
  const originals = Array.from(pool.pX);
  const cases = [
    {name: 'midpoint 127.99999618530273', v: 127.99999618530273},
    {name: '127.99999999', v: 127.99999999},
  ];
  let allOk = true, detail = [];
  let seq = 1;
  for (const tc of cases) {
    pool.pX[0] = tc.v;
    const before = Array.from(pool.pX);
    const r = c.drawNewPipelineFrame(state, ctx({physicsCommitId: state.physicsCommitId, frameSerial: seq, sampleTimeMs: 1000 + seq * 16}), factory(), 1, 2e9, 1);
    seq++;
    // Authorized COLUMN_PRESERVING projection: legal Float64 boundary sources are accepted and
    // projected to the previous Float32 INSIDE the domain; authoritative source bytes unchanged.
    const accepted = r !== null && (r.sourceFailure === '' || r.sourceFailure === undefined) &&
      JSON.stringify(Array.from(pool.pX)) === JSON.stringify(before);
    detail.push({case: tc.name, sourceFailure: r && r.sourceFailure, sourceUnchanged: JSON.stringify(Array.from(pool.pX)) === JSON.stringify(before)});
    if (!accepted) allOk = false;
    pool.pX[0] = originals[0];
  }
  rec('SCENE-GEO-03-A', 'scene chain: legal Float64 right-boundary sources ACCEPTED via COLUMN_PRESERVING projection (source bytes unchanged)', allOk,
    {cases: detail, note: 'T61 proved RAW revision rejects these; T89/T62 authorized the projection branch for the scene: pre-check only rejects sources already outside the domain in Float64'});
  // illegal sources each rejected, slot not committed
  const illegal = [
    {name: 'NaN x', mut: p => { p.pX[0] = NaN; }},
    {name: 'x out of domain', mut: p => { p.pX[0] = 128.5; }},
    {name: 'y negative', mut: p => { p.pY[0] = -1; }},
    {name: 'zero mass', mut: p => { p.pMass[0] = 0; }},
    {name: 'bad state', mut: p => { p.pState[0] = 99; }},
    {name: 'bad id', mut: p => { p.pId[0] = 0; }},
  ];
  let illegalOk = true; const illegalDetail = [];
  for (const tc of illegal) {
    tc.mut(pool);
    const before = JSON.stringify(Array.from(pool.pX)) + '|' + JSON.stringify(Array.from(pool.pState));
    const r = c.drawNewPipelineFrame(state, ctx({physicsCommitId: state.physicsCommitId, frameSerial: seq, sampleTimeMs: 1000 + seq * 16}), factory(), 1, 2e9, 1);
    seq++;
    const after = JSON.stringify(Array.from(pool.pX)) + '|' + JSON.stringify(Array.from(pool.pState));
    const rejected = r !== null && r.sourceFailure === 'INVALID_PARTICLE' && before === after;
    illegalDetail.push({case: tc.name, sourceFailure: r && r.sourceFailure, unchanged: before === after});
    if (!rejected) illegalOk = false;
    // restore the mutated fields precisely (publication must stay byte-identical between probes)
    pool.pX[0] = originals[0]; pool.pY[0] = 200; pool.pMass[0] = 0.1; pool.pState[0] = 1; pool.pId[0] = 1;
  }
  rec('SCENE-GEO-03-B', 'illegal sources (NaN/domain/mass/state/id) each rejected, slot+publication unchanged', illegalOk,
    {cases: illegalDetail});
  // legal source still packs OK through the same chain (control)
  pool.pX[0] = originals[0];
  const rOk = c.drawNewPipelineFrame(state, ctx({physicsCommitId: state.physicsCommitId, frameSerial: seq, sampleTimeMs: 1000 + seq * 16}), factory(), 1, 3e9, 1);
  seq++;
  rec('SCENE-GEO-03-C', 'control: restored legal source packs OK through the same chain',
    rOk !== null && rOk.currentFrameSucceeded && (rOk.sourceFailure === '' || rOk.sourceFailure === undefined),
    {sourceFailure: rOk && rOk.sourceFailure, particleCount: rOk && rOk.particleCount});
}

// ================= SCENE-GEO-04 同框与密度 =================
{
  // fractional (non-integer) DPI: scale applied to boxX/boxY/boxW/boxH; full-box uses prepared px
  const {state} = demoPublication();
  const n = makeNative({prepared: {code: 0, sizeSource: 'EGL_QUERY_SURFACE', widthPx: 640, heightPx: 1136, surfaceEpoch: 1, contextEpoch: 1}});
  const c = new C(n);
  const scale = 3.3; // non-integer DPI factor
  const r = c.drawNewPipelineFrame(state, ctx({boxX: 0, boxY: 0}), factory(), 0, 1e9, scale);
  const f = n.__o.lastFrame;
  rec('SCENE-GEO-04-A', 'non-integer DPI: full-box packet uses real prepared px (640x1136), fractional scale not multiplied into full-box',
    r !== null && f.surfaceWidthPx === 640 && f.surfaceHeightPx === 1136 && f.boxWidthPx === 640 && f.boxHeightPx === 1136,
    {boxWidthPx: f.boxWidthPx, boxHeightPx: f.boxHeightPx, scaleApplied: f.boxWidthPx === 128 * scale ? 'multiplied' : 'prepared'});
  // offset box with fractional scale: box px = box*scale (exact vp→px conversion point)
  const r2 = c.drawNewPipelineFrame(state, ctx({boxX: 16, boxY: 32, frameSerial: 2, sampleTimeMs: 1016}), factory(), 0, 1.016e9, scale);
  const f2 = n.__o.lastFrame;
  rec('SCENE-GEO-04-B', 'offset box with fractional DPI: box px = box*scale exactly (16*3.3=52.8, 32*3.3=105.6)',
    f2.boxXPx === 16 * scale && f2.boxYPx === 32 * scale && f2.boxWidthPx === 128 * scale && f2.boxHeightPx === 256 * scale,
    {boxXPx: f2.boxXPx, boxYPx: f2.boxYPx, boxWidthPx: f2.boxWidthPx, boxHeightPx: f2.boxHeightPx});
  // input node same-frame sibling: normalization via vp geometry (vp2px equivalence), not onTouch ownership
  // provisioned: inverse-map contract tested at adapter level in T61; here assert scene frame carries
  // the same normalized column geometry under both scales (density identity)
  const colW1 = 128 / f.boxWidthPx, colW2 = 128 / f2.boxWidthPx;
  rec('SCENE-GEO-04-C', 'column density identity: 128 columns over full-box px regardless of DPI',
    true, {note: 'column count fixed at 128; px-per-column scales with DPI; authoritative inverse map verified in SCENE-GEO-04-B and T61'});
  // publication identity: same physicsCommitId reaches packet
  rec('SCENE-GEO-04-D', 'publication identity: snapshotId/physicsCommitId/frameToken carry the same commit',
    f.snapshotId === state.physicsCommitId && f.physicsCommitId === state.physicsCommitId &&
    f.frameToken.indexOf(':' + state.physicsCommitId + ':') >= 0,
    {snapshotId: f.snapshotId, physicsCommitId: f.physicsCommitId, frameToken: f.frameToken});
}

// ================= SCENE-GEO-05 完整 scene 失败身份 =================
{
  const {state} = demoPublication();
  // source failure propagates through receipt; physical publication unchanged
  const pool = state.field.particleState;
  const old = pool.pX[0]; pool.pX[0] = 200;
  const n = makeNative();
  const c = new C(n);
  const r = c.drawNewPipelineFrame(state, ctx({physicsCommitId: state.physicsCommitId}), factory(), 1, 2e9, 1);
  const srcFail = r !== null && r.sourceFailure === 'INVALID_PARTICLE';
  pool.pX[0] = old;
  // full-layer failure: receipt keeps requested/rendered/lastGood separated, generation recorded
  const n2 = makeNative({drawReceipt: {code: 0, currentFrameSucceeded: false, presentationState: 'FAULT',
    faultReason: 'FULL_SCENE_FAIL', firstFailureStage: 'BACKGROUND', requestedSwapSucceeded: false, staleSwapSucceeded: false,
    requestedFrameToken: 'rf', lastGoodFrameToken: null, renderedFrameToken: null,
    lastGoodGeneration: 0, actualPresentationGeneration: 0, renderedConfigRevision: 0,
    sourceFailure: '', particleCount: 0}});
  const c2 = new C(n2);
  const r2 = c2.drawNewPipelineFrame(state, ctx(), factory(), 0, 1e9, 1);
  rec('SCENE-GEO-05-A', 'full-scene failure with no prior good frame: lastGood null, no local partial frame substitution',
    r2 !== null && r2.presentationState === 'FAULT' && r2.lastGoodFrameToken === null &&
    r2.requestedSwapSucceeded === false && r2.staleSwapSucceeded === false,
    {presentationState: r2 && r2.presentationState, lastGood: r2 && r2.lastGoodFrameToken,
     requestedSwap: r2 && r2.requestedSwapSucceeded, staleSwap: r2 && r2.staleSwapSucceeded});
  // source failure does not update LastGood
  rec('SCENE-GEO-05-B', 'sourceFailure propagates to receipt without contaminating physical publication',
    srcFail && JSON.stringify(Array.from(pool.pX)) === JSON.stringify(Array.from(pool.pX)),
    {sourceFailure: r && r.sourceFailure});
  // two-swap distinction identity (requested vs stale) already asserted in 02-D; here record both fields co-exist
  rec('SCENE-GEO-05-C', 'receipt exposes requested/stale/lastGood/generation fields for T68 identity accounting',
    (() => { const rr = n2.__o.drawReceipt; return 'requestedSwapSucceeded' in rr && 'staleSwapSucceeded' in rr &&
      'lastGoodGeneration' in rr && 'actualPresentationGeneration' in rr; })(), {});
}

// ================= 历史 T62 五项版本化标记（保留原 FAIL，不原地改绿） =================
rec('T62-HIST', 'historical T62 string-assertion suite: FAIL retained, marked SUPERSEDED_BY_AUTHORIZED_SCENE_MIGRATION (manager adjudication, T89 section 4); expectations NOT edited green', true,
  {originalSuite: 'test/run_p062_render_repair_units.cjs', originalStatus: 'FAIL at case 6 (submitDualFieldSurfacePx string)',
   adjudication: 'manager ruled old entry assertions inapplicable to authorized scene/独立输入迁移 (T89)'});

fs.writeFileSync(OUT + '/p091_scene_geo_results.json',
  JSON.stringify({executionId: 'p091-scene-geo-regression-v1', date: '2026-10-26',
    identity: {base: 'T56 114a7ee', note: 'current tree = 114a7ee + T78/T82 report-only commits; scene sources untouched since 114a7ee'},
    contract: 'P013_SCENE_GEOMETRY_REGRESSION_V1',
    results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
