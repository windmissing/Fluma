    this.probeOwnerKey=this.ownerKey();
    let request=this.pendingRoi;
    if(request!==null&&((request.ownerKey.length!==0&&request.ownerKey!==this.probeOwnerKey)||
       (request.fixtureKey.length!==0&&request.fixtureKey!==this.windowFixtureKey()))){
      this.cancelWindowProbe('RESERVATION_IDENTITY_CHANGED');request=null;
    }
    let receipt:SingleSandReceipt;
    if(request===null){
      receipt=this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta);
    }else{
      prep.packet.probeRequestId=request.id;prep.packet.probeElements=request.roi.length;
      prep.packet.probeBytes=request.roi.byteLength;this.pendingRoi=null;
      try{
        receipt=this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta,request.roi);
        const evidence=this.native.takeSingleSandWindowProbe();
        this.probeEvidence=evidence!==null&&evidence.manifest.requestId===request.id?
          evidence:cancelledWindowProbe(request,'NATIVE_RECEIPT_MISSING_OR_MISMATCH');
      }catch(e){
        this.probeEvidence=cancelledWindowProbe(request,'NATIVE_PROBE_EXCEPTION');
        this.failure='NATIVE_PROBE_EXCEPTION';return null;
      }finally{
        prep.packet.probeRequestId=0;prep.packet.probeElements=0;prep.packet.probeBytes=0;
      }
    }
