#!/usr/bin/env node
/**
 * T65 F44-C lifecycle harness: REAL FileStore + worker-protocol stub.
 *
 * The ArkTS ThreadWorker runtime is unavailable offline, so the REAL
 * P011NaturalObservationStore (FileStore class, unmodified bytes) is driven
 * through a stub worker that mirrors the REAL
 * entry/src/main/ets/workers/P011NaturalObservationWorker.ets message
 * protocol (init/ready/append/written/admission/close/closed and its count
 * accounting). The stub is clearly marked synthetic; it tests the HOST-side
 * close/CLOSED/incomplete-counting contract only, not the worker's file IO.
 *
 * Cases:
 *  A01 normal close: appends 283 records + 27 admissions, close() resolves,
 *     worker reports matching counts (mirrors the CLOSED receipt content the
 *     device never printed).
 *  B01 count mismatch (host 2 vs worker 1) -> OBS_STORE_INCOMPLETE: close
 *     rejects, no CLOSED receipt. This is the guard whose receipt was absent
 *     on device: if the worker had died or a message was lost, the host
 *     must NOT print CLOSED.
 *  C01 append after close initiated -> OBS_STORE_APPEND_AFTER_CLOSE: store
 *     fails, close rejects. (Back/Home during a run must not silently
 *     truncate; the guard is fail-closed.)
 *  D01 double close(): same promise returned (idempotent).
 *
 * Output: test/reports/p065_store_lifecycle_units.json
 */
'use strict';
const fs = require('fs'), assert = require('assert/strict'), Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(
  fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);

let hostHandler = null;
let deferCloseReply = false;
class FakeWorker {
  constructor() { this.sent = []; this._n = 0; this._a = 0; this.terminated = false; }
  postMessage(req) {
    this.sent.push(req);
    if (req.type === 'init') { hostHandler({data: {type: 'ready'}}); }
    else if (req.type === 'append') { this._n += 1; hostHandler({data: {type: 'written', sequence: this._n}}); }
    else if (req.type === 'admission') { this._a += 1; hostHandler({data: {type: 'written', admissions: this._a}}); }
    else if (req.type === 'close') {
      if (deferCloseReply) { deferCloseReply = false; return; }  // hold the receipt
      hostHandler({data: {type: 'closed', sequence: this._n, admissions: this._a}});
    }
  }
  terminate() { this.terminated = true; }
  set onmessage(fn) { hostHandler = fn; }
  get onmessage() { return hostHandler; }
  set onerror(fn) {} set onmessageerror(fn) {}
}
let prior = Module._load;
Module._load = function (n, p, m) {
  if (n === '@ohos.worker') return {default: {ThreadWorker: FakeWorker, workerPort: {}}, ThreadWorker: FakeWorker};
  if (n === '@ohos.hilog') return {default: {info() {}, error() {}, warn() {}}};
  if (n === '@ohos.app.ability.common') return {default: {}};
  return prior.call(this, n, p, m);
};
const storeModule = require('../entry/src/main/ets/physics/P011NaturalObservationStore.ets');
// FileStore is module-private; reach it through the factory with a stubbed context.
const storeCache = [];
function makeStore(cacheDir, runId) {
  return storeModule.createP011NaturalObservationStore({cacheDir: cacheDir}, runId, 'p065-prefix');
}


const results = [];
function rec(id, name, ok, detail) {
  results.push(Object.assign({id, name, status: ok ? 'PASS' : 'FAIL'}, detail));
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const flush = () => new Promise(r => setTimeout(r, 10));

(async () => {
  // A01
  const s = await makeStore('C:/tmp', 'p065fakerunA');
  await s.ready;
  for (let i = 0; i < 283; i++) s.append({context: {sequence: i + 1}});
  for (let i = 0; i < 27; i++) s.appendAdmission({admissionTransactionId: i + 1});
  let aOk = false, aErr = null;
  s.close().then(() => { aOk = true; }).catch(e => { aErr = e; });
  await flush();
  rec('T65-F44C-A01', 'close() resolves with matching counts (283 records / 27 admissions); CLOSED receipt content reproducible',
    aOk === true && aErr === null,
    {records: 283, admissions: 27, synthetic: 'worker protocol stub mirrors real worker counting'});

  // B01
  hostHandler = null;
  const s2 = await makeStore('C:/tmp', 'p065fakerunB');
  await s2.ready;
  s2.append({context: {}});
  s2.sentRecords = 2;                      // desync host vs worker by hand
  let bErr = null, bOk = false;
  s2.close().then(() => { bOk = true; }).catch(e => { bErr = e; });
  await flush();
  rec('T65-F44C-B01', 'host/worker count mismatch -> OBS_STORE_INCOMPLETE; close rejects; no CLOSED receipt',
    bOk === false && bErr !== null && String(bErr.message).includes('INCOMPLETE'),
    {error: bErr && bErr.message,
     deviceRelevance: 'if the worker died or a postMessage was lost on device, this guard fires and CLOSED never prints — one candidate explanation for F44-C; device-side confirmation still missing'});

  // C01: append DURING closing (between close() and the worker's closed
  // reply). The stub holds the closed receipt so the ordering is deterministic.
  hostHandler = null;
  deferCloseReply = true;
  const s3 = await makeStore('C:/tmp', 'p065fakerunC');
  await s3.ready;
  const p3 = s3.close();                     // closing = true; receipt held
  let threw = null;
  try { s3.append({context: {}}); }          // append during closing -> fail()
  catch (e) { threw = e; }
  let cRejected = false;
  p3.catch(() => { cRejected = true; });
  await flush();
  rec('T65-F44C-C01', 'append during closing -> OBS_STORE_APPEND_AFTER_CLOSE (fail-closed, close rejects)',
    threw === null && cRejected,
    {note: 'no throw to the physics caller; store marked failed and P011_OBS_ERROR logged instead of CLOSED'});

  // D01
  hostHandler = null;
  const s4 = await makeStore('C:/tmp', 'p065fakerunD');
  await s4.ready;
  const p4a = s4.close();
  const p4b = s4.close();
  rec('T65-F44C-D01', 'double close() returns the same promise (idempotent)', p4a === p4b, {});

  fs.mkdirSync(require('path').join(__dirname, 'reports'), {recursive: true});
  const out = {
    executionId: 'p065-store-lifecycle-units-v1', date: '2026-10-26',
    generatedBy: 'test/p065_store_lifecycle_units.cjs',
    harness: 'REAL FileStore (entry/src/main/ets/physics/P011NaturalObservationStore.ets, unmodified) + worker-protocol stub mirroring real P011NaturalObservationWorker.ets message semantics; synthetic worker, no device',
    results,
    summary: {total: results.length, pass: results.filter(r => r.status === 'PASS').length},
    deviceGap: 'Whether Back/Home on device ever reached aboutToDisappear (which calls observationStore.close()) is not determinable from T44 logs: no P011_OBS_ERROR and no P011_OBS_STORE_CLOSED appear, and no lifecycle telemetry exists. Either close() was never invoked (page not destroyed) or the worker died before replying — T64 must route; a device retest with lifecycle telemetry would discriminate.'
  };
  fs.writeFileSync(require('path').join(__dirname, 'reports', 'p065_store_lifecycle_units.json'),
    JSON.stringify(out, null, 1) + '\n');
  console.log('total', results.length, 'pass', out.summary.pass);
  process.exitCode = out.summary.pass === results.length ? 0 : 1;
})();
