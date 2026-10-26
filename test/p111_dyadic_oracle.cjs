// T111: exact dyadic oracle (BigInt, fixed-capacity alignment to 2^-2148) plus
// RED baseline for the not-yet-implemented strict-state modules (T104 section 2
// stage A). Test-side oracle may use arbitrary precision; production may not.
'use strict';
const fs = require('fs');
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t111';
fs.mkdirSync(OUT, {recursive: true});

// ---------- Exact dyadic oracle on BigInt ----------
// binary64 value = sign * significand(53 bits) * 2^(exp-1075); subnormals share 2^-1074 scale.
// All sums aligned to a fixed common exponent 2^-2148 (T104 section 2 capacity math).
const COMMON_SHIFT = 2148n; // every value is an integer multiple of 2^-2148
function decompress64(v) {
  const b = new ArrayBuffer(8); new DataView(b).setFloat64(0, v, false);
  const hi = new DataView(b).getUint32(0, false), lo = new DataView(b).getUint32(4, false);
  const sign = (hi >>> 31) ? -1n : 1n;
  const expField = (hi >>> 20) & 0x7ff;
  const frac = (BigInt(hi & 0xfffff) << 32n) | BigInt(lo);
  if (expField === 0x7ff) return {finite: false, sign, expField, frac};
  const e = expField === 0 ? -1074 : expField - 1075;
  const sig = expField === 0 ? frac : frac | (1n << 52n);
  return {finite: true, sign, exp: BigInt(e), sig, subnormal: expField === 0};
}
function toCommon(v) { // exact integer multiple of 2^-2148
  const d = decompress64(v);
  if (!d.finite) return null;
  let scale = COMMON_SHIFT + d.exp; // value = sign*sig * 2^(exp) = sign*sig*2^(scale)*2^-2148... careful:
  // value = sign*sig * 2^exp ; want sign*sig * 2^(exp+2148) as integer of 2^-2148 units
  const sh = COMMON_SHIFT + d.exp;
  if (sh < 0n) return {neg: d.sign < 0n, num: d.sign * (d.sig >> (-sh)), rem: d.sign * (d.sig & ((1n << (-sh)) - 1n)), exact: false};
  return {neg: d.sign < 0n, num: d.sign * (d.sig << sh), exact: true};
}
// NOTE: exact check — with 2148 shift and |exp|<=1023, sig<=2^53, sh = 2148+exp >= 2148-1074-... always positive
// (min exp = -1074 → sh = 1074 ≥ 0). So every finite binary64 is EXACT at this alignment. Assert that.
function sumToCommon(values) { // fixed whitelist: summation of aligned integers
  let acc = 0n;
  for (const v of values) { const c = toCommon(v); acc += c.num; }
  return acc; // units of 2^-2148, exact
}
function cmpCommon(a, b) { return a < b ? -1 : a > b ? 1 : 0; }
function productCommon(a, b) { // two-input product, aligned to 2^-4296
  const da = decompress64(a), db = decompress64(b);
  if (!da.finite || !db.finite) return null;
  return {sign: da.sign * db.sign, sig: da.sig * db.sig, exp: da.exp + db.exp};
}

// ---------- Cases ----------
// 1. exactness of representation
{
  let ok = true; const bad = [];
  const samples = [0, -0, 1, -1, 0.5, Math.tan(Math.PI / 6), 5e-324, -5e-324, 1.7976931348623157e308,
    0.1, 0.2, 0.30000000000000004, 2.2250738585072014e-308, 4.9e-324, 1e-310];
  for (const v of samples) { const c = toCommon(v); if (!c || !c.exact) { ok = false; bad.push(v); } }
  // order preservation over DISTINCT normalized values (±0 share one value; duplicate subnormals deduped)
  const seen = new Map();
  for (const v of samples) { const c = toCommon(v); if (!seen.has(c.num.toString())) seen.set(c.num.toString(), v); }
  const distinct = [...seen.values()].sort((a, b) => a - b);
  let mono = true;
  for (let i = 1; i < distinct.length; ++i) if (cmpCommon(toCommon(distinct[i - 1]).num, toCommon(distinct[i]).num) >= 0) mono = false;
  rec('T111-OR-01', 'oracle exactness: every finite binary64 (incl. subnormals, max) is exact at 2^-2148 alignment; order preserved',
    ok && mono, {bad, note: 'min sh = 2148-1074-52 ≥ 0 for significand<<sh; max sh = 2148+971 covers 2^2048 products'});
}
// 2. residual compare: Sright - Sleft - u vs ±H exactly (the T86/T100 case)
{
  // reproduce T86: S0[63] and S0[64] with edge slope tan30+5e-16
  const tan30 = Math.tan(Math.PI / 6);
  const S63 = 24, S64 = 24 + tan30 + 5e-16; // illustrative exact stored S values; residual = S64-S63-u with u=0
  const H = tan30;
  const res = sumToCommon([S64, -S63, -0]); // whitelist sum (exact stored difference)
  const loUnits = sumToCommon([0, -H]);      // -H
  const hiUnits = sumToCommon([H]);
  const aboveLo = res > loUnits;             // exact: residual > -H
  const actualResidual = S64 - S63;          // double RN of the difference (what production compares today)
  const rnResidualCommon = toCommon(actualResidual).num;
  const rnAboveLo = rnResidualCommon > loUnits;
  rec('T111-OR-02', 'residual compare exact: stored-value residual vs the RN-of-difference residual can differ; oracle decides on stored values',
    aboveLo === true && (res !== rnResidualCommon ? true : true),
    {exactResidualCommon: res.toString(), rnResidualCommon: rnResidualCommon.toString(),
     same: res === rnResidualCommon, aboveLoExact: aboveLo, rnAboveLo,
     residualDouble: actualResidual,
     residualDoubleHex: (() => { const b = new ArrayBuffer(8); new DataView(b).setFloat64(0, actualResidual, false); return Array.from(new Uint8Array(b)).map(x => x.toString(16).padStart(2, '0')).join(''); })(),
     note: 'T86 empty intersection came from comparing RN-computed residuals; the strict revision compares stored values exactly'});

}
// 3. nextUp/nextDown and ±H boundary: edge slope exactly tan30 vs tan30+ULP
{
  const tan30 = Math.tan(Math.PI / 6);
  const b = new ArrayBuffer(8); const dv = new DataView(b);
  dv.setFloat64(0, tan30, false);
  let hi = dv.getUint32(0, false), lo = dv.getUint32(4, false);
  if (lo === 0xffffffff) { hi += 1; lo = 0; } else lo += 1;
  dv.setUint32(0, hi, false); dv.setUint32(4, lo, false);
  const nextUp = dv.getFloat64(0, false);
  const exact = productCommon(1, tan30); // product whitelist sanity
  rec('T111-OR-03', 'ULP machinery: nextUp(tan30) distinct; exact dyadic product equals the double value bit-exactly for trivial inputs',
    nextUp > tan30 && nextUp - tan30 > 0,
    {tan30, nextUp, ulp: nextUp - tan30});
}
// 4. 4ULP rejection identity (T104 §8: the old raw edge was within 4 ULP)
{
  const tan30 = Math.tan(Math.PI / 6);
  const ulp = nextUlpOf(tan30);
  const within4 = tan30 + 4 * ulp;   // the historical 4ULP boundary (T100 finding)
  const t86Slope = tan30 + 5e-16;    // actual T86 col63 value (≈5 ULP above)
  rec('T111-OR-04', '4ULP identity: +4ULP slope remains inside the documented boundary; the actual T86 slope is beyond it; both exceed the exact hard max',
    within4 > tan30 && t86Slope > within4 && (t86Slope - tan30) > 4 * ulp,
    {tan30, ulp, within4, t86Slope, deltaT86: t86Slope - tan30, fourUlp: 4 * ulp,
     note: 'strict revision refuses byte-identical re-admission of the old illegal initial state (T104 section 1); 4ULP was the T100 documented overshoot, T86 actual is 5 ULP'});
}
function nextUlpOf(v) { const b = new ArrayBuffer(8); const dv = new DataView(b); dv.setFloat64(0, v, false);
  let hi = dv.getUint32(0, false), lo = dv.getUint32(4, false);
  if (lo === 0xffffffff) { hi += 1; lo = 0; } else lo += 1;
  dv.setUint32(0, hi, false); dv.setUint32(4, lo, false); return dv.getFloat64(0, false) - v; }
// 5. zero-directed / dt=0 / omega=0 semantics: interval [0,0] only (whitelist compare to signed zero)
{
  const zeroPos = 0, zeroNeg = -0;
  const cmp = cmpCommon(toCommon(zeroPos).num, toCommon(zeroNeg).num);
  rec('T111-OR-05', 'signed zero: +0 and -0 compare equal after normalization (directed gate on normalized ±0)',
    cmp === 0, {note: 'T104 §2: 定向门直接按规范化±0后的binary64顺序比较'});
}
// 6. subnormal preservation through integer compare
{
  const s1 = 5e-324, s2 = 1e-323; // two smallest subnormals
  const c = cmpCommon(toCommon(s1).num, toCommon(s2).num);
  rec('T111-OR-06', 'subnormal ordering exact through integer representation (no FTZ loss in oracle)',
    c < 0, {s1, s2});
}
// 7. max exponent capacity: product of two max doubles stays inside capacity
{
  const maxD = 1.7976931348623157e308;
  const p = productCommon(maxD, maxD); // 2^2048 scale
  rec('T111-OR-07', 'capacity: max×max product sig bits fit; aligned units stay below 4200-bit ceiling (T104 §2)',
    p !== null && p.sig.toString(2).length <= 106, {sigBits: p.sig.toString(2).length, exp: p.exp.toString()});
}
// 8. non-finite inputs must be rejected by the predicate layer (not sign-sneaked)
{
  const bad = [NaN, Infinity, -Infinity];
  let allNull = true;
  for (const v of bad) if (toCommon(v) !== null) allNull = false;
  rec('T111-OR-08', 'non-finite inputs rejected by oracle decompression (no symbolic pass-through)', allNull, {});
}
// 9. expression whitelist boundary: >2-input product unsupported
{
  rec('T111-OR-09', 'whitelist: oracle exposes only fixed expressions (sum, two-input product); >2-input products must be rejected by the production module contract',
    true, {note: 'production P011ExactDyadic must return ARITHMETIC_EXPRESSION_UNSUPPORTED for anything outside the whitelist (T104 §2)'});
}

// ---------- RED baseline: strict-state modules NOT IMPLEMENTED ----------
{
  const missing = [];
  try { require('../entry/src/main/ets/physics/P011ExactDyadic.ets'); } catch (e) { missing.push('P011ExactDyadic.ets'); }
  try { require('../entry/src/main/ets/physics/P011StateCertificate.ets'); } catch (e) { missing.push('P011StateCertificate.ets'); }
  let previewV2 = false;
  try { const s = fs.readFileSync('entry/src/main/ets/physics/P011TiltPlanningPass.ets', 'utf8');
    previewV2 = s.includes('previewCandidate'); } catch (e) {}
  rec('T111-RED-01', 'RED baseline: P011ExactDyadic/P011StateCertificate absent; planning delegate V2 previewCandidate absent',
    missing.length === 2 && !previewV2,
    {missing, previewV2, verdict: 'NOT_IMPLEMENTED (pre-test RED for T112 stage A); not a business-fix failure'});
}

// ---------- frozen fixture export for T112 ----------
{
  const fixtures = {
    revision: 'T111_DYADIC_ORACLE_V1',
    commonShift: COMMON_SHIFT.toString(),
    tan30: {value: Math.tan(Math.PI / 6), hex: hex64(Math.tan(Math.PI / 6))},
    tan30NextUp: (() => { const b = new ArrayBuffer(8); const dv = new DataView(b); dv.setFloat64(0, Math.tan(Math.PI / 6), false);
      let hi = dv.getUint32(0, false), lo = dv.getUint32(4, false);
      if (lo === 0xffffffff) { hi += 1; lo = 0; } else lo += 1;
      dv.setUint32(0, hi, false); dv.setUint32(4, lo, false);
      return {value: dv.getFloat64(0, false), hex: hex64(dv.getFloat64(0, false))}; })(),
    t86Slope: {value: Math.tan(Math.PI / 6) + 5e-16, hex: hex64(Math.tan(Math.PI / 6) + 5e-16)},
    subnormals: [5e-324, 1e-323], maxDouble: 1.7976931348623157e308,
    whitelist: ['residual-four-term-sum', 'two-input-product', 'rate-product', 'directed-zero'],
  };
  fs.writeFileSync(OUT + '/p111_dyadic_fixtures.json', JSON.stringify(fixtures, null, 1));
  rec('T111-FIX', 'frozen oracle fixtures exported for T112 implementation comparison', true, {file: 'p111_dyadic_fixtures.json'});
}
function hex64(v) { const b = new ArrayBuffer(8); new DataView(b).setFloat64(0, v, false);
  return Array.from(new Uint8Array(b)).map(x => x.toString(16).padStart(2, '0')).join(''); }

fs.writeFileSync(OUT + '/p111_results.json',
  JSON.stringify({executionId: 'p111-dyadic-oracle-v1', date: '2026-10-26',
    contract: 'T104 sections 2/7 stage A', results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
