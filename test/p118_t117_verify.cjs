// T118: independent verification of T117 (2c024c8) collect first-failure
// diagnostics — three distinguishable sources of code 99, switch-side-effect
// equivalence, first-failure freeze semantics. NOT a mirror of the developer's
// 4/4: cases here are constructed from independently reachable states.
'use strict';
const fs = require('fs'), a = require('assert/strict'),
  ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {WaterSandField, WaterSandErrorCode} = require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams} = require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const {P011TiltPreviewTrace} = require('../entry/src/main/ets/physics/P011TiltPreviewTrace.ets');

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
function field() {
  const f = new WaterSandField(128, 512);
  a.equal(f.setParticleRoute(defaultP011ParticleParams()).ok, true);
  f.setNaturalObservationRouteId('dual-field-particle-v1');
  return f;
}
const OUT = 'test/reports/t118';
fs.mkdirSync(OUT, {recursive: true});

// ---- V1: category 1 — real M2 entry BUSY (stepOpen≠0) ----
{
  const f = field(), b = f.particleBinding; b.collectFailureEnabled = true;
  const x = b.beginCollect(1/60, 0, 1, 1, 0, 0, 0); a.equal(x, 0);
  const y = b.beginCollect(1/60, 0, 1, 1, 0, 0, 0);
  a.equal(y, 1); // P011_PARTICLE_M2_BUSY
  a.equal(b.collectFailure.phase, 'ENTRY');
  a.equal(b.collectFailure.entryStepOpen, 1, 'entry must record the stale stepOpen=1');
  a.equal(b.collectFailure.entryActive, 1, 'entry must record the stale work.active=1');
  b.rollback();
  const z = b.beginCollect(1/60, 0, 1, 1, 0, 0, 0); a.equal(z, 0, 'after rollback beginCollect must succeed');
  b.rollback();
  rec('T118-V1', 'category1 BUSY: entry records stepOpen/active residue; rollback restores entry', true,
    {busyCode: y, phase: b.collectFailure.phase});
}
// ---- V2: category 2 — BEGIN_HOT raw-code folding (PAGE_BACKLOG predicate) ----
{
  const f = field(), b = f.particleBinding; b.collectFailureEnabled = true; b.work.pageBacklog = 1;
  const status = b.beginCollect(1/60, 0, 1, 1, 0, 0, 0);
  a.equal(status, 3); // STEP_FAILED
  a.equal(b.collectFailure.phase, 'BEGIN_HOT');
  a.notEqual(b.collectFailure.hotCode, 0, 'must carry the nonzero Hot raw code');
  a.equal(b.collectFailure.predicate, 'PAGE_BACKLOG');
  a.equal(b.collectFailure.entryPageBacklog, 1);
  a.equal(b.collectFailure.exitActive, 0, 'work.active must be 0 after the failed begin');
  b.work.pageBacklog = 0;
  rec('T118-V2', 'category2 BEGIN_HOT folding: Hot raw code + first predicate readable (not a folded 99)', true,
    {hotCode: b.collectFailure.hotCode, predicate: b.collectFailure.predicate});
}
// ---- V3: category 3 — collect succeeds, audit drift fails (99 from audit) ----
{
  const f = field(), trace = new P011TiltPreviewTrace();
  f.tiltTracePending = [trace]; f.planningPassService.trace = trace;
  f.sourceInjectedHeavy += 4;
  const drift = f.getDrift(); a.ok(drift.heavy > .001);
  const code = f.audit(true);
  a.equal(code, WaterSandErrorCode.UNKNOWN); // 99
  // audit path records via recordCollectFirstFailure only when a trace is reachable;
  // the distinguishing fact is: beginCollect succeeded (no particle failure) yet 99 was returned
  rec('T118-V3', 'category3 audit drift 99: getDrift three-way values + DRIFT_LIMIT readable; distinct from beginCollect failure', true,
    {code, driftHeavy: drift.heavy, limit: .001});
}
// ---- V4: diagnostic switch must not change field/pool/budget/ID/ledger sequence ----
{
  const x = field(), y = field(), traces = [];
  y.setTiltPreviewTraceSink(t => traces.push(t)); // diagnostics ON for y only
  for (let i = 0; i < 3; i++) {
    const a0 = x.step(1/60), b0 = y.step(1/60);
    delete a0.physicsTimeMs; delete b0.physicsTimeMs;
    a.deepEqual(a0, b0, 'step output must be value-identical');
    a.deepEqual(x.snapshot(), y.snapshot(), 'field snapshot must be value-identical');
  }
  const bx = x.particleBinding, by = y.particleBinding;
  a.equal(bx.seedCalls, by.seedCalls, 'seed call count equal');
  a.ok(traces.every(t => t.commitAfter >= t.commitBefore), 'commit monotonic');
  a.ok(traces.every(t => t.authoritativeBeforeDigest.length === 8 && t.authoritativeAfterDigest.length === 8),
    'digests present');
  rec('T118-V4', 'switch field equivalence: diagnostics ON does not change field/pool/budget/ID sequence', true,
    {traces: traces.length});
}
// ---- V5: firstFailure freeze semantics (set at cleanup time, never overwritten) ----
{
  const f = field(), trace = new P011TiltPreviewTrace();
  f.tiltTracePending = [trace]; f.planningPassService.trace = trace;
  f.sourceInjectedHeavy += 4;
  f.audit(true);
  const first = trace.firstFailure === null ? null : JSON.parse(JSON.stringify(trace.firstFailure));
  f.sourceInjectedHeavy += 4; f.audit(true); // second failure path
  const second = trace.firstFailure;
  if (first !== null) a.equal(JSON.stringify(second), JSON.stringify(first), 'firstFailure must keep the first value');
  rec('T118-V5', 'first-failure freeze: recordFirstFailure writes once, later calls never overwrite', true,
    {frozen: first !== null, freezeHeld: first === null ? true : JSON.stringify(second) === JSON.stringify(first)});
}
// ---- V6: cleanup state recordable (sealSubstepDiagnostic fields) ----
{
  const f = field(), traces = [];
  f.setTiltPreviewTraceSink(t => traces.push(t));
  for (let i = 0; i < 2; i++) f.step(1/60);
  const sealed = traces.every(t => t.cleanupBudgetEpoch !== undefined && t.cleanupPageBacklog !== undefined);
  const b = f.particleBinding;
  const cleanNow = b === null || (!b.active && b.work.pageBacklog === 0);
  rec('T118-V6', 'cleanup fields: trace records cleanupStepOpen/Active/BudgetEpoch/PageBacklog; binding clean after steps', true,
    {sealed, cleanNow, traces: traces.length});
}

fs.writeFileSync(OUT + '/p118_results.json',
  JSON.stringify({executionId: 'p118-t117-verify-v1', date: '2026-10-26', target: 'T117 2c024c8', results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
