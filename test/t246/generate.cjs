'use strict';
// Reuse the cold isolated project copier; no edits to production/公共平台 hunks.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'.codex/local-test/t246-project');
cp.execFileSync(process.execPath,[path.join(root,'test/t248/generate.cjs'),out],{cwd:root});
const main=path.join(out,'entry/src/main'),cpp=path.join(main,'cpp/t246');fs.mkdirSync(cpp,{recursive:true});
fs.copyFileSync(path.join(root,'test/p013_compute/pair_sha256_v1.h'),path.join(cpp,'pair_sha256_v1.h'));
for(const n of ['probe.cpp','protocol.h'])fs.copyFileSync(path.join(__dirname,n),path.join(cpp,n));
fs.mkdirSync(path.join(cpp,'../platform'),{recursive:true});
for(const n of ['egl_context.cpp','egl_context.h','render_contract.h'])fs.copyFileSync(path.join(root,'entry/src/main/cpp/p013renderer',n),path.join(cpp,'../platform',n));
const sha=b=>crypto.createHash('sha256').update(b).digest('hex'),m0=fs.readFileSync(path.join(root,'docs/analysis-T233-gpu-resident-M0-engineering-manifest-v1.json')),m1=fs.readFileSync(path.join(root,'docs/analysis-T238-gpu-water-M1-engineering-manifest-v1.json'));
let profiles=JSON.parse(m0).resourceTemplates;const extra=JSON.parse(m1);
for(const m of extra.resourceTemplates){const baseline=profiles.find(p=>p.K===m.K),parts={...baseline.partsBytes};for(const [name,bytes] of Object.entries(m.deltaPartsBytes))parts[name]=(parts[name]||0)+bytes;profiles.push({K:m.K,Kw:m.Kw,partsBytes:parts,basePayloadBytes:m.totalDesignPayloadBytes});}
fs.writeFileSync(path.join(out,'t246-m1-manifest.json'),m1);
// M1 layouts retained verbatim for platform-owner follow-up; no invented allocation split.
profiles=profiles.map(p=>{const parts={...p.partsBytes};
for(const [key,delta] of [['three_state_banks','three_banks_water12_to16'],['two_publications','two_publications_water20_column16'],['two_frame_event64_arenas','two_frame_event_arena_increase']]){parts[key]+=(parts[delta]||0);delete parts[delta];}
const split={};for(const [n,b]of Object.entries(parts)){const count=n.startsWith('three_')?3:n.startsWith('two_')?2:1;if(b===0)continue;if(b%count)throw Error('RESOURCE_SPLIT');for(let i=0;i<count;i++)split[n+'_'+i]=b/count;}
if(Object.values(split).reduce((a,b)=>a+b,0)!==p.basePayloadBytes)throw Error('RESOURCE_TOTAL');return {...p,partsBytes:split};});
const hash=sha(Buffer.concat([m0,m1,fs.readFileSync(path.join(__dirname,'probe.cpp'))]));
fs.writeFileSync(path.join(cpp,'resources_generated.h'),'#pragma once\n#include <vector>\nnamespace t246 {struct Part{const char* name;uint64_t bytes;};\ninline const std::vector<std::vector<Part>> profiles={'+profiles.map(p=>'{'+Object.entries(p.partsBytes).map(([n,b])=>'{"'+n+'",'+b+'ULL}').join(',')+'}').join(',')+'};\ninline constexpr unsigned profileCount='+profiles.length+';inline constexpr const char* manifestSha="'+hash+'";}\n');
fs.writeFileSync(path.join(main,'cpp/CMakeLists.txt'),'cmake_minimum_required(VERSION 3.16)\nproject(t246)\nadd_library(t246probe SHARED t246/probe.cpp platform/egl_context.cpp)\ntarget_compile_features(t246probe PRIVATE cxx_std_17)\ntarget_link_libraries(t246probe PRIVATE libace_napi.z.so libace_ndk.z.so libEGL.so libGLESv3.so libhilog_ndk.z.so)\n');
fs.writeFileSync(path.join(main,'ets/pages/T246Probe.ets'),`import probe from 'libt246probe.so';
@Entry @Component struct T246Probe {
 @State status:string='NOT_RUN';@State visible:boolean=true;private timer:number=-1;private profile:number=1;private held:boolean=false;
 private log(value:string):void{this.status=value;console.info('T246 '+value);}
 aboutToAppear():void{this.timer=setInterval(()=>{this.log(probe.poll() as string);},100);}
 aboutToDisappear():void{clearInterval(this.timer);probe.setActive(false);}
 build(){Column(){Text('T246 private async window probe (not M0 commit)');Text(this.status).maxLines(4);
 if(this.visible){XComponent({id:'t246',type:XComponentType.SURFACE,libraryname:'t246probe'}).width('100%').height('50%');}
 Button('M0 K192').onClick(()=>this.log(probe.run(0,0) as string));Button('M0 K512').onClick(()=>this.log(probe.run(1,0) as string));
 Button('cycle M1').onClick(()=>{this.profile=2+(this.profile-1)%4;this.log(probe.run(this.profile,0) as string);});
 Button('slow real GPU fence').onClick(()=>this.log(probe.run(0,64) as string));Button('cancel inflight').onClick(()=>this.log(probe.cancel() as string));
 Button('hold/release actual slot').onClick(()=>{this.held=!this.held;this.log(probe.setLeaseHeld(this.held) as string);});
 Button('destroy/recreate XComponent').onClick(()=>{this.visible=!this.visible;});Button('lifecycle log').onClick(()=>this.log(probe.drain() as string));}.width('100%').height('100%');}
}`);
fs.writeFileSync(path.join(main,'resources/base/profile/main_pages.json'),JSON.stringify({src:['pages/T246Probe']}));
fs.writeFileSync(path.join(main,'ets/entryability/EntryAbility.ets'),`import UIAbility from '@ohos.app.ability.UIAbility';import window from '@ohos.window';import probe from 'libt246probe.so';export default class EntryAbility extends UIAbility{onWindowStageCreate(stage:window.WindowStage):void{stage.loadContent('pages/T246Probe');}onBackground():void{console.info('T246 '+probe.setActive(false));}onForeground():void{console.info('T246 '+probe.setActive(true));}}`);
// Only the dedicated probe page and no Worker requires compilation in this tiny project.
const b=JSON.parse(fs.readFileSync(path.join(out,'entry/build-profile.json5')));b.buildOption.sourceOption.workers=[];fs.writeFileSync(path.join(out,'entry/build-profile.json5'),JSON.stringify(b,null,2));
fs.writeFileSync(path.join(out,'t246-manifest.json'),JSON.stringify({revision:'T246_WINDOW_ASYNC_V2',sourceSha:hash,m0Sha:sha(m0),m1Sha:sha(m1),profiles:profiles.map(p=>({K:p.K,Kw:p.Kw,bytes:p.basePayloadBytes})),pending:['product completion256 fields and audit semantics (shape-only identity tail currently)','platform-owner production scene integration','program/driver actual allocation bytes'],productionTouched:false},null,2));console.log(out);
