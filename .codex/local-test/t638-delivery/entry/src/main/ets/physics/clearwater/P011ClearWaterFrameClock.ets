// Raw difference precedes conversion. No clamp, catch-up or stale backlog.
export class ClearWaterFrameClock {
  previous:number=0;current:number=0;seconds:number=0;windowId:number=0;unit:string='';domain:string='';reason:string='FIRST_TICK';
  private ready:boolean=false;
  reset():void {this.ready=false;this.seconds=0;this.reason='PAUSED';}
  tick(raw:number,unit:string,domain:string):boolean {
    this.seconds=0;
    if(!Number.isFinite(raw)||raw<0||(unit!=='NS'&&unit!=='MS')||domain.length===0){this.reset();this.reason='INVALID_CLOCK';return false;}
    const old=this.current,compatible=this.ready&&unit===this.unit&&domain===this.domain;
    this.previous=old;this.current=raw;this.unit=unit;this.domain=domain;this.ready=true;
    if(!compatible){this.reason='FIRST_OR_CHANGED_CLOCK';return false;}
    const delta=raw-old,seconds=delta/(unit==='NS'?1e9:1000);
    if(delta<=0||seconds>1/30){this.ready=false;this.reason=delta<=0?'BACKWARD_CLOCK':'GAP';return false;}
    this.seconds=seconds;this.windowId++;this.reason='ACTIVE';return true;
  }
}
