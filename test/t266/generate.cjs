'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'.codex/local-test/t266-v1');
cp.execFileSync(process.execPath,[path.join(root,'test/t248/generate.cjs'),out],{cwd:root,stdio:'pipe'});
const ets=path.join(out,'entry/src/main/ets'),edits=[],sha=s=>crypto.createHash('sha256').update(s).digest('hex');
function patch(rel,a,b){const p=path.join(ets,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T266_WHITELIST:'+rel+':'+a.slice(0,60));const v=s.replace(a,b);fs.writeFileSync(p,v);edits.push({rel,from:a,to:b,sha:sha(v)});}
const diag='diagnostics/t248/T248Observation.ets',model='physics/derived/P051DerivedDemoV2.ets',facade='physics/derived/P051DerivedFacadeV2.ets',field='physics/WaterSandField.ets',page='pages/P051DerivedV2Demo.ets';
patch(model,"import { T248Observation }", "import { TxWorkTrace } from '../tx/TxWorkTrace';\nimport { T248Observation }");
fs.copyFileSync(path.join(__dirname,'T266Clock.ets'),path.join(ets,'diagnostics/t248/T266Clock.ets'));
patch(diag,"import { WaterSandSnapshot }", "import { T266Clock } from './T266Clock';\nimport { WaterSandSnapshot }");
patch(diag,'export class T248Observation {','export class T248Observation extends T266Clock {');
patch(diag,`  acceptedSeconds: number = 0; stepCount: number = 0;
  recordStep(dt: number,ok: boolean,stepped: boolean): void {
    if(ok&&stepped&&Number.isFinite(dt)&&dt>0){this.acceptedSeconds+=dt;this.stepCount++;}
  }`,'');
patch(diag,'T248_READONLY_V1','T266_READONLY_V2');
patch(field,"import { DerivedCheckpointV2 }", "import { T266Clock } from '../diagnostics/t248/T266Clock';\nimport { TxWorkTrace } from './tx/TxWorkTrace';\nimport { DerivedCheckpointV2 }");
patch(field,'export class WaterSandField {',`export class WaterSandField {
  t266Trace:TxWorkTrace|null=null;
  t266Bind(clock:T266Clock):void {clock.bind(this.p011RunId,this.renderStateEpoch);}
  t266Scope():TxWorkTrace|null {const t=this.t266Trace;if(t!==null){t.context[0]=this.p011SubstepId;t.context[1]=this.renderPhysicsCommitId;t.context[2]=this.particleBinding?.pool.nmax??0;t.context[3]=this.particleBinding===null?0:this.particleBinding.pool.nmax-this.particleBinding.pool.freeSlotCount;t.context[4]=this.renderStateEpoch;}return t;}
  t266HasBed():boolean {for(let c=0;c<this.cols;c++)if(this.mh[c]+this.ml[c]+this.bh[c]+this.bl[c]>0)return true;return false;}
`);
patch(facade,'    exportFailureCold():',`    t266Attach(trace:TxWorkTrace|null,clock:T266Clock):void{const run=clock.runId,epoch=clock.stateEpoch;this.field.t266Trace=trace;this.field.t266Bind(clock);if(trace!==null&&(run!==clock.runId||epoch!==clock.stateEpoch)){trace.count=0;trace.depth=0;trace.incomplete=false;trace.counts.fill(0);trace.wallInclusive.fill(0);trace.wallExclusive.fill(0);trace.wallCalls.fill(0);}}
    t266HasBed():boolean{return this.field.t266HasBed();}
    t266Commit():number{return this.field.getRenderPhysicsCommitId();}
    exportFailureCold():`);
patch(facade,"import { copyDerivedPayloadV2 }", "import { T266Clock } from '../../diagnostics/t248/T266Clock';\nimport { TxWorkTrace } from '../tx/TxWorkTrace';\nimport { copyDerivedPayloadV2 }");
patch(model,'    readonly t248 = new T248Observation();',`    readonly t248 = new T248Observation();
    readonly t266Trace:TxWorkTrace=new TxWorkTrace(4096);
    t266Enabled:boolean=true;
    t266Heavy:boolean=false;
    private t266Frame:number=0;
    private t266EncodeMs:number=0;
    t266LogMs:number=0;
    t266StepMs:number=0;
    t266AnchorFrames:number=0;
    private t266SawBed:boolean=false;
    private t266Saw8:boolean=false;
    t266Now:()=>number=():number=>Date.now();
    t266Drain(full:boolean=false):string {const begin=this.t266Now();const t=this.t266Trace;const stages:number[]=[125,302,266,249,239,250,166,20,304,289];const costs:number[]=[];for(const id of stages)costs.push(id,t.wallInclusive[id],t.wallExclusive[id],t.wallCalls[id]);const v=JSON.stringify({stepRows:full?this.t248.stepRows.slice(0,this.t248.stepRowCount*10):null,stepRowCount:this.t248.stepRowCount,stepRowsDropped:this.t248.stepRowsDropped,sourceRows:full?this.t248.sourceRows.slice(0,this.t248.sourceRowCount*7):null,sourceReasons:full?this.t248.sourceReasons.slice(0,this.t248.sourceRowCount):null,sourceRowsDropped:this.t248.sourceRowsDropped,fieldStepMs:this.t266StepMs,encodePreviousMs:this.t266EncodeMs,logPreviousMs:this.t266LogMs,phaseCosts:costs,clock:JSON.parse(this.t248.summary()),frame:this.t266Frame,rows:full?t.rows.slice(0,t.count*32):null,count:t.count,depth:t.depth,incomplete:t.incomplete,activeScopeId:t.depth?t.stack[t.depth-1]:null,stageContext:'substepId,physicsCommit,poolCapacity,poolLive,stateEpoch',clockDomain:'MONOTONIC_MS',wallMs:Date.now(),monotonicMs:this.t266Now()});t.count=0;t.incomplete=false;this.t266EncodeMs=this.t266Now()-begin;return v;}`);
patch(model,'        if (!this.active)\n            return true;',`        if (!this.active) return true;
        this.t266Trace.detail=true;this.t266Trace.clock=this.t266Now;this.t266Trace.root=++this.t266Frame;
        this.owner.t266Attach(this.t266Enabled?this.t266Trace:null,this.t248);`);
patch(model,'                const r = this.owner.inject(', '                const t266SourceBefore=this.owner.t266Commit();const r = this.owner.inject(');
patch(model,'        const t248Start=Date.now();const r = this.owner.step(dt);','        const t266CommitBefore=this.owner.t266Commit();const t248Start=this.t266Now();const r = this.owner.step(dt);this.t266StepMs=this.t266Now()-t248Start;');
patch(model,'stepMs:Date.now()-t248Start','stepMs:this.t266Now()-t248Start');
patch(model,'        this.t248.recordStep(r.original===null?0:r.original.substeps*r.original.substepDt,r.derivedFault===null,r.original!==null&&r.original.stepped);',`        this.t248.stepEvidence(this.t266Frame,dt,r.original?.substeps??0,r.original?.substepDt??0,t266CommitBefore,r.physicalCommit,r.ok,r.original?.errorCode??0);`);
patch(model,'\n        if (!r.ok) {',`\n        if(!this.t266SawBed&&this.owner.t266HasBed()){this.t266SawBed=true;this.t266AnchorFrames=4;}
        if(!this.t266Saw8&&this.t248.secondsSinceFirstBirth>=8){this.t266Saw8=true;this.t266AnchorFrames=4;}
        if (!r.ok) {console.info('T266_FAILURE '+JSON.stringify({result:r,boundedTrace:this.t266Drain(true)}));`);
patch(model,"                console.info('T248_SOURCE '+JSON.stringify({wallNs:ns,seconds:seconds,receipt:r}));",`                this.t248.source(seconds,r.original?.admittedH??0,(r.original?.admittedH??0)>0,r.physicalCommit,!r.ok,this.seq,t266SourceBefore,r.original?.rejectedReason??(r.ok?'NONE':'NO_ORIGINAL_RECEIPT'));
                if(this.t266Heavy||!r.ok)console.info('T266_SOURCE '+JSON.stringify({clock:JSON.parse(this.t248.summary()),sequence:this.seq,wallNs:ns,receipt:r}));`);
// Keep the old heavy arm as an explicit comparison; normal frames avoid large JSON.
for(const prefix of ["console.info('T248_STEP '","console.info('T248_LAND '"]){patch(model,'        '+prefix,'        if(this.t266Heavy)'+prefix);}
patch(model,"                hilog.info(0,", "                if(this.t266Heavy)hilog.info(0,");
patch(model,"this.seq = 0; this.status = 'READY';", "this.seq = 0; this.status = 'READY';this.t266AnchorFrames=0;this.t266SawBed=false;this.t266Saw8=false;this.owner.t266Attach(this.t266Enabled?this.t266Trace:null,this.t248);");
patch(page,"import sensor from", "import native from 'libp013renderer.so';\nimport sensor from");
patch(page,'onPageShow():void{this.active=true;',"onPageShow():void{this.model.t266Now=p013NowMs;this.model.t266Heavy=AppStorage.get<boolean>('t266heavy')===true;this.active=true;");
patch(page,"  const t248Authority=this.model.t248AuthorityCold(),t248Payload=p.exportPayloadCold(),t248Published=new T248Observation().capture(t248Payload.authority),t248RenderStart=Date.now();",`  const t266Anchor=this.model.t266Heavy||this.model.t266AnchorFrames>0;
  const t248Authority=t266Anchor?this.model.t248AuthorityCold():'NOT_SAMPLED',t248Published=t266Anchor?this.model.t248.capture(p.exportPayloadCold().authority):'NOT_SAMPLED',t248RenderStart=p013NowMs();`);
patch(page,"console.info('T248_FRAME '","if(t266Anchor||this.serial%60===0||!receipt?.currentFrameSucceeded){const t266LogBegin=p013NowMs();console.info('T266_FRAME '");
patch(page,'this.status=receipt?.currentFrameSucceeded?',"if(this.model.t266AnchorFrames>0)this.model.t266AnchorFrames--;this.status=receipt?.currentFrameSucceeded?");
patch(page,'renderInclusiveMs:Date.now()-t248RenderStart','renderInclusiveMs:p013NowMs()-t248RenderStart');
patch(page,"nativeStageTiming:'NOT_IMPLEMENTED_PLATFORM_PATCH_REQUIRED'","runClock:JSON.parse(this.model.t248.summary()),boundedTrace:this.model.t266Drain(t266Anchor),nativeStages:native.t266Drain()");
patch(page,"nativeStages:native.t266Drain()}));","nativeStages:native.t266Drain()}));this.model.t266LogMs=p013NowMs()-t266LogBegin;}");
patch('render/P013SceneController.ets','export class P013SceneController {','export class P013SceneController {t266CapturePacked:boolean=false;');
patch('render/P013SceneController.ets',"      console.info('T248_PACK '","      if(this.t266CapturePacked||AppStorage.get<boolean>('t266heavy')===true)console.info('T248_PACK '");
patch(page,'  const receipt=p.drawNewPipeline','  this.renderer.t266CapturePacked=t266Anchor;const receipt=p.drawNewPipeline');
// Original preexisting wrappers preserve firstCode/receipt/rollback. Inject only fallback trace.
{const p=path.join(ets,field),s=fs.readFileSync(p,'utf8'),needle='this.txD1===null?null:this.txD1.trace';if(s.split(needle).length!==207)throw Error('T266_FIELD_SCOPE_VERSION');fs.writeFileSync(p,s.replaceAll(needle,'this.txD1===null?this.t266Scope():this.txD1.trace'));}
// The known default must occur once; reject unknown versions rather than silent replacement.
{const p=path.join(ets,'entryability/EntryAbility.ets'),s=fs.readFileSync(p,'utf8'),matches=[...s.matchAll(/const probePage: string =[\s\S]*?;/g)];if(matches.length!==1||!matches[0][0].includes(": 'pages/SandCanvas'"))throw Error('T266_ROUTING_VERSION');fs.writeFileSync(p,s.replace(matches[0][0],"const probePage: string = 'pages/P051DerivedV2Demo';").replace('    this.applyWant(want.parameters, false);',"    AppStorage.setOrCreate('t266heavy', `${want.parameters?.t266heavy}` === 'true');\n    this.applyWant(want.parameters, false);"));}
const cpp=path.join(out,'entry/src/main/cpp/p013renderer');fs.copyFileSync(path.join(__dirname,'native_observation.h'),path.join(cpp,'t248_observation.h'));
{const p=path.join(cpp,'napi_bridge.cpp'),s=fs.readFileSync(p,'utf8'),a='  napi_property_descriptor d[] = {';if(s.split(a).length!==2)throw Error('T266_NAPI_VERSION');fs.writeFileSync(p,s.replace('#include "napi_bridge.h"','#include "napi_bridge.h"\n#include "t248_observation.h"').replace(a,`  napi_property_descriptor d[] = {
    {"t266Drain",nullptr,[](napi_env e,napi_callback_info)->napi_value{auto s=t248::drain();napi_value v;napi_create_string_utf8(e,s.c_str(),s.size(),&v);return v;},nullptr,nullptr,nullptr,napi_default,nullptr},`));}
{const p=path.join(cpp,'scene_gpu.cpp'),s=fs.readFileSync(p,'utf8'),needle='code=layers.Upload(p.data.data());';if(s.split(needle).length!==2)throw Error('T266_UPLOAD_VERSION');fs.writeFileSync(p,s.replace('#include "scene_gpu.h"','#include "scene_gpu.h"\n#include "t248_observation.h"').replace(needle,'const double t266Upload=t248::now();code=layers.Upload(p.data.data());t248::timing(p.token.c_str(),"UPLOAD_EXCLUSIVE",t266Upload,int(code));'));}
fs.writeFileSync(path.join(out,'t266-manifest.json'),JSON.stringify({revision:'T266_BOUNDED_DIAGNOSTICS_V2',sourceFiles:['T266Clock.ets','native_observation.h'].map(n=>({name:n,sha:sha(fs.readFileSync(path.join(__dirname,n)))})),edits,scopeSites:206,scopeCapacity:4096,route:'pages/P051DerivedV2Demo',heavyArmWant:'t266heavy=true',productionTouched:false,pending:['inflight watchdog export if UI cannot drain: NOT_CAPTURED','pixel probe requires explicit device capture'],phaseDictionary:{125:'consumePendingGestureSand',302:'saveBackup',266:'planAndCommitTilt',249:'substep collect',239:'restoreMotionSnapshot',250:'applyMotionChannels',166:'recordEndpointCommit',20:'finishNaturalObservation',304:'changedSinceBackup',289:'updateMetrics'}},null,2));console.log(out);
