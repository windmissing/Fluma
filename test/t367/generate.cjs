const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t359/generate.cjs'),out],{stdio:'inherit'});
const base=path.join(out,'entry/src/main/ets'),edits=[];
function patch(rel,from,to){
 const p=path.join(base,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');
 if(s.split(from).length!==2)throw Error('T367_ANCHOR '+rel+' '+from);
 fs.writeFileSync(p,s.replace(from,to));edits.push({rel,from,to});
}
const cuts='physics/derived/StopCuts.ets',hot='physics/P011ParticleHot.ets',field='physics/WaterSandField.ets';
const model='physics/derived/P051DerivedDemoV2.ets',facade='physics/derived/P051DerivedFacadeV2.ets';
const commit='physics/derived/P011DerivedCommitV2.ets',page='pages/P051DerivedV2Demo.ets';
fs.copyFileSync(path.join(__dirname,'StopCuts.ets'),path.join(base,cuts));
patch(field,'export class WaterSandField {',`export class WaterSandField {
 t367NextId():number {return this.derivedV2?.canonical.pool.nextId??0;}
 t367Admission(firstId:number,r:P051DryAdmissionReceipt):void {
  const d=this.derivedV2;
  if(d!==null&&r.admissionTransactionId!==null&&r.admittedH+r.admittedL>0)
   d.budget.cuts.admission(firstId,r.runId,this.renderStateEpoch,r.admissionTransactionId,r.sourceEpoch,r.admittedH+r.admittedL,d.canonical.pool,d.domain.field.surface);
 }
 t367Stop(reason:string,time:number):void {
  const d=this.derivedV2;
  if(d!==null){const c=d.budget.cuts;c.stop(reason,time);this.t359CaptureBefore();c.capture(9,d.canonical.pool,d.domain.field.surface,d.domain.field.mh,d.domain.field.ml);}
 }`);
patch(field,'c.capture(0,this.derivedV2.canonical.pool);','const f=this.derivedV2.domain.field;c.capture(0,this.derivedV2.canonical.pool,f.surface,f.mh,f.ml);');
patch(field,'c.capture(7,this.derivedV2.canonical.pool);','const f=this.derivedV2.domain.field;c.capture(7,this.derivedV2.canonical.pool,f.surface,f.mh,f.ml);');
patch(field,'d.budget.cuts.substep=this.p011SubstepId+1;', 'd.budget.cuts.substep=this.p011SubstepId+1;d.budget.cuts.attempt=0;');
patch(field,'      this.derivedV2?.budget.endSubstep(0,',`      if(this.derivedV2!==null){
        const d=this.derivedV2,c=d.budget.cuts;
        c.committed(this.renderPhysicsCommitId,this.effectiveTilt,substepDt,d.canonical.pool,d.domain.field.surface,d.domain.field.mh,d.domain.field.ml);
      }
      this.derivedV2?.budget.endSubstep(0,`);
patch(facade,'        let receipt: P051DryAdmissionReceipt | null = null;', '        const firstId=this.field.t367NextId();\n        let receipt: P051DryAdmissionReceipt | null = null;');
patch(facade,'        return { ok: receipt !== null && receipt.rejectedReason === null && fault === null,', '        if(receipt!==null&&receipt.rejectedReason===null&&fault===null)this.field.t367Admission(firstId,receipt);\n        return { ok: receipt !== null && receipt.rejectedReason === null && fault === null,');
patch(facade,'    step(dt: number): DerivedStepResultV2 {','    t367Stop(reason:string,time:number):void {this.field.t367Stop(reason,time);}\n    step(dt: number): DerivedStepResultV2 {');
patch(hot,'  try{code=applyParticlePlanIntoBody(work,expectedStepEpoch,plan,sink,results);return code;}','  const cuts=derived===null?null:derived.budget.cuts;if(cuts!==null)cuts.role=role;\n  try{code=applyParticlePlanIntoBody(work,expectedStepEpoch,plan,sink,results);return code;}');
patch(hot,'  finally{fine.fineEnd','  finally{if(cuts!==null)cuts.role=0;fine.fineEnd');
patch(hot,'  work.derivedV2?.budget.cuts.predicted(plan);','  work.derivedV2?.budget.cuts.predicted(plan,results,work.events);');
patch(hot,'c.capture(2,work.derivedV2.canonical.pool);','const f=work.derivedV2.domain.field;c.capture(2,work.derivedV2.canonical.pool,f.surface,f.mh,f.ml);');
patch(commit,'this.owner.budget.cuts.capture(3,this.owner.canonical.pool);','const f=this.owner.domain.field;this.owner.budget.cuts.capture(3,this.owner.canonical.pool,f.surface,f.mh,f.ml);');
patch(commit,'if(s.field.particleState)c.capture(4,s.field.particleState);','c.finalTilt=s.effectiveTiltDeg;if(s.field.particleState)c.capture(4,s.field.particleState,s.field.sandTop,s.field.mh,s.field.ml);');
patch(commit,'if(out.field.particleState)this.owner.budget.cuts.capture(5,out.field.particleState);','if(out.field.particleState)this.owner.budget.cuts.capture(5,out.field.particleState,out.field.sandTop,out.field.mh,out.field.ml);');
patch(model,'    stop(): void { this.held = false; this.clock.stop(); }',`    stop(reason:string='LIFECYCLE',time:number=Date.now()): void {
        if(this.held)this.owner.t367Stop(reason,time);
        this.held = false; this.clock.stop();
    }`);
patch(model,'            this.stop();\n            return;', "            this.stop('CANCEL');\n            return;");
patch(model,"        if (input.source === 'stale')\n            this.stop();","        if (input.source === 'stale')\n            this.stop('STALE_SENSOR');");
patch(model,'                    this.stop();','                    this.stop(\'ADMISSION_REJECT\');');
patch(model,'        if(r.derivedFault===null&&r.original!==null)cuts.elapsed+=r.original.substeps*r.original.substepDt;', '        ');
patch(model,'cuts.admissionCount=this.seq;', 'cuts.admissionDelta=cuts.acceptedTotal-cuts.lastFrameAccepted;cuts.lastFrameAccepted=cuts.acceptedTotal;');
patch(model,"this.owner.reset(); this.seq = 0;", "this.owner.reset();this.budget.cuts.resetRun(); this.seq = 0;");
patch(page,'this.model.stop();return;', "this.model.stop(event.type===TouchType.Up?'UP':'CANCEL',Date.now());return;");
fs.writeFileSync(path.join(out,'t367-manifest.json'),JSON.stringify({revision:'T367_STOP_COHORT_V1',isolated:true,sha256:crypto.createHash('sha256').update(JSON.stringify(edits)).update(fs.readFileSync(path.join(__dirname,'StopCuts.ets'))).digest('hex')},null,2));
