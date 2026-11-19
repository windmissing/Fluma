import { P051EmptySandDemo } from './P051EmptySandDemo';
import { P051DiagnosticBuffer } from './P051DiagnosticBuffer';
export interface P051InputGeometry { revision: string; geometryEpoch: number; widthVp: number; heightVp: number; ready: boolean; }
export interface P051TouchPoint { id: number; x: number; y: number; }
export interface P051TouchSnapshot { type: string; touches: P051TouchPoint[]; changedTouches: P051TouchPoint[]; timestamp: number; }
// No UI/native imports: one primary pointer, one existing model clock/queue.
export class P051TouchSourceAdapter {
  private primary: number = -1;
  private epoch: number = -1;
  private run: string = '';
  private stopVersion: number = 0;
  private generation: number = 0;
  private gesture: number = 0;
  private ordinal: number = 0;
  private frame: number = 0;
  private model: P051EmptySandDemo;
  private diagnostics: P051DiagnosticBuffer | null;
  constructor(model: P051EmptySandDemo, diagnostics: P051DiagnosticBuffer | null = null) { this.model=model;this.diagnostics=diagnostics; }
  get held(): boolean { return this.primary !== -1; }
  get pointerId(): number { return this.primary; }
  get inputGeneration(): number { return this.generation; }
  get gestureId(): number { return this.gesture; }
  invalidate(reason: string): void {
    this.model.stop(reason);this.primary=-1;this.epoch=-1;this.generation++;
    this.stopVersion=this.model.stopGeneration;
  }
  sync(): void {
    if(this.held && (!this.model.sourceHeld || this.stopVersion!==this.model.stopGeneration || this.run!==this.model.field.getP011RunId()))this.invalidate('MODEL_STOP');
  }
  private record(kind: string, reason: string, e: P051TouchSnapshot, g: P051InputGeometry, active: boolean, p: P051TouchPoint | null, x: number = 0, y: number = 0): void {
    // RECEIVE/transitions always recorded; MOVE mapping is sampled, ordinal counts all.
    this.diagnostics?.record({kind:kind,reason:reason,time:e.timestamp/1e6,rawTimestamp:e.timestamp,timestampUnit:'ns',
      runId:this.model.field.getP011RunId(),commit:this.model.field.getRenderPhysicsCommitId(),frame:this.frame,pageFrame:this.frame,
      inputRevision:'P051_TOUCH_INPUT_ENGINEERING_V1',sourceApiRevision:'P051_DRY_AIR_SOURCE_V1',source:e.type,
      geometryEpoch:g.geometryEpoch,widthVp:g.widthVp,heightVp:g.heightVp,geometryReady:g.ready,foreground:active,
      inputGeneration:this.generation,gestureId:this.gesture,eventOrdinal:this.ordinal,pointerId:p===null?this.primary:p.id,
      changedCount:e.changedTouches.length,touchCount:e.touches.length,x:x,y:y});
  }
  handle(e: P051TouchSnapshot, g: P051InputGeometry, active: boolean, pageFrame: number = 0): void {
    this.frame=pageFrame;this.ordinal++;this.sync();
    const raw=e.changedTouches.length?e.changedTouches[0]:e.touches.length?e.touches[0]:null;
    if(e.type!=='MOVE'||this.ordinal%8===0)this.record('input.RECEIVE','',e,g,active,raw,raw===null?0:raw.x,raw===null?0:raw.y);
    if(e.type==='CANCEL'){this.invalidate('CANCEL');this.record('input.STOP','CANCEL',e,g,active,raw);return;}
    if(!active || !g.ready || !Number.isFinite(g.widthVp) || !Number.isFinite(g.heightVp) || g.widthVp<=0 || g.heightVp<=0 || (this.held && this.epoch!==g.geometryEpoch)) {
      this.invalidate('GEOMETRY_OR_GATE');this.record('input.STOP','GEOMETRY_OR_GATE',e,g,active,raw);return;
    }
    if(e.type==='UP') {
      if(!this.held)return;
      if(e.changedTouches.length===0){this.invalidate('AMBIGUOUS_RELEASE');this.record('input.STOP','AMBIGUOUS_RELEASE',e,g,active,raw);return;}
      for(const p of e.changedTouches)if(p.id===this.primary){this.invalidate('UP');this.record('input.STOP','UP',e,g,active,p);return;}
      return;
    }
    let point: P051TouchPoint | null = null;
    if(e.type==='DOWN') {
      if(this.held)return;
      point=raw;
    } else if(e.type==='MOVE') {
      if(!this.held)return;
      for(const p of e.touches)if(p.id===this.primary)point=p;
    } else return;
    if(point===null || !Number.isFinite(point.x) || !Number.isFinite(point.y) || point.x<0 || point.x>=g.widthVp || point.y<0 || point.y>g.heightVp) {
      this.invalidate('INVALID_PRIMARY_POINT');this.record('input.STOP','INVALID_PRIMARY_POINT',e,g,active,point);return;
    }
    const x=128*point.x/g.widthVp,y=256*(1-point.y/g.heightVp);
    if(e.type==='DOWN'||this.ordinal%8===0)this.record('input.MAP',e.changedTouches.length?'CHANGED':'TOUCHES_COMPAT',e,g,active,point,x,y);
    this.model.setDiagnosticInputTime(e.timestamp/1e6);
    if(e.type==='DOWN') {
      const receipt=this.model.start(x,y);
      if(receipt.accepted){this.primary=point.id;this.epoch=g.geometryEpoch;this.run=this.model.field.getP011RunId();this.stopVersion=receipt.stopGeneration;this.gesture++;}
      this.record('input.START',receipt.reason,e,g,active,point,x,y);
    } else this.model.move(x,y);
    this.sync();
  }
}
