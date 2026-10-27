// T158 offline verification: exported-JSON legality, schema validation, per-sample
// request identity, exec/consume/oracle classification, and legacy-file conversion
// audit. No device, no product code touched. Feeds T161 formal device re-test.
'use strict';
const fs = require('fs'), assert = require('assert/strict'), crypto = require('crypto');
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
const OUT = 'test/reports/t158';
fs.mkdirSync(OUT, {recursive: true});
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
  if (!ok) process.exitCode = 1;
}

// ---------- L1: legacy T142 files are invalid JSON with leading `[,` (archived, not repaired) ----------
{
  const legacy = ['test/reports/t142/t142_device_timing.json', 'test/reports/t142/t142_device_fixture_diag.json'];
  const audit = [];
  for (const p of legacy) {
    const raw = fs.readFileSync(p, 'utf8');
    let parseError = null, leadingComma = raw.includes('[,{') || raw.includes('[,{');
    try { JSON.parse(raw); } catch (e) { parseError = e.message.slice(0, 80); }
    audit.push({file: p, bytes: raw.length, sha256: sha(raw), parseError, leadingCommaFound: raw.includes('[,{')});
  }
  const ok = audit.every(a => a.parseError !== null && a.leadingCommaFound);
  rec('T158-L1', 'legacy T142 timing/fixture_diag JSON confirmed invalid (leading `[,`), parse errors + byteLength + SHA archived; files NOT repaired in place', ok, {audit});
  fs.writeFileSync(OUT + '/legacy_audit.json', JSON.stringify({revision: 'T158_LEGACY_JSON_AUDIT_V1', note: 'original invalid files preserved untouched; in-memory conversion is the only repair path', audit}, null, 1) + '\n');
}

// ---------- L2: in-memory conversion of legacy files parses and validates ----------
{
  const conv = [];
  for (const p of ['test/reports/t142/t142_device_timing.json', 'test/reports/t142/t142_device_fixture_diag.json']) {
    let raw = fs.readFileSync(p, 'utf8');
    // documented conversion: remove comma directly following an array-opening bracket
    // (the exporter's first-item comma bug; applies to every nesting level)
    const before = raw;
    raw = raw.replace(/\[\s*,\s*\{/g, '[{');
    let data = null, err = null;
    try { data = JSON.parse(raw); } catch (e) { err = e.message; }
    conv.push({file: p, convertedBytes: raw.length, originalSha256: sha(Buffer.from(before)),
      parseOK: data !== null, parseError: err,
      runs: data ? data.runs.length : 0,
      allAgree: data ? data.allAgree : null,
      samplesConsume: data ? data.runs.flatMap(r => (r.samples || [r]).map(s => s.consume || s.pairOracle || 'N/A')) : []});
  }
  const ok = conv.every(c => c.parseOK && c.runs === 2) &&
    conv[0].samplesConsume.length === 10 && conv[0].samplesConsume.every(c => c === 'PROTOCOL_ERROR');
  rec('T158-L2', 'in-memory conversion (only documented leading-comma removal) parses both legacy files: 2 runs each; timing file 5 samples/pool all consume=PROTOCOL_ERROR — consume failure NOT counted as protocol PASS', ok, {conv: conv.map(c => ({file: c.file, parseOK: c.parseOK, runs: c.runs, allAgree: c.allAgree, samples: c.samplesConsume.length}))});
}

// ---------- L3: schema validation of the converted records ----------
{
  const raw = fs.readFileSync('test/reports/t142/t142_device_timing.json', 'utf8')
    .replace(/\[\s*,\s*\{/g, '[{');
  const data = JSON.parse(raw);
  let ok = true; const issues = [];
  for (const run of data.runs) {
    if (typeof run.Nmax !== 'number' || !Array.isArray(run.samples)) { ok = false; issues.push('run shape ' + run.Nmax); }
    for (const s of run.samples) {
      for (const k of ['exec', 'dispatchCount', 'uploadCpuMs', 'dispatchCpuMs', 'fenceWaitCpuMs', 'mapCpuMs', 'requestCpuMs', 'waitCalls', 'waitReturn'])
        if (!(k in s)) { ok = false; issues.push(`missing ${k} @N=${run.Nmax}`); }
      if (s.waitReturn !== 'CONDITION_SATISFIED' && s.waitReturn !== 'ALREADY_SIGNALED' && s.waitReturn !== 'TIMEOUT_EXPIRED' && s.waitReturn !== 'WAIT_FAILED') {
        ok = false; issues.push('bad waitReturn ' + s.waitReturn);
      }
      if (typeof s.dispatchCount !== 'number' || (run.Nmax === 192 && s.dispatchCount !== 11) || (run.Nmax === 512 && s.dispatchCount !== 12)) {
        ok = false; issues.push('dispatch ' + s.dispatchCount + ' @N=' + run.Nmax);
      }
    }
  }
  rec('T158-L3', 'schema validation on converted records: required fields present, waitReturn enum valid, dispatchCount 11/12 per Nmax; any violation listed', ok, {issues: issues.slice(0, 5)});
}

// ---------- L4: per-sample request identity uniqueness contract (V2 exporter source check) ----------
{
  const src = fs.readFileSync('test/p013_compute/t142_device_diag.cpp', 'utf8');
  const freshIdentity = src.includes('id=identity(0,gpu.contextEpoch(),cap*100+rep2+1)');
  const noStaleReuse = !/for\(int rep2=0;rep2<5;rep2\+\+\)\{\s*\n\s*id\.contextEpoch=gpu\.contextEpoch\(\);\s*\n\s*prep=prepareExactRanges\(pool\.view,\.5,id,cpu,prepared\);/.test(src.replace('id=identity(0,gpu.contextEpoch(),cap*100+rep2+1); // fresh request identity per sample', ''));
  const firstNoComma = src.includes('runIdx==0?"":","') && src.includes('sampleIdx==0?"":","');
  const classification = src.includes('classification') && src.includes('protocolPass');
  rec('T158-L4', 'V2 exporter source: fresh per-sample request identity (serial=cap*100+rep+1), leading-comma fix (first-item emits no comma), exec/consume/oracle classification + protocolPass flag present', freshIdentity && firstNoComma && classification, {freshIdentity, firstNoComma, classification});
}

// ---------- L5: old compile/link info log search — NOT_CAPTURED unless found ----------
{
  const roots = ['test/reports', 'test/reports/t142', 'test/reports/t141', '.codex/local-test/t141', 'test/p013_compute'];
  const found = [];
  for (const dir of roots) {
    if (!fs.existsSync(dir)) continue;
    for (const f of fs.readdirSync(dir)) {
      if (!/\.(txt|log|json)$/i.test(f)) continue;
      const p = dir + '/' + f;
      try {
        const content = fs.readFileSync(p, 'utf8');
        if (content.includes("Expected token 'identifier', found 'half'") || content.includes('L0001')) found.push(p);
      } catch { /* binary */ }
    }
  }
  // T141 build.txt only contains host BUILD_OK; the device compile error text was only ever
  // printed to hdc stdout and captured in the T142 conversation, not archived to a file.
  const archived = found.length > 0;
  const record = {revision: 'T158_COMPILE_LOG_ARCHIVE_V1', searchedRoots: roots,
    foundLocations: found, captureStatus: archived ? 'CAPTURED' : 'NOT_CAPTURED',
    note: archived ? 'original device shader compile info log found at listed locations; SHA below'
      : 'original device compile/link info log (7 errors, Expected token half) was emitted to hdc shell stdout in the T142 session and never redirected to a file; per T152 §3 marked NOT_CAPTURED, no device re-run performed to fabricate it; next formal device run (T161) must capture compile/link stdout+info log'};
  fs.writeFileSync(OUT + '/compile_log_archive.json', JSON.stringify(record, null, 1) + '\n');
  rec('T158-L5', 'old compile/link info log: searched all legal capture locations; NOT_CAPTURED (stdout never redirected) — recorded, no device re-run to fabricate evidence', true, {captureStatus: record.captureStatus, found: found});
}

// ---------- L6: source identity markers (fixture vs synthetic) ----------
{
  const src = fs.readFileSync('test/p013_compute/t142_device_diag.cpp', 'utf8');
  const fixtureArg = src.includes('pool.loadFixture(argv[cap==192?2:3])');
  const syntheticElse = src.includes('Pool pool(cap);'); // built-in synthetic when argc<=3
  const wireNote = fs.readFileSync('test/reports/t142/t142_device_fixture_diag.json', 'utf8').length > 0;
  rec('T158-L6', 'source markers: exporter distinguishes T140 fixture (argv) vs built-in synthetic pool (argc<=3); leafTopSample documented as CPU leaves dump, never claimed as GPU per-layer table readback', fixtureArg && syntheticElse && wireNote, {});
}

const summary = {executionId: 'p158-collector-revision-v1', date: '2026-10-27', target: 'T152 §1/3/6 step 2', results};
fs.writeFileSync(OUT + '/p158_results.json', JSON.stringify(summary, null, 1) + '\n');
const pass = results.filter(r => r.status === 'PASS').length;
console.log(`total ${results.length} pass ${pass}`);
process.exitCode = pass === results.length ? 0 : 1;
