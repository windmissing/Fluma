const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'.codex/local-test/t276-v1');cp.execFileSync(process.execPath,[path.join(root,'test/t266/generate.cjs'),out],{cwd:root,stdio:'pipe'});const ets=path.join(out,'entry/src/main/ets'),edits=[];const sha=s=>crypto.createHash('sha256').update(s).digest('hex');
function patch(rel,a,b){const p=path.join(ets,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T276_VERSION:'+rel+':'+a.slice(0,55));const v=s.replace(a,b);fs.writeFileSync(p,v);edits.push({rel,from:a,to:b,sha:sha(v)});}
const model='physics/derived/P051DerivedDemoV2.ets',facade='physics/derived/P051DerivedFacadeV2.ets',page='pages/P051DerivedV2Demo.ets',field='physics/WaterSandField.ets';
for(const n of ['T276Export.ets','T276Land.ets'])fs.copyFileSync(path.join(__dirname,n),path.join(ets,'diagnostics/t248',n));
patch('diagnostics/t248/T266Clock.ets','  get acceptedSeconds():number', '  get firstBirthAcceptedSeconds():number{return this.firstBirthCommit<0?NaN:this.birthClock;}\n  get acceptedSeconds():number');
patch(model,"import { TxWorkTrace }", "import { t276Export } from '../../diagnostics/t248/T276Export';\nimport { TxWorkTrace }");
patch(model,'    private readonly owner:',`    private t276Serial:number=0;\n    t276BedSeen():boolean{return this.t266SawBed;}
    t276Cold(kind:string,payload:string):void {t276Export(kind+'|'+this.t248.runId+'|'+this.t248.stateEpoch+'|'+(++this.t276Serial),payload,(line:string):void=>{console.info(line);});}
    t276Clock(reason:string,commit:number):void {const c=this.t248;const raw=JSON.stringify({revision:'T266_CLOCK_V3',runId:c.runId,stateEpoch:c.stateEpoch,frameSerial:this.t266Frame,physicsCommit:commit,firstBirthCommit:c.firstBirthCommit<0?null:c.firstBirthCommit,firstBirthAcceptedSeconds:c.firstBirthCommit<0?null:c.firstBirthAcceptedSeconds,acceptedPhysicsSeconds:c.acceptedPhysicsSeconds,sinceFirstBirth:c.firstBirthCommit<0?null:c.secondsSinceFirstBirth,committedSubstepCount:c.committedSubstepCount,sourceRequestedSeconds:c.sourceRequestedSeconds,admittedMass:c.sourceAdmittedMass,rejectedCount:c.sourceRejectedCount,stepDropped:c.stepRowsDropped,sourceDropped:c.sourceRowsDropped,anchorReason:reason,previousLogMs:this.t266LogMs,partIndex:0,partCount:1});if(raw.length>1400)throw new Error('T276_CLOCK_BOUND');console.info('T266_CLOCK_V3 '+raw);}
    private readonly owner:`);
patch(model,'        if(!this.t266SawBed&&this.owner.t266HasBed())',"        if(this.t266Frame%60===0)this.t276Clock('PERIODIC',r.physicalCommit);\n        if(!this.t266SawBed&&this.owner.t266HasBed())");
patch(model,'this.t266SawBed=true;this.t266AnchorFrames=4;',"this.t266SawBed=true;this.t266AnchorFrames=4;this.t276Clock('FIRST_BED',r.physicalCommit);");
patch(model,'this.t266Saw8=true;this.t266AnchorFrames=4;',"this.t266Saw8=true;this.t266AnchorFrames=4;this.t276Clock('SINCE_BIRTH_8S',r.physicalCommit);");
patch(model,"                if(this.t266Heavy||!r.ok)console.info", "                if(this.t248.firstBirthCommit===r.physicalCommit&&(r.original?.admittedH??0)>0)this.t276Clock('FIRST_BIRTH',r.physicalCommit);\n                if(this.t266Heavy||!r.ok)console.info");
// Replace every legacy big-object console call in these diagnostic seams.
for(const rel of [model,page]){const p=path.join(ets,rel);let s=fs.readFileSync(p,'utf8');let count=0;s=s.replace(/console\.info\('(T266_SOURCE|T266_FAILURE|T248_LAND|T248_STEP|T266_FRAME) '\+JSON\.stringify\((.*?)\)\);/gs,(_m,k,body)=>{count++;return (rel===page?'this.model':'this')+`.t276Cold('${k}',JSON.stringify(${body}));`;});if(count!==(rel===page?1:4))throw Error('T276_LOG_SEAMS:'+rel+':'+count);fs.writeFileSync(p,s);}
patch(page,'boundedTrace:this.model.t266Drain(t266Anchor),',"boundedTrace:this.model.t266Drain(t266Anchor),landWriter:this.model.t276LandCold(),pixel:'NOT_CAPTURED_USE_SAME_TOKEN_DEVICE_PROBE',");
patch('render/P013SceneController.ets',"import { derivedPublicationGateV2 }", "import { t276Export } from '../diagnostics/t248/T276Export';\nimport { derivedPublicationGateV2 }");
{const rel='render/P013SceneController.ets',p=path.join(ets,rel);let s=fs.readFileSync(p,'utf8'),n=0;s=s.replace(/console\.info\('T248_PACK '\+JSON\.stringify\((.*?)\)\);/gs,(_m,b)=>{n++;return `t276Export('PACK|'+f.runId+'|'+f.frameId,JSON.stringify(${b}),(line:string):void=>{console.info(line);});`;});if(n!==1)throw Error('T276_PACK');fs.writeFileSync(p,s);}
patch(field,"import { T266Clock }", "import { T276Land } from '../diagnostics/t248/T276Land';\nimport { T266Clock }");
patch(field,'  t266Trace:TxWorkTrace|null=null;',`  t276Land:T276Land=new T276Land();
  private t276Page:number=-1;private t276Epoch:number=-1;
  private t276CommitLand():void {if(this.t266Trace===null||this.particleBinding===null)return;const w=this.particleBinding.work;if(this.t276Epoch!==this.renderStateEpoch){this.t276Epoch=this.renderStateEpoch;this.t276Page=-1;}if(w.stepEpoch===this.t276Page)return;this.t276Page=w.stepEpoch;const ids=new Uint32Array(w.pool.hotCanonicalBuffers()[8]),states=new Uint8Array(w.pool.hotCanonicalBuffers()[7]),e=w.events;for(let i=0;i<e.count;i++)if(e.kind[i]===2&&e.granted[i]>0){let before=-1,after=0;for(let k=0;k<w.frozenPool.nmax;k++){if(w.frozenPool.pId[k]===e.particleId[i])before=w.frozenPool.pState[k];if(ids[k]===e.particleId[i])after=states[k];}this.t276Land.add(this.p011RunId,this.renderStateEpoch,this.p011SubstepId+1,w.stepEpoch,e.particleId[i],before,after,e.granted[i],this.renderPhysicsCommitId);}}
  t266Trace:TxWorkTrace|null=null;`);
patch(field,"this.observeFieldColumnV2('COMMIT_FINISH','COMMIT');", "this.observeFieldColumnV2('COMMIT_FINISH','COMMIT');this.t276CommitLand();");
patch(facade,'        this.field.startDerivedCommandV2(OWNER_PERMIT_V2, 2);','        this.field.t276Land.begin();this.field.startDerivedCommandV2(OWNER_PERMIT_V2, 2);');
patch(facade,"this.field.finishDerivedCommandV2(OWNER_PERMIT_V2, r !== null && r.ok && fault === null);", "this.field.finishDerivedCommandV2(OWNER_PERMIT_V2, r !== null && r.ok && fault === null);this.field.t276Land.seal(this.field.getRenderPhysicsCommitId(),fault===null);");
patch(facade,'    t266HasBed():boolean',"    t276LandCold():string{return this.field.t276Land.exportCold();}\n    t266HasBed():boolean");
patch(model,'    t248LandCold(): string',"    t276LandCold():string{return this.owner.t276LandCold();}\n    t248LandCold(): string");
const cpp=path.join(out,'entry/src/main/cpp/p013renderer');fs.copyFileSync(path.join(__dirname,'pixel.h'),path.join(cpp,'t276_pixel.h'));
function native(n,a,b){const p=path.join(cpp,n),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T276_NATIVE:'+n);fs.writeFileSync(p,s.replace(a,b));}
native('scene_compositor.cpp','#include "scene_compositor.h"','#include "scene_compositor.h"\n#include "t276_pixel.h"');
native('scene_compositor.cpp','t248::sample(p.token.c_str());','t276pixel::sample(p.token.c_str(),uint64_t(p.frame.frameId),int(p.frame.surfaceWidthPx),int(p.frame.surfaceHeightPx));');
native('napi_bridge.cpp','#include "napi_bridge.h"','#include "napi_bridge.h"\n#include "t276_pixel.h"');
native('napi_bridge.cpp','  napi_property_descriptor d[] = {',`  napi_property_descriptor d[] = {
 {"t276ArmPixels",nullptr,[](napi_env e,napi_callback_info i)->napi_value{size_t n=1;napi_value v;double f=0;napi_get_cb_info(e,i,&n,&v,nullptr,nullptr);bool ok=n==1&&napi_get_value_double(e,v,&f)==napi_ok&&std::isfinite(f)&&f>0&&f<=9007199254740991.0&&std::floor(f)==f;ok=ok&&t276pixel::arm(uint64_t(f));napi_value r;napi_get_boolean(e,ok,&r);return r;},nullptr,nullptr,nullptr,napi_default,nullptr},
 {"t276Pixels",nullptr,[](napi_env e,napi_callback_info)->napi_value{auto s=t276pixel::exportCold();napi_value v;napi_create_string_utf8(e,s.c_str(),s.size(),&v);return v;},nullptr,nullptr,nullptr,napi_default,nullptr},`);
patch(page,'  this.renderer.t266CapturePacked=t266Anchor;',"  if(this.model.t276BedSeen()&&this.model.t266AnchorFrames===4)native.t276ArmPixels(this.serial);this.renderer.t266CapturePacked=t266Anchor;");
patch(page,"pixel:'NOT_CAPTURED_USE_SAME_TOKEN_DEVICE_PROBE'","pixel:native.t276Pixels()");
fs.writeFileSync(path.join(out,'t276-manifest.json'),JSON.stringify({revision:'T276_CLOCK_COLD_V3',edits,files:['T276Export.ets','T276Land.ets','pixel.h'].map(n=>({name:n,sha:sha(fs.readFileSync(path.join(__dirname,n)))})),partMaxAscii:1800,clockMaxChars:1400,productionTouched:false,pixel:'ARMED_ONCE_AT_FIRST_ANCHOR_4_SAME_TOKEN_FRAMES_DEVICE_NOT_RUN'},null,2));console.log(out);



require('../t284/apply.cjs')(out);
