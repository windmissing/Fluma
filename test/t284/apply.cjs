// Applied after T276's exact source seams. Only the isolated generated project.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
module.exports=function(out){const ets=path.join(out,'entry/src/main/ets'),model='physics/derived/P051DerivedDemoV2.ets',page='pages/P051DerivedV2Demo.ets';
function edit(rel,fn){const p=path.join(ets,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');fs.writeFileSync(p,fn(s));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T284_SEAM '+a.slice(0,70));return s.replace(a,b);}
fs.copyFileSync(path.join(__dirname,'T284Cold.ets'),path.join(ets,'diagnostics/t248/T284Cold.ets'));
edit('diagnostics/t248/T248Observation.ets',s=>{
 s=one(s,'interface T248BedColumns {bh:number[];mh:number[];}','interface T248BedColumns {bh:number[];bl:number[];mh:number[];ml:number[];}\ninterface T284ParticleRows {id:number[];state:number[];mass:number[];x:number[];y:number[];vx:number[];vy:number[];}');
 s=one(s,'{bh:Array.from(s.bh),mh:Array.from(s.mh)}','{bh:Array.from(s.bh),bl:Array.from(s.bl),mh:Array.from(s.mh),ml:Array.from(s.ml)}');
 s=one(s,'    return JSON.stringify({revision:this.revision,',"    const particleRows:T284ParticleRows|null=p===null||p===undefined?null:{id:Array.from(p.pId),state:Array.from(p.pState),mass:Array.from(p.pMass),x:Array.from(p.pX),y:Array.from(p.pY),vx:Array.from(p.pVx),vy:Array.from(p.pVy)};\n    return JSON.stringify({revision:this.revision,");
 s=one(s,'bedColumns:bedColumns,landEvents:',"particleState:particleRows,bedColumns:bedColumns,landEvents:");return s;
});
edit(model,s=>{
 s=one(s,"import { t276Export } from '../../diagnostics/t248/T276Export';","import { t284Cold } from '../../diagnostics/t248/T284Cold';");
 s=s.replace(/    t276Cold\(kind:string,payload:string\):void \{[^\n]+/,'    t276Cold(kind:string,payload:string):void {t284Cold.enqueue(kind,payload);}')
 s=one(s,'    private t276Serial:number=0;','    t284Reason:string=\'\';\n    t284Choose(frame:number,commit:number):boolean {t284Cold.enabled=this.t266Heavy;t284Cold.now=this.t266Now;t284Cold.bind(this.t248.runId,this.t248.stateEpoch);this.t284Reason=t284Cold.select(frame,commit);return this.t284Reason!==\'\';}\n    t284Finish():void {t284Cold.finish();}\n    t284Drain():void {t284Cold.drain(!this.held);}\n    t284Manual(frame:number):void {t284Cold.manualFrame=frame;}');
 s=one(s,'this.t266Trace.detail=true;','t284Cold.enabled=this.t266Heavy;t284Cold.now=this.t266Now;this.t266Trace.detail=false;');
 s=one(s,"this.t266SawBed=true;this.t266AnchorFrames=4;this.t276Clock('FIRST_BED',r.physicalCommit);","this.t266SawBed=true;t284Cold.signal('FIRST_BED',this.t266Frame);this.t276Clock('FIRST_BED',r.physicalCommit);");
 s=one(s,"this.t266Saw8=true;this.t266AnchorFrames=4;this.t276Clock('SINCE_BIRTH_8S',r.physicalCommit);","this.t266Saw8=true;t284Cold.signal('SINCE_BIRTH_8S',this.t266Frame);this.t276Clock('SINCE_BIRTH_8S',r.physicalCommit);");
 s=one(s,"if(this.t248.firstBirthCommit===r.physicalCommit&&(r.original?.admittedH??0)>0)this.t276Clock('FIRST_BIRTH',r.physicalCommit);","if(this.t248.firstBirthCommit===r.physicalCommit&&(r.original?.admittedH??0)>0){t284Cold.signal('FIRST_BIRTH',this.t266Frame);this.t276Clock('FIRST_BIRTH',r.physicalCommit);}");
 // Do not even stringify a cold object outside selected frame capture.
 s=s.replace(/if\(this.t266Heavy\)this.t276Cold\('T248_(LAND|STEP)',JSON.stringify\((.*?)\)\);/gs,'');
 s=s.replace(/if\(this.t266Heavy\|\|!r.ok\)this.t276Cold\('T266_SOURCE',JSON.stringify\((.*?)\)\);/gs,'');
 s=one(s,"this.t276Cold('T266_FAILURE',JSON.stringify({result:r,boundedTrace:this.t266Drain(true)}));","this.t276Clock('FAILURE',r.physicalCommit);");
 s=one(s,'this.owner.t266Attach(this.t266Enabled?this.t266Trace:null,this.t248);\n        if (this.held)', 'this.owner.t266Attach(this.t266Enabled?this.t266Trace:null,this.t248);t284Cold.bind(this.t248.runId,this.t248.stateEpoch);\n        if (this.held)');
 // An explicitly sampled trace prefix is bounded and marked incomplete; no
 // multi-megabyte trace serialization can silently enter a frame callback.
 s=s.replace('t.rows.slice(0,t.count*32)','t.rows.slice(0,Math.min(t.count,64)*32)').replace('incomplete:t.incomplete','incomplete:t.incomplete||t.count>64,traceRowsOmitted:Math.max(0,t.count-64)');
 return s;
});
edit(page,s=>{
 s=one(s,'private previous=0;','private t284Timer:number=-1;private t284Pixels:number=0;private previous=0;');
 s=one(s,'this.active=true;this.model.setActive(true);',"this.active=true;this.model.setActive(true);this.model.t284Manual(AppStorage.get<number>('t284captureframe')??-1);if(this.t284Timer<0)this.t284Timer=setInterval(()=>{this.model.t284Drain();},50);");
 s=one(s,'this.onPageHide();this.callback.owner=null;','this.onPageHide();if(this.t284Timer>=0){clearInterval(this.t284Timer);this.t284Timer=-1;}this.model.t284Drain();this.callback.owner=null;');
 s=one(s,'const t266Anchor=this.model.t266Heavy||this.model.t266AnchorFrames>0;','const t266Anchor=this.model.t284Choose(this.serial,p.physicsCommitId);');
 s=one(s,'if(this.model.t276BedSeen()&&this.model.t266AnchorFrames===4)native.t276ArmPixels(this.serial);',"if(this.model.t284Reason==='FIRST_BED'){native.t276ArmPixels(this.serial);this.t284Pixels=4;}");
 s=one(s,'if(t266Anchor||this.serial%60===0||!receipt?.currentFrameSucceeded)', 'if(t266Anchor)');
 s=one(s,"this.model.t266LogMs=p013NowMs()-t266LogBegin;}","this.model.t266LogMs=p013NowMs()-t266LogBegin;this.model.t284Finish();}if(this.t284Pixels>0){this.t284Pixels--;const link=JSON.stringify({run:p.runId,epoch:p.stateEpoch,commit:p.physicsCommitId,frame:this.serial,token:receipt?.lastGoodFrameToken??'NOT_CAPTURED',current:receipt?.currentFrameSucceeded??false,pixel:native.t276Pixels()});if(link.length<=1800)console.info('T284_PIXEL_LINK '+link);else console.info('T284_BUDGET '+JSON.stringify({status:'INCOMPLETE',reason:'PIXEL_LINE_BUDGET',frame:this.serial}));}");
 // Native step cost drain remains a cold bounded ring, never a normal payload.
 return s;
});
edit('render/P013SceneController.ets',s=>{
 s=one(s,"import { t276Export } from '../diagnostics/t248/T276Export';","import { t284Cold } from '../diagnostics/t248/T284Cold';");
 s=one(s,"if(this.t266CapturePacked||AppStorage.get<boolean>('t266heavy')===true)","if(this.t266CapturePacked)");
 s=s.replace(/t276Export\('PACK\|'.*?,JSON.stringify\((.*?)\),\(line:string\):void=>\{console.info\(line\);\}\);/gs,"t284Cold.enqueue('PACK',JSON.stringify($1));");return s;
});
edit('entryability/EntryAbility.ets',s=>one(s,"    AppStorage.setOrCreate('t266heavy',", "    AppStorage.setOrCreate('t284captureframe', Number(want.parameters?.t284captureframe??-1));\n    AppStorage.setOrCreate('t266heavy',"));
const pixel=path.join(out,'entry/src/main/cpp/p013renderer/t276_pixel.h');let px=fs.readFileSync(pixel,'utf8');px=one(px,'for(size_t i=0;i<count;i++){const auto& r=rows[i];if(i)o<<', 'for(size_t i=count?count-1:0;i<count;i++){const auto& r=rows[i];if(false)o<<');fs.writeFileSync(pixel,px);
const files=[model,page,'render/P013SceneController.ets','diagnostics/t248/T284Cold.ets'];fs.writeFileSync(path.join(out,'t284-manifest.json'),JSON.stringify({revision:'T284_BOUNDED_COLD_V1',productionTouched:false,device:'NOT_RUN',anchors:{FIRST_BIRTH:1,FIRST_BED:1,SINCE_BIRTH_8S:1,MANUAL:1},budgets:{anchorRawBytes:262144,anchorParts:512,runRawBytes:1048576,runParts:2048,queueRawBytes:524288,queueItems:16,captureMs:4,batchParts:4,batchMs:2,exportMs:200},files:files.map(file=>({file,sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(ets,file))).digest('hex')}))},null,2));
};
