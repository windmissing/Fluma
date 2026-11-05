'use strict';
// T354 independent frame-budget checker.
// Oracle independence: this script ONLY consumes test/reports/t351/budget.json raw fields
// and re-derives expectations with its own arithmetic. It does NOT import or call any
// test/t351 or production assertion code.
// Outputs: test/reports/t354/t354_independent_check.json (+ stdout summary).
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const budgetPath = path.join(root, 'test/reports/t351/budget.json');
const outPath = path.join(root, 'test/reports/t354/t354_independent_check.json');

const budget = JSON.parse(fs.readFileSync(budgetPath, 'utf8'));
const findings = [];

function record(id, name, verdict, detail) {
  findings.push({ id, name, verdict, detail });
  console.log(`[${verdict}] ${id} ${name} :: ${detail}`);
}

// ---------- O1: exclusive stage-sum closure per profile ----------
// Independent expectation: stage totals are exclusive; a sum greater than the total wall
// budget upper bound (frames * wallP95, converted consistently) would indicate double count.
// NOTE: stages and wall are in DIFFERENT units in the asset (stage p50/total are per-frame
// ms statistics; wall is ms). The only unit-safe independent relation is sum(stage total)
// <= frames * max(stage max) per stage — i.e. totals cannot exceed frames*per-stage-max.
let o1Pass = 0, o1Fail = 0, o1Skipped = 0;
for (const p of budget.profiles) {
  let violated = false;
  for (const s of p.stages) {
    const mx = s.max !== undefined ? s.max : s.p95;
    if (mx === undefined || mx === null) continue;
    // 0.5ms/frame tolerance absorbs integer-ms rounding (Date.now granularity): a 1ms total
    // spread over 24 frames rounds every per-frame sample down to 0.
    const bound = Math.max(mx, 0.5) * p.frames;
    if (s.total > bound + 1e-9) { violated = true; record('O1', 'stage total vs frames*max', 'FAIL',
      `profile ${JSON.stringify(p.scenario)} ${p.mode} stage ${s.name}: total ${s.total} > frames*max ${bound.toFixed(2)}`); }
  }
  const sum = p.stages.reduce((a, s) => a + (s.total || 0), 0);
  if (sum === 0) { o1Skipped++; continue; }
  if (!violated) o1Pass++;
}
if (o1Fail === 0) record('O1', 'stage totals bounded by frames*per-stage-max', 'PASS',
  `${o1Pass} profiles, ${o1Skipped} zero-work skipped`);

// ---------- O2: authority counter relations ----------
// Empirically grounded independent relations (no assumed per-call granularity):
// O2a differential: for same scenario, after-mode calls < before-mode calls (when before>0).
// O2b diagnostic-independence: after-off/after-on/after-alternating calls identical.
// O2c live monotonicity: before calls non-decreasing in live count within same n.
let o2aPass = 0, o2aFail = 0, o2aNodata = 0;
let o2bPass = 0, o2bFail = 0;
let o2cPass = 0, o2cFail = 0;
const byScenario = new Map();
for (const p of budget.profiles) {
  const key = JSON.stringify(p.scenario);
  if (!byScenario.has(key)) byScenario.set(key, []);
  byScenario.get(key).push(p);
}
for (const [key, list] of byScenario) {
  const before = list.find(p => p.mode === 'before');
  const afters = list.filter(p => p.mode.startsWith('after'));
  if (before && afters.length) {
    const b = before.authorityCounterCalls || 0;
    const a0 = afters[0].authorityCounterCalls || 0;
    if (b === 0) { o2aNodata++; }
    else if (a0 < b) o2aPass++;
    else { o2aFail++; record('O2a', 'after < before calls', 'FAIL', `${key}: ${a0} >= ${b}`); }
  } else o2aNodata++;
  if (afters.length >= 2) {
    const vals = afters.map(p => p.authorityCounterCalls || 0);
    if (new Set(vals).size === 1) o2bPass++;
    else { o2bFail++; record('O2b', 'diagnostic-independence of counters', 'FAIL', `${key}: ${vals.join(',')}`); }
  }
  if (before) {
    const sortedByLive = byScenario.get(key).filter(p => p.mode === 'before').sort((a, b) => (a.scenario.l || 0) - (b.scenario.l || 0));
    if (sortedByLive.length >= 2) {
      let mono = true;
      for (let i = 1; i < sortedByLive.length; i++) {
        const c0 = sortedByLive[i - 1].authorityCounterCalls || 0;
        const c1 = sortedByLive[i].authorityCounterCalls || 0;
        if (c1 + 1e-9 < c0) { mono = false; record('O2c', 'before calls live-monotonic', 'FAIL',
          `${key}: l=${sortedByLive[i-1].scenario.l} calls ${c0} > l=${sortedByLive[i].scenario.l} calls ${c1}`); }
      }
      if (mono) o2cPass++;
    }
  }
}
if (o2aFail === 0 && o2bFail === 0 && o2cFail === 0) {
  record('O2', 'authority counter relations (differential/diagnostic-independence/live-monotonic)', 'PASS',
    `O2a ${o2aPass} pass/${o2aNodata} nodata, O2b ${o2bPass}, O2c ${o2cPass}`);
}

// ---------- O3: live-scaling monotonicity on per-particle stages ----------
// Same mode, same n: hotApply+collect totals must not decrease as endLive grows.
let o3Pass = 0, o3Violations = 0, o3Unknown = 0;
for (const [key, list] of byScenario) {
  for (const mode of ['before', 'after-off']) {
    const rows = list.filter(p => p.mode === mode).sort((a, b) => a.endLive - b.endLive);
    if (rows.length < 2) continue;
    const work = (p) => {
      const g = (n) => { const s = p.stages.find(x => x.name === n); return s ? s.total : 0; };
      return g('hotApply') + g('collect');
    };
    for (let i = 1; i < rows.length; i++) {
      const w0 = work(rows[i - 1]), w1 = work(rows[i]);
      if (w0 === 0 && w1 === 0) { o3Unknown++; continue; }
      if (w1 + 1e-9 < w0) { o3Violations++; record('O3', 'live-scaling monotonicity', 'FAIL',
        `${key} ${mode}: live ${rows[i-1].endLive}->${rows[i].endLive} work ${w0.toFixed(2)}->${w1.toFixed(2)} decreased`); }
      else o3Pass++;
    }
  }
}
if (o3Violations === 0) record('O3', 'live-scaling monotonicity (where measurable)', 'PASS', `${o3Pass} ordered pairs, ${o3Unknown} zero-work skipped`);

// ---------- O4: before/after verify-work differential ----------
const g3 = (p, names) => names.reduce((a, n) => a + (p.stages.find(s => s.name === n)?.total || 0), 0);
const NAMES = ['publicVerify', 'authorityCompare', 'exact70'];
let o4Pass = 0, o4Fail = 0, o4Nodata = 0;
for (const [key, list] of byScenario) {
  const before = list.find(p => p.mode === 'before');
  const after = list.find(p => p.mode.startsWith('after'));
  if (!before || !after) { o4Nodata++; continue; }
  const b = g3(before, NAMES), a = g3(after, NAMES);
  if (b === 0) { o4Nodata++; continue; }
  if (a <= b + 1e-9) o4Pass++;
  else { o4Fail++; record('O4', 'before/after differential', 'FAIL', `${key}: verify-work before ${b.toFixed(2)}ms -> after ${a.toFixed(2)}ms increased`); }
}
if (o4Fail === 0) record('O4', 'before/after verify-work differential', 'PASS', `${o4Pass} improved, ${o4Nodata} no-before-data`);

// ---------- O5: first-error bit-typing & counter oracle (RED expected) ----------
function exact70Probe() {
  const unit = Math.pow(2, -54);
  const a = 3 * unit, neg = -a;
  return (0 + neg + a) === 0 && a > 0 && neg < 0;
}
if (exact70Probe()) record('O5', 'independent exact70 probe (oracle self-check)', 'PASS', '2^-54 boundary arithmetic self-consistent');
else record('O5', 'independent exact70 probe', 'FAIL', 'oracle arithmetic broken');
record('O5b', 'first-error bit-typing asset check', 'NOT_IMPLEMENTED',
  't351 budget.json carries no per-boundary first-error records; RED baseline — needs T351-side export (not a product failure)');

// ---------- summary ----------
const summary = {
  revision: 'T354_INDEPENDENT_BUDGET_CHECK_V1',
  consumed: { file: 'test/reports/t351/budget.json', profiles: budget.profiles.length },
  oracle: 'independent (no t351/production assertion code imported)',
  findings,
  verdict: findings.every(f => f.verdict !== 'FAIL') ? 'NO_FAIL_RED_BASELINE' : 'FAIL_PRESENT'
};
fs.mkdirSync(path.dirname(outPath), { recursive: true });
fs.writeFileSync(outPath, JSON.stringify(summary, null, 2));
console.log('\nT354 verdict:', summary.verdict, '->', outPath);
