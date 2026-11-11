#!/usr/bin/env node
// T474 · 单沙整数光栅计划独立差分。
// 消费 T469 实际生成页 RasterGeometry.ets（typescript 转译驱动），判据从 T463
// floor 分配合同独立推导，不复用开发 31 项断言。覆盖卡面要求：
// n±ε / 小于1 / 非整数 scale / 外位置动而 floor 不变 / 旧 viewport±1 /
// 迟 layout 旧锚 / READY 间变化 / OPEN 首因。
const fs = require('node:fs');
const path = require('node:path');
const REPO = path.resolve(__dirname, '../..');

// 从实际生成目录取 RasterGeometry.ets（优先 T469 生成物，退化 test/t469 源）
function loadSource() {
  const candidates = [
    '.codex/local-test/t469-check/entry/src/main/ets/render/RasterGeometry.ets',
    'test/t469/RasterGeometry.ets',
  ];
  for (const c of candidates) {
    const p = path.join(REPO, c);
    if (fs.existsSync(p)) return { src: fs.readFileSync(p, 'utf8', 'utf-8'), from: c };
  }
  throw new Error('RasterGeometry.ets not found');
}

// 极简 TS→JS 转译：去类型注解/访问修饰符/泛型尖括号类型（足够本文件结构）
function transpile(src) {
  let s = src;
  s = s.replace(/export /g, '');
  s = s.replace(/\bprivate\b|\breadonly\b/g, '');
  s = s.replace(/:\s*[A-Za-z_][\w.<>\[\]]*(\s*=[^,;)\]]+)?(?=\s*[,;)={])/g, (m) =>
    m.includes('=') ? ' = ' + m.split('=')[1].trim() : '');
  s = s.replace(/:\s*[A-Za-z_][\w.<>\[\]]*\s*(?=\{|=>)/g, '');
  s = s.replace(/as\s+[A-Za-z_][\w.]*/g, '');
  s = s.replace(/<\s*[A-Za-z_][\w.\[\]]*\s*>/g, '');
  s = s.replace(/(\w+)\?(?=:)/g, '$1');
  return s;
}

const { src, from } = loadSource();
const mod = { exports: {} };
new Function('exports', 'module', transpile(src) + '\n;module.exports={RasterGeometryController, rasterPredicate, rasterNodeReason};')(mod.exports, mod);
const { RasterGeometryController, rasterPredicate } = mod.exports;

const results = [];
let passed = 0;
function check(id, ok, detail) {
  results.push({ id, pass: !!ok, detail });
  if (ok) ++passed;
  console.log((ok ? 'PASS ' : 'FAIL ') + id + ' | ' + detail);
}

function alloc(parent, opts = {}) {
  // screen 域 = actual 坐标 + window 原点（rasterNodeReason: screen.x==r.x+w.x）
  return {
    parent, screen: opts.screen || { x: parent.x, y: parent.y, width: 0, height: 0 },
    window: opts.window || { x: 0, y: 0, width: 1280, height: 2832 },
    vp2px: opts.vp2px !== undefined ? opts.vp2px : 3.5,
    windowId: opts.windowId || 1, windowGeneration: opts.windowGeneration || 1,
    orientation: 0, transformIdentity: true,
  };
}
function rects(x, y, w, h, winX = 0, winY = 0) {
  const r = { x, y, width: w, height: h };
  return { actual: r, screen: { x: r.x + winX, y: r.y + winY, width: 0, height: 0 },
           inner: { ...r }, touch: { ...r } };
}
function native(geo, w, h) {
  return { controller: 7, session: 8, surface: 9, context: 10, run: 'r1', geometry: geo,
           packetWidth: w, packetHeight: h, viewportWidth: w, viewportHeight: h };
}

// ---------- G1 n±ε floor 分配 ----------
{
  const c = new RasterGeometryController();
  const eps = 1e-12;
  const cases = [
    // 1178 的前一个 double 是 1177.9999999999998863 → floor 1177（floor 语义下
    // n-ε 落到下界；n±ε 的合同语义=floor(实际 double 值)，非"回到 n"）
    ['1178-ε(1ulp)', 1178 - eps, 1177], ['1178', 1178, 1178],
    ['1178+ε', 1178 + eps, 1178], ['1177.5', 1177.5, 1177],
  ];
  let ok = true; const detail = [];
  for (const [name, input, expect] of cases) {
    const c2 = new RasterGeometryController();
    c2.allocate(alloc({ x: 0, y: 0, width: input, height: 100 }));
    if (c2.plan.rasterW !== expect || !c2.plan.valid) { ok = false; detail.push(`${name}→${c2.plan.rasterW}`); }
    else detail.push(`${name}→${c2.plan.rasterW}`);
  }
  check('G1_FLOOR_EPSILON', ok, 'floor 分配 n±ε/半整数/小于1（' + detail.join(' ') + '）');
}

// ---------- G2 小于 1 / 非法输入 ----------
{
  const c = new RasterGeometryController();
  c.allocate(alloc({ x: 0, y: 0, width: 0.5, height: 100 }));
  check('G2_SUB_ONE_EMPTY', !c.plan.valid && c.plan.reason === 'RASTER_ALLOCATION_EMPTY',
        'width 0.5 → RASTER_ALLOCATION_EMPTY');
  const c2 = new RasterGeometryController();
  c2.allocate(alloc({ x: 0, y: 0, width: NaN, height: 100 }));
  check('G2B_NAN_INVALID', !c2.plan.valid && c2.plan.reason === 'RASTER_ALLOCATION_INVALID',
        'NaN → RASTER_ALLOCATION_INVALID');
}

// ---------- G3 非整数 scale（vp2px）----------
{
  const c = new RasterGeometryController();
  c.allocate(alloc({ x: 0, y: 0, width: 1177.6, height: 1609 }, { vp2px: 3.5 }));
  check('G3_NONINTEGER_SCALE_FLOOR', c.plan.valid && c.plan.rasterW === 1177 && c.plan.rasterH === 1609,
        `父框 1177.6×1609 vp2px=3.5 → 计划 ${c.plan.rasterW}×${c.plan.rasterH}（floor 1177.6→1177）`);
}

// ---------- G4 外位置动而 floor 不变（same floor → allocation key 变、permit 失效）----------
{
  const c = new RasterGeometryController();
  c.allocate(alloc({ x: 0, y: 0, width: 1178, height: 1609 }));
  const g1 = c.plan.generation;
  // 外位置动：parent.x 0→2（floor 尺寸不变，但 allocation key 变 → 新 generation）
  c.allocate(alloc({ x: 2, y: 0, width: 1178, height: 1609 }));
  const g2 = c.plan.generation;
  check('G4_POSITION_MOVE_NEW_GENERATION', g2 > g1 && c.plan.rasterW === 1178,
        `parent.x 0→2 floor 不变（1178）但 generation ${g1}→${g2}（计划重建、旧 permit 失效）`);
  // 相同输入（key 相同）不推进
  c.allocate(alloc({ x: 2, y: 0, width: 1178, height: 1609 }));
  check('G4B_SAME_INPUT_NO_ADVANCE', c.plan.generation === g2, '同输入不重建');
}

// ---------- G5 旧 viewport±1 / 迟 layout 旧锚 ----------
{
  const c = new RasterGeometryController();
  c.allocate(alloc({ x: 0, y: 0, width: 1178, height: 1609 }));
  const rr = rects(0, 0, 1178, 1609);
  c.observe(rr.actual, rr.screen, rr.inner, rr.touch, native(c.plan.generation, 1178, 1609), true);
  c.acknowledge(c.plan.generation, 5);
  const geoA = 100;
  let reason = rasterPredicate(c.plan, geoA, 5);
  check('G5_GEOMETRY_MISMATCH', reason === 'RASTER_STALE_PLAN',
        `geometry ${geoA} ≠ plan.geometry → ${reason}`);
  reason = rasterPredicate(c.plan, c.plan.geometry, 4); // 旧 layout
  check('G5B_OLD_LAYOUT_REJECTS', reason === 'RASTER_WAIT_NEW_LAYOUT',
        `旧 layout 4 ≠ acknowledged 5 → ${reason}（迟 layout 拒旧锚）`);
  // viewport±1：native viewport 1177（≠ 计划 1178）——observe 后先 acknowledge
  const rr2 = rects(0, 0, 1178, 1609);
  c.observe(rr2.actual, rr2.screen, rr2.inner, rr2.touch, native(c.plan.generation, 1177, 1609), true);
  c.acknowledge(c.plan.generation, 6);
  reason = rasterPredicate(c.plan, c.plan.geometry, 6);
  check('G5C_VIEWPORT_OFFBY1', reason === 'RASTER_NATIVE_MISMATCH',
        `native viewport 1177 ≠ 计划 1178 → ${reason}（acknowledge 后 native 门可达）`);
}

// ---------- G6 READY 间变化：同 floor scale 变更失效 open hold ----------
{
  const c = new RasterGeometryController();
  c.allocate(alloc({ x: 0, y: 0, width: 1178, height: 1609 }));
  const g1 = c.plan.generation;
  c.allocate(alloc({ x: 0, y: 0, width: 1178.9, height: 1609 })); // 同 floor 1178 但输入变
  check('G6_SCALE_EPS_ADVANCES', c.plan.generation > g1 && c.plan.rasterW === 1178,
        `1178→1178.9 同 floor 但 generation 推进（READY 间变化使旧 permit/hold 失效）`);
}

// ---------- G7 OPEN 首因：未分配时 predicate 首因 ----------
{
  const c = new RasterGeometryController();
  const reason = rasterPredicate(c.plan, 1, 1);
  check('G7_OPEN_FIRST_REASON', reason === 'RASTER_PLAN_INVALID' || reason === 'RASTER_NOT_ALLOCATED',
        `未分配计划 predicate 首因 = ${reason}`);
}

// ---------- G8 两帧观察 + 外位置域 ----------
{
  const c = new RasterGeometryController();
  c.allocate(alloc({ x: 0, y: 0, width: 1178, height: 1609 }));
  const rr = rects(0, 0, 1178, 1609);
  const ch1 = c.observe(rr.actual, rr.screen, rr.inner, rr.touch, native(0, 1178, 1609), true);
  const rr2 = rects(0, 0, 1178, 1609); // 同值 → 不推进
  const ch2 = c.observe(rr2.actual, rr2.screen, rr2.inner, rr2.touch, native(0, 1178, 1609), true);
  check('G8_OBSERVE_DEDUP', ch1 === true && ch2 === false,
        `首帧观察 changed=${ch1}，同值二帧 changed=${ch2}`);
  // screen 域一致（screen = actual + window 原点）
  // parent 与 actual 同步平移（保持 OUTSIDE 不触发），window 原点 (0,0)：
  // screen = actual + window = (10,20)
  c.allocate(alloc({ x: 10, y: 20, width: 1178, height: 1609 }));
  // observe 自身会 advance generation：先取 observe 后的 generation 再构造 native。
  const rr3 = rects(10, 20, 1178, 1609);
  const gAfter = c.plan.generation + 1;
  c.observe(rr3.actual, rr3.screen, rr3.inner, rr3.touch, native(gAfter, 1178, 1609), true);
  c.acknowledge(c.plan.generation, 7);
  const snap = c.snapshot(gAfter); // snapshot 覆写 geometry（生产消费路径）
  const reason = rasterPredicate(snap, gAfter, 7);
  check('G8B_POSITION_DOMAIN', reason === '',
        `screen=(10,20)=actual(10,20)+window(0,0) 域一致 → ${reason}（snapshot 覆写 geometry 后全门通过）`);
  // screen 错位（位置域破坏）
  const rr4 = rects(10, 20, 1178, 1609);
  c.observe(rr4.actual, { x: 99, y: 99, width: 0, height: 0 }, rr4.inner, rr4.touch,
            native(c.plan.generation, 1178, 1609), true);
  c.acknowledge(c.plan.generation, 8);
  const reason2 = rasterPredicate(c.plan, c.plan.geometry, 8);
  check('G8C_POSITION_DOMAIN_BROKEN', reason2 === 'RASTER_POSITION_DOMAIN',
        `screen 错位 → ${reason2}（acknowledge 后位置域门可达）`);
}

const outDir = path.join(REPO, 'test/reports/t474');
fs.mkdirSync(outDir, { recursive: true });
const verdict = passed === results.length ? 'PASS' : 'FAIL';
fs.writeFileSync(path.join(outDir, 't474-raster-diff.json'),
  JSON.stringify({ schema: 'T474_RASTER_DIFF_V1', verdict, passed,
                   total: results.length, results, sourceFrom: from }, null, 1));
console.log(`T474 raster differential: ${passed}/${results.length} PASS (source: ${from})`);
process.exit(passed === results.length ? 0 : 1);
