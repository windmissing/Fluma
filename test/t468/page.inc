 private createExportBusy:boolean=false;
 private async exportCreateDiagnostic(previous:boolean=false):Promise<void>{
  if(this.createExportBusy)return;this.createExportBusy=true;
  try{
   let raw:string=m1.m2CreateDiagnostics();
   if(previous){const latest=JSON.parse(raw) as M2CreateSnapshot;if(latest.previousSerial==='0'){this.operation='NO_PREVIOUS_CREATE';return;}raw=m1.m2CreateDiagnostics(latest.previousSerial);}
   const snapshot=JSON.parse(raw) as M2CreateSnapshot;
   if(snapshot.revision!=='T462_M2_CREATE_DIAGNOSTIC_V1'){this.operation='NO_CREATE_ATTEMPT';return;}
   const context=getContext(this) as common.UIAbilityContext;
   const files=new DiagnosticColdFiles(context.filesDir),binary=new M2CreateColdFiles(context.filesDir);
   const prefix='t468-create-'+snapshot.bootSerial+'-'+Date.now();
   const receipts:ColdFile[]=[];
   for(let i=0;i<3;i++){
    const result=JSON.parse(m1.m2CreateLog(snapshot.bootSerial,i)) as M2CreateLog;
    if(result.state!=='CREATE_LOG'||result.bootSerial!==snapshot.bootSerial||result.object!==i){this.operation='CREATE_EXPORT_INCOMPLETE';return;}
    receipts.push(await binary.saveRaw(prefix+'-'+i+'.log',result.hex));
   }
   const native=await files.save(prefix+'-native.json',raw);
   const final=await files.save(prefix+'-manifest.json',JSON.stringify({revision:snapshot.revision,declaration:m2PackageDeclaration(),native:native,rawLogs:receipts}));
   this.operation='CREATE_DIAGNOSTIC_EXPORTED';console.info('WS_EVENT t468_create '+JSON.stringify(final));
  }catch(e){this.operation='CREATE_EXPORT_FAILED '+String(e);}
  finally{this.createExportBusy=false;}
 }
