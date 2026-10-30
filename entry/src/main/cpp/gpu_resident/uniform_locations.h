#pragma once
#include <GLES3/gl31.h>
#include <array>
#include <cstring>
namespace grs {
// T317. Reflect once before READY. Never ask the driver to resolve names in
// the per-pass submission loop; no lazy hot miss or cache overflow fallback.
struct UniformLocations {
 struct Entry {std::array<char,96> name{};GLint location=-1;};
 std::array<Entry,128> entries{};uint32_t count=0;
 bool Prepare(GLuint program){
  count=0;GLint n=0;glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&n);
  if(n<0||n>GLint(entries.size()))return false;
  for(GLint i=0;i<n;i++){
   GLsizei length=0;GLint size=0;GLenum type=0;auto& e=entries[uint32_t(i)];
   glGetActiveUniform(program,GLuint(i),GLsizei(e.name.size()),&length,&size,&type,e.name.data());
   if(length<=0||length>=GLsizei(e.name.size()-1))return false;
   e.location=glGetUniformLocation(program,e.name.data());
   // SSBO block members are not active uniforms; array base names may use [0].
   if(length>3&&std::strcmp(e.name.data()+length-3,"[0]")==0)e.name[size_t(length)-3]=0;
   count++;
  }
  return true;
 }
 GLint Find(const char* name)const{for(uint32_t i=0;i<count;i++)if(std::strcmp(name,entries[i].name.data())==0)return entries[i].location;return -1;}
};
}
