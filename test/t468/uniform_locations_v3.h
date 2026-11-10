#pragma once
#include "../m2/create_diagnostics.h"
namespace grs::m2route {
struct UniformApi {
 void Count(GLuint p,GLint* n){glGetProgramiv(p,GL_ACTIVE_UNIFORMS,n);}
 void Active(GLuint p,GLuint i,GLsizei cap,GLsizei* len,GLint* size,GLenum* type,char* name){glGetActiveUniform(p,i,cap,len,size,type,name);}
 GLint Location(GLuint p,const char* n){return glGetUniformLocation(p,n);}GLenum Error(){return glGetError();}
};
struct UniformLocations {
 struct Entry {std::array<char,96> name{};GLint location=-1;};
 std::array<Entry,128> entries{};uint32_t count=0;
 template<class Api> bool PrepareUsing(GLuint program,Api& api){
  create468::Enter(create468::Stage::REFLECT,program);count=0;GLint n=0;GLenum raw=0;
  auto query=[&]{auto e=api.Error();if(e&&!raw)raw=e;};
  api.Count(program,&n);query();
  if(n<0||n>GLint(entries.size())){create468::Exit(false,raw,uint64_t(int64_t(n)),entries.size(),1);return false;}
  for(GLint i=0;i<n;++i){
   GLsizei length=0;GLint size=0;GLenum type=0;auto& e=entries[uint32_t(i)];
   api.Active(program,GLuint(i),GLsizei(e.name.size()),&length,&size,&type,e.name.data());query();
   if(length<=0||length>=GLsizei(e.name.size()-1)){create468::Exit(false,raw,uint64_t(i),uint64_t(int64_t(length)),e.name.size());return false;}
   e.location=api.Location(program,e.name.data());query();
   if(length>3&&std::strcmp(e.name.data()+length-3,"[0]")==0)e.name[size_t(length)-3]=0;
   ++count;
  }
  create468::Exit(true,raw,n,entries.size(),0);return true;
 }
 bool Prepare(GLuint program){UniformApi api;return PrepareUsing(program,api);}
 GLint Find(const char* name)const{for(uint32_t i=0;i<count;++i)if(std::strcmp(name,entries[i].name.data())==0)return entries[i].location;return -1;}
};
}
