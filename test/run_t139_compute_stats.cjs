// T139 raw-sample aggregation. Invalid/timeouts stay in report, never enter percentiles.
'use strict';
const fs=require('node:fs'),crypto=require('node:crypto');
function summarize(run){
  if(run.revision!=='T139_COMPUTE_HARNESS_V1'||!Array.isArray(run.samples)||!run.runId||!run.buildId)
    throw new Error('INVALID_RUN_IDENTITY');
  const groups=new Map(),invalid=[];
  for(const s of run.samples){
    if(!Number.isInteger(s.contextEpoch)||!Number.isInteger(s.ordinal)||!s.phase||!Number.isInteger(s.N))
      throw new Error('INVALID_SAMPLE_IDENTITY');
    const key=`${s.contextEpoch}:${s.N}:${s.phase}`;
    if(!s.valid||s.reason!=='NONE'||!['ALREADY_SIGNALED','CONDITION_SATISFIED'].includes(s.waitReturn)){
      invalid.push({key,ordinal:s.ordinal,reason:s.reason,waitReturn:s.waitReturn});continue;
    }
    if(!Number.isFinite(s.attemptCpuMs)||s.attemptCpuMs<0||!Number.isFinite(s.fenceWaitCpuMs))
      throw new Error('INVALID_SAMPLE_TIMING');
    if(!groups.has(key))groups.set(key,[]);groups.get(key).push(s);
  }
  const percentile=(a,p)=>a[Math.max(0,Math.ceil(p*a.length)-1)]; // nearest-rank
  const stats=[];
  for(const [key,rows] of groups){
    const totals=rows.map(x=>x.attemptCpuMs).sort((a,b)=>a-b);
    const waits=rows.map(x=>x.fenceWaitCpuMs).sort((a,b)=>a-b);
    const q1=percentile(totals,.25),q3=percentile(totals,.75),iqr=q3-q1;
    stats.push({key,validCount:rows.length,ordinals:rows.map(x=>x.ordinal),
      attemptCpuMs:{p50:percentile(totals,.5),p95:percentile(totals,.95)},
      fenceWaitCpuMs:{p50:percentile(waits,.5),p95:percentile(waits,.95)},
      outlierCandidateOrdinals:rows.filter(x=>x.attemptCpuMs>q3+1.5*iqr).map(x=>x.ordinal),
      gpuRuntime:'NOT_MEASURED'});
  }
  return {revision:'T139_STATS_V1',runId:run.runId,buildId:run.buildId,
    rawSha256:crypto.createHash('sha256').update(JSON.stringify(run.samples)).digest('hex'),
    percentileMethod:'NEAREST_RANK_CEIL',outlierPolicy:'Tukey 1.5 IQR annotated; no valid sample excluded',
    validGroups:stats,invalidSamples:invalid};
}
if(require.main===module){const file=process.argv[2],out=process.argv[3];
  if(!file||!out)throw new Error('usage: node run_t139_compute_stats.cjs RAW_JSON OUT_JSON');
  const report=summarize(JSON.parse(fs.readFileSync(file,'utf8')));
  fs.writeFileSync(out,JSON.stringify(report,null,2)+'\n');
  console.log(`T139 valid groups ${report.validGroups.length}, invalid samples ${report.invalidSamples.length}`);
}
module.exports={summarize};
