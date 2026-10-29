'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'.codex/local-test/t270-v1');
cp.execFileSync(process.execPath,[path.join(root,'test/t248/generate.cjs'),out],{cwd:root,stdio:'pipe'});
const ets=path.join(out,'entry/src/main/ets'),edits=[],sha=s=>crypto.createHash('sha256').update(s).digest('hex');
function patch(rel,a,b){const p=path.join(ets,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T270_WHITELIST:'+rel+':'+a.slice(0,60));const v=s.replace(a,b);fs.writeFileSync(p,v);edits.push({rel,from:a,to:b,sha:sha(v)});}
const diag=path.join(ets,'physics/clearwater/diagnostics');fs.mkdirSync(diag,{recursive:true});fs.copyFileSync(path.join(__dirname,'ClearWaterDiagnostic.ets'),path.join(diag,'ClearWaterDiagnostic.ets'));
const source='physics/clearwater/P011ClearWaterSourceController.ets';
patch(source,"import { P011ClearWaterEngine,","import { ClearWaterDiagnostic } from './diagnostics/ClearWaterDiagnostic';\nimport { P011ClearWaterEngine,");
patch(source,'export class ClearWaterSourceController {',`export class ClearWaterSourceController {
 diagnostic:ClearWaterDiagnostic|null=null;
 queryOld(request:ClearWaterSourceRequest,out:ClearWaterReceiptRead):CQ {const q=this.engine.copySourceReceiptInto(request,out);this.diagnostic?.event(10,this.engine,request,q===CQ.TERMINAL?out.receipt:null,q,1,'OLD_KEY_READ_ONLY');return q;}`);
patch(source,"stop(reason:string,geometry:number=this.engine.geometryEpoch):S {", "stop(reason:string,geometry:number=this.engine.geometryEpoch):S {this.diagnostic?.event(2,this.engine,null,null,this.kind,geometry,reason);");
patch(source,"this.stop('KIND_SWITCH');this.kind=kind;", "this.diagnostic?.event(1,this.engine,null,null,this.kind,kind,'SELECT');this.stop('KIND_SWITCH');if(this.diagnostic!==null&&this.seq>0)this.queryOld(this.request,this.terminal);this.kind=kind;");
patch(source,'this.gesture++;this.seq=0;return true;',"this.gesture++;this.seq=0;this.diagnostic?.event(3,this.engine,null,null,this.gesture,pointer,'DOWN');return true;");
patch(source,'  const result=e.enqueueSourceInto(r,this.ack);if(result!==S.OK)return result;',"  this.diagnostic?.event(5,e,r,null,c.previous,c.current,'CLOCK_WINDOW');const result=e.enqueueSourceInto(r,this.ack);this.diagnostic?.event(6,e,r,this.ack,result,0,'ENQUEUE');if(result!==S.OK)return result;");
patch(source,"  if(e.copySourceReceiptInto(r,this.terminal)===CQ.TERMINAL){", "  this.diagnostic?.event(7,e,r,null,this.batch.processed,0,'BATCH');const query=e.copySourceReceiptInto(r,this.terminal);this.diagnostic?.event(8,e,r,query===CQ.TERMINAL?this.terminal.receipt:null,query,0,'TERMINAL_QUERY');if(query===CQ.TERMINAL){");
patch(source,'const valid=this.clock.tick(raw,unit,domain);',"const valid=this.clock.tick(raw,unit,domain);this.diagnostic?.clockEvent(this.engine,raw,this.clock.seconds,this.clock.windowId,unit,domain,this.clock.reason,valid);");
const controller='render/P013ClearWaterController.ets';
patch(controller,"import p013renderer from","import { ClearWaterDiagnostic } from '../physics/clearwater/diagnostics/ClearWaterDiagnostic';\nimport { p013NowMs } from './P013RenderController';\nimport p013renderer from");
patch(controller,'export class P013ClearWaterController {',`export class P013ClearWaterController {
 diagnostic:ClearWaterDiagnostic|null=null;
 configureDiagnostic(capacity:number,firstFrame:number):void {const d=new ClearWaterDiagnostic(capacity);d.firstFrame=firstFrame;this.diagnostic=d;this.source.diagnostic=d;}
 exportDiagnosticCold(packageSha:string):string {return this.diagnostic===null?'DISABLED':this.diagnostic.exportCold(packageSha,p013renderer.cwDiagExport());}`);
patch(controller,'this.lastReceipt=r;this.counters[7]++;',"this.lastReceipt=r;if(this.diagnostic!==null){const begin=p013NowMs();const before=this.diagnostic.frames;this.diagnostic.capture(f,slot,r);if(this.diagnostic.frames>before)this.diagnostic.frameData[(this.diagnostic.frames-1)*16+14]=p013NowMs()-begin;}this.counters[7]++;");
const page='pages/ClearWaterDemo.ets';
patch(page,'aboutToAppear():void {this.callback.owner=this;}',`aboutToAppear():void {this.callback.owner=this;if(AppStorage.get<boolean>('cwdiag')===true)this.model.configureDiagnostic(256,AppStorage.get<number>('cwdiagFrame')??1);}`);
patch(page,"this.model.onLoad()).hitTestBehavior", "{this.model.onLoad();if(AppStorage.get<boolean>('cwdiag')===true)cwNative.cwDiagArm(AppStorage.get<number>('cwdiagFrame')??1,3,AppStorage.get<number>('cwdiagX')??64,AppStorage.get<number>('cwdiagY')??64,4,4);}).hitTestBehavior");
patch(page,"import sensor from", "import cwNative from 'libp013renderer.so';\nimport sensor from");
patch(page,"    Button('重置').onClick(()=>this.model.reset())",`    Button('重置').onClick(()=>this.model.reset())
    Button('导出诊断').onClick(()=>{const sha=AppStorage.get<string>('cwdiagSha')??'';try{const raw=this.model.exportDiagnosticCold(sha);let part=0;for(let i=0;i<raw.length;i+=1500)console.info('CW_C_DIAG_EXPORT '+part+++' '+raw.slice(i,i+1500));console.info('CW_C_DIAG_EXPORT_END '+raw.length);}catch(e){this.status='DIAG_EXPORT_REQUIRES_PACKAGE_SHA';}})`);
// Sole cold route is legal and explicit; no Canvas in whitelist/fallback.
{const p=path.join(ets,'entryability/EntryAbility.ets'),s=fs.readFileSync(p,'utf8'),m=[...s.matchAll(/const probePage: string =[\s\S]*?;/g)];if(m.length!==1)throw Error('T270_ABILITY_VERSION');fs.writeFileSync(p,s.replace(m[0][0],"const probePage: string = 'pages/ClearWaterDemo';").replace('    this.applyWant(want.parameters, false);',`    AppStorage.setOrCreate('cwdiag',want.parameters?.cwdiag===true||String(want.parameters?.cwdiag)==='true');
    AppStorage.setOrCreate('cwdiagFrame',Number(want.parameters?.cwdiagFrame??1));AppStorage.setOrCreate('cwdiagX',Number(want.parameters?.cwdiagX??64));AppStorage.setOrCreate('cwdiagY',Number(want.parameters?.cwdiagY??64));AppStorage.setOrCreate('cwdiagSha',String(want.parameters?.cwdiagSha??''));
    this.applyWant(want.parameters, false);`));}
fs.writeFileSync(path.join(out,'entry/src/main/resources/base/profile/main_pages.json'),JSON.stringify({src:['pages/ClearWaterDemo']}));
const cpp=path.join(out,'entry/src/main/cpp/p013renderer');fs.copyFileSync(path.join(__dirname,'native_diag.h'),path.join(cpp,'cw_diag.h'));
function native(name,a,b){const p=path.join(cpp,name),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T270_NATIVE_VERSION:'+name+':'+a.slice(0,45));const v=s.replace(a,b);fs.writeFileSync(p,v);edits.push({rel:'cpp/'+name,from:a,to:b,sha:sha(v)});}
native('clear_water_gpu.cpp','#include "clear_water_gpu.h"','#include "clear_water_gpu.h"\n#include "cw_diag.h"');
native('clear_water_gpu.h',' private:EglContext& egl;', ' private:Status LayerBody(int,const ClearWaterPacket&);EglContext& egl;');
native('clear_water_gpu.cpp','Status ClearWaterGlGpu::Layer(int layer,const ClearWaterPacket& p){','Status ClearWaterGlGpu::Layer(int layer,const ClearWaterPacket& p){auto code=LayerBody(layer,p);if(code==Status::OK)cwdiag::sample(layer,p,targets.fbos[targets.candidate]);return code;}\nStatus ClearWaterGlGpu::LayerBody(int layer,const ClearWaterPacket& p){');
native('napi_bridge.cpp','#include "napi_bridge.h"','#include "napi_bridge.h"\n#include "cw_diag.h"');
native('napi_bridge.cpp','  napi_property_descriptor d[] = {',`  napi_property_descriptor d[] = {
 {"cwDiagExport",nullptr,[](napi_env e,napi_callback_info)->napi_value{auto s=cwdiag::exportCold();napi_value v;napi_create_string_utf8(e,s.c_str(),s.size(),&v);return v;},nullptr,nullptr,nullptr,napi_default,nullptr},
 {"cwDiagArm",nullptr,[](napi_env e,napi_callback_info i)->napi_value{size_t n=6;napi_value a[6];double v[6]{};napi_get_cb_info(e,i,&n,a,nullptr,nullptr);bool ok=n==6;for(size_t j=0;j<n&&j<6;j++)ok=ok&&napi_get_value_double(e,a[j],&v[j])==napi_ok&&std::isfinite(v[j])&&std::floor(v[j])==v[j]&&v[j]>=0&&v[j]<=2147483647;ok=ok&&cwdiag::arm(uint64_t(v[0]),int(v[1]),int(v[2]),int(v[3]),int(v[4]),int(v[5]));napi_value r;napi_get_boolean(e,ok,&r);return r;},nullptr,nullptr,nullptr,napi_default,nullptr},`);
fs.writeFileSync(path.join(out,'t270-manifest.json'),JSON.stringify({revision:'CW_C_DIAG_V1',diagnosticDefault:false,sourceSha:sha(fs.readFileSync(path.join(__dirname,'ClearWaterDiagnostic.ets'))),nativeSha:sha(fs.readFileSync(path.join(__dirname,'native_diag.h'))),edits,productionTouched:false,rows:256,framesMax:3,roiMax:[16,16],pending:['device same-token ROI execution','screen capture and visible-water judgement']},null,2));console.log(out);
