const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t400/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets');
function edit(name,before,after){
 const file=path.join(ets,name),s=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 if(s.split(before).length!==2)throw Error('T423_ANCHOR '+name+' '+before);
 fs.writeFileSync(file,s.replace(before,after));
}
const model='physics/derived/P051DerivedDemoV2.ets';
const text=fs.readFileSync(path.join(ets,model),'utf8');
const constructor=text.match(/constructor\(nmax: number = 512\) \{ this\.owner = new P051DerivedFacadeV2\(\{[^\n]+\}\); \}/)?.[0];
if(!constructor)throw Error('T423_MODEL_CONSTRUCTOR');
let replacement=constructor.replace('nmax: number = 512','nmax: number = 512, cpuTxRoute:string=\'LEGACY_FULL\'');
if(!replacement.includes('motionRevision:'))replacement=replacement.replace("writerRegistry: 'T216_DRY_WRITERS_V2'","writerRegistry: 'T216_DRY_WRITERS_V2', motionRevision:'P011_CPU_DRY_CROSS_VERTICAL_V1'");
replacement=replacement.replace(' }); }',', cpuTxRoute:cpuTxRoute }); }');
edit(model,constructor,replacement+"\n    exportCpuTxUndoCold():string{return this.owner.exportCpuTxUndoCold();}");
const page='pages/P051DerivedV2Demo.ets';
const pg=fs.readFileSync(path.join(ets,page),'utf8');
const modelDeclaration=pg.match(/private readonly model=new P051DerivedDemoV2\(\);/)?.[0];
if(!modelDeclaration)throw Error('T423_PAGE_MODEL');
edit(page,modelDeclaration,`private model=new P051DerivedDemoV2();
 @State private t423Route:string='LEGACY_FULL';
 private t423Switch():void {
  this.model.stop();this.t423Route=this.t423Route==='LEGACY_FULL'?'LOCAL_UNDO_V1':'LEGACY_FULL';
  this.model=new P051DerivedDemoV2(512,this.t423Route);this.model.setActive(this.active);this.model.budget.enabled=true;
  this.renderer.budget=this.model.budget;this.publication=this.model.createPublicationCold();
  this.previous=0;this.schedule();
 }
 private async t423Export():Promise<void>{
  try{const context=getContext(this) as common.UIAbilityContext;
   const file=await new DiagnosticColdFiles(context.filesDir).save('t423-copy-'+Date.now()+'.json',this.model.exportCpuTxUndoCold());
   this.status=file.name;
  }catch(e){this.status='T423_EXPORT_FAILED';}
 }`);
edit(page,"    Button('导出帧预算')",`    Row(){Button(this.t423Route+' · 新run').onClick(()=>this.t423Switch());Button('导出事务复制账').onClick(()=>this.t423Export());}
    Button('导出帧预算')`);
