const fs=require('node:fs'), path=require('node:path'), cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t381/generate.cjs'),out],{stdio:'inherit'});
function edit(relative, transform){
 const file=path.join(out,'entry/src/main/ets',relative);
 fs.writeFileSync(file,transform(fs.readFileSync(file,'utf8')));
}
function replace(source, before, after){
 if(source.split(before).length!==2)throw Error('T392_GENERATOR_ANCHOR: '+before);
 return source.replace(before,after);
}
edit('physics/derived/P051DerivedDemoV2.ets',source=>{
 let s="import { P011Z08Archive } from './P011Z08Archive';\n"+
  "import { P013SceneFrame, P013SceneReceipt } from '../../render/P013SceneTypes';\n"+source;
 s=replace(s,"    private held = false;",`    private z08:P011Z08Archive|null=null;
    private z08Attached=false;
    private z08Fixture=false;
    armZ08(ids:string,waitForLand:boolean):string {
        if(this.z08!==null&&this.z08.exportCold().length===0)return 'Z08_ALREADY_ARMED';
        const selected:number[]=[];
        for(const item of ids.split(','))selected.push(Number(item.trim()));
        try { this.z08=new P011Z08Archive(selected,waitForLand);return 'Z08_ARMED'; }
        catch(error) { return 'Z08_INVALID_IDS'; }
    }
    startZ08NearBed():string {
        this.reset();
        const armed=this.armZ08('1,2,3',true);
        this.z08Fixture=armed==='Z08_ARMED';
        return armed;
    }
    beginZ08Frame():void {
        const archive=this.z08;
        if(archive===null||archive.exportCold().length!==0)return;
        archive.beginFrame();
        this.owner.bindZ08ArchiveCold(archive);
        this.z08Attached=true;
    }
    finishZ08Frame(slot:DerivedPublishedSlotV2|null,frame:P013SceneFrame|null,receipt:P013SceneReceipt|null,
        ns:number,pageFrame:number,stepFailed:boolean):void {
        const archive=this.z08;
        if(archive===null||!this.z08Attached)return;
        this.z08Attached=false;
        this.owner.bindZ08ArchiveCold(null);
        archive.finishFrame(slot?.readZ08Identity()??null,frame,receipt,ns,pageFrame,
            stepFailed?this.owner.exportCrossVerticalFailureCold():'');
    }
    exportZ08Cold():string {return this.z08?.exportCold()??'';}
    private held = false;`);
 s=replace(s,'this.stop(); this.owner.reset(); this.seq = 0;', 'this.stop(); this.z08=null; this.z08Attached=false; this.z08Fixture=false; this.owner.reset(); this.seq = 0;');
 s=replace(s,'        if (this.held) {',`        if(this.z08Fixture){
            this.z08Fixture=false;
            const r=this.owner.inject({revision:P051_DRY_REVISION,sourceId:51,sequence:++this.seq,inputTimeMs:ns/1e6,
                x:63.99999,y:0.004,seed:51,requestedH:0.3,requestedL:0});
            if(!r.ok){this.status='Z08_FIXTURE_ADMISSION_FAILED';return false;}
        }
        if (this.held) {`);
 return s;
});
edit('pages/P051DerivedV2Demo.ets',source=>{
 let s=replace(source,"private readonly useSingleSand:boolean=AppStorage.get<boolean>('singleSandDemo')===true;",'private readonly useSingleSand:boolean=false;');
 s=replace(s," @State private status:string='等待新管线';",` @State private status:string='等待新管线';
 @State private z08Ids:string='1,2,3';
 @State private z08Status:string='';`);
 s=replace(s,"  if(!this.model.tick(ns,dt)){","  this.model.beginZ08Frame();\n  if(!this.model.tick(ns,dt)){this.model.finishZ08Frame(null,null,null,ns,this.serial,true);");
 s=replace(s,"const receipt=p.drawNewPipeline(this.renderer,c,ns,this.getUIContext().vp2px(1));",
  "const receipt=p.drawNewPipeline(this.renderer,c,ns,this.getUIContext().vp2px(1));\n"+
  "  this.model.finishZ08Frame(p,receipt===null?null:this.renderer.frame,receipt,ns,this.serial,false);");
 s=replace(s,"finally{this.model.budget.end();}","finally{this.model.finishZ08Frame(null,null,null,ns,this.serial,false);this.model.budget.end();}");
 s=replace(s,"    Button('导出跨列收据')",`    Row({space:4}){
      TextInput({text:this.z08Ids,placeholder:'粒子 ID（最多 8 个）'}).onChange((value:string)=>{this.z08Ids=value;}).layoutWeight(1)
      Button('预约下一帧').onClick(()=>{this.z08Status=this.model.armZ08(this.z08Ids,false);})
      Button('预约 LAND').onClick(()=>{this.z08Status=this.model.armZ08(this.z08Ids,true);})
    }
    Row({space:4}){
      Button('近床夹具').onClick(()=>{this.z08Status=this.model.startZ08NearBed();this.publication=this.model.createPublicationCold();})
      Button('导出 Z08').onClick(async()=>{
        const snapshot=this.model.exportZ08Cold();
        if(snapshot.length===0){this.z08Status='Z08 尚未完成';return;}
        try{
          const context=getContext(this) as common.UIAbilityContext;
          const file=await new DiagnosticColdFiles(context.filesDir).save('t392-z08-'+Date.now()+'.json',snapshot);
          this.z08Status=file.name+' '+file.sha256;
          console.info('WS_EVENT t392_z08 '+JSON.stringify(file));
        }catch(error){this.z08Status='Z08_EXPORT_FAILED';}
      })
    }
    Text(this.z08Status).fontSize(10)
    Button('导出跨列收据')`);
 return s;
});
