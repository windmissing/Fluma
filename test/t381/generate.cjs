const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t351/generate.cjs'),out],{stdio:'inherit'});
const file=path.join(out,'entry/src/main/ets/physics/derived/P051DerivedDemoV2.ets');
const source=fs.readFileSync(file,'utf8');
const anchor="writerRegistry: 'T216_DRY_WRITERS_V2'";
if(source.split(anchor).length!==2)throw Error('T381_MOTION_ROUTE_ANCHOR');
fs.writeFileSync(file,source.replace(anchor,anchor+", motionRevision: 'P011_CPU_DRY_CROSS_VERTICAL_V1'"));

const enabled=fs.readFileSync(file,'utf8');
fs.writeFileSync(file,enabled.replace('    createPublicationCold():','    exportCrossVerticalCold():string{return this.owner.exportCrossVerticalCold();}\n    exportCrossVerticalFailureCold():string{return this.owner.exportCrossVerticalFailureCold();}\n    createPublicationCold():'));
const page=path.join(out,'entry/src/main/ets/pages/P051DerivedV2Demo.ets');
const body=fs.readFileSync(page,'utf8');
const button="    Button('导出帧预算')";
if(body.split(button).length!==2)throw Error('T381_EXPORT_ANCHOR');
fs.writeFileSync(page,body.replace(button,`    Button('导出跨列收据').onClick(async()=>{
      try{
        const context=getContext(this) as common.UIAbilityContext;
        const file=await new DiagnosticColdFiles(context.filesDir).save('t381-cross-'+Date.now()+'.json',this.model.exportCrossVerticalCold());
        await new DiagnosticColdFiles(context.filesDir).save(file.name.replace('.json','-failure.json'),this.model.exportCrossVerticalFailureCold());
        this.status=file.name;
      }catch(e){this.status='T381_EXPORT_FAILED';}
    })
`+button));
