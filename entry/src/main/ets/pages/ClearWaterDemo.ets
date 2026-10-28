import sensor from '@ohos.sensor';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { P051GravityConfig,P051GravityInputAdapter } from '../physics/P051GravityInputAdapter';
import { p013NowMs } from '../render/P013RenderController';
import { P013ClearWaterController } from '../render/P013ClearWaterController';
class ClearWaterFrame extends FrameCallback {owner:ClearWaterDemo|null=null;onFrame(ns:number):void{this.owner?.tick(ns);}}
@Entry
@Component
struct ClearWaterDemo {
 private readonly model:P013ClearWaterController=new P013ClearWaterController();private readonly callback:ClearWaterFrame=new ClearWaterFrame();
 private readonly gravityConfig:P051GravityConfig=new P051GravityConfig();private readonly sensorInput:P051GravityInputAdapter=new P051GravityInputAdapter(this.gravityConfig);
 private shown:boolean=false;private active:boolean=false;private queued:boolean=false;private sensorActive:boolean=false;private inputWidth:number=0;private inputHeight:number=0;
 @StorageLink('emptySandDemoForeground') @Watch('syncActive') private foreground:boolean=true;
 @State private status:string='等待新管线';@State private kind:string='沙';
 private readonly accelerometer=(data:sensor.AccelerometerResponse):void=>{if(this.active)this.sensorInput.consumeAccelerometer(data.x,data.y,data.z,p013NowMs());};
 aboutToAppear():void {this.callback.owner=this;}
 onPageShow():void {this.shown=true;this.syncActive();}
 onPageHide():void {this.shown=false;this.syncActive();}
 aboutToDisappear():void {this.onPageHide();this.callback.owner=null;this.model.release();}
 private syncActive():void {
  this.active=this.shown&&this.foreground;this.model.setActive(this.active);
  if(this.active&&!this.sensorActive){try{sensor.on(sensor.SensorId.ACCELEROMETER,this.accelerometer,{interval:this.gravityConfig.sensorIntervalNs});this.sensorActive=true;}catch(e){this.status='传感器不可用';}}
  else if(!this.active&&this.sensorActive){try{sensor.off(sensor.SensorId.ACCELEROMETER,this.accelerometer);}catch(e){}this.sensorActive=false;this.sensorInput.reset();}
  if(this.active)this.schedule();
 }
 private schedule():void {if(this.active&&!this.queued){this.queued=true;this.getUIContext().postFrameCallback(this.callback);}}
 tick(ns:number):void {this.queued=false;if(!this.active)return;const now=p013NowMs();try{this.model.tick(ns,now,this.sensorInput.latest(now),this.getUIContext().vp2px(1));this.status=this.model.status;}catch(e){this.model.source.stop('CONTROLLER_FAULT');this.status='FAULT';}this.schedule();}
 private touch(e:TouchEvent):void {
  if(e.type===TouchType.Cancel){this.model.source.stop('CANCEL');return;}
  const points=e.type===TouchType.Up?e.changedTouches:e.touches;if(points.length===0)return;const p=points[0];
  if(e.type===TouchType.Up){this.model.source.up(p.id);return;}if(this.inputWidth<=0||this.inputHeight<=0||!this.active)return;
  const x=p.x/this.inputWidth*128,y=(1-p.y/this.inputHeight)*256;
  if(e.type===TouchType.Down)this.model.source.down(p.id,x,y);else if(e.type===TouchType.Move)this.model.source.move(p.id,x,y);
 }
 build(){
  Column({space:8}) {
   Text('单沙＋明水 · C V1').fontSize(20)
   Text(this.status+' · 当前源：'+this.kind).fontSize(12)
   Text('水直接加入水层；摇动仅记录。参数待定标。').fontSize(12)
   Row({space:8}) {
    Button('沙').onClick(()=>{this.model.source.select(0);this.kind='沙';})
    Button('水').onClick(()=>{this.model.source.select(1);this.kind='水';})
    Button('重置').onClick(()=>this.model.reset())
   }
   Stack(){
    XComponent({id:'p013_clear_water_surface',type:XComponentType.TEXTURE,libraryname:'p013renderer'}).width('100%').height('100%').onLoad(()=>this.model.onLoad()).hitTestBehavior(HitTestMode.None)
    Stack(){}.width('100%').height('100%').onTouch((e:TouchEvent)=>this.touch(e))
   }.width('92%').layoutWeight(1).onAreaChange((_old:Area,area:Area)=>{const w=Number(area.width),h=Number(area.height);if(w!==this.inputWidth||h!==this.inputHeight){this.model.invalidateGeometry();this.inputWidth=w;this.inputHeight=h;}})
  }.width('100%').height('100%').padding({top:16,bottom:24})
 }
}
