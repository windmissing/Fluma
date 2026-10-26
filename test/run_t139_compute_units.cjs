'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs');
const {summarize}=require('./run_t139_compute_stats.cjs');
const source=fs.readFileSync('test/p013_native/t139_compute_harness.cpp','utf8');
for(const token of ['GL_SHADER_STORAGE_BARRIER_BIT','GL_BUFFER_UPDATE_BARRIER_BIT','GL_TIMEOUT_EXPIRED',
  'GL_WAIT_FAILED','glMapBufferRange','glUnmapBuffer','glDeleteSync','glDeleteBuffers',
  'CONTEXT_RECOVERY_FIRST','GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS','GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS'])
  assert.ok(source.includes(token),token);
const good=(n,ordinal,phase,ms)=>({contextEpoch:1,N:n,ordinal,phase,valid:true,reason:'NONE',
  waitReturn:'CONDITION_SATISFIED',attemptCpuMs:ms,fenceWaitCpuMs:ms/2});
const raw={revision:'T139_COMPUTE_HARNESS_V1',runId:'synthetic',buildId:'test',samples:[
  good(192,1,'BUFFER_FIRST',10),good(192,2,'PERSISTENT_WARM',2),
  {...good(192,3,'PERSISTENT_WARM',100),valid:false,reason:'WAIT_TIMEOUT',waitReturn:'TIMEOUT_EXPIRED'},
  good(192,4,'PERSISTENT_WARM',4)]};
const result=summarize(raw);
assert.equal(result.invalidSamples.length,1);
assert.equal(result.validGroups.length,2);
const warm=result.validGroups.find(x=>x.key==='1:192:PERSISTENT_WARM');
assert.equal(warm.validCount,2);assert.equal(warm.attemptCpuMs.p50,2);assert.equal(warm.attemptCpuMs.p95,4);
assert.deepEqual(warm.ordinals,[2,4]);assert.equal(warm.gpuRuntime,'NOT_MEASURED');
console.log('T139 source contract and invalid-sample/percentile units PASS');
