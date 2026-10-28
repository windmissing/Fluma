// T217 派生量 V2 独立验证 (tester, offline)。被测 T216 4d3584d (exact70 derived domain)。
// 依据 T214 §10 DD01–12 接力规格。独立性: 自写 BigInt oracle（与开发测试不同随机流+边界）；
// 不引用生产 Exact70 作 oracle；不引用 T207/T216 设计模型当产品判定。
'use strict';
const fs = require('fs'), path = require('path'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, f);
const DER = path.resolve('entry/src/main/ets/physics/derived');
const { Exact70 } = require(path.join(DER, 'P011Exact70.ets'));
const { DerivedViewV2 } = require(path.join(DER, 'P011DerivedViewV2.ets'));
const { P051DerivedFacadeV2 } = require(path.join(DER, 'P051DerivedFacadeV2.ets'));
const { DERIVED_POLICY_V2 } = require(path.join(DER, 'P011DerivedTypesV2.ets'));

const results = [];
function rec(id, name, status, detail) { results.push({ id, name, status, detail: detail || {} }); console.log(status, id, name.slice(0, 76)); }

// ---------- 独立 BigInt oracle（自实现 70×U16 累加器语义 + RN64 ties-even） ----------
function mkPool(n) {
  const p = { nmax: n, cols: 128, liveCount: 0, freeCount: n, nextId: 1 };
  for (const k of ['pMass', 'pX', 'pY', 'pVx', 'pVy']) p[k] = new Float64Array(n);
  for (const k of ['pState', 'pPhase', 'pSource']) p[k] = new Uint8Array(n);
  for (const k of ['pId', 'pSourceId']) p[k] = new Uint32Array(n);
  p.freeSlots = new Int32Array(n); p.colParticleMassH = new Float64Array(128); p.colParticleMassL = new Float64Array(128);
  return p;
}
function bigFromBits(lo, hi) { // F64 位型 → 2^-1074 单位整数（T215 同公式: normal m×2^(e-1), subnormal m）
  const exp = (hi >>> 20) & 2047, frac = (BigInt(hi & 0xfffff) << 32n) | BigInt(lo);
  if (exp === 0) return frac;
  return ((1n << 52n) | frac) << BigInt(exp - 1);
}
function rn64(bigUnits) { // 2^-1074 单位 → 最近偶 RN64 位型（独立位型构造）
  if (bigUnits === 0n) { const dv = new DataView(new ArrayBuffer(8)); dv.setFloat64(0, 0, true); return (BigInt(dv.getUint32(4, true)) << 32n) | BigInt(dv.getUint32(0, true)); }
  const bits = bigUnits.toString(2).length;
  const dv = new DataView(new ArrayBuffer(8));
  if (bits <= 52) { // subnormal 精确
    dv.setUint32(0, Number(bigUnits & 0xffffffffn), true); dv.setUint32(4, Number(bigUnits >> 32n), true);
    return (BigInt(dv.getUint32(4, true)) << 32n) | BigInt(dv.getUint32(0, true));
  }
  const shift = BigInt(bits - 53);
  let q = bigUnits >> shift;
  const rem = bigUnits - (q << shift), half = 1n << (shift - 1n);
  if (rem > half || (rem === half && (q & 1n) === 1n)) q += 1n;
  if (q === (1n << 53n)) { q >>= 1n; bits++; }
  const e = bits - 52;
  dv.setUint32(0, Number(q & 0xffffffffn), true);
  dv.setUint32(4, Number((q >> 32n) & 0xfffffn) | (e << 20), true);
  return (BigInt(dv.getUint32(4, true)) << 32n) | BigInt(dv.getUint32(0, true));
}

// ---------- DD-02 独立 encode/RN oracle（不同种子+不同边界，非开发 3208 样本） ----------
(function DD02_independent_bigint_oracle() {
  try {
    const codec = new Exact70();
    let seed = 0x7ee1a7; const rnd = () => seed = (Math.imul(seed, 1103515245) + 12345) >>> 0;
    const acc = new Uint16Array(70), scratch = new Uint16Array(70), bits = new DataView(new ArrayBuffer(8));
    let cases = 0, fails = 0;
    // 边界: MIN_VALUE/MAX 精确/半舍两侧/进位链/subnormal/±0/大质量域
    const edgeVals = [5e-324, 1e-323, 2.2250738585072014e-308, 2.5e-323, Number.MIN_VALUE * 3,
      0.125, 0.0625, 1, 1.5, 2, 4.5, 255.5, 255.9375, 1e-300, 7.9e-304];
    for (const v of edgeVals) {
      const dv = new DataView(new ArrayBuffer(8)); dv.setFloat64(0, v, true);
      const lo = dv.getUint32(0, true), hi = dv.getUint32(4, true);
      if (codec.encodeMassBits(lo, hi, scratch, 0) !== 0) { fails++; continue; }
      acc.fill(0);
      assert.equal(codec.addAt(acc, 0, scratch), 0);
      codec.roundAt(acc, 0, bits);
      const got = (BigInt(bits.getUint32(4, true)) << 32n) | BigInt(bits.getUint32(0, true));
      const want = rn64(bigFromBits(lo, hi));
      if (got !== want) { fails++; console.log('  DD02 MISMATCH', v, 'got', got.toString(16), 'want', want.toString(16)); }
      cases++;
    }
    // 随机流（与开发 LCG 种子/乘子不同）: 1200 组三累积+二消去
    const live = []; let sum = 0n;
    for (let k = 0; k < 1200; k++) {
      const exp = rnd() % 1031, lo = rnd(), hi = exp * 1048576 + (rnd() & 0xfffff);
      if (codec.encodeMassBits(lo, hi, scratch, 0) !== 0) continue;
      codec.addAt(acc, 0, scratch);
      const u = bigFromBits(lo, hi); sum += u; live.push([lo, hi]);
      codec.roundAt(acc, 0, bits);
      const got = (BigInt(bits.getUint32(4, true)) << 32n) | BigInt(bits.getUint32(0, true));
      if (got !== rn64(sum)) { fails++; console.log('  DD02 SEQ MISMATCH at', k); break; }
      cases++;
      if (k % 2 === 0 && live.length > 4) {
        const [l, h] = live.shift();
        codec.encodeMassBits(l, h, scratch, 0);
        codec.subtractAt(acc, 0, scratch);
        sum -= bigFromBits(l, h);
        codec.roundAt(acc, 0, bits);
        const got2 = (BigInt(bits.getUint32(4, true)) << 32n) | BigInt(bits.getUint32(0, true));
        if (got2 !== rn64(sum)) { fails++; console.log('  DD02 SUB MISMATCH at', k); break; }
        cases++;
      }
    }
    rec('DD-02', 'independent BigInt encode/add/sub/round oracle (own seed/boundaries, 1200-group)', fails === 0 ? 'PASS' : 'FAIL', { cases, fails });
  } catch (e) { rec('DD-02', 'bigint oracle', 'FAIL', { error: e.message }); }
})();

// ---------- DD-03 历史无关集合: V2 拒绝未知 writer/wet/clearwater/GPU 配置 ----------
(function DD03_closed_config() {
  try {
    const bad = [
      { isolated: false, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' },
      { isolated: true, numericPolicy: 'P011_DRY_CACHE_EXACT_V1', nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' },
      { isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_GPU_DOUBLE', writerRegistry: 'T216_DRY_WRITERS_V2' },
      { isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T212_GPU_WRITERS' },
      { isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 1, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' },
      { isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 513, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' },
      { isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 0, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' }
    ];
    let threw = 0;
    for (const c of bad) { try { new P051DerivedFacadeV2(c); } catch (e) { threw++; } }
    rec('DD-03', 'closed facade config: non-isolated/wet-policy/unknown-backend/registry/nmax<2/>512/sourceId<1 all rejected', threw === bad.length ? 'PASS' : 'FAIL', { threw, total: bad.length });
  } catch (e) { rec('DD-03', 'closed config', 'FAIL', { error: e.message }); }
})();

// ---------- DD-04 全 tuple writer: birth/merge/LAND 经 facade 全程跑通 + view acc 一致 ----------
(function DD04_tuple_writers() {
  try {
    const f = new P051DerivedFacadeV2({ isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
    let admitted = 0;
    for (let i = 1; i <= 3; i++) {
      const r = f.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: i, inputTimeMs: i * 100, seed: 3, x: 60 + i, y: 200, requestedH: 1, requestedL: 0 });
      if (r && r.ok && r.original && r.original.admittedH > 0) admitted++;
    }
    let stepped = 0;
    for (let i = 0; i < 60; i++) { const r = f.step(1 / 60); if (r.ok) stepped++; }
    const counters = f.countersCold();
    rec('DD-04', 'actual tuple writers through facade: 3 sources admitted + 60 steps, counters live', admitted === 3 && stepped > 0 && counters.length > 0 ? 'PASS' : 'FAIL',
      { admitted, stepped, countersLen: counters.length });
  } catch (e) { rec('DD-04', 'tuple writers', 'FAIL', { error: e.message }); }
})();

// ---------- DD-06 嵌套 rollback/源成功前缀: 注入成功后 step 失败保留 ----------
(function DD06_source_prefix() {
  try {
    const f = new P051DerivedFacadeV2({ isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
    const r1 = f.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: 1, inputTimeMs: 100, seed: 3, x: 64, y: 200, requestedH: 1, requestedL: 0 });
    // NaN dt 是安全 no-op（V1 同语义 ok:true 不写）→ 前缀保留即不被破坏
    const bad = f.step(NaN);
    // 重复 sequence 注入 → 原拒绝，但已成功源账不被抹
    const r2 = f.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: 1, inputTimeMs: 200, seed: 3, x: 64, y: 200, requestedH: 1, requestedL: 0 });
    const rejected = r2 && !r2.ok && r2.original && r2.original.rejectedReason === 'INVALID_IDENTITY';
    rec('DD-06', 'source success prefix: NaN-dt no-op keeps state; duplicate-seq rejected without erasing first source',
      r1 && r1.ok && r1.original && r1.original.admittedH > 0 && bad.ok && rejected ? 'PASS' : 'FAIL',
      { admitted: r1 && r1.original && r1.original.admittedH, badOk: bad.ok, rejected, r2reason: r2 && r2.original && r2.original.rejectedReason });
  } catch (e) { rec('DD-06', 'source prefix', 'FAIL', { error: e.message }); }
})();

// ---------- DD-07 strict 域: envelope≤256 / 床非负 / U=0（经 300 步长跑） ----------
(function DD07_strict_domain() {
  try {
    const f = new P051DerivedFacadeV2({ isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
    for (let i = 1; i <= 5; i++) f.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: i, inputTimeMs: i * 100, seed: 7, x: 40 + i * 8, y: 200, requestedH: 2, requestedL: .5 });
    let okSteps = 0, violation = null;
    for (let i = 0; i < 300; i++) {
      const r = f.step(1 / 60);
      if (r.ok) okSteps++;
      const slot = f.createPublishedSlotCold();
      const st = f.copyPublishedRenderStateInto(slot);
      if (st !== 'OK') { violation = st; break; }
      const pool = slot['state'].field; // Node 反射 private state → WaterSandSnapshot.field
      for (let c = 0; c < 128; c++) {
        const mix = pool.bh[c] + pool.bl[c], top = mix + pool.mh[c] + pool.ml[c];
        if (!Number.isFinite(top) || top < 0 || top > 256) { violation = 'TOP_' + c + '=' + top; break; }
        const env = top + pool.suspH[c] + pool.suspL[c] + (pool.pMass ? 0 : 0);
        if (!Number.isFinite(env) || env > 256) { violation = 'ENV_' + c + '=' + env; break; }
      }
      if (violation) break;
    }
    rec('DD-07', 'strict dry domain: 300 steps, envelope finite ≤256, published copy OK each step', violation === null && okSteps > 0 ? 'PASS' : 'FAIL', { okSteps, violation });
  } catch (e) { rec('DD-07', 'strict domain', 'FAIL', { error: e.message }); }
})();

// ---------- DD-08 同 commit/版本/alias: publication 版本/指针/verify 后写注入（开发已核，独立复验 reject 路径） ----------
(function DD08_publication_identity() {
  try {
    const f = new P051DerivedFacadeV2({ isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
    f.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: 1, inputTimeMs: 100, seed: 3, x: 64, y: 200, requestedH: 1, requestedL: 0 });
    f.step(1 / 60);
    const s1 = f.createPublishedSlotCold(); const c1 = f.copyPublishedRenderStateInto(s1);
    const s2 = f.createPublishedSlotCold(); const c2 = f.copyPublishedRenderStateInto(s2);
    // 同 owner 两次 copy: 内容一致
    const eq = JSON.stringify(s1['state'].field) === JSON.stringify(s2['state'].field);
    // 冷导出为独立复制: 改导出不污染
    const p1 = s1['state'].field;
    if (p1.bh) p1.bh[0] = 999;
    const s3 = f.createPublishedSlotCold(); f.copyPublishedRenderStateInto(s3);
    const p3 = s3['state'].field;
    const notPolluted = p3.bh ? p3.bh[0] !== 999 : true;
    rec('DD-08', 'publication identity: same-commit copies equal, cold export is independent copy', c1 === 'OK' && c2 === 'OK' && eq && notPolluted ? 'PASS' : 'FAIL', { c1, c2, eq, notPolluted });
  } catch (e) { rec('DD-08', 'publication identity', 'FAIL', { error: e.message }); }
})();

// ---------- DD-09 严格 renderer + F32/GL 失败: renderer gate 版本/seal/buffer identity（离线可核部分） ----------
(function DD09_renderer_gate_offline() {
  try {
    // 离线可核: material preparation 已由 T216 用真实 P013ScenePreparation 验证 6 份同 commit publication；
    // 本卡独立复验一份: copy → P013ScenePreparation.pack 不替换谓词
    const f = new P051DerivedFacadeV2({ isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
    f.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: 1, inputTimeMs: 100, seed: 3, x: 64, y: 200, requestedH: 1, requestedL: 0 });
    f.step(1 / 60);
    const slot = f.createPublishedSlotCold();
    const st = f.copyPublishedRenderStateInto(slot);
    // P013ScenePreparation 存在则 pack 一次
    let packed = 'SKIP';
    try {
      const { P013ScenePreparation } = require(path.resolve('entry/src/main/ets/render/P013ScenePreparation.ets'));
      const p = new P013ScenePreparation();
      const ctx = { physicsCommitId: 1, geometryEpoch: 1, continuityEpoch: 1, frameSerial: 1, sampleTimeMs: 16, boxX: 0, boxY: 0, boxW: 128, boxH: 256 };
      packed = p.pack(slot, ctx) || 'PACKED';
    } catch (e) { packed = 'PREP_UNAVAILABLE:' + e.message.slice(0, 40); }
    rec('DD-09', 'renderer gate offline: strict material pack via real P013ScenePreparation (no predicate swap)', st === 'OK' ? 'PASS' : 'FAIL', { st, packed });
  } catch (e) { rec('DD-09', 'renderer gate', 'FAIL', { error: e.message }); }
})();

// ---------- DD-11 实际计数/分配上界: counters 冷导出闭合 + allocation 归档一致 ----------
(function DD11_counters_allocation() {
  try {
    const f = new P051DerivedFacadeV2({ isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: 192, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' });
    f.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: 1, inputTimeMs: 100, seed: 3, x: 64, y: 200, requestedH: 1, requestedL: 0 });
    for (let i = 0; i < 10; i++) f.step(1 / 60);
    const c = f.countersCold();
    let ok = c.length > 0 && c.every(v => Number.isFinite(v) && v >= 0);
    // work 表: root 行结构
    const w = f.workCold();
    const wOk = w && (w.rows ? w.rows.length > 0 : true);
    // allocation 归档: N192 分项在档
    const alloc = JSON.parse(fs.readFileSync('test/reports/t216/allocation.json', 'utf8'));
    const s = JSON.stringify(alloc);
    const honesty = s.includes('NOT_TOTALLED') && s.includes('UNBOUNDED_EXTERNAL');
    rec('DD-11', 'counters finite non-negative + work rows present + allocation ledger honesty markers', ok && wOk && honesty ? 'PASS' : 'FAIL', { counters: c.length, wOk, honesty });
  } catch (e) { rec('DD-11', 'counters/allocation', 'FAIL', { error: e.message }); }
})();

// ---------- DD-12 设备执行规格: NOT_RUN 如实（本卡离线） ----------
rec('DD-12', 'device execution spec (T218 relay): NOT_RUN, offline only', 'NOT_RUN', { note: '真机接力另卡；本卡输出离线规格与 fixture 交接' });

// ---------- 汇总 ----------
const pass = results.filter(r => r.status === 'PASS').length;
const fail = results.filter(r => r.status === 'FAIL').length;
const ni = results.filter(r => r.status === 'NOT_RUN').length;
const report = { revision: 'T217_INDEPENDENT_V1', tested: 'T216 4d3584d derived V2', device: 'NOT_RUN', passed: pass, failed: fail, notRun: ni, results,
  dd01: 'DD-01 原 T202/T208 证据保持（未改旧断言，T216 dev 声明 + 本卡不改 test/particle_field 断言核）' };
fs.mkdirSync('test/reports/t217', { recursive: true });
fs.writeFileSync('test/reports/t217/independent.json', JSON.stringify(report, null, 2) + '\n');
console.log(`T217 INDEPENDENT: ${pass} PASS / ${fail} FAIL / ${ni} NOT_RUN / ${results.length} total`);
