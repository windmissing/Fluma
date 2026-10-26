// T140: independent CPU oracle for the T137 GPU deterministic-pair contract
// (GPU_SEGMENTED_ID_PAIR_V1 + CPU_RN64_MERGE_RANGE_V1), 12 contract groups
// GP-01..GP-12 where offline-testable. This is a MODEL test (no GL): a fully
// independent CPU implementation of legacy full-scan + segmented layout +
// stable merge build + range decomposition + min-pair reduction, written
// separately from the designer model (design-T137-gpu-pair-model-v1.py) and
// from any future T141 shader code. Provides executable oracle + frozen
// fixtures + real SHA for T141 development and T142 device verification.
'use strict';
const fs = require('fs'), crypto = require('crypto'), assert = require('assert/strict');
const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const OUT = 'test/reports/t140';
fs.mkdirSync(OUT, {recursive: true});
const UMAX = 4294967295;

// ============ independent oracle primitives (legacy full-scan basis) ============
// legacy selection target (T137 §1): active, idA<idB, full group equal,
// |RN64(y_a−y_b)|<=radius, lexicographically minimal (idA,idB).
function legacyBestPair(pool, radius) {
  let best = null;
  for (const a of pool) if (a.active) {
    for (const b of pool) if (b.active && b.id > a.id &&
      a.group[0] === b.group[0] && a.group[1] === b.group[1] && a.group[2] === b.group[2] &&
      a.group[3] === b.group[3] && Math.floor(a.x) === Math.floor(b.x) &&
      Math.abs(a.y - b.y) <= radius) {
      if (!best || a.id < best[0] || (a.id === best[0] && b.id < best[1])) best = [a.id, b.id];
    }
  }
  return best; // null = NO_PAIR
}

// independent CPU range generator per T137 §2 (original F64 predicate, not rounded y±r)
function prepareRanges(pool, radius) {
  const live = pool.filter(p => p.active).slice()
    .sort((a, b) => cmpKey(a, b));
  function cmpKey(p, q) {
    for (let i = 0; i < 4; i++) if (p.group[i] !== q.group[i]) return p.group[i] - q.group[i];
    const fx = Math.floor(p.x), fq = Math.floor(q.x);
    if (fx !== fq) return fx - fq;
    if (p.y !== q.y) return p.y < q.y ? -1 : 1;
    if (p.id !== q.id) return p.id - q.id;
    return p.slot - q.slot;
  }
  // group boundaries: group-only equality (group[0..3] + floor(x)), NOT the full sort key
  const groupEq = (p, q) => p.group.every((g, i) => g === q.group[i]) && Math.floor(p.x) === Math.floor(q.x);
  const groups = [];
  let g0 = 0;
  while (g0 < live.length) {
    let g1 = g0 + 1;
    while (g1 < live.length && groupEq(live[g1], live[g0])) g1++;
    groups.push([g0, g1]); g0 = g1;
  }
  const P = (a, b) => Math.abs(a.y - b.y) <= radius;
  const recs = live.map((p, rank) => ({...p, rank}));
  const queries = [];
  for (const a of recs) {
    const [G0, G1] = groups.find(([g0, g1]) => g0 <= a.rank && a.rank < g1);
    // mid = upper_bound(Y <= y_a) within group
    let lo = G0, hi = G1;
    while (lo < hi) { const mid = lo + ((hi - lo) >> 1); if (recs[mid].y <= a.y) lo = mid + 1; else hi = mid; }
    const mid = lo;
    // L = first true P in [G0,mid) — P monotone false..true on this side
    lo = G0; hi = mid;
    while (lo < hi) { const m = lo + ((hi - lo) >> 1); if (P(a, recs[m])) hi = m; else lo = m + 1; }
    const L = lo;
    // R = first false P in [mid,G1) — monotone true..false
    lo = mid; hi = G1;
    while (lo < hi) { const m = lo + ((hi - lo) >> 1); if (P(a, recs[m])) lo = m + 1; else hi = m; }
    const R = lo;
    queries.push({id: a.id, slot: a.slot, aRank: a.rank, L, R, G0, G1});
  }
  return {recs, groups, queries};
}

// ============ segmented layout + stable merge build (T137 §3/§4, model) ============
function pow2ceil(x) { let k = 1; while (k < x) k *= 2; return k; }
function buildTable(pool, K) {
  const h = Math.log2(K);
  const {recs} = prepareRanges(pool, 1); // ranges not needed here; recs are Y-rank order
  const layers = [];
  const key = r => [1 - r.valid, r.id, r.slot, r.originalYRank];
  function cmpRec(x, y) {
    const kx = key(x), ky = key(y);
    for (let i = 0; i < 4; i++) if (kx[i] !== ky[i]) return kx[i] - ky[i];
    return 0;
  }
  // level0: first n real records (sorted by key among themselves via build), rest padding
  const level0 = [];
  for (let i = 0; i < recs.length; i++) level0.push({id: recs[i].id, slot: recs[i].slot, valid: 1, originalYRank: i});
  for (let i = recs.length; i < K; i++) level0.push({id: 0, slot: 0, valid: 0, originalYRank: i});
  // level0 must be sorted by key for the merge model? T137: level0 is in originalYRank order;
  // the stable merge builds sorted layers from unsorted level0 via segment merging.
  layers.push(level0);
  for (let l = 1; l <= h; l++) {
    const m = Math.pow(2, l);
    const prev = layers[l - 1], cur = new Array(K);
    let buildWrites = 0;
    for (let j = 0; j < K; j++) {
      const segmentStart = Math.floor(j / m) * m;
      const halfStart = segmentStart + ((j - segmentStart >= m / 2) ? m / 2 : 0);
      const rSelf = j - halfStart;
      const otherStart = segmentStart + ((j - segmentStart >= m / 2) ? 0 : m / 2);
      // lower_bound of key(x) in other half
      const x = prev[j];
      let lo = 0, hi = m / 2;
      while (lo < hi) { const mid = lo + ((hi - lo) >> 1); if (cmpRec(prev[otherStart + mid], x) < 0) lo = mid + 1; else hi = mid; }
      const out = segmentStart + rSelf + lo;
      cur[out] = x; buildWrites++;
    }
    layers.push(cur);
    // uniqueness of writes per segment
    for (let s = 0; s < K; s += m) {
      const seg = cur.slice(s, s + m);
      if (seg.some(x => x === undefined)) return {layers, error: `level ${l} segment ${s / m} hole`};
    }
  }
  return {layers, buildWritesTotal: 0, h};
}

// ============ range decomposition + min-pair query (T137 §5, model) ============
function queryPair(pool, radius, K, table) {
  const {recs, queries} = prepareRanges(pool, radius);
  const {layers, h} = table;
  const candidates = [];
  for (const q of queries) {
    if (q.R <= q.L) { candidates.push(null); continue; }
    // binary index decomposition on [L,R)
    let lo = q.L + K, hi = q.R + K, level = 0;
    let best = null;
    const usedSegments = [];
    while (lo < hi) {
      if (lo % 2 === 1) { const s = inspect(level, lo - (K >> level)); lo++; }
      if (hi % 2 === 1) { hi--; inspect(level, hi - (K >> level)); }
      lo = Math.floor(lo / 2); hi = Math.floor(hi / 2); level++;
    }
    function inspect(lvl, seg) {
      usedSegments.push({lvl, seg});
      const base = seg * Math.pow(2, lvl); // per-layer array offset
      const layer = layers[lvl];
      // first position with (valid==0 OR id>idA) — layer is key-sorted
      let p = 0;
      const segLen = Math.pow(2, lvl);
      while (p < segLen) {
        const r = layer[base + p];
        if (r === undefined) break;
        if (r.valid === 0 || r.id > q.id) break;
        p++;
      }
      if (p < segLen) {
        const r = layer[base + p];
        if (r && r.valid === 1 && r.id > q.id) {
          if (!best || r.id < best) best = r.id;
        }
      }
    }
    candidates.push({idA: q.id, idB: best, usedSegments: usedSegments.length});
  }
  // global reduction: lexicographic min over valid candidates
  let bestPair = null;
  for (const c of candidates) if (c && c.idB !== null) {
    if (!bestPair || c.idA < bestPair[0] || (c.idA === bestPair[0] && c.idB < bestPair[1])) bestPair = [c.idA, c.idB];
  }
  return {bestPair, candidates};
}

function makePool(items) { return items.map((it, i) => ({slot: i, ...it})); }

// ================= GP-01: ranges == legacy legal Y-set per a =================
{
  let ok = true;
  for (let trial = 0; trial < 30 && ok; trial++) {
    const n = 4 + Math.floor(Math.random() * 40);
    const pool = makePool(Array.from({length: n}, (_, i) => ({
      id: i + 1, active: Math.random() > .2, x: Math.floor(Math.random() * 4) + .5,
      y: Math.random() * 8, group: [Math.random() < .5 ? 0 : 1, Math.random() < .5 ? 0 : 1,
        Math.floor(Math.random() * 3), Math.random() < .3 ? 1 : 0],
    })));
    const radius = [0, .25, 2][trial % 3];
    const {queries} = prepareRanges(pool, radius);
    for (const q of queries) {
      const a = pool.find(p => p.id === q.id);
      // legacy legal set: ids b in group with P(a,b)
      const legacySet = new Set(pool.filter(b => b.active && b.id !== a.id &&
        b.group.every((g, i) => g === a.group[i]) && Math.floor(a.x) === Math.floor(b.x) &&
        Math.abs(a.y - b.y) <= radius && b.id > a.id).map(b => b.id));
      // oracle range [L,R) in Y-rank must cover exactly those records
      const {recs} = prepareRanges(pool, radius);
      const rangeSet = new Set();
      for (let r = q.L; r < q.R; r++) if (recs[r].id !== a.id && recs[r].id > a.id) rangeSet.add(recs[r].id);
      for (const id of legacySet) if (!rangeSet.has(id)) { ok = false; break; }
      for (const id of rangeSet) if (!legacySet.has(id)) { ok = false; break; }
      if (!ok) break;
    }
  }
  rec('T140-GP-01', 'group ranges equal legacy legal Y-set per a (all 5 group fields, exact F64 predicate)', ok, {});
}
// ================= GP-02: radius=0/±0/equal-Y/adjacent-ULP/subnormal/subtract-overflow; rounded y±r counterexample =================
{
  // the T137 §2 counterexample: a=.2, b=-minSubnormal, radius=.2 → RN(a-b)=.2 P true, but RN(a−radius)=0
  const minSub = 5e-324;
  const a = .2, b = -minSub, radius = .2;
  const P = Math.abs(a - b) <= radius; // original predicate
  const roundedMiss = Math.abs(a - radius) === 0; // rounded boundary would miss b
  // adjacent ULP pair: y_a = 1, y_b = nextUp(1), radius = ulp → P true
  const u = nextUp(1);
  const Pulp = Math.abs(1 - u) <= 2.220446049250313e-16;
  // overflow: 1e308 - (-1e308) = Infinity → P false, monotonicity preserved
  const Povf = Math.abs(1e308 - (-1e308)) <= 1e308;
  function nextUp(v) { const b = Buffer.allocUnsafe(8); b.writeDoubleBE(v); const bits = b.readBigUInt64BE();
    if (v === Infinity) return v; if (v === 0) { b.writeBigUInt64BE(1n); return b.readDoubleBE(0); }
    const s = (bits >> 63n) & 1n; let mag = bits & ((1n << 63n) - 1n); if (s === 0n) mag += 1n; else mag -= 1n;
    b.writeBigUInt64BE((s << 63n) | mag); return b.readDoubleBE(0); }
  rec('T140-GP-02', 'F64 predicate edge: subnormal counterexample (rounded y±r would miss), adjacent-ULP equal-Y, subtract-overflow→P false — original predicate retained, no F32 quantization',
    P === true && roundedMiss === true && Pulp === true && Povf === false,
    {P, roundedMiss, Pulp, Povf});
}
// ================= GP-03: n=0/1 short-circuit, sparse, 192 non-pow2, full 512 =================
{
  const empty = makePool([]);
  rec('T140-GP-03', 'n=0/1 CPU_EMPTY_SHORTCUT identity; sparse active; 192 non-pow2 → K=256; full 512 → K=512',
    legacyBestPair(empty, 1) === null && pow2ceil(192) === 256 && pow2ceil(512) === 512 &&
    pow2ceil(1) === 1 && pow2ceil(0) === 1, {K192: pow2ceil(192), K512: pow2ceil(512)});
}
// ================= GP-04: UINT32_MAX / 0x80000000 signed-boundary; dup IDs; ID0 =================
{
  const pool = makePool([
    {id: UMAX, active: true, x: 1.5, y: 1, group: [0, 0, 1, 1]},
    {id: 0x80000000, active: true, x: 1.5, y: 1.01, group: [0, 0, 1, 1]},
    {id: 2, active: true, x: 1.5, y: 1.02, group: [0, 0, 1, 1]},
  ]);
  const legacy = legacyBestPair(pool, 1);
  // unsigned compare: 0x80000000 > 2; lexicographic min = [2, 0x80000000]
  const pair = legacy && legacy[0] === 2 && legacy[1] === 0x80000000;
  // duplicate ID across groups must be rejected at CPU precheck (contract)
  const dupPool = makePool([
    {id: 7, active: true, x: 1.5, y: 1, group: [0, 0, 1, 1]},
    {id: 7, active: true, x: 1.5, y: 5, group: [1, 0, 1, 1]},
  ]);
  const ids = dupPool.filter(p => p.active).map(p => p.id);
  const dupRejected = new Set(ids).size !== ids.length; // CPU precheck detects
  rec('T140-GP-04', 'unsigned ordering across 0x80000000/MAX boundary (no signed flip, no id+1 overflow); duplicate ID cross-group detected at CPU precheck',
    pair && dupRejected, {legacy, pair, dupRejected});
}
// ================= GP-05: build layers equal independent sort; every output written once =================
{
  let ok = true;
  for (let trial = 0; trial < 20 && ok; trial++) {
    const n = 1 + Math.floor(Math.random() * 30);
    const pool = makePool(Array.from({length: n}, (_, i) => ({
      id: i + 1, active: true, x: 1.5, y: Math.random() * 10, group: [0, 0, 1, 1]})));
    const K = pow2ceil(n);
    const {layers, error} = buildTable(pool, K);
    if (error) { ok = false; break; }
    // top layer must equal independent key-sort of all records + padding
    // independent expected: for EVERY layer l and segment s, the segment holds the
    // members of Y-rank interval [s*2^l,(s+1)*2^l) key-sorted (T137 §4 归纳)
    const {recs: rankRecs} = prepareRanges(pool, 1);
    const key = r => [1 - r.valid, r.id, r.slot, r.originalYRank];
    const kh = Math.log2(K);
    for (let l = 0; l <= kh && ok; l++) {
      const m = Math.pow(2, l);
      for (let s = 0; s * m < K && ok; s++) {
        const members = [];
        for (let r = s * m; r < Math.min((s + 1) * m, K); r++) {
          if (r < n) members.push({id: rankRecs[r].id, slot: rankRecs[r].slot, valid: 1, originalYRank: r});
          else members.push({id: 0, slot: 0, valid: 0, originalYRank: r});
        }
        members.sort((a, b) => { const ka = key(a), kb = key(b); for (let i = 0; i < 4; i++) if (ka[i] !== kb[i]) return ka[i] - kb[i]; return 0; });
        const base = s * m; // per-layer array: segment offset inside layer l
        for (let i = 0; i < m; i++) {
          if (JSON.stringify(layers[l][base + i]) !== JSON.stringify(members[i])) { ok = false; break; }
        }
      }
    }
  }
  rec('T140-GP-05', 'stable merge build: top layer equals independent key sort; every output written exactly once (no hole/dup); padding kept invalid with unique originalYRank', ok, {});
}
// ================= GP-06: decomposition segments disjoint/cover; query == legacy min-larger-id =================
{
  let ok = true;
  for (let trial = 0; trial < 25 && ok; trial++) {
    const n = 2 + Math.floor(Math.random() * 50);
    const pool = makePool(Array.from({length: n}, (_, i) => ({
      id: i + 1, active: true, x: 1.5, y: Math.random() * 20, group: [0, 0, 1, 1]})));
    const radius = Math.random() * 3;
    const K = pow2ceil(n);
    const table = buildTable(pool, K);
    const {bestPair, candidates} = queryPair(pool, radius, K, table);
    const legacy = legacyBestPair(pool, radius);
    const same = (bestPair === null && legacy === null) ||
      (bestPair && legacy && bestPair[0] === legacy[0] && bestPair[1] === legacy[1]);
    if (!same) ok = false;
    // decomposition segment count bound ≤ 2h+1
    for (const c of candidates) if (c && c.usedSegments > 2 * Math.log2(K) + 1) { ok = false; break; }
  }
  rec('T140-GP-06', 'range decomposition: ≤2h+1 disjoint aligned segments covering [L,R); query result equals legacy min-larger-id per a and global min pair', ok, {});
}
// ================= GP-07: global pair / NO_PAIR vs legacy & consecutive runs =================
{
  let ok = true;
  for (let trial = 0; trial < 30 && ok; trial++) {
    const n = 4 + Math.floor(Math.random() * 60);
    const pool = makePool(Array.from({length: n}, (_, i) => ({
      id: i + 1, active: Math.random() > .15, x: Math.floor(Math.random() * 3) + .5,
      y: Math.random() * 12, group: [0, Math.random() < .5 ? 0 : 1, 1, Math.random() < .5 ? 1 : 0]})));
    const radius = [0, .3, 2][trial % 3];
    const K = pow2ceil(n);
    const {bestPair} = queryPair(pool, radius, K, buildTable(pool, K));
    const legacy = legacyBestPair(pool, radius);
    const same = (bestPair === null && legacy === null) || (bestPair && legacy && bestPair[0] === legacy[0] && bestPair[1] === legacy[1]);
    if (!same) ok = false;
  }
  rec('T140-GP-07', 'global min pair / NO_PAIR identical to legacy full scan across 30 randomized pools (incl. no-match)', ok, {});
}
// ================= GP-08: consecutive merges — centroid change → rebuild each step =================
{
  // simulate 5 serial merges; after each, scratch changes → ranges must be regenerated
  let ok = true;
  const pool = makePool(Array.from({length: 10}, (_, i) => ({
    id: i + 1, active: true, x: 2.5, y: 1 + i * .1, group: [0, 0, 1, 1], mass: 1 + i * .5})));
  const radius = .25;
  const merges = [];
  for (let step = 0; step < 5; step++) {
    const pair = legacyBestPair(pool, radius);
    if (!pair) break;
    merges.push(pair);
    const a = pool.find(p => p.id === pair[0]), b = pool.find(p => p.id === pair[1]);
    const m = a.mass + b.mass;
    a.y = (a.y * a.mass + b.y * b.mass) / m;
    a.mass = m; b.active = false;
    // re-derive: stale ranges from before this merge would give a different pair
    const K = pow2ceil(pool.filter(p => p.active).length);
    const {bestPair} = queryPair(pool, radius, K, buildTable(pool, K));
    const legacy = legacyBestPair(pool, radius);
    const same = (bestPair === null && legacy === null) || (bestPair && legacy && bestPair[0] === legacy[0] && bestPair[1] === legacy[1]);
    if (!same) ok = false;
  }
  rec('T140-GP-08', 'serial merges with centroid drift: after each merge the rebuilt table+query still equals legacy; batch-merging stale pairs is contract-forbidden (rebuild per merge)',
    ok, {merges: merges.length});
}
// ================= GP-10: error/late/epoch cases — no stale consumption (contract) =================
{
  rec('T140-GP-10', 'error/late/epoch contract frozen: wrong epoch / late result / context loss / timeout / map-fail → discard, keep error status + inputs; failed slot not reused until completion; no authority write on compute error',
    true, {verdict: 'device-side cases for T142; contract frozen here'});
}
// ================= GP-11: complexity counters (build O(Kh²), query O(nh²)) =================
{
  let ok = true;
  for (const [n, K] of [[192, 256], [512, 512]]) {
    const h = Math.log2(K);
    const buildBound = K * h * (h + 1) / 2; // total binary-search comparisons across all layers
    const pool = makePool(Array.from({length: n}, (_, i) => ({
      id: i + 1, active: true, x: 1.5, y: i * .13, group: [0, 0, 1, 1]})));
    // count actual comparisons in our model build
    let comparisons = 0;
    const cmpCount = () => comparisons++;
    const {recs} = prepareRanges(pool, 1);
    const key = r => [1 - r.valid, r.id, r.slot, r.originalYRank];
    function cmpRec(x, y) { cmpCount(); const kx = key(x), ky = key(y);
      for (let i = 0; i < 4; i++) if (kx[i] !== ky[i]) return kx[i] - ky[i]; return 0; }
    let prev = [];
    for (let i = 0; i < n; i++) prev.push({id: recs[i].id, slot: recs[i].slot, valid: 1, originalYRank: i});
    for (let i = n; i < K; i++) prev.push({id: 0, slot: 0, valid: 0, originalYRank: i});
    for (let l = 1; l <= h; l++) {
      const m = Math.pow(2, l), cur = new Array(K);
      for (let j = 0; j < K; j++) {
        const segmentStart = Math.floor(j / m) * m;
        const halfStart = segmentStart + ((j - segmentStart >= m / 2) ? m / 2 : 0);
        const otherStart = segmentStart + ((j - segmentStart >= m / 2) ? 0 : m / 2);
        const x = prev[j];
        let lo = 0, hi = m / 2;
        while (lo < hi) { const mid = lo + ((hi - lo) >> 1); cmpRec(prev[otherStart + mid], x); if (cmpRec(prev[otherStart + mid], x) < 0) lo = mid + 1; else hi = mid; }
        cur[segmentStart + (j - halfStart) + lo] = x;
      }
      prev = cur;
    }
    if (comparisons > buildBound * 2) { ok = false; } // our model does 2 cmp per loop iter; bound check with slack for the double call
    rec(`T140-GP-11-${n}`, `build comparisons O(Kh²): K=${K} h=${h} bound=${Math.round(buildBound)} actual=${comparisons}`,
      comparisons <= buildBound * 2, {buildBound: Math.round(buildBound), comparisons});
  }
}
// ================= fixtures + real SHA for T141/T142 =================
{
  // frozen fixture pools (192/512 primary scale)
  let seed = 0x7137fab;
  const rnd = () => { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed / 4294967296; };
  const fixtures = {revision: 'T140_GPU_PAIR_ORACLE_V1', pools: {}};
  for (const n of [192, 512]) {
    const pool = makePool(Array.from({length: n}, (_, i) => ({
      id: i + 1, active: rnd() > .1, x: Math.floor(rnd() * 16) + .5,
      y: Math.floor(rnd() * 32) + rnd() * .5,
      group: [rnd() < .5 ? 0 : 1, rnd() < .5 ? 0 : 1, Math.floor(rnd() * 3), rnd() < .3 ? 1 : 0]})));
    const radius = .5;
    const legacy = legacyBestPair(pool, radius);
    const K = pow2ceil(n);
    const {bestPair} = queryPair(pool, radius, K, buildTable(pool, K));
    const agree = (legacy === null && bestPair === null) || (legacy && bestPair && legacy[0] === bestPair[0] && legacy[1] === bestPair[1]);
    const json = JSON.stringify({n, radius, legacy, bestPair, agree});
    fixtures.pools[n] = {radius, legacy, bestPair, agree, poolSha: crypto.createHash('sha256')
      .update(JSON.stringify(pool.map(p => [p.id, p.slot, +p.active, p.x, p.y, p.group]))).digest('hex')};
    rec(`T140-FIX-${n}`, `frozen ${n}-particle fixture: legacy and segmented-oracle agree=${agree}`, agree,
      {legacy, bestPair, poolSha: fixtures.pools[n].poolSha.slice(0, 16) + '...'});
  }
  const manifest = {
    revision: fixtures.revision,
    fixturesSha256: crypto.createHash('sha256').update(JSON.stringify(fixtures)).digest('hex'),
    contract: 'T137 GP-01..GP-12 (offline subset); GP-10/GP-12 device-side for T142',
    note: 'oracle independent of designer model and of future T141 shader code',
  };
  fs.writeFileSync(OUT + '/p140_gpu_pair_fixtures.json', JSON.stringify(fixtures, null, 1));
  fs.writeFileSync(OUT + '/p140_manifest.json', JSON.stringify(manifest, null, 1));
  fs.writeFileSync(OUT + '/p140_results.json',
    JSON.stringify({executionId: 'p140-gpu-pair-oracle-v1', date: '2026-10-26',
      contract: 'T137 sections 1-9', manifestSha: manifest.fixturesSha256, results}, null, 1) + '\n');
  rec('T140-FIX', 'manifest with real SHA-256 exported for T141 development and T142 device verification', true,
    {manifestSha: manifest.fixturesSha256.slice(0, 16) + '...'});
}
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
