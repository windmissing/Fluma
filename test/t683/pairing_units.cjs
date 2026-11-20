const assert = require('node:assert/strict');
const fs = require('node:fs');
const Module = require('node:module');
const path = require('node:path');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const render = path.resolve(process.argv[2], 'entry/src/main/ets/render');
const moduleUnderTest = new Module(__filename);
const source = ['RasterGeometry', 'PrepareReadiness', 'PreparePairingEvidence', 'PrepareReadinessV4']
  .map(name => fs.readFileSync(path.join(render, name + '.ets'), 'utf8').replace(/^import[^\n]*$/mg, '')).join('\n');
moduleUnderTest._compile(ts.transpileModule(source, {compilerOptions: {module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020}}).outputText, __filename);
const {PrepareReadiness, PrepareReadinessV4, buildPreparePairingEvidence, pairingUtf8Bytes} = moduleUnderTest.exports;
assert.equal(pairingUtf8Bytes('界😀'), 7);
const gate = new PrepareReadinessV4(() => {});
const early = JSON.parse(gate.report());
assert.equal(early.pairingEvidence.owner, null);
assert.equal(early.pairingEvidence.lastFrame.status, 'NOT_EVALUATED');
const actualReport = PrepareReadiness.prototype.report;
const base = {
  session:'fixture', prepareId:3, pageGeneration:5, observationSerial:7,
  inputSerial:0, startedMonoMs:0, state:'READY', readinessReason:'',
  diagnosticOverflow:false, keyboardAreaSeen:false, sessionEverEdited:false,
  inputs:Array.from({length:3},()=>({created:true,editing:false,focused:false})),
  snapshot:{run:'fixture', keyboardHeight:0, nativeReady:false, raster:{observationSerial:9}},
  events:[JSON.stringify({kind:'FRAME',prepareId:3,pageGeneration:5,serial:11,value:JSON.stringify({frame:0,now:0,reason:''})})]
};
let calls = 0;
PrepareReadiness.prototype.report = function () { calls++; return JSON.stringify(base); };
try {
  const before = [gate.prepareId, gate.pageGeneration, gate.state, gate.ownerSerial];
  const raw = gate.report();
  const e = JSON.parse(raw).pairingEvidence;
  assert.equal(calls,1);
  assert.equal(e.revision,'T680_PREPARE_PAIRING_EVIDENCE_V1');
  assert.deepEqual([e.gate.observationSerial,e.raster.observationSerial,e.lastFrame.observationSerial],[7,9,11]);
  assert.equal(e.lastFrame.snapshotSerialMatches,false);
  assert.equal(e.lastFrame.frame,0);
  assert.equal(e.keyboard.height,0);
  assert.equal(e.snapshot.windowId,null);
  assert.equal(e.snapshot.nativeReady,false);
  assert.deepEqual([gate.prepareId, gate.pageGeneration, gate.state, gate.ownerSerial],before);
  base.padding = '界'.repeat(5000);
  const truncated = gate.report();
  assert(Buffer.byteLength(truncated) <= 14336);
  assert.equal(JSON.parse(truncated).detailTruncated,true);
  assert.equal(JSON.parse(truncated).authority,undefined);
  assert.deepEqual(JSON.parse(truncated).pairingEvidence,e);
  base.snapshot.run = '界'.repeat(43);
  const oversized = buildPreparePairingEvidence(base,null,0);
  assert.equal(oversized.snapshot.run,null);
  assert(oversized.truncatedFields.includes('snapshot.run'));
  assert.equal(oversized.status,'INCOMPLETE');
  base.events = Array(257).fill(base.events[0]);
  assert(buildPreparePairingEvidence(base,null,0).truncatedFields.includes('lastFrame.events.after256'));
} finally { PrepareReadiness.prototype.report = actualReport; }
console.log(JSON.stringify({result:'PASS',scope:'actual generated readiness/report and pairing functions',device:'NOT_RUN'}));
