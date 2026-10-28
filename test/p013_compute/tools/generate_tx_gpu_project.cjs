'use strict';
// T212 independent whole-owner harness. Old V2/V3/V4 generators unchanged.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const root=path.resolve(__dirname,'../../..'),base=path.join(root,'.codex/local-test'),out=path.resolve(process.argv[2]||''),variant=process.argv[3];
if(!out.startsWith(base+path.sep)||!['STRICT_O0','STRICT_O2'].includes(variant)||fs.existsSync(out))throw Error('TX_GPU_FRESH_ISOLATED_OUTPUT_AND_VARIANT_REQUIRED');
cp.execFileSync(process.execPath,[path.join(__dirname,'generate_transaction_project_v4.cjs'),out,variant],{cwd:root,stdio:'pipe'});
const sha=b=>crypto.createHash('sha256').update(b).digest('hex'),v4=JSON.parse(fs.readFileSync(path.join(out,'t191-source-manifest.json'))),v3=v4.v3SourceManifest,v2=v3.v2SourceManifest;
const changes=[],inputs={};
function writeCopy(src,dst){const bytes=fs.readFileSync(path.join(root,src));fs.mkdirSync(path.dirname(path.join(out,dst)),{recursive:true});fs.writeFileSync(path.join(out,dst),bytes);inputs[src]=sha(bytes);}
function patch(file,from,to){const p=path.join(out,file),old=fs.readFileSync(p,'utf8'),eol=old.includes('\r\n')?'\r\n':'\n',s=old.replaceAll('\r\n','\n');if(s.split(from).length!==2)throw Error('TX_GPU_WHITELIST:'+file+':'+from.slice(0,64));const changed=s.replace(from,to),written=changed.replaceAll('\n',eol);if(changed.replace(to,from).replaceAll('\n',eol)!==old)throw Error('TX_GPU_INVERSE');fs.writeFileSync(p,written);changes.push({file,from,to,eol,originalSha256:sha(old),generatedSha256:sha(written)});}
// Verify all overwritten legacy integration sources against the parent source
// manifest before restoring the untouched production RN paths in this clone.
for(const name of ['P011ParticleHot','P011ParticleM2Field','P011ParticleM2Sink','WaterSandField']){
 const rel='physics/'+name+'.ets',bytes=fs.readFileSync(path.join(root,'entry/src/main/ets',rel)),original=v2.changes.find(x=>x.file===rel);
 if(original&&sha(bytes)!==original.originalSha256)throw Error('TX_GPU_PARENT_INPUT_CHANGED');
 const target='entry/src/main/ets/'+rel,generated=fs.readFileSync(path.join(out,target));fs.writeFileSync(path.join(out,target),bytes);
 changes.push({file:target,kind:'RESTORE_ORIGINAL_FROM_HASHED_V2_INPUT',parentGeneratedSha256:sha(generated),restoredSha256:sha(bytes)});
}
const treeReady=fs.existsSync(path.join(root,'entry/src/main/ets/physics/tx/TxWholePairAdapter.ets'));
for(const n of fs.readdirSync(path.join(root,'test/p013_compute/ets/tx')).filter(n=>n.endsWith('.ets')&&(n!=='TxGpuTreeAdapter.ets'||treeReady)))writeCopy('test/p013_compute/ets/tx/'+n,'entry/src/main/ets/physics/tx/'+n);
if(treeReady){for(const n of fs.readdirSync(path.join(root,'entry/src/main/ets/physics/tx')).filter(n=>n.endsWith('.ets')))writeCopy('entry/src/main/ets/physics/tx/'+n,'entry/src/main/ets/physics/txcore/'+n);
 patch('entry/src/main/ets/physics/tx/TxGpuWorker.ets',"import { runTxGpuFixture } from './TxGpuFixture';","import { runTxGpuFixture } from './TxGpuFixture';\nimport { TxGpuTreeAdapter } from './TxGpuTreeAdapter';");
 patch('entry/src/main/ets/physics/tx/TxGpuWorker.ets',"new NativeTransport():null,now)","new NativeTransport():null,now,c.config.backend==='TX_ADJ_AVL'?new TxGpuTreeAdapter(c.config.nmax):null)");
}
const hot='entry/src/main/ets/physics/P011ParticleHot.ets',m2='entry/src/main/ets/physics/P011ParticleM2Field.ets',field='entry/src/main/ets/physics/WaterSandField.ets';
patch(hot,'// P-011 D1-R3 S0/S1: fixed-capacity HOT workspace and pure dual collect.',"import { TxPairBackends } from './tx/TxPairBackends';\n// P-011 D1-R3 S0/S1: fixed-capacity HOT workspace and pure dual collect.");
patch(hot,'export class P011ParticleHotWorkspace {','export class P011ParticleHotWorkspace {\n  txPairBackend:TxPairBackends|null=null;');
patch(hot,'  const found=work.mergeIndex.find(p,work.params.mergeHeightRadius);\n  if(!found)return P011_HOT_INVALID_PATCH_STATE;\n  const a=work.mergeIndex.selectedA,b=work.mergeIndex.selectedB;',
 `  let a:number,b:number;
  if(work.txPairBackend===null){const found=work.mergeIndex.find(p,work.params.mergeHeightRadius);if(!found)return P011_HOT_INVALID_PATCH_STATE;a=work.mergeIndex.selectedA;b=work.mergeIndex.selectedB;}
  else {try{const pair=work.txPairBackend.selectPair(p,work.params.mergeHeightRadius,work);if(pair.status==='NO_PAIR')return P011_HOT_INVALID_PATCH_STATE;if(pair.status!=='PAIR')return P011_HOT_AUDIT_FAILED;a=pair.slotA;b=pair.slotB;}catch(error){return P011_HOT_AUDIT_FAILED;}}`);
patch(hot,'  const status=mergePairHot(work,p,a,b);',`  const txCpuStart=work.txPairBackend===null?NaN:work.txPairBackend.nowPublic();
  const status=mergePairHot(work,p,a,b);
  if(work.txPairBackend!==null)work.txPairBackend.merged(status,work.txPairBackend.nowPublic()-txCpuStart);`);
patch(m2,'    phase: number, sourceKind: number, sourceId: number): number {','    phase: number, sourceKind: number, sourceId: number): number {\n    this.work.txPairBackend?.enterRole(1);');
patch(m2,'sourceKind: number = 0): boolean {','sourceKind: number = 0): boolean {\n    this.work.txPairBackend?.enterRole(1);');
patch(m2,'    sourceHeavy: number, sourceLight: number, sourceEpoch: number): number {','    sourceHeavy: number, sourceLight: number, sourceEpoch: number): number {\n    this.work.txPairBackend?.enterRole(2);');
const firstImport=fs.readFileSync(path.join(out,field),'utf8').replaceAll('\r\n','\n').split('\n').find(line=>line.startsWith('import '));
patch(field,firstImport,"import { TxPairBackends } from './tx/TxPairBackends';\n"+firstImport);
patch(field,'export class WaterSandField {',`export class WaterSandField {
  txAttachBackend(backend:TxPairBackends):void {if(this.particleBinding===null)throw new Error('TX_GPU_NO_BINDING');this.particleBinding.work.txPairBackend=backend;}`);
patch(field,'      const status = this.particleBinding.apply(this.totalMotionBudgetScale,','      this.particleBinding.work.txPairBackend?.enterRole(3);\n      const status = this.particleBinding.apply(this.totalMotionBudgetScale,');
patch(field,'          const particleStatus = this.particleBinding.apply(scale,','          this.particleBinding.work.txPairBackend?.enterRole(4);\n          const particleStatus = this.particleBinding.apply(scale,');
const archive=v3.edits.find(e=>e.file==='physics/WaterSandField.ets'&&e.from==='  private audit(checkDrift: boolean): WaterSandErrorCode | null {');patch(field,archive.from,archive.to);
// Inclusive original CPU scopes; keep the exact body and return/first-code order.
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
function wrapCpu(file,name,kind){const text=fs.readFileSync(path.join(out,file),'utf8').replaceAll('\r\n','\n'),sf=ts.createSourceFile(file,text,ts.ScriptTarget.Latest,true,ts.ScriptKind.TS);let target=null;
 function visit(n){if((ts.isFunctionDeclaration(n)||ts.isMethodDeclaration(n))&&n.name&&n.name.getText(sf)===name&&n.body)target=n;ts.forEachChild(n,visit);}visit(sf);if(!target)throw Error('TX_GPU_CPU_SCOPE_MISSING:'+name);
 const from=target.getText(sf),body=target.body.getText(sf),inside=body.slice(1,-1),changed=from.slice(0,from.length-body.length)+'{const txScopeStart=work.txPairBackend===null?NaN:work.txPairBackend.nowPublic();try{'+inside+'}finally{if(work.txPairBackend!==null)work.txPairBackend.cpuScope('+kind+',work.txPairBackend.nowPublic()-txScopeStart);}}';patch(file,from,changed);
}
wrapCpu(hot,'auditParticleHot',0);wrapCpu('entry/src/main/ets/physics/P011ParticleM2Sink.ets','applyInto',1);
// Launcher replaces only the isolated page rendezvous. The Worker owns all
// physical work and native EGL state; UI receives a sealed archive notice.
fs.writeFileSync(path.join(out,'entry/src/main/ets/physics/T165PageLauncher.ets'),"export { t165PageLauncher } from './tx/TxGpuPageLauncher';\n");
const profileFile=path.join(out,'entry/build-profile.json5'),profile=JSON.parse(fs.readFileSync(profileFile));profile.buildOption.sourceOption.workers.push('./src/main/ets/physics/tx/TxGpuWorker.ets');fs.writeFileSync(profileFile,JSON.stringify(profile,null,2));
inputs['test/p013_compute/tools/generate_tx_gpu_project.cjs']=sha(fs.readFileSync(__filename));
const identityPayload={revision:'T212_GPU_SOURCE_V1',baseSource:v4.sourceManifestSha256,variant,inputs,changes};const sourceSha=sha(JSON.stringify(identityPayload));
for(const file of ['entry/src/main/ets/physics/T159BuildIdentity.ets','entry/src/main/cpp/p013_compute/CMakeLists.txt'])fs.writeFileSync(path.join(out,file),fs.readFileSync(path.join(out,file),'utf8').replaceAll(v4.sourceManifestSha256,sourceSha));
const manifest={revision:'T212_GPU_SOURCE_V1',sourceManifestSha256:sourceSha,variant,identityPayload,v4SourceManifest:v4,v3SourceManifest:v3,changes,inputs,productionTouched:false,abi:'arm64-v8a',instrumentation:'TX_GPU_EXPERIMENT_TRACE64_V1',treeBackend:treeReady?'T211_CORE_COMPILED':'INJECTION_INTERFACE_WAITING_T211',nativeProtocol:'UNCHANGED_WIRE_V1_CANONICAL_LE_V2',ownership:'ONE_NATIVE_OWNER_PER_WORKER_ENV'};
fs.writeFileSync(path.join(out,'t212-source-manifest.json'),JSON.stringify(manifest,null,2)+'\n');console.log(JSON.stringify({out,sourceManifestSha256:sourceSha,variant}));
