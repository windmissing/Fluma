const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t385/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets'),cpp=path.join(out,'entry/src/main/cpp/p013renderer');
const file=(root,name)=>path.join(root,name);
const read=(root,name)=>fs.readFileSync(file(root,name),'utf8').replaceAll('\r\n','\n');
function edit(root,name,before,after){
 const s=read(root,name);
 if(s.split(before).length!==2)throw Error('T394_ANCHOR '+name+' '+before.slice(0,100));
 fs.writeFileSync(file(root,name),s.replace(before,after));
}
const local=name=>read(__dirname,name).trimEnd();
fs.copyFileSync(file(__dirname,'T394Phase.ets'),file(ets,'render/T394Phase.ets'));

const fixture='render/RoiFixture.ets';
edit(ets,fixture,'export function chooseWindowFixture(', `export class WindowFixtureResult {
  readonly status:string;
  readonly roi:string;
  constructor(status:string,roi:string=''){this.status=status;this.roi=roi;}
}
export function inspectWindowFixture(`);
edit(ets,fixture,"meta:Uint32Array):string {","meta:Uint32Array):WindowFixtureResult {");
edit(ets,fixture,"||!p.enabled||p.count<1)return '';","||!p.enabled)return new WindowFixtureResult('GEOMETRY_UNSUPPORTED');");
edit(ets,fixture,"if(right-left<16||top-bottom<16)return '';",
 "if(right-left<16||top-bottom<16)return new WindowFixtureResult('GEOMETRY_UNSUPPORTED');\n  if(!profile.subarray(0,128).some((v:number)=>v>0))return new WindowFixtureResult('EMPTY_BED');");
edit(ets,fixture,"Math.min(32,Math.max(2,Math.ceil(r)+2))","Math.min(32,Math.max(8,Math.ceil(r)+2))");
edit(ets,fixture,"  if(air.length===0)return '';\n",'');
edit(ets,fixture,"  return bedRoi.length!==0&&bg.length!==0?bedRoi+';'+air+';'+bg:'';",`  if(!bedRoi)return new WindowFixtureResult('BED_ROI_UNSUPPORTED');
  if(!air)return new WindowFixtureResult('AIR_ROI_UNSUPPORTED');
  if(!bg)return new WindowFixtureResult('BG_ROI_UNSUPPORTED');
  return new WindowFixtureResult('READY',bedRoi+';'+air+';'+bg);`);
fs.appendFileSync(file(ets,fixture),`
export function chooseWindowFixture(p:SingleSandPacketV1,profile:Float32Array,xy:Float32Array,meta:Uint32Array):string {
  return inspectWindowFixture(p,profile,xy,meta).roi;
}
`);

const cold='render/SingleSandWindowCold.ets';
edit(ets,cold,"import { DiagnosticColdFiles }", "import { T394Archive } from './T394Phase';\nimport { DiagnosticColdFiles }");
edit(ets,cold,'export interface WindowProbeEvidence {','export interface WindowProbeEvidence {\n  phase?:T394Archive;');
edit(ets,cold,'interface WindowArchive {','interface WindowArchive {\n  phase?:T394Archive;');
edit(ets,cold,'const archive:WindowArchive={probe:m,candidate:candidate,back:back};',
 'const archive:WindowArchive={probe:m,candidate:candidate,back:back,phase:evidence.phase};');
edit(ets,cold,"'t385-'+m.requestId","'t394-'+m.requestId");

const field='physics/WaterSandField.ets';
edit(ets,field,'import { P011Z08Archive }', "import { T394Step, T394Land } from '../render/T394Phase';\nimport { P011Z08Archive }");
edit(ets,field,'initializeDryParticleDemo(config: P051DryRunConfig):',
 'initializeDryParticleDemo(config: P051DryRunConfig, t394Bed:boolean=false):');
edit(ets,field,"this.dryResetPending=true;this.drySourceId=config.sourceId;\n    this.initialize({waterRegions:[],heavyDepositedRegions:[],",
 "this.dryResetPending=true;this.drySourceId=config.sourceId;\n    this.initialize({waterRegions:[],heavyDepositedRegions:t394Bed?[{startCol:0,endColInclusive:127,bottom:0,height:48}]:[],");
edit(ets,field,'  private z08Archive:',`  private t394Steps:T394Step[]|null=null;
  private t394PendingStep:T394Step|null=null;
  t394StartTrace():void {this.t394Steps=[];}
  t394FinishTrace():T394Step[] {
    const rows=this.t394Steps;this.t394Steps=null;this.t394PendingStep=null;return rows??[];
  }
  private t394CollectLand(dt:number):void {
    const rows=this.t394Steps,events=this.particleBinding?.work.events;
    if(rows===null||rows.length>=2||events===undefined)return;
    const row=new T394Step(this.renderPhysicsCommitId+1,dt,this.effectiveTilt);
    for(let i=0;i<events.count;i++){
      if(events.kind[i]!==2||events.granted[i]<=0)continue;
      if(row.lands.length===8){row.landOverflow=true;break;}
      row.lands.push(new T394Land(events.particleId[i],events.column[i],events.requested[i],events.granted[i]));
    }
    this.t394PendingStep=row;
  }
  private t394Commit():void {
    const row=this.t394PendingStep;
    if(row!==null&&row.commit===this.renderPhysicsCommitId)this.t394Steps?.push(row);
    this.t394PendingStep=null;
  }
  private z08Archive:`);
edit(ets,field,'    const archive=this.z08Archive, crossing=', '    this.t394Commit();\n    const archive=this.z08Archive, crossing=');
edit(ets,field,'      // Production particle adapter already debited the real total ledger.',
 '      this.t394CollectLand(dt);\n      // Production particle adapter already debited the real total ledger.');

const facade='physics/derived/P051DerivedFacadeV2.ets';
edit(ets,facade,'import { CROSS_VERTICAL_V1', "import { T394Step } from '../../render/T394Phase';\nimport { CROSS_VERTICAL_V1");
edit(ets,facade,'private create(checkpoint: DerivedCheckpointV2 | null = null):',
 'private create(checkpoint: DerivedCheckpointV2 | null = null,t394Bed:boolean=false):');
edit(ets,facade,'particleParams: p });','particleParams: p },t394Bed);');
edit(ets,facade,'    setTilt(degrees: number): void {',`    resetT394Bed():void {this.field=this.create(null,true);this.stopped=false;}
    t394StartTrace():void {this.field.t394StartTrace();}
    t394FinishTrace():T394Step[] {return this.field.t394FinishTrace();}
    setTilt(degrees: number): void {`);

const model='physics/derived/P051DerivedDemoV2.ets';
edit(ets,model,'import { P051_DRY_REVISION }', "import { T394Step } from '../../render/T394Phase';\nimport { P051_DRY_REVISION, P051DryAdmissionReceipt }");
edit(ets,model,'    private held = false;',`    private t394Admissions:P051DryAdmissionReceipt[]=[];
    startT394Fixture(bed:boolean,time:number):void {
        this.reset();
        if(bed)this.owner.resetT394Bed();
        this.owner.setTilt(0);this.owner.setShakeInput(0);
        this.t394Inject(bed?64:32,time);
    }
    t394InjectB(time:number):void {this.t394Inject(96,time);}
    private t394Inject(x:number,time:number):void {
        const r=this.owner.inject({revision:P051_DRY_REVISION,sourceId:51,sequence:++this.seq,inputTimeMs:time,
            x:x,y:128,seed:51,requestedH:0.1,requestedL:0});
        if(r.original!==null)this.t394Admissions.push(r.original);
        if(!r.ok)this.status=r.original?.rejectedReason??'FIXTURE_ADMISSION_FAILED';
    }
    t394InputReceipts():P051DryAdmissionReceipt[] {return this.t394Admissions.slice();}
    t394StartTrace():void {this.owner.t394StartTrace();}
    t394FinishTrace():T394Step[] {return this.owner.t394FinishTrace();}
    private held = false;`);
edit(ets,model,"this.stop(); this.owner.reset(); this.seq = 0;", "this.stop(); this.owner.reset(); this.seq = 0;this.t394Admissions.length=0;");

const ctrl='render/P013SingleSandController.ets';
edit(ets,ctrl,"import { chooseWindowFixture, windowFixtureKey } from './RoiFixture';",
 `import { windowFixtureKey } from './RoiFixture';
import { p013NowMs } from './P013RenderController';
import { P051DryAdmissionReceipt } from '../physics/P051DryParticleAdmission';
import { T394Archive, T394Hold, T394Input, T394Phase, t394Geometry, t394DynamicRoi } from './T394Phase';`);
edit(ets,ctrl,'SingleSandConfigV1,SingleSandGrainV1,SingleSandNative,SingleSandReceipt',
 'SingleSandConfigV1,SingleSandGrainV1,SingleSandNative,SingleSandReceipt,SingleSandPacketV1');
const ctrlSource=read(ets,ctrl);
const start=ctrlSource.indexOf('  windowFixtureKey():');
const end=ctrlSource.indexOf('  armWindowProbe(',start);
edit(ets,ctrl,ctrlSource.slice(start,end),local('controller.inc')+'\n');
edit(ets,ctrl,'    if(!Number.isFinite(scale)', "    this.t394PrepareCut='GEOMETRY';\n    if(!Number.isFinite(scale)");
edit(ets,ctrl,"    this.t394PrepareCut='GEOMETRY';","    this.t394PrepareCode=null;\n    this.t394PrepareCut='GEOMETRY';");
edit(ets,ctrl,'    this.failure=this.preparation.pack(state,context);',
 "    this.t394PrepareCut='PACK';\n    this.failure=this.preparation.pack(state,context);");
edit(ets,ctrl,'    if(this.session===0){', "    this.t394PrepareCut='TARGET';\n    if(this.session===0){");
edit(ets,ctrl,"if(init.code!==0){this.failure='SINGLE_TARGET_UNSUPPORTED';return false;}",
 "if(init.code!==0){this.t394PrepareCode=init.code;this.failure='SINGLE_TARGET_UNSUPPORTED';return false;}");
edit(ets,ctrl,"if(ready.code!==0||ready.sizeSource!=='EGL_QUERY_SURFACE'){",
 "if(ready.code!==0||ready.sizeSource!=='EGL_QUERY_SURFACE'){\n      this.t394PrepareCode=ready.code;");
edit(ets,ctrl,'    if(!this.prepareSingleSandOptics())', "    this.t394PrepareCut='OPTICS';\n    if(!this.prepareSingleSandOptics())");
edit(ets,ctrl,'    if(!this.prepareSingleSandFrame(state,context,scale))return null;\n    const prep=this.preparation;',
 `    const stamp=this.t394WantsEvidence()?p013NowMs():0;
    if(!this.prepareSingleSandFrame(state,context,scale)){
      this.rejectT394(this.t394PrepareCut+':'+this.failure);return null;
    }
    const packMs=stamp===0?0:p013NowMs()-stamp;
    const prep=this.preparation;
    const prior=this.t394.sample;
    if(prior!==null&&(prior.geometry!==t394Geometry(prep.packet)||prior.fixtureKey!==windowFixtureKey(prep.packet)))
      this.rejectT394('STALE_INPUT');`);
edit(ets,ctrl,'    let request=this.pendingRoi;', '    this.t394ConfigureAuto(prep.packet);\n    const drawStamp=stamp===0?0:p013NowMs();\n    let request=this.pendingRoi;');
edit(ets,ctrl,'request.fixtureKey!==this.windowFixtureKey()', 'request.fixtureKey!==windowFixtureKey(prep.packet)');
edit(ets,ctrl,"this.failure='NATIVE_PROBE_EXCEPTION';return null;","this.failure='NATIVE_PROBE_EXCEPTION';this.rejectT394(this.failure);return null;");
edit(ets,ctrl,'    this.lastReceipt=receipt;',`    this.t394AfterDraw(state,receipt,packMs,drawStamp===0?0:p013NowMs()-drawStamp,request);
    this.lastReceipt=receipt;`);

const page='pages/P051DerivedV2Demo.ets';
edit(ets,page,'import { WindowProbeEvidence', "import { T394Input } from '../render/T394Phase';\nimport { WindowProbeEvidence");
edit(ets,page,' private async exportWindow()',local('page.inc')+'\n private async exportWindow()');
edit(ets,page,'tick(ns:number):void{this.queued=false;if(!this.active)return;', 'tick(ns:number):void{this.queued=false;if(!this.active||this.singleSand.t394Hold()!==null)return;');
edit(ets,page,'if(input!==null&&!this.fixtureLocked)', "if(input!==null&&!this.fixtureLocked&&this.t394Arm.length===0)");
edit(ets,page,"  if(!this.fixtureLocked&&!this.model.tick(ns,dt)){this.model.budget.outcome(3);this.status=this.model.status;this.schedule();return;}const copy=this.model.copyPublicationInto(this.publication);if(copy!=='OK'){this.model.budget.outcome(4);this.status=copy;this.schedule();return;}",`  const trace=this.singleSand.t394WantsEvidence();
  if(trace)this.model.t394StartTrace();
  const ok=this.fixtureLocked||this.model.tick(ns,dt);
  const steps=trace?this.model.t394FinishTrace():[];
  if(!ok){
   if(trace)this.singleSand.t394Frame(new T394Input(ns,0,dt,this.publication.effectiveTiltDeg,steps,this.model.t394InputReceipts()));
   this.singleSand.rejectT394('TICK:'+this.model.status);this.model.budget.outcome(3);this.status=this.model.status;this.schedule();return;
  }
  const copyStart=trace?p013NowMs():0;
  const copy=this.model.copyPublicationInto(this.publication);
  if(trace)this.singleSand.t394Frame(new T394Input(ns,p013NowMs()-copyStart,this.fixtureLocked?0:dt,
    this.publication.effectiveTiltDeg,steps,this.model.t394InputReceipts()));
  if(copy!=='OK'){this.singleSand.rejectT394('COPY:'+copy);this.model.budget.outcome(4);this.status=copy;this.schedule();return;}`);
edit(ets,page,"if(this.fixtureLocked)return;","if(this.fixtureLocked||this.t394Arm.length>0)return;");
edit(ets,page,"if(value.length===0){this.probeStatus='夹具未就绪：需床、AIR 和背景';return;}",
 "if(value.length===0){this.probeStatus=this.singleSand.t394PhaseStatus();return;}");
edit(ets,page,"this.singleSand.cancelWindowProbe('FIXTURE_CHANGED');this.probeFixtureKey='';this.model.stop();this.fixtureLocked=!this.fixtureLocked;",
 "this.t394Arm='';this.t394Phase(this.fixtureLocked?'UNLOCKED':'LOCKED',!this.fixtureLocked);");
edit(ets,page,"this.singleSand.cancelWindowProbe('PAGE_HIDDEN');this.active=false;",
 "this.singleSand.rejectT394('PAGE_HIDDEN');this.active=false;");
edit(ets,page,"this.singleSand.cancelWindowProbe('GEOMETRY_CHANGED');this.probeFixtureKey='';",
 "this.t394GeometryEpoch++;this.singleSand.rejectT394('GEOMETRY_CHANGED');this.probeFixtureKey='';this.schedule();");
edit(ets,page,'geometryEpoch:1,frameSerial:', 'geometryEpoch:this.t394GeometryEpoch,frameSerial:');
edit(ets,page,'   const receipt=p.drawSingleSand(this.singleSand,c,this.getUIContext().vp2px(1));',
 `   const receipt=p.drawSingleSand(this.singleSand,c,this.getUIContext().vp2px(1));
   if(this.probeStatus==='WAITING_LOCKED_FRAME'){
    const hold=this.singleSand.t394Hold(),cut=this.singleSand.t394PhaseStatus();
    if(hold!==null)this.probeStatus='CAPTURED · '+hold.name;
    else if(cut!=='WAITING_LOCKED_FRAME')this.probeStatus=cut;
   }`);
edit(ets,page,'    Text(this.status).fontSize(12)','    Text(this.status).fontSize(12).maxLines(1).textOverflow({overflow:TextOverflow.Ellipsis})');
edit(ets,page,"this.singleSand.cancelWindowProbe('RESET');this.probeFixtureKey='';this.fixtureLocked=false;",
 "this.t394Arm='';this.t394Phase('LIVE',false);");
edit(ets,page,"Button('中性').onClick(()=>this.singleSand.setProbeNeutral(true))","Button('中性').onClick(()=>this.t394SetOptics(true))");
edit(ets,page,"Button('纹理').onClick(()=>this.singleSand.setProbeNeutral(false))","Button('纹理').onClick(()=>this.t394SetOptics(false))");
edit(ets,page,"    Text(this.probeStatus).fontSize(12)",`    Row({space:4}){
      Button('厚床三区').onClick(()=>this.t394Start('STATIC'))
      Button('对照 A').onClick(()=>this.t394Start('CONTROL'))
      Button('锁定 A').onClick(()=>this.t394Start('LOCK'))
      Button('注入 B').onClick(()=>this.t394Next())
    }
    Row({space:4}){
      Button('封存截图时段').onClick(()=>this.t394SealHold())
      Button('导出阶段').onClick(()=>this.t394ExportPhase())
      Button('重锁').onClick(()=>{if(this.t394Arm==='LOCK')this.t394Phase('LOCK_RELOCK',true,false,true);})
    }
    Text(this.probeStatus).fontSize(12).maxLines(1).textOverflow({overflow:TextOverflow.Ellipsis})`);

const probe='window_probe.h',pass='single_sand_pass.cpp',native='single_sand_napi.inc',napi='window_probe_napi.inc';
edit(cpp,probe,' double ms=0;',` double ms=0;
 uint64_t candidateNs=0,backNs=0,captureEndNs=0,swapStartNs=0,swapEndNs=0,unbindEndNs=0;
 static uint64_t SteadyNs(){
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
   std::chrono::steady_clock::now().time_since_epoch()).count());
 }`);
edit(cpp,probe,'  ReadTarget(0,candidate,GL_COLOR_ATTACHMENT0);\n  ReadTarget(1,0,GL_BACK);',
 '  candidateNs=SteadyNs();ReadTarget(0,candidate,GL_COLOR_ATTACHMENT0);\n  backNs=SteadyNs();ReadTarget(1,0,GL_BACK);');
edit(cpp,probe,'  ms=std::chrono::duration<double', '  captureEndNs=SteadyNs();\n  ms=std::chrono::duration<double');
edit(cpp,pass,' if(egl.Swap(p.frame)!=Status::OK){out.actual.clear();return fail();}',
 ` if(probe)probe->swapStartNs=SingleSandWindowProbe::SteadyNs();
 const auto swap=egl.Swap(p.frame);
 if(probe)probe->swapEndNs=SingleSandWindowProbe::SteadyNs();
 if(swap!=Status::OK){out.actual.clear();return fail();}`);
edit(cpp,native,'  windowProbe.target={};windowProbe.ms=0;',
 `  windowProbe.candidateNs=windowProbe.backNs=windowProbe.captureEndNs=0;
  windowProbe.swapStartNs=windowProbe.swapEndNs=windowProbe.unbindEndNs=0;
  windowProbe.target={};windowProbe.ms=0;`);
edit(cpp,native,' const auto unbind=g.egl.Unbind();', ' const auto unbind=g.egl.Unbind();\n if(probe)probe->unbindEndNs=SingleSandWindowProbe::SteadyNs();');
edit(cpp,napi,' Put(e,o,"captureStatus",',` Put(e,o,"candidateNs",std::to_string(probe.candidateNs).c_str());
 Put(e,o,"backNs",std::to_string(probe.backNs).c_str());
 Put(e,o,"captureEndNs",std::to_string(probe.captureEndNs).c_str());
 Put(e,o,"swapStartNs",std::to_string(probe.swapStartNs).c_str());
 Put(e,o,"swapEndNs",std::to_string(probe.swapEndNs).c_str());
 Put(e,o,"unbindEndNs",std::to_string(probe.unbindEndNs).c_str());
 Put(e,o,"captureStatus",`);
const names=['generate.cjs','T394Phase.ets','controller.inc','page.inc'];
const sourceSha256=Object.fromEntries(names.map(n=>[n,crypto.createHash('sha256').update(fs.readFileSync(file(__dirname,n))).digest('hex')]));
fs.writeFileSync(file(out,'t394-manifest.json'),JSON.stringify({revision:'T394_SINGLE_SAND_PHASE_V1',isolated:true,device:'NOT_RUN',sourceSha256},null,2));
