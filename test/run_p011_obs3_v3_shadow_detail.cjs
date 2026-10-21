#!/usr/bin/env node
// Focused shadow audit detail: full transition records (type/alpha/segment/edge)
// for selected A values, plus ramp-leg transitions (fixing the serialization gap).
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
const CAND = { thetaStartDeg: 26, thetaStopDeg: 20 };

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
function shadowRunDetail(A) {
  const cmds = buildCommands(A);
  const transitions = [];
  const effBySeg = new Map();
  const field = new WaterSandField(COLS, CAPACITY);
  const params = baseParams();
  params.angleDeg = 4;
  field.configure(params);
  field.initialize(thickScene(COLS));
  const records = [];
  const probe = new P011NaturalObservationProbe(COLS, record => records.push(record));
  field.setNaturalObservationProbe(probe);
  field.setNaturalObservationRunMetadata('calibration', 'shadow-audit-v3-detail',
    CAND.thetaStartDeg, CAND.thetaStopDeg);
  field.setTilt(0);
  let cmdIndex = 0, simMs = 0, frameCount = 0, blockedTotal = 0;
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
    if (!result.ok) { blockedTotal += 1; continue; }
    const rec = records[records.length - 1];
    if (rec && rec.context) {
      const segId = rec.context.segmentId || 'H0';
      if (!effBySeg.has(segId)) effBySeg.set(segId, { min: Infinity, max: -Infinity, n: 0 });
      const e = effBySeg.get(segId);
      const eff = Number(rec.context.effectiveTilt);
      if (Number.isFinite(eff)) { e.min = Math.min(e.min, eff); e.max = Math.max(e.max, eff); }
      e.n += 1;
      const edges = rec.relaxHysteresis ? rec.relaxHysteresis.edges || [] : [];
      for (const ed of edges) {
        if (ed.transitionType === 'start' || ed.transitionType === 'stop') {
          transitions.push({ seg: segId, type: ed.transitionType,
            edge: ed.interfaceIndex,
            alpha: Number.isFinite(ed.decisionAlpha) ? Math.round(ed.decisionAlpha * 1000) / 1000 : null,
            effTilt: Math.round(Number(rec.context.effectiveTilt) * 1000) / 1000 });
        }
      }
    }
  }
  return { A, blockedTotal, transitions, effBySeg:
    Object.fromEntries([...effBySeg.entries()].map(([k, v]) => [k,
      { min: Math.round(v.min*1000)/1000, max: Math.round(v.max*1000)/1000, n: v.n }])) };
}
const A_LIST = (process.argv[2] || '19.4,19.5,20,15.1,16.6,18.5').split(',').map(Number);
const out = A_LIST.map(A => shadowRunDetail(A));
const path = process.argv[3];
if (path) fs.writeFileSync(path, JSON.stringify(out, null, 1));
for (const r of out) {
  const starts = r.transitions.filter(t => t.type === 'start');
  const stops = r.transitions.filter(t => t.type === 'stop');
  const bySeg = {};
  for (const t of r.transitions) {
    const key = t.seg + '/' + t.type;
    bySeg[key] = (bySeg[key] || 0) + 1;
  }
  console.log(`A=${r.A}: blocked=${r.blockedTotal} starts=${starts.length} stops=${stops.length}`,
    JSON.stringify(bySeg));
  if (starts.length) console.log('  first starts:', JSON.stringify(starts.slice(0, 4)));
  if (stops.length) console.log('  first stops:', JSON.stringify(stops.slice(0, 4)));
}
