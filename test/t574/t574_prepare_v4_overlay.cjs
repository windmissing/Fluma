const fs=require('node:fs'),path=require('node:path');
module.exports=function(out){
 const base=path.join(out,'entry/src/main/ets'),file=path.join(base,'pages/P051DerivedV2Demo.ets');
 fs.copyFileSync(path.join(__dirname,'../t426/PrepareReadinessV4.ets'),path.join(base,'render/PrepareReadinessV4.ets'));
 fs.copyFileSync(path.join(__dirname,'../t426/PreparePairingEvidence.ets'),path.join(base,'render/PreparePairingEvidence.ets'));
 let s=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 s="import { PrepareReadinessV4, PrepareAttemptOwner } from '../render/PrepareReadinessV4';\n"+s;
 s=s.replace('private readonly prepareGate:PrepareReadiness=new PrepareReadiness(', 'private readonly prepareGate:PrepareReadinessV4=new PrepareReadinessV4(');
 s=s.replace("@State private prepareText:string='CONFIGURING';","@State private reserveEnabled:boolean=false;\n @State private prepareText:string='CONFIGURING';");
 s=s.replace(' private prepareUpdate():void {'," private prepareUpdate():void {\n  this.reserveEnabled=this.prepareGate.state==='READY'&&!this.configuring&&!this.exporting&&this.evidence===null&&this.singleSand.capture.configurable();");
 s=s.replace(".enabled(this.prepareGate.state==='READY')",'.enabled(this.reserveEnabled)');
 s=s.replace('  s.nativeReady=native.ready;s.run=native.run;s.opticsRevision=native.revision;', '  s.nativeReady=native.ready;s.run=native.run;s.opticsRevision=native.revision;\n  const owner=new PrepareAttemptOwner();owner.prepareId=this.prepareGate.prepareId;owner.pageGeneration=this.prepareGate.pageGeneration;owner.branch=this.prepareBranch;owner.windowId=s.windowId;owner.windowGeneration=s.windowGeneration;owner.controller=s.controller;owner.session=s.session;owner.surface=s.surface;owner.context=s.context;owner.run=s.run;if(this.prepareObservation!==null&&this.prepareObservation.prepareId===this.prepareGate.prepareId)this.prepareGate.sampleFrame(owner);');
 s=s.replace("const r=this.singleSand.reserveCapture(roi,this.probeFixtureKey,Number(this.maxHoldText));","    const ticket=this.prepareGate.checkForReserve(this.readPrepareSnapshot(),p013NowMs());if(!ticket.valid){this.probeStatus=ticket.reason;this.prepareUpdate();return;}\nconst r=this.singleSand.reserveCapture(roi,this.probeFixtureKey,Number(this.maxHoldText));");
 s=s.replace('this.prepareGate.consume();',`if(!this.prepareGate.consumeTicket(ticket)){this.singleSand.capture.invalidate('PREPARE_TICKET_MISMATCH');this.probeStatus='PREPARE_TICKET_MISMATCH';}`);
 s=s.replace(' private async reserve():Promise<void> {',' private async reserveImpl598():Promise<void> {');
 const methods=`
 private reserveIo598:number=0;private reserveSerial598:number=0;private reserveFirst598:Uint32Array=new Uint32Array(64);private reserveLast598:Uint32Array=new Uint32Array(64);
 private async reserve():Promise<void>{const id=++this.reserveSerial598;const before=this.prepareGate.state;let failure='';this.reserveIo598=0;try{await this.reserveImpl598();}catch(error){failure=String(error);if(this.reserveIo598===1)this.reserveIo598=3;}finally{const core=new Uint32Array(64);core[0]=id;core[1]=this.prepareGate.prepareId;core[2]=this.prepareGate.pageGeneration;core[3]=this.prepareGate.observationSerial;core[4]=this.prepareGate.inputSerial;core[5]=this.configuring?1:0;core[6]=this.exporting?1:0;core[7]=this.evidence!==null?1:0;core[8]=this.reserveIo598;core[11]=parseWindowRoi(this.probeRoi)===null?0:1;core[12]=this.singleSand.capture.status().state==='RESERVED'?1:0;core[13]=this.prepareGate.reason==='PREPARE_PERMIT_CONSUMED'?1:0;core[14]=this.prepareGate.ownerSerial;core[9]=before==='READY'?1:0;core[10]=this.prepareGate.state==='READY'?1:0;const reason=failure||this.prepareGate.reason||this.probeStatus;for(let i=0;i<Math.min(48,reason.length);i++)core[16+i]=reason.charCodeAt(i);if(this.reserveFirst598[0]===0)this.reserveFirst598.set(core);this.reserveLast598.set(core);this.prepareUpdate();}}
 reserveAttemptWitnessV1(id:number):string{const first:number[]=[],last:number[]=[];if(id===0||id===this.reserveFirst598[0])for(let i=0;i<64;i++)first.push(this.reserveFirst598[i]);if(id===0||id===this.reserveLast598[0])for(let i=0;i<64;i++)last.push(this.reserveLast598[i]);const raw=JSON.stringify({revision:'ReserveAttemptWitnessV1',coreBytes:256,first:first,last:last});return this.rawBytes540(raw)<=4096?raw:'{"state":"INCOMPLETE_WITNESS_UTF8_LIMIT"}';}

 `;
 const start=s.indexOf(' private async reserveImpl598'),end=s.indexOf(' private async exportWindow',start);let reserve=s.slice(start,end);reserve=reserve.replace('  const context=getContext(this)', '  this.reserveIo598=1;\n  const context=getContext(this)').replace('  await this.statusCold();','  this.reserveIo598=2;\n  await this.statusCold();');s=s.slice(0,start)+reserve+s.slice(end);
 const at=s.lastIndexOf('}');s=s.slice(0,at)+methods+s.slice(at);
 fs.writeFileSync(file,s);
 require('./t659_owner_glue.cjs')(out);
};
