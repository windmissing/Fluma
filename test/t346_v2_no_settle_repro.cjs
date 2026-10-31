'use strict';
// T346: V2 particle no-settle offline 1:1 reproduction harness (decisive
// experiment per docs/architecture-T345-v2-no-settle-attribution-v1.md §3).
//
// Chain (1:1, zero porting divergence — the REAL ArkTS sources are compiled
// by the same TypeScript transpile used by the T216 suites; no rewrites):
//   WaterSandField + P011ParticleHot/M2Field/M2Sink + derived/ nine files
//   (V2 facade/barrier/commit), driven through P051DerivedFacadeV2 exactly
//   as the device page P051DerivedV2Demo drives it (page model
//   P051DerivedDemoV2.tick calls owner.inject then owner.step — the facade
//   wraps each command with startDerivedCommandV2/finishDerivedCommandV2).
//
// Sequence (T345 §3): new P051DerivedFacadeV2(config) → inject(×3, same T343
// parameters requestedH = 8×sec, y=220, x=64, sourceId=51, seed=51) →
// step(1/60)×600 (10 s physics) → every 60 steps dump five items:
//   a) binding.liveCount + live pool pY min/mean/max
//   b) work.planA.motionCount / requestCount  (did collect produce rows?)
//   c) work.mediumMode  (checkpoint §2 c': must be 'DRY_AIR_V1')
//   d) derivedV2.status + counters snapshot   (barrier fault?)
//   e) mh+ml column sums                      (did the bed grow?)
//
// Verdict (mutually exclusive, no hedging):
//   pY decreases window-over-window → OFFLINE_NOT_REPRODUCED → the break is
//     on the device layer (page lifecycle / sensor clock) → route to the
//     tester device-evidence card (T345 §3 device hook).
//   pY stays 220 (frozen) → OFFLINE_REPRODUCED → activate the three §2
//     candidates one by one to locate the break line.
//
// Output: test/reports/t346/harness-run.json (+ console).
const assert = require('assert/strict'), fs = require('fs'), path = require('path');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } }).outputText, f);

const { P051DerivedFacadeV2 } = require(path.resolve('entry/src/main/ets/physics/derived/P051DerivedFacadeV2.ets'));
const { P051_DRY_REVISION } = require(path.resolve('entry/src/main/ets/physics/P051DryParticleAdmission.ets'));

// T343 device parameters (test-report-T343 §1): 28 injects at x=64 y=220,
// requestedH = 8×seconds, requestedL = 0, seed 51, sourceId 51, nmax 512
// (P051DerivedDemoV2 default constructor).
const NMAX = 512, X = 64, Y = 220, SOURCE = 51, SEED = 51;
const INJECTS = [ // three admissions, seconds chosen from the T343 log spread
  { sequence: 1, inputTimeMs: 1, seconds: 0.0777 },
  { sequence: 2, inputTimeMs: 2, seconds: 0.1329 },
  { sequence: 3, inputTimeMs: 3, seconds: 0.1440 },
];
const STEPS = 600, DT = 1 / 60, DUMP_EVERY = 60;

// Cold readout helpers (no re-wiring; read-only views on live objects).
function poolStats(field) {
  const b = field.particleBinding; // page-visible cold accessor (used by p073)
  const p = b.pool;
  let live = 0, pYmin = Infinity, pYmax = -Infinity, pYsum = 0, pVyAbsMax = 0;
  for (let i = 0; i < p.nmax; i++) if (p.pState[i] !== 0) {
    live++;
    pYmin = Math.min(pYmin, p.pY[i]); pYmax = Math.max(pYmax, p.pY[i]);
    pYsum += p.pY[i]; pVyAbsMax = Math.max(pVyAbsMax, Math.abs(p.pVy[i]));
  }
  return { liveCount: live,
    pYmin: live === 0 ? null : pYmin, pYmean: live === 0 ? null : pYsum / live,
    pYmax: live === 0 ? null : pYmax, pVyAbsMax: live === 0 ? null : pVyAbsMax };
}
function planStats(field) {
  const w = field.particleBinding.work;
  return { motionCount: w.planA.motionCount, requestCount: w.planA.requestCount,
    totalRequested: w.planA.totalRequested };
}
function mediumMode(field) { return field.particleBinding.work.mediumMode; }
function bedSums(field) {
  const s = field.snapshot();
  let mh = 0, ml = 0, bh = 0, bl = 0;
  for (let c = 0; c < 128; c++) { mh += s.mh[c]; ml += s.ml[c]; bh += s.bh[c]; bl += s.bl[c]; }
  return { mh, ml, bh, bl };
}

function runHarness() {
  const facade = new P051DerivedFacadeV2({ isolated: true,
    numericPolicy: 'P011_DRY_CACHE_EXACT_V2', nmax: NMAX, sourceId: SOURCE,
    backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
  const injectReceipts = [];
  for (const inj of INJECTS) {
    const r = facade.inject({ revision: P051_DRY_REVISION, sourceId: SOURCE,
      sequence: inj.sequence, inputTimeMs: inj.inputTimeMs, x: X, y: Y, seed: SEED,
      requestedH: 8 * inj.seconds, requestedL: 0 });
    injectReceipts.push({ sequence: inj.sequence, ok: r.ok,
      admittedH: r.original ? r.original.admittedH : null,
      rejectedReason: r.original ? r.original.rejectedReason : 'noReceipt',
      derivedFault: r.derivedFault ? r.derivedFault.code : null });
    if (!r.ok) throw new Error('INJECT_FAILED seq=' + inj.sequence + ' ' +
      JSON.stringify(injectReceipts[injectReceipts.length - 1]));
  }
  const windows = [];
  let stepFailures = 0, firstStepFailure = null;
  for (let i = 0; i < STEPS; i++) {
    const r = facade.step(DT);
    if (!r.ok) { stepFailures++;
      if (firstStepFailure === null) firstStepFailure = { step: i,
        originalError: r.originalError, derivedFault: r.derivedFault ? r.derivedFault.code : null,
        reason: r.derivedFault ? r.derivedFault.reason : null }; }
    if ((i + 1) % DUMP_EVERY === 0) {
      const field = facade.field; // private in ArkTS, plain property in Node transpile
      const work = field.particleBinding.work;
      const ps = poolStats(field);
      windows.push({ window: (i + 1) / DUMP_EVERY, step: i + 1,
        ...ps, ...planStats(field), mediumMode: mediumMode(field),
        derivedStatus: field.derivedV2.status,
        derivedFault: field.derivedV2.status !== 0 ? field.derivedV2.fault.reason : null,
        ...bedSums(field) });
    }
  }
  return { injectReceipts, windows, stepFailures, firstStepFailure };
}

function verdict(windows) {
  // pY frozen means every window's pYmin/pYmax stayed at the birth value 220
  // (no descent at all). Any monotone window-over-window pY drop = reproduced
  // dynamics.
  let descended = 0, froze = 0;
  for (let k = 1; k < windows.length; k++) {
    const a = windows[k - 1], b = windows[k];
    if (a.pYmin !== null && b.pYmin !== null && b.pYmin < a.pYmin - 1e-9) descended++;
    else if (b.pYmin !== null && b.pYmin >= 220 - 1e-9) froze++;
  }
  const last = windows[windows.length - 1];
  const settled = last.pYmax !== null && last.pYmax <= 1e-6;
  return { descendedWindows: descended, frozenAt220: froze,
    finalPYmin: last.pYmin, finalPYmean: last.pYmean, finalPYmax: last.pYmax,
    finalLive: last.liveCount, bedGrew: last.mh + last.ml > 1e-9,
    conclusion: (last.pYmin !== null && last.pYmin >= 220 - 1e-9 && last.pYmax !== null && last.pYmax <= 220 + 1e-6)
      ? 'OFFLINE_REPRODUCED' : 'OFFLINE_NOT_REPRODUCED' };
}

const report = { revision: 'T346_V2_NO_SETTLE_OFFLINE_REPRO_V1', device: 'NOT_RUN',
  parameters: { nmax: NMAX, x: X, y: Y, injects: INJECTS, steps: STEPS, dt: DT } };
let run;
try { run = runHarness(); }
catch (e) { report.fatal = String(e && e.stack || e).slice(0, 2000);
  fs.mkdirSync('test/reports/t346', { recursive: true });
  fs.writeFileSync('test/reports/t346/harness-run.json', JSON.stringify(report, null, 2) + '\n');
  console.error(report.fatal); process.exit(1); }
report.injectReceipts = run.injectReceipts;
report.windows = run.windows;
report.stepFailures = run.stepFailures;
report.firstStepFailure = run.firstStepFailure;
report.verdict = verdict(run.windows);
fs.mkdirSync('test/reports/t346', { recursive: true });
fs.writeFileSync('test/reports/t346/harness-run.json', JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({ verdict: report.verdict,
  stepFailures: report.stepFailures, injects: report.injectReceipts,
  firstWindow: report.windows[0], lastWindow: report.windows[report.windows.length - 1] }, null, 2));
