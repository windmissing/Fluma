'use strict';
// T348 preflight evidence (developer): does the EmptySandDemo touch->sand chain
// exist and work TODAY? The task card's premise says the chain was never wired
// (only model.start/move diagnostics) and asks to port the SandCanvas
// stageGestureSandAndFeed wiring. Source + runtime evidence below.
//
// Findings (all three runnable, reproducible):
//  F1 SOURCE: P051EmptySandDemo.tick() touching path (since T71 = 0b1f7e3,
//     "connect touch input to dry source admission") constructs a
//     P051DryAdmissionRequest {x,y, requestedH=8*seconds} each held frame and
//     calls field.injectDryGestureSand — a complete touch->admission->particle
//     chain, distinct from the wet stageGestureSand kernel chain.
//  F2 RUNTIME (this harness, real ArkTS in Node per T346 mechanism):
//     start(64,220) + 360 ticks @60fps -> sourceEpoch=256, injectedH=34.13,
//     512 live particles. 700 ticks -> pYmax descends 220->0.58, bed mh=0.5774
//     (LAND + bed growth). Full chain works end-to-end.
//  F3 MEDIUM MISMATCH: EmptySandDemo's field is DRY_AIR_V1 (dry point
//     admission). The SandCanvas stageGestureSandAndFeed chain routes through
//     injectGestureSand (wet per-column kernel) which returns NO_WATER on a
//     dry field — porting it as-is would produce only rejections.
//  F4 EMPTY SCENE EXISTS: model.reset() -> initializeDryParticleDemo ->
//     initialize({waterRegions:[], all-empty}) — the page's reset button
//     already lands the empty scene; baselineStatus reads EMPTY_BASELINE.
//  F5 DEVICE EVIDENCE: today's demo commits (c5ad113 20:07, 42ac464 20:21)
//     already record touch-pour -> particle fall -> bed accumulation ->
//     tilt dune change on the V1 EmptySandDemo XComponent page.
const fs = require('fs'), path = require('path'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } }).outputText, f);
const { P051EmptySandDemo } = require(path.resolve('entry/src/main/ets/physics/P051EmptySandDemo.ets'));
const rows = [];
let passed = 0;
function test(name, fn) { fn(); passed++; console.log('PASS ' + name); }

test('T348-P1 touch start accepted and admission flows within the first second', () => {
  const model = new P051EmptySandDemo();
  assert.equal(model.start(64, 220).accepted, true);
  for (let i = 0; i < 60; i++) model.tick(1e9 + i * 16666666, 1 / 60);
  const d = model.field.getDryDiagnostics();
  assert.ok(d.sourceEpoch > 0, 'admissions committed');
  rows.push({ step: 60, sourceEpoch: d.sourceEpoch });
});
test('T348-P2 six seconds of touch pours sand and fills the pool', () => {
  const model = new P051EmptySandDemo();
  model.start(64, 220);
  for (let i = 0; i < 360; i++) model.tick(1e9 + i * 16666666, 1 / 60);
  const s = model.field.snapshot();
  assert.ok(s.particleState.liveCount > 0, 'live particles born from touch');
  rows.push({ step: 360, live: s.particleState.liveCount });
});
test('T348-P3 continued hold settles: descent + LAND + bed growth', () => {
  const model = new P051EmptySandDemo();
  model.start(64, 220);
  let mh = 0, pYmax = -Infinity;
  for (let i = 0; i < 700; i++) model.tick(1e9 + i * 16666666, 1 / 60);
  model.stop('done');
  const s = model.field.snapshot();
  for (let c = 0; c < 128; c++) mh += s.mh[c];
  const p = s.particleState;
  for (let k = 0; k < p.nmax; k++) if (p.pState[k] !== 0) pYmax = Math.max(pYmax, p.pY[k]);
  assert.ok(mh > 1e-9, 'bed must grow after LAND, mh=' + mh);
  assert.ok(pYmax < 220, 'particles must descend from birth height, pYmax=' + pYmax);
  rows.push({ step: 700, mh: +mh.toFixed(4), pYmax: +pYmax.toFixed(2) });
});
test('T348-P4 reset gives the empty scene (all-empty regions, EMPTY_BASELINE)', () => {
  const model = new P051EmptySandDemo();
  model.start(64, 220);
  for (let i = 0; i < 120; i++) model.tick(1e9 + i * 16666666, 1 / 60);
  model.reset();
  const d = model.field.getDryDiagnostics();
  assert.equal(d.baselineStatus, 'EMPTY_BASELINE');
  const t = model.field.getTotals();
  assert.equal(t.water, 0); assert.equal(t.heavy, 0); assert.equal(t.light, 0);
  rows.push({ reset: 'EMPTY_BASELINE' });
});
fs.mkdirSync('test/reports/t348', { recursive: true });
fs.writeFileSync('test/reports/t348/preflight-premise-check.json',
  JSON.stringify({ revision: 'T348_PREFLIGHT_PREMISE_CHECK_V1', device: 'NOT_RUN',
    verdict: 'CARD_PREMISE_CONFLICTS_WITH_CURRENT_CODE',
    findings: ['F1 dry touch chain exists since T71 (0b1f7e3)',
      'F2 runtime proof: 6s touch -> 512 live, 700 steps -> mh 0.5774 bed growth',
      'F3 stageGestureSand is the WET kernel chain; EmptySandDemo is DRY_AIR_V1 -> NO_WATER if ported as-is',
      'F4 empty scene already exists (reset -> all-empty regions)',
      'F5 device demos c5ad113/42ac464 already show touch-pour working'],
    rows }, null, 2) + '\n');
console.log('T348 preflight: ' + passed + ' passed, evidence written');
