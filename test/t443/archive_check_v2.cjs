// T443 · GPU 归档检查器版本化修复（T441 §4–5 合同）。
// checkerRevision: T441_ARCHIVE_CHECK_V2，输入协议 T409_EVIDENCE_ARCHIVE_V1 / T409_EVIDENCE_CAPTURE_PAGE_V1。
// 独立实现：不调用产品断言作唯一 oracle，不修改原档/生产协议；旧 T306 timeline 由旧 parser 保留。
//
// 用法: node test/t443/archive_check_v2.cjs <dir|manifest> [--selftest]
// 退出码: 0=ARCHIVE_INTEGRITY_PASS；1=FORMAT_OR_INTEGRITY_FAIL / UNJUDGABLE / INPUT_ERROR。
'use strict';
const fs = require('node:fs'), path = require('node:path'), crypto = require('node:crypto'), os = require('node:os');

const KNOWN_KINDS = new Set(['BEGIN', 'READY', 'SOURCE_ATTEMPT', 'FIRST_OFFER', 'FIRST_ACCEPT',
  'FIRST_FAILURE', 'REVOKE', 'END', 'GL_DETAIL']);
const LEDGER_FIELDS = ['generatedQuanta', 'acceptedQuantumCount', 'pendingQuanta', 'activePlanRemaining',
  'overloadDroppedQuanta', 'revokedDroppedQuanta', 'quotaCarry', 'clearedCarry'];
const U64_RE = /^(0|[1-9][0-9]{0,19})$/;
const SHA_RE = /^[0-9a-f]{64}$/;

class Fail { constructor(code, where, detail) { this.code = code; this.where = where; this.detail = detail; } }
function err(code, where, detail) { throw new Fail(code, where, detail || ''); }
function isU64(v) {
  if (typeof v === 'string') return U64_RE.test(v);
  return Number.isSafeInteger(v) && v >= 0;          // 真机档计数为数字；禁 NaN/负/溢出
}
function u64Le(v, max) { return isU64(v) && BigInt(v) <= BigInt(max); }
function canonU64(v) { return isU64(v) && (typeof v === 'number' || !/^0[0-9]/.test(v)); }

// ---------- §4.1 原文件、页序和 scope ----------
function checkArchive(manifestPath) {
  const fails = [];
  const add = (code, where, detail) => fails.push({ code, where, detail: detail || '' });
  const dir = path.dirname(path.resolve(manifestPath));
  let m;
  try { m = JSON.parse(fs.readFileSync(manifestPath, 'utf8')); }
  catch (e) { return { revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'INPUT_ERROR',
    fails: [{ code: 'INPUT_ERROR', where: 'manifest', detail: String(e).slice(0, 120) }] }; }

  const inputRev = m.revision;
  if (inputRev !== 'T409_EVIDENCE_ARCHIVE_V1')
    return { revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'UNSUPPORTED_INPUT', fails: [{ code: 'UNSUPPORTED_REVISION', where: 'manifest.revision', detail: String(inputRev) }] };

  try {
    // T1: manifest 必填结构
    for (const k of ['state', 'captureId', 'activeRevision', 'packageSha256', 'epoch', 'pageSession',
                     'startNs', 'endNs', 'rowCount', 'tokenCount', 'pageCount', 'files', 'summary'])
      if (m[k] === undefined) err('MANIFEST_FIELD_MISSING', 'manifest.' + k);
    if (!SHA_RE.test(m.packageSha256)) err('BAD_PACKAGE_SHA', 'manifest.packageSha256');
    if (m.state !== 'COMPLETE' && m.state !== 'INCOMPLETE') err('BAD_STATE', 'manifest.state');
    const incomplete = m.state !== 'COMPLETE';

    // T2: rowCount/tokenCount/pageCount 范围与公式（正式: 1+ceil(rowCount/4)+tokenCount）
    if (!u64Le(m.rowCount, 4096) || BigInt(m.rowCount) < 2n) err('ROW_COUNT_RANGE', 'manifest.rowCount', String(m.rowCount));
    if (!u64Le(m.tokenCount, 8)) err('TOKEN_COUNT_RANGE', 'manifest.tokenCount', String(m.tokenCount));
    const pagesFormula = 1n + (BigInt(m.rowCount) + 3n) / 4n + BigInt(m.tokenCount);
    if (BigInt(m.pageCount) !== pagesFormula) err('PAGE_COUNT_FORMULA', 'manifest.pageCount',
      `${m.pageCount} != 1+ceil(${m.rowCount}/4)+${m.tokenCount}=${pagesFormula}`);
    if (BigInt(m.pageCount) > 1033n) err('PAGE_COUNT_LIMIT', 'manifest.pageCount');

    // T3: files 引用唯一/存在/bytes 整数/SHA 64hex 与原字节一致；同目录
    if (!Array.isArray(m.files) || m.files.length !== Number(m.pageCount)) err('FILES_COUNT', 'manifest.files');
    const seenNames = new Set();
    const pageBuffers = [];
    for (let i = 0; i < m.files.length; i++) {
      const f = m.files[i];
      if (!f || typeof f.name !== 'string') err('FILE_REF_BAD', `files[${i}]`);
      if (f.name.includes('/') || f.name.includes('\\')) err('FILE_PATH_ESCAPE', `files[${i}]`, f.name);
      if (seenNames.has(f.name)) err('FILE_REF_DUPLICATE', `files[${i}]`, f.name);
      seenNames.add(f.name);
      if (!Number.isSafeInteger(f.bytes) || f.bytes < 0) err('FILE_BYTES_NOT_INT', `files[${i}].bytes`, String(f.bytes));
      if (!SHA_RE.test(f.sha256)) err('FILE_SHA_FORMAT', `files[${i}].sha256`);
      const p = path.join(dir, f.name);
      if (!fs.existsSync(p)) err('FILE_MISSING', `files[${i}]`, f.name);
      const buf = fs.readFileSync(p);
      if (buf.length !== f.bytes) err('FILE_BYTES_MISMATCH', f.name, `declared ${f.bytes} actual ${buf.length}`);
      const sha = crypto.createHash('sha256').update(buf).digest('hex');
      if (sha !== f.sha256) err('FILE_SHA_MISMATCH', f.name, `declared ${f.sha256.slice(0, 12)} actual ${sha.slice(0, 12)}`);
      pageBuffers.push(JSON.parse(buf.toString('utf8')));
    }

    // T4: dataBytes=页字节和；totalBytes=页字节+manifest 原 UTF8 字节；预算上限
    const declaredData = m.dataBytes !== undefined ? m.dataBytes : m.files.reduce((a, f) => a + f.bytes, 0);
    const actualData = m.files.reduce((a, f) => a + f.bytes, 0);
    if (BigInt(declaredData) !== BigInt(actualData)) err('DATA_BYTES_MISMATCH', 'manifest.dataBytes');
    const manifestBytes = fs.readFileSync(manifestPath).length;
    if (m.totalBytes !== undefined && BigInt(m.totalBytes) !== BigInt(actualData + manifestBytes))
      err('TOTAL_BYTES_MISMATCH', 'manifest.totalBytes', `${m.totalBytes} != ${actualData + manifestBytes}`);
    if (actualData + manifestBytes > 1048576) err('BUDGET_LIMIT', 'manifest.totalBytes');
    for (const f of m.files) if (f.bytes > 1048576) err('BUDGET_LIMIT', f.name);

    // T5: 页序——cursor 唯一定位 0..pageCount-1，next=cursor+1，complete 仅末页；
    // 第0页 SUMMARY，之后 ceil(rowCount/4) 个 EVENTS，之后 tokenCount 个 TOKEN
    const expectTypes = ['SUMMARY'];
    const eventsPages = Number((BigInt(m.rowCount) + 3n) / 4n);
    for (let i = 0; i < eventsPages; i++) expectTypes.push('EVENTS');
    for (let i = 0; i < Number(m.tokenCount); i++) expectTypes.push('TOKEN');
    const cursors = new Set();
    pageBuffers.forEach((pg, idx) => {
      if (!Number.isInteger(pg.cursor) || pg.cursor < 0 || pg.cursor >= Number(m.pageCount))
        err('PAGE_CURSOR_RANGE', `page[${idx}]`, String(pg.cursor));
      if (cursors.has(pg.cursor)) err('PAGE_CURSOR_DUPLICATE', `page[${idx}]`, String(pg.cursor));
      cursors.add(pg.cursor);
      if (pg.next !== undefined && pg.cursor < Number(m.pageCount) - 1 && pg.next !== pg.cursor + 1)
        err('PAGE_NEXT_CHAIN', `page[${pg.cursor}]`, String(pg.next));
      if (pg.complete === true && pg.cursor !== Number(m.pageCount) - 1)
        err('PAGE_COMPLETE_PREMATURE', `page[${pg.cursor}]`);
      const expect = expectTypes[pg.cursor];
      if (pg.type !== expect) err('PAGE_TYPE_ORDER', `page[${pg.cursor}]`, `expect ${expect} got ${pg.type}`);
      if (pg.revision !== 'T409_EVIDENCE_CAPTURE_PAGE_V1') err('PAGE_REVISION', `page[${pg.cursor}]`, String(pg.revision));
    });
    if (cursors.size !== Number(m.pageCount)) err('PAGE_CURSOR_MISSING', 'pages', `have ${cursors.size} of ${m.pageCount}`);

    // T6: 每页 scope 与 manifest 一致
    for (const pg of pageBuffers) {
      for (const k of ['captureId', 'activeRevision', 'packageSha256', 'epoch', 'runLo', 'runHi', 'pageSession'])
        if (JSON.stringify(pg[k]) !== JSON.stringify(m[k])) err('SCOPE_MISMATCH', `page[${pg.cursor}].${k}`, `${JSON.stringify(pg[k])} vs manifest ${JSON.stringify(m[k])}`);
      if (pg.state !== 'FROZEN') err('PAGE_STATE', `page[${pg.cursor}]`, String(pg.state));
      for (const k of ['rowCount', 'tokenCount', 'pageCount'])
        if (JSON.stringify(pg[k]) !== JSON.stringify(m[k])) err('PAGE_COUNT_MISMATCH', `page[${pg.cursor}].${k}`);
      if (pg.incomplete) add('PAGE_INCOMPLETE_FLAG', `page[${pg.cursor}]`);
      if (pg.reason !== undefined && pg.reason === 'INCOMPLETE') add('PAGE_INCOMPLETE_REASON', `page[${pg.cursor}]`);
    }

    // T7: SUMMARY 与 manifest.summary 逐字段相等
    const sm = pageBuffers[0];
    if (JSON.stringify(sm.summary) !== JSON.stringify(m.summary))
      err('SUMMARY_TAMPERED', 'page[0].summary vs manifest.summary');

    // T8: SUMMARY 时钟域与 manifest 对应（nativeClock→page.clock）
    if (m.nativeClock && sm.clock && sm.clock !== m.nativeClock) err('CLOCK_DOMAIN', 'page[0].clock', `${sm.clock} vs manifest ${m.nativeClock}`);

    // T9: 事件页扫描——serial 严格 1..rowCount 原始存储顺序（不排序修复）
    const eventRows = [];
    for (const pg of pageBuffers) {
      if (pg.type !== 'EVENTS') continue;
      for (const r of pg.rows || []) eventRows.push({ pg: pg.cursor, r });
    }
    if (eventRows.length !== Number(m.rowCount)) err('EVENT_ROWS_COUNT', 'events', `${eventRows.length} != rowCount ${m.rowCount}`);
    let firstLedger = null, lastLedger = null, firstKind = null, lastKind = null;
    eventRows.forEach((e, i) => {
      const r = e.r;
      if (!canonU64(r.serial) || BigInt(r.serial) !== BigInt(i + 1))
        err('SERIAL_SEQUENCE', `page[${e.pg}] row#${i}`, `serial=${r.serial} expect ${i + 1}`);
      if (r.captureId !== m.captureId) err('ROW_CAPTURE_SCOPE', `page[${e.pg}] row#${i}`);
      if (!KNOWN_KINDS.has(r.kind)) err('UNKNOWN_KIND', `page[${e.pg}] row#${i}`, String(r.kind));
      for (const [k, v] of Object.entries(r.ledger || {}))
        if (!canonU64(v)) err('LEDGER_U64', `page[${e.pg}] row#${i}.${k}`, String(v));
      if (i === 0) { firstKind = r.kind; firstLedger = r.ledger; }
      lastKind = r.kind; lastLedger = r.ledger;
    });
    if (firstKind !== 'BEGIN') err('FIRST_NOT_BEGIN', 'events', String(firstKind));
    if (lastKind !== 'END') err('LAST_NOT_END', 'events', String(lastKind));
    const beginCount = eventRows.filter(e => e.r.kind === 'BEGIN').length;
    const endCount = eventRows.filter(e => e.r.kind === 'END').length;
    if (beginCount !== 1) err('BEGIN_UNIQUENESS', 'events', String(beginCount));
    if (endCount !== 1) err('END_UNIQUENESS', 'events', String(endCount));

    // T10: 首末账（BEGIN账=initialLedger、END账=terminalLedger，逐字段）
    for (const f of LEDGER_FIELDS) {
      if (JSON.stringify((firstLedger || {})[f]) !== JSON.stringify((m.summary.initialLedger || {})[f]))
        err('BEGIN_LEDGER_MISMATCH', `events.begin.${f}`, `${(firstLedger || {})[f]} vs initial ${(m.summary.initialLedger || {})[f]}`);
      if (JSON.stringify((lastLedger || {})[f]) !== JSON.stringify((m.summary.terminalLedger || {})[f]))
        err('END_LEDGER_MISMATCH', `events.end.${f}`, `${(lastLedger || {})[f]} vs terminal ${(m.summary.terminalLedger || {})[f]}`);
    }

    // T11: requiredEvents 必须是 string[]（已知 kind、无重复、含 BEGIN/END）
    if (!Array.isArray(m.requiredEvents)) err('REQUIRED_NOT_ARRAY', 'manifest.requiredEvents', typeof m.requiredEvents);
    else {
      const seen = new Set();
      for (const k of m.requiredEvents) {
        if (typeof k !== 'string') err('REQUIRED_NOT_STRING', 'manifest.requiredEvents', String(k));
        if (!KNOWN_KINDS.has(k)) err('REQUIRED_UNKNOWN_KIND', 'manifest.requiredEvents', k);
        if (seen.has(k)) err('REQUIRED_DUPLICATE', 'manifest.requiredEvents', k);
        seen.add(k);
      }
      for (const k of ['BEGIN', 'END']) if (!seen.has(k)) err('REQUIRED_MISSING_BASE', 'manifest.requiredEvents', k);
    }

    // T12: 独立推导 required 集合 R（T441 §4.2）并核实际事件在场 + 反查一致
    const s = m.summary;
    const kindsPresent = new Set(eventRows.map(e => e.r.kind));
    const R = new Set(['BEGIN', 'END']);
    const flagErr = (cond, code, detail) => { if (cond) { R.add(code.replace('FLAG_', '')); } };
    if (s.readySeen === true) R.add('READY');
    if (s.readySeen === true && !kindsPresent.has('READY')) err('REQUIRED_EVENT_MISSING', 'events', 'READY seen=true');
    if (s.readySeen === false && kindsPresent.has('READY')) err('FLAG_EVENT_CONTRADICTION', 'summary.readySeen', 'flag false but READY present');
    if (s.sourceAttemptSeen === true) {
      R.add('SOURCE_ATTEMPT');
      if (!kindsPresent.has('SOURCE_ATTEMPT')) err('REQUIRED_EVENT_MISSING', 'events', 'SOURCE_ATTEMPT seen=true');
    }
    if (s.sourceAttemptSeen === false && kindsPresent.has('SOURCE_ATTEMPT')) err('FLAG_EVENT_CONTRADICTION', 'summary.sourceAttemptSeen');
    // offerCount>0 的相各需一条 FIRST_OFFER
    if (Array.isArray(s.sourceOfferCount)) {
      for (const ph of [0, 1]) {
        const c = s.sourceOfferCount[ph];
        if (isU64(c) && BigInt(c) > 0n) {
          R.add('FIRST_OFFER');
          const n = eventRows.filter(e => e.r.kind === 'FIRST_OFFER' && e.r.phase === ph).length;
          if (n === 0) err('REQUIRED_EVENT_MISSING', 'events', `FIRST_OFFER phase${ph} offerCount=${c}`);
        }
      }
      if (Array.isArray(s.sourceEnqueuedCount))
        for (const ph of [0, 1])
          if (isU64(s.sourceEnqueuedCount[ph]) && isU64(s.sourceOfferCount[ph]) &&
              BigInt(s.sourceEnqueuedCount[ph]) > BigInt(s.sourceOfferCount[ph]))
            err('ENQUEUED_OVER_OFFER', `summary.phase${ph}`);
    }
    if (s.sourceOfferSeen === true && !kindsPresent.has('FIRST_OFFER')) err('REQUIRED_EVENT_MISSING', 'events', 'FIRST_OFFER seen=true');
    if (s.sourceOfferSeen === false && kindsPresent.has('FIRST_OFFER')) err('FLAG_EVENT_CONTRADICTION', 'summary.sourceOfferSeen');
    if (s.acceptedSeen === true) {
      R.add('FIRST_ACCEPT');
      if (!kindsPresent.has('FIRST_ACCEPT')) err('REQUIRED_EVENT_MISSING', 'events', 'FIRST_ACCEPT seen=true');
      if (BigInt(m.tokenCount) === 0n) err('ACCEPT_WITHOUT_TOKEN', 'manifest.tokenCount');
    }
    if (s.acceptedSeen === false && kindsPresent.has('FIRST_ACCEPT')) err('FLAG_EVENT_CONTRADICTION', 'summary.acceptedSeen');
    if (BigInt(m.tokenCount) > 0n && s.acceptedSeen !== true) err('TOKEN_WITHOUT_ACCEPT', 'manifest.tokenCount');
    if (s.faultSeen === true) {
      R.add('FIRST_FAILURE');
      if (!kindsPresent.has('FIRST_FAILURE')) err('REQUIRED_EVENT_MISSING', 'events', 'FIRST_FAILURE seen=true');
    }
    if (s.faultSeen === false && kindsPresent.has('FIRST_FAILURE')) err('FLAG_EVENT_CONTRADICTION', 'summary.faultSeen');
    if (s.revokeSeen === true) {
      R.add('REVOKE');
      if (!kindsPresent.has('REVOKE')) err('REQUIRED_EVENT_MISSING', 'events', 'REVOKE seen=true');
    }
    if (s.revokeSeen === false && kindsPresent.has('REVOKE')) err('FLAG_EVENT_CONTRADICTION', 'summary.revokeSeen');

    // manifest.requiredEvents 必须与推导的 R 一致
    if (Array.isArray(m.requiredEvents)) {
      const declared = new Set(m.requiredEvents);
      for (const k of R) if (!declared.has(k)) err('REQUIRED_UNDER_DECLARED', 'manifest.requiredEvents', `derived ${k} not declared`);
      for (const k of declared) if (!R.has(k)) err('REQUIRED_OVER_DECLARED', 'manifest.requiredEvents', `declared ${k} not derived`);
    }

    // T13: 声明代替真实行——旧夹具式 present/readySeen 伪声明已被 T12 反查拦截

    // T14: TOKEN 页
    const tokenPages = pageBuffers.filter(pg => pg.type === 'TOKEN');
    if (tokenPages.length !== Number(m.tokenCount)) err('TOKEN_PAGES_COUNT', 'pages', `${tokenPages.length} != ${m.tokenCount}`);
    for (const pg of tokenPages) {
      const t = pg.token;
      if (!t || typeof t !== 'object') err('TOKEN_PAGE_BAD', `page[${pg.cursor}]`);
      if (t.kind !== undefined && !['ATTEMPTED', 'NOT_ATTEMPTED'].includes(t.kind)) err('TOKEN_KIND', `page[${pg.cursor}]`, String(t.kind));
    }

    // T15: dropped / incomplete 判定
    if (BigInt(s.dropped || '0') > 0n) add('DROPPED_ROWS', 'summary.dropped', String(s.dropped));
    const unjudgable = incomplete || fails.some(f => f.code.startsWith('PAGE_INCOMPLETE') || f.code === 'DROPPED_ROWS');
    if (unjudgable)
      return { revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'UNJUDGABLE', fails, activity: activityOf(m, kindsPresent) };
    if (fails.length) return { revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'FORMAT_OR_INTEGRITY_FAIL', fails };
    return { revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'ARCHIVE_INTEGRITY_PASS', fails: [],
      activity: activityOf(m, kindsPresent) };
  } catch (e) {
    if (e instanceof Fail) return { revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'FORMAT_OR_INTEGRITY_FAIL',
      fails: [{ code: e.code, where: e.where, detail: e.detail }] };
    return { revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'INPUT_ERROR', fails: [{ code: 'INPUT_ERROR', where: 'parse', detail: String(e).slice(0, 200) }] };
  }
}

function activityOf(m, kinds) {
  return {
    zeroAccept: !kinds.has('FIRST_ACCEPT') && m.tokenCount === '0' || m.tokenCount === 0,
    noToken: m.tokenCount === '0' || m.tokenCount === 0,
    idleWindowOnly: kinds.size === 2 && kinds.has('BEGIN') && kinds.has('END'),
    exercisedKinds: [...kinds].sort(),
    note: '空闲窗不产生双源或 30fps 通过；NOT_EXERCISED 活动需生产观测路径补证',
  };
}

// ---------- CLI ----------
function main() {
  const args = process.argv.slice(2);
  if (args[0] === '--selftest') process.exit(selftest());
  if (!args[0]) { console.error('用法: node archive_check_v2.cjs <dir|manifest> | --selftest'); process.exit(2); }
  const target = args[0];
  let manifestPath = target;
  if (fs.statSync(target).isDirectory()) {
    const cands = fs.readdirSync(target).filter(f => f.endsWith('-manifest.json'));
    if (cands.length !== 1) { console.error(JSON.stringify({ revision: 'T441_ARCHIVE_CHECK_V2', verdict: 'INPUT_ERROR', fails: [{ code: 'INPUT_ERROR', where: 'dir', detail: `expect 1 manifest, found ${cands.length}` }] })); process.exit(2); }
    manifestPath = path.join(target, cands[0]);
  }
  const result = checkArchive(manifestPath);
  console.log(JSON.stringify(result));
  process.exit(result.verdict === 'ARCHIVE_INTEGRITY_PASS' ? 0 : 1);
}

// ---------- selftest（T441 §5 fixture 矩阵，全部 SYNTHETIC_PROTOCOL） ----------
function selftest() {
  const results = [];
  const mk = (name, mutate) => {
    const base = fs.mkdtempSync(path.join(os.tmpdir(), 't443-base-'));
    const mp0 = baseManifest();   // 返回 base 目录中的 manifest 路径
    const m = JSON.parse(fs.readFileSync(mp0, 'utf8'));
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 't443-'));
    // 把 base 的页文件复制过来
    for (const f of m.files) fs.copyFileSync(path.join(path.dirname(mp0), f.name), path.join(dir, f.name));
    mutate(m, dir);
    const mp = path.join(dir, 'gpu-evidence-x-manifest.json');
    fs.writeFileSync(mp, JSON.stringify(m));
    const r = checkArchive(mp);
    results.push({ fixture: name, verdict: r.verdict, codes: r.fails.map(f => f.code) });
    return r;
  };
  const syncPages = (dir, m) => {
    for (const f of m.files) {
      const p = path.join(dir, f.name);
      const buf = Buffer.from(fs.readFileSync(p));
      f.bytes = buf.length;
      f.sha256 = crypto.createHash('sha256').update(buf).digest('hex');
    }
    m.dataBytes = m.files.reduce((a, f) => a + f.bytes, 0);
    return rewriteManifest(dir, m);
  };
  const rewriteManifest = (dir, m) => {
    const mp = path.join(dir, 'gpu-evidence-x-manifest.json');
    let total = 0;
    for (let i = 0; i < 4; i++) {
      m.totalBytes = m.files.reduce((a, f) => a + f.bytes, 0) + total;
      fs.writeFileSync(mp, JSON.stringify(m));
      total = fs.readFileSync(mp).length;
    }
    return mp;
  };
  const baseManifest = () => {
    // 最小合法档：rowCount2 BEGIN/END，pageCount2（自建 base 目录，返回 manifest 路径）
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 't443-base-'));
    const rows = [
      { serial: '1', kind: 'BEGIN', captureId: '9', ns: '100', ledger: ledger(0) },
      { serial: '2', kind: 'END', captureId: '9', ns: '200', ledger: ledger(0) },
    ];
    const summary = baseSummary();
    const page0 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'SUMMARY', cursor: 0, next: 1, complete: false, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, summary, clock: 'STEADY_NS' };
    const page1 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'EVENTS', cursor: 1, complete: true, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, rows };
    return writeArchive(dir, '9', summary, [page0, page1]);
  };
  const ledger = (n) => Object.fromEntries(LEDGER_FIELDS.map(f => [f, String(n)]));
  const baseSummary = () => ({ dropped: '0', suppressedDetailedRows: '0',
    readySeen: false, sourceOfferSeen: false, sourceOfferCount: ['0', '0'],
    sourceEnqueuedCount: ['0', '0'], sourceAttemptSeen: false, acceptedSeen: false,
    faultSeen: false, revokeSeen: false,
    initialLedger: ledger(0), terminalLedger: ledger(0) });
  const writeArchive = (dir, captureId, summary, pages) => {
    const files = [];
    pages.forEach((pg, i) => {
      const buf = Buffer.from(JSON.stringify(pg));
      const name = `page${i}.json`;
      fs.writeFileSync(path.join(dir, name), buf);
      files.push({ name, bytes: buf.length, sha256: crypto.createHash('sha256').update(buf).digest('hex') });
    });
    const manifest = { revision: 'T409_EVIDENCE_ARCHIVE_V1', state: 'COMPLETE', reason: 'SYNTHETIC',
      captureId, activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64),
      epoch: '2', runLo: 1, runHi: 0, pageSession: '1', startNs: '100', endNs: '200',
      rowCount: 2, tokenCount: 0, pageCount: pages.length,
      dataBytes: files.reduce((a, f) => a + f.bytes, 0), totalBytes: files.reduce((a, f) => a + f.bytes, 0) + 10,
      files, summary, requiredEvents: ['BEGIN', 'END'] };
    const mp = path.join(dir, 'gpu-evidence-x-manifest.json');
    // totalBytes 自引用：迭代两次收敛（长度变化趋于稳定）
    let total = 0;
    for (let i = 0; i < 4; i++) {
      manifest.totalBytes = files.reduce((a, f) => a + f.bytes, 0) + total;
      fs.writeFileSync(mp, JSON.stringify(manifest));
      total = fs.readFileSync(mp).length;
    }
    return mp;
  };

  // P0 正例
  {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 't443-p0-'));
    const rows = [
      { serial: '1', kind: 'BEGIN', captureId: '9', ns: '100', ledger: ledger(0) },
      { serial: '2', kind: 'END', captureId: '9', ns: '200', ledger: ledger(0) },
    ];
    const summary = baseSummary();
    const page0 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'SUMMARY', cursor: 0, next: 1, complete: false, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, summary, clock: 'STEADY_NS' };
    const page1 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'EVENTS', cursor: 1, complete: true, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, rows };
    const mp = writeArchive(dir, '9', summary, [page0, page1]);
    const r = checkArchive(mp);
    results.push({ fixture: 'P0_VALID_IDLE', verdict: r.verdict, codes: r.fails.map(f => f.code) });
    if (r.verdict !== 'ARCHIVE_INTEGRITY_PASS') process.exitCode = 1;
  }
  // N1 required 数组当对象读（旧 T10 误报路径）→ 现在对象直接拒
  mk('N1_REQUIRED_OBJECT', (m, dir) => { m.requiredEvents = { present: ['BEGIN', 'END'], readySeen: false }; rewriteManifest(dir, m); });
  // N2 声明含 FIRST_ACCEPT 但实际缺行（声明代真行）
  {
    // N2: required/summary 声称 FIRST_ACCEPT+token，但 EVENTS 无该行——文件门合法后事件门拒
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 't443-n2-'));
    const mp0 = baseManifest();
    const m = JSON.parse(fs.readFileSync(mp0, 'utf8'));
    for (const f of m.files) fs.copyFileSync(path.join(path.dirname(mp0), f.name), path.join(dir, f.name));
    const p0p = path.join(dir, m.files[0].name);
    const page0 = JSON.parse(fs.readFileSync(p0p, 'utf8'));
    m.summary.acceptedSeen = true;
    page0.summary = JSON.parse(JSON.stringify(m.summary));   // T7 同步
    fs.writeFileSync(p0p, JSON.stringify(page0));
    m.requiredEvents = ['BEGIN', 'END', 'FIRST_ACCEPT'];
    syncPages(dir, m);
    const r = checkArchive(path.join(dir, 'gpu-evidence-x-manifest.json'));
    results.push({ fixture: 'N2_DECLARED_NOT_PRESENT', verdict: r.verdict, codes: r.fails.map(f => f.code) });
    if (!r.fails.some(f => f.code.startsWith('REQUIRED_EVENT_MISSING') || f.code.startsWith('ACCEPT_WITHOUT_TOKEN') || f.code.startsWith('REQUIRED_'))) process.exitCode = 1;
  }
  // N3 seen=true 但事件缺失（重算哈希合法文件后仍拒）
  {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 't443-n3-'));
    const rows = [
      { serial: '1', kind: 'BEGIN', captureId: '9', ns: '100', ledger: ledger(0) },
      { serial: '2', kind: 'END', captureId: '9', ns: '200', ledger: ledger(0) },
    ];
    const summary = baseSummary(); summary.readySeen = true;   // flag 声称 READY 但无行
    const page0 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'SUMMARY', cursor: 0, next: 1, complete: false, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, summary, clock: 'STEADY_NS' };
    const page1 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'EVENTS', cursor: 1, complete: true, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, rows };
    const mp = writeArchive(dir, '9', summary, [page0, page1]);
    const m = JSON.parse(fs.readFileSync(mp, 'utf8'));
    m.requiredEvents = ['BEGIN', 'READY', 'END'];
    // 重算 manifest 字节（不重排 serial）
    fs.writeFileSync(mp, JSON.stringify(m));
    const mb = fs.readFileSync(mp).length;
    m.totalBytes = m.files.reduce((a, f) => a + f.bytes, 0) + mb;
    fs.writeFileSync(mp, JSON.stringify(m));
    const r = checkArchive(mp);
    results.push({ fixture: 'N3_FLAG_TRUE_EVENT_MISSING', verdict: r.verdict, codes: r.fails.map(f => f.code) });
    if (!r.fails.some(f => f.code === 'REQUIRED_EVENT_MISSING')) process.exitCode = 1;
  }
  // N4 flag=false 但事件在场（反查一致）
  {
    // N4: flag=false 但 READY 行真实在场（反查一致门）——构造含 READY 的完整合法档
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 't443-n4-'));
    const rows = [
      { serial: '1', kind: 'BEGIN', captureId: '9', ns: '100', ledger: ledger(0) },
      { serial: '2', kind: 'READY', captureId: '9', ns: '150', ledger: ledger(0) },
      { serial: '3', kind: 'END', captureId: '9', ns: '200', ledger: ledger(0) },
    ];
    const summary = baseSummary();
    summary.readySeen = false;                      // flag 与实际矛盾
    const page0 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'SUMMARY', cursor: 0, next: 1, complete: false, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 3, tokenCount: 0, pageCount: 2, summary, clock: 'STEADY_NS' };
    const page1 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'EVENTS', cursor: 1, complete: true, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 3, tokenCount: 0, pageCount: 2, rows };
    // manifest 与页同步 rowCount=3（writeArchive 写死 2——此处手写 rowCount 3 的 manifest）
    const files = [];
    [page0, page1].forEach((pg, i) => {
      const buf = Buffer.from(JSON.stringify(pg));
      const name = `page${i}.json`;
      fs.writeFileSync(path.join(dir, name), buf);
      files.push({ name, bytes: buf.length, sha256: crypto.createHash('sha256').update(buf).digest('hex') });
    });
    const manifest = { revision: 'T409_EVIDENCE_ARCHIVE_V1', state: 'COMPLETE', reason: 'SYNTHETIC',
      captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64),
      epoch: '2', runLo: 1, runHi: 0, pageSession: '1', startNs: '100', endNs: '200',
      rowCount: 3, tokenCount: 0, pageCount: 2,
      dataBytes: files.reduce((a, f) => a + f.bytes, 0), totalBytes: 0,
      files, summary, requiredEvents: ['BEGIN', 'END'] };
    const mp = path.join(dir, 'gpu-evidence-x-manifest.json');
    let total = 0;
    for (let i = 0; i < 4; i++) {
      manifest.totalBytes = files.reduce((a, f) => a + f.bytes, 0) + total;
      fs.writeFileSync(mp, JSON.stringify(manifest));
      total = fs.readFileSync(mp).length;
    }
    const r = checkArchive(mp);
    results.push({ fixture: 'N4_FLAG_FALSE_EVENT_PRESENT', verdict: r.verdict, codes: r.fails.map(f => f.code) });
    if (!r.fails.some(f => f.code === 'FLAG_EVENT_CONTRADICTION')) process.exitCode = 1;
  }
  // N5 INCOMPLETE → UNJUDGABLE 非成功退出
  mk('N5_INCOMPLETE_UNJUDGABLE', (m, dir) => { m.state = 'INCOMPLETE'; rewriteManifest(dir, m); });
  // N6 页数公式（tokenCount=1 时 pageCount 应 1+1+1=3）
  mk('N6_PAGE_COUNT_FORMULA', (m) => { m.tokenCount = '1'; });   // pageCount 仍 2 → 拒
  // N7 未知 revision
  mk('N7_UNKNOWN_REVISION', (m) => { m.revision = 'T999_FUTURE_V9'; });
  // N8 END 账≠terminal
  {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 't443-n8-'));
    const rows = [
      { serial: '1', kind: 'BEGIN', captureId: '9', ns: '100', ledger: ledger(0) },
      { serial: '2', kind: 'END', captureId: '9', ns: '200', ledger: ledger(3) },
    ];
    const summary = baseSummary();
    const page0 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'SUMMARY', cursor: 0, next: 1, complete: false, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, summary, clock: 'STEADY_NS' };
    const page1 = { revision: 'T409_EVIDENCE_CAPTURE_PAGE_V1', type: 'EVENTS', cursor: 1, complete: true, state: 'FROZEN', captureId: '9', activeRevision: 'GRS_M1_PROGRESS_CLOCK_V1', packageSha256: 'a'.repeat(64), epoch: '2', runLo: 1, runHi: 0, pageSession: '1', rowCount: 2, tokenCount: 0, pageCount: 2, rows };
    const mp = writeArchive(dir, '9', summary, [page0, page1]);
    const r = checkArchive(mp);
    results.push({ fixture: 'N8_END_LEDGER_MISMATCH', verdict: r.verdict, codes: r.fails.map(f => f.code) });
    if (!r.fails.some(f => f.code === 'END_LEDGER_MISMATCH')) process.exitCode = 1;
  }
  // N9 串 scope（页 captureId 与 manifest 不同）
  mk('N9_SCOPE_MISMATCH', (m, dir) => {
    const p = path.join(dir, m.files[0].name);
    const pg = JSON.parse(fs.readFileSync(p, 'utf8'));
    pg.captureId = 'ZZZ';
    const buf = Buffer.from(JSON.stringify(pg));
    fs.writeFileSync(p, buf);
    m.files[0].bytes = buf.length;
    m.files[0].sha256 = crypto.createHash('sha256').update(buf).digest('hex');
    m.dataBytes = m.files.reduce((a, f) => a + f.bytes, 0);
    rewriteManifest(dir, m);
  });
  // N10 文件 SHA 篡改
  mk('N10_FILE_SHA_TAMPER', (m, dir) => {
    const p = path.join(dir, m.files[1].name);
    const pg = JSON.parse(fs.readFileSync(p, 'utf8'));
    pg.rows[0].ns = '999';
    fs.writeFileSync(p, JSON.stringify(pg));   // 未重算 SHA → 文件门拒
  });
  // N11 空目录/未知输入 → 非成功
  {
    const empty = fs.mkdtempSync(path.join(os.tmpdir(), 't443-empty-'));
    const r = checkArchive(path.join(empty, 'nonexistent.json'));
    results.push({ fixture: 'N11_MISSING_INPUT', verdict: r.verdict, codes: r.fails.map(f => f.code) });
    if (r.verdict === 'ARCHIVE_INTEGRITY_PASS') process.exitCode = 1;
  }
  // N12 数字型 serial / 前导零
  mk('N12_LEDGER_BAD_U64', (m, dir) => {
    const p = path.join(dir, m.files[1].name);
    const pg = JSON.parse(fs.readFileSync(p, 'utf8'));
    pg.rows[1].ledger.generatedQuanta = '00x';   // 非法 U64
    const buf = Buffer.from(JSON.stringify(pg));
    fs.writeFileSync(p, buf);
    m.files[1].bytes = buf.length;
    m.files[1].sha256 = crypto.createHash('sha256').update(buf).digest('hex');
    m.dataBytes = m.files.reduce((a, f) => a + f.bytes, 0);
    rewriteManifest(dir, m);
  });

  const pass = results.filter(r => r.verdict !== 'ARCHIVE_INTEGRITY_PASS').length;
  console.error(JSON.stringify({ revision: 'T443_SELFTEST_V1', fixtures: results.length,
    nonPass: pass, results }));
  // 期望: P0 PASS，其余 N* 全部非 PASS
  const p0 = results.find(r => r.fixture === 'P0_VALID_IDLE');
  if (!p0 || p0.verdict !== 'ARCHIVE_INTEGRITY_PASS') process.exitCode = 1;
  const need = (f, code) => {
    const r = results.find(x => x.fixture === f);
    const hit = r && (r.verdict.startsWith(code) || r.codes.some(c => c.startsWith(code)));
    if (!r || r.verdict === 'ARCHIVE_INTEGRITY_PASS' || !hit) process.exitCode = 1;
  };
  need('N2_DECLARED_NOT_PRESENT', 'ACCEPT');           // 事件门拒（seen=true 缺 FIRST_ACCEPT/token）
  need('N3_FLAG_TRUE_EVENT_MISSING', 'REQUIRED_EVENT_MISSING');
  need('N5_INCOMPLETE_UNJUDGABLE', 'UNJUDGABLE');      // verdict 级
  need('N6_PAGE_COUNT_FORMULA', 'PAGE_COUNT_FORMULA');
  need('N7_UNKNOWN_REVISION', 'UNSUPPORTED');
  need('N8_END_LEDGER_MISMATCH', 'END_LEDGER');
  need('N9_SCOPE_MISMATCH', 'SCOPE_MISMATCH');
  need('N10_FILE_SHA_TAMPER', 'FILE_SHA_MISMATCH');
  need('N12_LEDGER_BAD_U64', 'LEDGER_U64');
}

main();
module.exports = { checkArchive };
