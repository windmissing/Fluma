from pathlib import Path
import sys,json,shutil,re
ROOT=Path(__file__).resolve().parents[2]
def one(s,a,b):
 if s.count(a)!=1:raise ValueError('T538_ANCHOR '+a[:100])
 return s.replace(a,b)
def overlay(out,route):
 main=out/'entry/src/main';cpp=main/'cpp';base=cpp/'gpu_resident'
 shutil.copytree(ROOT/'entry/src/main/cpp/gpu_resident/resource_lab',base/'resource_lab',dirs_exist_ok=True)
 shutil.copyfile(ROOT/'test/t538/lab_bridge.cpp',cpp/'t538_lab_bridge.cpp')
 for rel in ['resource_ledger.h','program_cache/artifact_store.h']:
  p=base/rel;s=p.read_text(encoding='utf8');inc='resource_lab/bounded_stream.h' if '/' not in rel else '../resource_lab/bounded_stream.h';s='#include "'+inc+'"\n'+s;s=s.replace('std::ostringstream o;','resource538::Stream o;');p.write_text(s,encoding='utf8')
 p=base/'app_bridge.cpp';s=p.read_text(encoding='utf8');s='#include "resource_lab/bounded_stream.h"\n'+s
 s=one(s,'auto snapshot=a.resources->Copy();std::ostringstream out;','auto snapshot=a.resources->Copy();resource538::Stream out;')
 # Preserve existing keys and semantics while adding actual two-slot ownership.
 a=s.index('out<<"}";return text(e,out.str());}',s.index('napi_value resourceState('))
 s=s[:a]+'''{std::lock_guard<std::mutex> lock(a.mutex);out<<",\\\"trace\\\":{\\\"capacityBytes\\\":"<<a.coldTrace.Capacity()<<",\\\"reservedBytes\\\":"<<a.coldTrace.Reserved()<<",\\\"reader\\\":"<<(a.coldTrace.reading?"true":"false")<<",\\\"builder\\\":"<<a.coldTrace.builder<<",\\\"slots\\\":[";for(unsigned i=0;i<2;++i){if(i)out<<',';const auto& t=a.coldTrace.slots[i];out<<"{\\\"state\\\":"<<t.state<<",\\\"capacityBytes\\\":"<<t.capacity*4<<",\\\"reservedBytes\\\":"<<t.reserved*4<<",\\\"reader\\\":"<<(t.reader?"true":"false")<<'}';}out<<"]}";}
 '''+s[a:];s=one(s,'napi_value init(napi_env e,napi_value out)', '\n#ifdef GRS_APP_M1\n'+(ROOT/'test/t538/sample_napi.inc').read_text()+'\n#endif\nnapi_value init(napi_env e,napi_value out)')
 s=one(s,'{"resourceState",nullptr,resourceState', '{"resourceSampleBeginV1",nullptr,resourceSampleBeginV1,nullptr,nullptr,nullptr,napi_default,nullptr},\n{"resourceSampleEndV1",nullptr,resourceSampleEndV1,nullptr,nullptr,nullptr,napi_default,nullptr},\n{"resourceState",nullptr,resourceState');p.write_text(s,encoding='utf8')
 p=cpp/'CMakeLists.txt';s=p.read_text(encoding='utf8');s+='''
# T531_RESOURCE_LAB_V1: isolated native module; production grsm1 has no faults.
add_library(grs_resource_lab SHARED t538_lab_bridge.cpp)
target_include_directories(grs_resource_lab PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/gpu_resident")
target_compile_features(grs_resource_lab PRIVATE cxx_std_17)
target_compile_options(grs_resource_lab PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
target_compile_definitions(grs_resource_lab PRIVATE GRS_PROGRAM_CACHE_TEST_FAULTS T531_RESOURCE_LAB_V1)
target_link_libraries(grs_resource_lab PRIVATE grs_foundation libace_napi.z.so libace_ndk.z.so libEGL.so libGLESv3.so)
''';p.write_text(s,encoding='utf8')
 types=cpp/'types/libgrs_resource_lab';types.mkdir(parents=True,exist_ok=True)
 (types/'oh-package.json5').write_text(json.dumps({'name':'libgrs_resource_lab.so','types':'./index.d.ts','version':'1.0.0'}))
 (types/'index.d.ts').write_text('export const resourceLabStartV1:(caseId:string,filesDir:string)=>string;\n'+''.join('export const '+n+':(jobId:string)=>string;\n' for n in ['resourceLabPollV1','resourceLabReleaseBarrierV1','resourceLabCancelV1','resourceLabExportV1'])+'export const resourceLabFilesBeginV2:(jobId:string)=>string;\nexport const resourceLabFilesIndexV2:(exportId:string,page:number)=>string;\nexport const resourceLabFilePageV2:(exportId:string,fileId:string,offset:string)=>string;\nexport const resourceLabFilesEndV2:(exportId:string)=>string;\n')
 p=out/'entry/oh-package.json5';s=json.loads(p.read_text(encoding='utf8'));s.setdefault('dependencies',{})['libgrs_resource_lab.so']='file:./src/main/cpp/types/libgrs_resource_lab';p.write_text(json.dumps(s,indent=2))
 for p in (cpp/'types').rglob('index.d.ts'):
  if p.parent.name=='libgrsm1':p.write_text(p.read_text()+'\nexport const resourceSampleBeginV1:(label:string,expectedScope:string)=>string;\nexport const resourceSampleEndV1:(sampleId:string)=>string;\n')
 for name in ['GpuResourceSnapshotCold.ets','GpuResourceLabPort.ets']:shutil.copyfile(ROOT/'entry/src/main/ets/physics'/name,main/'ets/physics'/name)
 shutil.copyfile(ROOT/'test/t538/GpuResourceLab.ets',main/'ets/pages/GpuResourceLab.ets')
 p=main/'resources/base/profile/main_pages.json';doc=json.loads(p.read_text());doc['src'].append('pages/GpuResourceLab');p.write_text(json.dumps(doc,indent=2))
 page=main/'ets/pages'/('GpuResidentM1.ets' if route=='m1' else 'GpuSandWaterExperience.ets');s=page.read_text(encoding='utf8')
 s="import router from '@ohos.router';\nimport { resourceLab538 as lab538 } from '../physics/GpuResourceLabPort';\nimport { GpuResourceSnapshotCold, ResourcePort538 } from '../physics/GpuResourceSnapshotCold';\n"+s
 start=s.index('{',s.index('struct '+page.stem))
 methods=(ROOT/'test/t538/page_methods.inc').read_text(encoding='utf8');s=s[:start+1]+'\n'+methods+s[start+1:]
 anchor="Row({space:4}){Button('诊断三色图')";idx=s.index(anchor)
 buttons="""Row({space:3}){Button('资源/缓存快照').onClick(()=>this.resourceExport538());Button('独立资源Lab').onClick(()=>{router.pushUrl({url:'pages/GpuResourceLab'});});Button('内联Lab').onClick(()=>{this.labOpen538=!this.labOpen538;});}
   Text(this.resourceStatus538).fontSize(9).height(28).maxLines(2)
   if(this.labOpen538){Select([{value:'TRACE_MAX_SOURCE'},{value:'TRACE_MAX_Q'},{value:'TWO_SLOT_PIN_REPLACE'},{value:'TTL_WALL'},{value:'TTL_EDGE_INJECTED'},{value:'DIAGNOSTIC_CREDIT'},{value:'DIAGNOSTIC_BUSY_FAIL'},{value:'CACHE_FAULT'}]).selected(0).onSelect((_index:number,value:string)=>{this.labCase538=value;});Row(){Button('启动合成Lab').onClick(()=>this.labStart538());Button('查询').onClick(()=>this.labPoll538());Button('释放屏障').onClick(()=>{lab538.resourceLabReleaseBarrierV1(this.labJob538);this.labPoll538();});Button('取消').onClick(()=>{lab538.resourceLabCancelV1(this.labJob538);this.labPoll538();});Button('导出Lab').onClick(()=>this.labExport538());}}
   """
 s=s[:idx]+buttons+s[idx:];page.write_text(s,encoding='utf8')
 (out/'t538-manifest.json').write_text(json.dumps({'revision':'T531_RESOURCE_LAB_V1','route':route,'tests':'NOT_RUN_BY_USER_DIRECTION','device':'NOT_RUN','labNativeModule':'libgrs_resource_lab.so'},indent=2))
if __name__=='__main__':overlay(Path(sys.argv[1]),sys.argv[2])
