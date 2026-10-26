#pragma once
#include "render_contract.h"
#include <GLES3/gl3.h>
namespace p013 {
class FullFrameTargets {
 public:
 Status Ensure(int,int,uint64_t);
 void Reset(bool);
 void Commit() noexcept {std::swap(candidate,lastGood);}
 GLuint textures[2]{},fbos[2]{};int candidate=0,lastGood=1,width=0,height=0;uint64_t context=0,allocations=0;
 size_t Bytes()const{return size_t(width)*height*8;}
};
}
