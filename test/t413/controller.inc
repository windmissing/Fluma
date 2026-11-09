  readonly capture=new T413Capture();
  captureGeometry(g:CaptureGeometry):void {this.capture.setGeometry(g);}
  private captureValidate():void {
    const s=this.capture.status();
    if(s.state!=='HOLD_OPEN'&&s.state!=='SEALED')return;
    const p=this.preparation.packet,n=this.native.getState();
    if(s.owner!==this.ownerKey()||s.token!==p.token||s.frame!==p.frame||
       n.surfaceEpoch!==p.surface||n.contextEpoch!==p.context||n.widthPx!==p.width||n.heightPx!==p.height)
      this.capture.invalidate('DISPLAY_HOLD_OWNER_CHANGED');
  }
  holdStatus():HoldStatus {this.captureValidate();return this.capture.status();}
  reserveCapture(roi:Int32Array,fixtureKey:string,maxHoldMs:number):CaptureReply {
    const p=this.preparation.packet;
    const invalid=this.capture.canReserve(roi,maxHoldMs,p.width,p.height);
    if(invalid)return this.capture.reply(invalid);
    if(this.probeBusy()||this.probeSerial>=Number.MAX_SAFE_INTEGER)return this.capture.reply('PROBE_BUSY');
    if(this.t394.sequence>=Number.MAX_SAFE_INTEGER)return this.capture.reply('PHASE_ID_EXHAUSTED');
    if(this.t406PlanRoi!==null&&this.t394.name!=='CALIBRATION'&&Array.from(roi).join(',')!==Array.from(this.t406PlanRoi).join(','))return this.capture.reply('FROZEN_PLAN_ROI_MISMATCH');
    if(fixtureKey&&fixtureKey!==windowFixtureKey(p))return this.capture.reply('FIXTURE_IDENTITY');
    const native=this.native.getState();
    if(!p.enabled||!p.token||p.geometry!==this.capture.geometry().geometry||native.surfaceEpoch!==p.surface||native.contextEpoch!==p.context||
       native.widthPx!==p.width||native.heightPx!==p.height)return this.capture.reply('OWNER_NOT_READY');
    // All rejectable validation precedes the phase mutation and request commit.
    this.beginT394(this.t394.name,this.t394.locked,false,true);
    this.pendingRoi=new WindowProbeRequest(++this.probeSerial,new Int32Array(roi),fixtureKey,this.probeOwnerKey);
    return this.capture.reserve(roi,fixtureKey,maxHoldMs,this.probeSerial,this.t394.sequence);
  }
  sampleHoldClock(expectedHoldId:string):ClockSample|null {this.captureValidate();return this.capture.sample(expectedHoldId);}
  sealCapture(expectedHoldId:string):CaptureReply {this.captureValidate();return this.capture.seal(expectedHoldId);}
  ackArchive(attemptId:string,requestId:number,manifestSha256:string):CaptureReply {
    const reply=this.capture.ack(attemptId,requestId,manifestSha256);
    if(reply.state!=='REJECTED')this.probeEvidence=null;return reply;
  }
  cancelCapture(expectedAttemptId:string,reason:string):CaptureReply {
    const reply=this.capture.cancel(expectedAttemptId,reason);
    if(reply.state!=='REJECTED')this.cancelWindowProbe(reason);return reply;
  }
  releaseCapture(expectedAttemptId:string,expectedHoldId:string):CaptureReply {
    const reply=this.capture.release(expectedAttemptId,expectedHoldId);
    if(reply.state!=='REJECTED'){this.t394HoldState=null;this.probeEvidence=null;this.pendingRoi=null;}
    return reply;
  }
  private captureAfterDraw():void {
    const s=this.capture.status();if(s.state!=='RESERVED')return;
    const h=this.t394HoldState,e=this.probeEvidence,p=this.preparation.packet;
    if(h!==null&&e!==null&&e.manifest.captureStatus==='CAPTURED'&&e.manifest.actualToken===h.token&&
       e.manifest.requestedToken===h.token&&e.manifest.frameSerial===h.frame&&h.token===p.token&&h.frame===p.frame&&
       e.manifest.captureStage==='COMPLETE'&&e.manifest.presentationState==='CURRENT'&&
       e.manifest.swapSucceeded===true&&e.manifest.unbindSucceeded===true&&
       e.manifest.session===p.session&&e.manifest.surfaceEpoch===p.surface&&e.manifest.contextEpoch===p.context&&
       e.manifest.geometryEpoch===p.geometry&&e.manifest.requestedRoiWords?.join(',')===s.roi.join(',')&&
       this.lastReceipt?.presentationState==='CURRENT'){
      this.capture.open(h.requestId,h.sequence,h.token,h.frame,p.run,this.ownerKey(),h.startMonoMs);
    }else if(e!==null)this.capture.invalidate('NATIVE_CAPTURE_NOT_CURRENT');
  }
