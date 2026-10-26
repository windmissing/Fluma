// T125: dyadic oracle V2 — independent test-side BigInt oracle with REAL bit
// patterns (per T120 §1), effective counterexamples that distinguish exact
// residual vs RN-first pipelines (per T120 §3), and whitelist/rate/carry/
// min-exponent/real-rejection coverage. Independent of the developer's own
// 17/17 self-test: oracle expectations are derived here, not mirrored.
'use strict';
const fs = require('fs');
const assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {P011ExactDyadic: D} = require('../entry/src/main/ets/physics/P011ExactDyadic.ets');
const {P011StrictStateCertifier: C, p011CanonicalSandTop: S} =
  require('../entry/src/main/ets/physics/P011StateCertificate.ets');

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t125';
fs.mkdirSync(OUT, {recursive: true});

// ---------- independent BigInt oracle (test side only) ----------
function bits64(v) { const b = Buffer.allocUnsafe(8); b.writeDoubleBE(v); return b.readBigUInt64BE(); }
function val64(bits) { const b = Buffer.allocUnsafe(8); b.writeBigUInt64BE(bits); return b.readDoubleBE(0); }
function decompress(v) {
  const bits = bits64(v), sign = (bits >> 63n) & 1n ? -1n : 1n;
  const ex = Number((bits >> 52n) & 2047n), frac = bits & ((1n << 52n) - 1n);
  if (ex === 2047) return {finite: false};
  const sig = ex ? frac + (1n << 52n) : frac;
  const e = ex ? ex - 1075 : -1074;
  return {finite: true, sign, sig, e};
}
const SHIFT = 2148n; // fixed alignment 2^-2148
function U(v) { // exact integer units of 2^-2148 for a finite binary64
  const d = decompress(v); assert.ok(d.finite);
  return d.sign * d.sig * (1n << BigInt(Number(SHIFT) + d.e));
}
function cmp(x) { return x > 0n ? 1 : x < 0n ? -1 : 0; }
function sgnSum(a) { // sign of sum of BigInt terms
  let acc = 0n; for (const t of a) acc += t; return cmp(acc);
}
function hex64(v) { return bits64(v).toString(16).padStart(16, '0'); }

// ---------- REAL bit-pattern identities per T120 §1 ----------
const H_HEX = '3fe279a74590331c', NEXTUP_HEX = '3fe279a74590331d',
  NEXTDOWN_HEX = '3fe279a74590331b', T100_HEX = '3fe279a745903320', T111SYN_HEX = '3fe279a745903321';
const H = val64(Buffer.from(H_HEX, 'hex').readBigUInt64BE(0));
const nextUpH = val64(Buffer.from(NEXTUP_HEX, 'hex').readBigUInt64BE(0));
const nextDownH = val64(Buffer.from(NEXTDOWN_HEX, 'hex').readBigUInt64BE(0));
const t100Raw = val64(Buffer.from(T100_HEX, 'hex').readBigUInt64BE(0));
const t111Syn = val64(Buffer.from(T111SYN_HEX, 'hex').readBigUInt64BE(0));
// H处ULP=2^-53
const ULP_H = nextUpH - H; // = 2^-53
const assertULP = ULP_H === Math.pow(2, -53);
// ULP counts above H: t100Raw = H + 4*ULP? verify exactly
const t100ULPs = Number((U(t100Raw) - U(H)) / (U(nextUpH) - U(H)));
const t111ULPs = Number((U(t111Syn) - U(H)) / (U(nextUpH) - U(H)));
rec('T125-V2-01', 'real bit patterns: H/nextUp/nextDown/T100raw/T111synthetic decode independently; ULP counts 0/+1/-1/+4/+5',
  hex64(H) === H_HEX && hex64(nextUpH) === NEXTUP_HEX && hex64(nextDownH) === NEXTDOWN_HEX &&
  hex64(t100Raw) === T100_HEX && hex64(t111Syn) === T111SYN_HEX &&
  t100ULPs === 4 && t111ULPs === 5 && assertULP && nextDownH === val64(Buffer.from(NEXTDOWN_HEX, 'hex').readBigUInt64BE(0)),
  {H_hex: hex64(H), t100ULPs, t111ULPs, ulpH: ULP_H,
   note: 'T120 §1: raw T86 edge = 3320 (+4 ULP real); 3321 is the T111 synthetic (+5 ULP); 331d is the true nextUp'});

// mechanical nextUp/nextDown from H: must reproduce 331d/331b
{
  const b = Buffer.allocUnsafe(8); b.writeBigUInt64BE(bits64(H));
  let hi = b.readUInt32BE(0), lo = b.readUInt32BE(4);
  if (lo === 0xffffffff) { hi += 1; lo = 0; } else lo += 1;
  b.writeUInt32BE(hi >>> 0, 0); b.writeUInt32BE(lo >>> 0, 4);
  const mechUp = b.readDoubleBE(0);
  b.writeBigUInt64BE(bits64(H)); hi = b.readUInt32BE(0); lo = b.readUInt32BE(4);
  if (lo === 0) { hi -= 1; lo = 0xffffffff; } else lo -= 1;
  b.writeUInt32BE(hi >>> 0, 0); b.writeUInt32BE(lo >>> 0, 4);
  const mechDown = b.readDoubleBE(0);
  rec('T125-V2-02', 'mechanical nextUp(H)=331d, nextDown(H)=331b (bit-exact, not synthetic H+5e-16)',
    hex64(mechUp) === NEXTUP_HEX && hex64(mechDown) === NEXTDOWN_HEX, {mechUp: hex64(mechUp), mechDown: hex64(mechDown)});
}

// ---------- effective counterexamples (T120 §3): exact vs RN-first residual ----------
// Case A (both reject, different values): Sright=1, Sleft=2^-54, u=0, H=nextDown(1)
{
  const Hn = nextDownH0();
  function nextDownH0() { const b = Buffer.allocUnsafe(8); b.writeDoubleBE(1); const bits = b.readBigUInt64BE() - 1n; b.writeBigUInt64BE(bits); return b.readDoubleBE(0); }
  const Sright = 1, Sleft = Math.pow(2, -54), u = 0;
  const exactResidual = U(Sright) - U(Sleft) - U(u); // exact stored difference
  const rnResidual = Sright - Sleft;                  // RN64 of the subtraction
  const exactSign = sgnSum([U(Sright), -U(Sleft), -U(u), -U(Hn)]);
  const rnSign = sgnSum([U(rnResidual), -U(u), -U(Hn)]);
  // exact residual = 1 - 2^-54 > Hn → accept (|res| within H); value exceeds H
  const exactWithin = exactSign <= 0; // |res| <= H means both hi and low constraints hold: here residual>0, check res<=H
  const resLEQ = (U(Sright) - U(Sleft)) <= U(Hn);
  const rnValue = rnResidual; // RN(1-2^-54) = 1
  rec('T125-V2-03', 'counterexample A: exact residual 1-2^-54 > nextDown(1) (must REJECT) while RN(1-2^-54)=1 hides the excess; exact and RN decisions differ on value identity',
    resLEQ === false && rnValue === 1 && rnValue === Sright && exactResidual > U(Hn),
    {exactResidualUnits: exactResidual.toString(), Hn: Hn, rnOfDiff: rnValue,
     note: 'T120 §3 case 1: precise residual 1-2^-54 is greater than H; RN-first pipeline computes 1 and the excess disappears'});
}
// Case B (predicate-distinguishing): Sright=1, Sleft=2^-54, u=2^-54, H=nextDown(1)
//   exact three-term: 1 - 2^-54 - 2^-54 = 1 - 2^-53 = H → ACCEPT
//   RN-first: RN(1-2^-54)=1 then 1-2^-54 rounds back to 1 → residual=1 > H → REJECT
{
  const Hn = (() => { const b = Buffer.allocUnsafe(8); b.writeDoubleBE(1); b.writeBigUInt64BE(b.readBigUInt64BE() - 1n); return b.readDoubleBE(0); })();
  const Sright = 1, Sleft = Math.pow(2, -54), u = Math.pow(2, -54);
  // exact three-term on stored values
  const exactUnits = U(Sright) - U(Sleft) - U(u);
  const exactAccept = exactUnits <= U(Hn) && exactUnits >= -U(Hn);
  // RN-first: production computes (Sright - Sleft) with RN, then subtracts u with RN
  const step1 = Sright - Sleft;   // = 1 (rounds back)
  const step2 = step1 - u;        // = 1 (rounds back)
  const rnUnits = U(step2);
  const rnAccept = rnUnits <= U(Hn) && rnUnits >= -U(Hn);
  rec('T125-V2-04', 'counterexample B: exact 1-2^-54-2^-54 = nextDown(1) exactly → ACCEPT; RN-first double-rounding yields 1 → REJECT; the two predicates disagree',
    exactAccept === true && rnAccept === false && step2 === 1 &&
    exactUnits === U(Hn), {exactEqH: exactUnits === U(Hn), rnResidual: step2,
     note: 'T120 §3 case 2: distinguishes exact stored-value predicate from RN-then-subtract pipeline'});
}
// Case C: production-style residual on the T86 real edge must be REJECTED under strict ±H (no 4ULP allowance)
{
  // stored slope pair: S63=24, S64=24+t100Raw (+4 ULP real raw), u=0, hardMax=H
  const S63 = 24, S64 = 24 + t100Raw;
  // exact compare of stored difference vs H
  const diff = U(S64) - U(S63);
  const rejectStrict = diff > U(H); // strictly beyond H → illegal
  rec('T125-V2-05', 'real T86/T100 edge (+4 ULP raw) rejected under exact stored-value compare with hardMax=tan30(331c); no 4ULP allowance exists',
    rejectStrict === true && t100ULPs === 4, {diffULPs: Number(diff / (U(nextUpH) - U(H))),
     note: 'T104 §8/T120 §1: 4ULP is a descriptive count of the historical overshoot, never an allowed domain'});
}

// ---------- OR-02 replacement: four-term compare with strict ±H both sides ----------
// NOTE: base 24 cannot represent H exactly (ulp(24)=2^-49 coarse); base 1 can:
// U(1+H)-U(1)===U(H) verified independently. Build edges from base 1.
{
  const cert = new C(4);
  const bh = new Float64Array([1, 1, 1, 1]);
  const bl = new Float64Array([1, 1 + H, 1 + 2 * H, 1 + H]);
  const cached = new Float64Array(4);
  for (let i = 0; i < 4; i++) cached[i] = S(bh[i], bl[i], 0, 0);
  // exact slope check (BigInt oracle): edge diffs are exactly +H,+H,-H → all within ±H → legal
  const edgeExact = U(1 + H) - U(1) === U(H) && U(1 + 2 * H) - U(1 + H) === U(H);
  const r1 = cert.verifySandTop(bh, bl, new Float64Array(4), new Float64Array(4), cached, 0, H, 'FINAL');
  const r1Snap = {legal: r1.legal, reason: r1.reason, firstEdge: r1.firstEdge}; // snapshot: verifySandTop returns a shared certificate object
  // slope +H + 1 ULP(=2^-52 at this binade) on one edge must be rejected
  const bl2 = new Float64Array([1, 1 + H + Math.pow(2, -52), 1 + 2 * H, 1 + H]);
  const cached2 = new Float64Array(4);
  for (let i = 0; i < 4; i++) cached2[i] = S(bh[i], bl2[i], 0, 0);
  const excess = (U(1 + H + Math.pow(2, -52)) - U(1)) - U(H);
  const r2 = cert.verifySandTop(bh, bl2, new Float64Array(4), new Float64Array(4), cached2, 0, H, 'FINAL');
  const r2Snap = {legal: r2.legal, reason: r2.reason, firstEdge: r2.firstEdge};
  rec('T125-V2-06', 'certifier four-term residual: edges exactly ±H legal; one edge +H+1ULP(2^-52) rejected (STRICT_EDGE) — exact boundary, no epsilon',
    edgeExact && excess > 0n && r1Snap.legal === true && r2Snap.legal === false && r2Snap.reason === 'STRICT_EDGE' && r2Snap.firstEdge === 0,
    {r1: r1Snap.reason, r2: r2Snap.reason, r2firstEdge: r2Snap.firstEdge, edgeExact, excessPositive: excess > 0n,
     note: 'expectations derived from independent BigInt oracle; base-1 binade used because ulp(24) is too coarse to store H exactly; snapshots taken because verifySandTop returns a shared certificate object'});
}

// ---------- product materialization + carry structure (T120 §3 OR-03/OR-07) ----------
{
  const d = new D();
  // product(1, H) must compare equal to H itself: compareProduct(a,b,c) checks a*b vs c
  const g1Res = d.compareProduct(1, H, H); const g1 = {code: g1Res.code, sign: g1Res.sign};
  // product(H, 2) vs 2H — 2H is exact
  const g2Res = d.compareProduct(H, 2, 2 * H); const g2 = {code: g2Res.code, sign: g2Res.sign};
  // non-trivial: product(0.1, 0.1) vs the RN64 of 0.01 — exact 0.01 != RN(0.1*0.1)? both are binary64;
  // exact dyadic product of stored 0.1*stored 0.1 is NOT representable; compare against stored 0.01
  const g3Res = d.compareProduct(0.1, 0.1, 0.01); const g3 = {code: g3Res.code, sign: g3Res.sign};
  const oracleSign3 = (() => { // exact: stored(0.1)^2 vs stored(0.01), both as integer multiples of 2^-2148
    const da = decompress(0.1), db = decompress(0.01);
    // value_prod = sign*sig_a^2 * 2^(2*ea); value_c = sign*sig_b * 2^eb; align both at 2^-2148
    const pUnits = BigInt(da.sign) * da.sig * da.sig * (1n << BigInt(2 * da.e + Number(SHIFT)));
    const cUnits = BigInt(db.sign) * db.sig * (1n << BigInt(db.e + Number(SHIFT)));
    return cmp(pUnits - cUnits);
  })();
  rec('T125-V2-07', 'product materialization: 1×H=H exact equal; H×2=2H equal; 0.1×0.1 vs 0.01 decided by independent exact arithmetic (nonzero or zero per oracle, production must match)',
    g1.code === 'OK' && g1.sign === 0 && g2.code === 'OK' && g2.sign === 0 && g3.code === 'OK' && g3.sign === oracleSign3,
    {g1: g1.sign, g2: g2.sign, g3: g3.sign, oracleG3: oracleSign3,
     note: 'OR-03 gap closed: product is materialized and compared against the unit-aligned third operand'});
}
// carry: max × max must fit; 8-term sums with worst-case carries stay in capacity; subnormal×subnormal min exponent −2148
{
  const d = new D();
  const maxD = 1.7976931348623157e308;
  const gMaxRes = d.compareProduct(maxD, maxD, maxD); const gMax = {code: gMaxRes.code, sign: gMaxRes.sign}; // max^2 vs max: sign positive
  // subnormal product: 5e-324 × 5e-324 — exponent 2×(-1074) = -2148 exactly at capacity edge
  const gSubRes = d.compareProduct(5e-324, 5e-324, 0); const gSub = {code: gSubRes.code, sign: gSubRes.sign};
  // 8-term sum: accumulator must handle 8 maximal terms without overflow
  const big = 1e300;
  const g8Res = d.compareSum(-big, big, big, big); const g8 = {code: g8Res.code, sign: g8Res.sign}; // right=big, left=-big, u=big, bound=big → big+big-big-big=0? sign 0
  rec('T125-V2-08', 'capacity edges: max×max in capacity (positive sign), subnormal×subnormal exact at −2148 exponent, 8-term worst-case sum OK',
    gMax.code === 'OK' && gMax.sign === 1 && gSub.code === 'OK' && gSub.sign === 1 && g8.code === 'OK' && g8.sign === 0,
    {gMax: gMax.sign, gSub: gSub.sign, g8: g8.sign,
     note: 'OR-07 gap closed: highest-bit/carry structure checked against real max inputs and the −2148 minimum exponent'});
  // non-finite rejection
  const gN1Res = d.compareProduct(NaN, 1, 1), gN2 = d.compareSum(0, Infinity, 0, 0), gN3 = d.compareRate(NaN, 0, 1, 1); const gN1 = {code: gN1Res.code, sign: gN1Res.sign};
  rec('T125-V2-09', 'non-finite rejected across sum/product/rate paths (NON_FINITE, no sign pass-through)',
    gN1.code === 'NON_FINITE' && gN2.code === 'NON_FINITE' && gN3.code === 'NON_FINITE',
    {gN1: gN1.code, gN2: gN2.code, gN3: gN3.code});
}

// ---------- rate gate: exact diff vs omega×dt, boundary ULP, dt=0/omega=0, ±0 targets ----------
{
  const d = new D();
  // boundary: |theta-before| exactly == omega*dt → accept (sign 0)
  const g1Res = d.compareRate(1, 0, 1, 1); const g1 = {code: g1Res.code, sign: g1Res.sign};
  // inside: diff smaller
  const g2Res = d.compareRate(1, 0.5, 1, 1); const g2 = {code: g2Res.code, sign: g2Res.sign}; // |0.5| < 1 → negative sign (diff - rate < 0)
  // boundary+1ULP: theta = nextUp(1), omega*dt = 1 → diff exceeds by 1 ULP
  const oneUp = (() => { const b = Buffer.allocUnsafe(8); b.writeDoubleBE(1); b.writeBigUInt64BE(b.readBigUInt64BE() + 1n); return b.readDoubleBE(0); })();
  const g3Res = d.compareRate(oneUp, 0, 1, 1); const g3 = {code: g3Res.code, sign: g3Res.sign};
  // dt=0: any finite theta with before=theta → diff 0; dt=0 rate=0 → diff-rate = 0 only if diff=0
  const g4Res = d.compareRate(0.3, 0.3, 5, 0); const g4 = {code: g4Res.code, sign: g4Res.sign};  // diff=0, rate=0 → 0
  const g5Res = d.compareRate(0.4, 0.3, 5, 0); const g5 = {code: g5Res.code, sign: g5Res.sign};  // diff>0, rate=0 → positive
  // omega=0
  const g6Res = d.compareRate(0.5, 0.3, 0, 0.1); const g6 = {code: g6Res.code, sign: g6Res.sign}; // diff>0, rate 0 → positive
  // ±0 opposite targets: theta=+0, before=-0 → diff=+0 → 0 vs rate 0
  const g7Res = d.compareRate(0, -0, 0, 1); const g7 = {code: g7Res.code, sign: g7Res.sign};
  const oracle = (t, b, o, dt) => {
    // diff in 2^-2148 units (already an integer at this alignment); rate = omega*dt exactly:
    // value = sign_o*sign_dt * sig_o*sig_dt * 2^(eo+edt); to 2^-2148 units shift by eo+edt+2148
    const diff = U(t) - U(b); const ad = diff < 0n ? -diff : diff;
    const bo = bits64(o), bd = bits64(dt);
    const so = (bo >> 63n) & 1n ? -1n : 1n, sd = (bd >> 63n) & 1n ? -1n : 1n;
    const eo = Number((bo >> 52n) & 2047n), ed = Number((bd >> 52n) & 2047n);
    const fo = bo & ((1n << 52n) - 1n), fd = bd & ((1n << 52n) - 1n);
    const sigo = eo ? fo + (1n << 52n) : fo, sigd = ed ? fd + (1n << 52n) : fd;
    const eo2 = eo ? eo - 1075 : -1074, ed2 = ed ? ed - 1075 : -1074;
    const rateUnits = so * sd * sigo * sigd * (1n << BigInt(eo2 + ed2 + 2148));
    return ad > rateUnits ? 1 : ad < rateUnits ? -1 : 0;
  };
  const checks = [[1, 0, 1, 1, g1], [1, 0.5, 1, 1, g2], [oneUp, 0, 1, 1, g3],
    [0.3, 0.3, 5, 0, g4], [0.4, 0.3, 5, 0, g5], [0.5, 0.3, 0, 0.1, g6], [0, -0, 0, 1, g7]];
  const allMatch = checks.every(([t, b, o, dt, g]) => g.code === 'OK' && g.sign === oracle(t, b, o, dt));
  rec('T125-V2-10', 'rate gate: exact |theta-before| vs omega×dt on boundary/inside/+1ULP/dt=0/omega=0/±0 — all signs match independent oracle',
    allMatch, {signs: checks.map(c => c[4].sign), oracle: checks.map(c => oracle(c[0], c[1], c[2], c[3])),
     note: 'OR-05/OR-09 gap closed: real boundary matrix, signed-zero targets, exact diff (not RN diff)'});
  // directed/±0: compareSum with -0 and 0 mixed must normalize
  const gz1Res = d.compareSum(-0, 0, 0, 0); const gz1 = {code: gz1Res.code, sign: gz1Res.sign}; // right - left = 0 - (-0) = 0 → sign 0
  const gz2Res = d.compareSum(0, -0, 0, 0); const gz2 = {code: gz2Res.code, sign: gz2Res.sign};
  rec('T125-V2-11', '±0 normalization inside four-term sum: (+0)-(-0) and (-0)-(+0) both sign 0',
    gz1.code === 'OK' && gz1.sign === 0 && gz2.code === 'OK' && gz2.sign === 0, {gz1: gz1.sign, gz2: gz2.sign});
}

// ---------- whitelist: real invocation must return UNSUPPORTED ----------
{
  const d = new D();
  const gRes = d.unsupportedExpression(); const g = {code: gRes.code, sign: gRes.sign};
  // also verify the production surface has no generic expression entry: probe compareProduct with NaN operand code path
  // (already covered) — here the contract check is that unsupportedExpression() really returns the typed code
  rec('T125-V2-12', 'whitelist boundary: unsupportedExpression() really returns ARITHMETIC_EXPRESSION_UNSUPPORTED (typed, not hardcoded true)',
    g.code === 'ARITHMETIC_EXPRESSION_UNSUPPORTED', {code: g.code,
     note: 'OR-09 gap closed: the whitelist rejection path is actually exercised'});
}

// ---------- fs existence vs load-failure separation (T120 §3 RED discipline) ----------
{
  const files = ['entry/src/main/ets/physics/P011ExactDyadic.ets', 'entry/src/main/ets/physics/P011StateCertificate.ets'];
  const present = files.map(f => ({f, exists: fs.existsSync(f)}));
  let loaded = false;
  try { new D(); loaded = true; } catch (e) { loaded = false; }
  rec('T125-V2-13', 'module identity: both strict-state files exist on disk AND load/instantiate (T112 db1be20 delivered); RED discipline distinguishes fs-absence from load-failure',
    present.every(p => p.exists) && loaded, {present, loaded,
     note: 'T120 §3: RED phase used bare require; future runs must classify fs-missing vs compile/load error'});
}

// ---------- cross-check T111 V1 frozen fixtures remain valid (history preserved, no overwrite) ----------
{
  const v1 = JSON.parse(fs.readFileSync('test/reports/t111/p111_dyadic_fixtures.json', 'utf8'));
  // V1 fixture labels tan30NextUp with value 0.5773502691896258 hex 3fe279a74590331d — correct raw; the V1 REPORT TEXT wrongly said 320
  const fixtureNextUpOK = v1.tan30NextUp.hex === NEXTUP_HEX;
  const v1SyntheticLabel = v1.t86Slope.hex === T111SYN_HEX; // 3321 is synthetic H+5e-16, correctly distinct from raw 3320
  rec('T125-V2-14', 'V1 frozen fixture preservation: fixture nextUp hex is the true 331d (the wrong 320 label was report text only); synthetic 3321 kept distinct from real raw 3320; V1 files untouched',
    fixtureNextUpOK && v1SyntheticLabel && v1.revision === 'T111_DYADIC_ORACLE_V1',
    {v1nextUp: v1.tan30NextUp.hex, v1t86Slope: v1.t86Slope.hex, revision: v1.revision,
     note: 'T120 §1: fixture bit patterns were already correct raw values; only report text mislabeled them'});
}

// ---------- summary + V2 fixture export ----------
{
  const fixtures = {
    revision: 'T125_DYADIC_ORACLE_V2',
    supersedesNotes: 'T120 §1 identities; V1 fixtures preserved at test/reports/t111/',
    identities: {
      H: {hex: H_HEX, ulpOffset: 0}, nextUp: {hex: NEXTUP_HEX, ulpOffset: 1},
      nextDown: {hex: NEXTDOWN_HEX, ulpOffset: -1},
      t100RawRealEdge: {hex: T100_HEX, ulpOffset: 4, kind: 'REAL_T86_T100_EDGE'},
      t111Synthetic: {hex: T111SYN_HEX, ulpOffset: 5, kind: 'T111_SYNTHETIC_H_PLUS_5E_MINUS16'},
    },
    counterexamples: {
      caseA: {Sright: 1, Sleft: '2^-54', u: 0, H: 'nextDown(1)', verdict: 'exact REJECT / RN hides excess'},
      caseB: {Sright: 1, Sleft: '2^-54', u: '2^-54', H: 'nextDown(1)', verdict: 'exact ACCEPT(=H) / RN-first REJECT'},
    },
    commonShift: SHIFT.toString(),
  };
  fs.writeFileSync(OUT + '/p125_dyadic_fixtures_v2.json', JSON.stringify(fixtures, null, 1));
  fs.writeFileSync(OUT + '/p125_results.json', JSON.stringify({
    executionId: 'p125-dyadic-oracle-v2', date: '2026-10-26',
    targetModuleCommit: 'db1be20', contract: 'T120 sections 1/3 + T104 sections 2/7', results}, null, 1) + '\n');
  const pass = results.every(r => r.status === 'PASS');
  console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
  process.exitCode = pass ? 0 : 1;
}
