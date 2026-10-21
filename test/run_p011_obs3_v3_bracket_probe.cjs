#!/usr/bin/env node
// Bracket width probe: per-edge per-substep decisionAlpha series near transitions.
// Verifies §5.2 bracket width <= 0.5 deg at threshold crossings (adjacent committed
// substeps) for the four main cells at candidate A values.
const fs = require('fs');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const hypium = { describe: (_n, fn) => fn(), it: () => {},
  expect: () => ({ assertEqual: () => {}, assertTrue: () => {}, assertFalse: () => {},
    assertNull: () => {}, assertUndefined: () => {}, assertContain: () => {},
    assertLarger: () => {}, assertLess: () => {}, assertFail: () => {} }) };
const originalLoad = Module._load;
Module._load = function(name, parent, main) {
  return name === '@ohos/hypium' ? hypium : originalLoad.call(this, name, parent, main);
};
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }}).outputText, filename);
const physics = require('../entry/src/main/ets/physics/WaterSandField.ets');
const observation = require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
const { WaterSandField, defaultWaterSandParams } = physics;
const { P011NaturalObservationProbe } = observation;

const COLS = 128, CAPACITY = 192, RATE = 0.5, HOLD_MS = 5100, STEP_MS = 100;
const DT = 1 / 60, FRAME_MS = 1000 / 60;

function thickScene(cols) {
  return {
    waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 144, height: 38 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 72 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 72, height: 72 }],
    heavySuspendedRegions: [], lightSuspendedRegions: []
  };
}
function baseParams() {
  const p = defaultWaterSandParams();
  p.heavyDiffusion = 0; p.lightDiffusion = 0; p.waterLevelRate = 0.001;
  return p;
}
function buildCommands(A) {
  const platforms = [0, 15, A, 15, 0, -15, -A, -15, 0];
  const cmds = []; let seq = 1, off = 0;
  cmds.push({ seq: seq++, off, seg: 'H0', kind: 'platform', leg: '', target: 0 });
  off += HOLD_MS;
  const pairs = [[0,1],[1,2],[2,3],[3,4],[4,5],[5,6],[6,7],[7,8]];
  for (let li = 0; li < pairs.length; li++) {
    const [f, t] = pairs[li];
    const legId = 'L' + (li + 1);
    const t0 = platforms[f], t1 = platforms[t];
    const n = Math.round(Math.abs(t1 - t0) / (RATE * STEP_MS / 1000));
    cmds.push({ seq: seq++, off, seg: legId, kind: 'ramp', leg: legId, target: t0 });
    off += STEP_MS;
    for (let s = 1; s <= n; s++) {
      cmds.push({ seq: seq++, off, seg: legId, kind: 'ramp', leg: legId,
        target: t0 + (t1 - t0) * s / n });
      off += STEP_MS;
    }
    cmds.push({ seq: seq++, off, seg: 'H' + t, kind: 'platform', leg: '', target: t1 });
    off += HOLD_MS;
  }
  return cmds;
}
// per-edge alpha series per segment (only the激励 legs and peak platforms)
function shadowRunBrackets(A) {
  const cmds = buildCommands(A);
  const alphaSeries = new Map(); // seg -> Map(edge -> [alphas])
  const records = [];
  const probe = new P011NaturalObservationProbe(COLS, record => records.push(record));
  const field = new WaterSandField(COLS, CAPACITY);
  const params = baseParams();
  params.angleDeg = 4;
  field.configure(params);
  field.initialize(thickScene(COLS));
  field.setNaturalObservationProbe(probe);
  field.setNaturalObservationRunMetadata('calibration', 'shadow-bracket-probe', 26, 20);
  field.setTilt(0);
  let cmdIndex = 0, simMs = 0, frameCount = 0, blocked = 0;
  const totalMs = cmds[cmds.length - 1].off + HOLD_MS;
  while (simMs < totalMs && frameCount < 200000) {
    while (cmdIndex < cmds.length && cmds[cmdIndex].off <= simMs) {
      const c = cmds[cmdIndex];
      field.setTilt(c.target);
      field.setP011CurrentCommandSegment(c.leg || null, c.seg, c.kind);
      cmdIndex += 1;
    }
    const result = field.step(DT);
    frameCount += 1; simMs += FRAME_MS;
    if (!result.ok) { blocked += 1; continue; }
    const rec = records[records.length - 1];
    if (rec && rec.context) {
      const segId = rec.context.segmentId || 'H0';
      if (!['L2','L3','L6','L7','H2','H3','H6','H7'].includes(segId)) continue;
      if (!alphaSeries.has(segId)) alphaSeries.set(segId, new Map());
      const m = alphaSeries.get(segId);
      const edges = rec.relaxHysteresis ? rec.relaxHysteresis.edges || [] : [];
      for (const ed of edges) {
        if (!Number.isFinite(ed.decisionAlpha)) continue;
        const e = ed.interfaceIndex;
        if (!m.has(e)) m.set(e, []);
        m.get(e).push(Math.round(ed.decisionAlpha * 10000) / 10000);
      }
    }
  }
  // bracket analysis: for each seg/edge series, find start crossings (>=26 from <26)
  // and stop crossings (<=20 from >20) on ADJACENT values; width = |a1-a0|
  const brackets = [];
  for (const [seg, m] of alphaSeries) {
    const sign = (seg === 'L2' || seg === 'H2' || seg === 'L3' || seg === 'H3') ? '+' : '-';
    for (const [edge, arr] of m) {
      for (let i = 1; i < arr.length; i++) {
        const a0 = arr[i-1], a1 = arr[i];
        if (a0 < 26 && a1 >= 26) brackets.push({ seg, edge, type: 'start', sign,
          prev: a0, now: a1, width: Math.round((a1 - a0) * 10000) / 10000 });
        if (a0 > 20 && a1 <= 20) brackets.push({ seg, edge, type: 'stop', sign,
          prev: a0, now: a1, width: Math.round((a0 - a1) * 10000) / 10000 });
      }
    }
  }
  return { A, blocked,
    bracketsStartP: brackets.filter(b => b.type === 'start' && b.sign === '+'),
    bracketsStopP: brackets.filter(b => b.type === 'stop' && b.sign === '+'),
    bracketsStartN: brackets.filter(b => b.type === 'start' && b.sign === '-'),
    bracketsStopN: brackets.filter(b => b.type === 'stop' && b.sign === '-') };
}
const A_LIST = (process.argv[2] || '19.4,19.5,20').split(',').map(Number);
const out = A_LIST.map(A => shadowRunBrackets(A));
if (process.argv[3]) fs.writeFileSync(process.argv[3], JSON.stringify(out, null, 1));
for (const r of out) {
  const summary = (arr) => arr.length
    ? `n=${arr.length} maxWidth=${Math.max(...arr.map(b => b.width))} segs=${[...new Set(arr.map(b => b.seg))].join(',')}`
    : 'NONE';
  console.log(`A=${r.A} blocked=${r.blocked}`);
  console.log('  start/+ :', summary(r.bracketsStartP));
  console.log('  stop/+  :', summary(r.bracketsStopP));
  console.log('  start/- :', summary(r.bracketsStartN));
  console.log('  stop/-  :', summary(r.bracketsStopN));
}
