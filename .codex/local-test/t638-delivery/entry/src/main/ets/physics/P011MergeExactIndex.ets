import { TxWholePairAdapter } from './tx/TxWholePairAdapter';
// T123: fixed-capacity, exact lexicographic merge-pair search.
// Rebuilt from the current patch scratch after every successful merge.
import { P011ParticleSnapshot } from './P011ParticleField';
import { P011PoolProgressDiagnostic } from './P011PoolProgressDiagnostic';

export class P011MergeExactIndex {
  txD1:TxWholePairAdapter|null=null;
  diagnostic: P011PoolProgressDiagnostic | null=null;
  readonly groupOrder: Int32Array;
  readonly idOrder: Int32Array;
  readonly position: Int32Array;
  readonly minimum: Int32Array;
  readonly treeBase: number;
  selectedA: number=-1; selectedB: number=-1;
  sortComparisons: number=0; treeVisits: number=0; rangeQueries: number=0;
  aSelections: number=0; rebuilds: number=0; invalid: boolean=false;
  private size: number=0;
  constructor(nmax: number) {
    this.groupOrder=new Int32Array(nmax);this.idOrder=new Int32Array(nmax);
    this.position=new Int32Array(nmax);
    let base=1;while(base<nmax)base*=2;this.treeBase=base;
    this.minimum=new Int32Array(base*2);
  }
  coldBuffers(): ArrayBuffer[] {
    return [this.groupOrder.buffer,this.idOrder.buffer,this.position.buffer,this.minimum.buffer];
  }
  private compareGroup(p: P011ParticleSnapshot,a: number,b: number): number {
    if(p.pPhase[a]!==p.pPhase[b])return p.pPhase[a]<p.pPhase[b]?-1:1;
    if(p.pSource[a]!==p.pSource[b])return p.pSource[a]<p.pSource[b]?-1:1;
    if(p.pSourceId[a]!==p.pSourceId[b])return p.pSourceId[a]<p.pSourceId[b]?-1:1;
    if(p.pState[a]!==p.pState[b])return p.pState[a]<p.pState[b]?-1:1;
    const ca=Math.floor(p.pX[a]),cb=Math.floor(p.pX[b]);
    return ca===cb?0:ca<cb?-1:1;
  }
  private compare(p: P011ParticleSnapshot,a: number,b: number,group: boolean): number {
    this.sortComparisons++;this.diagnostic?.increment(10);
    if(group){const g=this.compareGroup(p,a,b);if(g!==0)return g;
      if(p.pY[a]!==p.pY[b])return p.pY[a]<p.pY[b]?-1:1;}
    if(p.pId[a]!==p.pId[b])return p.pId[a]<p.pId[b]?-1:1;
    return a===b?0:a<b?-1:1;
  }
  private sift(p: P011ParticleSnapshot,arr: Int32Array,start: number,end: number,group: boolean): void {
    let root=start;
    while(root*2+1<=end){let child=root*2+1;
      if(this.diagnostic!==null){this.diagnostic.increment(11);this.diagnostic.cursor(start,end,root,child,this.size);}
      if(child+1<=end&&this.compare(p,arr[child],arr[child+1],group)<0)child++;
      if(this.compare(p,arr[root],arr[child],group)>=0)return;
      const tmp=arr[root];arr[root]=arr[child];arr[child]=tmp;root=child;
    }
  }
  private sort(p: P011ParticleSnapshot,arr: Int32Array,group: boolean): void {
    const d=this.diagnostic;let span=-1;
    if(d!==null){d.cursor(Math.floor((this.size-2)/2),this.size-1,-1,-1,this.size);
      span=d.begin(group?'INDEX_SORT_GROUP':'INDEX_SORT_ID');d.increment(9);}
    try {
      for(let start=Math.floor((this.size-2)/2);start>=0;start--){
        d?.cursor(start,this.size-1,start,-1,this.size);this.sift(p,arr,start,this.size-1,group);}
      for(let end=this.size-1;end>0;end--){d?.cursor(0,end,0,-1,this.size);
        const tmp=arr[0];arr[0]=arr[end];arr[end]=tmp;this.sift(p,arr,0,end-1,group);}
    } finally {d?.endSpan(span,0);}
  }
  private smaller(p: P011ParticleSnapshot,a: number,b: number): number {
    if(a<0)return b;if(b<0)return a;
    return p.pId[a]<p.pId[b]?a:p.pId[b]<p.pId[a]?b:a<b?a:b;
  }
  private remove(p: P011ParticleSnapshot,slot: number): void {
    let node=this.treeBase+this.position[slot];this.minimum[node]=-1;
    while(node>1){node=Math.floor(node/2);this.minimum[node]=this.smaller(p,this.minimum[node*2],this.minimum[node*2+1]);this.treeVisits++;this.diagnostic?.increment(12);}
  }
  private rangeMin(p: P011ParticleSnapshot,left: number,right: number): number {
    this.rangeQueries++;this.diagnostic?.increment(13);let best=-1;let l=this.treeBase+left,r=this.treeBase+right;
    while(l<r){if(l%2===1){best=this.smaller(p,best,this.minimum[l]);l++;this.treeVisits++;this.diagnostic?.increment(12);}
      if(r%2===1){r--;best=this.smaller(p,best,this.minimum[r]);this.treeVisits++;this.diagnostic?.increment(12);}
      l=Math.floor(l/2);r=Math.floor(r/2);}
    return best;
  }
  private groupBegin(p: P011ParticleSnapshot,a: number): number {
    let l=0,r=this.size;while(l<r){const m=Math.floor((l+r)/2);
      if(this.compareGroup(p,this.groupOrder[m],a)<0)l=m+1;else r=m;}return l;
  }
  private groupEnd(p: P011ParticleSnapshot,a: number): number {
    let l=0,r=this.size;while(l<r){const m=Math.floor((l+r)/2);
      if(this.compareGroup(p,this.groupOrder[m],a)<=0)l=m+1;else r=m;}return l;
  }
  private findBody(p: P011ParticleSnapshot,radius: number): boolean {
    this.selectedA=this.selectedB=-1;this.invalid=false;this.size=0;this.rebuilds++;this.diagnostic?.increment(20);
    if(!Number.isFinite(radius)||radius<0||p.nmax!==this.groupOrder.length){this.invalid=true;return false;}
    for(let slot=0;slot<p.nmax;slot++)if(p.pState[slot]!==0){
      if(!Number.isFinite(p.pX[slot])||!Number.isFinite(p.pY[slot])||p.pId[slot]===0){this.invalid=true;return false;}
      this.groupOrder[this.size]=slot;this.idOrder[this.size]=slot;this.size++;
    }
    if(this.size<2)return false;
    this.sort(p,this.groupOrder,true);this.sort(p,this.idOrder,false);
    for(let i=1;i<this.size;i++)if(p.pId[this.idOrder[i]]===p.pId[this.idOrder[i-1]]){this.invalid=true;return false;}
    this.minimum.fill(-1);
    for(let i=0;i<this.size;i++){const slot=this.groupOrder[i];this.position[slot]=i;this.minimum[this.treeBase+i]=slot;}
    const treeSpan=this.diagnostic===null?-1:this.diagnostic.begin('INDEX_TREE_BUILD');
    for(let node=this.treeBase-1;node>0;node--){
      this.diagnostic?.cursor(node,this.treeBase,node,node*2,this.size);
      this.minimum[node]=this.smaller(p,this.minimum[node*2],this.minimum[node*2+1]);this.treeVisits++;this.diagnostic?.increment(12);}
    this.diagnostic?.endSpan(treeSpan,0);
    for(let order=0;order<this.size;order++){
      const a=this.idOrder[order];this.aSelections++;this.remove(p,a);
      const begin=this.groupBegin(p,a),end=this.groupEnd(p,a),ya=p.pY[a];
      let l=begin,r=end;while(l<r){const m=Math.floor((l+r)/2);
        if(p.pY[this.groupOrder[m]]<ya)l=m+1;else r=m;}const split=l;
      l=begin;r=split;while(l<r){const m=Math.floor((l+r)/2);
        if(Math.abs(ya-p.pY[this.groupOrder[m]])<=radius)r=m;else l=m+1;}const low=l;
      l=split;r=end;while(l<r){const m=Math.floor((l+r)/2);
        if(Math.abs(ya-p.pY[this.groupOrder[m]])<=radius)l=m+1;else r=m;}const high=l;
      const rangeSpan=this.diagnostic===null?-1:this.diagnostic.begin('INDEX_RANGE');
      this.diagnostic?.cursor(low,high,l,r,this.size);
      const b=this.rangeMin(p,low,high);this.diagnostic?.endSpan(rangeSpan,0);
      if(b>=0){this.selectedA=a;this.selectedB=b;return true;}
    }
    return false;
  }
  find(p: P011ParticleSnapshot,radius: number): boolean {
    if(this.txD1!==null){const r=this.txD1.selectBound(p,radius);this.selectedA=r.slotA;this.selectedB=r.slotB;return r.status==='PAIR';}
    const d=this.diagnostic;let span=-1;let result=false;
    if(d!==null){d.input(p);span=d.begin('INDEX_FIND');}
    try{result=this.findBody(p,radius);return result;}
    finally{d?.endSpan(span,this.invalid?2:0);}
  }

}
