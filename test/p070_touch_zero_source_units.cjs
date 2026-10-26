#!/usr/bin/env node
/**
 * T70: touch zero-source objective evidence (tester side, offline).
 *
 * Establishes, with the REAL T62-build model bytes (P051EmptySandDemo /
 * P051GravityInputAdapter / P011GestureRateClock, verified against the T63
 * snapshot hashes), which link of the touch -> source -> admission chain can
 * and cannot produce a zero source on device:
 *
 *  A01 legal positive: real model + direct model coordinates -> nonzero
 *      source, particles, and deposition. Physics admission is NOT the
 *      blocker.
 *  B01/B02 gravity adapter freshness contract (sensor 20ms vs tick 16.7ms).
 *  B03 stale input stops touching (model.stop) before any injection —
 *      reproduces the device zero-source signature from a stale pipeline
 *      alone.
 *  C01 touching=true with NO gravity input still injects (input===null does
 *      not stop) — rules out "sensor never delivered" as a sole cause IF the
 *      adapter returned null; the adapter actually returns a stale object
 *      (never null after the first sample), so the deciding branch is
 *      applyDefinedInput(stale).
 *  D01 stale-on-every-tick keeps source at zero for 2s (device signature).
 *
 * Device-side facts that CANNOT be produced offline (recorded as gaps):
 * touch-event reception at the page (Down/Move/Up/Cancel counts), local
 * coordinates, model.start/move timing, and whether sensor.on ever delivered
 * samples (demo_gravity_config logs config but the page logs no sensor
 * sample counter). Output feeds T69 attribution and T71 RED baseline.
 *
 * Output: test/reports/p070_touch_zero_source_units.json
 */
'use strict';
const fs = require('fs'), Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(
  fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);

const {P051EmptySandDemo} = require('../entry/src/main/ets/physics/P051EmptySandDemo.ets');
const {P051GravityInputAdapter, P051GravityConfig} =
  require('../entry/src/main/ets/physics/P051GravityInputAdapter.ets');

const results = [];
function rec(id, name, ok, detail) {
  results.push(Object.assign({id, name, status: ok ? 'PASS' : 'FAIL'}, detail || {}));
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}

// A01 legal positive
{
  const m = new P051EmptySandDemo();
  m.setActive(true);
  m.start(64.5, 220);
  let ns = 1e9; const sourceRows = [];
  for (let i = 0; i < 120; i++) {
    ns += 1e9 / 60;
    if (!m.tick(ns, 1 / 60)) break;
    sourceRows.push(m.field.getMetrics().sourceInjectedHeavy);
  }
  const met = m.field.getMetrics();
  rec('T70-A01', 'real model: touching with direct model coords yields nonzero source + particles + deposition (legal positive)',
    met.sourceInjectedHeavy > 0 && met.particleMass > 0,
    {sourceInjectedHeavy: met.sourceInjectedHeavy, particleMass: met.particleMass,
     depositedMass: met.depositedMass, firstNonzeroRow: sourceRows.findIndex(v => v > 0),
     conclusion: 'physics admission accepts touch-sourced sand; device zero must originate at the page event/coordination layer'});
}

// B01/B02 adapter freshness
{
  const cfg = new P051GravityConfig();
  const g = new P051GravityInputAdapter(cfg);
  let t = 1e6;
  g.consumeAccelerometer(0.1, 0.2, 9.8, t);
  for (let i = 0; i < 100; i++) { t += 20; g.consumeAccelerometer(0.1, 0.2, 9.8, t); }
  const fresh = g.latest(t + 16);
  rec('T70-B01', 'adapter latest() fresh within 250ms (sensor 20ms cadence)',
    fresh !== null && fresh.source === 'sensor' && fresh.valid === true,
    {source: fresh && fresh.source, targetTiltDeg: fresh && fresh.targetTiltDeg});
  const stale = g.latest(t + 251);
  rec('T70-B02', 'adapter latest() beyond 250ms -> source=stale, targetTiltDeg=null, valid=true',
    stale !== null && stale.source === 'stale' && stale.targetTiltDeg === null && stale.valid === true, {});
}

// B03 stale stops touching before any injection
{
  const m2 = new P051EmptySandDemo();
  m2.setActive(true);
  m2.start(64.5, 220);
  m2.applyDefinedInput({source: 'stale', sampleTimeMs: 0, valid: true, targetTiltDeg: null, shakeInput: 0, shakeMagnitudeMps2: 0});
  m2.tick(1e9, 1 / 60);
  const met2 = m2.field.getMetrics();
  rec('T70-B03', 'one stale input stops touching (model.stop) before any injection', met2.sourceInjectedHeavy === 0,
    {sourceInjectedHeavy: met2.sourceInjectedHeavy,
     note: 'mechanism: applyDefinedInput accepts valid=true stale objects; model.tick sees input.source==="stale" and calls stop()'});
}

// C01 no gravity input at all does not stop touching
{
  const m3 = new P051EmptySandDemo();
  m3.setActive(true);
  m3.start(64.5, 220);
  for (let i = 0; i < 30; i++) m3.tick(2e9 + i * 1e9 / 60, 1 / 60);
  const met3 = m3.field.getMetrics();
  rec('T70-C01', 'touching=true with NO gravity input still injects (input===null never stops)',
    met3.sourceInjectedHeavy > 0, {sourceInjectedHeavy: met3.sourceInjectedHeavy});
}

// D01 stale-on-every-tick reproduces the device zero-source signature
{
  const m4 = new P051EmptySandDemo();
  m4.setActive(true);
  m4.start(64.5, 220);
  for (let i = 0; i < 120; i++) {
    m4.applyDefinedInput({source: 'stale', sampleTimeMs: i, valid: true, targetTiltDeg: null, shakeInput: 0, shakeMagnitudeMps2: 0});
    if (!m4.tick(1e9 + i * 1e9 / 60, 1 / 60)) break;
  }
  const met4 = m4.field.getMetrics();
  rec('T70-D01', 'stale-on-every-tick keeps source/particles zero across 2s (device zero-source signature)',
    met4.sourceInjectedHeavy === 0 && met4.particleMass === 0 && met4.depositedMass === 0,
    {sourceInjectedHeavy: met4.sourceInjectedHeavy, particleMass: met4.particleMass, depositedMass: met4.depositedMass});
}

// E01 device-log cross-check: every T63 demo sample shows input.source='sensor'
// with a fresh targetTilt (never 'stale'), so the stale mechanism is NOT
// evidenced on device — recorded as an observation, not a criterion.
{
  const glob = require('path').join(__dirname, 'artifacts', 't63-demo-retest-20261026');
  let sensor = 0, stale = 0, total = 0;
  for (const f of fs.readdirSync(glob)) {
    if (!f.endsWith('-events.txt')) continue;
    const text = fs.readFileSync(require('path').join(glob, f), 'utf8');
    for (const line of text.split('\n')) {
      const i = line.indexOf('WS_EVENT empty_sand_demo ');
      if (i < 0) continue;
      try {
        const d = JSON.parse(line.slice(i + 'WS_EVENT empty_sand_demo '.length));
        total++;
        if (d.input && d.input.source === 'sensor') sensor++;
        if (d.input && d.input.source === 'stale') stale++;
      } catch (e) { /* truncated lines stay unpatched */ }
    }
  }
  rec('T70-E01', 'T63 device samples: input.source distribution (observation; sampling every 60 frames, not exhaustive)',
    true, {totalSamples: total, sensor, stale,
     note: 'all logged samples are source=sensor with fresh targetTilt (~77.6deg resting); no stale sample logged. The stale-stop mechanism is therefore NOT evidenced by these samples; the deciding device-side facts (touch event reception and sensor delivery during the 0-250ms boundary) remain missing.'});
}

fs.mkdirSync(require('path').join(__dirname, 'reports'), {recursive: true});
const out = {
  executionId: 'p070-touch-zero-source-units-v1', date: '2026-10-26',
  generatedBy: 'test/p070_touch_zero_source_units.cjs',
  modelIdentity: 'P051EmptySandDemo / P051GravityInputAdapter / P011GestureRateClock from the shared working tree; T63 snapshot 86366dd model files are byte-identical to these (T63 build exported from 86366dd; working tree has not touched these files since)',
  results,
  chainVerdict: {
    physicsAdmission: 'RULED OUT as sole cause (A01 positive path accepts touch sand)',
    staleStopMechanism: 'REPRODUCED offline (B03/D01) but NOT evidenced on device (E01: all samples source=sensor)',
    pageEventReception: 'DEVICE GAP: the demo page logs no touch events (no Down/Move/Up/Cancel counter, no local-coordinate logging); cannot distinguish event-not-delivered vs delivered-but-coordinate-mapped vs model-started-then-stopped',
    sensorDelivery: 'DEVICE GAP: page logs config only; no per-sample counter to prove sensor.on delivered during the touch window'
  },
  deviceEvidenceNeeds: ['page-level touch event log (type/local x,y/timestamp)', 'sensor sample counter with timestamps', 'model.start/move/stop invocation log', 'one sample at serial%60 boundary during active touch with touching-state flag'],
  summary: {total: results.length, pass: results.filter(r => r.status === 'PASS').length}
};
fs.writeFileSync(require('path').join(__dirname, 'reports', 'p070_touch_zero_source_units.json'),
  JSON.stringify(out, null, 1) + '\n');
console.log('total', results.length, 'pass', out.summary.pass);
process.exitCode = results.every(r => r.status === 'PASS') ? 0 : 1;
