import m1 from 'libgrsm1.so';
interface M1Status {ready:boolean;permissionState:string;epoch:number;readyEpoch:number;runLo:number;runHi:number;geometryLo:number;geometryHi:number;inputLo:number;inputHi:number;configLo:number;configHi:number;}
@Entry @Component
struct GpuResidentM1 {
 @State status:string='WAIT_WINDOW';@State kind:number=0;private timer:number=-1;private sequence:number=0;private identity:M1Status|null=null;private surfaceWidth:number=1;private surfaceHeight:number=1;
 onPageShow():void{m1.setActive(true);this.timer=setInterval(()=>{this.status=m1.status();const s:M1Status=JSON.parse(this.status) as M1Status;if(s.ready&&s.permissionState==='READY'&&s.epoch===s.readyEpoch){if(this.identity===null||this.identity.runLo!==s.runLo||this.identity.runHi!==s.runHi)this.sequence=0;this.identity=s;}else this.identity=null;},100);}
 onPageHide():void{this.identity=null;if(this.timer>=0)clearInterval(this.timer);this.timer=-1;m1.setActive(false);}
 aboutToDisappear():void{this.onPageHide();}
 private offer(x:number,y:number):void{const id=this.identity;if(id===null)return;const b=new ArrayBuffer(128),w=new Uint32Array(b),d=new DataView(b);w[0]=1129534023;w[1]=2;w[2]=this.kind;w[4]=id.runLo;w[5]=id.runHi;w[6]=++this.sequence;w[8]=id.inputLo;w[9]=id.inputHi;w[10]=this.sequence;w[12]=id.geometryLo;w[13]=id.geometryHi;w[14]=w[15]=1;d.setFloat64(64,Math.min(127.99,Math.max(0,x)),true);d.setFloat64(72,Math.min(256,Math.max(0,y)),true);d.setFloat64(80,.125,true);w[24]=this.sequence;w[26]=w[27]=1;w[28]=id.configLo;w[29]=id.configHi;console.info('GRS_M1_OFFER '+m1.offer(b));}
 build(){Column(){Text('GPU M1 · 新空场').fontSize(18);Text(this.status).maxLines(4).fontSize(10);
 Row(){Button('沙').onClick(()=>{this.kind=0;});Button('水').onClick(()=>{this.kind=1;});Button('发射').onClick(()=>this.offer(64.5,100));}
 XComponent({id:'grs-m1',type:XComponentType.SURFACE,libraryname:'grsm1'}).width('100%').layoutWeight(1).onAreaChange((_o:Area,a:Area)=>{this.surfaceWidth=Number(a.width);this.surfaceHeight=Number(a.height);}).onTouch((e:TouchEvent)=>{if(e.type===TouchType.Down&&e.touches.length>0)this.offer(e.touches[0].x/this.surfaceWidth*128,(1-e.touches[0].y/this.surfaceHeight)*256);});
 Slider({min:-30,max:30,value:0,step:1}).onChange((v:number)=>{m1.setAngle(v*256);});
 Row(){Button('K192/W128 新场').onClick(()=>{this.identity=null;m1.configureM1(192,128,3,2);});Button('K512/W64 新场').onClick(()=>{this.identity=null;m1.configureM1(512,64,3,2);});}
 Button('重置新空场').onClick(()=>{this.identity=null;m1.newRun();});
 }.width('100%').height('100%');}
}
