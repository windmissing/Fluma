'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),zlib=require('node:zlib'),cp=require('node:child_process'),Module=require('node:module'),assert=require('node:assert/strict');
const {audit}=require('../probe_p011_particle_hot_graph.cjs');
const {model}=require('./p011_hot_context_graph.cjs');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const repo=path.resolve(__dirname,'../..'),out=path.join(repo,'test/reports/t168-rev2');fs.mkdirSync(out,{recursive:true});
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const summary={revision:'T168_GATE_REPORT_V2',date:'2026-10-27',device:'NOT_RUN',runtimeEquivalence:'NOT_RUN_T155_OWNS',productStability:'NOT_RUN',profiles:{}};
function archive(name,result){const bytes=Buffer.from(JSON.stringify(result,null,2)+'\n'),compressed=zlib.gzipSync(bytes),file=path.join(out,name+'.json.gz');fs.writeFileSync(file,compressed);
 return {file:path.relative(repo,file).replaceAll('\\','/'),jsonBytes:bytes.length,gzipBytes:compressed.length,gzipSha256:hash(compressed),jsonSha256:hash(bytes)};}
const legacy=audit();
// Preserve the old API result, not merely a revised function with the same label.
const original=cp.execFileSync('git',['show','50ebbff:test/probe_p011_particle_hot_graph.cjs'],{cwd:repo,encoding:'utf8'});
const file=path.join(repo,'test/probe_p011_particle_hot_graph.cjs'),m=new Module(file,module);m.filename=file;m.paths=Module._nodeModulePaths(path.dirname(file));m._compile(original,file);
assert.deepEqual(legacy,m.exports.audit());
summary.legacyConservative={status:legacy.status,remaining:legacy.remainingSiteCount,unresolved:legacy.unresolvedCalls.length,allocationSites:legacy.hotAllocationSites.length,registryComplete:legacy.registryComplete,exactOriginalApiEquality:true,archive:archive('legacy-conservative',legacy)};
for(const profile of ['OFF_ALL','PROGRESS_OFF_EVIDENCE_ON','PROGRESS_ON']){
 const r=audit({profile});
 summary.profiles[profile]={status:r.status,remaining:r.remainingSiteCount,allocationEffects:r.allocationEffects.length,allocationSites:r.allocationSites.length,unresolved:r.unresolvedCalls.length,issues:r.analysisIssues.length,registryComplete:r.registryComplete,legacySites:r.legacySiteCount,roots:r.rootManifest.length,contexts:r.reachableContexts.length,pruned:r.prunedBranches.length,productionHotPathZeroAllocation:r.productionHotPathZeroAllocation,archive:archive(profile.toLowerCase(),r)};
 const table=r.allocationSites.map(a=>({owner:a.owner,file:a.file,line:a.line,kind:a.kind,text:a.text,
  phase:a.witness.some(w=>w.callee?.includes('captureFailure'))?'HOT_FAILURE':a.witness.some(w=>w.callee?.includes('checkLong'))?'HOT_THRESHOLD_CAPTURE':'OTHER_REACHABLE_SOURCE',
  trigger:'ABSTRACT_REACHABILITY_NOT_DYNAMIC_COUNT',witness:a.witness}));
 fs.writeFileSync(path.join(out,profile.toLowerCase()+'-allocations.json'),JSON.stringify({profile,status:r.status,staticSiteCount:table.length,sites:table,unresolvedExternalClock:r.unresolvedCalls.filter(x=>x.text==='this.clock()'),runtimeAllocations:null,runtimeBytes:null,rss:null,elapsedMs:null},null,2)+'\n');
 if(profile==='OFF_ALL')fs.writeFileSync(path.join(out,'off-all-remaining.json'),JSON.stringify({status:r.status,registryComplete:r.registryComplete,layers:r.layers,havocProofs:r.havocProofs,storageProofs:r.storageProofs,allocationSites:r.allocationSites,unresolvedCalls:r.unresolvedCalls,analysisIssues:r.analysisIssues,sourceManifest:r.sourceManifest,rootManifest:r.rootManifest},null,2)+'\n');
 console.log(profile+' '+r.status+' remaining='+r.remainingSiteCount);
}
summary.productionHotPathZeroAllocation=summary.profiles.OFF_ALL.productionHotPathZeroAllocation;
// Label source inventory phases for ON cost review. These labels never prune
// the graph or exempt a method; cold/export reachability is established by
// the full graphs above, not by these display names.
const sourceModel=model(path.join(repo,'entry/src/main/ets/physics')),inventory=[];
for(const file of ['P011PoolProgressDiagnostic.ets','P011FieldColumnEvidenceV2.ets','P011MergeExactIndex.ets']){
 const sf=sourceModel.sources.get(file);
 function visit(n){let kind=ts.isNewExpression(n)?'constructor':ts.isObjectLiteralExpression(n)?'object-literal':ts.isArrayLiteralExpression(n)?'array-literal':ts.isArrowFunction(n)||ts.isFunctionExpression(n)?'closure':ts.isTemplateExpression(n)?'template-string':null;
  if(kind){let owner=n.parent;while(owner&&!ts.isFunctionLike(owner)&&!ts.isPropertyDeclaration(owner))owner=owner.parent;
   const name=owner?.name?.getText()||'',phase=!owner||ts.isPropertyDeclaration(owner)||ts.isConstructorDeclaration(owner)?'COLD_INITIALIZATION':/export|rowJson/i.test(name)?'EXPORT_SOURCE_INVENTORY':file==='P011FieldColumnEvidenceV2.ets'?'FAILURE_OR_OBSERVATION_SOURCE':file==='P011PoolProgressDiagnostic.ets'?'THRESHOLD_OR_PROGRESS_SOURCE':'INDEX_SOURCE';
   inventory.push({...sourceModel.pos(n),kind,owner:name,phase,classificationOnly:true,reachabilityAuthority:'FULL_PROFILE_GRAPH',runtimeCount:null});}
  ts.forEachChild(n,visit);
 }visit(sf);
}
fs.writeFileSync(path.join(out,'diagnostic-on-source-inventory.json'),JSON.stringify({scope:'Source phase inventory; not a method allowlist and not measured runtime overhead',inventory},null,2)+'\n');
summary.delivery=summary.productionHotPathZeroAllocation?'SOURCE_PROFILE_PROVEN':'PARTIAL_FAIL_CLOSED_REQUIRES_ARCHITECT';
fs.writeFileSync(path.join(out,'gate-summary.json'),JSON.stringify(summary,null,2)+'\n');
// Archival succeeds even for a failing analyzed program; graph CLI still exits 2.
console.log(JSON.stringify(summary));
