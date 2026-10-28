#pragma once
#include "clear_water_packet.h"
#include "fullframe_targets.h"
#include "dualfield_pass.h"
#include "egl_context.h"
namespace p013 {
class ClearWaterGlGpu final:public ClearWaterGpu {
 public:explicit ClearWaterGlGpu(EglContext& e):egl(e){}
 Status Begin(const ClearWaterPacket&)override;Status Layer(int,const ClearWaterPacket&)override;Status Present()override;
 Status Swap(uint64_t epoch)override{return egl.Swap(epoch);}void Commit()override{targets.Commit();active=1-active;}
 void Reset(bool);FullFrameTargets targets;GLuint program=0,vao=0,profile[2]{},sand=0;uint64_t context=0;int active=0;
 private:EglContext& egl;DualFieldPass dual;const ClearWaterPacket* packet=nullptr;
 std::array<float,1024> staging{};std::array<float,512> bed{};
};
}
