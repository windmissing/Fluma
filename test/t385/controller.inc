  private pendingRoi:WindowProbeRequest|null=null;
  private probeEvidence:WindowProbeEvidence|null=null;
  private probeSerial:number=0;
  private probeOwnerKey:string='';
  probeBusy():boolean {return this.pendingRoi!==null||this.probeEvidence!==null;}
  private ownerKey():string {
    const p=this.preparation.packet;
    return p.run.length===0?'':p.run+':'+p.epoch+':'+p.session+':'+p.surface+':'+p.context+':'+p.geometry+':'+p.width+':'+p.height+':'+p.x+':'+p.y+':'+p.w+':'+p.h;
  }
  windowFixtureKey():string {return windowFixtureKey(this.preparation.packet);}
  windowFixture():string {
    if(this.lastReceipt===null||!this.lastReceipt.currentFrameSucceeded||
       this.lastReceipt.actualToken!==this.preparation.packet.token)return '';
    return chooseWindowFixture(this.preparation.packet,this.preparation.profile,
      this.preparation.slot.particleXY,this.preparation.slot.particleMeta);
  }
  armWindowProbe(roi:Int32Array,fixtureKey:string):boolean {
    if(this.probeBusy()||this.probeSerial>=Number.MAX_SAFE_INTEGER)return false;
    this.pendingRoi=new WindowProbeRequest(++this.probeSerial,new Int32Array(roi),fixtureKey,this.probeOwnerKey);
    return true;
  }
  cancelWindowProbe(stage:string):void {
    const request=this.pendingRoi;
    if(request===null)return;
    this.pendingRoi=null;this.probeEvidence=cancelledWindowProbe(request,stage);
  }
  takeWindowProbe():WindowProbeEvidence|null {return this.probeEvidence;}
  ackWindowProbe():void {this.probeEvidence=null;}
