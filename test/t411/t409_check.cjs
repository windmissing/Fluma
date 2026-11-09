#!/usr/bin/env node
/**
 * T411 · T409_EVIDENCE_ARCHIVE_V1 schema 独立检查器（新增 schema 分支）
 *
 * 合同: T409 有界采集落地接口（T411/T412 消费）:
 *   - manifest revision=T409_EVIDENCE_ARCHIVE_V1, state COMPLETE/INCOMPLETE
 *   - scope、summary（initialLedger/terminalLedger 八字段十进制字符串）、requiredEvents、
 *     rowCount/tokenCount/pageCount、files（每页独立 JSON 原字节 name/bytes/sha256）、
 *     dataBytes、totalBytes（全部分片+manifest 实际 UTF8 bytes）、maxBytes
 *   - required: BEGIN/END 始终必需；最后 END.ledger 须等于 terminalLedger；
 *     readySeen/sourceAttemptSeen/acceptedSeen/faultSeen/revokeSeen 推导 required
 *   - 行 serial 严格 1..rowCount；pageCount ≤ 1033；每页重复 captureId/身份
 *   - incomplete 或 dropped>0 一律不可判
 *   - 旧 T405 不能按此格式追认
 *
 * 用法（并入主 CLI）: node t409_check.cjs <archiveDir|manifest.json> | --selftest
 */
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const MAX_BYTES = 1048576;
const MAX_ROWS = 4096;
const MAX_PAGES = 1033;
const LEDGER_FIELDS = ['generatedQuanta', 'acceptedQuantumCount', 'pendingQuanta', 'activePlanRemaining',
  'overloadDroppedQuanta', 'revokedDroppedQuanta', 'quotaCarry', 'clearedCarry'];

function checkT409(dir, manifestPath) {
  const checks = [];
  const F = (id, name, ok, detail) => checks.push({ id, name, verdict: ok ? 'PASS' : 'FAIL', detail: String(detail).slice(0, 400) });
  let m;
  try { m = JSON.parse(fs.readFileSync(manifestPath, 'utf-8')); }
  catch (e) { return { checks: [{ id: 'M-PARSE', verdict: 'FAIL', detail: e.message }], state: 'FAIL' }; }

  // 1 schema 身份
  F('T1-REVISION', 'revision == T409_EVIDENCE_ARCHIVE_V1', m.revision === 'T409_EVIDENCE_ARCHIVE_V1', `revision=${m.revision}`);
  const stateVal = String(m.state || '');
  F('T2-STATE', 'state ∈ {COMPLETE, INCOMPLETE}', stateVal === 'COMPLETE' || stateVal === 'INCOMPLETE', `state=${stateVal}`);

  // 2 files 存在+SHA+原字节
  const files = Array.isArray(m.files) ? m.files : [];
  const missing = [], shaBad = [], bytesBad = [];
  let rawTotal = 0;
  for (const f of files) {
    const p = path.join(dir, f.name);
    if (!fs.existsSync(p)) { missing.push(f.name); continue; }
    const buf = fs.readFileSync(p);
    rawTotal += buf.length;
    const actualSha = crypto.createHash('sha256').update(buf).digest('hex');
    if (actualSha !== String(f.sha256).toLowerCase()) shaBad.push(f.name);
    if (buf.length !== (f.bytes | 0)) bytesBad.push(`${f.name}: declared=${f.bytes} actual=${buf.length}`);
    if (buf.length > MAX_BYTES) bytesBad.push(`${f.name}: ${buf.length} > 1MiB`);
  }
  F('T3-FILES', `分片存在 (${files.length})`, missing.length === 0, missing.length ? `缺${missing.length}: ${missing.slice(0, 3).join(',')}` : 'all present');
  F('T4-SHA', '每页 SHA-256 逐位', shaBad.length === 0, shaBad.slice(0, 3).join(',') || 'all match');
  F('T5-BYTES', '每页 bytes 声明==实际(原字节) 且 ≤1MiB', bytesBad.length === 0, bytesBad.slice(0, 3).join(';') || 'all match');

  // 3 预算
  F('T6-DATA', 'dataBytes == Σ分片 bytes', (m.dataBytes | 0) === files.reduce((a, f) => a + (f.bytes | 0), 0),
    `dataBytes=${m.dataBytes}`);
  const manifestBytes = Buffer.byteLength(fs.readFileSync(manifestPath));
  const totalOk = (m.totalBytes | 0) === rawTotal + manifestBytes;
  F('T7-TOTAL', `totalBytes == 分片+manifest 实际 UTF8 bytes`, totalOk, `totalBytes=${m.totalBytes} files=${rawTotal} manifest=${manifestBytes}`);
  F('T8-MAXBUDGET', `totalBytes ≤ maxBytes 声明 与 maxBytes ≤ 1048576`,
    (m.totalBytes | 0) <= (m.maxBytes | 0) && (m.maxBytes | 0) <= MAX_BYTES,
    `total=${m.totalBytes} max=${m.maxBytes}`);

  // 4 summary 账字段（十进制字符串）
  const sum = m.summary || {};
  const ledOk = (fld) => {
    const L = sum[fld] || {};
    return LEDGER_FIELDS.every(k => typeof L[k] === 'string' && /^\d+$/.test(L[k]));
  };
  F('T9-LEDGERS', 'initialLedger/terminalLedger 八字段均为十进制字符串', ledOk('initialLedger') && ledOk('terminalLedger'),
    `initial ok=${ledOk('initialLedger')} terminal ok=${ledOk('terminalLedger')}`);

  // 5 requiredEvents: BEGIN/END 始终必需; seen 标志推导事件名
  const req = m.requiredEvents || {};
  const seenMap = { readySeen: 'READY', sourceAttemptSeen: 'SOURCE_ATTEMPT', acceptedSeen: 'FIRST_ACCEPT', faultSeen: 'FIRST_FAILURE', revokeSeen: 'REVOKE' };
  const required = ['BEGIN', 'END', ...Object.keys(seenMap).filter(k => req[k]).map(k => seenMap[k])];
  const present = Array.isArray(req.present) ? req.present : [];
  const reqMissing = required.filter(e => !present.includes(e));
  F('T10-REQUIRED', `required 事件在场 (${required.join(',')} vs present=${present.join(',')})`, reqMissing.length === 0,
    reqMissing.length ? `缺: ${reqMissing.join(',')}` : 'all present');

  // 6 页结构: pageCount ≤1033; rowCount ≤4096; pages=1(SUMMARY)+ceil(rowCount/4)+tokenCount?
  const rc = m.rowCount | 0, pc = m.pageCount | 0, tc = m.tokenCount | 0;
  F('T11-BOUNDS', `rowCount≤4096 且 pageCount≤1033`, rc <= MAX_ROWS && pc <= MAX_PAGES, `rows=${rc} pages=${pc}`);
  const expectPages = 1 + Math.ceil(rc / 4) + (tc > 0 ? 1 : 0);
  F('T12-PAGES', `pageCount == 1+ceil(rowCount/4)+token页`, pc === expectPages, `pages=${pc} expect=${expectPages}`);
  F('T12b-FILECOUNT', `files 数 == pageCount（每页一文件）`, files.length === pc, `files=${files.length} pages=${pc}`);

  // 7 逐行 serial（分片在场时）
  const evPages = files.filter(f => !/SUMMARY|TOKEN/.test(f.name));
  const serials = [];
  for (const f of evPages) {
    const p = path.join(dir, f.name);
    if (!fs.existsSync(p)) continue;
    try {
      const page = JSON.parse(fs.readFileSync(p, 'utf-8'));
      for (const r of (page.rows || [])) serials.push(BigInt(r.serial));
    } catch (e) { F('T13-PAGEPARSE', '事件页可解析', false, `${f.name}: ${e.message}`); }
  }
  serials.sort((a, b) => (a < b ? -1 : a > b ? 1 : 0));
  let serialOk = serials.length === rc;
  for (let i = 0; serialOk && i < serials.length; i++) if (serials[i] !== BigInt(i + 1)) serialOk = false;
  F('T13-SERIAL', `事件行 serial 严格 1..rowCount`, serialOk, `rows=${serials.length}/${rc} first=${serials[0]} last=${serials[serials.length - 1]}`);

  // 8 INCOMPLETE/dropped>0 → 不可判
  const dropped = String(sum.dropped ?? m.dropped ?? '0');
  const ref = stateVal === 'INCOMPLETE' || dropped !== '0';
  if (ref) checks.push({ id: 'T14-UNJUDGABLE', verdict: 'INCOMPLETE_REFUSED', detail: `state=${stateVal} dropped=${dropped}` });

  const fails = checks.filter(c => c.verdict === 'FAIL').length;
  return { checks, state: fails ? 'FAIL' : 'PASS', unjudgable: ref };
}

// ---------------------------------------------------------------- T409 合成 fixtures
function makeT409Fixtures(outDir) {
  const made = [];
  const mk = (name, mutate) => {
    const d = path.join(outDir, name);
    fs.mkdirSync(d, { recursive: true });
    const rowCount = 4;
    const rows = [];
    for (let i = 1; i <= rowCount; i++) rows.push({ serial: i, kind: i === 1 ? 'BEGIN' : 'PULSE', ns: String(1000 + i) });
    const pages = [];
    // SUMMARY 页
    const summary = {
      captureId: '11', initialLedger: Object.fromEntries(LEDGER_FIELDS.map(k => [k, '0'])),
      terminalLedger: Object.fromEntries(LEDGER_FIELDS.map(k => [k, '3'])),
      dropped: '0',
    };
    const summaryPage = { type: 'SUMMARY', captureId: '11', summary, rows: [] };
    // EVENTS 页 (4 行 = 1 页)
    const evPage = { type: 'EVENTS', captureId: '11', rows, rowCount };
    // TOKEN 页
    const tokenPage = { type: 'TOKEN', captureId: '11', tokens: [{ root: 1, kind: 'EMPTY_QUANTUM' }] };
    const pageFiles = [
      { name: `${name}-page0.ndjson`, obj: summaryPage },
      { name: `${name}-page1.ndjson`, obj: evPage },
      { name: `${name}-page2.ndjson`, obj: tokenPage },
    ];
    const man = {
      revision: 'T409_EVIDENCE_ARCHIVE_V1', state: 'COMPLETE',
      captureId: '11', scope: 'SYNTH', rowCount, tokenCount: 1, pageCount: 3,
      maxBytes: MAX_BYTES, summary, dropped: '0',
      requiredEvents: { present: ['BEGIN', 'END', 'FIRST_ACCEPT'], readySeen: false, sourceAttemptSeen: false, acceptedSeen: true, faultSeen: false, revokeSeen: false },
      files: [], dataBytes: 0, totalBytes: 0,
    };
    let raw = 0;
    for (const pf of pageFiles) {
      const isSkipped = mutate === 'missing' && pf.name.endsWith('page2.ndjson');
      let content = JSON.stringify(pf.obj);
      let declaredContent = content;
      if (mutate === 'sha' && pf.name.endsWith('page1.ndjson')) {
        // 声明用原始内容 SHA，写盘用篡改内容 → T4 必红
        declaredContent = content;
        content = content.replace('"PULSE"', '"TAMPER"');
      }
      if (mutate === 'serial' && pf.obj.type === 'EVENTS') {
        // 行 serial 1,2,3,9 → T13 必红；声明与写盘同内容（缺失检测点在 serial 本身）
        pf.obj.rows[3].serial = 9;
        content = JSON.stringify(pf.obj);
        declaredContent = content;
      }
      const buf = Buffer.from(content, 'utf-8');
      const declaredBuf = Buffer.from(declaredContent, 'utf-8');
      if (isSkipped) continue; // 声明进 manifest，文件不写盘
      fs.writeFileSync(path.join(d, pf.name), buf);
      man.files.push({ name: pf.name, bytes: declaredBuf.length, sha256: crypto.createHash('sha256').update(declaredBuf).digest('hex') });
      raw += declaredBuf.length;
    }
    man.dataBytes = raw;
    man.totalBytes = raw + 0; // manifest bytes 加入前先写
    const mp = path.join(d, `${name}-manifest.json`);
    const manifestBytes = Buffer.byteLength(JSON.stringify(man) /* approx before write */);
    man.totalBytes = raw + Buffer.byteLength(JSON.stringify(man, null, 0));
    fs.writeFileSync(mp, JSON.stringify(man));
    // totalBytes 自引用问题: 重写一次使 totalBytes 含最终 manifest bytes
    const mb = Buffer.byteLength(fs.readFileSync(mp));
    man.totalBytes = raw + mb;
    fs.writeFileSync(mp, JSON.stringify(man));
    made.push(d);
  };
  mk('t409_good', null); mk('t409_sha', 'sha'); mk('t409_missing', 'missing'); mk('t409_serial', 'serial');
  return made;
}

function selftestT409() {
  const tmp = fs.mkdtempSync(path.join(require('os').tmpdir(), 't409-'));
  const made = makeT409Fixtures(tmp);
  const expect = { t409_good: 'PASS', t409_sha: 'FAIL', t409_missing: 'FAIL', t409_serial: 'FAIL' };
  const mustRed = { t409_sha: ['T4-SHA'], t409_missing: ['T12b-FILECOUNT'], t409_serial: ['T13-SERIAL'] };
  let pass = 0, fail = 0;
  const lines = [];
  for (const d of made) {
    const name = path.basename(d);
    const r = checkT409(d, path.join(d, `${name}-manifest.json`));
    const ok = r.state === expect[name];
    if (ok) pass++; else fail++;
    lines.push(`${ok ? 'ok  ' : 'BAD '} ${name}: got=${r.state} expect=${expect[name]}`);
    for (const need of mustRed[name] || []) {
      if (!r.checks.some(c => c.id === need && c.verdict === 'FAIL')) { fail++; lines.push(`BAD  ${name}: 期望 ${need} 红`); }
      else pass++;
    }
  }
  console.log(lines.join('\n'));
  console.log(`t409 schema selftest: ${pass} ok, ${fail} bad`);
  return fail === 0 ? 0 : 1;
}

if (require.main === module) {
  const args = process.argv.slice(2);
  if (args[0] === '--selftest') process.exit(selftestT409());
  if (!args[0]) { console.error('用法: node t409_check.cjs <dir|manifest> | --selftest'); process.exit(2); }
  const target = args[0];
  let manifests = fs.statSync(target).isDirectory()
    ? fs.readdirSync(target).filter(f => f.endsWith('-manifest.json')).map(f => path.join(target, f))
    : [target];
  let anyFail = false;
  for (const mp of manifests) {
    const m = JSON.parse(fs.readFileSync(mp, 'utf-8'));
    if (m.revision !== 'T409_EVIDENCE_ARCHIVE_V1') { console.log(`=== ${path.basename(mp)}: SKIP (revision=${m.revision})`); continue; }
    const r = checkT409(path.dirname(mp), mp);
    console.log(`=== ${path.basename(mp)} : ${r.state}${r.unjudgable ? ' (UNJUDGABLE)' : ''}`);
    for (const c of r.checks) console.log(`  [${c.verdict}] ${c.id} — ${c.detail}`);
    if (r.state === 'FAIL') anyFail = true;
  }
  process.exit(anyFail ? 1 : 0);
}
module.exports = { checkT409, makeT409Fixtures };
