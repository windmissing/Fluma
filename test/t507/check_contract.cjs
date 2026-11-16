const fs=require('node:fs'),assert=require('node:assert/strict'),crypto=require('node:crypto');
const contract=JSON.parse(fs.readFileSync('docs/architecture-T503-cold-manifest-v1.json','utf8'));
const protocol=fs.readFileSync('entry/src/main/cpp/gpu_resident/cold_protocol.h','utf8');
for(const [name,value]of Object.entries({MaxTraceBytes:contract.trace.maxCumulativeCapacityBytes/2,TraceBudget:contract.trace.maxCumulativeCapacityBytes,PageBytes:contract.trace.pageUtf8Limit,PageWords:contract.trace.pageWords,DiagnosticBytes:contract.diagnostic.maxUtf8Bytes,TtlNs:contract.trace.ttlFromRequestNs})){
 assert(new RegExp('\\b'+name+'\\s*=\\s*'+value+'(?:ULL)?\\b').test(protocol),name+' differs from machine contract');
}
const manager=fs.readFileSync('entry/src/main/cpp/gpu_resident/cold_trace.h','utf8');assert(manager.includes('std::array<Slot,2>'));assert(manager.includes('std::array<Receipt,32>'));
for(const route of ['m1','m2']){
 const project='.codex/local-test/'+(route==='m1'?'t507-m1-delivery':'t507-m2-1116');
 const diagnostics=fs.readFileSync(project+'/entry/src/main/cpp/gpu_resident/execution_diagnostics.h');
 assert.equal(crypto.createHash('sha256').update(diagnostics).digest('hex'),contract.baseline.executionDiagnosticsSHA256);
 const bridge=fs.readFileSync(project+'/entry/src/main/cpp/gpu_resident/app_bridge.cpp','utf8');
 assert(bridge.includes('napi_value executionDiagnostics(napi_env e,napi_callback_info){return text(e,cold503::Upgrade);}'));
 if(route==='m2'){
  assert(!bridge.includes('struct M2ColdTrace {'));
  for(const name of ['requestM2Trace','readM2Trace'])assert(bridge.includes('napi_value '+name+'(napi_env e,napi_callback_info){return text(e,cold503::Upgrade);}'));
 }
}
console.log(JSON.stringify({revision:'T507_MACHINE_CONTRACT_CHECK_V1',passed:1,failed:0,baseline:'enhanced delivery hash matches',limits:'native constants match T503 manifest',legacy:'online upgrade refusal',device:'NOT_RUN'}));
