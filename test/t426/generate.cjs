const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t413/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets');
const read=n=>fs.readFileSync(path.join(ets,n),'utf8').replaceAll('\r\n','\n');
const local=n=>fs.readFileSync(path.join(__dirname,n),'utf8').replace(/^\uFEFF/,'').replaceAll('\r\n','\n');
function edit(n,a,b){const s=read(n);if(s.split(a).length!==2)throw Error('T426_ANCHOR '+n+' '+a.slice(0,100));fs.writeFileSync(path.join(ets,n),s.replace(a,b));}
for(const n of ['PrepareReadiness.ets','PrepareMainWindow.ets','PrepareReadinessV4.ets'])fs.writeFileSync(path.join(ets,'render',n),local(n));
const ability='entryability/EntryAbility.ets';
edit(ability,"import window from '@ohos.window';","import window from '@ohos.window';\nimport { PrepareMainWindow } from '../render/PrepareMainWindow';");
edit(ability,'  onWindowStageCreate(windowStage: window.WindowStage): void {','  onWindowStageCreate(windowStage: window.WindowStage): void {\n    PrepareMainWindow.set(windowStage.getMainWindowSync());');
edit(ability,'  onWindowStageDestroy(): void {','  onWindowStageDestroy(): void {\n    PrepareMainWindow.set(null);');
const controller='render/P013SingleSandController.ets';
edit(controller,'  readonly capture=new T413Capture();',`  private prepareGate:(()=>string)|null=null;
  bindPrepareGateCold(check:()=>string):void {this.prepareGate=check;}
  prepareReadinessCold():PrepareNative {
    const p=this.preparation.packet,n=this.native.getState();
    return {controller:this.session,session:p.session,surface:n.surfaceEpoch,context:n.contextEpoch,
     geometry:p.geometry,width:n.widthPx,height:n.heightPx,run:p.run,revision:p.revision,
     ready:this.session>0&&p.enabled&&p.surface===n.surfaceEpoch&&p.context===n.contextEpoch&&
      p.width===n.widthPx&&p.height===n.heightPx&&this.lastReceipt?.presentationState==='CURRENT'};
  }
  readonly capture=new T413Capture();`);
edit(controller,'export function singleSandConfigDigest',`export interface PrepareNative {
 controller:number;session:number;surface:number;context:number;geometry:number;width:number;height:number;run:string;revision:number;ready:boolean;
}
export function singleSandConfigDigest`);
edit(controller,'  reserveCapture(roi:Int32Array,fixtureKey:string,maxHoldMs:number):CaptureReply {',`  reserveCapture(roi:Int32Array,fixtureKey:string,maxHoldMs:number):CaptureReply {
    const prepare=this.prepareGate===null?'PREPARE_NOT_READY':this.prepareGate();
    if(prepare)return this.capture.reply(prepare);`);
const page='pages/P051DerivedV2Demo.ets';
edit(page,"import inputMethod from '@ohos.inputMethod';",`import window from '@ohos.window';
import deviceInfo from '@ohos.deviceInfo';
import { PrepareReadiness, PrepareSnapshot, PrepareRect, PrepareApiRecord } from '../render/PrepareReadiness';
import { PrepareMainWindow } from '../render/PrepareMainWindow';`);
edit(page,'@Entry',`interface PrepareCallError {code?:number;name?:string;message?:string;runStatus?:string;}
class T426PrepareFrame extends FrameCallback {
 owner:P051DerivedV2Demo|null=null;generation:number=0;prepareId:number=0;frameNs:number=0;
 onFrame(ns:number):void {this.frameNs=ns;}
 onIdle(_remaining:number):void {this.owner?.prepareIdle(this);}
}
@Entry`);
let p=read(page),a=p.indexOf(' private async prepare():'),b=p.indexOf(' private async reserve():',a);
if(a<0||b<0)throw Error('T426_PREPARE_SPAN');
p=p.slice(0,a)+local('page.inc')+'\n'+p.slice(b);
p=p.replace("this.active=true;this.model.setActive(true);", "this.active=true;this.prepareConnect();this.model.setActive(true);");
p=p.replace("onPageHide():void{", "onPageHide():void{this.prepareDisconnect();");
p=p.replace("if(r.state==='RESERVED'){", "if(r.state==='RESERVED'){this.prepareGate.consume();this.prepareUpdate();");
p=p.replace("Button('预约').onClick", "Button('预约').enabled(this.prepareGate.state==='READY').onClick");
p=p.replace("if(this.allowed())this.configuring=true;", "if(this.allowed()){this.prepareGate.configure();this.configuring=true;this.prepareUpdate();}");
p=p.replace("Text(this.captureText).fontSize(10)","Text(this.prepareText+' · '+this.captureText).fontSize(10)");
p=p.replace("TextInput({text:this.probeRoi,", "TextInput({controller:this.roiInput,text:this.probeRoi,");
p=p.replace("TextInput({text:this.planText,", "TextInput({controller:this.planInput,text:this.planText,");
p=p.replace("TextInput({text:this.maxHoldText,", "TextInput({controller:this.limitInput,text:this.maxHoldText,");
for(const [i,name] of ['roiInput','planInput','limitInput'].entries()){
 const re=new RegExp('TextInput\\(\\{controller:this\\.'+name+'[^\\n]+');
 const line=p.match(re)?.[0];if(!line)throw Error('T426_INPUT '+name);
 const patched=line.replace(/\.onChange\(/,".defaultFocus(false).onAppear(()=>this.inputCreated("+i+")).onEditChange((v:boolean)=>this.inputEdit("+i+",v)).onFocus(()=>this.inputFocus("+i+",true)).onBlur(()=>this.inputFocus("+i+",false)).onChange(")
  .replace('if(this.allowed()){','if(this.allowed()){this.prepareChanged();').replace('if(this.allowed())this.maxHoldText=v;','if(this.allowed()){this.prepareChanged();this.maxHoldText=v;}');
 p=p.replace(line,patched);
}
p=p.replace("Row(){Button('中性')", "Row(){Button(this.fixedKeyboardConfirmed?'已确认固定键盘':'确认全屏手机固定键盘').onClick(()=>{if(this.allowed()){this.fixedKeyboardConfirmed=!this.fixedKeyboardConfirmed;this.prepareChanged();}}).fontSize(8).width(120);Button('中性')");
p=p.replace("}.width('92%').layoutWeight(1).onAreaChange", "}.id('t426-surface').width('92%').layoutWeight(1).onAreaChange");
fs.writeFileSync(path.join(ets,page),p);
fs.writeFileSync(path.join(out,'t426-manifest.json'),JSON.stringify({revision:'T426_PREPARE_V1',base:'T414-finalized',device:'NOT_RUN'},null,2));
