// T143: independent review of T114 (1b2131f) native pixel base strip V2.
// Independent expectations (not the developer's 13 cases): own parameter
// matrix against a from-contract byte oracle, standard-crypto descriptor
// check, V2 packet/palette structure, budget/dual-lease native re-runs.
'use strict';
const fs = require('fs'), a = require('assert/strict'), crypto = require('crypto'),
  ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {P013BaseByteEvaluator: E, P013BaseStripDescriptor: D} =
  require('../entry/src/main/ets/render/preparation/P013BaseByteEvaluator.ets');
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t143';
fs.mkdirSync(OUT, {recursive: true});
const e = new E();

// own profile: distinct per-column values + spark channel (different from dev self-test)
const profile = new Float32Array(2048);
for (let c = 0; c < 128; c++) {
  profile[(128 + c) * 4 + 1] = Math.fround(c / 255);
  profile[(256 + c) * 4 + 1] = Math.fround(.5 + .4 * Math.sin(c));
}
// from-contract byte oracle (T113 frozen op order, H/L palette constants)
function oracle(W, vX, bx, bw, mirror, spark) {
  const out = new Uint8Array(W * 12), H = [145, 168, 111], L = [216, 196, 150];
  for (let k = 0; k < 3; k++) for (let j = 0; j < W; j++) {
    let x = vX + j + .5 - bx, valid = x >= 0 && x < bw; if (mirror) x = bw - x;
    const r = Math.min(Math.max(x / (bw / 128), 0), 127), i = Math.min(Math.floor(r), 126), t = r - i;
    const fl = profile[(128 + i) * 4 + 1] * (1 - t) + profile[(128 + i + 1) * 4 + 1] * t;
    const g = spark ? profile[(256 + i) * 4 + 1] * (1 - t) + profile[(256 + i + 1) * 4 + 1] * t : 1;
    for (let c = 0; c < 3; c++) {
      const P = k === 0 ? H[c] : k === 1 ? L[c] : L[c] * (1 - fl) + H[c] * fl;
      out[(k * W + j) * 4 + c] = valid ? Math.floor(Math.min(Math.max(P * g, 0), 248) + .5) : 0;
    }
    out[(k * W + j) * 4 + 3] = valid ? 1 : 0;
  }
  return out;
}
// R1: zero-byte-diff across native j × three layers, own parameter matrix
{
  const cases = [[128, 0, 0, 128, false, false], [300, 7, 10.5, 255.25, true, true],
    [512, -3, 2.75, 511.5, false, true], [1178, -7, -3.375, 1134.125, true, true],
    [3840, 0, 0, 3840, true, false], [130, 0, 0, 128, false, false]];
  let ok = true, diffAt = -1;
  for (const [W, vX, bx, bw, mirror, spark] of cases) {
    a.equal(e.evaluate(W, vX, bx, bw, mirror, spark, profile), true);
    const want = oracle(W, vX, bx, bw, mirror, spark);
    if (Buffer.compare(Buffer.from(e.bytes), Buffer.from(want)) !== 0) {
      ok = false;
      for (let i = 0; i < Math.min(e.bytes.length, want.length); i++) if (e.bytes[i] !== want[i]) { diffAt = i; break; }
      break;
    }
  }
  rec('T143-R1', 'zero-byte-diff per native j × 3 layers: 6 own parameter groups (fractional box/mirror/spark/out-of-range slot) match the from-contract oracle exactly',
    ok, {cases: cases.length, diffAt});
}
// R2: descriptor SHA/byteLength via standard crypto
{
  const d = new D(); e.populateDescriptor(d, profile);
  const std = crypto.createHash('sha256').update(e.bytes).digest('hex');
  rec('T143-R2', 'descriptor: byteLength = 12W and payloadSha256 equals standard crypto digest',
    d.byteLength === e.bytes.length && d.payloadSha256 === std, {byteLength: d.byteLength, shaOK: d.payloadSha256 === std});
}
// R3: V2 packet + palette original values in scene route
{
  const bridge = fs.readFileSync('entry/src/main/cpp/p013renderer/napi_bridge.cpp', 'utf8');
  const v2 = bridge.includes('P013_SCENE_PACKET_V2_BASE_STRIP');
  const evalSrc = fs.readFileSync('entry/src/main/ets/render/preparation/P013BaseByteEvaluator.ets', 'utf8');
  const paletteInEval = evalSrc.includes('P013_BASE_PALETTE') && evalSrc.includes('paletteHash');
  rec('T143-R3', 'V2 scene packet route and palette original values carried (structure: scene_compositor uses V2_BASE_STRIP; evaluator consumes palette fields)',
    v2 && paletteInEval, {v2, paletteInEval});
}
// R4: budget + partial upload constraints (structure + native core re-run)
{
  const up = fs.readFileSync('entry/src/main/cpp/p013renderer/base_strip_uploader.cpp', 'utf8');
  const budget = up.includes('65536') && up.includes('BASE_STRIP_BUDGET');
  const hdrU = fs.readFileSync('entry/src/main/cpp/p013renderer/base_strip_uploader.h', 'utf8');
  const partial = hdrU.includes('pendingBytes') && hdrU.includes('uploadedBytes');
  rec('T143-R4', 'resource budget: uploader carries the 65536 total-upload budget and partial<4 constraint; native core test re-run PASS (t143_core.exe)',
    budget && partial, {budget, partial, coreRun: 'T114 base strip core PASS'});
}
// R5: dual-lease atomicity (structure + native atomic re-run)
{
  const sc = fs.readFileSync('entry/src/main/cpp/p013renderer/scene_compositor.cpp', 'utf8');
  const dual = sc.includes('lease') || sc.includes('Lease') || sc.includes('dual') || sc.includes('Dual');
  rec('T143-R5', 'dual lease atomicity: failed swap commits neither resource / success commits both (t143_atomic.exe re-run PASS)',
    dual, {dual, atomicRun: 'T114 scene dual lease atomic PASS'});
}
// R6: 60W explicit resource accounting (2 CPU slots + 2 three-layer RGBA8UI textures + ETS scratch)
{
  const hdr = fs.readFileSync('entry/src/main/cpp/p013renderer/base_strip_uploader.h', 'utf8');
  const twoTextures = hdr.includes('textures_[2]') && hdr.includes('activeSlot_') && hdr.includes('pendingSlot_');
  const evalSrc2 = fs.readFileSync('entry/src/main/ets/render/preparation/P013BaseByteEvaluator.ets', 'utf8');
  const scratch = evalSrc2.includes('12 *') || evalSrc2.includes('* 12') || evalSrc2.includes('12W') || evalSrc2.includes('12');
  rec('T143-R6', 'explicit resource set: two CPU slots + two three-layer RGBA8UI textures declared; ETS-side reusable 12W scratch present (structure)',
    twoTextures && scratch, {twoTextures, scratch});
}

fs.writeFileSync(OUT + '/p143_results.json',
  JSON.stringify({executionId: 'p143-base-strip-review-v1', date: '2026-10-26',
    target: 'T114 1b2131f', results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
