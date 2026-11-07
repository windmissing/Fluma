    private z08:P011Z08Archive|null=null;
    private z08Attached=false;
    private z08Serial=0;
    private z08SaveFailure='';
    z08Id():number {return this.z08?.arm.id??0;}
    z08Fixed():boolean {return this.z08!==null&&this.z08.arm.fixture.length>0&&this.z08.exportCold().length===0;}
    z08Dt(raw:number):number {
        const arm=this.z08?.arm;
        if(arm===undefined||arm.terminal.length!==0)return raw;
        if(!this.z08Fixed())return arm.steps===T400Arm.MAX_STEPS-1?Math.min(raw,1/60):raw;
        return arm.steps<T400Arm.MAX_STEPS?1/60:0;
    }
    z08Status():string {
        if(this.z08SaveFailure)return 'SAVE_FAILED:'+this.z08SaveFailure;
        const arm=this.z08?.arm;
        return arm===undefined?'Z08_IDLE':arm.waitingStatus()+' #'+arm.id+' · '+arm.callbacks+' 帧 / '+arm.steps+' 步';
    }
    z08SaveResult(reason:string):void {this.z08SaveFailure=reason;}
    armZ08(ids:string,mode:string,now:number,fixture:string=''):string {
        if(this.z08!==null&&this.z08.exportCold().length===0)return 'Z08_ALREADY_ARMED';
        const selected:number[]=[];
        for(const item of ids.split(','))selected.push(Number(item.trim()));
        const slot=this.owner.createPublishedSlotCold();
        if(this.owner.copyPublishedRenderStateInto(slot)!=='OK')return 'Z08_ARM_IDENTITY_MISSING';
        try {
            this.z08=new P011Z08Archive(selected,new T400Arm(++this.z08Serial,slot.runId,slot.stateEpoch,now,mode,fixture));
            this.z08SaveFailure='';
            return this.z08Status();
        } catch(error) { return 'Z08_INVALID_IDS'; }
    }
    startZ08NearBed(restricted:boolean,now:number):string {
        if(this.z08!==null&&this.z08.exportCold().length===0)return 'Z08_ALREADY_ARMED';
        this.reset(now);
        this.owner.setTilt(restricted?25:0);this.owner.setShakeInput(0);
        return this.armZ08('1,2,3',restricted?'FIRST_T3':'POSITIVE',now,restricted?'TILT25':'LEVEL0');
    }
    beginZ08Frame(now:number):void {
        const archive=this.z08;
        if(archive===null)return;
        archive.expire(now);
        if(archive.exportCold().length!==0)return;
        archive.beginFrame();
        this.owner.bindZ08ArchiveCold(archive);
        this.z08Attached=true;
        archive.arm.bound=true;
    }
    finishZ08Frame(slot:DerivedPublishedSlotV2|null,frame:P013SceneFrame|null,receipt:P013SceneReceipt|null,
        ns:number,pageFrame:number,stepFailed:boolean,now:number):void {
        const archive=this.z08;
        if(archive===null||!this.z08Attached)return;
        this.z08Attached=false;
        this.owner.bindZ08ArchiveCold(null);
        archive.finishFrame(slot?.readZ08Identity()??null,frame,receipt,ns,pageFrame,
            stepFailed?this.owner.exportCrossVerticalFailureCold():'',now);
    }
    expireZ08(id:number,now:number):void {
        if(this.z08?.arm.id===id)this.z08?.expire(now);
    }
    cancelZ08(reason:string,now:number):void {
        if(this.z08Attached){this.z08Attached=false;this.owner.bindZ08ArchiveCold(null);}
        this.z08?.cancel(reason,now);
    }
    exportZ08Cold():string {return this.z08?.exportCold()??'';}
    private admitZ08Fixture(now:number):boolean {
        const archive=this.z08;
        if(archive===null||!this.z08Fixed()||archive.arm.admissionState!=='NOT_ENTERED')return true;
        const arm=archive.arm;
        if(!this.z08Attached||!arm.bound){
            arm.admissionState='NOT_BOUND';archive.cancel('NOT_BOUND',now);return false;
        }
        const x=arm.fixture==='TILT25'?63.99999:63.5,y=0.004;
        const r=this.owner.inject({revision:P051_DRY_REVISION,sourceId:51,sequence:++this.seq,inputTimeMs:now,
            x:x,y:y,seed:51,requestedH:0.3,requestedL:0});
        arm.admission=r.original;
        if(!r.ok||r.original===null||r.original.runId!==arm.run||r.original.admittedH!==0.3||r.original.admittedL!==0){
            arm.admissionState='ADMISSION_REJECTED';archive.cancel('ADMISSION_REJECTED',now);return false;
        }
        const cp=this.owner.exportCheckpointCold(),pool=cp.authority.particleState;
        if(pool!==undefined&&pool!==null){
            for(let slot=0;slot<pool.nmax;slot++){
                if(pool.pState[slot]===0)continue;
                arm.births.push(new T400Birth(slot,pool.pId[slot],cp.generations[slot],pool.pMass[slot],pool.pX[slot],pool.pY[slot]));
            }
        }
        const births=arm.births;
        if(cp.runId!==arm.run||cp.stateEpoch!==arm.epoch||pool===undefined||pool===null||pool.nmax!==512||
            births.length!==3||!([1,2,3].every((id:number)=>births.some((b:T400Birth)=>
                b.id===id&&b.generation===1&&cp.ids[b.slot]===id&&b.x===x&&b.y===y&&
                b.mass===(id===3?0.09999999999999998:0.1)&&pool.pPhase[b.slot]===0)))){
            arm.admissionState='BIRTH_IDENTITY_MISMATCH';archive.cancel('BIRTH_IDENTITY_MISMATCH',now);return false;
        }
        arm.admissionState='ADMITTED';arm.seenId=true;
        return true;
    }
