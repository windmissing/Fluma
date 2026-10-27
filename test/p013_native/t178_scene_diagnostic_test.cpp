#define main t56_original_main
#include "t56_scene_core_test.cpp"
#undef main
int main(){
 CHECK(t56_original_main()==0);
 Gpu resource;MaterialUploader material(resource);DrawGpu gpu;SceneCompositor scene;
 auto p=Packet();auto c=Config(0,1,1);CHECK(material.BindConfiguration(1,1,c.digest).code==0);CHECK(scene.Register(c));
 auto r=scene.Reject(p,100,"SCENE_MATERIAL_DATA",&gpu,true);
 CHECK(r.compatibilityFailureMask&1);CHECK(!r.replayAttempted&&r.presentOldStatus==-1&&r.swapOldStatus==-1);
 r=scene.Draw(p,material,gpu);CHECK(r.current);
 p.frame.frameId++;r=scene.Reject(p,100,"SCENE_MATERIAL_DATA",&gpu,true);
 CHECK(r.compatibilityFailureMask==0&&r.replayAttempted&&r.presentOldStatus==0&&r.swapOldStatus==0&&r.state=="STALE");
 CHECK(r.originalCode==100&&r.originalStage=="SCENE_MATERIAL_DATA"&&r.code==100&&!r.current);
 gpu.fail=7;r=scene.Reject(p,100,"SCENE_MATERIAL_DATA",&gpu,true);
 CHECK(r.replayAttempted&&r.presentOldStatus==int(Status::GL_DRAW_FAILED)&&r.swapOldStatus==-1&&r.stage=="SCENE_MATERIAL_DATA");
 gpu.fail=-1;gpu.swapFail=true;r=scene.Reject(p,100,"SCENE_MATERIAL_DATA",&gpu,true);
 CHECK(r.presentOldStatus==0&&r.swapOldStatus==int(Status::SWAP_FAILED)&&!r.staleSwap&&r.originalCode==100);
 gpu.swapFail=false;p.frame.geometryEpoch++;r=scene.Reject(p,100,"SCENE_MATERIAL_DATA",&gpu,true);
 CHECK(r.compatibilityFailureMask==16&&!r.replayAttempted);
 p.frame.geometryEpoch--;p.run="different";r=scene.Reject(p,100,"SCENE_MATERIAL_DATA",&gpu,true);CHECK(r.compatibilityFailureMask==2&&!r.replayAttempted);
 std::puts("PASS T178 real SceneCompositor rejection: compatibility bits, raw present/swap statuses, NOT_RUN, original first cause, no atomicity regression; SceneGpu is host stub.");return 0;
}
