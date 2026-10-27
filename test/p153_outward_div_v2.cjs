// T153: outward division oracle correction re-test (V2), per architecture-T148 §1–2.
// V1 (test/p147_outward_independent.cjs) is preserved unchanged with its original FAIL;
// this V2 revises the division oracle only: q−v = −(v·b−a)/b, so the directed parameter
// is t = −sRaw·sign(b). The production divInto minus sign is CORRECT and stays.
// Counts attempted/skippedZero/rejectedOverflow/acceptedExact/acceptedInexact/mismatch.
'use strict';
const fs = require('fs'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {P011OutwardArithmetic, P011OutwardInterval} = require('../entry/src/main/ets/physics/P011OutwardArithmetic.ets');
const {P011ExactDyadic} = require('../entry/src/main/ets/physics/P011ExactDyadic.ets');

const OUT = 'test/reports/t153';
fs.mkdirSync(OUT, {recursive: true});
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
  if (!ok) process.exitCode = 1;
}
const SHIFT = 1n << 4296n;
function bitsOf(v) { const b = Buffer.alloc(8); b.writeDoubleBE(v); return b.readBigUInt64BE(); }
function bitsHex(v) { return bitsOf(v).toString(16).padStart(16, '0'); }
function du(v) {
  const x = bitsOf(v), sign = (x >> 63n) & 1n ? -1n : 1n;
  const ex = Number((x >> 52n) & 2047n), frac = x & ((1n << 52n) - 1n);
  const sig = ex ? frac + (1n << 52n) : frac, e = ex ? ex - 1075 : -1074;
  return sign * sig * (1n << BigInt(e + 4296));
}
const GRID = 1n << 4296n;
function nextUpO(v) {
  if (Number.isNaN(v) || v === Infinity) return v;
  if (v === 0) return 5e-324;
  const b = Buffer.alloc(8); b.writeDoubleBE(v);
  let hi = b.readUInt32BE(0), lo = b.readUInt32BE(4);
  if (v > 0) { if (lo === 0xffffffff) { lo = 0; hi++; } else lo++; }
  else if (lo === 0) { lo = 0xffffffff; hi--; } else lo--;
  b.writeUInt32BE(hi >>> 0, 0); b.writeUInt32BE(lo >>> 0, 4);
  return b.readDoubleBE(0);
}
const nextDownO = v => -nextUpO(-v);
// sign of exact (v*b − a) on the common 2^-8592 grid
function signVBminusA(v, b, a) {
  const sum = du(v) * du(b) + du(-a) * GRID;
  return sum > 0n ? 1 : sum < 0n ? -1 : 0;
}
// exact rational containment: does [lo,hi] contain the exact quotient a/b?
// exact q − lo = (a·1 − lo·b)/b scaled; test sign of (a − lo*b)/b and (hi*b − a)/b
function intervalContainsExactQuotient(lo, hi, a, b) {
  // all on the common 2^-8592 grid: du(x)*du(y) = x*y*2^8592, du(x)*GRID = x*2^8592
  const aLo = du(a) * GRID - du(lo) * du(b); // (a − lo·b) * 2^8592
  const aHi = du(hi) * du(b) - du(a) * GRID; // (hi·b − a) * 2^8592
  const bs = b > 0 ? 1n : -1n;
  const qGeLo = (aLo * bs) >= 0n;  // (a−lo·b)/b ≥ 0 ⟺ q ≥ lo
  const qLeHi = (aHi * bs) >= 0n;  // (hi·b−a)/b ≥ 0 ⟺ q ≤ hi
  return qGeLo && qLeHi;
}
function expectEndpoints(v, t) { // t = sign(q−v)
  return {lo: t < 0 ? nextDownO(v) : v, hi: t > 0 ? nextUpO(v) : v};
}

let seedInitial = 20261027;
let seed = seedInitial;
const rnd = () => { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed / 4294967296; };
const rndExp = () => {
  const r = rnd();
  if (r < 0.05) return (rnd() < .5 ? 1 : -1) * (5e-324 + rnd() * 1e-320);      // subnormal
  if (r < 0.08) return rnd() < .5 ? 0 : -0;                                     // signed zeros
  if (r < 0.11) return (rnd() < .5 ? 1 : -1) * (1e300 + rnd() * 1e8);           // near overflow
  const mag = Math.pow(2, -40 + Math.floor(rnd() * 80)) * (1 + rnd());
  return rnd() < .5 ? mag : -mag;
};
const H_CASES = [1/3, -1/3, 1/10, -1/10];

const oa = new P011OutwardArithmetic(), out = new P011OutwardInterval();
assert.equal(oa.environmentSupported, true);

// ================= T153-D2b: div endpoints with corrected oracle =================
{
  const counts = {attempted: 0, skippedZero: 0, rejectedOverflow: 0, acceptedExact: 0, acceptedInexact: 0, mismatch: 0};
  const rows = []; // every valid sample archived with raw bits
  for (let i = 0; i < 400; i++) {
    const a = rndExp(), b = rndExp();
    counts.attempted++;
    if (b === 0) { counts.skippedZero++; continue; }
    const code = oa.divInto(a, b, out);
    const v = a / b;
    if (!Number.isFinite(v)) {
      if (code === 'RANGE_UNSUPPORTED') counts.rejectedOverflow++;
      else counts.mismatch++;
      continue;
    }
    if (code !== 'OK') { counts.rejectedOverflow++; continue; }
    const sRaw = signVBminusA(v, b, a);
    const t = -sRaw * (b > 0 ? 1 : -1); // T148 §1: q−v = −(v·b−a)/b
    const exp = expectEndpoints(v, t);
    const loOK = Object.is(out.lo, exp.lo), hiOK = Object.is(out.hi, exp.hi);
    // rational containment cross-check on the production interval
    const contains = intervalContainsExactQuotient(out.lo, out.hi, a, b);
    const exact = sRaw === 0;
    if (exact) counts.acceptedExact++; else counts.acceptedInexact++;
    if (!loOK || !hiOK || !contains) {
      counts.mismatch++;
      rows.push({a, b, v, loBits: bitsHex(out.lo), hiBits: bitsHex(out.hi),
        wantLoBits: bitsHex(exp.lo), wantHiBits: bitsHex(exp.hi),
        sRaw, t, containsExactQuotient: contains, exact});
    } else {
      rows.push({a, b, v, loBits: bitsHex(out.lo), hiBits: bitsHex(out.hi),
        wantLoBits: bitsHex(exp.lo), wantHiBits: bitsHex(exp.hi),
        sRaw, t, containsExactQuotient: contains, exact});
    }
  }
  // fixed cases: ±1/3, ±1/10 (numerator and denominator), positive & negative divisors,
  // exact quotient, underflow to ±0/subnormal
  const fixed = [];
  const nums = [1/3, -1/3, 1/10, -1/10, 1, -1, 5e-324, -5e-324];
  for (const x of nums) for (const y of [1/3, -1/3, 1/10, -1/10, 3, -3, 1e10, -1e10]) {
    const code = oa.divInto(x, y, out);
    const v = x / y;
    if (!Number.isFinite(v)) { fixed.push({x, y, code, ok: code === 'RANGE_UNSUPPORTED'}); continue; }
    const sRaw = signVBminusA(v, y, x);
    const t = -sRaw * (y > 0 ? 1 : -1);
    const exp = expectEndpoints(v, t);
    const ok = Object.is(out.lo, exp.lo) && Object.is(out.hi, exp.hi) &&
      intervalContainsExactQuotient(out.lo, out.hi, x, y);
    fixed.push({x, y, v, lo: bitsHex(out.lo), hi: bitsHex(out.hi), t, ok});
    if (!ok) counts.mismatch++;
  }
  // exact-quotient case: a=4, b=2 → q=2 exactly, endpoints [2,2]
  {
    oa.divInto(4, 2, out);
    if (!(Object.is(out.lo, 2) && Object.is(out.hi, 2))) counts.mismatch++;
    // underflow: tiny/tiny → subnormal or zero quotient with directed endpoints
    oa.divInto(5e-324, 2, out); // q = 2.5e-324 → RN = 0 (even) → t decides ±0 endpoints
    // zero divide & overflow rejections
    if (oa.divInto(1, 0, out) !== 'DIVIDE_BY_ZERO') counts.mismatch++;
    if (oa.divInto(1e300, 1e-300, out) !== 'RANGE_UNSUPPORTED') counts.mismatch++;
  }
  rec('T153-D2b', 'div endpoints V2: corrected oracle t=−sRaw·sign(b); 400 random + fixed ±1/3/±1/10/±0/subnormal matrix; production interval contains exact quotient (rational cross-multiply) on every accepted sample; counts separated',
    counts.mismatch === 0, {...counts, seedInitial, seedFinal: seed, mismatchRows: rows.filter(r => !r.containsExactQuotient).slice(0, 5)});
  fs.writeFileSync(OUT + '/d2b_v2_samples.json', JSON.stringify({seedInitial, seedFinal: seed, counts, fixed}, null, 1) + '\n');
}

// ================= T153-D2c: T148 decisive case corrected direction =================
{
  const a = 181.74490928649902, b = 3.2622665372628035e-05, v = a / b;
  const code = oa.divInto(a, b, out);
  // T148: v·b−a > 0 → q−v < 0 → correct endpoints [nextDown(v), v]
  const sRaw = signVBminusA(v, b, a);
  const t = -sRaw * (b > 0 ? 1 : -1);
  const exp = expectEndpoints(v, t);
  const bitMatch = Object.is(out.lo, exp.lo) && Object.is(out.hi, exp.hi);
  const contains = intervalContainsExactQuotient(out.lo, out.hi, a, b);
  // verify T148's exact fractions via independent integer arithmetic
  // v*b−a = 503444847/37778931862957161709568 > 0 (from T148 doc)
  rec('T153-D2c', 'T148 decisive case: production [nextDown(v), v] IS the correct containment (oracle V1 claim [v,nextUp(v)] excluded the exact quotient); bit-level endpoints + rational containment confirmed',
    code === 'OK' && bitMatch && contains && sRaw > 0,
    {v, loBits: bitsHex(out.lo), hiBits: bitsHex(out.hi), sRaw, t, contains});
}

// ================= T153-D4: non-crossing intervalDiv four corners incl all-negative divisors =================
{
  let bad = 0, n = 0; const detail = [];
  const mk = (lo, hi) => { const x = new P011OutwardInterval(); x.lo = lo; x.hi = hi; x.valid = true; x.reason = 'OK'; return x; };
  const cases = [
    [[1, 2], [2, 4]], [[-2, -1], [-4, -2]],    // positive / all-negative divisors
    [[-1, 2], [1, 3]], [[1, 2], [-4, -1]],     // mixed signs, divisor strictly negative
    [[2, 4], [0.5, 1]], [[-4, -2], [-1, -0.5]] // reciprocal signs
  ];
  for (const [[al, ah], [bl, bh]] of cases) {
    const code = oa.intervalDivInto(mk(al, ah), mk(bl, bh), out);
    if (code !== 'OK') { bad++; detail.push({al, ah, bl, bh, code}); continue; }
    // independent: each corner quotient, directed endpoints via corrected t, then union
    let lo = Infinity, hi = -Infinity;
    for (const x of [al, ah]) for (const y of [bl, bh]) {
      const v = x / y;
      const sRaw = signVBminusA(v, y, x);
      const t = -sRaw * (y > 0 ? 1 : -1);
      const e = expectEndpoints(v, t);
      lo = Math.min(lo, e.lo); hi = Math.max(hi, e.hi);
      if (!intervalContainsExactQuotient(e.lo, e.hi, x, y)) { bad++; detail.push({corner: [x, y]}); }
    }
    if (!Object.is(out.lo, lo) || !Object.is(out.hi, hi)) {
      bad++; detail.push({al, ah, bl, bh, got: [out.lo, out.hi], want: [lo, hi]});
    } else n++;
  }
  // crossing-zero and endpoint-zero divisors must still be rejected
  const r1 = oa.intervalDivInto(mk(1, 2), mk(-1, 1), out);
  const r2 = oa.intervalDivInto(mk(1, 2), mk(0, 1), out);
  const r3 = oa.intervalDivInto(mk(1, 2), mk(-1, 0), out);
  rec('T153-D4', 'non-crossing intervalDiv: four-corner exact containment (6 sign matrices incl all-negative divisors); crossing-zero and zero-endpoint divisors rejected',
    bad === 0 && r1 === 'DIVIDE_BY_ZERO' && r2 === 'DIVIDE_BY_ZERO' && r3 === 'DIVIDE_BY_ZERO',
    {matched: n, bad, detail: detail.slice(0, 3), rejections: [r1, r2, r3]});
}

// ================= T153-D5: shared result snapshot discipline (div path) =================
{
  const exact = new P011ExactDyadic();
  const r1 = exact.compareAffineBound(3, 2, new Float64Array([-1]), 1);
  const snap = {code: r1.code, sign: r1.sign}; // snapshot before reuse
  exact.compareAffineBound(1, 1, new Float64Array([0]), 1); // overwrite shared result
  const ok = snap.code === 'OK' && snap.sign === 1;
  rec('T153-D5', 'shared result snapshot: div-path affine call result copied before next compare survives', ok, {snap});
}

const summary = {executionId: 'p153-outward-div-v2', date: '2026-10-27', target: 'T148 §1–2 oracle correction', seedInitial, seedFinal: seed, results};
fs.writeFileSync(OUT + '/p153_results.json', JSON.stringify(summary, null, 1) + '\n');
const pass = results.filter(r => r.status === 'PASS').length;
console.log(`total ${results.length} pass ${pass}`);
process.exitCode = pass === results.length ? 0 : 1;
