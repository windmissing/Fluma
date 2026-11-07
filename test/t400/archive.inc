  expire(now:number):void {
    if(this.output.length!==0)return;
    this.arm.observe(now);
    if(this.arm.wallMs>=T400Arm.MAX_WALL_MS){
      this.arm.limit='WALL_15S';
      this.cancel(this.noEventStatus(),now);
    }
  }
  private noEventStatus():string {
    if(!this.arm.bound)return 'NOT_BOUND';
    if(this.arm.admissionState==='NOT_ENTERED')return 'ADMISSION_NOT_ENTERED';
    if(!this.arm.seenId)return 'TARGET_ID_NOT_OBSERVED';
    return this.firstT3===null?'NO_COMMITTED_T3':'ZERO_GRANT_ONLY';
  }
  cancel(reason:string,now:number):void {
    if(this.output.length!==0)return;
    this.arm.observe(now);
    this.frameOpen=false;
    this.freeze(reason);
  }
  finishFrame(pub: Z08PublicationIdentity|null, frame: P013SceneFrame|null, receipt: P013SceneReceipt|null,
    frameNs: number, pageFrame: number, failureText: string, now:number): void {
    if (!this.frameOpen) return;
    this.frameOpen=false;
    this.arm.observe(now);
    this.frameNs=frameNs;this.pageFrame=pageFrame;this.selectedPublication=pub;
    this.nativeFrame=frame===null?0:frame.frameId;this.renderReceipt=receipt;
    this.failure=failureText.length===0?null:JSON.parse(failureText) as Z08FailureReceipt;
    const last=this.candidates.length===0?null:this.candidates[this.candidates.length-1];
    const selected=pub!==null && last!==null && samePublication(last.publication,pub) ? last : null;
    this.presentation='PUBLICATION_NOT_FROM_CALLBACK';
    if (selected!==null && pub!==null) {
      this.presentation=z08Presentation(pub,frame,receipt,frameNs);
      selected.presentation=this.presentation;
    }
    const currentT3=this.candidates.find((c:Z08Candidate)=>c.lands.length>0)??null;
    const currentPositive=this.candidates.find((c:Z08Candidate)=>c.lands.some((l:Z08LandEvidence)=>l.event.granted>0))??null;
    if(this.firstT3===null)this.firstT3=currentT3;
    if(this.firstPositive===null)this.firstPositive=currentPositive;
    const matchingLand=selected!==null&&selected.lands.some((l:Z08LandEvidence)=>l.verified&&l.event.granted>0);
    this.landSwap=matchingLand&&this.presentation==='PRESENT_CURRENT_SWAP';
    if(this.failure!==null&&this.failure.failed){this.freeze('PHYSICS_FAILURE');return;}
    if(failureText.length!==0){this.freeze('STEP_FAILED_WITHOUT_CROSS_RECEIPT');return;}
    if(this.frameIncomplete||this.incomplete||this.candidates.some((c:Z08Candidate)=>c.incomplete)){
      this.freeze('INCOMPLETE');return;
    }
    this.arm.limit=this.arm.limitReached();
    if(this.arm.limit==='WALL_15S'){
      this.freeze(currentT3===null?this.noEventStatus():'WALL_EXPIRED_WITH_COMMITTED_T3');return;
    }
    if(this.arm.mode==='NEXT'){this.freeze(selected===null?'PUBLICATION_MISSING':'CAPTURED');return;}
    if(this.arm.mode==='FIRST_T3'&&currentT3!==null&&currentPositive===null){this.freeze('ZERO_GRANT_T3');return;}
    if(currentPositive!==null){
      this.freeze(this.landSwap?'POSITIVE_LAND_CURRENT_SWAP':'POSITIVE_LAND_NO_MATCHING_SWAP');return;
    }
    if(this.arm.limit.length!==0)this.freeze(this.noEventStatus());
  }
  private freeze(reason:string):void {
    this.arm.terminal=reason;
    this.output=JSON.stringify({
      revision:'T400_Z08_ARCHIVE_V1',arm:this.arm,ids:Array.from(this.ids),
      callbacks:this.arm.callbacks,successfulSubsteps:this.arm.steps,frameNs:this.frameNs,pageFrame:this.pageFrame,
      candidates:this.candidates,firstT3:this.firstT3,firstPositive:this.firstPositive,
      selectedPublication:this.selectedPublication,presentation:this.presentation,landCurrentSwap:this.landSwap,
      renderReceipt:this.renderReceipt,nativeFrame:this.nativeFrame,systemPresentVerified:this.renderReceipt?.displayPresentVerified??false,
      failure:this.failure?.failed===true?this.failure:null,captureStatus:reason
    });
  }
