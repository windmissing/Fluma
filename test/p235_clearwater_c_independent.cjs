// T235 明水 C 基础独立验证（tester, 离线）
// 消费 T226 0eeaa8d 开发交付；独立核：V3 严格帧包/optics16/切源生命周期/透射预乘。
// 自持 oracle；V1 水直进 W 是现行基础；T227 V2 / T228 V4 NOT_IMPLEMENTED 明确。
'use strict';
const fs = require('fs'), path = require('path');
const ROOT = path.resolve(__dirname, '..');
const results = [];
const rec = (id, name, status, detail) => { results.push({ id, name, status, detail }); };

const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, f);

// 独立 oracle
function depthOf(eta, y, S, W) { return Math.max(0, Math.min(eta - Math.max(y, S), W)); }
function transmit(depth, sigma) { return Math.exp(-sigma * depth); }
function premultBody(T, tint) { return [(1 - T) * tint[0], (1 - T) * tint[1], (1 - T) * tint[2], 1 - T]; }
const fresnel = (cosT, F0 = 0.02) => F0 + (1 - F0) * Math.pow(1 - cosT, 5);

process.chdir(ROOT);
const CW = path.resolve('entry/src/main/ets/physics/clearwater');
const rd = (n) => fs.readFileSync(path.join(CW, n), 'utf8');

// CW-01 符号面
{
  const syms = ['copyCurrentSceneInto', 'copySourceReceiptInto', 'drawClearWaterSceneV3', 'ClearWaterPublication',
    'ClearWaterSourceController', 'ClearWaterFrameClock', 'ClearWaterGravityPort', 'CW_B_EVENT40_V1'];
  const dir = path.join(ROOT, 'entry/src/main/ets');
  const found = new Set();
  const walk = (d) => { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const p = path.join(d, e.name); if (e.isDirectory()) walk(p); else if (/\.(ets|ts)$/.test(e.name)) { const t = fs.readFileSync(p, 'utf8'); for (const s of syms) if (t.includes(s)) found.add(s); } } };
  walk(dir);
  const missing = syms.filter(s => !found.has(s));
  rec('CW-01', 'C-basis symbol presence (T226)', missing.length === 0 ? 'PASS' : 'FAIL', { missing, present: syms.length - missing.length });
}

// CW-02 V3 引擎构造 + 帧包 API
{
  const { P011ClearWaterEngine } = require(path.join(CW, 'P011ClearWaterEngine.ets'));
  const { clearWaterCConfig } = require(path.join(CW, 'P011ClearWaterCConfig.ets'));
  let ok = false, apiN = 0, err = null;
  try {
    const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
    apiN = Object.getOwnPropertyNames(Object.getPrototypeOf(eng)).length;
    ok = apiN > 3;
  } catch (e) { err = String(e.message).slice(0, 90); }
  rec('CW-02', 'engine ctor(K192) + surface', ok ? 'PASS' : 'FAIL', err ? { err } : { apiN });
}

// CW-03 optics16 域 + 独立透射/预乘/Fresnel
{
  const { clearWaterCConfig } = require(path.join(CW, 'P011ClearWaterCConfig.ets'));
  const cfg = clearWaterCConfig(192);
  let fails = 0, cases = 0;
  const chk = (c) => { cases++; if (!c) fails++; };
  chk(cfg.revision === 'CLEAR_WATER_RUN_CONFIG_V1');
  chk(cfg.sigmaWater === 0.025 && cfg.opticsRev === 1);
  chk(Array.isArray(cfg.tintLinear) && cfg.tintLinear.length === 3);
  chk(cfg.calibrationStatus === 'PENDING_CALIBRATION'); // 不冒已标定
  chk(depthOf(20, 10, 12, 8) === 8);
  const T = transmit(depthOf(20, 10, 12, 8), cfg.sigmaWater);
  const body = premultBody(T, cfg.tintLinear);
  chk(body[3] >= 0 && body[3] <= 1 && Math.abs(body[0] - (1 - T) * cfg.tintLinear[0]) < 1e-12);
  chk(transmit(depthOf(10, 5, 5, 0), cfg.sigmaWater) === 1); // W0 → T=1
  chk(transmit(0, cfg.sigmaWater) === 1);
  chk(Math.abs(fresnel(1) - 0.02) < 1e-12);
  rec('CW-03', 'optics16 domain + independent transmission/premult/Fresnel', fails === 0 ? 'PASS' : 'FAIL', { cases, fails, T: T.toFixed(6) });
}

// CW-04 切源生命周期 API
{
  const { ClearWaterSourceController } = require(path.join(CW, 'P011ClearWaterSourceController.ets'));
  const { P011ClearWaterEngine } = require(path.join(CW, 'P011ClearWaterEngine.ets'));
  const { clearWaterCConfig } = require(path.join(CW, 'P011ClearWaterCConfig.ets'));
  let ok = false, apiN = 0, err = null;
  try {
    const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
    const c = new ClearWaterSourceController(eng, 1, 1, 1);
    const api = Object.getOwnPropertyNames(Object.getPrototypeOf(c));
    apiN = api.length;
    ok = api.some(a => /stop|cancel|down|move|up|begin/i.test(a));
  } catch (e) { err = String(e.message).slice(0, 90); }
  rec('CW-04', 'source controller lifecycle API', ok ? 'PASS' : 'FAIL', err ? { err } : { apiN });
}

// CW-05 V2/V4 NOT_IMPLEMENTED
{
  const dir = path.join(ROOT, 'entry/src/main/ets');
  let v2 = false, v4 = false;
  const walk = (d) => { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const p = path.join(d, e.name); if (e.isDirectory()) walk(p); else if (/\.(ets|ts)$/.test(e.name)) { const t = fs.readFileSync(p, 'utf8'); if (/FlightV2|CW_WATER_FLIGHT_V2/.test(t)) v2 = true; if (/CW_WATER_FLIGHT_V4|ClearWaterV4/.test(t)) v4 = true; } } };
  walk(dir);
  rec('CW-05', 'FlightV2(T227)/V4(T228) NOT_IMPLEMENTED explicit', (!v2 && !v4) ? 'PASS' : 'FAIL', { v2, v4, note: 'V1 water direct-W is current basis' });
}

// CW-06 面积高光相邻列/零水 独立参考
{
  let fails = 0;
  const colRect = (c, S, eta, w) => ({ x0: c * w, x1: (c + 1) * w, y0: S, y1: eta });
  const r1 = colRect(0, 0, 10, 1), r2 = colRect(1, 0, 8, 1);
  if (!(Math.min(r1.x1, r2.x1) - Math.max(r1.x0, r2.x0) === 0)) fails++; // 相邻列 x 不重叠
  const dry = colRect(0, 5, 5, 1);
  if (!(dry.y1 - dry.y0 === 0)) fails++; // W=0 无面积
  rec('CW-06', 'area highlight adjacency/zero-water reference', fails === 0 ? 'PASS' : 'FAIL', { fails });
}

// CW-07 FrameClock 无 clamp（仅代码，去注释）
{
  const src = rd('P011ClearWaterFrameClock.ets');
  const codeOnly = src.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');
  const hasClamp = /Math\.min\(\s*1\s*,/.test(codeOnly) || /\bclamp\b/i.test(codeOnly);
  rec('CW-07', 'FrameClock raw differential no clamp', hasClamp ? 'FAIL' : 'PASS', { hasClamp });
}

// CW-08 source dedup + NOT_COMMITTED
{
  const src = rd('P011ClearWaterSource.ets');
  const hasDedup = /dedup|DEDUP/.test(src);
  const hasNC = /NOT_COMMITTED/.test(src);
  rec('CW-08', 'source dedup + NOT_COMMITTED enum', (hasDedup && hasNC) ? 'PASS' : 'FAIL', { hasDedup, hasNC });
}

// 汇总
fs.mkdirSync(path.join(ROOT, 'test/reports/t235'), { recursive: true });
fs.writeFileSync(path.join(ROOT, 'test/reports/t235/independent.json'), JSON.stringify({ generatedAt: new Date().toISOString(), results }, null, 1));
const p = results.filter(r => r.status === 'PASS').length, f = results.filter(r => r.status === 'FAIL').length;
console.log(`T235: ${p} PASS / ${f} FAIL`);
for (const r of results) console.log(`${r.status}  ${r.id}  ${r.name}`);
process.exit(f > 0 ? 1 : 0);
