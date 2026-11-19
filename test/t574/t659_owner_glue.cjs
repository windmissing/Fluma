// T659: patch the real T540/T598 product page, with exact anchors (no output-only fix).
const fs=require('node:fs'),path=require('node:path');
module.exports=function(out){
const file=path.join(out,'entry/src/main/ets/pages/P051DerivedV2Demo.ets');let s=fs.readFileSync(file,'utf8').replace(/\r\n/g,'\n');
function one(a,b){if(s.split(a).length!==2)throw Error('T659_ANCHOR '+a.slice(0,100));s=s.replace(a,b);}
one('PrepareReadinessV4, PrepareAttemptOwner','PrepareReadinessV4, PrepareAttemptOwner, prepareUtf8Bytes');
// The old injected bind is replaced with a guarded fresh builder; no method on JSON owners.
const old='  const owner=new PrepareAttemptOwner();owner.prepareId=this.prepareGate.prepareId;owner.pageGeneration=this.prepareGate.pageGeneration;owner.branch=this.prepareBranch;owner.windowId=s.windowId;owner.windowGeneration=s.windowGeneration;owner.controller=s.controller;owner.session=s.session;owner.surface=s.surface;owner.context=s.context;owner.run=s.run;if(this.prepareObservation!==null&&this.prepareObservation.prepareId===this.prepareGate.prepareId)this.prepareGate.sampleFrame(owner);';
one(old,`  if(!this.prepareScope659(readId,readGeneration,readBranch,w,readWindowGeneration)){this.prepareGate.discardOwnerCallback();return new PrepareSnapshot();}
  if(this.prepareObservation!==null&&this.prepareObservation.prepareId===readId&&(this.prepareGate.state==='PREPARING'||this.prepareGate.state==='READY'))this.prepareGate.sampleFrame(this.prepareOwnerValue659(s));`);
one(' private readPrepareSnapshot():PrepareSnapshot {',` private readPrepareSnapshot():PrepareSnapshot {
  const readId=this.prepareGate.prepareId,readGeneration=this.prepareGate.pageGeneration,readBranch=this.prepareBranch,readWindowGeneration=PrepareMainWindow.generation;
`);
one('  if(w===null||w!==PrepareMainWindow.value)return s;',"  if(w===null||w!==PrepareMainWindow.value){this.prepareGate.invalidate('OWNER_CHANGED');return s;}");
one('  try{this.prepareGate.observe(this.readPrepareSnapshot());}',`  const id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration,branch=this.prepareBranch,w=this.prepareWindow,wg=PrepareMainWindow.generation;
  try{const snapshot=this.readPrepareSnapshot();if(!this.prepareScope659(id,generation,branch,w,wg)){this.prepareGate.discardOwnerCallback();return;}this.prepareGate.observe(snapshot);}`);
one('    const snapshot=this.readPrepareSnapshot();\n    const reason=',`    const readId=this.prepareGate.prepareId,readGeneration=this.prepareGate.pageGeneration,readBranch=this.prepareBranch,readWindow=this.prepareWindow,readWg=PrepareMainWindow.generation;
    const snapshot=this.readPrepareSnapshot();
    if(!this.prepareScope659(readId,readGeneration,readBranch,readWindow,readWg)){this.prepareGate.discardOwnerCallback();return 'PREPARE_STALE';}
    const reason=`);
one('const ticket=this.prepareGate.checkForReserve(this.readPrepareSnapshot(),p013NowMs());',`const reserveId=this.prepareGate.prepareId,reserveGeneration=this.prepareGate.pageGeneration,reserveBranch=this.prepareBranch,reserveWindow=this.prepareWindow,reserveWg=PrepareMainWindow.generation;
    const fresh=this.readPrepareSnapshot();if(!this.prepareScope659(reserveId,reserveGeneration,reserveBranch,reserveWindow,reserveWg)){this.prepareGate.discardOwnerCallback();return;}
    const ticket=this.prepareGate.checkForReserve(fresh,p013NowMs());`);
// All window subscriptions capture the actual attempt and are rearmed at new branch.
let a=s.indexOf('   this.avoidListener=(event:'),b=s.indexOf('this.windowSubscribed=true;',a);if(a<0||b<0)throw Error('T659_WINDOW_BLOCK');
s=s.slice(0,a)+'   this.subscribePrepareWindow659();'+s.slice(b+'this.windowSubscribed=true;'.length);
one('  this.prepareBranch=branch;\n  return true;', "  this.prepareBranch=branch;\n  try{this.subscribePrepareWindow659();}catch(error){this.prepareRawFailure659('window.rebind',error,id,generation,branch);this.stopPrepareObservation();return false;}\n  return true;");
// Frame observation additionally owns the exact window reference, not just its numeric id.
one(' private prepareContext:UIContext|null=null;', ' private prepareObservedWindow659:window.Window|null=null;\n private prepareContext:UIContext|null=null;');
one('  this.prepareObservation=o;','  this.prepareObservation=o;this.prepareObservedWindow659=w;');
one('   this.prepareWindow!==null&&this.prepareWindow===PrepareMainWindow.value&&', '   this.prepareWindow!==null&&this.prepareWindow===this.prepareObservedWindow659&&this.prepareWindow===PrepareMainWindow.value&&');
for(const [callback,stage] of [['prepareLayoutListener','frame.layout'],['prepareDrawListener','frame.draw']]){const start=s.indexOf('   this.'+callback+'=():void=>{'),end=s.indexOf('\n   };',start);if(start<0||end<0)throw Error('T659_FRAME_GUARD '+callback);const body=s.slice(start,end);s=s.slice(0,start)+body.replace('=():void=>{','=():void=>{this.prepareCallback659(\''+stage+'\',o,():void=>{')+'});'+s.slice(end);}
// Former stale-frame path could invalidate a new attempt. Old callbacks only discard.
one("if(!this.prepareOwner(o)){o.drop(p013NowMs(),ns,'STALE_FRAME_OWNER');this.prepareFrameDeadline(o,p013NowMs());return;}", "if(!this.prepareOwner(o)){o.drop(p013NowMs(),ns,'STALE_FRAME_OWNER');this.prepareGate.discardOwnerCallback();return;}");
a=s.indexOf('  if(!this.prepareOwner(o)){',s.indexOf(' private prepareFrameDeadline('));b=s.indexOf('  if(now-this.prepareGate.startedMonoMs()',a);if(a<0||b<0)throw Error('T659_DEADLINE_BLOCK');s=s.slice(0,a)+"  if(!this.prepareOwner(o)){o.drop(now,0,'STALE_DEADLINE_OWNER');this.prepareGate.discardOwnerCallback();return false;}\n"+s.slice(b);
// Revalidate after synchronous native/window reads, before observing or advancing READY.
one('     if(snapshot.windowId!==o.windowId||snapshot.windowGeneration!==o.windowGeneration){',"     if(!this.prepareOwner(o)){o.drop(now,ns,'STALE_DRAW_READ');this.prepareGate.discardOwnerCallback();return;}\n     if(snapshot.windowId!==o.windowId||snapshot.windowGeneration!==o.windowGeneration){");
one("    }catch(e){this.prepareGate.fail('PREPARE_OBSERVATION_FAILED');}","    }catch(e){this.prepareRawFailure659('frame.snapshot',e,o.prepareId,o.pageGeneration,o.branch);}");
one("  catch(e){this.prepareGate.fail('PREPARE_POST_FAILED');this.prepareUpdate();}","  catch(e){this.prepareRawFailure659('frame.post',e,o.prepareId,o.pageGeneration,o.branch);this.prepareUpdate();}");
one("   }catch(error){this.prepareGate.fail('PREPARE_OBSERVATION_FAILED');this.prepareUpdate();return 'PREPARE_OBSERVATION_FAILED';}","   }catch(error){this.prepareRawFailure659('reserve.fresh',error,this.prepareGate.prepareId,this.prepareGate.pageGeneration,this.prepareBranch);this.prepareUpdate();return 'PREPARE_OBSERVATION_FAILED';}");
one("  catch(error){this.prepareGate.fail('PREPARE_OBSERVATION_FAILED');this.prepareApiFailure('window.snapshot',error,this.prepareGate.prepareId,p013NowMs(),this.prepareGate.pageGeneration);}","  catch(error){this.prepareRawFailure659('window.snapshot',error,this.prepareGate.prepareId,this.prepareGate.pageGeneration,this.prepareBranch);}");
one("this.prepareRawFailure659('window.snapshot',error,this.prepareGate.prepareId,this.prepareGate.pageGeneration,this.prepareBranch);","this.prepareRawFailure659('window.snapshot',error,id,generation,branch);");
one('this.prepareGate.show();this.configuring=true;', 'this.prepareGate.show();this.prepareLastOwner659=null;this.configuring=true;');
one('  const id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration;\n  if(this.prepareTimer', '  this.prepareLastOwner659=null;\n  const id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration;\n  if(this.prepareTimer');
one('  this.singleSand.bindPrepareGateCold(():string=>{',`  this.singleSand.bindPrepareGateCold(():string=>{
   const coldId=this.prepareGate.prepareId,coldGeneration=this.prepareGate.pageGeneration,coldBranch=this.prepareBranch;`);
one("this.prepareRawFailure659('reserve.fresh',error,this.prepareGate.prepareId,this.prepareGate.pageGeneration,this.prepareBranch);","this.prepareRawFailure659('reserve.fresh',error,coldId,coldGeneration,coldBranch);");
one("const before=this.prepareGate.state;let failure='';","const before=this.prepareGate.state,attemptPrepareId=this.prepareGate.prepareId,attemptPageGeneration=this.prepareGate.pageGeneration,attemptBranch=this.prepareBranch;let failure='';");
one('core[1]=this.prepareGate.prepareId;core[2]=this.prepareGate.pageGeneration;', 'core[1]=attemptPrepareId;core[2]=attemptPageGeneration;core[15]=this.prepareGate.currentId(attemptPrepareId,attemptPageGeneration)?0:1;');
// All actual API errors retain first/last bounded raw data before current-attempt failure.
one('  const raw=error as PrepareCallError,record=new PrepareApiRecord();','  this.prepareRawFailure659(api,error,id,generation,this.prepareBranch);\n  const raw=error as PrepareCallError,record=new PrepareApiRecord();');
// A fresh sample and ticket, reserve and consume remain synchronous, before any await.
one("if(!this.prepareGate.consumeTicket(ticket)){this.singleSand.capture.invalidate('PREPARE_TICKET_MISMATCH');this.probeStatus='PREPARE_TICKET_MISMATCH';}","if(!this.prepareGate.consumeTicket(ticket)){this.singleSand.capture.invalidate('PREPARE_TICKET_MISMATCH');this.probeStatus='PREPARE_TICKET_MISMATCH';this.prepareUpdate();return;}");
one("catch(error){failure=String(error);if(this.reserveIo598===1)this.reserveIo598=3;}","catch(error){failure=String(error);this.prepareRawFailure659('reserve.attempt',error,this.prepareGate.prepareId,this.prepareGate.pageGeneration,this.prepareBranch);if(this.reserveIo598===1)this.reserveIo598=3;}");
one("this.prepareRawFailure659('reserve.attempt',error,this.prepareGate.prepareId,this.prepareGate.pageGeneration,this.prepareBranch);","this.prepareRawFailure659('reserve.attempt',error,attemptPrepareId,attemptPageGeneration,attemptBranch);");
// Bounded cold writing: total <=16KiB, catches/finally do not grant readiness.
a=s.indexOf("  const text='{",s.indexOf(' private async prepareCold()'));b=s.indexOf('  try{',a);if(a<0||b<0)throw Error('T659_COLD_BLOCK');s=s.slice(0,a)+`  const readiness=this.prepareGate.report(),full='{"revision":"T659_PREPARE_OWNER_VALUE_V1","observation":'+JSON.stringify(this.prepareObservation)+',"readiness":'+readiness+'}';
  const text=prepareUtf8Bytes(full)<=16384?full:'{"revision":"T659_PREPARE_OWNER_VALUE_V1","observationTruncated":true,"readiness":'+readiness+'}',id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration;
`+s.slice(b);
// Catch export only affects its own attempt; preserve the original exception.
one("}catch(error){if(this.prepareGate.currentId(id,generation)){this.prepareGate.fail('PREPARE_EXPORT_FAILED');this.prepareText=this.prepareGate.state+' '+this.prepareGate.reason;}}", "}catch(error){if(this.prepareGate.currentId(id,generation)){this.prepareRawFailure659('prepare.cold.write',error,id,generation,this.prepareBranch);this.prepareText=this.prepareGate.state+' '+this.prepareGate.reason;}}finally{this.prepareColdPending659=false;}");
one(' private async prepareCold():Promise<void> {',' private prepareColdPending659:boolean=false;\n private async prepareCold():Promise<void> {\n  this.prepareColdPending659=true;');
const methods=`
 private prepareCallback659(stage:string,o:PrepareFrameObservation,action:()=>void):void {if(!this.prepareOwner(o)){o.drop(p013NowMs(),0,'STALE_CALLBACK_OWNER');this.prepareGate.discardOwnerCallback();return;}try{action();}catch(error){if(this.prepareObservation===o&&this.prepareScope659(o.prepareId,o.pageGeneration,o.branch,this.prepareObservedWindow659,o.windowGeneration))this.prepareRawFailure659(stage,error,o.prepareId,o.pageGeneration,o.branch);else this.prepareGate.discardOwnerCallback();}this.prepareUpdate();}
 private prepareOwnerValue659(s:PrepareSnapshot):PrepareAttemptOwner {
  const o=new PrepareAttemptOwner();o.prepareId=this.prepareGate.prepareId;o.pageGeneration=this.prepareGate.pageGeneration;o.branch=this.prepareBranch;o.windowId=s.windowId;o.windowGeneration=s.windowGeneration;o.controller=s.controller;o.session=s.session;o.surface=s.surface;o.context=s.context;o.run=s.run;return o;
 }
 private prepareScope659(id:number,generation:number,branch:number,w:window.Window|null,windowGeneration:number):boolean {
  return this.active&&this.prepareGate.active&&this.prepareGate.prepareId===id&&this.prepareGate.pageGeneration===generation&&this.prepareBranch===branch&&w!==null&&this.prepareWindow===w&&PrepareMainWindow.value===w&&PrepareMainWindow.generation===windowGeneration;
 }
 private prepareRawFailure659(stage:string,error:Object,id:number,generation:number,branch:number):void {
  if(error===null||error===undefined){this.prepareGate.ownerFailure(stage,-1,'',String(error),this.prepareGate.ownerSnapshot(),this.prepareLastOwner659,id,generation,branch);return;}
  const raw=error as PrepareCallError,before=this.prepareGate.ownerSnapshot();
  this.prepareGate.ownerFailure(stage,typeof raw.code==='number'?raw.code:-1,typeof raw.name==='string'?raw.name:'',typeof raw.message==='string'?raw.message:String(error),before,this.prepareLastOwner659,id,generation,branch);
 }
 private prepareLastOwner659:PrepareAttemptOwner|null=null;
 private subscribePrepareWindow659():void {
  const w=this.prepareWindow;if(w===null)return;
  if(this.avoidListener!==null)w.off('avoidAreaChange',this.avoidListener);
  if(this.windowListener!==null)w.off('windowEvent',this.windowListener);
  if(this.heightListener!==null)w.off('keyboardHeightChange',this.heightListener);
  if(this.modeListener!==null)w.off('windowStatusChange',this.modeListener);
  this.windowSubscribed=false;
  const id=this.prepareGate.prepareId,generation=this.prepareGate.pageGeneration,branch=this.prepareBranch,wg=PrepareMainWindow.generation;
  const current=():boolean=>this.prepareScope659(id,generation,branch,w,wg);
  const run=(stage:string,action:()=>void):void=>{if(!current()){this.prepareGate.discardOwnerCallback();return;}try{action();}catch(error){if(current())this.prepareRawFailure659(stage,error,id,generation,branch);else this.prepareGate.discardOwnerCallback();}this.prepareUpdate();};
  this.avoidListener=(event:window.AvoidAreaOptions):void=>run('window.avoid',():void=>{if(event.type===window.AvoidAreaType.TYPE_KEYBOARD){this.prepareLayoutChanged();this.prepareGate.changed('KEYBOARD_AVOID_CHANGED');this.observePrepare();}});
  this.windowListener=(event:window.WindowEventType):void=>run('window.event',():void=>{if(event===window.WindowEventType.WINDOW_HIDDEN||event===window.WindowEventType.WINDOW_INACTIVE||event===window.WindowEventType.WINDOW_DESTROYED){this.windowShown=false;this.prepareGate.invalidate('PREPARE_WINDOW_CHANGED');}else if(event===window.WindowEventType.WINDOW_SHOWN||event===window.WindowEventType.WINDOW_ACTIVE)this.windowShown=true;this.observePrepare();});
  this.heightListener=(height:number):void=>run('window.keyboardHeight',():void=>{this.keyboardHeight=height;this.prepareLayoutChanged();this.prepareGate.changed('KEYBOARD_HEIGHT_CHANGED');this.observePrepare();});
  this.modeListener=(_mode:window.WindowStatusType):void=>run('window.mode',():void=>{this.prepareLayoutChanged();this.prepareGate.invalidate('WINDOW_CONFIGURATION_CHANGED');this.observePrepare();});
  w.on('avoidAreaChange',this.avoidListener);w.on('windowEvent',this.windowListener);w.on('keyboardHeightChange',this.heightListener);w.on('windowStatusChange',this.modeListener);this.windowSubscribed=true;
 }
`;
// Keep a private copied after-value for raw error evidence; gate still owns the authority.
one('this.prepareGate.sampleFrame(this.prepareOwnerValue659(s));','{const fresh=this.prepareOwnerValue659(s);this.prepareLastOwner659=fresh;this.prepareGate.sampleFrame(fresh);}');
const at=s.lastIndexOf('}');s=s.slice(0,at)+methods+s.slice(at);
if(/ownerV4\.same\(|before\.same\(/.test(s))throw Error('T659_METHOD_OWNER_REMAINS');
fs.writeFileSync(file,s);
};
