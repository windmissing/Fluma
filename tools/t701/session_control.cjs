'use strict';

function one(body, anchor, replacement) {
  if (body.split(anchor).length !== 2) throw Error('T717_PARENT_DRIFT ' + anchor);
  return body.replace(anchor, replacement);
}

function applySessionControl(edit) {
  edit('cpp/gpu_resident/app_bridge.cpp', body => {
    body = '#include "t717_session.h"\n' + body;
    body = one(body, ' } firstFrameInit;', ` } firstFrameInit;
#ifdef GRS_APP_M1
 t717::Evidence firstFrameSession;
 t717::Scope FirstFrameScopeLocked()const{
  using t717::Wide;
  return {epoch.load(),Wide(identity.run),Wide(identity.owner),Wide(identity.context),
          Wide(identity.surface),permissionRevision,progress.session,
          reinterpret_cast<uintptr_t>(window),firstFrameInit.epoch==epoch?firstFrameInit.ownerTid:0};
 }
#endif`);
    const pulse = '  const auto result=progress.Pulse(session,seq,callback,observedPulse.observedReceiveNs);';
    body = one(body, pulse, `  const auto priorSession=progress.session, priorSequence=progress.sequence;
  const auto priorCallback=progress.uiNs, priorReceive=progress.receiveNs;
  const bool openBefore=progress.open;
  const auto priorFault=progress.fault;
${pulse}`);
    // A stale call observes prior revocation. Only the clock's newly rejected
    // timestamp can enter the clock-fault handler here.
    body = one(body, '}else if(progressV2&&!progress.open){',
      '}else if(progressV2&&!progress.open&&' +
      '(std::strcmp(result,"CLOCK_GAP")==0||std::strcmp(result,"PULSE_CLOCK_REVERSED")==0||' +
      'std::strcmp(result,"CONTROL_CLOCK_REVERSED")==0)){');
    body = one(body, '  checkingPulse=false;wake.notify_all();return progressV2?result:"PAGE_PULSE_ACCEPTED";',
      `  if(progressV2&&(!progress.open||session!=priorSession)){
   auto& p=firstFrameSession.rejection;
   if(!p.observed||p.scope.epoch!=epoch||p.scope.session!=priorSession){
    p={FirstFrameScopeLocked(),session,seq,callback,observedPulse.observedReceiveNs,
       priorSequence,priorCallback,priorReceive,openBefore,progress.open,result,priorFault,progress.fault,phase,true};
   }
  }
  checkingPulse=false;wake.notify_all();return progressV2?result:"PAGE_PULSE_ACCEPTED";`);
    const accepted = '  ++accepted;rootNonce=a.nonce;CounterIdentityLocked();';
    body = one(body, accepted, `${accepted}
  auto& first=firstFrameSession.firstRoot;
  if(!first.ns||first.scope.epoch!=epoch||first.scope.session!=progress.session){
   first={FirstFrameScopeLocked(),t717::Wide(a.nonce),accepted,t717::Wide(a.physics),Stamp()};
  }`);
    body = one(body, '    RootRequest presenting;presenting.nonce=a.nonce;',
      '    RootRequest presenting;presenting.nonce=a.nonce;t717::Scope presentingScope;');
    body = one(body, '     if(!PermitLocked(presenting,Stamp()))continue;',
      '     if(!PermitLocked(presenting,Stamp()))continue;\n     presentingScope=FirstFrameScopeLocked();');
    body = one(body, 'RootRequest root;m1::RootIsolation* gate;};\n    Swap args{this,seen,display,surface,presenting,&isolation};',
      'RootRequest root;m1::RootIsolation* gate;t717::Scope scope;};\n    Swap args{this,seen,display,surface,presenting,&isolation,presentingScope};');
    const swap = '     if(!v.gate->Check(v.root,m1::RootCut::Swap)){v.app->Mark(BridgeStage::SwapEnd,v.epoch,0,0,2);return false;}';
    body = one(body, swap, `     const auto nonce=t717::Wide(v.root.nonce);
     bool observe=false;
     {
      std::lock_guard<std::mutex> lock(v.app->mutex);
      auto& first=v.app->firstFrameSession.firstSwap;
      observe=!first.begin||first.scope.epoch!=v.scope.epoch||first.scope.session!=v.scope.session;
      if(observe)first={v.scope,nonce,v.app->Stamp()};
     }
     const auto record=[&](int code,bool called,bool native){
      if(!observe)return;
      std::lock_guard<std::mutex> lock(v.app->mutex);
      auto& first=v.app->firstFrameSession.firstSwap;
      first.code=code;if(!first.end)first.end=v.app->Stamp();
      first.called=called;first.returned=called;first.nativeSucceeded=native;
     };
     if(!v.gate->Check(v.root,m1::RootCut::Swap)){record(2,false,false);v.app->Mark(BridgeStage::SwapEnd,v.epoch,0,0,2);return false;}`);
    body = one(body, '     const bool native=eglSwapBuffers(v.d,v.s)==EGL_TRUE;',
      `     if(observe){std::lock_guard<std::mutex> lock(v.app->mutex);auto& first=v.app->firstFrameSession.firstSwap;
      first.called=true;first.nativeBegin=v.app->Stamp();}
     const bool native=eglSwapBuffers(v.d,v.s)==EGL_TRUE;
     if(observe){std::lock_guard<std::mutex> lock(v.app->mutex);auto& first=v.app->firstFrameSession.firstSwap;
      first.returned=true;first.nativeSucceeded=native;first.end=v.app->Stamp();}`);
    body = one(body, '     const uint32_t outcome=allowed?(native?0:1):(native?2:3);',
      '     const uint32_t outcome=allowed?(native?0:1):(native?2:3);\n     record(int(outcome),true,native);');
    return one(body, 'a.FirstFrameInitJsonLocked(o);', 'a.FirstFrameInitJsonLocked(o);a.firstFrameSession.Json(o);');
  });
  edit('ets/physics/GpuM1DemoController.ets', body => one(body, ' progressClockRevision?:string;',
    ` firstFrameSession?:M1FirstFrameSession;
 progressClockRevision?:string;`).replace('export interface GpuM1DemoStatus {',
    `interface M1SessionScope { epoch:string; runLo:number; runHi:number; session:string; }
interface M1PulseRejection { scope:M1SessionScope; token:string; openAfter:boolean; result:string; faultAfter:string; }
interface M1FirstFrameSession { pulse:M1PulseRejection|null; }
export interface GpuM1DemoStatus {`));
  edit('ets/pages/GpuResidentM1.ets', body => {
    body = one(body, '  const code:string=m1.pagePulse(this.session,String(++this.sequence),String(ns));\n  this.owner.tick(ns,code);',
      '  const code:string=m1.pagePulse(this.session,String(++this.sequence),String(ns));\n  this.owner.tick(this,ns,code);');
    body = one(body, ' onPageShow():void{this.frame.owner=null;this.frame=new M1DemoFrame();this.frame.owner=this;this.queued=false;this.shown=true;this.syncActive();}',
      ' onPageShow():void{this.shown=true;this.syncActive();}');
    body = one(body, '   this.frame.owner=null;this.frame=new M1DemoFrame();this.frame.owner=this;\n   this.frame.session=m1.beginPageSession();this.queued=false;',
      '   this.bindFrame();');
    body = one(body, ' private updateSensor():void {', ` private bindFrame():void {
  if(!this.active)return;
  this.frame.owner=null;this.frame=new M1DemoFrame();this.frame.owner=this;
  this.frame.session=m1.beginPageSession();this.queued=false;
 }
 private requestNewFrame():void {this.bindFrame();this.schedule();}
 private updateSensor():void {`);
    body = one(body, 'if(this.active&&!this.queued)', "if(this.active&&this.frame.owner===this&&this.frame.session!=='0'&&!this.queued)");
    body = one(body, ' tick(ns:number,pulse:string):void {\n  this.queued=false;if(!this.active)return;',
      ` tick(frame:M1DemoFrame,ns:number,pulse:string):void {
  if(frame!==this.frame||frame.owner!==this)return;
  this.queued=false;if(!this.active)return;
  const terminal=pulse==='STALE_PAGE_SESSION'||pulse==='CLOCK_GAP'||pulse==='PULSE_CLOCK_REVERSED'||pulse==='CONTROL_CLOCK_REVERSED';
  if(terminal)frame.owner=null;`);
    body = one(body, '   this.model.refresh();const s=this.model.snapshot;',
      `   this.model.refresh();const s=this.model.snapshot;
   const witness=s?.firstFrameSession?.pulse;
   const reason=pulse==='STALE_PAGE_SESSION'&&s!==null&&witness!==undefined&&witness!==null&&
    witness.token===frame.session&&witness.scope.session===frame.session&&witness.scope.epoch===String(s.epoch)&&
    witness.scope.runLo===s.runLo&&witness.scope.runHi===s.runHi&&
    !witness.openAfter&&witness.faultAfter!==''?witness.faultAfter:pulse;`);
    body = one(body, "this.model.stop(pulse);", "this.model.stop(reason);");
    body = one(body, 'if(this.lastTextNs<0||ns<this.lastTextNs||ns-this.lastTextNs>=33333334)', 'if(terminal||this.lastTextNs<0||ns<this.lastTextNs||ns-this.lastTextNs>=33333334)');
    body = one(body, "(s?.stage??s?.state??'STATUS_FAULT'):pulse)", "(s?.stage??s?.state??'STATUS_FAULT'):reason)");
    body = one(body, "Button('新空场').onClick(()=>this.model.reset());",
      "Button('新空场').onClick(()=>{this.model.reset();this.requestNewFrame();});");
    body = one(body, '.onClick(()=>this.model.applyOptics(this.sigma,this.gain,this.widthPx))',
      ".onClick(()=>{if(this.model.applyOptics(this.sigma,this.gain,this.widthPx)==='NEW_EMPTY_M1_OPTICS_RUN_REQUESTED')this.requestNewFrame();})");
    for (const k of [192,512]) {
      body = one(body, `.onClick(()=>this.model.profile(${k},128))`,
        `.onClick(()=>{this.model.profile(${k},128);this.requestNewFrame();})`);
    }
    return body;
  });
}

module.exports = {applySessionControl};
