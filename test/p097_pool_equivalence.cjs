// T97 arm 1: NEW (c0c58c3) vs OLD (067fd84) same-pool step-by-step equivalence.
// Uses the real WaterSandField from each revision dir with a seeded, identical
// workload (births, merges, tail births, two phases, motion, overflow attempts,
// rollback) and compares per-step observables: pool digest, events, status,
// free/live/nextId, column masses, budgets.
'use strict';
const fs = require('fs'), a = require('assert/strict'), Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
function loadRevision(dir) {
  const req = new Module(dir + '/driver.js');
  req.filename = dir + '/driver.js';
  req.paths = Module._nodeModulePaths(dir);
  req._compile(`
    const fs=require('fs'),Module=require('module');
    const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
    require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
    const load=Module._load;Module._load=function(n,p,m){if(n==='libp013renderer.so')return {default:{}};if(n==='@ohos.hilog')return {default:{info(){},warn(){},error(){}}};return load.call(this,n,p,m)};
    const {WaterSandField}=require('${dir}/entry/src/main/ets/physics/WaterSandField.ets');
    const {defaultP011ParticleParams}=require('${dir}/entry/src/main/ets/physics/P011ParticleM2Field.ets');
    module.exports={WaterSandField,defaultP011ParticleParams};
  `, dir + '/driver.js');
  return req.exports;
}
const NEW = loadRevision(require('path').resolve('.codex/local-test/t97/c0c58c3').replace(/\\/g, '/'));
const OLD = loadRevision(require('path').resolve('.codex/local-test/t97/067fd84').replace(/\\/g, '/'));

// deterministic PRNG
function makeRng(seed) { let s = seed >>> 0; return () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; }; }

function makeField(mod, nmax) {
  const f = new mod.WaterSandField(128, 512);
  const p = mod.defaultP011ParticleParams(); p.nmax = nmax;
  a.equal(f.setParticleRoute(p).ok, true);
  f.setNaturalObservationRouteId('dual-field-particle-v1');
  f.initialize({waterRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 160}],
    heavyDepositedRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 24}],
    lightDepositedRegions: [], heavySuspendedRegions: [], lightSuspendedRegions: []});
  return f;
}
function digest(f) {
  const snap = f.particleBinding.pool.snapshot();
  const keys = ['pId','pState','pPhase','pMass','pX','pY','pVx','pVy','pSource','pSourceId','liveCount','nextId','freeCount'];
  const o = {};
  for (const k of keys) o[k] = Array.from(snap[k] instanceof Float64Array || snap[k] instanceof Int32Array || snap[k] instanceof Uint32Array ? snap[k] : (Array.isArray(snap[k]) ? snap[k] : [snap[k]]));
  o.colH = Array.from(f.particleBinding.pool.colParticleMassH ? snap.colParticleMassH || [] : []);
  return JSON.stringify(o);
}
function eventsOf(f) { return f.endpointParticleEvents.map(JSON.parse); }

function runWorkload(mod, nmax, seed, steps) {
  const f = makeField(mod, nmax);
  const rng = makeRng(seed);
  const b = f.particleBinding;
  const trace = [];
  // seed population: random columns/phases; leave 3 free slots for tail births at N16
  const pop = nmax === 16 ? nmax - 3 : nmax - 5;
  for (let i = 0; i < pop; i++) {
    const col = Math.floor(rng() * 128), phase = rng() < 0.5 ? 0 : 1;
    a.ok(b.pool.allocate({mass: 0.1 + (i % 3) * 0.05, x: col + 0.5 + (rng() - 0.5) * 0.2,
      y: 100 + (i % 5) * 3, vx: 0, vy: 0, phase, sourceKind: 0, sourceId: 100 + i}) >= 0);
  }
  for (let s = 0; s < steps; s++) {
    // inject a random small source each step (heavy/light mix, random column)
    const h = new Float64Array(128), l = new Float64Array(128);
    const hc = Math.floor(rng() * 128), lc = Math.floor(rng() * 128);
    h[hc] = 0.1 + rng() * 0.3; l[lc] = rng() * 0.2;
    f.injectGestureSand({perColumnHeavy: h, perColumnLight: l, sourceId: 9000 + s, sequence: s + 1,
      inputTime: s * 16, x: hc + 0.5, y: 100, seed: seed + s});
    const r = f.step(1 / 60);
    trace.push({step: s, ok: r.ok, errorCode: r.errorCode, digest: digest(f),
      events: eventsOf(f).map(e => ({kind: e.kind, id: e.particleId, mergedIds: e.mergedIds,
        mass: e.mass !== undefined ? e.mass : undefined})),
      free: f.particleBinding.pool.freeCount, live: f.particleBinding.pool.liveCount,
      nextId: f.particleBinding.pool.nextId});
    if (!r.ok) break;
  }
  return trace;
}

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t97';
fs.mkdirSync(OUT, {recursive: true});

// workload matrix: N16 tail-heavy, N512 wide, multi-merge columns, two phases
const workloads = [
  {nmax: 16, seed: 11, steps: 60, tag: 'N16-tail'},
  {nmax: 16, seed: 77, steps: 60, tag: 'N16-mix'},
  {nmax: 512, seed: 1234, steps: 40, tag: 'N512-wide'},
];
let firstMismatch = null;
for (const w of workloads) {
  const tNew = runWorkload(NEW, w.nmax, w.seed, w.steps);
  const tOld = runWorkload(OLD, w.nmax, w.seed, w.steps);
  const n = Math.max(tNew.length, tOld.length);
  let same = tNew.length === tOld.length;
  let diffAt = null;
  for (let i = 0; i < Math.min(tNew.length, tOld.length); ++i) {
    if (JSON.stringify(tNew[i]) !== JSON.stringify(tOld[i])) { same = false; diffAt = i; break; }
  }
  if (!same && firstMismatch === null) firstMismatch = {workload: w.tag, diffAt};
  const totalMergesNew = tNew.reduce((m, t) => m + t.events.filter(e => e.kind === 'MERGE').length, 0);
  rec(`T97-EQ-${w.tag}`, `identical per-step pool/events/status NEW vs OLD (${w.tag}, ${w.steps} steps)`,
    same, {stepsCompared: Math.min(tNew.length, tOld.length), diffAt,
      totalMergesNew, firstTraceStep: {ok: tNew[0].ok, free: tNew[0].free, live: tNew[0].live}});
}

// diagnostic ON/OFF equivalence on the NEW revision (must not alter physics)
{
  const mk = () => { const f = makeField(NEW, 16); return f; };
  const off = mk(), on = mk();
  on.particleBinding.work.diagnosticEnabled = true;
  const rngA = makeRng(4242), rngB = makeRng(4242);
  let equiv = true;
  for (let s = 0; s < 40 && equiv; s++) {
    for (const [f, rng] of [[off, rngA], [on, rngB]]) {
      const h = new Float64Array(128), l = new Float64Array(128);
      const hc = Math.floor(rng() * 128); h[hc] = 0.05 + rng() * 0.4;
      f.injectGestureSand({perColumnHeavy: h, perColumnLight: l, sourceId: 500 + s, sequence: s + 1,
        inputTime: s * 16, x: hc + 0.5, y: 100, seed: 42 + s});
      f.step(1 / 60);
    }
    if (JSON.stringify(digest(off)) !== JSON.stringify(digest(on))) equiv = false;
  }
  const diag = on.getPoolProgressDiagnostic ? on.getPoolProgressDiagnostic() : null;
  rec('T97-DIAG', 'diagnostic ON/OFF: physics identical over 40 steps; diagnostic string present when ON, null when OFF',
    equiv && diag !== null && diag.includes('mergeCandidateChecks') &&
    (off.getPoolProgressDiagnostic ? off.getPoolProgressDiagnostic() : null) === null,
    {diagHead: diag && diag.slice(0, 160)});
}

fs.writeFileSync(OUT + '/p097_equivalence_results.json',
  JSON.stringify({executionId: 'p097-pool-equivalence-v1', date: '2026-10-26',
    identity: {newRev: 'c0c58c3', oldRev: '067fd84 (c0c58c3 parent)'},
    results, firstMismatch}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
