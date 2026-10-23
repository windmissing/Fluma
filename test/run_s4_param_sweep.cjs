// P-011 S4 deep-creep calibration sweep (§3.1/§4.1-S6):
// 60 combinations (k_deep 5 × mu_y 4 × hMin 3) × 120 s per run,
// five automatic metrics (A_bh / L_bh / T_bh / zero-flux gate /
// attribution breakdown). Node channel; seed fixed 20261023.
'use strict';
const fs = require('fs');
const path = require('path');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), {
  compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }
}).outputText, filename);

const physics = require('../entry/src/main/ets/physics/WaterSandField.ets');
const { WaterSandField, defaultWaterSandParams,
  P011_DEEP_CREEP_ROUTE_ID } = physics;

const COLS = 128;
const CAP = 512;
const RUN_SECONDS = 300;
const DT = 1 / 60;
// Sweep v2: the dormant-freeze diagnosis (v1 ran ONE step per combo)
// landed; the k grid now spans across the exploration window — from the
// v1 top (0.05) up two decades (1.0) plus finer steps in the promising
// band. mu_y > 0 stays in the grid for the p_eff-normalization evidence.
// In-domain only (fail-closed cap kDeep ≤ 0.2); k > 0.2 needs the
// architect's domain revision — covered by the mechanism-domain
// analysis in the v2 report instead of live runs.
const K_GRID = [0.01, 0.02, 0.05, 0.1, 0.2];
const MU_GRID = [0, 0.1];
const HMIN_GRID = [0.5, 1.0, 2.0];
// pxPerMaterial from the device geometry (192-cap box ≈ 80 display px
// per material unit — the task doc's calibration constant).
const PX_PER_MATERIAL = 2.4;

function seededProfile(seed) {
  // Deterministic multi-scale bh perturbation (the S-flat interface the
  // sweep measures): a sum of three sinusoids with seeded phases.
  const bh = new Array(COLS);
  let s = seed >>> 0;
  const rand = () => {
    s = (Math.imul(s ^ (s >>> 15), s | 1) ^ s + Math.imul(s ^ (s >>> 7), s | 61)) >>> 0;
    return ((s ^ (s >>> 14)) >>> 0) / 4294967296;
  };
  const p1 = rand() * Math.PI * 2;
  const p2 = rand() * Math.PI * 2;
  const p3 = rand() * Math.PI * 2;
  for (let c = 0; c < COLS; c++) {
    bh[c] = 100 + 12 * Math.sin(c * 0.13 + p1) +
      6 * Math.sin(c * 0.31 + p2) + 3 * Math.sin(c * 0.71 + p3);
  }
  return bh;
}

function makeField(bhProfile) {
  const field = new WaterSandField(COLS, CAP);
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0; params.lightDiffusion = 0;
  params.waterLevelRate = 0.001; params.angleDeg = 4;
  params.relaxMode = 'legacy-single';
  params.thetaStartDeg = undefined; params.thetaStopDeg = undefined;
  params.thresholdSource = 'legacy';
  field.configure(params);
  const heavyRuns = [];
  const lightRuns = [];
  let hr = null;
  let lr = null;
  const total = bhProfile[0] + 100;
  for (let c = 0; c < COLS; c++) {
    const bh = bhProfile[c];
    const bl = total - bh;
    if (hr === null || Math.abs(hr.height - bh) > 1e-12) {
      if (hr !== null) {
        heavyRuns.push(hr);
      }
      hr = { startCol: c, endColInclusive: c, bottom: 0, height: bh };
    } else {
      hr.endColInclusive = c;
    }
    if (lr === null || Math.abs(lr.bottom - bh) > 1e-12) {
      if (lr !== null) {
        lightRuns.push(lr);
      }
      lr = { startCol: c, endColInclusive: c, bottom: bh, height: bl };
    } else {
      lr.endColInclusive = c;
    }
  }
  heavyRuns.push(hr);
  lightRuns.push(lr);
  field.initialize({
    waterRegions: [{ startCol: 0, endColInclusive: COLS - 1,
      bottom: 300, height: 160 }],
    heavyDepositedRegions: heavyRuns,
    lightDepositedRegions: lightRuns,
    heavySuspendedRegions: [], lightSuspendedRegions: []
  });
  return field;
}

/** Detrended P90-P10 amplitude of the bh profile (px). */
function amplitudePx(bh) {
  // Wide moving average (half the domain) detrend.
  const w = Math.floor(COLS / 2);
  const detrended = [];
  for (let c = 0; c < COLS; c++) {
    let sum = 0;
    let n = 0;
    for (let d = -w / 2; d < w / 2; d++) {
      const cc = c + Math.floor(d);
      if (cc >= 0 && cc < COLS) {
        sum += bh[cc];
        n += 1;
      }
    }
    detrended.push(bh[c] - sum / n);
  }
  const sorted = [...detrended].sort((a, b) => a - b);
  const p10 = sorted[Math.floor(COLS * 0.10)];
  const p90 = sorted[Math.floor(COLS * 0.90)];
  return (p90 - p10) * PX_PER_MATERIAL;
}

/** First-1/e autocorrelation distance (columns) + jump P95. */
function structureMetrics(bh) {
  const mean = bh.reduce((a, b) => a + b, 0) / COLS;
  const centered = bh.map(v => v - mean);
  let var0 = 0;
  for (const v of centered) {
    var0 += v * v;
  }
  var0 /= COLS;
  let lag1e = COLS;
  for (let lag = 1; lag < COLS; lag++) {
    let cov = 0;
    for (let c = 0; c + lag < COLS; c++) {
      cov += centered[c] * centered[c + lag];
    }
    cov /= Math.max(1, COLS - lag);
    if (cov / Math.max(var0, 1e-12) < 1 / Math.E) {
      lag1e = lag;
      break;
    }
  }
  const jumps = [];
  for (let c = 0; c < COLS - 1; c++) {
    jumps.push(Math.abs(bh[c + 1] - bh[c]));
  }
  jumps.sort((a, b) => a - b);
  return {
    lag1e: lag1e,
    jumpP95: jumps[Math.floor(COLS * 0.95)] * PX_PER_MATERIAL
  };
}

function runOnce(k, mu, hMin, uniform) {
  const profile = uniform
    ? new Array(COLS).fill(100)
    : seededProfile(20261023);
  const field = makeField(profile);
  field.setDeepCreepParams({ kDeep: k, muY: mu, hMin });
  field.setNaturalObservationRouteId(P011_DEEP_CREEP_ROUTE_ID);
  field.setTilt(0);
  const steps = Math.floor(RUN_SECONDS / DT);
  let firstFluxStep = -1;
  let t50samples = [];
  let attrAccum = { noDrive: 0, noInventory: 0, budgetClipped: 0 };
  const t0 = Date.now();
  for (let i = 0; i < steps; i++) {
    field.step(DT);
    const attr = field.getDeepCreepAttribution();
    attrAccum.noDrive += attr.noDrive;
    attrAccum.noInventory += attr.noInventory;
    attrAccum.budgetClipped += attr.budgetClipped;
    if (attr.totalUsed > 0 && firstFluxStep < 0) {
      firstFluxStep = i;
    }
    // T_bh: track a rolling amplitude for the 50% arrival estimate.
    if (i % 600 === 0 && i > 0) {
      const snap = field.snapshot();
      t50samples.push({ t: i * DT, amp: amplitudePx(Array.from(snap.bh)) });
    }
  }
  const elapsedMs = Date.now() - t0;
  const snap = field.snapshot();
  const bh = Array.from(snap.bh);
  const totals = field.getTotals();
  const attrFinal = field.getDeepCreepAttribution();
  // Decay: the initial amplitude vs the final (the S4 share of the
  // relaxation — other channels are near-static in this fixture).
  const amplitude0 = uniform ? 0 : amplitudePx(profile);
  const amplitude1 = uniform ? 0 : amplitudePx(bh);
  const metrics = {
    kDeep: k, muY: mu, hMin,
    uniformArm: uniform,
    amplitudePx: amplitude1,
    amplitude0Px: amplitude0,
    decayRatio: amplitude0 > 0 ? amplitude1 / amplitude0 : 1,
    ...structureMetrics(bh),
    totalUsed: attrFinal.totalUsed,
    fluxSteps: firstFluxStep,
    elapsedMs,
    driftHeavy: Math.abs(totals.heavy - (12800)),
    attribution: attrAccum
  };
  return metrics;
}

// Sweep: 60 combos × (perturbed + uniform arms).
const results = [];
let idx = 0;
for (const k of K_GRID) {
  for (const mu of MU_GRID) {
    for (const h of HMIN_GRID) {
      idx += 1;
      const perturbed = runOnce(k, mu, h, false);
      const uniform = runOnce(k, mu, h, true);
      // Zero-flux HARD gate on the uniform arm.
      uniform.gate = uniform.totalUsed === 0 ? 'PASS' : 'FAIL';
      results.push({ combo: idx, perturbed, uniform });
      console.log(`[${idx}/60] k=${k} mu=${mu} hMin=${h} ` +
        `A_bh=${perturbed.amplitudePx.toFixed(2)}px ` +
        `L(lag1e)=${perturbed.lag1e} jumpP95=${perturbed.jumpP95.toFixed(2)} ` +
        `uniformFlux=${uniform.totalUsed.toFixed(6)} gate=${uniform.gate}`);
    }
  }
}

// Summary: exploration-domain hits (A_bh in 2-8 px).
const domain = results.filter(r => r.perturbed.amplitudePx >= 2 &&
  r.perturbed.amplitudePx <= 8);
const failed = results.filter(r => r.uniform.gate === 'FAIL');

const report = {
  suite: 's4_param_sweep_v2',
  seed: 20261023,
  runSeconds: RUN_SECONDS,
  combos: results.length,
  explorationDomainHits: domain.map(r => ({
    k: r.perturbed.kDeep, mu: r.perturbed.muY, h: r.perturbed.hMin,
    amplitudePx: +r.perturbed.amplitudePx.toFixed(3),
    lag1e: r.perturbed.lag1e,
    jumpP95: +r.perturbed.jumpP95.toFixed(3)
  })),
  zeroFluxGateFailures: failed.map(r => r.combo),
  summaryTable: results.map(r => ({
    k: r.perturbed.kDeep, mu: r.perturbed.muY, h: r.perturbed.hMin,
    ampPx: +r.perturbed.amplitudePx.toFixed(3),
    amp0Px: +r.perturbed.amplitude0Px.toFixed(3),
    decay: +r.perturbed.decayRatio.toFixed(4),
    lag: r.perturbed.lag1e,
    used: +r.perturbed.totalUsed.toFixed(4),
    uniUsed: +r.uniform.totalUsed.toFixed(4),
    noDrive: r.perturbed.attribution.noDrive,
    noInv: r.perturbed.attribution.noInventory,
    clip: r.perturbed.attribution.budgetClipped
  }))
};
fs.mkdirSync(path.join(__dirname, 'reports'), { recursive: true });
fs.writeFileSync(path.join(__dirname, 'reports', 's4-sweep-v2.json'),
  JSON.stringify(report, null, 2));
console.log('\\n=== SUMMARY ===');
console.log(`combos: ${results.length}`);
console.log(`exploration-domain (2-8px) hits: ${domain.length}`);
console.log(`zero-flux gate failures: ${failed.length}`);
process.exitCode = failed.length > 0 ? 1 : 0;
