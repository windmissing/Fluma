#pragma once
#include "m1/quantum_candidate.h"
#include "angles_generated.h"
#include "quantum_commit_kernel.h"
namespace grs {namespace m1 {
struct QuantumCommitTestAccess {
 static void Install(QuantumCandidate& q){glDeleteProgram(q.program);GLuint sh=glCreateShader(GL_COMPUTE_SHADER);const char* s=kM1QuantumCommit;glShaderSource(sh,1,&s,nullptr);glCompileShader(sh);GLint good;glGetShaderiv(sh,GL_COMPILE_STATUS,&good);assert(good);q.program=glCreateProgram();glAttachShader(q.program,sh);glLinkProgram(q.program);glDeleteShader(sh);glGetProgramiv(q.program,GL_LINK_STATUS,&good);assert(good);glUseProgram(q.program);q.Set("uSpare",q.workWords);q.Set("uReplayFrozen",q.workWords+q.words);uint32_t n=q.workWords+q.words+256;glBindBuffer(GL_SHADER_STORAGE_BUFFER,q.workspace);glBufferData(GL_SHADER_STORAGE_BUFFER,n*4,nullptr,GL_DYNAMIC_COPY);glBufferSubData(GL_SHADER_STORAGE_BUFFER,q.angles*4,sizeof(kAngleWords),kAngleWords);}
 static void Audit(QuantumCandidate& q){glDeleteSync(q.fence);q.fence=nullptr;q.Pass(21,1);q.Pass(22,q.words);q.Pass(23,1);q.fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();}
};
}}
