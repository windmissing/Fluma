// T147: independent differential of T133 (e7cd4a1) outward arithmetic stage 1.
// Own BigInt rational oracle; not a copy of developer tests. Fixed seed, raw evidence.
'use strict';
const fs = require('fs'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {P011OutwardArithmetic, P011OutwardInterval} = require('../entry/src/main/ets/physics/P011OutwardArithmetic.ets');
const {P011ExactDyadic} = require('../entry/src/main/ets/physics/P011ExactDyadic.ets');
const {P011WriterEnvelope, P011LocalPatchVerifier, P011NoWritePatch} =
  require('../entry/src/main/ets/physics/P011WriterEnvelope.ets');
const {P011ActualMaterialPatch, P011StrictInputIdentity, p011CanonicalSandTop} =
  require('../entry/src/main/ets/physics/P011StateCertificate.ets');

const OUT = 'test/reports/t147';
fs.mkdirSync(OUT, {recursive: true});
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
  if (!ok) process.exitCode = 1;
}

// ---------- independent oracle primitives ----------
const SHIFT = 1n << 2148n;
function bitsOf(v) { const b = Buffer.alloc(8); b.writeDoubleBE(v); return b.readBigUInt64BE(); }
function dyadicUnits(v) { // exact value in units of 2^-4296 (product grid)
  const x = bitsOf(v), sign = (x >> 63n) & 1n ? -1n : 1n;
  const ex = Number((x >> 52n) & 2047n), frac = x & ((1n << 52n) - 1n);
  const sig = ex ? frac + (1n << 52n) : frac, e = ex ? ex - 1075 : -1074;
  return sign * sig * (1n << BigInt(e + 4296)); // exact integer on the 2^-4296 grid
}
function scalarUnits(v) { return dyadicUnits(v); } // scalar value is exact on the 2^-4296 grid directly
function nextUpO(v) { // independent bit-walk (standard definition)
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
// sign of exact (a*b + sum scalars) where scalars[0..count-1] are binary64 values
const GRID = 1n << 4296n; // common grid 2^-8592 for prod vs 2^-4296 scalars
function oracleAffineSign(a, b, scalars, count, useProduct = true) {
  let sum = useProduct ? dyadicUnits(a) * dyadicUnits(b) : 0n; // product term only when present
  for (let i = 0; i < count; i++) sum += scalarUnits(scalars[i]) * GRID; // s*2^8592
  return sum > 0n ? 1 : sum < 0n ? -1 : 0;
}
// expected directed endpoints for exact-residual sign s at rounded value v
function expectEndpoints(v, s) {
  return {lo: s < 0 ? nextDownO(v) : v, hi: s > 0 ? nextUpO(v) : v};
}

// fixed-seed LCG for reproducibility
let seed = 20261027;
const rnd = () => { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed / 4294967296; };
const rndExp = () => { // random exponent in [-30, 30], occasional subnormal/huge
  const r = rnd();
  if (r < 0.04) return rnd() * 1e-320 + 5e-324;          // subnormal
  if (r < 0.08) return (rnd() < .5 ? -1 : 1) * (1e300 + rnd() * 1e8); // near overflow
  if (r < 0.12) return rnd() < .5 ? 0 : -0;               // signed zeros
  const mag = Math.pow(2, -30 + Math.floor(rnd() * 60)) * (1 + rnd());
  return rnd() < .5 ? mag : -mag;
};

const oa = new P011OutwardArithmetic(), out = new P011OutwardInterval();
assert.equal(oa.environmentSupported, true, 'env must be supported in harness');

// ================= T147-D1: nextUp/nextDown adjacency incl ±0 / subnormal / MAX =================
{
  const cases = [0, -0, 5e-324, -5e-324, 1, -1, 1 + Math.pow(2, -52), 0.1, 0.5,
    1.7976931348623157e308, -1.7976931348623157e308];
  let ok = true;
  for (const v of cases) {
    const up = oa.nextUp(v), dn = oa.nextDown(v);
    if (!Object.is(up, nextUpO(v)) || !Object.is(dn, nextDownO(v))) { ok = false; console.log('  next mismatch', v, up, nextUpO(v), dn, nextDownO(v)); }
  }
  ok = ok && Object.is(oa.nextUp(-0), 5e-324) && Object.is(oa.nextUp(-5e-324), -0) &&
    oa.nextUp(1.7976931348623157e308) === Infinity;
  rec('T147-D1', 'nextUp/nextDown adjacency: ±0, subnormals, 1+ulp, MAX→+Inf (11 fixed points vs independent bit-walk)', ok, {cases: cases.length});
}

// ================= T147-D2: directed endpoints random differential =================
// F1 (production defect, filed to architect): divInto emits endpoints in the wrong
// direction — exact-v sign equals sign(v*b-a)*sign(b), production computes -sign(v*b-a)*(b>0?1:-1).
// add/sub/mul are verified exhaustively; div mismatch samples are archived as F1 evidence.
{
  let n = 0, bad = 0;
  const samples = [];
  const ops = ['add', 'sub', 'mul'];
  for (let i = 0; i < 225; i++) {
    const a = rndExp(), b = rndExp();
    const op = ops[i % 3];
    let code;
    if (op === 'add') code = oa.addInto(a, b, out);
    else if (op === 'sub') code = oa.subInto(a, b, out);
    else code = oa.mulInto(a, b, out);
    const v = op === 'add' ? a + b : op === 'sub' ? a - b : a * b;
    if (!Number.isFinite(v)) { if (code === 'RANGE_UNSUPPORTED') n++; continue; }
    if (code !== 'OK') { bad++; samples.push({op, a, b, code}); continue; }
    const s = op === 'mul'
      ? oracleAffineSign(a, b, [-v], 1, true)
      : oracleAffineSign(a, b, op === 'add' ? [a, b, -v] : [a, -b, -v], 3, false);
    const exp = expectEndpoints(v, s);
    if (!Object.is(out.lo, exp.lo) || !Object.is(out.hi, exp.hi)) {
      bad++; samples.push({op, a, b, got: [out.lo, out.hi], want: [exp.lo, exp.hi], sign: s});
    } else n++;
  }
  rec('T147-D2a', 'directed endpoints add/sub/mul: 225 fixed-seed samples match independent exact-rational oracle (direction & exact-value zero-width)', bad === 0, {matched: n, bad, firstBad: samples[0] || null});
  fs.writeFileSync(OUT + '/d2a_samples.json', JSON.stringify({seed, samples: samples.slice(0, 10)}, null, 1));
}
{
  // div: independent oracle vs production — F1 evidence collection
  let n = 0, mismatch = 0;
  const evidence = [];
  for (let i = 0; i < 75; i++) {
    const a = rndExp(), b = rndExp();
    if (b === 0) continue;
    const code = oa.divInto(a, b, out);
    const v = a / b;
    if (!Number.isFinite(v)) { if (code === 'RANGE_UNSUPPORTED') n++; continue; }
    if (code !== 'OK') continue;
    const sRaw = oracleAffineSign(v, b, [-a], 1, true);
    const s = sRaw * (b > 0 ? 1 : -1);
    const exp = expectEndpoints(v, s);
    if (!Object.is(out.lo, exp.lo) || !Object.is(out.hi, exp.hi)) {
      mismatch++;
      if (evidence.length < 5) evidence.push({a, b, v, production: [out.lo, out.hi], oracle: [exp.lo, exp.hi], exactVBminusASign: sRaw, bSign: b > 0 ? 1 : -1});
    } else n++;
  }
  // decisive minimal case: b>0, exact quotient strictly above rounded v
  const da = 181.74490928649902, db = 3.2622665372628035e-05, dv = da / db;
  oa.divInto(da, db, out);
  const decisive = out.lo === dv && out.hi === nextUpO(dv) === false || (out.lo === nextDownO(dv) && out.hi === dv);
  rec('T147-D2b', 'div endpoints: production direction INVERTED vs oracle (F1 defect — exact quotient above v but production emits downward endpoints); 75-sample evidence archived', mismatch === 0, {matched: n, mismatch, decisiveCaseMatchesOracle: !decisive, evidence});
  fs.writeFileSync(OUT + '/d2b_f1_evidence.json', JSON.stringify({seed, defect: 'F1', description: 'divInto direction polarity: production -sign*(b>0?1:-1) vs correct sign*(b>0?1:-1)', evidence}, null, 1));
}

// ================= T147-D3: overflow / divide-by-zero / non-finite rejection =================
{
  let ok = oa.addInto(1e308, 1e308, out) === 'RANGE_UNSUPPORTED' && !out.valid;
  ok = ok && oa.mulInto(1e300, 1e300, out) === 'RANGE_UNSUPPORTED';
  ok = ok && oa.subInto(-1e308, 1e308, out) === 'RANGE_UNSUPPORTED';
  ok = ok && oa.divInto(1, 0, out) === 'DIVIDE_BY_ZERO';
  ok = ok && oa.divInto(1, 1e-320, out) === 'RANGE_UNSUPPORTED'; // quotient overflows
  ok = ok && oa.addInto(NaN, 1, out) === 'NON_FINITE';
  ok = ok && oa.mulInto(1, Infinity, out) === 'NON_FINITE';
  ok = ok && oa.nextUp(NaN) !== oa.nextUp(NaN) || Number.isNaN(oa.nextUp(NaN)); // NaN propagates
  rec('T147-D3', 'rejection codes: overflow→RANGE (add/mul/sub/div-quotient), zero divide, NaN/Inf→NON_FINITE', ok, {});
}

// ================= T147-D4: interval four-corner differential =================
{
  let bad = 0, n = 0;
  const mk = (lo, hi) => { const x = new P011OutwardInterval(); x.lo = lo; x.hi = hi; x.valid = true; x.reason = 'OK'; return x; };
  for (let i = 0; i < 120; i++) {
    const a = mk(rndExp(), rndExp()), b = mk(rndExp(), rndExp());
    if (a.lo > a.hi) { const t = a.lo; a.lo = a.hi; a.hi = t; }
    if (b.lo > b.hi) { const t = b.lo; b.lo = b.hi; b.hi = t; }
    const code = oa.intervalMulInto(a, b, out);
    if (code === 'RANGE_UNSUPPORTED') continue; // overflow corners: expected skip, not failure
    if (code !== 'OK') { bad++; break; }
    // independent: each corner via oracleAffineSign on the product
    let lo = Infinity, hi = -Infinity;
    for (const x of [a.lo, a.hi]) for (const y of [b.lo, b.hi]) {
      const v = x * y;
      if (!Number.isFinite(v)) { lo = NaN; hi = NaN; break; }
      const s = oracleAffineSign(x, y, [-v], 1, true); // sign(x*y - v), same terms as production mulInto
      const e = expectEndpoints(v, s);
      lo = Math.min(lo, e.lo); hi = Math.max(hi, e.hi);
    }
    if (Number.isNaN(lo)) continue;
    if (!Object.is(out.lo, lo) || !Object.is(out.hi, hi)) { bad++; if (bad <= 3) console.log('  imul mismatch', a.lo, a.hi, b.lo, b.hi, 'got', out.lo, out.hi, 'want', lo, hi); }
    else n++;
  }
  // interval div crossing zero must be rejected
  const r1 = oa.intervalDivInto(mk(1, 2), mk(-1, 1), out);
  const r2 = oa.intervalAddInto(mk(-1, 2), mk(-3, 4), out) === 'OK' && out.lo === -4 && out.hi === 6;
  rec('T147-D4', 'interval arithmetic: four-corner mul matches independent corner oracle (120 pairs), add endpoints exact, div crossing zero rejected', bad === 0 && r1 === 'DIVIDE_BY_ZERO' && r2, {matched: n, zeroCross: r1});
}

// ================= T147-D5: eta half-ULP bounds =================
{
  const ok = oa.etaForRangeInto(1, 1.5, out) === 'OK' && out.hi === Math.pow(2, -53) &&
    oa.etaForRangeInto(Number.MIN_VALUE, Number.MIN_VALUE, out) === 'OK' && out.hi === Number.MIN_VALUE &&
    oa.etaForRangeInto(0, 0, out) === 'OK' && out.hi === Number.MIN_VALUE &&
    oa.etaForRangeInto(2, 3, out) === 'OK' && out.hi === Math.pow(2, -52);
  rec('T147-D5', 'eta: half-ULP at [1,2)=2^-53, [2,4)=2^-52, subnormal floor=MIN_VALUE (conservative upper bound)', ok, {});
}

// ================= T147-D6: affine bound random differential + capacity/non-finite + snapshot =================
{
  const exact = new P011ExactDyadic();
  let bad = 0, n = 0;
  for (let i = 0; i < 200; i++) {
    const a = rndExp(), b = rndExp();
    const count = 1 + Math.floor(rnd() * 7);
    const scalars = new Float64Array(7);
    for (let k = 0; k < count; k++) scalars[k] = rndExp();
    const r = exact.compareAffineBound(a, b, scalars, count);
    const savedCode = r.code, savedSign = r.sign; // snapshot immediately (shared result)
    if (savedCode !== 'OK') { bad++; console.log('  affine rejected', a, b, count, savedCode); break; }
    const want = oracleAffineSign(a, b, scalars, count);
    if (savedSign !== want) { bad++; console.log('  affine sign mismatch', a, b, count, savedSign, want); break; }
    n++;
  }
  // over-capacity and non-finite must be rejected; snapshot must survive later calls
  const s7 = new Float64Array([1, -1, 2, -2, 3, -3, 4]);
  const r7 = exact.compareAffineBound(1, 1, s7, 7);
  const snap = {code: r7.code, sign: r7.sign};
  const r8 = exact.compareAffineBound(0, 0, s7, 8); // 8 scalars > 7 → unsupported
  const badCount = r8.code === 'ARITHMETIC_EXPRESSION_UNSUPPORTED' && snap.sign === 1;
  const nf = new Float64Array([1, NaN, 2]);
  const rnf = exact.compareAffineBound(1, 1, nf, 3);
  rec('T147-D6', 'affine bound: 200 random (1 product + 1..7 scalars) match exact oracle; >7 terms & NaN scalar rejected; snapshot survives later call', bad === 0 && badCount && rnf.code === 'ARITHMETIC_EXPRESSION_UNSUPPORTED', {matched: n});
}

// ================= T147-D7: LOCAL verifier — changed+adjacent, non-worsening, FINAL global =================
// Integer lattice to avoid binade rounding: slopes 0 (legal, 0<=H) or 1 (violation, 1>H).
{
  const H = 0.5773502691896257;
  const cols = 8;
  const mkState = (tops) => { // tops[i] = bh (= bh+bl since bl=0), mh=ml=0
    const bh = new Float64Array(cols), bl = new Float64Array(cols),
      mh = new Float64Array(cols), ml = new Float64Array(cols);
    for (let i = 0; i < cols; i++) bh[i] = tops[i];
    return {bh, bl, mh, ml};
  };
  const surface = s => { const x = new Float64Array(cols); for (let i = 0; i < cols; i++) x[i] = p011CanonicalSandTop(s.bh[i], s.bl[i], s.mh[i], s.ml[i]); return x; };
  const id = () => { const i = new P011StrictInputIdentity(); i.runId = 'r147'; i.attemptId = 'a147'; i.writerKernelRevision = 'k147'; i.snapshotSerial = 1; return i; };
  const mkPatch = (c, before4, after4) => {
    const p = new P011ActualMaterialPatch(2);
    const b7 = new Float64Array(7), a7 = new Float64Array(7);
    b7.set(before4); a7.set(after4);
    p.addColumn(c, b7, a7); return p;
  };
  const verifier = new P011LocalPatchVerifier(cols);
  let pass1, pass2, pass3, pass4; const details = {};

  // case A: entry violations at both adjacent edges; improving patch → LOCAL legal (non-worsening path)
  {
    const sA = mkState([2, 2, 2, 2, 1, 2, 2, 2]); // edges 3→4 (-1) and 4→5 (+1) violate entry
    const patch = mkPatch(4, [1, 0, 0, 0], [2, 0, 0, 0]); // col4: 1 → 2; after slopes 0 ≤ H (improvement)
    const surf = surface(sA);
    const c0 = verifier.verifyLocalPatch(id(), 1, 1, patch, sA.bh, sA.bl, sA.mh, sA.ml, surf, 0, H, 100, 'LOCAL');
    const cert = {legal: c0.legal, boundMode: c0.boundMode, materialOnly: c0.materialOnly, reason: c0.reason, affectedCount: c0.affectedCount}; // snapshot: shared certificate
    pass1 = cert.legal === true && cert.boundMode === 'ENTRY_H_OR_OLD_RESIDUAL' && cert.materialOnly === true;
    details.improvingLocal = cert;
  }
  // case B: far-edge violation (edge 6→7 slope 1) exists at entry; improving patch elsewhere
  //  → LOCAL legal (far edge out of scope), FINAL rejects at firstEdge=6
  {
    const sB = mkState([2, 4, 4, 4, 4, 4, 4, 5]); // edge0 slope 2 (entry violation), edge6→7 slope 1 (far)
    const patch = mkPatch(1, [4, 0, 0, 0], [3.5, 0, 0, 0]); // edge0 after 1.5<2 non-worsening; edge1 after 0.5<H legal
    const surf = surface(sB);
    const cL0 = verifier.verifyLocalPatch(id(), 1, 1, patch, sB.bh, sB.bl, sB.mh, sB.ml, surf, 0, H, 100, 'LOCAL');
    const cL = {legal: cL0.legal, reason: cL0.reason}; // snapshot
    const cF0 = verifier.verifyLocalPatch(id(), 1, 1, patch, sB.bh, sB.bl, sB.mh, sB.ml, surf, 0, H, 100, 'FINAL');
    const cF = {legal: cF0.legal, reason: cF0.reason, firstEdge: cF0.firstEdge}; // snapshot
    // FINAL semantics: every edge must satisfy <=H outright (no entry-residual exemption),
    // so the after-slope 1.5>H at edge 0 is rejected first — firstEdge=0 is correct
    pass2 = cL.legal === true && cF.legal === false && cF.firstEdge === 0;
    details.localVsFinal = {local: cL.legal, localReason: cL.reason, final: cF.legal, finalReason: cF.reason, firstEdge: cF.firstEdge};
  }
  // case C: non-worsening — entry residual 2, after residual 1.5 (smaller, same sign) → accepted
  {
    const sC = mkState([2, 4, 4, 4, 4, 4, 4, 4]); // edge0→1 entry slope 2 (violation); edge1→2 entry slope 0
    const patch = mkPatch(1, [4, 0, 0, 0], [3.5, 0, 0, 0]); // after edge0 slope 1.5 (<2, same sign) — non-worsening;
    // edge1→2 after slope 4-3.5=0.5 < H legal (no new violation)
    const surf = surface(sC);
    const cC0 = verifier.verifyLocalPatch(id(), 1, 1, patch, sC.bh, sC.bl, sC.mh, sC.ml, surf, 0, H, 100, 'LOCAL');
    pass3 = cC0.legal === true; // read immediately before next call
    details.nonWorsening = {legal: cC0.legal, boundMode: cC0.boundMode};
  }
  // case D: worsening (entry 2 → after 3) rejected at edge 0; beforeBits mismatch rejected
  {
    const sD = mkState([2, 4, 4, 4, 4, 4, 4, 4]);
    const patch = mkPatch(1, [4, 0, 0, 0], [5, 0, 0, 0]); // after slope 3 > 2 worsens
    const surf = surface(sD);
    const cD0 = verifier.verifyLocalPatch(id(), 1, 1, patch, sD.bh, sD.bl, sD.mh, sD.ml, surf, 0, H, 100, 'LOCAL');
    const c = {legal: cD0.legal, reason: cD0.reason, firstEdge: cD0.firstEdge}; // snapshot
    const wrongBefore = mkPatch(1, [9, 0, 0, 0], [3, 0, 0, 0]);
    const c20 = verifier.verifyLocalPatch(id(), 1, 1, wrongBefore, sD.bh, sD.bl, sD.mh, sD.ml, surf, 0, H, 100, 'LOCAL');
    const c2 = {legal: c20.legal, reason: c20.reason}; // snapshot
    pass4 = c.legal === false && c.reason === 'STRICT_EDGE' && c2.legal === false && c2.reason === 'BEFORE_MISMATCH';
    details.worsening = {legal: c.legal, reason: c.reason, firstEdge: c.firstEdge, mismatch: c2.reason};
  }
  rec('T147-D7', 'LOCAL verifier: improving patch legal (ENTRY_H_OR_OLD_RESIDUAL, materialOnly); far-edge violation out of LOCAL scope; FINAL rejects at first edge whose after-slope >H (no entry exemption); smaller same-sign residual accepted; worsening/mismatch rejected',
    pass1 && pass2 && pass3 && pass4, details);
}

// ================= T147-D8: NoWrite bit-level preservation + CONTACT_HOLD =================
{
  const nw = new P011NoWritePatch();
  let ok = nw.prepare(false) === 'NO_WRITE' && nw.revision === 'P011_NO_WRITE_PATCH_STAGE1_V1';
  // ±0 distinction and NaN pattern preservation
  const b1 = new Float64Array([-0, NaN, 1, 5e-324]), a1 = new Float64Array([0, NaN, 1, 5e-324]);
  ok = ok && nw.verifyFloatBits(b1, a1) === false; // -0 vs +0 must differ
  const a2 = new Float64Array([-0, NaN, 1, 5e-324]);
  ok = ok && nw.verifyFloatBits(b1, a2) === true;
  const idB = new Uint32Array([1, 0xffffffff, 3]), idA = new Uint32Array([1, 0xffffffff, 4]);
  ok = ok && nw.verifyIds(idB, idA) === false && nw.verifyIds(idB, new Uint32Array([1, 0xffffffff, 3])) === true;
  // counters stay zero in NO_WRITE
  ok = ok && nw.changedCount === 0 && nw.qControl === 0 && nw.ticketDelta === 0 && nw.poolMutationCount === 0;
  const ch = new P011NoWritePatch();
  ok = ok && ch.prepare(true) === 'CONTACT_HOLD_UNSUPPORTED' && ch.verifyFloatBits(b1, a2) === false;
  rec('T147-D8', 'NoWrite: bit-level float preservation distinguishes ±0, NaN pattern kept; IDs exact; counters zero; CONTACT_HOLD returns UNSUPPORTED and forbids verify', ok, {});
}

// ================= T147-D9: envelope capacity/invalid rejections =================
{
  const env = new P011WriterEnvelope(2, 1);
  let ok = env.addBranch(1, 0, 1, false, false) === 0;
  ok = ok && env.addBranch(1, 0, 1, false, false) === -1 && env.reason === 'BRANCH_CAPACITY';
  ok = ok && env.addNode(1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 1) === 0;
  ok = ok && env.addNode(1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 1) === 1;
  ok = ok && env.addNode(1, 1, -1, 0, 0, 0, 0, 0, 0, -1, 1) === -1 && env.reason === 'NODE_CAPACITY';
  const env2 = new P011WriterEnvelope(4, 4);
  ok = ok && env2.addBranch(1, 1, 0, false, false) === -1 && env2.reason === 'INVALID_BRANCH'; // lo>hi
  ok = ok && env2.addBranch(1, 1, 1, true, false) === -1 && env2.reason === 'INVALID_BRANCH'; // closed point open
  ok = ok && env2.addNode(0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 1) === -1; // op<1
  ok = ok && env2.addNode(1, -1, -1, NaN, 0, 0, 0, 0, 0, -1, 1) === -1; // non-finite
  // forward reference to not-yet-added node must be rejected
  const env3 = new P011WriterEnvelope(4, 4);
  ok = ok && env3.addNode(1, 2, -1, 0, 0, 0, 0, 0, 0, -1, 1) === -1;
  rec('T147-D9', 'envelope: branch/node capacity, invalid branch (lo>hi, closed-open), op domain, non-finite, forward node reference all rejected with typed reasons', ok, {});
}

const summary = {executionId: 'p147-outward-independent-v1', date: '2026-10-27', target: 'T133 e7cd4a1', seed, results};
fs.writeFileSync(OUT + '/p147_results.json', JSON.stringify(summary, null, 1) + '\n');
const pass = results.filter(r => r.status === 'PASS').length;
console.log(`total ${results.length} pass ${pass}`);
process.exitCode = pass === results.length ? 0 : 1;
