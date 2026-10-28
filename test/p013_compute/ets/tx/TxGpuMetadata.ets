import { P011ParticleSnapshot } from '../P011ParticleField';
import { P011ParticleHotWorkspace } from '../P011ParticleHot';
import { txPositiveCounter } from './TxGpuCompatWork';
// A separate instance for every fixture owner. No global arm, Map or snapshot
// pointer hash. Control revisions never participate in physical rollback.
export class TxGpuMetadata {
  readonly pools:(P011ParticleSnapshot|null)[]=new Array<P011ParticleSnapshot|null>(8).fill(null);
  readonly rows:Float64Array=new Float64Array(8*12);readonly ordinals:Uint32Array=new Uint32Array(6);
  rootId:number=0;contentRev:number=0;requestSerial:number=0;branchSerial:number=0;generation:number=1;
  role:number=0;active:boolean=true;ownerId:number=0;viewSlot:number=-1;branchGeneration:number=0;
  beginRoot():void {if(!this.active)throw new Error('TX_GPU_OWNER_INACTIVE');this.rootId=txPositiveCounter(this.rootId);this.role=0;}
  enterRole(role:number):void {if(role<1||role>5||!Number.isInteger(role))throw new Error('TX_GPU_ROLE');this.role=role;}
  bind(p:P011ParticleSnapshot,w:P011ParticleHotWorkspace):number {
    if(!this.active||this.rootId===0||this.role===0)throw new Error('TX_GPU_ROOT_OR_ROLE_REQUIRED');
    if(this.ownerId===0)this.ownerId=w.ownerId;else if(w.ownerId!==this.ownerId)throw new Error('TX_GPU_FOREIGN_WORK_OWNER');
    let slot=-1;for(let i=0;i<8;i++)if(this.pools[i]===p){slot=i;break;}
    if(slot<0){for(let i=0;i<8;i++)if(this.pools[i]===null){slot=i;this.pools[i]=p;break;}}
    if(slot<0)throw new Error('TX_GPU_VIEW_PROFILE_EXHAUSTED');
    const b=slot*12;
    if(this.rows[b]!==this.rootId||this.rows[b+1]!==w.runToken||this.rows[b+2]!==w.stepEpoch||this.rows[b+3]!==w.attemptToken||this.rows[b+4]!==w.substepId||this.rows[b+5]!==this.role){
      this.branchSerial=txPositiveCounter(this.branchSerial);this.rows[b]=this.rootId;this.rows[b+1]=w.runToken;this.rows[b+2]=w.stepEpoch;this.rows[b+3]=w.attemptToken;this.rows[b+4]=w.substepId;this.rows[b+5]=this.role;this.rows[b+6]=this.branchSerial;
    }
    // Conservative per-find revision includes rollback-restored bytes and slot
    // reuse. Native always repeats freeze/SHA; no cross-mutation hash reuse.
    this.contentRev=txPositiveCounter(this.contentRev);this.requestSerial=txPositiveCounter(this.requestSerial);
    if(this.ordinals[this.role]===0xffffffff)throw new Error('TX_GPU_ORDINAL_EXHAUSTED');this.ordinals[this.role]++;
    this.viewSlot=slot;this.branchGeneration=this.rows[b+6];this.rows[b+7]=this.contentRev;return slot;
  }
  cancel():void {this.active=false;this.generation=txPositiveCounter(this.generation);this.contentRev=txPositiveCounter(this.contentRev);}
}
