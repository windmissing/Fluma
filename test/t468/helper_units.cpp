#include "m2/create_diagnostics.h"
#include "uniform_locations_v3.h"
#include <cassert>
#include <sstream>
#include <iostream>
#include <string>
using namespace grs;using namespace grs::create468;
struct FakeApi {
 bool compile=true,link=true,shaderAlive=false,programAlive=false;std::string message;GLenum pending=0,compileRaw=0,queryRaw=0;unsigned logCalls=0;
 GLuint CreateShader(){shaderAlive=true;return 1;}void Source(GLuint,const char*){}void Compile(GLuint){pending=compileRaw;}
 void Shaderiv(GLuint,GLenum which,GLint* v){if(which==GL_COMPILE_STATUS)*v=compile;else{*v=message.empty()?0:int(message.size()+1);pending=queryRaw;}}
 void ShaderLog(GLuint,GLsizei cap,GLsizei* n,char* out){assert(shaderAlive);++logCalls;*n=std::min(int(message.size()),cap-1);std::memcpy(out,message.data(),*n);out[*n]=0;}
 GLuint CreateProgram(){programAlive=true;return 2;}void Attach(GLuint,GLuint){}void Link(GLuint){}
 void Programiv(GLuint,GLenum which,GLint* v){if(which==GL_LINK_STATUS)*v=link;else *v=message.empty()?0:int(message.size()+1);}
 void ProgramLog(GLuint,GLsizei cap,GLsizei* n,char* out){assert(programAlive&&shaderAlive);++logCalls;*n=std::min(int(message.size()),cap-1);std::memcpy(out,message.data(),*n);out[*n]=0;}
 void DeleteShader(GLuint){shaderAlive=false;}GLenum Error(){auto e=pending;pending=0;return e;}
};
struct UniformFake {
 int n=1,length=4;GLenum pending=0;void Count(GLuint,GLint* v){*v=n;}void Active(GLuint,GLuint,GLsizei,GLsizei* len,GLint* size,GLenum* type,char* out){*len=length;*size=1;*type=GL_UNSIGNED_INT;std::strcpy(out,"uFoo");}GLint Location(GLuint,const char*){return 3;}GLenum Error(){auto e=pending;pending=0;return e;}
};
int main(){
 Store store;Identity id{};unsigned cases=0;Attempt snapshot{};
 for(unsigned object=0;object<3;++object){
  Recorder recorder{store};recorder.serial=store.Begin(object+1,1,2,3,id,192,128);active=&recorder;
  FakeApi api;api.compile=false;GLuint p=0;GLenum error=0;
  assert(!Build(api,object,"void main(){}","abc",p,error));assert(api.logCalls==1&&!api.shaderAlive);
  recorder.Finish(6);assert(store.Copy(recorder.serial,snapshot));assert(snapshot.firstFailure.object==object&&snapshot.firstFailure.stage==Stage::COMPILE&&snapshot.programs[object].compile.queried&&snapshot.programs[object].compile.empty);++cases;
  auto old=snapshot;auto oldSerial=snapshot.serial;store.Begin(100,1,2,3,id,192,128);assert(store.Copy(oldSerial,snapshot)&&snapshot.firstFailure.stage==old.firstFailure.stage);++cases;
  recorder.serial=store.Begin(object+10,1,2,3,id,192,128);api={};api.link=false;api.message="compile warning / link failure";error=0;
  assert(!Build(api,object,"shader","abc",p,error));recorder.Finish(6);store.Copy(recorder.serial,snapshot);assert(snapshot.firstFailure.stage==Stage::LINK&&api.logCalls==2&&snapshot.programs[object].link.copied>0);++cases;
  recorder.serial=store.Begin(object+20,1,2,3,id,192,128);recorder.object=object;m2route::UniformLocations locations;UniformFake uniforms;uniforms.n=129;
  assert(!locations.PrepareUsing(2,uniforms));recorder.Finish(6);store.Copy(recorder.serial,snapshot);assert(snapshot.firstFailure.stage==Stage::REFLECT&&snapshot.firstFailure.a==129&&snapshot.firstFailure.b==128);++cases;
 }
 Recorder r{store};r.serial=store.Begin(50,1,2,3,id,512,128);active=&r;FakeApi api;api.compile=false;api.message=std::string(10000,'x');GLuint program=0;GLenum error=0;
 assert(!Build(api,0,"x","sha",program,error));r.Finish(6);store.Copy(r.serial,snapshot);assert(snapshot.programs[0].used==4095&&snapshot.programs[0].compile.truncated&&snapshot.programs[0].compile.reported==10001);++cases;
 r.serial=store.Begin(51,1,2,3,id,192,128);api={};api.compile=false;api.compileRaw=GL_INVALID_OPERATION;api.queryRaw=GL_INVALID_ENUM;error=0;
 assert(!Build(api,0,"x","sha",program,error));assert(error==GL_INVALID_OPERATION);assert(r.Consume(0)==GL_INVALID_OPERATION);assert(r.pending==0);r.Finish(6);store.Copy(r.serial,snapshot);assert(snapshot.firstFailure.rawGl==GL_INVALID_OPERATION&&snapshot.programs[0].compile.queryError==GL_INVALID_ENUM);++cases;
 r.serial=store.Begin(52,1,2,3,id,192,128);api={};api.message="warning";error=0;assert(Build(api,2,"shader","sha",program,error));r.Finish(0);store.Copy(r.serial,snapshot);assert(!snapshot.failed&&snapshot.programs[2].compile.copied>0);++cases;
 r.serial=store.Begin(53,1,2,3,id,192,128);r.object=2;UniformFake invalid;invalid.length=96;m2route::UniformLocations locations;assert(!locations.PrepareUsing(2,invalid));r.Finish(6);store.Copy(r.serial,snapshot);assert(snapshot.firstFailure.a==0&&snapshot.firstFailure.b==96&&snapshot.firstFailure.c==96);++cases;
 r.serial=store.Begin(54,1,2,3,id,192,128);invalid={};assert(locations.PrepareUsing(2,invalid)&&locations.Find("uFoo")==3);r.Finish(0);++cases;
 r.serial=store.Begin(55,1,2,3,id,192,128);r.Enter(Stage::COMPILE,2);store.Copy(r.serial,snapshot);assert(snapshot.calling&&!snapshot.sealed);const auto calls=api.logCalls;std::ostringstream out;Json(out,snapshot);assert(api.logCalls==calls&&out.str().find("inCallDurationNs")!=std::string::npos);++cases;
 r.Exit(false,GL_OUT_OF_MEMORY);r.Finish(6);store.Copy(r.serial,snapshot);const auto first=snapshot.firstFailure;
 r.Enter(Stage::LINK,4);store.Copy(r.serial,snapshot);assert(snapshot.firstFailure.stage==first.stage&&!snapshot.calling);++cases;
 r.pending=0;r.serial=store.Begin(56,1,2,3,id,192,128);api={};api.compileRaw=GL_INVALID_OPERATION;error=0;assert(Build(api,0,"shader","sha",program,error));assert(error==GL_INVALID_OPERATION&&r.Consume(0)==GL_INVALID_OPERATION);r.Finish(5);++cases;
 r.pending=0;r.serial=store.Begin(57,1,2,3,id,192,128);api={};api.queryRaw=GL_INVALID_ENUM;error=0;assert(Build(api,0,"shader","sha",program,error));assert(error==0&&r.pending==0);r.Finish(0);store.Copy(r.serial,snapshot);assert(!snapshot.failed&&snapshot.programs[0].compile.queryError==GL_INVALID_ENUM);++cases;
 active=nullptr;
 std::cout<<"{\"schema\":\"T468_ACTUAL_HELPER_UNITS_V1\",\"passed\":"<<cases<<",\"failed\":0}"<<std::endl;
}
