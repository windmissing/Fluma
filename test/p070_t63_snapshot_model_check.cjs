#!/usr/bin/env node
/**
 * T70 snapshot check: rerun the three decisive cases against the EXACT
 * 86366dd (T63 build) model bytes pulled from git, because the shared
 * working tree has since diverged (T58/T59 dry-demo work).
 */
'use strict';
const fs = require('fs'), Module = require('module'), cp = require('child_process'), path = require('path');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const git = 'C:/Program Files/WorkBuddy/resources/vendor/PortableGit/cmd/git.exe';
const snapFiles = {
  'entry/src/main/ets/physics/P051EmptySandDemo.ets':
    cp.execFileSync(git, ['show', '86366dd:entry/src/main/ets/physics/P051EmptySandDemo.ets'], {encoding: 'utf8'}),
  'entry/src/main/ets/physics/P051GravityInputAdapter.ets':
    cp.execFileSync(git, ['show', '86366dd:entry/src/main/ets/physics/P051GravityInputAdapter.ets'], {encoding: 'utf8'})
};
require.extensions['.ets'] = (m, f) => {
  const key = Object.keys(snapFiles).find(k => f.endsWith(k));
  const src = key !== undefined ? snapFiles[key] : fs.readFileSync(f, 'utf8');
  return m._compile(ts.transpileModule(src,
    {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
};
const {P051EmptySandDemo} = require(path.join(__dirname, '..', 'entry/src/main/ets/physics/P051EmptySandDemo.ets'));

const out = [];
function rec(id, name, ok, detail) {
  out.push(Object.assign({id, name, status: ok ? 'PASS' : 'FAIL'}, detail || {}));
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}

// A01 on exact T63 bytes
const m = new P051EmptySandDemo();
m.setActive(true);
m.start(64.5, 220);
let ns = 1e9; const rows = [];
for (let i = 0; i < 120; i++) {
  ns += 1e9 / 60;
  if (!m.tick(ns, 1 / 60)) break;
  rows.push(m.field.getMetrics().sourceInjectedHeavy);
}
const met = m.field.getMetrics();
rec('T70-A01', 'T63 exact bytes: legal positive still nonzero',
  met.sourceInjectedHeavy > 0 && met.particleMass > 0,
  {sourceInjectedHeavy: met.sourceInjectedHeavy, particleMass: met.particleMass,
   firstNonzeroRow: rows.findIndex(v => v > 0)});

// D01 stale signature on T63 bytes
const m4 = new P051EmptySandDemo();
m4.setActive(true);
m4.start(64.5, 220);
for (let i = 0; i < 120; i++) {
  m4.applyDefinedInput({source: 'stale', sampleTimeMs: i, valid: true, targetTiltDeg: null, shakeInput: 0, shakeMagnitudeMps2: 0});
  if (!m4.tick(1e9 + i * 1e9 / 60, 1 / 60)) break;
}
const met4 = m4.field.getMetrics();
rec('T70-D01', 'T63 exact bytes: stale-on-every-tick zero-source signature reproduced',
  met4.sourceInjectedHeavy === 0 && met4.particleMass === 0,
  {sourceInjectedHeavy: met4.sourceInjectedHeavy, particleMass: met4.particleMass});

// B03 single stale stops
const m2 = new P051EmptySandDemo();
m2.setActive(true);
m2.start(64.5, 220);
m2.applyDefinedInput({source: 'stale', sampleTimeMs: 0, valid: true, targetTiltDeg: null, shakeInput: 0, shakeMagnitudeMps2: 0});
m2.tick(1e9, 1 / 60);
rec('T70-B03', 'T63 exact bytes: one stale input stops touching before injection',
  m2.field.getMetrics().sourceInjectedHeavy === 0, {});

fs.writeFileSync(path.join(__dirname, 'reports', 'p070_t63_snapshot_model_check.json'),
  JSON.stringify({executionId: 'p070-t63-snapshot-model-check-v1', date: '2026-10-26',
    note: 'A01/B03/D01 rerun against the EXACT 86366dd (T63 build) model bytes; the shared working tree has since diverged only by T58/T59 dry-demo work absent from the T63 build',
    results: out}, null, 1) + '\n');
console.log('done', out.every(r => r.status === 'PASS') ? 'ALL PASS' : 'FAIL');
process.exitCode = out.every(r => r.status === 'PASS') ? 0 : 1;
