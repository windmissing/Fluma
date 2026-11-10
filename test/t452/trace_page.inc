 private async exportM2Trace():Promise<void>{
  if(this.exportingTrace)return;
  this.exportingTrace=true;
  try{
   const raw:string=m1.readM2Trace('0',0);
   const meta=JSON.parse(raw) as M2TraceMeta;
   if(meta.state!=='TRACE_READY'){this.operation=meta.state;return;}
   const context=getContext(this) as common.UIAbilityContext;
   const files=new DiagnosticColdFiles(context.filesDir);
   const prefix='m2-v3-'+Date.now()+'-'+meta.traceId;
   const receipts:ColdFile[]=[];
   for(let i=1;i<=meta.pages;i++){
    const body:string=m1.readM2Trace(meta.traceId,i);
    const reply=JSON.parse(body) as M2TraceMeta;
    if(reply.state!=='TRACE_READY'||reply.traceId!==meta.traceId||reply.page!==i){this.operation='TRACE_INCOMPLETE';return;}
    receipts.push(await files.save(prefix+'-'+i+'.json',body));
   }
   const summary=await files.save(prefix+'-manifest.json',JSON.stringify({header:meta,files:receipts}));
   this.operation='TRACE_EXPORTED';console.info('WS_EVENT t452_trace '+JSON.stringify(summary));
  }catch(e){this.operation='TRACE_INCOMPLETE '+String(e);}
  finally{this.exportingTrace=false;}
 }
