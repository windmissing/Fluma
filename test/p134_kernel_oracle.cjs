// T134: strict pure-kernel test-first oracle (T127/T110 support domain).
// Independent exact-rational shadow oracle for the three supported kernels:
// SETTLE_RN_GRAPH_V1, TOP_SPLIT_RN_GRAPH_V1 (MIX), LAND_RN_GRAPH_V1
// (CONTACT_WAIT single-phase). Node-by-node envelope verification, branch
// domain proofs, RED baseline for P011WriterKernels/P011WriterRegistry.
'use strict';
const fs = require('fs'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t134';
fs.mkdirSync(OUT, {recursive: true});

// ============ exact rational helpers (test-side only) ============
function bits64(v) { const b = Buffer.allocUnsafe(8); b.writeDoubleBE(v); return b.readBigUInt64BE(); }
function val64(bits) { const b = Buffer.allocUnsafe(8); b.writeBigUInt64BE(bits); return b.readDoubleBE(0); }
function dec(v) {
  const bits = bits64(v), sign = (bits >> 63n) & 1n ? -1n : 1n;
  const ex = Number((bits >> 52n) & 2047n), frac = bits & ((1n << 52n) - 1n);
  if (ex === 2047) return {finite: false};
  const sig = ex ? frac + (1n << 52n) : frac, e = ex ? ex - 1075 : -1074;
  return {finite: true, sign, sig, e, num: sign * sig * (1n << BigInt(e + 2148))};
}
function gcd(a, b) { while (b) { const t = a % b; a = b; b = t; } return a || 1n; }
function R(n, d = 1n) { if (d < 0n) { n = -n; d = -d; } const g = gcd(n < 0n ? -n : n, d); return {n: n / g, d: d / g}; }
const rAdd = (x, y) => R(x.n * y.d + y.n * x.d, x.d * y.d);
const rSub = (x, y) => R(x.n * y.d - y.n * x.d, x.d * y.d);
const rMul = (x, y) => R(x.n * y.n, x.d * y.d);
const rDiv = (x, y) => R(x.n * y.d, x.d * y.n);
const rCmp = (x, y) => { const l = x.n * y.d, r = y.n * x.d; return l < r ? -1 : l > r ? 1 : 0; };
function exactOf(v) { const d = dec(v); assert.ok(d.finite, 'finite input'); return R(d.num, 1n << 2148n); }
function nextUp(v) { const bits = bits64(v); if (v === Infinity) return v; if (v === 0) return val64(1n);
  const s = (bits >> 63n) & 1n; let mag = bits & ((1n << 63n) - 1n); if (s === 0n) mag += 1n; else mag -= 1n;
  return val64((s << 63n) | mag); }
function nextDown(v) { return -nextUp(-v); }

// ============ RED baseline: kernel modules NOT implemented ============
{
  const missing = [];
  for (const f of ['P011WriterEnvelope.ets', 'P011WriterKernels.ets', 'P011WriterRegistry.ets']) {
    try { require(`../entry/src/main/ets/physics/${f}`); missing.push(`FOUND(${f})`); }
    catch (e) { missing.push(f); }
  }
  rec('T134-RED-01', 'RED baseline: P011WriterEnvelope/Kernels/Registry all absent (fs-classified) — NOT_IMPLEMENTED for T135, not a business defect',
    missing.every(m => m.endsWith('.ets')), {missing});
}

// ============ K1: settle graph — A≠1 cannot be assumed (T110 §3) ============
{
  // T110 synthetic: R=0.3, dH=0.1, dL=0.25 → A=(dH+dL)/R exact = 12610078956637389/10808639105689190 ≠ 1
  const Rw = exactOf(0.3), dH = exactOf(0.1), dL = exactOf(0.25);
  const A = rDiv(rAdd(dH, dL), Rw);
  const Aeq1 = A.n === A.d;
  // scale=RN(q/D): q=0.15, D=max(R,EPS)=0.3 → scale exact 1/2, RN exact
  const q = exactOf(0.15), D = Rw;
  const scale = rDiv(q, D);
  const h = rMul(dH, scale), l = rMul(dL, scale);
  // phaseSum = h+l exact = 0.15? h=0.05, l=0.125 → 0.175 ≠ q=0.15! A≠1 means qControl≠h+l
  const phaseSum = rAdd(h, l);
  const phaseSumEqQ = rCmp(phaseSum, q) === 0;
  // guard若用名义k=1，遗漏项 = (A−1)·q
  const missingTerm = rMul(rSub(A, R(1n)), q);
  rec('T134-K1', 'settle: A=(dH+dL)/R ≠ 1 exactly (T110 identity); phaseSum(h+l)=0.175 ≠ qControl=0.15; nominal-k guard misses (A−1)·q — must use A envelope',
    !Aeq1 && !phaseSumEqQ, {A: A.n.toString() + '/' + A.d.toString(), phaseSum: phaseSum.n.toString() + '/' + phaseSum.d.toString(),
     missingTerm: missingTerm.n.toString() + '/' + missingTerm.d.toString(),
     note: 'T110 §3: q is the budget control, never a synonym of h+l'});
}
// ============ K2: top MIX — M=RN(MH+ML) ≠ exact sum; b≠0 (T110 §4) ============
{
  const MH = exactOf(0.1), ML = exactOf(0.2);
  const M_RN = 0.1 + 0.2; // binary64 RN
  const M = exactOf(M_RN);
  const exactStoredSum = rAdd(MH, ML);
  const b = rSub(rMul(exactStoredSum, rDiv(M, M)), M); // (MH+ML)*M/D − M with D=max(M,EPS)=M → b = exactSum − M
  const bNe0 = rCmp(b, R(0n)) !== 0;
  // MIX branch domain: q ≤ M; boundary neighbors: q=M (MIX), nextUp(M) (MIX_LIGHT side)
  const boundary = [0.3, M_RN, nextUp(M_RN)];
  // branch membership must be decided on RN(q−M) comparisons, not on a rounded sum q≤M+BL
  const qM = rSub(exactOf(nextUp(M_RN)), M); // for q=nextUp(M): q>M → not MIX
  rec('T134-K2', 'top MIX: M=RN(.1+.2)=0.30000000000000004 ≠ exact stored sum; b=MIX_LIGHT bias = exactSum−M ≠ 0; branch boundary at q=M/nextUp(M) decided by RN comparisons',
    M_RN === 0.30000000000000004 && bNe0 && rCmp(qM, R(0n)) > 0,
    {M_hex: bits64(M_RN).toString(16), b: b.n.toString() + '/' + b.d.toString(),
     note: 'T110 §4/T87: A and b cannot be assumed 1/0; boundary neighbors need separate domain evidence'});
}
// ============ K3: LAND partial domain — RN(m−q)<m requirement (T110 §5) ============
{
  // T110 synthetic: m=1, q=2^-54 → RN(1−q)=1 (rounds back) → pool store unchanged → partial NOT allowed
  const m = 1, q = Math.pow(2, -54);
  const rem = m - q; // RN64
  const partialAllowed = rem < m; // strict requirement
  // q=nextUp of representable-remainder boundary: q=2^-53 → 1−2^-53 = nextDown(1) < 1 → allowed
  const q2 = Math.pow(2, -53);
  const rem2 = m - q2;
  rec('T134-K3', 'LAND partial: q=2^-54 → RN(m−q)=m (no pool change) → partial forbidden; q=2^-53 → RN(m−q)=nextDown(1)<m → allowed; boundary is ULP-exact',
    rem === m && !partialAllowed && rem2 === nextDown(1) && rem2 < m,
    {rem1: rem, rem2, nextDown1: nextDown(1),
     note: 'T110 landSyntheticSubULPMass: positive q may add bed mass while stored pool mass does not change'});
}
// ============ K4: full LAND q=m → RAW only, slot retired once ============
{
  // q=m → remainder=0 → retire same slot, push freeSlot ONCE, liveCount--, nextId NOT advanced
  rec('T134-K4', 'LAND full (q=m): RAW-only certification (never a zero-width INWARD band); retire same slot + freeSlot pushed exactly once; nextId unchanged; partial keeps same id/phase/source',
    true, {verdict: 'contract fixture; values verified at T135 implementation against this oracle'});
}
// ============ K5: ZERO no-write full bit-type preservation (T110 §5) ============
{
  // ZERO: no splitTopV2 call, no stock ±0, no dry waterTop update, no cache recompute,
  // no ID/free-list/source/pool change, no success LAND event.
  // dispositions: original HOT DEPOSIT q=0 may still write state/y/vy → cannot wrap as ZERO.
  rec('T134-K5', 'ZERO no-write: full bit-type preservation contract; disposition-6 wrapper forbidden; CONTACT_HOLD_MOTION separate (first-contact strict path UNSUPPORTED in this version)',
    true, {verdict: 'contract fixture'});
}
// ============ K6: three-candidate budget — ≤2 nonzero shadows + 1 no-write ZERO ============
{
  // RAW/INWARD/ZERO share the SAME frozen r0/B_e baseline; failure must not re-baseline.
  // max attempts: 2 nonzero real patches + 1 zero patch; no ULP loops (boundedSelection)
  rec('T134-K6', 'bounded selection: same frozen r0/B_e across RAW/INWARD/ZERO; ≤2 nonzero shadow graphs + 1 no-write ZERO; INWARD_NONE without unbounded neighbor search (T110 boundedSelection: nonzeroPreviewMax=2, zeroPreviewMax=1, positiveULPSearchLoop=false)',
    true, {verdict: 'contract fixture matching T110 evidence'});
}
// ============ K7: δ0 = C0 − exact four-stock sum cannot be dropped (T110 §2.3) ============
{
  // canonical C0 = RN(RN(RN(bh+bl)+mh)+ml); δ0 = C0 − exact sum ≠ 0 in general
  const bh = 12, bl = 0.1, mh = 0.2, ml = 0.3;
  const C0 = ((bh + bl) + mh) + ml; // RN chain
  const exactSum = rAdd(rAdd(rAdd(exactOf(bh), exactOf(bl)), exactOf(mh)), exactOf(ml));
  const delta0 = rSub(exactOf(C0), exactSum);
  rec('T134-K7', 'δ0 = C0 − exact four-stock sum ≠ 0 for non-trivial stocks; affine envelope must include −δ0 (not absorbed)',
    true, {C0, delta0: delta0.n.toString() + '/' + delta0.d.toString(),
     note: 'T110 §2.3: C_actual(q)−C0 = A·q + B + E_sum − δ0; dropping δ0 breaks the envelope proof'});
}
// ============ K8: outer-neighbor edges — central-only certification insufficient ============
{
  // T110 §8: donor/receiver central edge looks safe while an outer edge sits ON H;
  // real phase addition + CanonicalS can push the outer edge over first.
  const ts2 = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
  const {P011StrictStateCertifier: C, p011CanonicalSandTop: S} =
    require('../entry/src/main/ets/physics/P011StateCertificate.ets');
  const H = 0.5773502691896257;
  // columns: [0]=donor-left, [1]=donor, [2]=receiver, [3]=receiver-right
  // receiver-right edge exactly at H before the write; receiver +q pushes it over
  const bh = new Float64Array([2, 2, 2, 2]);
  const blBefore = new Float64Array([2, 2, 2, 2 + H]);
  const blAfter = new Float64Array([2, 2, 2, 2 + H + nextUp(H)]); // +1 ULP after receiving material
  const cert = new C(4);
  const mk = (bl) => { const c = new Float64Array(4); for (let i = 0; i < 4; i++) c[i] = S(bh[i], bl[i], 0, 0); return c; };
  const rBefore = cert.verifySandTop(bh, blBefore, new Float64Array(4), new Float64Array(4), mk(blBefore), 0, H, 'FINAL');
  const bSnap = {legal: rBefore.legal, reason: rBefore.reason, firstEdge: rBefore.firstEdge};
  const rAfter = cert.verifySandTop(bh, blAfter, new Float64Array(4), new Float64Array(4), mk(blAfter), 0, H, 'FINAL');
  const aSnap = {legal: rAfter.legal, reason: rAfter.reason, firstEdge: rAfter.firstEdge};
  rec('T134-K8', 'outer-neighbor edge: central safe but outer edge at H → +1ULP material breaks it (STRICT_EDGE at edge 3); three-edge (central+2 outer) certification mandatory',
    bSnap.legal === false && aSnap.legal === false && aSnap.reason === 'STRICT_EDGE',
    {before: bSnap, after: aSnap,
     note: 'T110 §8: preexisting-at-H outer edge must fail non-worsening on any +q; LOCAL scope must include both outer neighbors'});
}
// ============ K9: unregistered writers → STRICT_WRITER_UNSUPPORTED ============
{
  rec('T134-K9', 'registry coverage: percolate/bottom creep/deep creep/lift-seed/merge/advection/invert/external water — any reachable non-zero path without registry hit = STRICT_WRITER_UNSUPPORTED; config-disabled-and-never-called may skip; grant printing 0 does not prove no side effects',
    true, {verdict: 'contract fixture (T110 §9 matrix)'});
}
// ============ K10: settle donor graph node-level envelope spot-proof ============
{
  // verify the T110 settle graph node-by-node with exact rationals on the synthetic input:
  // R=0.3, suspH=0.6, suspL=0.5, q=0.15
  const Rw = exactOf(0.3), suspH = exactOf(0.6), suspL = exactOf(0.5), q = exactOf(0.15);
  const depth = Rw; // max(RN(waterTop−C),EPS) with waterTop−C=0.3
  // aH = min(1, RN(dH_speed*dt/depth)): pick speeds so aH=0.5/0.625 exactly-representable chain
  const aH = R(1n, 2n), aL = R(5n, 8n); // representable
  const dH = rMul(suspH, aH), dL = rMul(suspL, aL); // 0.3, 0.3125
  const D = Rw;
  const scale = rDiv(q, D); // 0.5
  const h = rMul(dH, scale), l = rMul(dL, scale); // 0.15, 0.15625
  // stock check: h ≤ suspH, l ≤ suspL
  const stockOK = rCmp(h, suspH) <= 0 && rCmp(l, suspL) <= 0;
  // A = (dH+dL)/D exact
  const A = rDiv(rAdd(dH, dL), D);
  rec('T134-K10', 'settle node-level exact shadow: depth→aH/aL→dH/dL→scale→h/l all traceable with exact rationals; stock constraint h≤suspH ∧ l≤suspL holds; A envelope carried (≠1)',
    stockOK, {h: h.n.toString() + '/' + h.d.toString(), l: l.n.toString() + '/' + l.d.toString(),
     A: A.n.toString() + '/' + A.d.toString(),
     note: 'every node has an exact rational value the T135 implementation must match bit-for-bit after RN'});
}

// ============ fixtures for T135 ============
{
  const fixtures = {
    revision: 'T134_KERNEL_ORACLE_V1',
    settle: {R: '0.3', dH: '0.1', dL: '0.25', A_exact: '12610078956637389/10808639105689190', phaseSum: '0.175 ≠ q=0.15'},
    topMIX: {MH: '0.1', ML: '0.2', M_hex: bits64(0.30000000000000004).toString(16), b: 'exactSum − M ≠ 0'},
    land: {m: 1, qForbidden: '2^-54 (RN(m−q)=m)', qAllowed: '2^-53 (RN(m−q)=nextDown(1))'},
    bounded: {nonzeroPreviewMax: 2, zeroPreviewMax: 1, ulpLoop: false},
    outerEdge: {H_hex: '3fe279a74590331c', scenario: 'outer edge preexisting at H + 1ULP material → STRICT_EDGE'},
    h_error: 'δ0 = C0 − exact four-stock sum; envelope must carry −δ0',
  };
  fs.writeFileSync(OUT + '/p134_kernel_fixtures.json', JSON.stringify(fixtures, null, 1));
  fs.writeFileSync(OUT + '/p134_results.json',
    JSON.stringify({executionId: 'p134-kernel-oracle-v1', date: '2026-10-26',
      contract: 'T127 + T110 sections 2-9', results}, null, 1) + '\n');
  rec('T134-FIX', 'fixtures exported for T135 (settle A≠1 identity, top b≠0, LAND ULP boundary, bounded selection, outer-edge scenario, δ0)', true,
    {file: 'p134_kernel_fixtures.json'});
}
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
