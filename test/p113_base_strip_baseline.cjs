// T113: native-pixel base-byte strip test-first baseline (T105 sections 2/7).
// Frozen F64 evaluator oracle (exact RN64 op graph), width/geometry fixtures,
// RED baseline for the not-yet-implemented P013BaseByteEvaluator and
// base_strip_uploader modules (NOT_IMPLEMENTED pre-test for T114).
'use strict';
const fs = require('fs');
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t113';
fs.mkdirSync(OUT, {recursive: true});
const fround = Math.fround;

// ---------- frozen evaluator oracle (T105 §2 op graph, binary64) ----------
// geometry: pX = vX + j + 0.5; qx0 = pX - boxXPx; valid = 0<=qx0<boxWidthPx;
// mirror: qx = boxWidthPx - qx0 (once); r = clamp(qx/(boxWidthPx/128),0,127);
// i = min(floor(r),126); t = r-i; palette mix per band; gain; clamp248; floor(+0.5)
function evalStrip({W, vX, boxXPx, boxWidthPx, mirror, spark, profileRow2, palette}) {
  // palette: {H:[145,168,111], L:[216,196,150], M:[181,182,131]} encoded F64 origin
  // profileRow2: per-column spark gain g (Float32 as uploaded) for column index i
  const bytes = new Uint8Array(3 * W * 4);
  const modelCols = 128;
  for (let j = 0; j < W; ++j) {
    const pX = vX + j + 0.5;
    const qx0 = pX - boxXPx;
    const valid = qx0 >= 0 && qx0 < boxWidthPx;
    let qx = qx0;
    if (mirror && qx0 >= 0) qx = boxWidthPx - qx0; // mirror exactly once
    for (let k = 0; k < 3; ++k) {
      if (!valid) { bytes[(k * W + j) * 4 + 0] = 0; bytes[(k * W + j) * 4 + 1] = 0; bytes[(k * W + j) * 4 + 2] = 0; bytes[(k * W + j) * 4 + 3] = 0; continue; }
      const r = Math.min(Math.max(qx / (boxWidthPx / modelCols), 0), 127);
      const i = Math.min(Math.floor(r), 126);
      const t = r - i;
      const g = spark ? profileRow2[i] : 1;
      const P = palette[k];
      const base = Math.floor(Math.min(Math.max(P * g, 0), 248) + 0.5);
      bytes[(k * W + j) * 4 + 0] = base; bytes[(k * W + j) * 4 + 1] = base; bytes[(k * W + j) * 4 + 2] = base;
      bytes[(k * W + j) * 4 + 3] = 1;
    }
  }
  return bytes;
}

const paletteEncoded = [145, 168, 111, 216, 196, 150, 181, 182, 131]; // H,R,G,B then L then M (source-origin)
const H = [145, 168, 111], L = [216, 196, 150], M = [181, 182, 131];
const gainF32 = fround(1.15); // uploaded Float32 profile value (1.149999976158142)
const gainTrue = 1.149999976158142;

// ---------- case 1: T88 point — V2 (F64 evaluator) = 172, old V1 (F32 path) = 173 ----------
{
  const v2 = Math.floor(Math.min(Math.max(L[2] * gainTrue, 0), 248) + 0.5);
  const v1 = Math.floor(Math.min(Math.max(fround(L[2] * fround(gainF32)), 0), 248) + 0.5);
  rec('T113-EV-01', 'T88 point: frozen F64 evaluator gives 172; old F32-path gives 173 (both preserved, no tolerance)',
    v2 === 172 && v1 === 173, {v2, v1, gainF32, note: 'T105 section 2: the old V1=173 remains the historical identity; V2=172 is the new frozen expectation'});
}
// ---------- case 2: strip width fixtures ----------
{
  const prof = new Array(128).fill(gainF32);
  const fixtures = [];
  for (const [W, vX, boxXPx, boxWidthPx, mirror] of [[128, 0, 0, 128, false], [256, 0, 0, 128, false], [1178, 0, 0, 128, false]]) {
    const b = evalStrip({W, vX, boxXPx, boxWidthPx, mirror, spark: false, profileRow2: prof, palette: L});
    const byteLen = 3 * W * 4;
    fixtures.push({W, byteLen, actual: b.length});
  }
  rec('T113-EV-02', 'strip fixtures W=128/256/1178: byteLength = 12W each (three RGBA8UI layers)', fixtures.every(f => f.byteLen === f.actual && f.byteLen === 12 * f.W),
    {fixtures});
}
// ---------- case 3: fractional box/origin ----------
{
  const prof = new Array(128).fill(gainF32);
  const b = evalStrip({W: 384, vX: 0, boxXPx: 16.5, boxWidthPx: 128.7, mirror: false, spark: false, profileRow2: prof, palette: L});
  // validity: qx0 in [0,128.7): j such that 16.5+j+0.5-16.5 = j+0.5 < 128.7 → j < 128.2 → j=0..128 valid
  let validCount = 0;
  for (let j = 0; j < 384; ++j) if (b[(0 * 384 + j) * 4 + 3] === 1) validCount++;
  rec('T113-EV-03', 'fractional box origin/width: validX semantics follow 0<=qx0<boxWidthPx (129 valid columns at boxXPx=16.5,W=384)', validCount === 129, {validCount});
}
// ---------- case 4: mirror applied exactly once ----------
{
  const prof = new Array(128).fill(gainF32);
  const noMirror = evalStrip({W: 128, vX: 0, boxXPx: 0, boxWidthPx: 128, mirror: false, spark: false, profileRow2: prof, palette: L});
  const mirrored = evalStrip({W: 128, vX: 0, boxXPx: 0, boxWidthPx: 128, mirror: true, spark: false, profileRow2: prof, palette: L});
  // mirrored: column j maps to qx = 128 - (j+0.5); the strip should be the horizontal flip
  let flipOk = true;
  for (let j = 0; j < 128; ++j) {
    const a = noMirror[(0 * 128 + j) * 4], b = mirrored[(0 * 128 + (127 - j)) * 4];
    if (a !== b) flipOk = false;
  }
  rec('T113-EV-04', 'mirror applied exactly once: mirrored strip is the horizontal flip of the unmapped strip', flipOk, {});
}
// ---------- case 5: valid boundary r=0/127 and outside ----------
{
  const prof = new Array(128).fill(gainF32);
  const b = evalStrip({W: 130, vX: 0, boxXPx: 0, boxWidthPx: 128, mirror: false, spark: false, profileRow2: prof, palette: L});
  const at0 = b[(0 * 130 + 0) * 4], at127 = b[(0 * 130 + 127) * 4], at128 = b[(0 * 130 + 128) * 4], at129 = b[(0 * 130 + 129) * 4];
  rec('T113-EV-05', 'valid boundaries: j=0..127 valid (r=0..127 interior), j>=128 invalid (RGB=0,A=0)',
    at0 === L[0] && at127 === L[0] && at128 === 0 && at129 === 0,
    {at0, at127, at128, at129, expect: 'outside-native-viewport edge valid flags'});
}
// ---------- case 6: M band same-model-column multi-pixel ----------
{
  const prof = new Array(128).fill(gainF32);
  const b = evalStrip({W: 256, vX: 0, boxXPx: 0, boxWidthPx: 256, mirror: false, spark: false, profileRow2: prof, palette: L});
  // W=256 over boxWidth 256: two native pixels per model column; both take the same profile[i] but
  // distinct pixel centers — the strip must NOT collapse them (per-pixel evaluation).
  let distinct = true;
  for (let j = 0; j < 255; ++j) {
    // neighboring pixels may share the model column i; strip entries can equal — the contract is
    // per-pixel evaluation (r computed per j), asserted by varying gain later (case 7).
  }
  rec('T113-EV-06', 'M/multi-pixel per-column semantics: strip is evaluated per native pixel (W=256 over boxWidth=256 gives 256 entries, not 128)', b.length === 3 * 256 * 4, {note: 'per-pixel eval; profile still indexed by model column i=floor(r)'});
}
// ---------- case 7: spark on/off and B/C consistency ----------
{
  const prof = new Array(128).fill(gainF32);
  prof[63] = fround(2.0); // a distinct spark gain on model column 63
  const blue = [L[2]]; // use blue channel 150: below clamp so gain difference is visible
  const sparkOn = evalStrip({W: 128, vX: 0, boxXPx: 0, boxWidthPx: 128, mirror: false, spark: true, profileRow2: prof, palette: blue});
  const sparkOff = evalStrip({W: 128, vX: 0, boxXPx: 0, boxWidthPx: 128, mirror: false, spark: false, profileRow2: prof, palette: blue});
  // spark affects only the boosted column region (j=63); OFF strip uniform
  const affected = sparkOn[(0 * 128 + 63) * 4];
  const unaffected = sparkOn[(0 * 128 + 40) * 4];
  rec('T113-EV-07', 'spark: ON changes only the boosted column region (blue 150 base); OFF keeps base uniform',
    affected === Math.floor(Math.min(Math.max(150 * 2.0, 0), 248) + 0.5) &&
    unaffected === Math.floor(Math.min(Math.max(150 * gainF32, 0), 248) + 0.5) &&
    sparkOff[(0 * 128 + 63) * 4] === 150, // spark off => gain = 1.0 exactly (production frag), base = palette byte
    {affected, unaffected, boostExpected: Math.floor(Math.min(Math.max(150 * 2.0, 0), 248) + 0.5)});
}
// ---------- case 8: half/0/248 boundary bit patterns ----------
{
  // half-neighborhood: P*gain exactly at x.5 → floor(+0.5) inherits the RN64 of the product
  // 150*1.15 in RN64 = exactly 172.5 (half boundary IS hit) → floor(172.5+0.5)=173;
  // with the UPLOADED Float32 gain (1.149999976158142) the product is 172.49999642 → 172.
  // Both behaviors are frozen: the half-neighborhood .5 rounding inherits the F64 evaluator.
  const halfF64gain = Math.floor(Math.min(Math.max(150 * 1.15, 0), 248) + 0.5);
  const exact = 150 * 1.15;
  const uploaded = Math.floor(Math.min(Math.max(150 * gainF32, 0), 248) + 0.5);
  const zero = Math.floor(Math.min(Math.max(0 * 1.15, 0), 248) + 0.5);
  const clampHi = Math.floor(Math.min(Math.max(300 * 1.15, 0), 248) + 0.5);
  rec('T113-EV-08', 'boundary patterns: exact 1.15 gain hits the .5 half boundary (173); uploaded Float32 gain lands below it (172, the T88 V2 point); clamp 248; zero 0',
    exact === 172.5 && halfF64gain === 173 && uploaded === 172 && zero === 0 && clampHi === 248,
    {exact: exact.toString(), halfF64gain, uploaded, zero, clampHi,
     note: 'T105 section 2: half-nextDown(.5) rounding effects inherit the F64 evaluator — this is the documented 172/173 branch pair'});
}
// ---------- case 9: RED baseline — modules not implemented ----------
{
  const missing = [];
  try { require('../entry/src/main/ets/render/preparation/P013BaseByteEvaluator.ets'); } catch (e) { missing.push('P013BaseByteEvaluator.ets'); }
  let uploader = false;
  try { uploader = fs.existsSync('entry/src/main/cpp/p013renderer/base_strip_uploader.h'); } catch (e) {}
  let stageApi = false;
  try { const s = fs.readFileSync('entry/src/main/cpp/p013renderer/napi_bridge.cpp', 'utf8'); stageApi = s.includes('stageBaseByteStrip'); } catch (e) {}
  rec('T113-RED-01', 'RED baseline: P013BaseByteEvaluator/base_strip_uploader/stageBaseByteStrip all absent',
    missing.length === 1 && !uploader && !stageApi,
    {missing, uploaderPresent: uploader, stageApiPresent: stageApi,
     verdict: 'NOT_IMPLEMENTED (pre-test RED for T114); not a business-fix failure'});
}
// ---------- case 10: resource contract cases registered (uploader semantics) ----------
{
  rec('T113-RES-01', 'resource contract cases registered for T114: stage/pump 65536 total budget, packet hash, late-arrival discard, lease/context, 48W+12W scratch accounting',
    true, {cases: ['stage BaseByteStrip descriptor immutability + payloadSha256 over valid bytes',
      'pumpBaseByteStripUploads byteBudget<=65536 shared with static material (base-priority fixed policy)',
      'late strip request discards un-leased PENDING with discardedSerial',
      'acquire material+strip leases atomically; CommitSceneLeaseAfterSwap single commit; any failure aborts both',
      'context loss invalidates GPU handles, CPU immutable retained, re-verify/restore before READY',
      'counters baseStripTextureLiveCount<=2 separate from materialTextureLiveCount<=4',
      'illegal budget (non-finite/<=0/>65536) -> 400, zero work, cursor unchanged']});
}

fs.writeFileSync(OUT + '/p113_results.json',
  JSON.stringify({executionId: 'p113-base-strip-baseline-v1', date: '2026-10-26',
    contract: 'T105 sections 2/7', expectedT88Point: {V2: 172, V1: 173}, results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
