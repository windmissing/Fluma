const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t392/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets');
function edit(name,before,after){
 const file=path.join(ets,name),s=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 if(s.split(before).length!==2)throw Error('T400_GENERATOR_ANCHOR '+name+' '+before.slice(0,100));
 fs.writeFileSync(file,s.replace(before,after));
}
function span(name,from,to,fragment){
 const s=fs.readFileSync(path.join(ets,name),'utf8').replaceAll('\r\n','\n');
 const start=s.indexOf(from),end=s.indexOf(to,start+from.length);
 if(start<0||end<0)throw Error('T400_GENERATOR_SPAN '+name+' '+from);
 edit(name,s.slice(start,end),fragment);
}
const local=name=>fs.readFileSync(path.join(__dirname,name),'utf8').trimEnd()+'\n';
const archive='physics/derived/P011Z08Archive.ets';
fs.copyFileSync(path.join(__dirname,'T400Arm.ets'),path.join(ets,'physics/derived/T400Arm.ets'));
edit(archive,'import { P013SceneFrame', "import { T400Arm } from './T400Arm';\nimport { P013SceneFrame");
edit(archive,'  private static readonly MAX_FRAMES=2400;\n','');
edit(archive,'  private frames=0;\n','');
edit(archive,'  readonly waitForLand: boolean;\n  constructor(ids: number[], waitForLand: boolean) {',`  readonly arm:T400Arm;
  private firstT3:Z08Candidate|null=null;
  private firstPositive:Z08Candidate|null=null;
  private frameNs=0;
  private pageFrame=0;
  private nativeFrame=0;
  private selectedPublication:Z08PublicationIdentity|null=null;
  private renderReceipt:P013SceneReceipt|null=null;
  private failure:Z08FailureReceipt|null=null;
  private presentation='PUBLICATION_NOT_FROM_CALLBACK';
  private landSwap=false;
  constructor(ids:number[],arm:T400Arm) {`);
edit(archive,'this.ids=new Uint32Array(ids);this.waitForLand=waitForLand;',
 'this.ids=new Uint32Array(ids);this.arm=arm;');
edit(archive,'this.frames++;this.frameOpen=true;', 'this.arm.callbacks++;this.frameOpen=true;');
edit(archive,'    if (pub===null || this.candidates.length===2)',`    this.arm.steps++;
    if(pub!==null&&(pub.run!==this.arm.run||pub.epoch!==this.arm.epoch)){
      this.cancel('ARM_IDENTITY_MISMATCH',this.arm.startedMs+this.arm.wallMs);return;
    }
    const pool=owner.canonical.pool;
    for(let i=0;!this.arm.seenId&&i<pool.nmax;i++){
      if(pool.pState[i]!==0&&this.ids.indexOf(pool.pId[i])>=0)this.arm.seenId=true;
    }
    if(this.count>0)this.arm.seenId=true;
    if (pub===null || this.candidates.length===2)`);
span(archive,'  finishFrame(pub:','  exportCold():',local('archive.inc'));

const model='physics/derived/P051DerivedDemoV2.ets';
edit(model,'import { P011Z08Archive }', "import { T400Arm, T400Birth } from './T400Arm';\nimport { P011Z08Archive }");
span(model,'    private z08:P011Z08Archive','    private held = false;',local('model.inc'));
edit(model,'if (!input.valid || !this.active)','if (!input.valid || !this.active || this.z08Fixed())');
edit(model,'setTilt(degrees: number): void { this.owner.setTilt(degrees); }',
 'setTilt(degrees: number): void { if(!this.z08Fixed())this.owner.setTilt(degrees); }');
edit(model,'setShakeInput(value: number): void { this.owner.setShakeInput(value); }',
 'setShakeInput(value: number): void { if(!this.z08Fixed())this.owner.setShakeInput(value); }');
edit(model,'reset(): void { this.stop(); this.z08=null; this.z08Attached=false; this.z08Fixture=false;',
 "reset(now:number=0): void { this.stop(); this.cancelZ08('RESET',now);");
span(model,'        if(this.z08Fixture){','        if (this.held) {',
 '        if(!this.admitZ08Fixture(now))return false;\n');
edit(model,'tick(ns: number, dt: number): boolean {','tick(ns: number, dt: number, now:number=ns/1e6): boolean {');
edit(model,'const r = this.owner.step(dt);','const r = this.owner.step(this.z08Dt(dt));');
edit(model,'if (!this.active || !Number.isFinite(x)', 'if (!this.active || this.z08Fixed() || !Number.isFinite(x)');

const page='pages/P051DerivedV2Demo.ets';
edit(page," @State private z08Status:string='';",` @State private z08Status:string='';
 private z08Timer:number=-1;
 private z08Exporting=false;
 private updateZ08Timer():void{
  if(this.z08Timer>=0)clearTimeout(this.z08Timer);
  const arm=this.model.z08Id();
  this.z08Timer=setTimeout(()=>{
   this.model.expireZ08(arm,p013NowMs());this.z08Status=this.model.z08Status();this.z08Timer=-1;
  },15000);
 }
 private armZ08(mode:string):void{
  const before=this.model.z08Id();
  this.z08Status=this.model.armZ08(this.z08Ids,mode,p013NowMs());
  if(this.model.z08Id()!==before)this.updateZ08Timer();
 }
 private fixtureZ08(restricted:boolean):void{
  const before=this.model.z08Id();
  this.z08Status=this.model.startZ08NearBed(restricted,p013NowMs());
  if(this.model.z08Id()!==before){this.publication=this.model.createPublicationCold();this.updateZ08Timer();}
 }`);
edit(page,'onPageHide():void{this.active=false;',
 "onPageHide():void{this.model.cancelZ08('PAGE_HIDDEN',p013NowMs());this.z08Status=this.model.z08Status();this.active=false;");
edit(page,'const dt=this.previous===0?0:Math.min(1/30,(ns-this.previous)/1e9);',
 'const dt=this.model.z08Dt(this.previous===0?0:Math.min(1/30,(ns-this.previous)/1e9));');
edit(page,'if(input!==null)this.model.applyDefinedInput(input);',
 'if(input!==null&&!this.model.z08Fixed())this.model.applyDefinedInput(input);');
edit(page,'this.model.beginZ08Frame();','this.model.beginZ08Frame(p013NowMs());');
edit(page,'this.model.tick(ns,dt)','this.model.tick(ns,dt,p013NowMs())');
edit(page,'this.model.finishZ08Frame(null,null,null,ns,this.serial,true);',
 'this.model.finishZ08Frame(null,null,null,ns,this.serial,true,p013NowMs());');
edit(page,'this.model.finishZ08Frame(p,receipt===null?null:this.renderer.frame,receipt,ns,this.serial,false);',
 'this.model.finishZ08Frame(p,receipt===null?null:this.renderer.frame,receipt,ns,this.serial,false,p013NowMs());');
edit(page,'this.model.finishZ08Frame(null,null,null,ns,this.serial,false);this.model.budget.end();',
 'this.model.finishZ08Frame(null,null,null,ns,this.serial,false,p013NowMs());this.z08Status=this.model.z08Status();this.model.budget.end();');
edit(page,"this.model.reset();this.publication", "this.model.reset(p013NowMs());this.z08Status=this.model.z08Status();this.publication");
edit(page,"this.model.armZ08(this.z08Ids,false);","this.armZ08('NEXT');");
edit(page,"this.z08Status=this.armZ08('NEXT');","this.armZ08('NEXT');");
edit(page,"Button('预约 LAND').onClick(()=>{this.z08Status=this.model.armZ08(this.z08Ids,true);})",
 "Button('预约首个 T3').onClick(()=>this.armZ08('FIRST_T3'))");
edit(page,"Button('近床夹具').onClick(()=>{this.z08Status=this.model.startZ08NearBed();this.publication=this.model.createPublicationCold();})",
 "Button('近床正控 0°').onClick(()=>this.fixtureZ08(false))\n      Button('受限对照 25°').onClick(()=>this.fixtureZ08(true))");
edit(page,"this.z08Status='Z08 尚未完成';","this.z08Status=this.model.z08Status();");
edit(page,"'t392-z08-'+Date.now()","'t400-z08-'+Date.now()");
edit(page,"      Button('导出 Z08').onClick(async()=>{",
 "      Button('导出 Z08').onClick(async()=>{\n        if(this.z08Exporting)return;\n        const arm=this.model.z08Id();");
edit(page,"if(snapshot.length===0){this.z08Status=this.model.z08Status();return;}\n        try{",
 "if(snapshot.length===0){this.z08Status=this.model.z08Status();return;}\n        this.z08Exporting=true;\n        try{");
edit(page,"console.info('WS_EVENT t392_z08 '+JSON.stringify(file));",
 "if(this.model.z08Id()===arm)this.model.z08SaveResult('');console.info('WS_EVENT t400_z08 '+JSON.stringify(file));");
edit(page,"}catch(error){this.z08Status='Z08_EXPORT_FAILED';}",
 "}catch(error){if(this.model.z08Id()===arm){this.model.z08SaveResult((error as Error).message);this.z08Status=this.model.z08Status();}}\n        finally{this.z08Exporting=false;}");
edit(page,"Text(this.z08Status).fontSize(10)","Text(this.z08Status).fontSize(10).maxLines(1).textOverflow({overflow:TextOverflow.Ellipsis})");
