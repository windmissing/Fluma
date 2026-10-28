import { P011ClearWaterEngine } from './P011ClearWaterEngine';
import { ClearWaterCaptureBoundary,ClearWaterSceneSlot } from './P011ClearWaterCTypes';
import { ClearWaterStatus as S } from './P011ClearWaterTypes';
export class ClearWaterPublication {
 readonly slots:ClearWaterSceneSlot[];readonly boundary:ClearWaterCaptureBoundary=new ClearWaterCaptureBoundary();publishedIndex:number=-1;serial:number=0;status:S=S.OK;
 constructor(k:number){this.slots=[new ClearWaterSceneSlot(k),new ClearWaterSceneSlot(k)];}
 capture(engine:P011ClearWaterEngine,configRev:number):S {
  const b=this.boundary;b.runId=engine.runId;b.runGeneration=engine.runGeneration;b.configRev=configRev;b.inputGeneration=engine.currentInputGeneration;b.geometryEpoch=engine.geometryEpoch;b.publicationSerial=this.serial+1;
  const next=this.publishedIndex===0?1:0;this.status=engine.copyCurrentSceneInto(b,this.slots[next]);
  if(this.status===S.OK){this.slots[next].digest();this.serial++;this.publishedIndex=next;}return this.status;
 }
}
