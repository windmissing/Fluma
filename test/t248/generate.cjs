'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'.codex/local-test/t248-project');
if(!out.startsWith(path.join(root,'.codex/local-test')+path.sep)||fs.existsSync(out))throw Error('FRESH_ISOLATED_OUTPUT_REQUIRED');
let files={},edits=[];const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
function copy(a,b){const st=fs.statSync(a);if(st.isDirectory()){fs.mkdirSync(b,{recursive:true});for(const n of fs.readdirSync(a))if(!['build','.hvigor','oh_modules'].includes(n))copy(path.join(a,n),path.join(b,n));}else{const bytes=fs.readFileSync(a);fs.mkdirSync(path.dirname(b),{recursive:true});fs.writeFileSync(b,bytes);files[path.relative(root,a)]=sha(bytes);}}
for(const n of ['AppScope','hvigor','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5','local.properties'])copy(path.join(root,n),path.join(out,n));
for(const n of ['src/main','build-profile.json5','hvigorfile.ts','oh-package.json5'])copy(path.join(root,'entry',n),path.join(out,'entry',n));
fs.symlinkSync(path.join(root,'oh_modules'),path.join(out,'oh_modules'),'junction');
const ets=path.join(out,'entry/src/main/ets');
function patch(rel,a,b){let p=path.join(ets,rel),orig=fs.readFileSync(p,'utf8'),s=orig.replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('WHITELIST:'+rel+':'+a);const value=s.replace(a,b);if(value.replace(b,a)!==s)throw Error('INVERSE');fs.writeFileSync(p,value);edits.push({rel,from:a,to:b,originalSha:sha(orig),generatedSha:sha(value)});}
copy(path.join(__dirname,'T248Observation.ets'),path.join(ets,'diagnostics/t248/T248Observation.ets'));
const model='physics/derived/P051DerivedDemoV2.ets';
patch(model,"import { P051GravityInput }", "import { T248Observation } from '../../diagnostics/t248/T248Observation';\nimport { P051GravityInput }");
patch(model,'    private readonly owner: P051DerivedFacadeV2;','    readonly t248 = new T248Observation();\n    t248AuthorityCold(): string {const c=this.owner.exportCheckpointCold();return JSON.stringify({runId:c.runId,stateEpoch:c.stateEpoch,commit:c.commit,observation:this.t248.capture(c.authority)});}\n    private readonly owner: P051DerivedFacadeV2;');
patch('physics/derived/P051DerivedFacadeV2.ets','    exportFailureCold():',"    t248LandCold(): string {return this.field.t248LandCold();}\n    exportFailureCold():");
patch('physics/WaterSandField.ets','export class WaterSandField {',`export class WaterSandField {
  t248LandCold(): string {
    const binding=this.particleBinding;if(binding===null)return '[]';
    const w=binding.work,e=w.events,rows:string[]=[];
    for(let i=0;i<e.count;i++)if(e.kind[i]===2)rows.push(JSON.stringify({kind:'LAND',particleId:e.particleId[i],granted:e.granted[i],column:e.column[i],requestOrdinal:e.requestOrdinal[i],substep:w.sealedPlan.substepId,epoch:w.sealedPlan.stepEpoch,page:w.committedPageToken}));
    return '['+rows.join(',')+']';
  }`);
patch(model,"    t248AuthorityCold(): string {", "    t248LandCold(): string {return this.owner.t248LandCold();}\n    t248AuthorityCold(): string {");
patch(model,'        const r = this.owner.step(dt);','        const t248Start=Date.now();const r = this.owner.step(dt);\n        this.t248.recordStep(r.original===null?0:r.original.substeps*r.original.substepDt,r.derivedFault===null,r.original!==null&&r.original.stepped);\n        console.info(\'T248_STEP \'+JSON.stringify({dt:dt,accepted:r.derivedFault===null&&r.original!==null&&r.original.stepped,original:r.original,commit:r.physicalCommit,stepMs:Date.now()-t248Start,cumulative:this.t248.acceptedSeconds}));');
patch(model,"        this.t248.recordStep(r.original===null?0:r.original.substeps*r.original.substepDt,r.derivedFault===null,r.original!==null&&r.original.stepped);", "        this.t248.recordStep(r.original===null?0:r.original.substeps*r.original.substepDt,r.derivedFault===null,r.original!==null&&r.original.stepped);\n        console.info('T248_LAND '+JSON.stringify({commit:r.physicalCommit,accepted:r.ok,events:this.t248LandCold(),scope:'last original committed event page; deduplicate by requestKey/particle/substep/commit'}));");
patch(model,'                hilog.info(0,',"                console.info('T248_SOURCE '+JSON.stringify({wallNs:ns,seconds:seconds,receipt:r}));\n                hilog.info(0,");
const page='pages/P051DerivedV2Demo.ets';
patch(page,"import sensor from", "import { T248Observation } from '../diagnostics/t248/T248Observation';\nimport sensor from");
patch(page,'const copy=this.model.copyPublicationInto(this.publication);','const t248CopyStart=Date.now();const copy=this.model.copyPublicationInto(this.publication);const t248CopyMs=Date.now()-t248CopyStart;');
patch(page,'  const receipt=p.drawNewPipeline',"  const t248Authority=this.model.t248AuthorityCold(),t248Payload=p.exportPayloadCold(),t248Published=new T248Observation().capture(t248Payload.authority),t248RenderStart=Date.now();\n  const receipt=p.drawNewPipeline");
patch(page,"this.status=receipt?.currentFrameSucceeded?", "console.info('T248_FRAME '+JSON.stringify({frame:this.serial,wallNs:ns,dt:dt,runId:p.runId,epoch:p.stateEpoch,commit:p.physicsCommitId,publicationMs:t248CopyMs,renderInclusiveMs:Date.now()-t248RenderStart,authority:t248Authority,publication:t248Published,receipt:receipt,nativeStageTiming:'NOT_IMPLEMENTED_PLATFORM_PATCH_REQUIRED'}));this.status=receipt?.currentFrameSucceeded?");
patch('render/P013SceneController.ets',"      f.sourceFailure=this.preparation.pack(state,context);", "      const t248PackStart=Date.now();f.sourceFailure=this.preparation.pack(state,context);\n      console.info('T248_PACK '+JSON.stringify({token:f.frameToken,run:f.runId,commit:f.physicsCommitId,epoch:f.stateEpoch,frame:f.frameId,packMs:Date.now()-t248PackStart,viewport:[f.surfaceWidthPx,f.surfaceHeightPx],data:Array.from(this.preparation.data),status:f.sourceFailure}));");
copy(path.join(__dirname,'native_observation.h'),path.join(out,'entry/src/main/cpp/p013renderer/t248_observation.h'));
function nativePatch(a,b){const rel=path.join(out,'entry/src/main/cpp/p013renderer/scene_compositor.cpp'),s=fs.readFileSync(rel,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T248_NATIVE_WHITELIST:'+a);const v=s.replace(a,b);fs.writeFileSync(rel,v);edits.push({rel:'cpp/scene_compositor.cpp',from:a,to:b,originalSha:sha(s),generatedSha:sha(v)});}
nativePatch('#include "scene_compositor.h"','#include "scene_compositor.h"\n#include "t248_observation.h"');
nativePatch(' auto status=gpu.Begin(p);', ' const double t248Begin=t248::now();auto status=gpu.Begin(p);t248::timing(p.token.c_str(),"TARGET_BEGIN_UPLOAD_INCLUSIVE",t248Begin,int(status));');
nativePatch('status=gpu.Layer(layer,p,*config,lease.desc?&lease:nullptr);', 'const double t248Layer=t248::now();status=gpu.Layer(layer,p,*config,lease.desc?&lease:nullptr);t248::timing(p.token.c_str(),stages[layer],t248Layer,int(status));');
nativePatch(' status=gpu.Present(false);', ' t248::sample(p.token.c_str());const double t248Present=t248::now();status=gpu.Present(false);t248::timing(p.token.c_str(),"PRESENT",t248Present,int(status));');
nativePatch(' status=gpu.Swap(uint64_t(p.frame.frameId));', ' const double t248Swap=t248::now();status=gpu.Swap(uint64_t(p.frame.frameId));t248::timing(p.token.c_str(),"SWAP",t248Swap,int(status));');
// Keep original default untouched; this clone opens only the observed V2 page.
fs.writeFileSync(path.join(out,'entry/src/main/resources/base/profile/main_pages.json'),JSON.stringify({src:['pages/P051DerivedV2Demo']}));
let ability=path.join(ets,'entryability/EntryAbility.ets'),s=fs.readFileSync(ability,'utf8');s=s.replaceAll("'pages/Index'","'pages/P051DerivedV2Demo'");fs.writeFileSync(ability,s);
fs.writeFileSync(path.join(out,'t248-manifest.json'),JSON.stringify({revision:'T248_ISOLATED_V1',files,edits,productionTouched:false,pending:['standalone upload-exclusive native scope: current TARGET_BEGIN_UPLOAD_INCLUSIVE and layer inclusive only','device pixel/GL result verification']},null,2));console.log(out);
