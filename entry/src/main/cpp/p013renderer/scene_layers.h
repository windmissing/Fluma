#pragma once
#include "scene_compositor.h"
namespace p013 {
class SceneLayers {
 public:
 Status Ensure(uint64_t);
 Status Upload(const float*);
 Status Draw(int,const ScenePacket&,const SceneConfiguration&);
 Status Present(GLuint,int,int);
 void Reset(bool);
 std::string sourceHash;
 GLuint program=0,present=0,vao=0,data=0,legacy=0;uint64_t epoch=0;
};
}
