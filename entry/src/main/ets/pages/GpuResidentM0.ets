import m0 from 'libgrsm0.so';
interface M0Status {state:string;ready:boolean;runLo:number;runHi:number;geometryLo:number;geometryHi:number;inputLo:number;inputHi:number;configLo:number;configHi:number;}
@Entry @Component
struct GpuResidentM0 {
  @State status:string='WAIT_WINDOW';
  private timer:number=-1;
  private sequence:number=0;
  private identity:M0Status|null=null;
  private surfaceWidth:number=1;
  onPageShow():void {m0.setActive(true);this.timer=setInterval(()=>{
    this.status=m0.status();const s:M0Status=JSON.parse(this.status) as M0Status;
    if(s.ready===true){if(this.identity===null||this.identity.inputLo!==s.inputLo||this.identity.inputHi!==s.inputHi||this.identity.runLo!==s.runLo||this.identity.runHi!==s.runHi)this.sequence=0;this.identity=s;}else this.identity=null;
  },100);}
  onPageHide():void {this.identity=null;clearInterval(this.timer);this.timer=-1;m0.setActive(false);}
  aboutToDisappear():void {if(this.timer>=0)clearInterval(this.timer);m0.setActive(false);}
  private source(x:number):void {
    const id=this.identity;if(id===null)return;
    const b=new ArrayBuffer(128),w=new Uint32Array(b),d=new DataView(b);
    w[0]=1129534023;w[1]=1;w[4]=id.runLo;w[5]=id.runHi;w[6]=++this.sequence;w[8]=id.inputLo;w[9]=id.inputHi;
    w[10]=this.sequence;w[12]=id.geometryLo;w[13]=id.geometryHi;w[14]=1;w[15]=1;
    d.setFloat64(64,Math.min(127.99,Math.max(0,x)),true);
    d.setFloat64(72,100,true);d.setFloat64(80,0.125,true);
    w[22]=16;w[24]=this.sequence;w[26]=1;w[27]=1;w[28]=id.configLo;w[29]=id.configHi;
    console.info('GRS_SOURCE '+m0.offer(b));
  }
  build(){Column(){
    Text('GPU resident M0 / Q20 / experimental');
    Text(this.status).maxLines(4).fontSize(11);
    XComponent({id:'grs-m0',type:XComponentType.SURFACE,libraryname:'grsm0'})
      .width('100%').layoutWeight(1)
      .onAreaChange((_old:Area,area:Area)=>{this.surfaceWidth=Number(area.width);})
      .onTouch((event:TouchEvent)=>{if(event.type===TouchType.Down&&event.touches.length>0)this.source(event.touches[0].x/this.surfaceWidth*128);});
    Button('Spawn sand').onClick(()=>this.source(64.5));
    Slider({min:-30,max:30,value:0,step:1}).onChange((v:number)=>{m0.setAngle(v*256);});
    Button('New empty run').onClick(()=>{this.identity=null;m0.newRun();});
  }.width('100%').height('100%');}
}
