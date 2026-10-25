'use strict';
// T40 independent verification of the T39 O-2 implementation (commit 2f864f0).
// Tester-side: probes are written from the architect §9.2/§9.3 contract text and
// the manager ruling (healthy count == sourceEpoch; independence means an
// independent successful-receipt accumulation source, never a copy of epoch).
// No device, no judgement change, no protocol edit. The delivered checker is
// consumed read-only by the python driver.
const fs = require('fs'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, f);
const { WaterSandField, defaultWaterSandParams } = require('../entry/src/main/ets/physics/WaterSandField.ets');
const { P011NaturalObservationProbe } = require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
const { P011ExternalAdmissions } = require('../entry/src/main/ets/physics/P011ExternalAdmissions.ets');

const results = [];
const writerContexts = {};
function test(name, fn) {
  try { fn(); results.push({ name, status: 'PASS' }); }
  catch (e) { results.push({ name, status: 'FAIL', error: e.stack }); }
}
function field() {
  const f = new WaterSandField(128, 512), p = defaultWaterSandParams();
  p.heavyDiffusion = 0; p.lightDiffusion = 0; p.angleDeg = 4; f.configure(p);
  f.initialize({ waterRegions: [{ startCol: 0, endColInclusive: 127, bottom: 300, height: 160 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: 127, bottom: 0, height: 100 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: 127, bottom: 100, height: 100 }],
    heavySuspendedRegions: [], lightSuspendedRegions: [] });
  // Bind the clock at run start, before any admitted source or substep.
  f.setEndpointSampling({ clockId: f.getP011RunId() + ':startup-elapsed-v1', nowElapsedMs: () => 1000 },
    { occurrenceId: 'admission-test', commandSequence: f.p011InputSequence, commandPhaseId: f.commandPhaseId, segmentId: 's' });
  return f;
}
function request(sequence = 1) {
  const h = new Float64Array(128), l = new Float64Array(128);
  h[60] = .1; h[61] = .1; l[60] = .1; l[62] = .1;
  return { perColumnHeavy: h, perColumnLight: l, sourceId: 7, sequence, inputTime: 1000 + sequence, x: 500, y: 240, seed: 17 };
}
function admit(f, seq = 1) {
  const r = f.injectGestureSand(request(seq));
  assert.ok(r && r.rejectedReason === null && r.admittedH + r.admittedL > 0); return r;
}
function context(f) {
  const records = [];
  f.setNaturalObservationProbe(new P011NaturalObservationProbe(128, r => records.push(r)));
  assert.equal(f.step(1 / 60).ok, true);
  return records.at(-1).context;
}
function ev(f) { return f.getExternalAdmissionEvidence(); }

// ---------- Group A: independent accumulation source ----------
test('A1 count recomputed from raw successful records agrees with count/epoch/masses', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  const r1 = admit(f), r2 = admit(f, 2), r3 = admit(f, 3);
  const e = ev(f);
  // Independent recomputation straight from the exported raw records.
  const ids = new Set(raw.map(r => r.admissionTransactionId));
  let sumH = 0, sumL = 0;
  for (const r of raw) { assert.ok(r.admittedH + r.admittedL > 0); sumH += r.admittedH; sumL += r.admittedL; }
  assert.equal(ids.size, raw.length);                       // no duplicate transaction ids
  assert.equal(raw.length, 3);
  assert.equal(e.count, 3); assert.equal(f.sourceEpoch, 3);
  assert.equal(e.count, ids.size);                          // count == distinct successful records
  assert.equal(e.observedTransactions, 3);
  assert.ok(Math.abs(sumH - (r1.admittedH + r2.admittedH + r3.admittedH)) < 1e-12);
  assert.ok(Math.abs(sumL - (r1.admittedL + r2.admittedL + r3.admittedL)) < 1e-12);
  assert.ok(Math.abs(e.admittedH - f.sourceInjectedHeavy) < 1e-12);
  assert.ok(Math.abs(e.admittedL - f.sourceInjectedLight) < 1e-12);
  assert.equal(e.rawHistoryStatus, 'STREAMED');
  for (const r of raw) {
    assert.equal(r.runId, f.getP011RunId()); assert.equal(r.sourceId, 7);
    assert.ok(r.sequence >= 1 && r.sequence <= 3);
    assert.equal(r.cumulativeCount, r.admissionTransactionId);
  }
});
test('A2 count is not derived from epoch: corrupting epoch leaves count and writer value intact', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  admit(f); admit(f, 2);
  f.sourceEpoch = 99;                                        // simulate epoch corruption
  assert.equal(ev(f).count, 2);
  const c = context(f);
  assert.equal(c.externalAdmissionCount, 2);
  assert.equal(c.sourceEpoch, 99);                           // both serialized independently
  writerContexts.epochFault = c;
});
test('A3 epoch is not derived from count: corrupting count leaves epoch intact', () => {
  const f = field(); admit(f); admit(f, 2);
  f.externalAdmissions.evidence.count = 0;                   // simulate lost update
  assert.equal(f.sourceEpoch, 2);
  const c = context(f);
  assert.equal(c.externalAdmissionCount, 0);
  assert.equal(c.sourceEpoch, 2);
  writerContexts.lost = c;
  const g = field(); admit(g);
  g.externalAdmissions.evidence.count = 2;                   // simulate duplicate publication
  writerContexts.duplicate = context(g);
  assert.equal(writerContexts.duplicate.sourceEpoch, 1);
  assert.equal(writerContexts.duplicate.externalAdmissionCount, 2);
});
test('A4 consumer tampering with an exported record copy cannot change canonical evidence', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  admit(f);
  raw[0].cumulativeCount = 999; raw[0].admittedH = -5;
  assert.equal(ev(f).count, 1);
  assert.equal(ev(f).admittedH, f.sourceInjectedHeavy);
  const snap = ev(f);
  snap.count = 42;                                           // snapshot copy is detached too
  assert.equal(ev(f).count, 1);
});
test('A5 same request identity replayed is two distinct successful transactions', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  admit(f); admit(f);                                        // identical request content
  assert.equal(ev(f).count, 2); assert.equal(f.sourceEpoch, 2);
  assert.deepEqual(raw.map(r => r.admissionTransactionId), [1, 2]);
  f.externalAdmissions.exportLast();                         // replay does not recount
  assert.equal(raw.length, 3);
  assert.equal(raw[1].admissionTransactionId, raw[2].admissionTransactionId);
  assert.equal(ev(f).count, 2);
});

// ---------- Group B: healthy equality and no-increment cases ----------
test('B1 two-phase multi-column transaction counts once; fractional mass still one transaction', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  const r = admit(f);
  assert.equal(ev(f).count, 1); assert.equal(f.sourceEpoch, 1);
  assert.equal(raw.length, 1);
  assert.ok(r.admittedH > 0 && r.admittedL > 0);             // both phases, several columns
  assert.ok(Math.abs(ev(f).admittedH - r.admittedH) < 1e-12);
});
test('B2 zero request is rejected before any record exists', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  const zero = request(); zero.perColumnHeavy.fill(0); zero.perColumnLight.fill(0);
  assert.equal(f.injectGestureSand(zero), null);
  assert.equal(ev(f).count, 0); assert.equal(raw.length, 0);
  assert.equal(ev(f).lastRecord, null);
});
test('B3 capacity rejection and lambda exhaustion do not count', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  const before = f.snapshot();
  f.gestureInjectionAudit = () => false;                     // always fail -> lambda exhausts
  const rej = f.injectGestureSand(request());
  assert.equal(rej.rejectedReason, 'CAPACITY_BLOCKED');
  assert.equal(ev(f).count, 0); assert.equal(f.sourceEpoch, 0);
  assert.equal(raw.length, 0);
  assert.deepEqual(f.snapshot().waterTop, before.waterTop);
  assert.deepEqual(f.snapshot().suspH, before.suspH);
  assert.deepEqual(f.snapshot().suspL, before.suspL);
});
test('B4 no-water rejection after a success does not change the count', () => {
  const f = field(), raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  admit(f);
  f.waterTop.fill(200);
  const rej = f.injectGestureSand(request());
  assert.equal(rej.rejectedReason, 'NO_WATER');
  assert.equal(ev(f).count, 1); assert.equal(f.sourceEpoch, 1);
  assert.equal(raw.length, 1);
});
test('B5 failed lambda trial followed by success counts only the final receipt', () => {
  const f = field();
  const audit = f.gestureInjectionAudit.bind(f);
  let calls = 0;
  f.gestureInjectionAudit = () => ++calls === 1 ? false : audit();
  const r = admit(f);
  assert.ok(calls > 1);
  assert.equal(ev(f).count, 1);
  assert.ok(Math.abs(ev(f).admittedH - r.admittedH) < 1e-12);
  assert.ok(Math.abs(f.sourceInjectedHeavy - r.admittedH) < 1e-12);
});
test('B6 shake, step, invert, snapshot reads and export do not count external sources', () => {
  const f = field(); admit(f);
  f.setShakeInput(1);
  assert.equal(f.step(1 / 60).ok, true);
  f.invert180(); f.snapshot(); f.snapshot(); f.externalAdmissions.exportLast();
  assert.equal(ev(f).count, 1); assert.equal(f.sourceEpoch, 1);
});
test('B7 healthy writer context passes the delivered checker without mismatch', () => {
  const f = field(); admit(f);
  writerContexts.healthy = context(f);
  assert.equal(writerContexts.healthy.externalAdmissionCount, 1);
  assert.equal(writerContexts.healthy.sourceEpoch, 1);
});

// ---------- Group C: reset / restore boundaries ----------
test('C1 scene reset clears count, masses, epoch and record identity as a group', () => {
  const f = field(); admit(f);
  const run = f.getP011RunId();
  f.reset();
  assert.notEqual(f.getP011RunId(), run);
  assert.equal(f.sourceEpoch, 0); assert.equal(f.sourceInjectedHeavy, 0);
  assert.equal(ev(f).count, 0); assert.equal(ev(f).lastRecord, null);
});
test('C2 restore brings initial masses, source amounts, epoch and receipt count back as a group', () => {
  const f = field(); admit(f); admit(f, 2);
  const snap = f.snapshot();
  const g = field(); admit(g);
  g.restoreFromSnapshot(snap);
  assert.equal(g.sourceEpoch, 2); assert.equal(ev(g).count, 2);
  assert.equal(g.sourceInjectedHeavy, f.sourceInjectedHeavy);
  assert.equal(g.initialHeavy, f.initialHeavy);
  assert.equal(ev(g).rawHistoryStatus, 'SUMMARY_ONLY');      // checkpoint is not a raw stream
  admit(g, 3);
  assert.equal(ev(g).count, 3);                              // accumulation continues after restore
});
test('C3 restore copies the evidence; tampering with the snapshot afterwards cannot leak in', () => {
  const f = field(); admit(f);
  const snap = f.snapshot();
  const g = field();
  g.restoreFromSnapshot(snap);
  // The snapshot is the source of truth at restore time; afterwards the restored
  // field must own a detached copy, not an alias of the snapshot object.
  snap.externalSourceState.admissions.lastRecord.admittedH = 900;
  snap.externalSourceState.admissions.count = 77;
  snap.externalSourceState.admissions.admittedH = 900;
  assert.notEqual(ev(g).lastRecord.admittedH, 900);
  assert.equal(ev(g).count, 1);
  assert.ok(Math.abs(ev(g).lastRecord.admittedH - f.sourceInjectedHeavy) < 1e-12);
  assert.ok(Math.abs(ev(g).admittedH - f.sourceInjectedHeavy) < 1e-12);
});
test('C4 legacy restore without source state keeps count null until an explicit reset', () => {
  const f = field(); admit(f);
  const snap = f.snapshot();
  delete snap.externalSourceState;
  const g = field();
  g.restoreFromSnapshot(snap);
  assert.equal(ev(g).count, null);
  admit(g);                                                  // later admission cannot backfill
  assert.equal(ev(g).count, null);
  writerContexts.legacy = context(g);
  assert.equal(writerContexts.legacy.externalAdmissionCount, null);
  g.reset();
  assert.equal(ev(g).count, 0);
});
test('C5 invalid source checkpoint is rejected before any field mutation', () => {
  const f = field(), g = field();
  admit(f);
  const snap = f.snapshot();
  snap.externalSourceState.sourceEpoch = 42;                 // epoch inconsistent with count
  snap.waterTop.fill(1);
  const before = g.snapshot();
  assert.throws(() => g.restoreFromSnapshot(snap), /MISMATCH/);
  assert.deepEqual(g.snapshot().waterTop, before.waterTop);
  assert.equal(ev(g).count, 0); assert.equal(g.sourceEpoch, 0);
});
test('C6 null count with non-null masses is an invalid snapshot', () => {
  const ledger = new P011ExternalAdmissions();
  const e = ledger.snapshot();
  e.count = null; e.admittedH = 5; e.admittedL = 0;
  const src = { revision: 'p011-external-source-v1', initialWater: 1, initialHeavy: 1, initialLight: 1,
    sourceInjectedH: 5, sourceInjectedL: 0, sourceEpoch: 0, admissions: e };
  assert.throws(() => ledger.validateSource(src), /INVALID_EXTERNAL_SOURCE_SNAPSHOT/);
});

// ---------- Group D: disclosure status and atomicity ----------
test('D1 raw history status discloses stream availability', () => {
  const f = field();
  assert.equal(ev(f).rawHistoryStatus, 'SUMMARY_ONLY');      // no sink: summary only
  const raw = [];
  f.setExternalAdmissionEventSink(r => raw.push(r));
  admit(f);
  assert.equal(ev(f).rawHistoryStatus, 'STREAMED');
  assert.equal(raw.length, 1);
});
test('D2 export failure is disclosed without undoing or double counting', () => {
  const f = field();
  f.setExternalAdmissionEventSink(() => { throw new Error('EXPORT_FAILED'); });
  admit(f);
  const e = ev(f);
  assert.equal(e.count, 1); assert.equal(f.sourceEpoch, 1);
  assert.equal(e.exportFailures, 1);
  assert.equal(e.rawHistoryStatus, 'INCOMPLETE');
});
test('D3 a prepared record is single-use; duplicate commit is rejected', () => {
  const ledger = new P011ExternalAdmissions();
  const r = ledger.prepare('r', 1, 1, 1, .1, 0);
  ledger.commit(r);
  assert.throws(() => ledger.commit(r), /DUPLICATE/);
  assert.equal(ledger.count, 1);
});
test('D4 zero-mass prepared record is invalid', () => {
  const ledger = new P011ExternalAdmissions();
  assert.throws(() => ledger.prepare('r', 1, 1, 1, 0, 0), /INVALID_EXTERNAL_ADMISSION_RECORD/);
  assert.throws(() => ledger.prepare('r', 1, 1, 1, -1, 2), /INVALID_EXTERNAL_ADMISSION_RECORD/);
  assert.throws(() => ledger.prepare('r', 1, 1, 1, NaN, 1), /INVALID_EXTERNAL_ADMISSION_RECORD/);
});
test('D5 source epoch overflow is rejected atomically before publication', () => {
  const f = field();
  f.sourceEpoch = Number.MAX_SAFE_INTEGER;
  const before = f.snapshot();
  assert.throws(() => f.injectGestureSand(request()), /OVERFLOW/);
  assert.deepEqual(f.snapshot().waterTop, before.waterTop);
  assert.deepEqual(f.snapshot().suspH, before.suspH);
  assert.equal(ev(f).count, 0); assert.equal(f.sourceEpoch, Number.MAX_SAFE_INTEGER);
});
test('D6 transport corruption of a healthy context is detectable downstream', () => {
  // The writer itself never emits these; they model serialization/transport damage.
  writerContexts.transportCorrupt = JSON.parse(JSON.stringify(writerContexts.healthy));
  writerContexts.transportCorrupt.externalAdmissionCount += 1;
});

const failed = results.filter(r => r.status === 'FAIL');
console.log(JSON.stringify({
  suite: 't40_external_admission_independent',
  passed: results.length - failed.length,
  failed: failed.length,
  results,
  writerContexts
}, null, 2));
if (failed.length) process.exitCode = 1;
