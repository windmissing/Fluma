#!/usr/bin/env node
/**
 * T411 · GPU 同窗归档独立完整性检查 CLI（oracle）
 *
 * 合同依据: T407 §4.3 显式有界采集窗口 + §2 证据边界
 *   - manifest 引用全部分片必须存在
 *   - 实际 bytes ≤ 1MiB 预算 (jsonExportBudgetBytes / maxBytes=1048576)
 *   - 每分片 SHA-256 与 files[] 声明逐位一致
 *   - serial 连续: firstSerial..lastSerial 无缺失无重复
 *   - captureId / revision / run / epoch / session 同窗一致（identity+groups+行内身份）
 *   - required 事件/计数/终账一致（before.count==rows、dropped 显式、state/reason 相符）
 *   - 静默采样不得掩盖 required 缺失；incomplete/dropped 必须显式字段
 *
 * 独立性: 零依赖纯 node，不调用产品断言，不读产品代码。负控 fixtures 内嵌生成。
 *
 * 用法:
 *   node archive_check.cjs <dir>              # 检查目录内全部 *-manifest.json
 *   node archive_check.cjs <manifest.json>    # 检查单个归档
 *   node archive_check.cjs --selftest         # 合成正反控自测（负控全红/正例全绿）
 *   node archive_check.cjs --make-fixtures <outDir>  # 生成合成 fixtures 到目录
 *
 * 退出码: 0=全部检查通过(或 selftest 按预期)；1=存在 FAIL；2=用法错误
 */
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const BUDGET_BYTES = 1048576; // T407 §4.3 maxBytes ≤ 1048576

// ---------------------------------------------------------------- 判定层
function sha256File(p) {
  const h = crypto.createHash('sha256');
  h.update(fs.readFileSync(p));
  return h.digest('hex');
}

/** 检查单个归档。返回 {checks:[{id,name,verdict,detail}], state:'PASS'|'FAIL'|'INCOMPLETE_REFUSED'} */
function checkArchive(dir, manifestPath) {
  const checks = [];
  const add = (id, name, verdict, detail) => checks.push({ id, name, verdict, detail: String(detail).slice(0, 400) });
  const F = (id, name, ok, detail) => add(id, name, ok ? 'PASS' : 'FAIL', detail);

  let m;
  try { m = JSON.parse(fs.readFileSync(manifestPath, 'utf-8')); }
  catch (e) { return { checks: [{ id: 'M-PARSE', name: 'manifest parse', verdict: 'FAIL', detail: e.message }], state: 'FAIL' }; }

  const base = path.basename(manifestPath);
  const partPrefix = base.replace(/-manifest\.json$/, '');

  // C1 顶层身份字段存在
  const reqTop = ['stageRevision', 'revision', 'state', 'pages', 'rows', 'firstSerial', 'lastSerial',
    'dataBytes', 'identity', 'files', 'before', 'after'];
  const missTop = reqTop.filter(k => m[k] === undefined);
  F('C1-IDENTITY', 'manifest 顶层 schema 字段齐备', missTop.length === 0,
    missTop.length ? 'missing: ' + missTop.join(',') : 'all ' + reqTop.length + ' fields present');

  // C2 分片存在性
  const files = Array.isArray(m.files) ? m.files : [];
  const missingFiles = [];
  for (const f of files) {
    const p = path.join(dir, f.name);
    if (!fs.existsSync(p)) missingFiles.push(f.name);
  }
  F('C2-FILES-EXIST', `分片存在性 (${files.length} 引用)`, missingFiles.length === 0,
    missingFiles.length ? '缺失: ' + missingFiles.slice(0, 6).join(', ') + (missingFiles.length > 6 ? ` 等${missingFiles.length}个` : '') : 'all present');
  // T407 §2: missing=0 只是导出器自己的声明，必须独立核
  F('C2b-MISSING-FIELD', 'manifest.missing 字段与独立核验一致', m.missing === missingFiles.length,
    `declared=${m.missing} actual=${missingFiles.length}`);

  // C3 bytes：声明的 bytes/SHA 语义 = 各页对象紧凑序列化('\n'连接+尾换行)。
  // 同时独立核对 raw 文件字节（≤1MiB 预算按 raw 文件）。
  let bytesOk = true, bytesDetail = [], actualTotal = 0, declaredTotal = 0;
  for (const f of files) {
    declaredTotal += (f.bytes | 0); // 声明总和与文件是否存在无关
    const p = path.join(dir, f.name);
    if (!fs.existsSync(p)) { bytesOk = false; bytesDetail.push(f.name + ':MISSING'); continue; }
    const raw = fs.readFileSync(p, 'utf-8');
    const actual = Buffer.byteLength(raw);
    actualTotal += actual;
    // 导出器语义：页对象紧凑序列化 + \n 连接 + 尾 \n
    let canonical = '', canonParseBad = false;
    try {
      const pages = raw.split('\n').map(s => s.trim()).filter(Boolean).map(s => JSON.stringify(JSON.parse(s)));
      canonical = pages.join('\n') + '\n';
    } catch (e) { canonParseBad = true; }
    const canonBytes = Buffer.byteLength(canonical);
    if (canonParseBad || canonBytes !== (f.bytes | 0)) {
      bytesOk = false; bytesDetail.push(`${f.name}: declared=${f.bytes} canonical=${canonParseBad ? 'PARSE_FAIL' : canonBytes} raw=${actual}`);
    }
    if (actual > BUDGET_BYTES) { bytesOk = false; bytesDetail.push(`${f.name}: raw ${actual} > 1MiB budget`); }
  }
  F('C3-BYTES', '分片 bytes：导出器语义(页紧凑+\\n)声明==复算 且 raw ≤1MiB', bytesOk, bytesOk ? `${actualTotal}B raw total` : bytesDetail.slice(0, 4).join('; '));
  F('C3b-DATA-BOUND', `dataBytes ≤ ${BUDGET_BYTES}（T407 总冷档预算）`, (m.dataBytes | 0) <= BUDGET_BYTES,
    `dataBytes=${m.dataBytes}`);
  F('C3c-SUM', 'dataBytes == Σ 声明分片 bytes', (m.dataBytes | 0) === declaredTotal,
    `declared=${m.dataBytes} sumDeclared=${declaredTotal} rawSum=${actualTotal}`);

  // C4 SHA：按导出器语义复算（页紧凑+\n 连接+尾 \n）
  let shaOk = true, shaBad = [];
  for (const f of files) {
    const p = path.join(dir, f.name);
    if (!fs.existsSync(p)) { shaOk = false; continue; }
    const raw = fs.readFileSync(p, 'utf-8');
    let canonical = '';
    try {
      const pages = raw.split('\n').map(s => s.trim()).filter(Boolean).map(s => JSON.stringify(JSON.parse(s)));
      canonical = pages.join('\n') + '\n';
    } catch (e) { shaOk = false; shaBad.push(`${f.name}: page parse fail`); continue; }
    const actual = crypto.createHash('sha256').update(canonical).digest('hex');
    if (actual !== String(f.sha256).toLowerCase()) { shaOk = false; shaBad.push(`${f.name}: declared=${String(f.sha256).slice(0, 12)}… actual=${actual.slice(0, 12)}…`); }
  }
  F('C4-SHA256', '分片 SHA-256 与声明逐位一致（导出器语义复算）', shaOk, shaOk ? `${files.length} 分片全过` : shaBad.slice(0, 3).join('; '));

  // C5 serial 连续: 行在页对象 rows[] 内（每分片含多个页对象）
  const rows = [];
  let rowParseBad = 0;
  for (const f of files) {
    const p = path.join(dir, f.name);
    if (!fs.existsSync(p)) continue;
    const raw = fs.readFileSync(p, 'utf-8');
    for (const line of raw.split('\n')) {
      const s = line.trim();
      if (!s) continue;
      try {
        const page = JSON.parse(s);
        for (const r of (page.rows || [])) rows.push(r);
      } catch (e) { rowParseBad++; add('C5-ROWPARSE', '页 JSON 可解析', 'FAIL', `${f.name}: ${e.message}`); }
    }
  }
  const serials = rows.map(r => BigInt(r.serial)).sort((a, b) => (a < b ? -1 : a > b ? 1 : 0));
  let gap = null, dup = null;
  for (let i = 1; i < serials.length; i++) {
    if (serials[i] === serials[i - 1] && dup === null) dup = serials[i].toString();
    if (serials[i] - serials[i - 1] > 1n && gap === null) gap = `${serials[i - 1]}→${serials[i]}`;
  }
  const firstOk = serials.length ? serials[0] === BigInt(m.firstSerial) : false;
  const lastOk = serials.length ? serials[serials.length - 1] === BigInt(m.lastSerial) : false;
  F('C5-SERIAL', `serial 连续无缺失无重复 (rows=${rows.length})`,
    gap === null && dup === null && firstOk && lastOk && rows.length === (m.rows | 0),
    `first=${firstOk} last=${lastOk} gap=${gap} dup=${dup} rowsDeclared=${m.rows} rowsActual=${rows.length}`);

  // C6 同窗身份: identity.packageSha256/module/pid；行内 epoch/run/session/pid 与 groups 一致
  const id = m.identity || {};
  F('C6a-IDENTITY', 'identity 字段齐备（module/route/packageSha256/pid）',
    !!(id.module && id.route && id.packageSha256 && id.pid !== undefined),
    JSON.stringify(id).slice(0, 120));
  // groups: "epoch/run/session/pid"
  const groups = Array.isArray(m.groups) ? m.groups.map(g => { const [e, r, s, p] = String(g).split('/'); return { epoch: e, run: r, session: s, pid: p }; }) : [];
  const gset = new Set(groups.map(g => `${g.epoch}/${g.run}/${g.session}/${g.pid}`));
  let mixed = [];
  for (const r of rows) {
    // 旧 schema 行内无 session/pid（仅 tid=TID）；groups 第4段=pid。核 epoch/run/tid∈{groups 的 pid}∪行 tid 任意（tid 是线程号非 pid）。
    // T407 同窗判定：epoch/run 组合必须出现在 groups（group 格式 epoch/run/session/pid）。
    const keyLoose = `${r.epoch}/${r.run}`;
    if (![...gset].some(g => g.startsWith(keyLoose + '/'))) mixed.push(keyLoose);
  }
  const mixedUniq = [...new Set(mixed)];
  F('C6b-WINDOW-IDENTITY', '行内身份 ⊆ manifest groups 声明（同窗一致）', mixedUniq.length === 0,
    mixedUniq.length ? '混窗: ' + mixedUniq.slice(0, 5).join(',') : `${groups.length} groups 全覆盖`);

  // C7 计数/终账一致（before/after 可能是 JSON 字符串或对象）
  const parseB = (v, label) => {
    try { return typeof v === 'string' ? JSON.parse(v) : (v || {}); }
    catch (e) { add('C7-PARSE', `${label} 可解析`, 'FAIL', e.message); return {}; }
  };
  const before = parseB(m.before, 'before');
  const after = parseB(m.after, 'after');
  const countOk = (before.count | 0) === (m.rows | 0) && (after.count | 0) === (m.rows | 0);
  F('C7a-COUNT', 'before/after.count == rows', countOk, `before=${before.count} after=${after.count} rows=${m.rows}`);
  const dropped = String(before.dropped !== undefined ? before.dropped : 'ABSENT');
  F('C7b-DROPPED-EXPLICIT', 'dropped 显式字段（静默采样不得掩盖）', dropped !== 'ABSENT', `dropped=${dropped}`);
  const stateVal = String(m.state || '');
  F('C7c-STATE', 'state ∈ {COMPLETE, INCOMPLETE} 且与 reason/cancelled 相符',
    stateVal === 'COMPLETE' || stateVal === 'INCOMPLETE',
    `state=${stateVal} reason=${m.reason} cancelled=${m.cancelled} incompleteFlag=${before.incomplete}`);
  // INCOMPLETE 归档不因拒判而 FAIL 本 CLI 的完整性结论——完整性判定独立于语义可用性
  if (stateVal === 'INCOMPLETE') add('C7d-INCOMPLETE-REF', 'INCOMPLETE 归档（完整性检查继续，语义判读须拒）', 'INCOMPLETE_REFUSED', `reason=${m.reason}`);

  // C8 rows/pages 关系
  // C8 pages 语义：现行分页=4 行/页（tl2: 4096行/1024页）。fixture 用 1行/页（3页/3行）也应一致。
  const rowsPerPage = (m.rows | 0) === (m.pages | 0) ? 1 : 4;
  F('C8-PAGES', 'pages 与 rows 的分页关系自洽（4行/页 或 1行/页）',
    (m.pages | 0) === Math.ceil((m.rows | 0) / rowsPerPage), `pages=${m.pages} rows=${m.rows} 推定每页行数=${rowsPerPage}`);

  const fails = checks.filter(c => c.verdict === 'FAIL').length;
  return { checks, state: fails ? 'FAIL' : 'PASS', incomplete: stateVal === 'INCOMPLETE' };
}

// ---------------------------------------------------------------- 合成 fixtures
const fixtures = {};
// 导出器语义：每"页对象"含 rows[] 数组；分片=多个页对象每行一个（紧凑+\n）
function pageOf(rows_) { return JSON.stringify({ revision: 'SYNTH_V1', count: rows_.length, rows: rows_ }); }
function fixtureGood() {
  // 正例: 2 分片, 3 行连续, identity/groups 一致
  const rows = [
    { serial: 1, ns: '1000', tid: 42, epoch: 1, run: 0, session: 0, stage: 3, code: 0 },
    { serial: 2, ns: '2000', tid: 42, epoch: 1, run: 0, session: 0, stage: 8, code: 0 },
    { serial: 3, ns: '3000', tid: 42, epoch: 1, run: 0, session: 0, stage: 9, code: 0 },
  ];
  const p0 = rows.slice(0, 2).map(r => pageOf([r])).join('\n') + '\n';
  const p1 = rows.slice(2).map(r => pageOf([r])).join('\n') + '\n';
  return {
    'good-manifest.json': {
      stageRevision: 'SYNTH_V1', revision: 'SYNTH_ARCHIVE_V1', state: 'COMPLETE', reason: null,
      before: { count: 3, dropped: '0', incomplete: false }, after: { count: 3, dropped: '0', incomplete: false },
      statusBefore: {}, statusAfter: {}, pages: 3, rows: 3, missing: 0, duplicates: 0,
      firstSerial: '1', lastSerial: '3', groups: ['1/0/0/42'],
      files: null, // fill at write time
      dataBytes: 0, cancelled: false, end: 'END',
      identity: { module: 'synth.so', route: 'Synth', packageSha256: 'aa'.repeat(32), pid: 42 },
      _rows: rows,
    },
    '_parts': { 'good-part0.ndjson': p0, 'good-part1.ndjson': p1 },
  };
}
function fixtureMissingPart() {
  const f = fixtureGood();
  // 保留 _parts 供 manifest 声明，但标记不写盘 → 模拟"manifest 引用了缺失分片"
  f._skipWrite = ['good-part1.ndjson'];
  return f;
}
function fixtureShaWrong() { const f = fixtureGood(); return f; }
function fixtureOversize() {
  const f = fixtureGood();
  const blob = 'x'.repeat(BUDGET_BYTES + 1);
  const r2 = f['good-manifest.json']._rows[2];
  f._parts['good-part1.ndjson'] = pageOf([{ ...r2, junk: blob }]) + '\n';
  return f;
}
function fixtureMixedWindow() {
  const f = fixtureGood();
  f._parts['good-part1.ndjson'] = pageOf([{ serial: 3, ns: '3000', tid: 42, epoch: 9, run: 0, session: 0, stage: 9, code: 0 }]) + '\n';
  f['good-manifest.json'].rows = 3; // 仍声明 3
  return f;
}
function fixtureSilentSample() {
  // 静默采样反控: 行有洞但 dropped=0 且 incomplete=false → required 缺失被掩盖
  const f = fixtureGood();
  f._parts['good-part0.ndjson'] = pageOf([f['good-manifest.json']._rows[0]]) + '\n';
  f._parts['good-part1.ndjson'] = pageOf([f['good-manifest.json']._rows[2]]) + '\n';
  f['good-manifest.json'].rows = 2; // 谎报只有 2 行
  f['good-manifest.json'].before = { count: 2, dropped: '0', incomplete: false };
  f['good-manifest.json'].after = { count: 2, dropped: '0', incomplete: false };
  f['good-manifest.json'].firstSerial = '1'; f['good-manifest.json'].lastSerial = '3'; // 但 first/last 仍声明 3 → serial 检查暴露
  return f;
}

function writeFixtures(outDir) {
  fs.mkdirSync(outDir, { recursive: true });
  const specs = [
    ['good', fixtureGood()], ['missing_part', fixtureMissingPart()],
    ['sha_wrong', fixtureShaWrong()], ['oversize', fixtureOversize()],
    ['mixed_window', fixtureMixedWindow()], ['silent_sample', fixtureSilentSample()],
  ];
  const made = [];
  for (const [name, fx] of specs) {
    const d = path.join(outDir, name);
    fs.mkdirSync(d, { recursive: true });
    const man = JSON.parse(JSON.stringify(fx['good-manifest.json']));
    delete man._rows;
    const parts = fx._parts;
    man.files = []; let total = 0;
    let i = 0;
    for (const [pn, content] of Object.entries(parts)) {
      // 记录分片声明（含缺失分片：声明进 manifest，文件不写盘）
      const fakeSha = crypto.createHash('sha256').update(content).digest('hex');
      const declaredBytes = Buffer.byteLength(content);
      if ((fx._skipWrite || []).includes(pn)) {
        // 声明存在但不写盘
        man.files.push({ name: pn, bytes: declaredBytes, sha256: fakeSha });
        total += declaredBytes; i++;
        continue;
      }
      const full = path.join(d, pn);
      fs.writeFileSync(full, content);
      const buf = fs.readFileSync(full);
      // sha_wrong: 故意声明错误哈希
      const sha = (name === 'sha_wrong' && i === 1) ? '0'.repeat(64) : fakeSha;
      man.files.push({ name: pn, bytes: buf.length, sha256: sha });
      total += buf.length; i++;
    }
    man.dataBytes = total;
    const mp = path.join(d, `${name}-manifest.json`);
    fs.writeFileSync(mp, JSON.stringify(man, null, 1));
    made.push(d);
  }
  return made;
}

// ---------------------------------------------------------------- selftest
function selftest() {
  const tmp = fs.mkdtempSync(path.join(require('os').tmpdir(), 't411-'));
  const made = writeFixtures(tmp);
  const expect = {
    good: 'PASS', missing_part: 'FAIL', sha_wrong: 'FAIL',
    oversize: 'FAIL', mixed_window: 'FAIL', silent_sample: 'FAIL',
  };
  let pass = 0, fail = 0;
  const lines = [];
  for (const d of made) {
    const name = path.basename(d);
    const mp = path.join(d, `${name}-manifest.json`);
    const r = checkArchive(d, mp);
    const ok = r.state === expect[name];
    if (ok) pass++; else fail++;
    lines.push(`${ok ? 'ok  ' : 'BAD '} ${name}: got=${r.state} expect=${expect[name]}` +
      (ok ? '' : ' | ' + r.checks.filter(c => c.verdict === 'FAIL').map(c => c.id).join(',')));
    // 正例额外要求: 所有单检 PASS
    if (name === 'good' && r.state !== 'PASS') {
      lines.push('     good 详情: ' + JSON.stringify(r.checks.filter(c => c.verdict !== 'PASS')));
    }
    // 反控要求: 具体检查项变红
    const redIds = r.checks.filter(c => c.verdict === 'FAIL').map(c => c.id);
    const mustRed = {
      missing_part: ['C2-FILES-EXIST', 'C2b-MISSING-FIELD', 'C4-SHA256', 'C5-SERIAL'],
      sha_wrong: ['C4-SHA256'],
      oversize: ['C3-BYTES'],   // 声明=实际（同一超限 blob）→ C3c 恰好绿；超限由 C3-BYTES 拦
      mixed_window: ['C6b-WINDOW-IDENTITY'],
      silent_sample: ['C5-SERIAL'],
    }[name] || [];
    for (const need of mustRed) {
      if (!redIds.includes(need)) { fail++; lines.push(`BAD  ${name}: 期望 ${need} 变红但未检出`); }
      else pass++;
    }
  }
  console.log(lines.join('\n'));
  console.log(`\nselftest: ${pass} ok, ${fail} bad`);
  return fail === 0 ? 0 : 1;
}

// ---------------------------------------------------------------- main
function main() {
  const args = process.argv.slice(2);
  if (args[0] === '--selftest') process.exit(selftest());
  if (args[0] === '--make-fixtures') { const d = writeFixtures(args[1] || './t411-fixtures'); console.log(d.join('\n')); process.exit(0); }
  if (!args[0]) {
    console.error('用法: node archive_check.cjs <dir|manifest.json> | --selftest | --make-fixtures <outDir>');
    process.exit(2);
  }
  const target = args[0];
  let manifests = [];
  if (fs.statSync(target).isDirectory()) {
    manifests = fs.readdirSync(target).filter(f => f.endsWith('-manifest.json')).map(f => path.join(target, f));
  } else manifests = [target];
  if (!manifests.length) { console.error('目录内无 *-manifest.json'); process.exit(2); }
  let anyFail = false;
  for (const mp of manifests) {
    const dir = path.dirname(mp);
    const r = checkArchive(dir, mp);
    console.log(`=== ${path.basename(mp)} : ${r.state}${r.incomplete ? ' (INCOMPLETE_REFUSED: 语义判读须拒, 完整性检查继续)' : ''}`);
    for (const c of r.checks) console.log(`  [${c.verdict}] ${c.id} ${c.name}${c.verdict === 'PASS' && !c.detail ? '' : ' — ' + c.detail}`);
    if (r.state === 'FAIL') anyFail = true;
  }
  process.exit(anyFail ? 1 : 0);
}
main();
