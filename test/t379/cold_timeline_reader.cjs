// T379 GPU cold-timeline independent reader (tester forensic tool).
// Oracle independence: no imports from production/native test assertions; consumes
// raw timeline row arrays (T337b rows_runN.json shape / T376 tl-partN.ndjson pages)
// and optional host-side text (T375 partial-host.txt shape).
// Checks (per card): stage42 vs READY/first-anchor order, same-run/TID raw-ns
// monotonicity + serial continuity, first/last batch + GAP attribution,
// accepted->same-token evidence chain (explicit NOT_JUDGABLE when fields absent).
'use strict';
const fs = require('fs');

const STAGE = {
  INVALID: 0, Invalidate: 1, InitBegin: 2, Ready: 3, ReadyRejected: 4,
  WaitEnter: 5, Wake: 6, FirstTick: 7, AdvanceBegin: 8, AdvanceEnd: 9,
  AnchorSet: 10, PumpBegin: 11, PumpEnd: 12, DrawBegin: 13, DrawEnd: 14,
  SwapBegin: 15, SwapEnd: 16, PauseBegin: 17, PauseEnd: 18, Offer: 19,
  Consume: 20, Stale: 21, RootSubmit: 22, RootPoll: 23,
};
const SUBMIT_WARMUP = 42, SUBMIT_ENTER = 40, SUBMIT_EXIT = 41,
  BATCH_BEGIN = 24, BATCH_END = 25, OP_BEGIN = 34, OP_END = 35;

function u64(s) { return typeof s === 'string' && /^\d+$/.test(s) || typeof s === 'number' && Number.isSafeInteger(s) && s >= 0; }
function ns(r) { return typeof r.ns === 'string' ? BigInt(r.ns) : BigInt(r.ns); }

// normalize a row into comparable shape with BigInt ns
function norm(r) {
  const out = { ...r };
  for (const k of ['serial', 'ns', 'now', 'last', 'delta', 'root']) out[k + 'B'] = BigInt(out[k]);
  return out;
}

function readRows(source) {
  if (Array.isArray(source)) return source.map(norm);
  const text = String(source);
  // ndjson pages (T376 tl-partN) or bare rows
  const rows = [];
  for (const line of text.split('\n')) {
    const t = line.trim(); if (!t) continue;
    let obj; try { obj = JSON.parse(t); } catch { continue; }
    if (Array.isArray(obj)) rows.push(...obj.map(norm));
    else if (Array.isArray(obj.rows)) rows.push(...obj.rows.map(norm));
    else if (obj.stage !== undefined) rows.push(norm(obj));
  }
  return rows;
}

function judge(rowsIn, opts = {}) {
  const rep = { checks: [], gaps: [], findings: {} };
  const add = (name, ok, detail) => rep.checks.push({ name, verdict: ok ? 'PASS' : (detail && detail.notJudgable ? 'NOT_JUDGABLE' : 'FAIL'), detail });
  const rows = rowsIn.slice().sort((a, b) => (a.serialB < b.serialB ? -1 : 1));

  // C1 serial continuity + ns monotonicity (global, original units)
  let gaps = [];
  for (let i = 1; i < rows.length; i++) {
    if (rows[i].serialB !== rows[i - 1].serialB + 1n) gaps.push({ at: String(rows[i - 1].serialB), missing: String(rows[i].serialB - rows[i - 1].serialB - 1n) });
    if (rows[i].nsB < rows[i - 1].nsB) rep.findings.NS_BACKWARD = { at: rows[i].serial, ns: String(rows[i].nsB), prev: String(rows[i - 1].nsB) };
  }
  rep.gaps = gaps;
  add('serial-continuity-raw-units', gaps.length === 0 && !rep.findings.NS_BACKWARD, { detail: `${gaps.length} gaps` });

  // C2 group by (epoch,run,surface,tid); per-group ns monotonic
  const groups = new Map();
  for (const r of rows) {
    const k = `${r.epoch}/${r.run}/${r.surface}/${r.tid}`;
    if (!groups.has(k)) groups.set(k, []);
    groups.get(k).push(r);
  }
  rep.groups = [...groups.keys()];
  let perGroupOk = true;
  for (const [, g] of groups) {
    for (let i = 1; i < g.length; i++) if (g[i].nsB < g[i - 1].nsB) { perGroupOk = false; break; }
  }
  add('same-run-tid-ns-monotonic', perGroupOk);

  // C3 stage42 vs Ready vs first AnchorSet order per run-group
  const orderFindings = [];
  for (const [k, g] of groups) {
    const w42 = g.filter(r => r.stage === SUBMIT_WARMUP);
    const ready = g.filter(r => r.stage === STAGE.Ready);
    const anchor = g.filter(r => r.stage === STAGE.AnchorSet);
    if (w42.length === 0) { orderFindings.push({ group: k, note: 'no-stage42-in-window' }); continue; }
    if (ready.length === 0 && !opts.requireReady) { orderFindings.push({ group: k, note: 'no-ready-in-window (window boundary) — order vs Ready NOT_JUDGABLE' }); }
    const first42 = w42[0].serialB;
    const anchorOk = anchor.length === 0 || w42.every(r => r.serialB < anchor[0].serialB);
    const readyOk = ready.length === 0 ? null : w42.every(r => r.serialB < ready[0].serialB);
    // carry contract: exactly one carry0 with delta=0, one carry1 with delta>0, root=0 both
    const c0 = w42.filter(r => r.carry === 0), c1 = w42.filter(r => r.carry === 1);
    const pairOk = w42.length === 2 && c0.length === 1 && c1.length === 1 && c0[0].deltaB === 0n && c1[0].deltaB > 0n && w42.every(r => r.rootB === 0n);
    orderFindings.push({ group: k, first42: String(first42), firstReady: ready[0] ? String(ready[0].serialB) : null, firstAnchor: anchor[0] ? String(anchor[0].serialB) : null, pairOk, orderOk: (readyOk === null || readyOk) && anchorOk, carry1WarmupNs: String(c1.length ? c1[0].deltaB : '') });
  }
  rep.findings.stage42Order = orderFindings;
  add('stage42-before-ready-anchor-pairs', orderFindings.every(f => f.note || (f.pairOk && f.orderOk)));

  // C4 batches: group rows between BATCH_BEGIN..BATCH_END within (epoch,run,root)
  const batchFindings = [];
  for (const [k, g] of groups) {
    let cur = null; const batches = [];
    for (const r of g) {
      if (r.stage === BATCH_BEGIN) { cur = [r]; }
      else if (cur && r.stage === BATCH_END) { cur.push(r); batches.push(cur); cur = null; }
      else if (cur) cur.push(r);
    }
    batches.forEach((b, i) => {
      const wall = Number(b[b.length - 1].nsB - b[0].nsB) / 1e6;
      const ops = b.filter(r => r.stage === OP_BEGIN).length;
      // tail gap: last OP_END -> BATCH_END
      const lastOp = [...b].reverse().find(r => r.stage === OP_END);
      const tailMs = lastOp ? Number(b[b.length - 1].nsB - lastOp.nsB) / 1e6 : null;
      batchFindings.push({ group: k, batch: i + 1, wallMs: +wall.toFixed(3), ops, tailMs: tailMs === null ? null : +tailMs.toFixed(3) });
    });
  }
  rep.findings.batches = batchFindings;
  const b1 = batchFindings.filter(b => b.batch === 1), b2 = batchFindings.filter(b => b.batch === 2);
  add('batch1-wall<=1ms', b1.length > 0 && b1.every(b => b.wallMs <= 1.0), { detail: JSON.stringify(b1) });
  add('batch2-wall<=1ms', b2.length > 0 && b2.every(b => b.wallMs <= 1.0), { detail: JSON.stringify(b2) });

  // C5 GAP attribution: AnchorSet -> PauseBegin interval per run (>33ms window flag)
  const gapFindings = [];
  for (const [k, g] of groups) {
    for (let i = 1; i < g.length; i++) {
      if (g[i - 1].stage === STAGE.AnchorSet && g[i].stage === STAGE.PauseBegin) {
        gapFindings.push({ group: k, ms: +(Number(g[i].nsB - g[i - 1].nsB) / 1e6).toFixed(3) });
      }
    }
  }
  rep.findings.anchorToPauseGaps = gapFindings;

  // C6 accepted->same-token chain: timeline rows carry no token/accepted fields.
  // RootSubmit(root>0) -> RootPoll same root proves submission completed; accepted
  // count and token equality live in publication receipts (outside this archive).
  const rs = rows.filter(r => r.stage === STAGE.RootSubmit);
  const rp = rows.filter(r => r.stage === STAGE.RootPoll);
  const submitPairs = [];
  for (const a of rs) {
    const b = rp.find(x => x.epoch === a.epoch && x.run === a.run && x.root === a.root && x.serialB > a.serialB);
    if (b) submitPairs.push({ group: `${a.epoch}/${a.run}`, root: String(a.rootB), wallMs: +(Number(b.nsB - a.nsB) / 1e6).toFixed(3) });
  }
  rep.findings.rootSubmitPoll = submitPairs;
  const tokenFieldsPresent = rows.some(r => r.token !== undefined || r.accepted !== undefined);
  if (!tokenFieldsPresent) add('accepted-to-same-token', false, { notJudgable: true, detail: 'timeline rows carry no accepted/token fields; requires publication receipt (external). RootSubmit->RootPoll pairs listed as partial evidence.' });
  else add('accepted-to-same-token', true, { detail: 'token fields present' });

  return rep;
}

// ---- negative controls (card-mandated) ----
function negativeControls(sampleRows) {
  const out = {};
  // A) truncation: drop middle row -> SERIAL gap must be flagged
  const cut = sampleRows.filter((_, i) => i !== Math.floor(sampleRows.length / 2)).map(norm);
  const r1 = judge(cut, { requireReady: false });
  out.truncationFlagsGap = r1.gaps.length > 0;
  // B) cross-run mixing: rows from another run keep their own group (no false monotonicity failure)
  const foreign = sampleRows.map(r => ({ ...r, epoch: String(Number(r.epoch) + 100) }));
  const mixed = sampleRows.concat(foreign).map(norm);
  const r2 = judge(mixed, { requireReady: false });
  out.crossRunIsolated = r2.checks.find(c => c.name === 'same-run-tid-ns-monotonic').verdict === 'PASS';
  // C) wrong-unit: convert some ns from ns to ms magnitude -> NS_BACKWARD flagged
  const bad = sampleRows.map((r, i) => i === 5 ? { ...r, ns: String(Math.floor(Number(r.ns) / 1e6)) } : r).map(norm);
  const r3 = judge(bad, { requireReady: false });
  out.wrongUnitFlagged = r3.findings.NS_BACKWARD !== undefined;
  return out;
}

// ---- CLI ----
function main(argv) {
  const [mode, ...paths] = argv;
  const results = {};
  if (mode === 't337b') {
    for (const p of paths) {
      const rows = JSON.parse(fs.readFileSync(p, 'utf-8')).map(norm);
      results[p] = judge(rows, { requireReady: false });
      results[p].negativeControls = negativeControls(JSON.parse(fs.readFileSync(paths[0], 'utf-8')));
    }
  } else if (mode === 'ndjson') {
    const text = paths.map(p => fs.readFileSync(p, 'utf-8')).join('\n');
    results.rows = judge(readRows(text), { requireReady: false });
  } else if (mode === 'host') {
    // T375 host text: structural expectations only (pair counts, CASE_W presence)
    const text = fs.readFileSync(paths[0], 'utf-8');
    const cases = {};
    for (const m of text.matchAll(/CASE_W[^\n]*/g)) cases.CASE_W = m[0];
    results.host = { caseWLine: cases.CASE_W || null, note: 'host text has no per-row timeline; only structural presence checks apply' };
  }
  console.log(JSON.stringify(results, null, 1));
}
if (require.main === module) main(process.argv.slice(2));
module.exports = { judge, readRows, negativeControls, norm };
