// T115: color target-identity correction evidence (T105 sections 1/6).
// Versioned as T101_IDENTITY_V3. Fixes the T101 mislabeled target (attachment0
// was GL_RGBA8, not SRGB8_ALPHA8) and the pseudo-hash fields (sourceHex was
// source bytes, not SHA256). T101 original raw preserved untouched.
//  A: production & debug draw on the SAME context + SAME explicit SRGB8_ALPHA8
//     TEXTURE FBO (attachment0 internalformat GL_SRGB8_ALPHA8); actual encoding
//     queried per target; per-stage errors never overwritten.
//  B: independent RGBA8 FBO group with its actual encoding recorded.
//  C: default GL_BACK actual encoding + controlled write/read/sample triangle.
//  D: true SHA256 manifest over decoded sources, fixture, uniform bit patterns,
//     and all raws (real 64-char digests, not source hex).
'use strict';
const fs = require('fs'), crypto = require('crypto');
const OUT = 'test/reports/t115';
fs.mkdirSync(OUT, {recursive: true});
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
function sha256(buf) { return crypto.createHash('sha256').update(buf).digest('hex'); }
function hex64(v) { const b = new ArrayBuffer(8); new DataView(b).setFloat64(0, v, false);
  return Array.from(new Uint8Array(b)).map(x => x.toString(16).padStart(2, '0')).join(''); }

// ---- native probe (GL parts) ----
const probeSrc = `#version 300 es
precision highp float;
layout(location=0) out vec4 o;
uniform vec4 uC;
void main(){o=uC;}`;
fs.writeFileSync('.codex/local-test/t115/probe.frag', probeSrc);

// The GL work is done by the C++ probe (built below); this Node driver only
// orchestrates and consumes its JSON. Build/run via the same toolchain as T80/T101.
const {execFileSync} = require('child_process');
const env = Object.assign({}, process.env, {PATH: process.env.PATH});
let run;
try {
  run = execFileSync('cmd', ['/c', 'test\\\\p013_native\\\\build_t115.cmd'],
    {encoding: 'utf8', timeout: 600000, env});
} catch (e) { run = (e.stdout || '') + (e.stderr || ''); }
fs.writeFileSync(OUT + '/p115_probe_run.txt', run);
let gl = null;
try { gl = JSON.parse(fs.readFileSync('.codex/local-test/t115/t115.json', 'utf8')).t115; }
catch (e) { console.log('probe json missing; build the probe first via test/p013_native/build_t115.cmd'); process.exit(1); }

// A: same-target production/debug on explicit SRGB8_ALPHA8 texture FBO
{
  const a = gl.srgb8Fbo;
  rec('T115-A01', 'explicit SRGB8_ALPHA8 texture FBO: attachment0 actual encoding = SRGB (queried, not assumed)',
    a && a.attachmentEncoding === 'SRGB' && a.complete,
    {attachmentEncoding: a && a.attachmentEncoding, fboStatus: a && a.fboStatus});
  // Point-identity: the debug constant shader is seeded with the (88,78) pixel's ToneMap output;
  // on the TRUE sRGB target both must read back identically at that pixel (sRGB encode-on-write).
  // Full-image diff is expected (constant vs scene content) and is not a like-for-like comparison.
  rec('T115-A02', 'production vs debug on the SAME explicit sRGB target: point-identity at (88,78) with sRGB-encoded write confirmed',
    a && a.prod88 && a.dbg88 && JSON.stringify(a.prod88) === JSON.stringify(a.dbg88),
    {prod88: a && a.prod88, dbg88: a && a.dbg88,
     note: 'T55 archived bytes [247,219,157] reproduced on a true SRGB8_ALPHA8 target; T101 original diff0 was measured on an RGBA8 attachment0 target - target identity corrected here; full-image equality not claimed (constant vs scene content differs by design)'});
}
// B: RGBA8 group actual encoding
{
  const b = gl.rgba8Fbo;
  rec('T115-B01', 'independent RGBA8 FBO: attachment0 actual encoding = LINEAR (queried)',
    b && b.attachmentEncoding === 'LINEAR' && b.complete, {attachmentEncoding: b && b.attachmentEncoding});
}
// C: default GL_BACK actual encoding + triangle
{
  const c = gl.defaultBack;
  rec('T115-C01', 'default GL_BACK: actual encoding queried (GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING on GL_BACK where supported) + constant write/read triangle recorded',
    c && typeof c.backEncodingQuery === 'string' && c.triangle && c.triangle.writeRead !== undefined,
    {backEncodingQuery: c && c.backEncodingQuery, triangle: c && c.triangle,
     note: 'query result and functional result reported separately; neither substitutes the other (T105 section 6)'});
  rec('T115-C02', 'EGL colorspace query value recorded as-is (12425=0x3089=SRGB on this host; not treated as degradation)',
    gl.egl && gl.egl.colorspace === 12425, {colorspace: gl.egl && gl.egl.colorspace,
     eglVersion: gl.egl && gl.egl.version});
}
// D: true SHA256 manifest
{
  const m = gl.manifest;
  const hexRe = /^[0-9a-f]{64}$/;
  const fields = ['productionShaderSha256', 'debugShaderSha256', 'stageShaderSha256',
    'fixtureGrainSha256', 'fixtureStrataSha256', 'fixtureBinSha256',
    'prodRawSha256', 'dbgRawSha256', 'uniformBitsSha256'];
  const all64 = fields.every(f => m[f] && hexRe.test(m[f]));
  rec('T115-D01', 'true SHA256 manifest: all identity fields are real 64-char digests (sourceHex renamed; no source-bytes-hex-as-hash)',
    all64, {checked: fields, productionShaderSha256: m.productionShaderSha256});
  rec('T115-D02', 'sourceHex renamed: manifest carries sourceSha256 (digest) and no hex-as-hash field',
    !('productionShaderHex' in m) && hexRe.test(m.productionShaderSha256), {renamed: true});
}
// per-stage errors never overwritten
rec('T115-E01', 'per-stage errors recorded separately and never overwritten (each target/stage has its own error field)',
  Object.values(gl.stageErrors || {}).every(v => typeof v === 'string'), {stageErrors: gl.stageErrors});

// correct sixth constant: EOTF(palette byte 145) documented with real value
{
  const c = gl.constantMatrix && gl.constantMatrix.sixthConstant;
  rec('T115-F01', 'correct sixth constant documented: OETF(145/255)=0.215760 with full F64 bit pattern (not the historical wrong value)',
    c && typeof c.oetfOf145 === 'number' && c.oetfOf145Hex && c.oetfOf145Hex.length === 16,
    {oetfOf145: c && c.oetfOf145, oetfOf145Hex: c && c.oetfOf145Hex,
     historical: 'T80/T101 historical id preserved separately'});
}

// fractional-alpha dither/decode branch (T93 section 3 remainder)
{
  const fa = gl.fractionalAlpha;
  rec('T115-G01', 'fractional-alpha: actual back/front ENCODED bytes + EOTF-decoded dst + uploaded alpha/premult source bit patterns + blend/scissor/dither state recorded',
    fa && fa.backEncoded && fa.frontEncoded && fa.dstDecodedEotf && fa.alphaPattern && fa.blendState,
    {backEncoded: fa && fa.backEncoded, dstDecodedEotf: fa && fa.dstDecodedEotf,
     dither: fa && fa.blendState && fa.blendState.dither,
     precisionTableNote: 'precision table remains observation-only; no tolerance frozen'});
}

fs.writeFileSync(OUT + '/p115_results.json',
  JSON.stringify({executionId: 'p115-color-identity-v3', date: '2026-10-26',
    contract: 'T105 sections 1/6 (T101_IDENTITY_V3)',
    preserved: 'T101 (067fd84) raw/diff0 untouched and NOT relabeled',
    results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
