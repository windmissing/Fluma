'use strict';
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),zlib=require('node:zlib'),crypto=require('node:crypto');
const repo=path.resolve(__dirname,'../..'),out=path.join(repo,'test/reports/t168-rev2'),original=path.join(out,'original-81');fs.mkdirSync(original,{recursive:true});
const sha=b=>crypto.createHash('sha256').update(b).digest('hex'),files=['gate-summary.json','off_all.json.gz','off-all-remaining.json'];
const manifest=files.map(file=>{const bytes=cp.execFileSync('git',['show','6c0c654:test/reports/t168/'+file],{cwd:repo,maxBuffer:32*1024*1024});fs.writeFileSync(path.join(original,file),bytes);return {file,sha256:sha(bytes),bytes:bytes.length,origin:'git:6c0c654:test/reports/t168/'+file};});
const before=JSON.parse(zlib.gunzipSync(fs.readFileSync(path.join(original,'off_all.json.gz'))));
const after=JSON.parse(zlib.gunzipSync(fs.readFileSync(path.join(out,'off_all.json.gz'))));
const rows=[...before.allocationSites.map(x=>({...x,previousLayer:'SOURCE_ALLOCATION'})),...before.unresolvedCalls.map(x=>({...x,previousLayer:'UNRESOLVED'}))].map((row,i)=>{
 const source=fs.readFileSync(path.join(repo,'entry/src/main/ets/physics',row.file),'utf8'),ownerContexts=after.reachableContexts.filter(x=>x.owner===row.owner);
 const retained=source.includes(row.text),stillReported=[...after.allocationSites,...after.unresolvedCalls,...after.allocationEffects].some(x=>x.owner===row.owner&&x.text===row.text);
 return {id:'V1-'+String(i+1).padStart(3,'0'),previous:row,retainedSourceExcerpt:retained,currentOwnerContexts:ownerContexts.map(c=>c.key),stillReported,
  disposition:stillReported?'STILL_BLOCKING':!retained?'SOURCE_REVISED_SEE_T174_DIFF':ownerContexts.length?'MODELED_OR_BRANCH_PRUNED_SEE_CURRENT_GRAPH':'OWNER_NOT_REACHABLE_SEE_CURRENT_GRAPH'};
});
if(rows.length!==81||before.remainingSiteCount!==81)throw Error('ORIGINAL_81_IDENTITY_CHANGED');
fs.writeFileSync(path.join(out,'v1-reconciliation.json'),JSON.stringify({origin:'6c0c654',note:'Evidence index only. Classification never removes a blocking site or changes the verdict. Old witness retained in every row; actual current graph is authority.',manifest,oldRemaining:81,currentRemaining:after.remainingSiteCount,rows},null,2)+'\n');
console.log(JSON.stringify({oldRemaining:81,currentRemaining:after.remainingSiteCount,archived:manifest}));
