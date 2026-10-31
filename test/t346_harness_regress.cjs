'use strict';
// T346 regression form: the T345 harness as a permanent unit — asserts the
// offline verdict (particles descend + bed grows through the V2 facade),
// keeping the decisive experiment re-runnable in every future suite run.
const assert = require('assert/strict'), fs = require('fs'), path = require('path');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } }).outputText, f);
const { P051DerivedFacadeV2 } = require(path.resolve('entry/src/main/ets/physics/derived/P051DerivedFacadeV2.ets'));
const { P051_DRY_REVISION } = require(path.resolve('entry/src/main/ets/physics/P051DryParticleAdmission.ets'));

const NMAX = 512, X = 64, Y = 220, SOURCE = 51, SEED = 51;
const INJECTS = [{ sequence: 1, inputTimeMs: 1, seconds: 0.0777 },
  { sequence: 2, inputTimeMs: 2, seconds: 0.1329 }, { sequence: 3, inputTimeMs: 3, seconds: 0.1440 }];
let passed = 0;
function test(name, fn) { fn(); passed++; console.log('PASS ' + name); }

test('T346-R1 offline V2 chain settles: pY descends, particles land, bed grows', () => {
  const facade = new P051DerivedFacadeV2({ isolated: true, numericPolicy: 'P011_DRY_CACHE_EXACT_V2',
    nmax: NMAX, sourceId: SOURCE, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
  for (const inj of INJECTS)
    assert.equal(facade.inject({ revision: P051_DRY_REVISION, sourceId: SOURCE, sequence: inj.sequence,
      inputTimeMs: inj.inputTimeMs, x: X, y: Y, seed: SEED, requestedH: 8 * inj.seconds, requestedL: 0 }).ok, true);
  let pYminFirst = null, pYmaxFirst = null, final = null;
  for (let i = 0; i < 600; i++) {
    const r = facade.step(1 / 60);
    assert.equal(r.ok, true, 'step ' + i + ' failed');
    if (i === 59) {
      const p = facade.field.particleBinding.pool;
      for (let k = 0; k < p.nmax; k++) if (p.pState[k] !== 0) {
        pYminFirst = pYminFirst === null ? p.pY[k] : Math.min(pYminFirst, p.pY[k]);
        pYmaxFirst = pYmaxFirst === null ? p.pY[k] : Math.max(pYmaxFirst, p.pY[k]);
      }
    }
    if (i === 599) {
      const b = facade.field.particleBinding;
      const s = facade.field.snapshot();
      let mh = 0, ml = 0, live = 0, pYmax = -Infinity;
      for (let c = 0; c < 128; c++) { mh += s.mh[c]; ml += s.ml[c]; }
      for (let k = 0; k < b.pool.nmax; k++) if (b.pool.pState[k] !== 0) { live++; pYmax = Math.max(pYmax, b.pool.pY[k]); }
      final = { mh, ml, live, pYmax };
    }
  }
  // Descent is monotone and material: window-1 pY already below birth y=220.
  assert.ok(pYminFirst < 220 - 1, 'window-1 pYmin=' + pYminFirst + ' must be below 220');
  // Bed grew (particles deposited) and survivors sit at the bed surface.
  assert.ok(final.mh + final.ml > 1e-9, 'bed must grow, mh=' + final.mh + ' ml=' + final.ml);
  assert.ok(final.pYmax < 1, 'survivors must sit on the bed, pYmax=' + final.pYmax);
});

test('T346-R2 medium mode and barrier stay healthy through the whole run', () => {
  const facade = new P051DerivedFacadeV2({ isolated: true, numericPolicy: 'P011_DRY_CACHE_EXACT_V2',
    nmax: NMAX, sourceId: SOURCE, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
  for (const inj of INJECTS)
    facade.inject({ revision: P051_DRY_REVISION, sourceId: SOURCE, sequence: inj.sequence,
      inputTimeMs: inj.inputTimeMs, x: X, y: Y, seed: SEED, requestedH: 8 * inj.seconds, requestedL: 0 });
  for (let i = 0; i < 600; i++) {
    facade.step(1 / 60);
    const f = facade.field;
    assert.equal(f.particleBinding.work.mediumMode, 'DRY_AIR_V1');
    assert.equal(f.derivedV2.status, 0, 'barrier fault at step ' + i + ': ' + f.derivedV2.fault.reason);
  }
});

console.log('T346 regression: ' + passed + ' passed');
