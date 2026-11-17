 @State private pixelMode:boolean=false;
 @State private pixelMessage:string='DIAGNOSTIC_NOT_PHYSICS';
 private inputLayout537:number=0;private inputWidth537:number=0;private inputHeight537:number=0;
 private geometry537(area:Area):void {
  this.inputLayout537++;this.inputWidth537=Number(area.width);this.inputHeight537=Number(area.height);
  if(this.pixelMode){m1.pixelDiagnosticStop();this.pixelMode=false;this.pixelMessage='STALE_GEOMETRY_NEW_SCOPE_REQUIRED';}
  this.model.stop('GEOMETRY_OBSERVED_NEEDS_DOWN');this.model.geometry(this.inputWidth537,this.inputHeight537);
 }
 private beginPixelDiagnostic537():void {
  this.model.stop('DIAGNOSTIC_SWITCH_NEEDS_NEW_DOWN');
  try {
   const geometry:string=JSON.stringify({revision:'T537_RAW_NODE_GEOMETRY_V1',layout:this.inputLayout537,
    coordinateUnit:'COMPONENT_INFO_PHYSICAL_PX',renderNode:'T537_RENDER_ID',touchNode:'t537-touch',
    render:componentUtils.getRectangleById('T537_RENDER_ID'),touch:componentUtils.getRectangleById('t537-touch')});
   this.pixelMessage=m1.pixelDiagnosticStart(geometry);this.pixelMode=this.pixelMessage==='DIAGNOSTIC_NEW_SCOPE_REQUESTED';
  }catch(e){this.pixelMessage='DIAGNOSTIC_GEOMETRY_UNAVAILABLE';this.pixelMode=false;}
 }
 pixelFrame537(ns:number):boolean {
  if(!this.pixelMode)return false;
  this.pixelMessage=m1.pixelDiagnosticPulse(ns);
  this.PIXEL_REQUEUE_537
  return true;
 }
 private async exportPixel537():Promise<void> {
  try {
   const context=getContext(this) as common.UIAbilityContext;
   const files=new DiagnosticColdFiles(context.filesDir);
   const raw:string=m1.pixelDiagnosticSnapshot();
   const out=await files.save('t537-pixel-'+Date.now()+'.json',raw);
   this.pixelMessage='DIAGNOSTIC_NOT_PHYSICS export '+JSON.stringify(out);
  }catch(e){this.pixelMessage='INCOMPLETE_PIXEL_EXPORT';}
 }
