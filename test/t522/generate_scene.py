"""Version the established shader renderer for medium3 and V4 lease gates."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
def read(n):return (GPU/n).read_text(encoding='utf-8-sig')
def one(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
header=read('m1/scene.h').replace('#include "full_transport.h"','#include "scene_source.h"').replace('namespace grs {namespace m1 {','namespace grs {namespace m3a {').replace('FullM1Transport&','SceneSourceV4&')
header=one(header,'const std::string& PrepareFailure()const', 'bool PrepareTargets(uint32_t width,uint32_t height,uint64_t contextGeneration){return targets.Ensure(width,height,contextGeneration)==p013::Status::OK;}\n const std::string& PrepareFailure()const')
source=read('m1/scene.cpp').replace('m1_scene_sources.h','m3a_scene_sources.h').replace('namespace grs {namespace m1 {','namespace grs {namespace m3a {').replace('FullM1Transport&','SceneSourceV4&').replace('M1_SCENE_ABI_V1','M3_SCENE_ABI_V4_MEDIUM3').replace('GRS_GPU_SCENE_Q20_M1_V2','GRS_GPU_SCENE_Q20_M3_V4')
source=one(source,'const auto& header=source.Owner().Header();','const auto header=source.SceneHeaderV4();')
source=one(source,'header[1]!=2||header[31]!=3||header[32]!=0||header[122]!=1||header[124]!=1||header[125]!=1','header[1]!=4||header[31]!=15||header[32]!=0||header[27]!=3||header[122]!=1||header[124]!=2||header[125]!=2')
source=source.replace('source.Acquire(','source.AcquireV4(').replace('source.ReleasePublication(','source.ReleaseV4(')
for old,new in [('kM1FullVertex','kM3FullVertex'),('kM1SceneFragment','kM3SceneFragment'),('kM1ParticleVertex','kM3ParticleVertex'),('kM1ParticleFragment','kM3ParticleFragment'),('kM1WaterVertex','kM3WaterVertex'),('kM1WaterFragment','kM3WaterFragment'),('kM1SceneSha','kM3SceneSha')]:source=source.replace(old,new)
(GPU/'m3a/scene.h').write_text(header,encoding='utf-8')
(GPU/'m3a/scene.cpp').write_text('// Independent T522 V4 renderer. See generate_scene.py for versioned reuse.\n'+source,encoding='utf-8')
fragment=read('m1/scene.frag')
fragment=one(fragment,'uintBitsToFloat(P[uColumn+9u*128u+uint(c)]),0.)','uintBitsToFloat(P[uColumn+9u*128u+uint(c)]),uintBitsToFloat(P[uColumn+14u*128u+uint(c)]))')
fragment=one(fragment,'max(y,a.x)','max(y,a.w)')
fragment=one(fragment,'float ys=a.x/256.','float ys=a.w/256.')
(GPU/'m3a/scene.frag').write_text('// V4 free water starts above occupied material S+P.\n'+fragment,encoding='utf-8')
