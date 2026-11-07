  readonly t394=new T394Phase();
  private t394Input:T394Input|null=null;
  private t394Auto=false;
  private t394Roi:Int32Array|null=null;
  private t394GeometryKey='';
  private t394HoldState:T394Hold|null=null;
  private t394PrepareCut='';
  private t394PrepareCode:number|null=null;
  private t394BaselineHash='';
  private t394BaselineDigest='';
  private t394LastEvidence:T394Archive|null=null;
  beginT394(name:string,locked:boolean,auto:boolean=false,reuse:boolean=false):void {
    if(this.pendingRoi!==null)this.rejectT394('PHASE_CHANGED');
    this.t394.begin(name,locked,p013NowMs());
    this.t394Input=null;this.t394HoldState=null;this.t394Auto=auto;
    this.t394LastEvidence=null;
    if(!reuse){this.t394Roi=null;this.t394GeometryKey='';this.t394BaselineHash='';this.t394BaselineDigest='';}
  }
  t394WantsEvidence():boolean {
    return this.t394Auto||this.pendingRoi!==null||this.t394.locked&&this.t394.sample===null;
  }
  t394Frame(input:T394Input):void {this.t394Input=input;}
  t394Hold():T394Hold|null {return this.t394HoldState;}
  t394VerifyHold():T394Hold|null {
    const hold=this.t394HoldState;
    if(hold===null)return null;
    const p=this.preparation.packet,current=this.native.getState();
    if(current.surfaceEpoch!==p.surface||current.contextEpoch!==p.context||hold.token!==p.token||hold.frame!==p.frame){
      this.rejectT394('DISPLAY_HOLD_OWNER_CHANGED');return null;
    }
    return hold;
  }
  t394PhaseStatus():string {return this.t394.status;}
  rejectT394(cut:string,receipt:SingleSandReceipt|null=null):void {
    this.t394.reject(cut);this.t394Auto=false;this.t394HoldState=null;
    this.cancelWindowProbe(cut);
    this.t394LastEvidence=new T394Archive(this.t394.sequence,this.t394.name,this.t394.startedMs,
      this.t394.status,this.t394.firstFailure,null,this.t394Input,null,receipt,this.t394PrepareCode);
    if(this.probeEvidence!==null&&this.probeEvidence.phase===undefined)
      this.probeEvidence.phase=this.t394LastEvidence;
    this.t394Input=null;
  }
  exportT394Phase():string {return this.t394LastEvidence===null?'':JSON.stringify(this.t394LastEvidence);}
  windowFixtureKey():string {return this.t394.sample?.fixtureKey??'';}
  windowFixture():string {return this.t394.locked?this.t394.sample?.fixture.roi??'':'';}
  private t394ConfigureAuto(p:SingleSandPacketV1):void {
    if(!this.t394Auto)return;
    this.t394Auto=false;
    const geometry=t394Geometry(p);
    if(this.t394Roi===null){
      this.t394Roi=t394DynamicRoi(p);this.t394GeometryKey=geometry;
    }
    const roi=this.t394Roi;
    if(roi===null||geometry!==this.t394GeometryKey||!this.armWindowProbe(roi,'')){
      this.rejectT394('GEOMETRY_UNSUPPORTED');return;
    }
  }
  private t394AfterDraw(state:P011PublishedRenderState,receipt:SingleSandReceipt,
    packMs:number,drawMs:number,request:WindowProbeRequest|null):void {
    const prep=this.preparation,p=prep.packet,input=this.t394Input;
    if(!receipt.currentFrameSucceeded||!receipt.swapSucceeded||receipt.actualToken!==p.token){
      this.rejectT394('DRAW:'+receipt.firstFailureStage+':'+receipt.reason,receipt);
      return;
    }
    this.t394Input=null;
    if(request===null&&(!this.t394.locked||this.t394.sample!==null))return;
    const lockedSnapshot=this.t394.sample;
    const sample=this.t394.accept(p,receipt,state,prep.profile,prep.slot.particleXY,prep.slot.particleMeta,
      input?.copyMs??0,packMs,drawMs,input?.frameNs??0);
    if(sample!==null&&this.t394.locked&&lockedSnapshot!==null)this.t394.sample=lockedSnapshot;
    const evidence=this.probeEvidence;
    let phaseStatus=this.t394.status;
    if(request!==null&&evidence!==null){
      const isA=this.t394.name.endsWith('_A'),isB=this.t394.name.endsWith('_B');
      let holdable=sample!==null&&evidence.manifest.captureStatus==='CAPTURED'&&evidence.manifest.actualToken===sample.token;
      if(holdable&&sample!==null&&isB){
        const newVisible=sample.projectionMeta.some((v:number,i:number)=>i%2===1&&v===2);
        const admitted=input?.admissions.some((row:P051DryAdmissionReceipt)=>row.sequence===2&&row.admittedH>0)??false;
        holdable=this.t394BaselineHash!==sample.particleHash&&this.t394BaselineDigest===sample.digest&&
          newVisible&&admitted&&(input?.steps.length??0)>0;
        if(!holdable)phaseStatus='DYNAMIC_B_INPUT_NOT_PROVEN';
      }
      if(holdable&&sample!==null){
        if(isA){this.t394BaselineHash=sample.particleHash;this.t394BaselineDigest=sample.digest;}
        this.t394HoldState=new T394Hold(this.t394.sequence,this.t394.name,request.id,sample.token,sample.frame,
          sample.width,sample.height,sample.geometry,Date.now(),p013NowMs());
      }
    }
    this.t394LastEvidence=new T394Archive(this.t394.sequence,this.t394.name,this.t394.startedMs,phaseStatus,
      this.t394.firstFailure,sample,input,this.t394HoldState,receipt);
    if(request!==null&&evidence!==null)evidence.phase=this.t394LastEvidence;
  }
