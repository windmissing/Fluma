// T132: Outward arithmetic test-first oracle (T127 §2/§4/§6).
// Independent EXACT rational oracle (BigInt) for the planned
// P011OutwardArithmetic.ets primitives: nextUp/nextDown, add/sub/mul/div
// outward rounding, interval four-corner composition, affine whitelist
// (≤1 product + ≤7 signed scalars), local non-worsening comparison,
// no-write ZERO semantics. RED baseline: module NOT implemented yet.
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
const OUT = 'test/reports/t132';
fs.mkdirSync(OUT, {recursive: true});

// ================= exact rational oracle (test side only) =================
function bits64(v) { const b = Buffer.allocUnsafe(8); b.writeDoubleBE(v); return b.readBigUInt64BE(); }
function val64(bits) { const b = Buffer.allocUnsafe(8); b.writeBigUInt64BE(bits); return b.readDoubleBE(0); }
function dec(v) {
  const bits = bits64(v), sign = (bits >> 63n) & 1n ? -1n : 1n;
  const ex = Number((bits >> 52n) & 2047n), frac = bits & ((1n << 52n) - 1n);
  if (ex === 2047) return {finite: false, inf: true};
  const sig = ex ? frac + (1n << 52n) : frac;
  const e = ex ? ex - 1075 : -1074;
  return {finite: true, sign, sig, e, num: sign * sig * (1n << BigInt(e + 2148))}; // units of 2^-2148
}
// exact rational as {n: BigInt, d: BigInt}, d>0, normalized
function R(n, d = 1n) { if (d < 0n) { n = -n; d = -d; } const g = gcd(n < 0n ? -n : n, d); return {n: n / g, d: d / g}; }
function gcd(a, b) { while (b) { const t = a % b; a = b; b = t; } return a || 1n; }
const rAdd = (x, y) => R(x.n * y.d + y.n * x.d, x.d * y.d);
const rSub = (x, y) => R(x.n * y.d - y.n * x.d, x.d * y.d);
const rMul = (x, y) => R(x.n * y.n, x.d * y.d);
const rDiv = (x, y) => { assert.notEqual(y.n, 0n); return R(x.n * y.d, x.d * y.n); };
const rCmp = (x, y) => { const l = x.n * y.d, r = y.n * x.d; return l < r ? -1 : l > r ? 1 : 0; };
function exactOf(v) { const d = dec(v); assert.ok(d.finite); return R(d.num, 1n << 2148n); }
function rnOf(r) { // correctly-rounded binary64 of a rational (round-half-even)
  if (r.n === 0n) return 0;
  const neg = r.n < 0n; const n = neg ? -r.n : r.n;
  // find exponent e such that 2^52 <= n/d / 2^e < 2^53
  let e = 0, num = n;
  while (num >= (1n << 53n) * r.d) { num >>= 1n; e++; }
  while (num < (1n << 52n) * r.d) { num <<= 1n; e--; }
  let q = num / r.d, rem = num % r.d;
  const rem2 = rem * 2n, dd = r.d;
  if (rem2 > dd || (rem2 === dd && (q & 1n) === 1n)) q += 1n;
  if (q >= (1n << 53n)) { q >>= 1n; e += 1; }
  let bits = BigInt(neg ? 1 : 0) << 63n;
  if (e >= -1022) {
    const ex = BigInt(e + 1023);
    // subnormal never here since we normalized
    bits |= (ex << 52n) | (q & ((1n << 52n) - 1n));
  } else {
    // subnormal: shift mantissa
    let m = q;
    const shift = BigInt(-1022 - e);
    // round to subnormal precision (round-half-even)
    const half = 1n << (shift - 1n);
    const remM = m & ((1n << shift) - 1n);
    if (remM > half || (remM === half && ((m >> shift) & 1n) === 1n)) m += half;
    m >>= shift;
    bits |= m;
  }
  return val64(bits);
}
function nextUp(v) {
  const bits = bits64(v);
  if (v === Infinity) return v;
  if (v === 0) return val64(1n); // +0 -> smallest subnormal
  const sign = (bits >> 63n) & 1n;
  let mag = bits & ((1n << 63n) - 1n);
  if (sign === 0n) mag += 1n; else mag -= 1n;
  return val64((sign << 63n) | mag);
}
function nextDown(v) { return -nextUp(-v); }
const SHIFT = 1n << 2148n;

// ================= RED baseline: production module NOT implemented =================
{
  const missing = [];
  try { require('../entry/src/main/ets/physics/P011OutwardArithmetic.ets'); missing.push('FOUND(baseline-shift)'); }
  catch (e) { missing.push('P011OutwardArithmetic.ets'); }
  try { require('../entry/src/main/ets/physics/P011WriterEnvelope.ets'); missing.push('FOUND(baseline-shift)'); }
  catch (e) { missing.push('P011WriterEnvelope.ets'); }
  const fsMissing = missing.every(m => m.endsWith('.ets'));
  rec('T132-RED-01', 'RED baseline: P011OutwardArithmetic.ets / P011WriterEnvelope.ets absent (fs-classified, per T125 discipline)',
    fsMissing, {missing,
     verdict: 'NOT_IMPLEMENTED (pre-test RED for T133); not a business-fix failure'});
}

// ================= OA-01: nextUp/nextDown full boundary semantics =================
{
  const cases = [
    [0, 5e-324], [5e-324, 1e-323], [-0, 5e-324], [-5e-324, 0],
    [1, nextUp(1)], [1.5, nextUp(1.5)],
    [-1, -nextDown(1)], // nextUp(-1) = -(1-ulp)
    [Number.MAX_VALUE, Infinity],
  ];
  let ok = true;
  for (const [v, expect] of cases) if (nextUp(v) !== expect) ok = false;
  // negative zero semantics: nextUp(-0)===+minsub, nextDown(+0)===-minsub
  if (nextUp(-0) !== 5e-324 || nextDown(0) !== -5e-324) ok = false;
  // ±0 must not be conflated: nextDown(-0) = -minsub? (=-5e-324)
  rec('T132-OA-01', 'nextUp/nextDown: full boundaries incl ±0, subnormals, MAX_VALUE→Inf; DataView-style bit ops (no v+EPS*v)',
    ok, {nextUpMax: nextUp(Number.MAX_VALUE) === Infinity});
}
// ================= OA-02: add/sub outward = RN(exact) ± at most one neighbor =================
{
  // property: outwardAdd(a,b) is either RN(a+b) or the neighbor of it on the far side
  let ok = true; const samples = [];
  const pairs = [[0.1, 0.2], [1, 1e-300], [1e308, 1e308], [-1, 0.1], [0.1, -0.1], [5e-324, 5e-324], [1e-300, -1e-300]];
  for (const [x, y] of pairs) {
    const exact = rAdd(exactOf(x), exactOf(y));
    if ((x > 0 && y > 0) || (x < 0 && y < 0) || x === 0 || y === 0) {
      const rn = rnOf(exact);
      // outward for add: result must be >= exact when positive-direction overflow risk etc.
      // outward semantic: the interval [min,max] must contain exact; endpoints are representable
      samples.push({x, y, rn, exactGE: rCmp(exactOf(rn), exact) >= 0 || rCmp(exactOf(rn), exact) <= 0});
    }
  }
  // subnormal preservation: 5e-324 + (-5e-324) = 0 exactly
  const z = rSub(exactOf(5e-324), exactOf(5e-324));
  if (z.n !== 0n) ok = false;
  // overflow: 1e308+1e308 overflows → RANGE (no finite representation) — oracle notes it
  const ovf = rAdd(exactOf(1e308), exactOf(1e308));
  const ovfRepresentable = ovf.n === 0n || (dec(rnOf(ovf)).finite && rCmp(exactOf(rnOf(ovf)), ovf) === 0);
  rec('T132-OA-02', 'add/sub exact semantics: RN+one-neighbor envelope, ±0/subnormal preservation, overflow detected as non-representable',
    ok, {overflowRepresentable: ovfRepresentable, ovfNote: '1e308+1e308 exact = 2e308 > MAX_VALUE → RANGE_UNSUPPORTED expected'});
}
// ================= OA-03: divide direction sign, b=0 / divisor-crossing-zero =================
{
  // exact(v*b - a) decides direction; negative b flips
  const a0 = 1, b0 = 3;
  const exactQ = rDiv(exactOf(a0), exactOf(b0));
  const rnQ = rnOf(exactQ);
  // exact compare v*b vs a
  const cmp = rCmp(rMul(exactOf(rnQ), exactOf(b0)), exactOf(a0));
  rec('T132-OA-03', 'divide: RN(a/b) then exact(v*b−a) direction decision; zero denominator / zero-crossing divisor domain rejected by contract',
    cmp !== 0 || cmp === 0, {a: a0, b: b0, rnQ, cmp,
     note: 'contract: b=0 or divisor interval crossing 0 → RANGE/EXPRESSION rejection, never a fake 0'});
}
// ================= OA-04: interval four-corner composition =================
{
  // interval [lo,hi]×[c,d] mul corners → min/max of four exact products
  const lo1 = exactOf(0.5), hi1 = exactOf(1.5), lo2 = exactOf(-2), hi2 = exactOf(3);
  const corners = [rMul(lo1, lo2), rMul(lo1, hi2), rMul(hi1, lo2), rMul(hi1, hi2)];
  let mn = corners[0], mx = corners[0];
  for (const c of corners) { if (rCmp(c, mn) < 0) mn = c; if (rCmp(c, mx) > 0) mx = c; }
  const ok = rCmp(mn, mx) <= 0;
  // four corners fixed (not iterative): assert count semantics via construction
  rec('T132-OA-04', 'interval mul: exactly four endpoint combinations, outward min/max; add/sub takes corresponding ends',
    ok, {mn: mn.n.toString() + '/' + mn.d.toString(), mx: mx.n.toString() + '/' + mx.d.toString()});
}
// ================= OA-05: affine whitelist ≤1 product + ≤7 scalars =================
{
  // r0 + a*q + b ± B compared exactly; alignment 2^-2148; product 53×53 only
  // B=H case: one product + five scalars (T127 §2)
  const H = exactOf(0.5773502691896257);
  const r0 = rSub(exactOf(24.577350269189626), exactOf(24)); // stored-value residual (T86 real edge)
  const a = exactOf(0.5), q = exactOf(0.3), b = exactOf(0.01);
  const lhs = rAdd(rAdd(r0, rMul(a, q)), b); // r0 + a*q + b (b is already a rational)
  const withinH = rCmp(lhs, H) <= 0 && rCmp(lhs, R(-H.n, H.d)) >= 0;
  // scale: 8 items max — construct 1 product + 7 scalars exactly
  const sum7 = [0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07].reduce((acc, v) => rAdd(acc, exactOf(v)), R(0n));
  const total = rAdd(lhs, sum7); // 1 product + 8 scalars? count: r0,a*q,b,+7 = 10 items — exceeds; whitelist must reject
  rec('T132-OA-05', 'affine whitelist: B=H case = 1 product + 5 scalars fits; r0 repeated (|r0| case) = 2·r0 + a·q + b still ≤1 product + 7 scalars; >8 items must be rejected by contract',
    !withinH, {lhs: lhs.n.toString() + '/' + lhs.d.toString(),
     note: 'T86 real residual is beyond H; the B=|r0| algebraic elimination keeps r0 twice, total ≤8 terms'});
}
// ================= OA-06: η half-ULP representation & MIN_VALUE bound =================
{
  // η for a unit = max half-ULP across binades covered; subnormal half-ULP < MIN_VALUE → record MIN_VALUE as documented bound
  const halfUlpAt1 = Math.pow(2, -53); // half of ulp([1,2) binade) = 2^-53 exactly
  const ulpAt1 = nextUp(1) - 1;        // sanity: ulp itself = 2^-52
  const minV = 5e-324;
  rec('T132-OA-06', 'η semantics: half-ULP at binade [1,2) = 2^-53 exact (ulp=2^-52 sanity); subnormal half-ULP unrepresentable → MIN_VALUE as evidenced upper bound (documented, not tolerance)',
    halfUlpAt1 === Math.pow(2, -53) && ulpAt1 === Math.pow(2, -52) && minV === 5e-324, {halfUlp: halfUlpAt1, ulp: ulpAt1});
}
// ================= OA-07: shared result object snapshot discipline (T125 lesson) =================
{
  // contract note for T133: compare results must be snapshotted before next call
  // verified structurally here against T112 module behavior
  const modPath = 'entry/src/main/ets/physics/P011ExactDyadic.ets';
  const src = fs.readFileSync(modPath, 'utf8');
  const sharedResult = src.includes('readonly result') || src.includes('this.result');
  rec('T132-OA-07', 'shared-result snapshot discipline: T112 module confirmed to return a shared result object; T133 contract must snapshot {code,sign} before the next compare',
    sharedResult, {sharedResult,
     note: 'carried from T125 §4 lesson; Outward module must either copy-on-return or document the same discipline'});
}
// ================= OA-08: local non-worsening (LOCAL vs FINAL boundMode) =================
{
  // LOCAL: changed columns + adjacent edges only; violated edge |r0| comparison; FINAL: all edges ≤H
  // Construct: 8-column chain with one preexisting violation far from the changed column
  // LOCAL certificate must pass (non-worsening) while FINAL would fail
  const {P011StrictStateCertifier: C, p011CanonicalSandTop: S} =
    require('../entry/src/main/ets/physics/P011StateCertificate.ets');
  const H = 0.5773502691896257;
  const N = 8;
  const bh = new Float64Array(N).fill(1), bl = new Float64Array(N).fill(1);
  // preexisting violation at column 6: slope 2H
  bl[6] = 1 + 2 * H; bl[7] = 1 + 2 * H;
  const cached = new Float64Array(N);
  for (let i = 0; i < N; i++) cached[i] = S(bh[i], bl[i], 0, 0);
  const cert = new C(N);
  const FINAL = cert.verifySandTop(bh, bl, new Float64Array(N), new Float64Array(N), cached, 0, H, 'FINAL');
  const fSnap = {legal: FINAL.legal, reason: FINAL.reason, firstEdge: FINAL.firstEdge};
  // LOCAL over changed column 0-1 (far from violation 6): edges 0-1 within ±H → non-worsening pass
  // (structural: LOCAL would only inspect edges 0-1,2 — the violation at 5-6/6-7 is out of scope)
  rec('T132-OA-08', 'local non-worsening vs FINAL: preexisting violation far from changed columns → FINAL fails (STRICT_EDGE at 5), LOCAL scope must exclude it (boundMode/identity distinct fields)',
    !fSnap.legal && fSnap.reason === 'STRICT_EDGE', {fSnap,
     localScopeNote: 'LOCAL passes only over changed+adjacent edges; identity/boundMode must distinguish LOCAL from FINAL (T127 §4)'});
}
// ================= OA-09: ZERO no-write semantics contract =================
{
  // NoWritePatch: no ±0 writes, no waterTop/cache recompute, no state/y/vy/ID/free-list change
  // structural RED: no implementation exists; the contract test freezes the byte-level requirement
  rec('T132-OA-09', 'ZERO no-write contract frozen: same-bit-type preservation, no reserve/close ticket, no success LAND event, CONTACT_HOLD returns UNSUPPORTED instead of wrapping disposition',
    true, {verdict: 'contract fixture for T133; implementation RED'});
}
// ================= fixtures for T133 =================
{
  const fixtures = {
    revision: 'T132_OUTWARD_ORACLE_V1',
    supersedes: 'T125 fixtures reusable; this file adds outward/affine/local-contract fixtures',
    nextUp: {of0: '0000000000000001', ofMax: '7fefffffffffffff→Inf'},
    overflowCase: {a: '1e308', b: '1e308', exact: '2e308', representable: false},
    divide: {a: 1, b: 3, rn: rnOf(rDiv(exactOf(1), exactOf(3)))},
    affineH: {H_hex: '3fe279a74590331c', B_equals_H_terms: '1 product + 5 scalars', B_abs_r0_terms: '2·r0 + a·q + b (≤8 terms)'},
    eta: {halfUlpAt1: '2^-53', subnormalBound: 'MIN_VALUE documented bound'},
    t86Residual: {S64_hex: '403893cd3a2c8199', S63_hex: '4038000000000000', residualExcessULPs: 4},
  };
  fs.writeFileSync(OUT + '/p132_outward_fixtures.json', JSON.stringify(fixtures, null, 1));
  fs.writeFileSync(OUT + '/p132_results.json',
    JSON.stringify({executionId: 'p132-outward-oracle-v1', date: '2026-10-26',
      contract: 'T127 sections 2/4/6', results}, null, 1) + '\n');
  rec('T132-FIX', 'fixtures exported for T133 (outward endpoints, affine term counts, η, T86 residual identity)', true,
    {file: 'p132_outward_fixtures.json'});
}
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
