 build(){
  Column({space:0}) {
   // Geometry is fixed before reserve. Configuration editor is a separate view.
   Column({space:2}) {
    Text('单沙显式冻结截图 V2').fontSize(16).height(22)
    Text(this.captureText).fontSize(10).height(18).maxLines(1).id('t413-status-text')
    Text(this.probeStatus).fontSize(10).height(18).maxLines(1)
    if(this.configuring){
     Row(){
      Button('标定').onClick(()=>{if(!this.allowed())return;this.t394Arm='';this.t394WaitB=false;this.t394Phase('CALIBRATION',true);this.singleSand.setT406Calibration(true);}).id('t413-calibration')
      Button('厚床').onClick(()=>this.t394Start('STATIC')).id('t413-static')
      Button('对照A').onClick(()=>this.t394Start('CONTROL')).id('t413-control')
      Button('锁定A').onClick(()=>this.t394Start('LOCK')).id('t413-lock')
      Button('注入B').onClick(()=>this.t394Next()).id('t413-next')
     }.height(30)
     Row(){Button('中性').onClick(()=>this.t394SetOptics(true));Button('纹理').onClick(()=>this.t394SetOptics(false))}.height(28)
     TextInput({text:this.probeRoi,placeholder:'三组 x,y,w,h'}).onChange((v:string)=>{if(this.allowed()){this.probeRoi=v;this.probeFixtureKey='';}}).height(30).id('t413-roi')
     TextInput({text:this.planText,placeholder:'calibrationId|planId|三组ROI'}).onChange((v:string)=>{if(this.allowed()){this.planText=v;this.probeStatus=this.singleSand.setT406Plan(v);}}).height(30).id('t413-plan')
     Row(){TextInput({text:this.maxHoldText,placeholder:'maxHoldMs'}).onChange((v:string)=>{if(this.allowed())this.maxHoldText=v;}).width(110).height(28)
      Button('收键盘并准备').onClick(()=>this.prepare()).id('t413-prepare')}.height(30)
    }else{
     Row(){
      Button('预约').onClick(()=>this.reserve()).id('t413-reserve')
      Button('归档').onClick(()=>this.exportWindow()).id('t413-export')
      Button('时钟').onClick(()=>this.clockCold()).id('t413-clock')
      Button('状态').onClick(()=>this.statusCold()).id('t413-status')
     }.height(36)
     Row(){
      Button('封存').onClick(()=>this.terminalCold(true)).id('t413-seal')
      Button('取消').onClick(()=>this.cancel()).id('t413-cancel')
      Button('释放').onClick(()=>this.release()).id('t413-release')
      Button('配置').onClick(()=>{if(this.allowed())this.configuring=true;else this.probeStatus='HOLD_ACTIVE';}).id('t413-configure')
     }.height(36)
     Text('HOLD_OPEN 后保持画面；截图取回后才封存').fontSize(10).height(32).maxLines(2)
     Text(JSON.stringify(this.layout)).fontSize(8).height(46).maxLines(3)
    }
   }.height(236).width('100%').clip(true)
   Stack(){
    XComponent({id:'p013_lmat_surface',type:XComponentType.TEXTURE,libraryname:'p013renderer'})
     .width('100%').height('100%').onLoad(()=>this.renderer.onLoad()).hitTestBehavior(HitTestMode.None)
    Stack(){}.width('100%').height('100%').onTouch((e:TouchEvent)=>this.touch(e))
   }.width('92%').layoutWeight(1).onAreaChange((_old:Area,area:Area)=>this.area(area))
     .onVisibleAreaChange([1],(visible:boolean,ratio:number)=>this.captureVisibility(visible,ratio))
  }.width('100%').height('100%').padding({top:8,bottom:8})
 }
