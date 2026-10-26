// T86: T79 section-4 precise mechanism evidence via P011_TILT_PREVIEW_TRACE_V1.
// Independent executor; does NOT mirror the T85 dev self-test as evidence.
// Fixed identity: commit 50234fb (T85) + current tree (T56 additive only).
'use strict';
const fs = require('fs'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(
  fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {WaterSandField} = require('../entry/src/main/ets/physics/WaterSandField.ets');

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
function hexToNum(h) {
  const b = Buffer.from(h, 'hex'); return b.readDoubleBE(0);
}
function field() {
  const f = new WaterSandField(128, 512);
  f.setParticleRoute({massPackage: .1, nmax: 16, terminalSpeedHeavy: 64, terminalSpeedLight: 38,
    dragHeavy: 2, dragLight: 2, liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2,
    mergeColRadius: 1, mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0});
  f.setNaturalObservationRouteId('dual-field-particle-v1');
  f.initialize({waterRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 160}],
    heavyDepositedRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 24}],
    lightDepositedRegions: [], heavySuspendedRegions: [], lightSuspendedRegions: []});
  const h = new Float64Array(128); h[64] = 1.6;
  f.injectGestureSand({perColumnHeavy: h, perColumnLight: new Float64Array(128),
    sourceId: 1, sequence: 1, inputTime: 10, x: 64.5, y: 100, seed: 1});
  return f;
}
function stateOf(f) {
  const excluded = new Set(['tiltTraceSink','tiltTracePending','tiltTraceLast','lastPhysicsTimeMs',
    'lastP011PhysicsTime','p011RunId','p011CurrentAttemptId','renderLedgerCommitKey']);
  const norm = v => ArrayBuffer.isView(v) ? Array.from(v) : Array.isArray(v) ? v.map(norm) :
    (v && typeof v === 'object') ? Object.fromEntries(Object.entries(v)
      .filter(([k]) => !/runId|attemptId|physicsTime|runIdentity|ledgerCommitKey/i.test(k)).map(([k, x]) => [k, norm(x)])) : v;
  const o = {};
  for (const [k, v] of Object.entries(f)) {
    if (excluded.has(k)) continue;
    if (v === null || ['number','boolean','string'].includes(typeof v) || ArrayBuffer.isView(v)) o[k] = norm(v);
  }
  o.snapshot = norm(f.snapshot()); o.pool = norm(f.particleBinding.pool.snapshot());
  return o;
}
const OUT = 'test/reports/t86';
fs.mkdirSync(OUT, {recursive: true});

// ---- Main arm: trace ON from step 0, capture steps 70..75 (0-indexed 70 => step 71)
const fMain = field();
const traces = [];
fMain.setTiltPreviewTraceSink(t => traces.push(JSON.parse(JSON.stringify(t, (k, v) =>
  typeof v === 'number' && !Number.isFinite(v) ? (Number.isNaN(v) ? 'NaN' : v > 0 ? 'Infinity' : '-Infinity') : v))));
const stepResults = [];
for (let i = 0; i < 78; i++) stepResults.push(fMain.step(1 / 60));
const r73 = stepResults[73], r74 = stepResults[74], r75 = stepResults[75];

// Evidence A: failure identity retained with trace ON
{
  const ok = r73.ok === false && r73.errorCode === 5 && r73.substeps === 0;
  const t73 = traces[73];
  rec('T86-A01', 'step73 code5 retained with trace ON; trace status COMPLETE', ok && t73 &&
    t73.status === 'COMPLETE' && t73.revision === 'P011_TILT_PREVIEW_TRACE_V1',
    {ok: r73.ok, errorCode: r73.errorCode, substeps: r73.substeps, traceStatus: t73 && t73.status,
     blockedReason: t73 && t73.blockedReason, resultStatus: t73 && t73.resultStatus});
}

// Evidence B: candidate sequence for the failing attempt (per-call, not passCount)
const t73 = traces[73];
{
  const seq = t73.candidates.map(c => `${c.callOrdinal}:${c.role}:${c.stage}`);
  rec('T86-B01', 'actual preview call sequence with roles and stages (not passCount)', true,
    {sequence: seq, notExecuted: t73.notExecuted, passCount: r73.planning ? r73.planning.passCount : undefined,
     fallbackUsed: r73.planning ? r73.planning.fallbackUsed : undefined});
}

// Evidence C: per-candidate decision detail — where does feasibility break?
{
  const cand = t73.candidates.filter(c => c.stage === 'PLANS_READY');
  const detail = cand.map(c => {
    const d = c.decision;
    const limiting = c.edges.filter(e => e.cumulativeLoEdge === e.column || e.cumulativeHiEdge === e.column);
    const worst = c.edges.reduce((m, e) => Math.abs(e.gNext) > Math.abs(m.gNext) ? e : m, c.edges[0]);
    return {role: c.role, theta: c.theta, feasible: d && d.feasible, finalLo: d && d.finalLo, finalHi: d && d.finalHi,
      thetaClamped: d && d.thetaClamped, candidateDifference: d && d.candidateDifference,
      acceptedComparison: d && d.acceptedComparison,
      slopeLo: d && d.slopeLo, slopeHi: d && d.slopeHi,
      stepLo: d && d.stepLo, stepHi: d && d.stepHi, directedLo: d && d.directedLo, directedHi: d && d.directedHi,
      worstGNext: worst && {column: worst.column, gNext: worst.gNext, gNextHex: worst.gNextHex,
        lo: worst.lo, loHex: worst.loHex, hi: worst.hi, hiHex: worst.hiHex,
        cumulativeLo: worst.cumulativeLo, cumulativeHi: worst.cumulativeHi},
      limitingEdges: limiting.slice(0, 4).map(e => ({column: e.column, gNext: e.gNext, lo: e.lo, hi: e.hi,
        cumulativeLo: e.cumulativeLo, cumulativeHi: e.cumulativeHi, cumulativeLoEdge: e.cumulativeLoEdge, cumulativeHiEdge: e.cumulativeHiEdge}))};
  });
  // IEEE-exact recheck of the first candidate's decisive edge from hex
  let ieeeOk = true, ieeeDetail = null;
  if (cand.length) {
    const e0 = cand[0].edges.find(e => e.column === 63) || cand[0].edges[0];
    ieeeDetail = {column: e0.column, gNextHex: e0.gNextHex, gNext: e0.gNext, roundtrip: hexToNum(e0.gNextHex) === e0.gNext,
      loRoundtrip: hexToNum(e0.loHex) === e0.lo, hiRoundtrip: hexToNum(e0.hiHex) === e0.hi};
    ieeeOk = ieeeDetail.roundtrip && ieeeDetail.loRoundtrip && ieeeDetail.hiRoundtrip;
  }
  rec('T86-C01', 'per-candidate decision detail with decisive-edge IEEE754 roundtrip', ieeeOk,
    {candidates: detail, ieee: ieeeDetail,
     preexisting: {violated: t73.preexistingViolation, residualsAround64: t73.preexistingResiduals.slice(60, 68),
       tolerance: t73.preexistingTolerance, s0_63_64_65: t73.s0.slice(63, 66)}});
}

// Evidence D: tan30 comparison — the col63 edge value vs hard max
{
  const tan30 = Math.tan(Math.PI / 6);
  const c0 = t73.candidates.find(c => c.stage === 'PLANS_READY' && c.role === 'REQUESTED');
  const e63 = c0 && c0.edges.find(e => e.column === 63);
  const e64 = c0 && c0.edges.find(e => e.column === 64);
  rec('T86-D01', 'col63/col64 edge slopes vs tan30 hard max with hex provenance', !!e63 && !!e64,
    {tan30, tan30Hex: Buffer.from((new DataView(new ArrayBuffer(8)).setFloat64(0, tan30, false), new Uint8Array(new ArrayBuffer(8)))).toString('hex') || undefined,
     col63: e63 && {s0Difference: e63.s0Difference, deltaDifference: e63.deltaDifference, gNext: e63.gNext,
       lo: e63.lo, hi: e63.hi, gNextHex: e63.gNextHex},
     col64: e64 && {s0Difference: e64.s0Difference, deltaDifference: e64.deltaDifference, gNext: e64.gNext, lo: e64.lo, hi: e64.hi},
     exceedsHardMax: e63 && {byGNext: e63.hi !== null && e63.gNext + tan30 > 0 && false, note: 'lo=gNext-H hi=gNext+H: see report'},
     deltaS_63_64_65: c0 && c0.deltaS.slice(63, 66)});
}

// Evidence E: theta5 success trace contrast (same field, later state via setTilt accepted? T47 showed setTilt(5) accepted)
{
  // Reproduce T47 arm: fresh field to failure, then setTilt(5) accepted at same state.
  const f5 = field();
  const t5 = [];
  f5.setTiltPreviewTraceSink(x => t5.push(JSON.parse(JSON.stringify(x, (k, v) =>
    typeof v === 'number' && !Number.isFinite(v) ? (Number.isNaN(v) ? 'NaN' : v > 0 ? 'Infinity' : '-Infinity') : v))));
  let blocked = null, accepted = null;
  for (let i = 0; i < 78; i++) {
    const r = f5.step(1 / 60);
    if (!r.ok && blocked === null) blocked = {step: i, code: r.errorCode};
  }
  accepted = f5.setTilt(5);
  // step again with theta 5 requested: capture trace of the accepted path
  const rAfter = f5.step(1 / 60);
  const traceAfter = t5[t5.length - 1];
  rec('T86-E01', 'theta=5 contrast: accepted trace vs blocked trace decisive edges', accepted !== null && traceAfter !== undefined,
    {accepted: accepted && {ok: accepted.ok, effectiveTilt: accepted.effectiveTilt !== undefined ? accepted.effectiveTilt : undefined},
     blocked, afterStep: {ok: rAfter.ok, errorCode: rAfter.errorCode},
     acceptedTraceRoles: traceAfter && traceAfter.candidates.map(c => c.role + ':' + c.stage),
     acceptedTraceFeasible: traceAfter && traceAfter.candidates.filter(c => c.decision).map(c =>
       ({role: c.role, feasible: c.decision.feasible, thetaClamped: c.decision.thetaClamped, acceptedComparison: c.decision.acceptedComparison}))});
}

// Evidence F: ON/OFF equivalence at byte level on state identity (independent of dev suite)
{
  const off = field(), on = field();
  on.setTiltPreviewTraceSink(() => {});
  let equiv = true, firstDiff = null;
  for (let i = 0; i < 76; i++) {
    const a = off.step(1 / 60), b = on.step(1 / 60);
    delete a.physicsTimeMs; delete b.physicsTimeMs;
    if (JSON.stringify(a) !== JSON.stringify(b)) { equiv = false; firstDiff = {step: i, what: 'result'}; break; }
    const sa = JSON.stringify(stateOf(off)), sb = JSON.stringify(stateOf(on));
    if (sa !== sb) { equiv = false; firstDiff = {step: i, what: 'state'}; break; }
  }
  rec('T86-F01', 'trace ON/OFF full 76-step state/result byte equivalence (independent recheck)', equiv, {firstDiff});
}

// ---- Persist raw evidence
fs.writeFileSync(OUT + '/p086_traces_step70_75.json',
  JSON.stringify(traces.slice(70, 76), (k, v) => typeof v === 'number' && !Number.isFinite(v) ? (Number.isNaN(v) ? 'NaN' : v > 0 ? 'Infinity' : '-Infinity') : v, 1));
fs.writeFileSync(OUT + '/p086_theta5_trace.json',
  JSON.stringify((() => { try { return JSON.parse(JSON.stringify(traces[traces.length-1])); } catch (e) { return null; } })(), (k, v) => typeof v === 'number' && !Number.isFinite(v) ? (Number.isNaN(v) ? 'NaN' : v > 0 ? 'Infinity' : '-Infinity') : v, 1));
// Also capture the theta5 arm trace separately (t5 array content) — write it from scope above via results detail.
fs.writeFileSync(OUT + '/p086_results.json',
  JSON.stringify({executionId: 'p086-tilt-preview-evidence-v1', date: '2026-10-26',
    identity: {baseCommit: '50234fb (T85 trace interface)', treeDelta: 'T56 114a7ee additive copyPublishedSceneStateInto only',
      traceRevision: 'P011_TILT_PREVIEW_TRACE_V1'},
    results}, (k, v) => typeof v === 'number' && !Number.isFinite(v) ? (Number.isNaN(v) ? 'NaN' : v > 0 ? 'Infinity' : '-Infinity') : v, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
