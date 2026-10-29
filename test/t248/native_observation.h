// Isolated diagnostic build only. CPU wall times, never a GPU timer.
#pragma once
#include <chrono>
#include <GLES3/gl3.h>
#include <hilog/log.h>
namespace t248 {
inline double now(){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline void timing(const char* token,const char* stage,double begin,int code){OH_LOG_Print(LOG_APP,LOG_INFO,0x248,"T248","token=%{public}s stage=%{public}s wallMs=%{public}.6f code=%{public}d gpuTimer=NOT_MEASURED",token,stage,now()-begin,code);}
inline void sample(const char* token){GLint viewport[4]={};glGetIntegerv(GL_VIEWPORT,viewport);unsigned char rgba[12]={};
 // Three documented pixel positions in current full-frame candidate target, before Present/Swap.
 for(int i=0;i<3;i++)glReadPixels(viewport[0]+viewport[2]/2,viewport[1]+(i+1)*viewport[3]/4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba+4*i);
 OH_LOG_Print(LOG_APP,LOG_INFO,0x248,"T248","token=%{public}s phase=BEFORE_PRESENT viewport=%{public}d,%{public}d,%{public}d,%{public}d pixelRGBA=%{public}u,%{public}u,%{public}u,%{public}u;%{public}u,%{public}u,%{public}u,%{public}u;%{public}u,%{public}u,%{public}u,%{public}u sampleStatus=READ_ISSUED_GL_ERROR_OWNED_BY_PIPELINE",token,viewport[0],viewport[1],viewport[2],viewport[3],rgba[0],rgba[1],rgba[2],rgba[3],rgba[4],rgba[5],rgba[6],rgba[7],rgba[8],rgba[9],rgba[10],rgba[11]);
}
}
