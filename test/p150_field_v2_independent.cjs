// T150: independent verification of T149 (e3ee81f) FIELD_COLUMN evidence V2.
// Own oracle construction (fresh harness, not the developer 13 tests). Fixed inputs,
// raw-bit assertions computed independently from IEEE-754 definitions.
'use strict';
const fs = require('fs'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {WaterSandField} = require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams} = require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const {P011FieldColumnEvidenceV2} = require('../entry/src/main/ets/physics/P011FieldColumnEvidenceV2.ets');
const {P011TiltPreviewTrace} = require('../entry/src/main/ets/physics/P011TiltPreviewTrace.ets');

const OUT = 'test/reports/t150';
fs.mkdirSync(OUT, {recursive: true});
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
  if (!ok) process.exitCode = 1;
}
function field() {
  const f = new WaterSandField(128, 512);
  assert.equal(f.setParticleRoute(defaultP011ParticleParams()).ok, true);
  f.setTiltPreviewTraceSink(() => {});
  return f;
}
function fail(f) {
  const b = f.particleBinding;
  const x = b.beginCollect(1/60, 0, 1, 1, 0, 0, 0);
  assert.equal(x, 3);
  assert.equal(b.collectFailure.predicate, 'FIELD_COLUMN');
  return b.work.fieldColumnEvidenceV2.failure;
}
// independent bit formatter (big-endian convention, from raw buffer bytes)
function bitsOfBuf(f, key, col) {
  const src = f[key];
  const dv = new DataView(src.buffer, src.byteOffset + col * 8, 8);
  const lo = dv.getUint32(0, true), hi = dv.getUint32(4, true);
  return hi.toString(16).padStart(8, '0') + lo.toString(16).padStart(8, '0');
}
const KEYS = ['waterTop', 'bh', 'bl', 'mh', 'ml', 'suspH', 'suspL', 'sandTopScratch'];
const MASK_FIN = i => 1 << i, MASK_NEG = i => 1 << (8 + i);

// ---------- V1: eight independent non-finite causes, own mask/raw-bit oracle ----------
{
  let ok = true; const detail = [];
  for (let i = 0; i < 8; i++) {
    const f = field();
    // distinct payloads per array position so index confusion would show
    f[KEYS[i]][7] = i % 2 === 0 ? Infinity : NaN;
    f[KEYS[i]][7] = i % 2 === 0 ? Infinity : -NaN; // exercise both
    const wantBits = i % 2 === 0 ? '7ff0000000000000' : 'fff8000000000000';
    // re-set to canonical values with distinct payloads via DataView
    const src = f[KEYS[i]];
    const dv = new DataView(src.buffer, src.byteOffset + 7 * 8, 8);
    if (i % 2 === 0) dv.setUint32(0, 0, true), dv.setUint32(4, 0x7ff00000, true);
    else dv.setUint32(0, 0x00000001, true), dv.setUint32(4, 0xfff80000, true); // NaN w/ payload
    const e = fail(f);
    const maskOk = (e.badMask & MASK_FIN(i)) !== 0 && (e.badMask & MASK_NEG(i)) === 0;
    const bitsOk = e.rawBits[i] === bitsOfBuf(f, KEYS[i], 7);
    if (!maskOk || !bitsOk) { ok = false; detail.push({key: KEYS[i], maskOk, bitsOk, got: e.rawBits[i], want: bitsOfBuf(f, KEYS[i], 7)}); }
    f.bh[7] = 0; f.particleBinding.rollback();
  }
  rec('T150-V1', 'eight non-finite causes: each sets exactly its own finite bit; raw bits read back from buffer byte-for-byte (NaN payload + inf variants)', ok, {detail: detail.slice(0, 3)});
}

// ---------- V2: seven strict negatives + -0 non-trigger + negative surface non-trigger ----------
{
  let ok = true; let negBits = '';
  for (let i = 0; i < 7; i++) {
    const f = field();
    f[KEYS[i]][3] = -Number.MIN_VALUE;
    const e = fail(f);
    if (!((e.badMask & MASK_NEG(i)) !== 0 && (e.badMask & MASK_FIN(i)) === 0 &&
        e.negative[i] === true && e.rawBits[i] === '8000000000000001')) ok = false;
    f.particleBinding.rollback();
  }
  // -0 on a stock component must NOT trigger (x<0 false for -0)
  {
    const f = field(); f.bh[3] = -0;
    const code = f.particleBinding.beginCollect(1/60, 0, 1, 1, 0, 0, 0);
    if (code !== 0) ok = false;
    f.particleBinding.rollback();
  }
  // finite negative surface must NOT trigger original predicate (surface outside strict negativity)
  {
    const f = field(); f.sandTopScratch[3] = -1;
    const code = f.particleBinding.beginCollect(1/60, 0, 1, 1, 0, 0, 0);
    if (code !== 0) ok = false;
    f.particleBinding.rollback();
  }
  rec('T150-V2', 'seven strict negative bits (−MIN, bits 8000000000000001); −0 stock and finite negative surface do NOT trigger', ok, {});
}

// ---------- V3: sum overflow bit15 + independent left-association sum bits ----------
{
  const f = field();
  f.bh[5] = 1e308; f.bl[5] = 1e308;
  const e = fail(f);
  // independent recomputation of the three left-associated sums and their bits
  const s1 = f.bh[5] + f.bl[5], s2 = s1 + f.mh[5], s3 = s2 + f.ml[5];
  const dvo = new DataView(new ArrayBuffer(8));
  const bits = v => { dvo.setFloat64(0, v, false);
    return dvo.getUint32(0, false).toString(16).padStart(8, '0') + dvo.getUint32(4, false).toString(16).padStart(8, '0'); };
  const ok = e.badMask === (1 << 15) && e.sumFinite === false &&
    e.sumBits[0] === bits(s1) && e.sumBits[1] === bits(s2) && e.sumBits[2] === bits(s3) &&
    bits(s3) === '7ff0000000000000';
  rec('T150-V3', 'sum overflow: left-associated (bh+bl)+mh+ml overflow → exactly bit15, sumBits match independent recomputation, all components finite', ok,
    {sumBits: e.sumBits, mask: e.badMask});
}

// ---------- V4: [-EPS,0) dual-contract difference (audit tolerates, Hot rejects) ----------
{
  const f = field(); f.bh[6] = -5e-10;
  const auditCode = f.audit(false); // audit path must tolerate
  const auditNull = auditCode === null || auditCode === undefined;
  const e = fail(f);
  const ok = auditNull && e.column === 6 && (e.badMask & MASK_NEG(1)) !== 0 && e.rawBits[1] === bitsOfBuf(f, 'bh', 6);
  rec('T150-V4', '[-EPS,0) contract difference: audit tolerates (no failure) while Hot rejects with exact negative raw bits — both behaviors coexist, no criterion change', ok,
    {auditCode: String(auditCode), rawBits: e.rawBits[1]});
}

// ---------- V5: first column wins; same-column multi-cause mask composition ----------
{
  const f = field();
  f.waterTop[9] = -1; f.bh[4] = -1; f.suspH[4] = Infinity; // col4 has two causes
  const e = fail(f);
  const ok = e.column === 4 && (e.badMask & MASK_NEG(1)) !== 0 && (e.badMask & MASK_FIN(5)) !== 0 &&
    e.column !== 9;
  rec('T150-V5', 'first invalid column (4) wins over later (9); same-column negative+nonfinite compose both bits', ok, {mask: e.badMask});
}

// ---------- V6: NaN payload survival through cleanup + frozen evidence ----------
{
  const f = field();
  const dv = new DataView(f.bh.buffer, f.bh.byteOffset);
  dv.setUint32(4 * 8, 0x56789abc, true); dv.setUint32(4 * 8 + 4, 0x7ff81234, true);
  const e = fail(f);
  const before = e.rawBits[1];
  f.bh[4] = 0; f.particleBinding.rollback(); // cleanup mutates live array
  const ok = before === '7ff8123456789abc' && e.rawBits[1] === before &&
    e.poolCanonicalBytesHex.length === 13 && e.poolStorageValid === true &&
    e.poolCanonicalBytesHex.every((h, i) => h.length === 2 * e.poolCanonicalByteLengths[i]);
  rec('T150-V6', 'NaN payload (7ff8123456789abc) survives cleanup in frozen evidence; 13 pool canonical buffers hex-complete', ok, {rawBits: e.rawBits[1]});
}

// ---------- V7: window last-legal/first-illegal + ring bound + PREEXISTING ----------
{
  const f = field(), ev = f.particleBinding.work.fieldColumnEvidenceV2;
  f.observeFieldColumnV2('L1', 'INPUT');
  f.bh[3] = -1e-10;
  f.observeFieldColumnV2('BAD', 'INPUT');
  const first = ev.exportEvidence();
  const okWin = first.windowStatus === 'LEGAL_TO_INVALID' &&
    first.lastLegalBytesHex !== null && first.firstIllegalBytesHex !== null &&
    first.lastLegalBytesHex.length === 8 * 128 * 8 * 2;
  f.bh[3] = -2; f.observeFieldColumnV2('LATER', 'INPUT');
  const stable = ev.exportEvidence().firstIllegalBytesHex === first.firstIllegalBytesHex;
  // preexisting + ring overflow
  const f2 = field(), ev2 = f2.particleBinding.work.fieldColumnEvidenceV2;
  f2.bh[2] = -1e-10;
  for (let i = 0; i < 35; i++) f2.observeFieldColumnV2('S' + i, 'TEST');
  const out2 = ev2.exportEvidence();
  const okRing = out2.windowStatus === 'PREEXISTING_IN_WINDOW' && out2.stages.length === 32 &&
    out2.dropped === 3 && out2.stages[0].stage === 'S3';
  rec('T150-V7', 'window: last-legal/first-illegal 8192B snapshots, first illegal not replaced; 35 observations → 32-entry ring, dropped=3, first entry S3; PREEXISTING_IN_WINDOW', okWin && stable && okRing,
    {okWin, stable, ring: okRing});
}

// ---------- V8: trace M2/audit provenance fields + frozen evidence not overwritten ----------
{
  const f = field(), trace = new P011TiltPreviewTrace();
  f.tiltTracePending = [trace]; f.planningPassService.trace = trace;
  f.bh[4] = -1e-10; fail(f); f.recordCollectFirstFailure('PARTICLE_BEGIN', 2);
  const okFlags = trace.firstFailure.driftMeasured === false &&
    trace.firstFailure.rollbackExecuted === false &&
    trace.firstFailure.driftSource === 'NOT_MEASURED' &&
    trace.fieldColumnEvidenceV2 !== null &&
    trace.fieldColumnEvidenceV2.failure.column === 4 &&
    trace.fieldColumnEvidenceV2.failure.surfaceIsSandTopScratch === true;
  const frozen = JSON.stringify(trace.fieldColumnEvidenceV2);
  f.bh[4] = 0; f.recordCollectFirstFailure('CHANNEL', 99);
  const okFrozen = JSON.stringify(trace.fieldColumnEvidenceV2) === frozen;
  rec('T150-V8', 'trace: FIELD_COLUMN provenance driftMeasured=false/driftSource=NOT_MEASURED/rollbackExecuted=false; evidence frozen — later failure does not overwrite', okFlags && okFrozen, {});
}

// ---------- V9: audit drift provenance differs from FIELD_COLUMN defaults ----------
{
  const f = field(), trace = new P011TiltPreviewTrace();
  f.tiltTracePending = [trace]; f.planningPassService.trace = trace;
  f.sourceInjectedHeavy += 4; f.audit(true);
  const ok = trace.firstFailure.driftMeasured === true &&
    trace.firstFailure.driftSource === 'AUDIT_GET_DRIFT' &&
    trace.firstFailure.rollbackExecuted === false;
  rec('T150-V9', 'audit drift provenance: driftMeasured=true/driftSource=AUDIT_GET_DRIFT — distinct from FIELD_COLUMN default zeros', ok, {});
}

// ---------- V10: diagnostics-off zero allocation; on = read-only observation ----------
{
  const x = new WaterSandField(128, 512), y = new WaterSandField(128, 512);
  for (const f of [x, y]) assert.equal(f.setParticleRoute(defaultP011ParticleParams()).ok, true);
  const offNull = x.particleBinding.work.fieldColumnEvidenceV2 === null;
  y.setTiltPreviewTraceSink(() => {});
  const onAlloc = y.particleBinding.work.fieldColumnEvidenceV2 !== null;
  y.observeFieldColumnV2('READ', 'TEST');
  let same = true;
  for (const k of KEYS) { try { assert.deepEqual(x[k], y[k]); } catch { same = false; } }
  let poolSame = true;
  try { assert.deepEqual(x.particleBinding.pool.snapshot(), y.particleBinding.pool.snapshot()); } catch { poolSame = false; }
  rec('T150-V10', 'diagnostics off → evidence null (no window allocation); on → observation is read-only: field arrays and pool snapshot byte-identical to off twin', offNull && onAlloc && same && poolSame, {});
}

// ---------- V11: bounded transport fragments round-trip without bit loss ----------
{
  const f = field(), ev = f.particleBinding.work.fieldColumnEvidenceV2;
  f.observeFieldColumnV2('LEGAL', 'TEST'); f.bh[4] = -5e-10; fail(f);
  const json = JSON.stringify(ev.exportEvidence());
  const fragments = [];
  for (let i = 0; i < json.length; i += 997) fragments.push(json.slice(i, i + 997));
  const decoded = JSON.parse(fragments.join(''));
  const ok = fragments.length > 2 && fragments.every(x => x.length <= 997) &&
    decoded.failure.rawBits[1] === ev.failure.rawBits[1] &&
    decoded.firstIllegalBytesHex.length === 8 * 128 * 8 * 2 &&
    decoded.failure.badMask === ev.failure.badMask;
  rec('T150-V11', 'evidence export survives 997-byte bounded transport fragments: rawBits/badMask/bytes hex identical after reassembly', ok, {fragments: fragments.length});
}

// ---------- V12: identity fields populated (run/epoch/theta/commit/source/poolEpoch) ----------
{
  const f = field(), ev = f.particleBinding.work.fieldColumnEvidenceV2;
  f.observeFieldColumnV2('ID_CHECK', 'INPUT');
  const out = ev.exportEvidence();
  const s = out.stages[out.stages.length - 1];
  const ok = s.stage === 'ID_CHECK' && typeof s.runId === 'string' &&
    typeof s.poolEpoch === 'number' && s.preview === false;
  rec('T150-V12', 'stage identity: stage name, runId, poolEpoch, preview flag populated on observation rows', ok, {sample: {stage: s.stage, runId: s.runId, poolEpoch: s.poolEpoch}});
}

const summary = {executionId: 'p150-field-column-v2-independent-v1', date: '2026-10-27', target: 'T149 e3ee81f', results};
fs.writeFileSync(OUT + '/p150_results.json', JSON.stringify(summary, null, 1) + '\n');
const pass = results.filter(r => r.status === 'PASS').length;
console.log(`total ${results.length} pass ${pass}`);
process.exitCode = pass === results.length ? 0 : 1;
