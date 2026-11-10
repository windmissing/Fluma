#pragma once
#include "layout_generated.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <limits>

namespace grs::m2 {
// Call on the cold EGL owner. Admission does not start until this succeeds.
enum class ProbeCode { OK, NO_CONTEXT, UNSUPPORTED_PROFILE, UNSUPPORTED_BINDINGS,
 UNSUPPORTED_GROUP, UNSUPPORTED_BLOCK_SIZE, UNSUPPORTED_ALIGNMENT, GL_ERROR };
struct Limits {
 GLint bindings=0,computeBlocks=0,fragmentBlocks=0,invocations=0,sizeX=0,sharedBytes=0,alignment=0;
 GLint64 blockBytes=0;
 GLenum glError=GL_NO_ERROR;
};
inline const Profile* SelectProfile(uint32_t k,uint32_t kw,uint32_t revision,uint32_t features,uint32_t policy){
 if(revision!=rev_wire || features!=rev_featureBitmapLo || policy!=kPolicyId)return nullptr;
 for(const auto& profile:kProfiles)if(profile.k==k&&profile.kw==kw)return &profile;
 return nullptr;
}
inline ProbeCode ProbeCurrent(const Profile* p,Limits& limits){
 limits={};
 if(!p)return ProbeCode::UNSUPPORTED_PROFILE;
 if(eglGetCurrentContext()==EGL_NO_CONTEXT)return ProbeCode::NO_CONTEXT;
 glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&limits.bindings);
 glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&limits.computeBlocks);
 glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&limits.fragmentBlocks);
 glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&limits.invocations);
 glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0,&limits.sizeX);
 glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&limits.sharedBytes);
 glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&limits.alignment);
 glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&limits.blockBytes);
 limits.glError=glGetError();
 if(limits.glError!=GL_NO_ERROR)return ProbeCode::GL_ERROR;
 if(limits.bindings<4||limits.computeBlocks<4||limits.fragmentBlocks<1)return ProbeCode::UNSUPPORTED_BINDINGS;
 if(limits.invocations<64||limits.sizeX<64||limits.sharedBytes<2048)return ProbeCode::UNSUPPORTED_GROUP;
 if(limits.alignment<=0||limits.alignment%4!=0)return ProbeCode::UNSUPPORTED_ALIGNMENT;
 const uint64_t required=p->coldTransientBytes;
 if(limits.blockBytes<0||uint64_t(limits.blockBytes)<required||
    required>uint64_t(std::numeric_limits<GLsizeiptr>::max()))return ProbeCode::UNSUPPORTED_BLOCK_SIZE;
 return ProbeCode::OK;
}
}
