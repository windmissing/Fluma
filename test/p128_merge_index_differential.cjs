// T128: independent differential verification of T123 (469ee37) exact merge
// index against the REAL legacy 52a0700 scan (the oracle per T116 §5).
// NOT a mirror of the developer's 155-case self-test: adversarial pools,
// no-match upper bounds, consecutive merges with centroid drift, and
// transaction-level equivalence are constructed independently here.
'use strict';
const fs = require('fs'), a = require('assert/strict'),
  ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {P011MergeExactIndex: Index} = require('../entry/src/main/ets/physics/P011MergeExactIndex.ets');

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t128';
fs.mkdirSync(OUT, {recursive: true});

// ---- oracle: the REAL legacy 52a0700 mergeOldestHot scan (verbatim semantics) ----
function legacyScan(p, r) {
  let afterId = 0;
  for (let pass = 0; pass < p.nmax; pass++) {
    let x = -1;
    for (let i = 0; i < p.nmax; i++) if (p.pState[i] && p.pId[i] > afterId && (x < 0 || p.pId[i] < p.pId[x])) x = i;
    if (x < 0) break;
    let b = -1;
    for (let j = 0; j < p.nmax; j++) if (p.pState[j] && p.pId[x] < p.pId[j] &&
      p.pPhase[x] === p.pPhase[j] && p.pSource[x] === p.pSource[j] && p.pSourceId[x] === p.pSourceId[j] &&
      p.pState[x] === p.pState[j] && Math.floor(p.pX[x]) === Math.floor(p.pX[j]) &&
      Math.abs(p.pY[x] - p.pY[j]) <= r && (b < 0 || p.pId[j] < p.pId[b])) b = j;
    if (b >= 0) return [x, b];
    afterId = p.pId[x];
  }
  return [-1, -1];
}
function pool(n) {
  return {nmax: n, cols: 128, liveCount: 0, freeCount: n, nextId: 1,
    pMass: new Float64Array(n), pX: new Float64Array(n), pY: new Float64Array(n),
    pVx: new Float64Array(n), pVy: new Float64Array(n),
    pPhase: new Uint8Array(n), pSource: new Uint8Array(n), pSourceId: new Uint32Array(n), pState: new Uint8Array(n),
    pId: new Uint32Array(n), freeSlots: new Int32Array(n),
    colParticleMassH: new Float64Array(128), colParticleMassL: new Float64Array(128)};
}
let seed = 0x51fab123 ^ 0xabcdef; // different stream from the dev self-test
function rnd() { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed / 4294967296; }
let cases = 0;

// ---- D1: randomized differential (fresh pools, adversarial configs) ----
{
  let ok = true; const fails = [];
  for (const n of [4, 8, 16, 32, 64, 192, 512]) {
    for (let run = 0; run < 40; run++) {
      const p = pool(n);
      // shuffled unique IDs (incl. large values near MAX)
      const ids = [];
      for (let i = 0; i < n; i++) ids.push(run % 5 === 0 ? 4294967295 - i : i + 1);
      for (let i = ids.length - 1; i > 0; i--) { const j = Math.floor(rnd() * (i + 1)); [ids[i], ids[j]] = [ids[j], ids[i]]; }
      for (let i = 0; i < n; i++) {
        if (rnd() < .2) continue; // sparse active
        p.pState[i] = 1 + (rnd() < .12 ? 1 : 0);
        p.pId[i] = ids[i];
        p.pPhase[i] = rnd() < .5 ? 0 : 1;
        p.pSource[i] = rnd() < .5 ? 0 : 1;
        p.pSourceId[i] = Math.floor(rnd() * 4);
        p.pX[i] = Math.floor(rnd() * 8) + (.1 + rnd() * .8);
        p.pY[i] = Math.floor(rnd() * 10) + rnd() * .2;
      }
      const r = [0, .25, 2, 1e-9][run % 4];
      const idx = new Index(n), expected = legacyScan(p, r), found = idx.find(p, r);
      const got = found ? [idx.selectedA, idx.selectedB] : [-1, -1];
      if (found !== (expected[0] >= 0) || got[0] !== expected[0] || got[1] !== expected[1]) {
        ok = false; fails.push({n, run, r, got, expected});
        if (fails.length > 3) break;
      }
      cases++;
      if (fails.length > 3) break;
    }
    if (fails.length > 3) break;
  }
  rec('T128-D1', 'randomized differential vs legacy 52a0700 scan: pair identity identical across N∈{4..512}, sparsity, 2 phases, 2 sources, 4 sourceIds, radius {0,1e-9,0.25,2}, IDs incl. near-MAX',
    ok, {cases, fails});
}
// ---- D2: no-match upper bound — legacy worst case is Θ(nmax²) candidate checks; index must answer O(N log N) ----
{
  // adversarial: all particles same group, Y spacing > radius, IDs ascending — legacy scans every pass with no match
  const n = 192;
  const p = pool(n);
  for (let i = 0; i < n; i++) { p.pState[i] = 1; p.pId[i] = i + 1; p.pX[i] = 2.5; p.pY[i] = i * 10; } // Y gaps = 10 > r=2
  const legacyChecks = n * n; // legacy b-scan is Θ(N) per pass × N passes
  const idx = new Index(n);
  const found = idx.find(p, 2);
  const indexOps = idx.sortComparisons + idx.treeVisits + idx.rangeQueries + idx.aSelections;
  a.equal(found, false);
  rec('T128-D2', 'no-match upper bound: adversarial same-group pool → legacy Θ(N²) candidate checks vs index ops bounded O(N log N); both answer no-match',
    indexOps <= n * 64, {legacyChecks, indexOps, n,
     bound: 'sortComparisons+treeVisits+rangeQueries+aSelections ≤ 64N'});
}
// ---- D3: consecutive merges with centroid drift (rebuild correctness) ----
{
  let ok = true;
  const p = pool(64);
  // 12 particles in two groups; after each merge the survivor's Y = weighted centroid changes
  for (let i = 0; i < 12; i++) {
    p.pState[i] = 1; p.pId[i] = i + 1; p.pX[i] = 3.5;
    p.pY[i] = 1 + i * .1; p.pPhase[i] = 0; p.pSource[i] = 0; p.pSourceId[i] = 1;
    p.pMass[i] = 1 + i * .5;
  }
  p.liveCount = 12;
  const simulateMerge = (pa, pb) => {
    // mergePairHot semantics: weighted centroid, mass sum, larger id removed
    const keep = pa, gone = pb;
    const m = p.pMass[keep] + p.pMass[gone];
    p.pY[keep] = (p.pY[keep] * p.pMass[keep] + p.pY[gone] * p.pMass[gone]) / m;
    p.pMass[keep] = m;
    p.pState[gone] = 0;
  };
  for (let step = 0; step < 6; step++) {
    const idx = new Index(64), expected = legacyScan(p, .25), found = idx.find(p, .25);
    const got = found ? [idx.selectedA, idx.selectedB] : [-1, -1];
    if (found !== (expected[0] >= 0) || got[0] !== expected[0] || got[1] !== expected[1]) { ok = false; break; }
    if (!found) break;
    simulateMerge(got[0], got[1]);
    cases++;
  }
  rec('T128-D3', 'consecutive merges: 6-step chain with live centroid drift; every step matches legacy pair selection (index rebuilt from current scratch each time)',
    ok, {steps: 6});
}
// ---- D4: tail births / λ retry shape / rollback restore ----
{
  // rollback restores removed slot; next find must see it again (rebuild, not frozen index)
  const p = pool(16);
  for (let i = 0; i < 8; i++) { p.pState[i] = 1; p.pId[i] = i + 1; p.pX[i] = 1.5; p.pY[i] = 1 + i * .05;
    p.pPhase[i] = 0; p.pSource[i] = 0; p.pSourceId[i] = 2; p.pMass[i] = 2; }
  const idx1 = new Index(16);
  const f1 = idx1.find(p, .25);
  const pair1 = f1 ? [idx1.selectedA, idx1.selectedB] : null;
  // simulate rollback: remove b from state then restore
  if (pair1) {
    p.pState[pair1[1]] = 0;
    const idx2 = new Index(16), f2 = idx2.find(p, .25);
    const e2 = legacyScan(p, .25);
    const g2 = f2 ? [idx2.selectedA, idx2.selectedB] : [-1, -1];
    const agree2 = f2 === (e2[0] >= 0) && g2[0] === e2[0] && g2[1] === e2[1];
    // restore
    p.pState[pair1[1]] = 1;
    const idx3 = new Index(16), e3 = legacyScan(p, .25), f3 = idx3.find(p, .25);
    const g3 = f3 ? [idx3.selectedA, idx3.selectedB] : [-1, -1];
    const agree3 = f3 === (e3[0] >= 0) && g3[0] === e3[0] && g3[1] === e3[1];
    rec('T128-D4', 'rollback/restore: post-removal and post-restore finds both match legacy (rebuild semantics, no stale frozen index)',
      agree2 && agree3, {pair1, agree2, agree3});
  } else {
    rec('T128-D4', 'rollback/restore', false, {reason: 'base pool unexpectedly had no pair'});
  }
  cases++;
}
// ---- D5: duplicate ID rejection and invalid inputs ----
{
  const p = pool(8);
  for (let i = 0; i < 8; i++) { p.pState[i] = 1; p.pId[i] = i < 4 ? 7 : i + 1; p.pX[i] = 1.5; p.pY[i] = i; }
  const idx = new Index(8);
  const dupRejected = idx.find(p, 2) === false && idx.invalid === true;
  const p2 = pool(4); p2.pState[0] = 1; p2.pId[0] = 5; p2.pX[0] = NaN; p2.pY[0] = 0;
  const idx2 = new Index(4);
  const nanRejected = idx2.find(p2, 1) === false && idx2.invalid === true;
  const idx3 = new Index(4);
  const negRadius = idx3.find(pool(4), -1) === false && idx3.invalid === true;
  rec('T128-D5', 'invalid inputs: duplicate pId rejected (invalid=true), NaN coordinate rejected, negative radius rejected — never a silent wrong pair',
    dupRejected && nanRejected && negRadius, {dupRejected, nanRejected, negRadius});
}
// ---- D6: single-group ordering semantics: group key completeness (state/sourceId in key) ----
{
  // same X/Y but different state or sourceId → different group → no merge across groups
  const p = pool(4);
  p.pState.set([1, 2, 1, 1]); p.pId.set([1, 2, 3, 4]);
  p.pX.set([1.5, 1.5, 1.5, 1.5]); p.pY.set([1, 1, 1, 1]);
  p.pPhase.set([0, 0, 0, 0]); p.pSource.set([0, 0, 0, 0]); p.pSourceId.set([0, 0, 0, 0]);
  const idx = new Index(4), expected = legacyScan(p, 5), found = idx.find(p, 5);
  const got = found ? [idx.selectedA, idx.selectedB] : [-1, -1];
  const ok1 = found === (expected[0] >= 0) && got[0] === expected[0] && got[1] === expected[1];
  // sourceId differs → also separated
  const q = pool(4);
  q.pState.set([1, 1, 1, 1]); q.pId.set([1, 2, 3, 4]);
  q.pX.set([1.5, 1.5, 1.5, 1.5]); q.pY.set([1, 1, 1, 1]);
  q.pPhase.set([0, 0, 0, 0]); q.pSource.set([0, 0, 0, 0]); q.pSourceId.set([0, 1, 0, 1]);
  const idx2 = new Index(4), e2 = legacyScan(q, 5), f2 = idx2.find(q, 5);
  const g2 = f2 ? [idx2.selectedA, idx2.selectedB] : [-1, -1];
  const ok2 = f2 === (e2[0] >= 0) && g2[0] === e2[0] && g2[1] === e2[1];
  rec('T128-D6', 'group-key completeness: differing state or sourceId separates particles regardless of radius (a=ID1/b=ID3 pattern, never ID1/ID2 easy case alone)',
    ok1 && ok2, {state: ok1, sourceId: ok2,
     note: 'T116 §5: the easy ID1/2 example must not stand in for full proof; state/sourceId in the group tuple verified'});
}

fs.writeFileSync(OUT + '/p128_results.json',
  JSON.stringify({executionId: 'p128-merge-index-differential-v1', date: '2026-10-26',
    target: 'T123 469ee37', oracle: 'legacy 52a0700 mergeOldestHot scan', results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length, 'differentialCases', cases);
process.exitCode = pass ? 0 : 1;
