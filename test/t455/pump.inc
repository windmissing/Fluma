 private prepareContext:UIContext|null=null;
 private prepareObserver:UIObserver|null=null;
 private prepareLayoutListener:(()=>void)|null=null;
 private prepareDrawListener:(()=>void)|null=null;
 private prepareObservation:PrepareFrameObservation|null=null;
 @State private prepareBranch:number=0;
 @State private preparePaint:boolean=false;

 private stopPrepareObservation():void {
  if(this.prepareTimer!==-1){clearTimeout(this.prepareTimer);this.prepareTimer=-1;}
  if(this.prepareFrame!==null){this.prepareFrame.owner=null;this.prepareFrame=null;}
  const observer=this.prepareObserver,layout=this.prepareLayoutListener,draw=this.prepareDrawListener;
  this.prepareObserver=null;this.prepareContext=null;this.prepareLayoutListener=null;this.prepareDrawListener=null;
  if(observer!==null){
   try{if(layout!==null)observer.off('didLayout',layout);}catch(e){console.error('T455 didLayout off '+String(e));}
   try{if(draw!==null)observer.off('willDraw',draw);}catch(e){console.error('T455 willDraw off '+String(e));}
  }
 }

 private prepareOwner(o:PrepareFrameObservation):boolean {
  return this.active&&this.prepareGate.state==='PREPARING'&&this.prepareObservation===o&&
   this.prepareGate.currentId(o.prepareId,o.pageGeneration)&&this.prepareBranch===o.branch&&
   this.prepareWindow!==null&&this.prepareWindow===PrepareMainWindow.value&&
   PrepareMainWindow.generation===o.windowGeneration;
 }

 private prepareAnchor(context:UIContext,branch:number):string {
  try {
   const rect=context.getComponentUtils().getRectangleById('t449-action-'+branch);
   if(rect.size.width<=0||rect.size.height<=0)return '';
   return JSON.stringify([rect.size.width,rect.size.height,rect.windowOffset.x,rect.windowOffset.y]);
  }catch(e){return '';}
 }

 private prepareLayoutChanged():void {
  if(this.prepareGate.state==='PREPARING')this.prepareObservation?.layoutChanged();
  if(this.prepareGate.state==='READY')this.prepareGate.changed('PREPARE_LAYOUT_CHANGED');
 }

 private startPrepareObservation(id:number,generation:number):boolean {
  this.stopPrepareObservation();
  const w=this.prepareWindow;
  if(w===null)return false;
  const branch=this.prepareBranch+1;
  const context=this.getUIContext();
  const o=new PrepareFrameObservation(id,generation,w.getWindowProperties().id,PrepareMainWindow.generation,branch);
  this.prepareObservation=o;
  this.prepareContext=context;
  try{
   const observer=context.getUIObserver();
   this.prepareObserver=observer;
   this.prepareLayoutListener=():void=>{
    if(!this.prepareOwner(o)||this.configuring){o.drop(p013NowMs(),0,'STALE_LAYOUT_OWNER');return;}
    o.didLayout(p013NowMs(),this.prepareAnchor(context,branch));
   };
   this.prepareDrawListener=():void=>{
    if(!this.prepareOwner(o)||this.configuring){o.drop(p013NowMs(),0,'STALE_DRAW_OWNER');return;}
    const now=p013NowMs();
    if(!this.prepareFrameDeadline(o,now))return;
    const ns=o.draw(now,this.prepareAnchor(context,branch));
    if(ns===0)return;
    try{
     const snapshot=this.readPrepareSnapshot();
     if(snapshot.windowId!==o.windowId||snapshot.windowGeneration!==o.windowGeneration){
      this.prepareGate.invalidate('PREPARE_STALE');this.prepareUpdate();return;
     }
     o.sample(now,ns);
     this.prepareGate.observe(snapshot);
     this.prepareGate.frame(ns,now);
    }catch(e){this.prepareGate.fail('PREPARE_OBSERVATION_FAILED');}
    this.prepareUpdate();
   };
   observer.on('didLayout',this.prepareLayoutListener);
   observer.on('willDraw',this.prepareDrawListener);
  }catch(e){
   this.prepareApiFailure('UIObserver.subscribe',e,id,p013NowMs(),generation);
   this.stopPrepareObservation();return false;
  }
  this.prepareBranch=branch;
  return true;
 }

 private postPrepareFrame():void {
  const o=this.prepareObservation,context=this.prepareContext;
  if(o===null||context===null||!this.prepareOwner(o)||this.prepareFrame!==null)return;
  const callback=new T426PrepareFrame();callback.owner=this;callback.observation=o;this.prepareFrame=callback;
  try {context.postFrameCallback(callback);o.post(p013NowMs());}
  catch(e){this.prepareGate.fail('PREPARE_POST_FAILED');this.prepareUpdate();}
 }

 prepareFrameEnter(callback:T426PrepareFrame,ns:number):void {
  const o=callback.observation;
  if(o===null)return;
  if(this.prepareFrame!==callback){o.drop(p013NowMs(),ns,'STALE_FRAME_SLOT');return;}
  if(!this.prepareOwner(o)){o.drop(p013NowMs(),ns,'STALE_FRAME_OWNER');this.prepareFrameDeadline(o,p013NowMs());return;}
  this.prepareFrame=null;callback.owner=null;
  const now=p013NowMs();
  if(!this.prepareFrameDeadline(o,now))return;
  o.frame(now,ns);
  // A fixed-size status line invalidation makes the static page request draw.
  // The ticket still requires an independently delivered willDraw callback.
  this.preparePaint=!this.preparePaint;
  this.postPrepareFrame();
 }

 private prepareFrameDeadline(o:PrepareFrameObservation,now:number):boolean {
  if(!this.prepareOwner(o)){
   if(this.prepareObservation===o&&this.prepareGate.currentId(o.prepareId,o.pageGeneration)&&
      this.prepareGate.state==='PREPARING'){this.prepareGate.invalidate('PREPARE_STALE');this.prepareUpdate();}
   return false;
  }
  if(now-this.prepareGate.startedMonoMs()>=3000){
   this.prepareGate.reason=o.timeoutReason(this.prepareGate.reason==='WAIT_OBSERVATIONS'?'':this.prepareGate.reason);
  }
  if(this.prepareGate.elapsed(now))return true;
  this.prepareUpdate();return false;
 }
