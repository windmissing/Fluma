import { P011ParticleSnapshot, particleSnapshotBufferAtHot } from '../P011ParticleField';
import { DerivedViewV2 } from '../derived/P011DerivedViewV2';
import { TxWordUndo } from './TxWordUndo';

// T420 sparse dry row closure. Four births, one retire or one crossing.
// The checked old slot/column set also covers the derived exact70 writer.
export class CpuLocalRowUndoV1 {
 private readonly undo:TxWordUndo;
 private readonly targets:DataView[]=[];
 private readonly widths:number[]=[8,8,8,8,8,1,1,1,4,4];
 private readonly pool:P011ParticleSnapshot;
 private readonly before:P011ParticleSnapshot;
 private readonly view:DerivedViewV2;
 private readonly beforeView:DerivedViewV2;
 private live=0;private free=0;private nextId=0;private revision=0;
 rows=0;fallbackRows=0;removedBytes=0;undoBytes=0;replayedBytes=0;materializedBytes=0;
 selected=false;afterReady=false;materialized=false;
 constructor(pool:P011ParticleSnapshot,before:P011ParticleSnapshot,view:DerivedViewV2,beforeView:DerivedViewV2){
  this.pool=pool;this.before=before;this.view=view;this.beforeView=beforeView;
  const buffers:ArrayBuffer[]=[];
  for(let i=0;i<13;i++){
   buffers.push(particleSnapshotBufferAtHot(pool,i));this.targets.push(new DataView(particleSnapshotBufferAtHot(before,i)));
  }
  for(const b of view.persistentBuffers())buffers.push(b);
  for(const b of beforeView.persistentBuffers())this.targets.push(new DataView(b));
  // At most 48 slot words + 1 free entry + 2 × (2 × 72 + 1) column words.
  this.undo=new TxWordUndo(buffers,339);
 }
 private log(id:number,index:number,width:number):void {this.undo.beforeWrite(id,index*width,width);this.undoBytes+=width;}
 private slot(slot:number):void {
  const p=this.pool,b=this.before;
  for(let i=0;i<this.widths.length;i++)this.log(i,slot,this.widths[i]);
  this.log(16,slot,8);this.log(17,slot,4);
  // T381/Z08 row readers need only this original slot, even before cold materialization.
  b.pId[slot]=p.pId[slot];b.pX[slot]=p.pX[slot];b.pY[slot]=p.pY[slot];b.pVx[slot]=p.pVx[slot];b.pVy[slot]=p.pVy[slot];
 }
 private column(col:number):void {
  this.log(15,col,1);
  for(let phase=0;phase<2;phase++){
   this.log(11+phase,col,8);
   const bucket=phase*128+col;
   this.log(14,bucket,4);
   for(let j=0;j<70;j++)this.log(13,bucket*70+j,2);
  }
 }
 begin(slot:number,births:number,column:number,target:number,mayRetire:boolean):void {
  this.clear();this.selected=true;
  const p=this.pool;this.live=p.liveCount;this.free=p.freeCount;this.nextId=p.nextId;this.revision=this.view.revision;
  if(slot>=0)this.slot(slot);
  for(let i=0;i<births;i++)this.slot(p.freeSlots[p.freeCount-1-i]);
  if(mayRetire)this.log(10,p.freeCount,4);
  this.column(column);if(target!==column)this.column(target);
 }
 restoreBefore():void {
  for(let i=this.undo.tail-1;i>=0;i--){this.undo.copyPreviousWord(i,this.targets[this.undo.bufferId[i]]);this.replayedBytes+=this.undo.width[i];}
  this.before.liveCount=this.live;this.before.freeCount=this.free;this.before.nextId=this.nextId;this.beforeView.revision=this.revision;
 }
 clear():void {this.undo.commitRoot();this.selected=false;this.afterReady=false;this.materialized=false;}
 record(bytes:number):void {this.rows++;this.removedBytes+=bytes;}
}
