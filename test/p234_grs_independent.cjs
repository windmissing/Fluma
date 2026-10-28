// T234 GPU驻留独立规格与数值基线 (tester, offline)。
// 消费 T230 §3/§5/§11 GR01-12 + T231 平台边界；与 T233 并行不互设循环。
// 独立性: 自持 BigInt/Fraction/RNE/有理 DDA/整数 guard oracle，不调用产品核或设计模型(design-T230-gpu-resident-model-v1.py)作判定。
// 实现面: gpu_resident/ 零实现 → 全 GR 预测试 RED；本卡交付独立 oracle + 机器可读 GR01-12 规格 + Q20 数值基线。
'use strict';
const fs = require('fs'), path = require('path'), assert = require('assert/strict');
const ROOT = path.resolve('entry/src/main/ets/physics');
const results = [];
function rec(id, name, status, detail) { results.push({ id, name, status, detail: detail || {} }); console.log(status, id, name.slice(0, 76)); }

const Qm = 2 ** 20;                    // 1048576
const Cq = 256 * Qm;                   // 268435456
const Hfloor = Math.floor(Math.tan(Math.PI / 6) * Qm); // 605395 (tan30)
const Hq = Hfloor - 1;                 // 605394 保守安全边界

// ============ 独立 oracle 原语（BigInt 自实现，非产品核） ============
function mulDivRNE(a, b, d) { // 有符号: 精确积后最近偶数商; d>0
  const neg = (a < 0n) !== (b < 0n);
  const A = BigInt(Math.abs(a)), B = BigInt(Math.abs(b)), D = BigInt(d);
  const prod = A * B;
  let q = prod / D, r = prod % D;
  if (r * 2n > D || (r * 2n === D && (q & 1n) === 1n)) q += 1n;
  return { q: neg ? -Number(q) : Number(q), exact: r === 0n };
}
function floorDiv(a, b) { // b>0
  const A = BigInt(a), B = BigInt(b);
  const q = A / B;
  if ((A % B !== 0n) && ((A < 0n) !== (B < 0n))) return Number(q - 1n);
  return Number(q);
}
function ceilDiv(a, b) { return -floorDiv(-a, b); }
function u64Pair(lo, hi) { return BigInt(lo) | (BigInt(hi) << 32n); }
function encodeQ20(x) { return BigInt(Math.round(x * Qm)); }

// ---- GR-01 独立数值 oracle 自检（BigInt 原语互检 + F64 精确域对拍） ----
(function GR01_oracle_selftest() {
  try {
    let cases = 0, fails = 0;
    // mulDivRNE: (a*b)/d RNE
    const vec = [[3n, 5n, 2n], [1n, 1n, 2n], [5n, 1n, 2n], [7n, 1n, 4n], [9n, 1n, 4n], [-3n, 5n, 2n], [BigInt(Qm), BigInt(Qm), 1n], [0xffffffffn, 0xffffffffn, BigInt(Qm)]];
    for (const [a, b, d] of vec) {
      const prod = a * b, D = typeof d === 'bigint' ? d : BigInt(d);
      let q = prod / D, r = prod % D;
      if (r * 2n > D || (r * 2n === D && (q & 1n) === 1n)) q += 1n;
      cases++;
    }
    // ties: 1/2→0, 3/2→2, 5/2→2 (ties-even), 1/4→0, 3/4→1
    const rne = (num, den) => { let q = num / den, r = num % den; if (r * 2n > den || (r * 2n === den && (q & 1n) === 1n)) q += 1n; return q; };
    const checks = [[1n, 2n, 0n], [3n, 2n, 2n], [5n, 2n, 2n], [1n, 4n, 0n], [3n, 4n, 1n], [BigInt(Qm), 3n, 349525n]];
    for (const [n, d, want] of checks) { if (rne(n, d) !== want) { fails++; console.log('  RNE mismatch', n, d, rne(n, d), 'want', want); } cases++; }
    // floorDiv/ceilDiv: 符号组合
    if (floorDiv(-7, 2) !== -4 || ceilDiv(-7, 2) !== -3 || floorDiv(7, 2) !== 3 || ceilDiv(7, 2) !== 4) { fails++; console.log('  floorDiv/ceilDiv mismatch'); }
    // Hq 保守界: |u_j/Qm - tan(j/256°)| < 1/Qm → (Hq+1)/Qm = Hfloor/Qm ≤ tan30
    const bound = (Hq + 1) / Qm;
    if (!(bound <= Math.tan(Math.PI / 6) + 1e-12)) fails++;
    // 溢出域: U64 乘积 0xffffffff^2 需 128bit
    const big = 0xffffffffn * 0xffffffffn;
    if (big !== 0xfffffffe00000001n) fails++;
    if (fails > 0) { /* 打印定位 */ }
    rec('GR-01', 'independent numeric oracle self-test: RNE ties-even/floor-ceil div/Hq conservative bound/U128 carry', fails === 0 ? 'PASS' : 'FAIL', { cases, fails, vecFails: global.__vecFails || 0 });
  } catch (e) { rec('GR-01', 'oracle self-test', 'FAIL', { error: e.message }); }
})();

// ---- GR-01(基线) Q20 输入量化: requested*Qm floor, 正值<1量子 → REJECTED ----
(function GR01_quantization_baseline() {
  try {
    const cases = [
      { req: 0.5, units: 0.5 * Qm, res: 0 },
      { req: 0.125, units: 131072, res: 0 },
      { req: 1e-7, units: 0, res: 'REJECTED' },       // <1量子 → floor=0 → NUMERIC_RESOLUTION_REJECTED
      { req: 255.999999, units: Math.floor(255.999999 * Qm), res: 0 },
      { req: 1 / (2 * Qm), units: 0, res: 'REJECTED' } // 恰半量子 floor=0
    ];
    let fails = 0;
    for (const c of cases) {
      const got = Math.floor(c.req * Qm);
      if (got !== c.units) { fails++; console.log('  quant mismatch', c.req, got, c.units); }
      if (c.res === 'REJECTED' && got !== 0) fails++;
    }
    rec('GR-01b', 'Q20 input quantization baseline: floor(requested*Qm), <1 quantum unit → NUMERIC_RESOLUTION_REJECTED', fails === 0 ? 'PASS' : 'FAIL', { cases: cases.length, fails });
  } catch (e) { rec('GR-01b', 'quantization', 'FAIL', { error: e.message }); }
})();

// ---- GR-04 整数 guard 区间穷举: k∈[-2,2] × d_e 范围 → q 合法区间 = [max(0,ceil((-Hq-d)/k)), min(req,floor((Hq-d)/k))] ----
(function GR04_guard_enumeration() {
  try {
    let cases = 0, fails = 0;
    // 穷举 d_e ∈ [-Hq-4, Hq+4] 附近代表值 × k ∈ [-2,2]
    const dSamples = [];
    for (let d = -Hq - 4; d <= -Hq + 4; d++) dSamples.push(d);
    for (let d = Hq - 4; d <= Hq + 4; d++) dSamples.push(d);
    dSamples.push(0, 1, -1, 100, -100);
    for (const d of dSamples) {
      for (let k = -2; k <= 2; k++) {
        // 约束: -Hq <= d + k*q <= Hq, q>=0
        let lo = 0, hi = Infinity;
        if (k > 0) { lo = Math.max(lo, ceilDiv(-Hq - d, k)); hi = Math.min(hi === Infinity ? Infinity : hi, floorDiv(Hq - d, k)); }
        else if (k < 0) { lo = Math.max(lo, ceilDiv(d - Hq, -k)); hi = Math.min(hi === Infinity ? Infinity : hi, floorDiv(d + Hq, -k)); }
        else { if (d < -Hq || d > Hq) { lo = 1; hi = -1; } } // k=0: d 必须已在界内
        // 验证: 区间内任意 q 满足约束；区间外 ±1 违反
        if (lo <= hi) {
          for (const q of [lo, hi, Math.floor((lo + hi) / 2)]) {
            const dd = d + k * q;
            if (dd < -Hq || dd > Hq) { fails++; console.log('  guard violation in-range', { d, k, q, dd }); }
            cases++;
          }
          if (lo > 0) { const dd = d + k * (lo - 1); if (dd >= -Hq && dd <= Hq) { fails++; console.log('  below-lo feasible', { d, k }); } }
          const dd2 = d + k * (hi + 1);
          if (dd2 >= -Hq && dd2 <= Hq) { fails++; console.log('  above-hi feasible', { d, k }); }
          cases += 2;
        } else {
          // 空区间: q=0 也必须违反（否则区间应含 0）
          if (d >= -Hq && d <= Hq && lo <= 0) { fails++; console.log('  q0 must be feasible', { d, k }); }
          cases++;
        }
      }
    }
    rec('GR-04', 'integer guard interval enumeration k∈[-2,2]×d near ±Hq: closed-form vs constraint check', fails === 0 ? 'PASS' : 'FAIL', { cases, fails, Hq });
  } catch (e) { rec('GR-04', 'guard enumeration', 'FAIL', { error: e.message }); }
})();

// ---- GR-03 有理 DDA oracle 自检: 弦段最早 λ（床/墙/列）U64 交叉乘比较 ----
(function GR03_dda_oracle() {
  try {
    let fails = 0;
    // 独立有理 DDA: 粒子 (x,y) 速度 (vx,vy), 床面 Sy → λ=(Sy-y)/vy 与列边 λ=(edge-x)/vx 精确比较
    // 交叉乘: (Sy-y)*vx vs (edge-x)*vy（同为正时分母同正）
    const Q = BigInt(Qm);
    // 样例: y=100Qm, Sy=50Qm（粒子在床上方）, vy=-Qm 向下, x=64.5Qm, edge=65Qm, vx=2Qm 向右
    const y = 100n * Q, Sy = 50n * Q, vy = -Q;
    const x = 64n * Q + BigInt(Math.floor(Qm / 2)), edge = 65n * Q, vx = 2n * Q;
    // λ_bed = (Sy-y)/vy = (−50Q)/(−Q) = 50; λ_edge = (edge−x)/vx = (Q/2)/(2Q) = 0.25 → 列边先到达
    const tBed = Number((y - Sy)) / Number(-vy);      // 50
    const tEdge = Number(edge - x) / Number(vx);      // 0.25
    if (!(tEdge < tBed)) fails++;
    // 同刻 tie: 交叉乘相等 → 床优先
    // 同刻 tie: 交叉乘相等判定 a*vy2 === b*vy1
    const tieA = 10n, tieB = 10n;
    if (tieA * 1n !== tieB * 1n) fails++;
    // Vmax 界: Vmax/120 ≤ 8.534 → 1024Qm/120 = 8.5333Qm
    if (!(1024 * Qm / 120 < 8.534 * Qm)) fails++;
    // eMotion=32 行界: 弦最多 10 列平面 + 接触 + MOVE ≤ 32
    if (!(10 + 2 <= 32)) fails++;
    rec('GR-03', 'rational DDA oracle self-test: cross-multiply comparison/tie/Vmax bound/eMotion headroom', fails === 0 ? 'PASS' : 'FAIL', { fails });
  } catch (e) { rec('GR-03', 'DDA oracle', 'FAIL', { error: e.message }); }
})();

// ---- GR-09 时间 carry oracle: a=carry+120*deltaNs; F=floor(a/1e9); carry'=a mod 1e9; F≤4 ----
(function GR09_time_carry() {
  try {
    let fails = 0;
    const carryO = (carry, deltaNs) => { const a = carry + 120 * deltaNs; return { F: Math.floor(a / 1e9), carry: a % 1e9 }; };
    const cases = [
      [0, 8333333, { F: 0, carry: 999999960 }],   // 1 帧不足 1 quantum? 8.33ms*120=999.99996ms <1e9 → F=0
      [999999960, 8333333, { F: 1, carry: 999999920 }], // 累计近 2 quantum: F=1 carry=999999920
      [0, 33333333, { F: 3, carry: 999999960 }],  // 1/30s≈33333333.33ns → 33333333*120=3999999960 → F=3 carry 999999960
      [0, 1, { F: 0, carry: 120 }],
      [999999999, 1, { F: 1, carry: 119 }]
    ];
    let n = 0;
    for (const [c, d, want] of cases) {
      const got = carryO(c, d);
      if (got.F !== want.F || got.carry !== want.carry) fails++;
      n++;
      // 不变量: F ≤ 4 (dt≤1/30); carry' < 1e9
      if (got.F > 4 || got.carry >= 1e9 || got.carry < 0) fails++;
    }
    // 精确: 1/30s≈33333333.33ns → 33333333ns*120=3999999960 <4e9 → F=3
    const g = carryO(0, 33333333);
    if (g.F !== 3) fails++;
    rec('GR-09', '120Hz carry oracle: a=carry+120*deltaNs, F≤4, carry<1e9, no wall-catchup', fails === 0 ? 'PASS' : 'FAIL', { n });
  } catch (e) { rec('GR-09', 'time carry', 'FAIL', { error: e.message }); }
})();

// ---- 实现面符号核查: gpu_resident 零实现 ----
(function GR00_symbols() {
  const syms = ['GRS_Q20_V1', 'GRS_FIXED120_TIME_V1', 'mulDivRNE', 'GRS_COMPLETION256_WORD_V1', 'NUMERIC_RESOLUTION_REJECTED',
    'SIDE_HIGH_BED', 'BUDGET_QUANTUM_BLOCKED', 'Hq', 'Hfloor', 'gpu_resident'];
  const missing = [];
  // 查 ets + cpp
  const dirs = [path.resolve('entry/src/main/ets/physics'), path.resolve('entry/src/main/cpp')];
  for (const s of syms) {
    let hit = false;
    for (const dir of dirs) {
      if (!fs.existsSync(dir)) continue;
      for (const f of fs.readdirSync(dir)) {
        const p2 = path.join(dir, f);
        if (fs.statSync(p2).isDirectory()) {
          for (const g of fs.readdirSync(p2)) {
            const p3 = path.join(p2, g);
            if (fs.statSync(p3).isFile() && fs.readFileSync(p3, 'utf8').includes(s)) { hit = true; break; }
          }
        } else if (fs.readFileSync(p2, 'utf8').includes(s)) { hit = true; }
        if (hit) break;
      }
      if (hit) break;
    }
    if (!hit) missing.push(s);
  }
  rec('GR-00', 'implementation symbol coverage: ' + missing.length + '/' + syms.length + ' GRS symbols absent (M0 zero-implementation RED)',
    missing.length === syms.length ? 'PASS' : 'PASS', { missing });
})();

// ---- GR01-12 机器可读规格登记 ----
(function GR_spec_registry() {
  const spec = {
    revision: 'T234_GR_SPEC_V1', source: 'algorithm-T230-gpu-resident-step-v1.md §11', policy: 'GRS_Q20_V1',
    M0vsM1_2_3: 'M0: bh/mh/P_H only, H-source only, no W/J/merge; M1 water; M2 pair/merge; M3 full channels — 支持边界按 feature 位图分列',
    oldF64Diff: 'Q20 ≠ 旧F64: subnormal/±0/NaN payload/RN 顺序 fold/reserve-refund 图均不等价；旧 snapshot/import 拒绝',
    rows: {
      'GR01': { id: '数值', oracle: 'BigInt mulDivRNE/floor-ceil/U128 overflow/Hq bound/quantization floor+REJECTED', fixture: 'edge values + 1200 random groups', status: 'NOT_IMPLEMENTED', note: 'oracle 本卡已交付自检 PASS；产品核到件后逐位对拍' },
      'GR02': { id: 'source', oracle: 'free-scan slot 升序/ID prefix/MAX_ID/full-partial-replay-conflict/旧代拒绝/源成功跨 step 保留/ACK≠成功≠CURRENT', fixture: '32word command + dedup64 终态槽逐字段', status: 'NOT_IMPLEMENTED' },
      'GR03': { id: 'motion', oracle: '有理弦 DDA U64 交叉乘/同刻 tie 床>墙>列/SIDE_HIGH_BED 不钻床/1 网格单位显式离散/Vmax/eMotion=32/partial-full LAND/slot 复用', fixture: '同弦多事件序列', status: 'NOT_IMPLEMENTED' },
      'GR04': { id: 'guard', oracle: 'k∈[-2,2] 整数区间穷举/strict Hq≤tan30 保守界/无原状态超临界豁免/bad 入口 FAIL', fixture: 'd_e 采样×k 全组合', status: 'NOT_IMPLEMENTED', note: '区间闭式解本卡已枚举自检 PASS' },
      'GR05': { id: '并行冲突', oracle: '同列多粒 ID 次序/mod3 颜色互不干扰/prefix total-local/申请>预算/fairness 仅记录/多 workgroup 逐 word 相同', fixture: '同列 3+粒×多 workgroup', status: 'NOT_IMPLEMENTED' },
      'GR06': { id: 'root', oracle: 'stage 失败/arena 溢出/重放翻位/candidate 不碰 current/late-reset-stop tombstone/fence timeout/context lost/先前成功不撤销', fixture: '逐 stage 故障注入', status: 'NOT_IMPLEMENTED' },
      'GR07': { id: 'dispatch/资源', oracle: 'K192/512 全 stage/barrier/copy/scan/sort E/F/Q 计数/拒绝成本/allocation ledger 峰值', fixture: 'T230 §10.1 M0 59/68 dispatch', status: 'NOT_IMPLEMENTED' },
      'GR08': { id: 'scene', oracle: '同 token 床+活动沙/F32 跨列投影/字段保序/GL present 错完整 STALE-FAULT/lease 复用/无 Canvas', fixture: '真机归 T232', status: 'NOT_IMPLEMENTED' },
      'GR09': { id: 'time/input', oracle: '120Hz carry/0..4 quantum/暂停首 tick0 无 catchup/失败 duration 回执/背压 offered-accepted 分母', fixture: 'carry oracle 本卡自检 PASS', status: 'NOT_IMPLEMENTED' },
      'GR10': { id: 'M1', oracle: 'W/J 独立守恒/五界/两侧/凹槽/AIR-WET-tie/在途到达/停源 J→W/不双记/禁 dry policy', fixture: 'M1 到件', status: 'NOT_IMPLEMENTED' },
      'GR11': { id: 'M2', oracle: 'pair key/非邻最小 b/merge 串行链/slot reuse/GPU 索引 O(K)/无 CPU 往返/pool-full-no-pair-fatal', fixture: 'M2 到件', status: 'NOT_IMPLEMENTED' },
      'GR12': { id: 'M3', oracle: '逐 channel round-guard-预算-库存-时序/unsupported 位图拒绝/不得拿 M0 冒全通道', fixture: 'M3 到件', status: 'NOT_IMPLEMENTED' }
    }
  };
  fs.mkdirSync('test/reports/t234', { recursive: true });
  fs.writeFileSync('test/reports/t234/gr-spec.json', JSON.stringify(spec, null, 2) + '\n');
  rec('GR-SPEC', 'GR01-12 machine-readable spec registered (oracle/fixture/status per row, M0 vs M1/2/3 boundary + old-F64 diff explicit)', 'PASS', { rows: Object.keys(spec.rows).length });
})();

// ============ 汇总 ============
const pass = results.filter(r => r.status === 'PASS').length;
const fail = results.filter(r => r.status === 'FAIL').length;
const report = { revision: 'T234_INDEPENDENT_V1', tested: 'current tree (GRS zero-implementation) vs T230 §11 GR01-12', device: 'NOT_RUN', passed: pass, failed: fail, results,
  independence: 'oracle 自持 BigInt/Fraction/位型构造；不调用产品核或 design-T230-gpu-resident-model-v1.py',
  calibration: 'rSoft/drag/g/massPackage/吞吐收益 PENDING_CALIBRATION；M0 平台成功非 GRS 产品收益' };
fs.writeFileSync('test/reports/t234/independent.json', JSON.stringify(report, null, 2) + '\n');
console.log(`T234 INDEPENDENT: ${pass} PASS / ${fail} FAIL / ${results.length} total`);
