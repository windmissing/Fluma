const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t426/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets');
const local=n=>fs.readFileSync(path.join(__dirname,n),'utf8').replaceAll('\r\n','\n');
function edit(name,fn){const p=path.join(ets,name);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T455_ANCHOR '+a);return s.replace(a,b);}
fs.writeFileSync(path.join(ets,'render/PrepareFrameObservation.ets'),local('PrepareFrameObservation.ets'));
edit('render/PrepareReadiness.ets',s=>{
 s=one(s,'export class PrepareSnapshot {','export class PrepareSnapshot {\n layoutGeneration:number=0;');
 return one(s,' elapsed(now:number):boolean {',' startedMonoMs():number{return this.started;}\n elapsed(now:number):boolean {');
});
edit('pages/P051DerivedV2Demo.ets',s=>{
 s=one(s,"import { FrameCallback } from '@ohos.arkui.UIContext';",`import { FrameCallback, UIContext, UIObserver } from '@ohos.arkui.UIContext';
import { PrepareFrameObservation } from '../render/PrepareFrameObservation';`);
 const frame=s.indexOf('class T426PrepareFrame'),entry=s.indexOf('@Entry',frame);
 if(frame<0||entry<0)throw Error('T455_FRAME_SPAN');
 s=s.slice(0,frame)+`class T426PrepareFrame extends FrameCallback {
 owner:P051DerivedV2Demo|null=null;observation:PrepareFrameObservation|null=null;
 onFrame(ns:number):void {this.owner?.prepareFrameEnter(this,ns);}
}
`+s.slice(entry);
 const post=s.indexOf(' private postPrepareFrame()'),observe=s.indexOf(' private observePrepare()',post);
 if(post<0||observe<0)throw Error('T455_PUMP_SPAN');
 s=s.slice(0,post)+local('pump.inc')+s.slice(observe);
 s=one(s,' private prepareUpdate():void {',' private prepareUpdate():void {\n  if(this.prepareGate.state!==\'PREPARING\')this.stopPrepareObservation();');
 s=one(s,'const text=this.prepareGate.report(),id=',`const text='{\"revision\":\"T449_FRAME_OBSERVATION_V2\",\"observation\":'+JSON.stringify(this.prepareObservation)+',\"readiness\":'+this.prepareGate.report()+'}',id=`);
 s=one(s,"'t426-prepare-'", "'t455-prepare-'");
 s=one(s,'this.prepareGate.show();this.configuring=true;','this.stopPrepareObservation();this.prepareObservation=null;this.prepareGate.show();this.configuring=true;');
 s=one(s,'   this.observePrepare();this.postPrepareFrame();','   this.observePrepare();');
 s=one(s,'  this.prepareGate.hide();this.pendingPrepareId=0;', '  this.prepareGate.hide();this.stopPrepareObservation();this.pendingPrepareId=0;');
 for(const reason of ['KEYBOARD_AVOID_CHANGED','KEYBOARD_HEIGHT_CHANGED','WINDOW_CONFIGURATION_CHANGED']){
  const method=reason==='WINDOW_CONFIGURATION_CHANGED'?'invalidate':'changed';
  s=one(s,`this.prepareGate.${method}('${reason}');`, `this.prepareLayoutChanged();this.prepareGate.${method}('${reason}');`);
 }
 s=one(s,'    const snapshot=this.readPrepareSnapshot();\n    const reason=',`    const o=this.prepareObservation;
    if(this.prepareGate.state==='READY'&&(o===null||this.configuring||this.prepareBranch!==o.branch||
       !o.matchesLayout(this.prepareAnchor(this.getUIContext(),o.branch)))){
     this.prepareGate.invalidate('PREPARE_STALE_LAYOUT');this.prepareUpdate();return 'PREPARE_STALE_LAYOUT';
    }
    const snapshot=this.readPrepareSnapshot();
    const reason=`);
 s=one(s,'  s.windowId=props.id;', '  s.layoutGeneration=this.prepareObservation?.layoutGeneration??0;\n  s.windowId=props.id;');
 s=one(s,'   this.probeFixtureKey=\'\';this.schedule();','   this.prepareLayoutChanged();this.probeFixtureKey=\'\';this.schedule();');
 s=one(s,'  this.fullyVisible=full;','  this.fullyVisible=full;this.prepareLayoutChanged();');
 s=one(s,'  this.configuring=false;this.prepareUpdate();this.schedule();this.postPrepareFrame();',`  if(this.prepareGate.state!=='PREPARING'||!this.startPrepareObservation(id,generation)){
   this.prepareGate.fail('PREPARE_OBSERVER_UNAVAILABLE');this.prepareUpdate();return;
  }
  this.configuring=false;this.prepareUpdate();this.schedule();this.postPrepareFrame();`);
 s=one(s,'   this.prepareGate.elapsed(p013NowMs());this.prepareUpdate();',
   '   const o=this.prepareObservation;if(o!==null)this.prepareFrameDeadline(o,p013NowMs());');
 s=one(s,"Text(this.prepareText+' · '+this.captureText)","Text(this.prepareText+' · '+this.captureText+(this.preparePaint?' ·':'  '))");
 s=one(s,"      Button('预约').enabled", "      Text('').width(2).height(2).id('t449-action-'+this.prepareBranch)\n      Button('预约').enabled");
 return s;
});
fs.writeFileSync(path.join(out,'t455-manifest.json'),JSON.stringify({revision:'T449_FRAME_OBSERVATION_V2',base:'8bd7c1e',device:'NOT_RUN'},null,2));
