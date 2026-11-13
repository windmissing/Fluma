#pragma once
#include "program_factory.h"
#include <string_view>
#include <tuple>

namespace grs::programcache {
struct ShaderStageSource {
  GLenum stage = 0;
  std::string_view bytes;
};
struct GraphicsProgramDesc {
  std::string_view role;
  std::array<ShaderStageSource, 2> stages;
  std::string_view bindingAbi;
};
inline constexpr char kGraphicsLinkConfig[] =
    "VERTEX+FRAGMENT|SEPARABLE=0|GLSL_EXPLICIT_ATTRIBUTES|NO_PRELINK_ATTRIBUTE_OR_OUTPUT_CALLS";
struct GraphicsUniformTable {
  struct Entry {std::string name;GLint location=-1,size=0;GLenum type=0;};
  std::array<Entry, 16> entries{};
  size_t count = 0;
  GLint Find(const char* name) const {
    for(size_t i=0;i<count;++i)if(entries[i].name==name)return entries[i].location;
    return -1;
  }
};
struct GraphicsLog {
  std::string bytes;
  GLint reported = 0;
  GLenum queryError = 0;
};
struct GraphicsRecord {
  std::string key,role,abi,environment,route,cacheReason;
  std::array<std::string,2> shaderSha{};
  std::array<size_t,2> shaderBytes{};
  std::array<GraphicsLog,2> compileLog{};
  GraphicsLog linkLog;
  GraphicsUniformTable uniforms;
  std::vector<GraphicsUniformTable::Entry> attributes;
  GLint publicationBinding=-1,colorType=0,colorSize=0,colorLocation=-1;
  bool stateAttributeInactive=false;
  std::vector<GLint> formats;
  EGLContext context = EGL_NO_CONTEXT;
  std::thread::id owner;
  uint64_t generation=0,begin=0,compileCalls=0,linkCalls=0,loadCalls=0;
  uint64_t compileBegin=0,compileEnd=0,linkBegin=0,linkEnd=0,loadBegin=0,loadEnd=0;
  uint64_t reflectBegin=0,reflectEnd=0,prepareBegin=0,prepareEnd=0;
  GLenum productError=0,queryError=0;
  EGLint queryEgl=EGL_SUCCESS;
  bool cacheEnabled=true,prepared=false;
};
using GraphicsIdentity=std::tuple<uintptr_t,uint64_t,GLuint>;
inline thread_local std::map<GraphicsIdentity,GraphicsRecord> graphicsRecords;
inline GraphicsIdentity GraphicsId(GLuint program,uint64_t generation,EGLContext context) {
  return {reinterpret_cast<uintptr_t>(context),generation,program};
}
inline void AppendGraphicsField(std::string& out,std::string_view value) {
  const uint64_t size=value.size();
  for(unsigned i=0;i<8;++i)out+=char(size>>(8*i));
  out.append(value.data(),value.size());
}
inline bool ValidGraphics(const GraphicsProgramDesc& d) {
  if(d.role!="SCENE_FULL"&&d.role!="SCENE_PARTICLES"&&d.role!="SCENE_WATER")return false;
  if(d.stages[0].stage!=GL_VERTEX_SHADER||d.stages[1].stage!=GL_FRAGMENT_SHADER||
     d.bindingAbi.empty()||d.bindingAbi.size()>kEntryLimit)return false;
  size_t total=d.bindingAbi.size();
  for(const auto& stage:d.stages) {
    if(stage.bytes.empty()||stage.bytes.size()>kEntryLimit-total||
       stage.bytes.find('\0')!=std::string_view::npos)return false;
    total+=stage.bytes.size();
  }
  return true;
}
inline std::string GraphicsKey(const GraphicsProgramDesc& d,const std::string& environment,
                               std::string_view link=kGraphicsLinkConfig) {
  if(!ValidGraphics(d))return {};
  std::string bytes="T480_GRAPHICS_KEY_V1";
  for(const auto& stage:d.stages) {
    const char value[]={char(stage.stage),char(stage.stage>>8),char(stage.stage>>16),char(stage.stage>>24)};
    AppendGraphicsField(bytes,std::string_view(value,4));
    AppendGraphicsField(bytes,stage.bytes);
  }
  AppendGraphicsField(bytes,link);AppendGraphicsField(bytes,d.bindingAbi);
  AppendGraphicsField(bytes,environment);
  return Hash(bytes);
}
inline GraphicsLog GraphicsObjectLog(GLuint object,bool shader,GLsizei budget) {
  GraphicsLog out;
  if(shader)glGetShaderiv(object,GL_INFO_LOG_LENGTH,&out.reported);
  else glGetProgramiv(object,GL_INFO_LOG_LENGTH,&out.reported);
  out.queryError=glGetError();
  if(out.reported>1) {
    out.bytes.resize(size_t(std::min(out.reported,budget)));
    GLsizei copied=0;
    if(shader)glGetShaderInfoLog(object,GLsizei(out.bytes.size()),&copied,out.bytes.data());
    else glGetProgramInfoLog(object,GLsizei(out.bytes.size()),&copied,out.bytes.data());
    const auto error=glGetError();if(error&&!out.queryError)out.queryError=error;
    out.bytes.resize(size_t(std::clamp(copied,0,GLsizei(out.bytes.size()-1))));
  }
  return out;
}
inline void GraphicsLogJson(std::ostream& out,const GraphicsLog& log) {
  out<<"{\"text\":"<<Quote(log.bytes)<<",\"bytesHex\":"<<Quote(Hex(log.bytes.data(),log.bytes.size()))
     <<",\"reported\":"<<log.reported<<",\"copied\":"<<log.bytes.size()
     <<",\"truncated\":"<<(log.reported>GLint(log.bytes.size()+1)?"true":"false")
     <<",\"queryError\":"<<log.queryError<<'}';
}
inline void GraphicsReceipt(const GraphicsRecord& r,const char* stage,
                            std::shared_ptr<Artifact> artifact={}) {
  std::ostringstream out;
  out<<"{\"revision\":\"T480_SCENE_PROGRAM_ARTIFACT_V1\",\"programKind\":\"GRAPHICS\",\"role\":"
     <<Quote(r.role)<<",\"key\":"<<Quote(r.key)<<",\"bindingAbiSHA\":"<<Quote(Hash(r.abi))
     <<",\"linkConfigSHA\":"<<Quote(Hash(kGraphicsLinkConfig))<<",\"environmentSHA\":"<<Quote(r.environment)
     <<",\"packageSource\":"<<Quote(packageSource)<<",\"context\":\""<<reinterpret_cast<uintptr_t>(r.context)
     <<"\",\"contextGeneration\":\""<<r.generation<<"\",\"owner\":\""<<std::hash<std::thread::id>{}(r.owner)
     <<"\",\"route\":"<<Quote(r.route)<<",\"stage\":"<<Quote(stage)<<",\"compileCalls\":"<<r.compileCalls
     <<",\"cacheReason\":"<<Quote(r.cacheReason)
     <<",\"linkCalls\":"<<r.linkCalls<<",\"loadCalls\":"<<r.loadCalls
     <<",\"firstProductError\":"<<r.productError<<",\"queryError\":"<<r.queryError<<",\"queryEgl\":"<<r.queryEgl
     <<",\"beginNs\":"<<r.begin<<",\"endNs\":"<<Stamp()
     <<",\"compileBeginNs\":"<<r.compileBegin<<",\"compileEndNs\":"<<r.compileEnd
     <<",\"linkBeginNs\":"<<r.linkBegin<<",\"linkEndNs\":"<<r.linkEnd
     <<",\"loadBeginNs\":"<<r.loadBegin<<",\"loadEndNs\":"<<r.loadEnd
     <<",\"reflectBeginNs\":"<<r.reflectBegin<<",\"reflectEndNs\":"<<r.reflectEnd
     <<",\"prepareBeginNs\":"<<r.prepareBegin<<",\"prepareEndNs\":"<<r.prepareEnd
     <<",\"binaryBytes\":"<<(artifact?artifact->bytes.size():0)<<",\"binaryFormat\":"<<(artifact?artifact->format:0)
     <<",\"shaderStages\":[";
  for(size_t i=0;i<2;++i) {
    if(i)out<<',';
    out<<"{\"stage\":"<<(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER)<<",\"sha256\":"
       <<Quote(r.shaderSha[i])<<",\"bytes\":"<<r.shaderBytes[i]<<",\"compileLog\":";
    GraphicsLogJson(out,r.compileLog[i]);out<<'}';
  }
  out<<"],\"linkLog\":";GraphicsLogJson(out,r.linkLog);out<<",\"uniforms\":[";
  for(size_t i=0;i<r.uniforms.count;++i) {
    if(i)out<<',';
    const auto& u=r.uniforms.entries[i];
    out<<"{\"name\":"<<Quote(u.name)<<",\"location\":"<<u.location<<",\"type\":"<<u.type<<",\"size\":"<<u.size<<'}';
  }
  out<<"],\"attributes\":[";
  for(size_t i=0;i<r.attributes.size();++i){
    if(i)out<<',';
    const auto& a=r.attributes[i];
    out<<"{\"name\":"<<Quote(a.name)<<",\"location\":"<<a.location<<",\"type\":"<<a.type<<",\"size\":"<<a.size<<'}';
  }
  out<<"],\"aStateInactive\":"<<(r.stateAttributeInactive?"true":"false")
     <<",\"publicationBinding\":"<<r.publicationBinding<<",\"colorType\":"<<r.colorType
     <<",\"colorSize\":"<<r.colorSize<<",\"colorLocation\":"<<r.colorLocation<<"}";
  if(!r.key.empty())Store().Receipt(r.key,out.str(),std::move(artifact));
}
inline bool BuildGraphics(const GraphicsProgramDesc& d,GLuint& program,GLenum& firstProductError,
                          uint64_t generation,bool cacheEnabled=true) {
  program=0;
  if(!generation||!ValidGraphics(d)||graphicsRecords.size()>=kCountLimit)return false;
  GraphicsRecord r;
  r.begin=Stamp();r.context=eglGetCurrentContext();r.owner=std::this_thread::get_id();
  r.generation=generation;r.role=std::string(d.role);r.abi=std::string(d.bindingAbi);r.cacheEnabled=cacheEnabled;
  for(size_t i=0;i<2;++i){r.shaderSha[i]=Hash(d.stages[i].bytes.data(),d.stages[i].bytes.size());r.shaderBytes[i]=d.stages[i].bytes.size();}
  r.key=GraphicsKey(d,"ENVIRONMENT_NOT_QUERIED");
  const auto prior=glGetError();
  if(prior||r.context==EGL_NO_CONTEXT) {
    r.productError=prior;if(prior&&!firstProductError)firstProductError=prior;
    GraphicsReceipt(r,prior?"PRIOR_PRODUCT_GL_ERROR":"NO_CURRENT_CONTEXT");return false;
  }
  r.environment=Environment();r.queryError=glGetError();r.queryEgl=eglGetError();r.key=GraphicsKey(d,r.environment);
  GLint count=0;glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,&count);
  std::vector<GLint> formats;
  if(count>0&&count<=256){formats.resize(size_t(count));glGetIntegerv(GL_PROGRAM_BINARY_FORMATS,formats.data());}
  const auto query=glGetError();if(query&&!r.queryError)r.queryError=query;
  if(r.queryError||r.queryEgl!=EGL_SUCCESS||r.environment.empty())formats.clear();
  r.formats=formats;
  r.route=cacheEnabled?"SOURCE_COMPILE":"CACHE_DISABLED_SOURCE";
  if(cacheEnabled){r.cacheReason=Store().Rejection(r.key);if(!r.cacheReason.empty()){r.route="REJECT_THEN_SOURCE_ONCE";GraphicsReceipt(r,"DISK_REJECTED");}}
  if(cacheEnabled&&!formats.empty())if(auto artifact=Store().Get(r.key)) {
    if(std::find(formats.begin(),formats.end(),GLint(artifact->format))!=formats.end()) {
      r.loadBegin=Stamp();++r.loadCalls;GraphicsReceipt(r,"BINARY_LOAD_ENTER",artifact);
      program=glCreateProgram();glProgramBinary(program,artifact->format,artifact->bytes.data(),GLsizei(artifact->bytes.size()));
      GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);
      const auto error=glGetError();if(error)r.queryError=error;
      r.linkLog=GraphicsObjectLog(program,false,1024);r.loadEnd=Stamp();
      if(linked&&!error&&!r.linkLog.queryError) {
        r.route="BINARY_LOAD";GraphicsReceipt(r,"LOAD_LINK_OK_REFLECTION_PENDING",artifact);
        graphicsRecords.emplace(GraphicsId(program,generation,r.context),r);return true;
      }
      glDeleteProgram(program);program=0;
    }
    Store().Reject(r.key);r.route="REJECT_THEN_SOURCE_ONCE";GraphicsReceipt(r,"LOAD_REJECTED");
  }
  std::array<GLuint,2> shaders{};
  bool success=true;
  r.compileBegin=Stamp();
  for(size_t i=0;i<2;++i) {
    shaders[i]=glCreateShader(d.stages[i].stage);
    const auto* bytes=d.stages[i].bytes.data();const GLint length=GLint(d.stages[i].bytes.size());
    glShaderSource(shaders[i],1,&bytes,&length);++r.compileCalls;GraphicsReceipt(r,i==0?"VERTEX_COMPILE_ENTER":"FRAGMENT_COMPILE_ENTER");
    glCompileShader(shaders[i]);GLint compiled=0;glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&compiled);
    const auto error=glGetError();if(error&&!r.productError)r.productError=error;
    r.compileLog[i]=GraphicsObjectLog(shaders[i],true,1536);
    if(!compiled||error){success=false;break;}
  }
  r.compileEnd=Stamp();
  if(success) {
    program=glCreateProgram();
    for(auto shader:shaders)glAttachShader(program,shader);
    if(cacheEnabled&&!formats.empty()){glProgramParameteri(program,GL_PROGRAM_BINARY_RETRIEVABLE_HINT,GL_TRUE);const auto error=glGetError();if(error)r.queryError=error;}
    r.linkBegin=Stamp();++r.linkCalls;GraphicsReceipt(r,"LINK_ENTER");glLinkProgram(program);
    GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);
    const auto error=glGetError();if(error&&!r.productError)r.productError=error;
    r.linkEnd=Stamp();r.linkLog=GraphicsObjectLog(program,false,1024);
    success=linked&&!error;
  }
  for(auto shader:shaders)if(shader)glDeleteShader(shader);
  if(!success) {
    if(program)glDeleteProgram(program);program=0;
    if(r.productError&&!firstProductError)firstProductError=r.productError;
    GraphicsReceipt(r,"COMPILE_OR_LINK_FAILED");return false;
  }
  GraphicsReceipt(r,"LINK_OK_REFLECTION_PENDING");
  graphicsRecords.emplace(GraphicsId(program,generation,r.context),r);return true;
}
inline GraphicsRecord* GraphicsCurrent(GLuint program,uint64_t generation) {
  const auto at=graphicsRecords.find(GraphicsId(program,generation,eglGetCurrentContext()));
  if(at==graphicsRecords.end()||at->second.owner!=std::this_thread::get_id())return nullptr;
  return &at->second;
}
inline bool ReflectGraphics(GLuint program,GraphicsUniformTable& table,const GraphicsProgramDesc& d,
                            GLenum& firstProductError,uint64_t generation) {
  auto* record=GraphicsCurrent(program,generation);if(!record)return false;
  if(record->role!=d.role||record->abi!=d.bindingAbi)return false;
  record->reflectBegin=Stamp();table={};record->attributes.clear();record->stateAttributeInactive=false;
  record->publicationBinding=-1;
  GLint count=0;glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&count);
  bool valid=count>=0&&count<=GLint(table.entries.size());
  for(GLint i=0;valid&&i<count;++i) {
    char name[96]{};GLsizei length=0;auto& entry=table.entries[table.count++];
    glGetActiveUniform(program,GLuint(i),sizeof(name),&length,&entry.size,&entry.type,name);
    valid=length>0&&length<GLsizei(sizeof(name)-1);entry.name=name;
    entry.location=glGetUniformLocation(program,name);
    if(entry.name.size()>3&&entry.name.substr(entry.name.size()-3)=="[0]")entry.name.resize(entry.name.size()-3);
    valid=valid&&entry.location>=0;
  }
  auto uniform=[&](const char* name,GLenum type,GLint size=1) {
    for(size_t i=0;i<table.count;++i){const auto& e=table.entries[i];if(e.name==name)return e.location>=0&&e.type==type&&e.size>=size;}
    return false;
  };
  valid=valid&&uniform("uResolution",GL_FLOAT_VEC2)&&uniform("uBox",GL_FLOAT_VEC4)&&uniform("uMirror",GL_BOOL);
  if(d.role=="SCENE_FULL") {
    valid=valid&&uniform("uColumn",GL_UNSIGNED_INT)&&uniform("uLayer",GL_INT)&&uniform("uFrame",GL_SAMPLER_2D)&&uniform("uOptics",GL_FLOAT_VEC4,4);
    const GLuint block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"Publication");
    const GLenum property=GL_BUFFER_BINDING;GLint binding=-1;
    if(block!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_SHADER_STORAGE_BLOCK,block,1,&property,1,nullptr,&binding);
    record->publicationBinding=binding;
    valid=valid&&block!=GL_INVALID_INDEX&&binding==0;
  }else {
    const char* names[]={"aX","aY","aMass","aState","aRadius"};
    const GLenum types[]={GL_FLOAT,GL_FLOAT,GL_UNSIGNED_INT,GL_UNSIGNED_INT,GL_FLOAT};
    const size_t required=d.role=="SCENE_PARTICLES"?5:3;
    GLint active=0;glGetProgramiv(program,GL_ACTIVE_ATTRIBUTES,&active);
    for(size_t i=0;i<required;++i) {
      const GLint location=glGetAttribLocation(program,names[i]);
      if(i==3&&location==-1){record->stateAttributeInactive=true;continue;}
      bool matched=false;
      for(GLint a=0;a<active;++a) {
        char name[96]{};GLsizei length=0;GLint size=0;GLenum type=0;
        glGetActiveAttrib(program,GLuint(a),sizeof(name),&length,&size,&type,name);
        if(std::strcmp(name,names[i])==0){
          matched=type==types[i]&&size==1;
          record->attributes.push_back({name,location,size,type});
        }
      }
      valid=valid&&location==GLint(i)&&matched;
    }
    if(d.role=="SCENE_WATER")valid=valid&&uniform("uRadius",GL_FLOAT)&&uniform("uMassReference",GL_FLOAT)&&uniform("uOptics",GL_FLOAT_VEC4,3);
  }
  GLint outputs=0;glGetProgramInterfaceiv(program,GL_PROGRAM_OUTPUT,GL_ACTIVE_RESOURCES,&outputs);
  const GLuint color=glGetProgramResourceIndex(program,GL_PROGRAM_OUTPUT,"color");
  const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};GLint output[3]={};
  if(color!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_OUTPUT,color,3,properties,3,nullptr,output);
  record->colorType=output[0];record->colorSize=output[1];record->colorLocation=output[2];
  valid=valid&&outputs==1&&color!=GL_INVALID_INDEX&&output[0]==GL_FLOAT_VEC4&&output[1]==1&&
   output[2]==0&&glGetFragDataLocation(program,"color")==0;
  const auto error=glGetError();if(error&&!firstProductError)firstProductError=error;
  if(error&&!record->productError)record->productError=error;
  record->uniforms=table;record->reflectEnd=Stamp();
  GraphicsReceipt(*record,valid&&!error?"REFLECT_OK":"REFLECT_FAILED");
  if(!valid||error){if(record->route=="BINARY_LOAD")Store().Reject(record->key);return false;}
  return true;
}
inline GLenum GraphicsPrepared(GLuint program,uint64_t generation) {
  auto* r=GraphicsCurrent(program,generation);if(!r)return GL_INVALID_OPERATION;
  r->prepareBegin=Stamp();const auto error=glGetError();r->prepareEnd=Stamp();
  if(error){r->productError=error;GraphicsReceipt(*r,"VAO_PREPARE_FAILED");return error;}
  r->prepared=true;
  if(!r->cacheEnabled){GraphicsReceipt(*r,"CACHE_DISABLED_PREPARED");return GL_NO_ERROR;}
  if(r->route=="BINARY_LOAD"){GraphicsReceipt(*r,"REFLECT_PREPARE_OK",Store().Get(r->key));return GL_NO_ERROR;}
  if(r->formats.empty()){GraphicsReceipt(*r,"BINARY_UNAVAILABLE");return GL_NO_ERROR;}
  if(Store().ExportUnavailable(r->key)){GraphicsReceipt(*r,"EXPORT_UNAVAILABLE_THIS_STARTUP");return GL_NO_ERROR;}
  GLint size=0;glGetProgramiv(program,GL_PROGRAM_BINARY_LENGTH,&size);r->queryError=glGetError();
  if(r->queryError||size<=0||size>GLint(kEntryLimit)){Store().DisableExport(r->key);GraphicsReceipt(*r,"BINARY_UNAVAILABLE");return GL_NO_ERROR;}
  try {
    std::vector<uint8_t> bytes(size_t(size),0);GLsizei copied=0;GLenum format=0;
    glGetProgramBinary(program,size,&copied,&format,bytes.data());r->queryError=glGetError();
    if(r->queryError||copied<=0||copied>size||
       std::find(r->formats.begin(),r->formats.end(),GLint(format))==r->formats.end()){
      Store().DisableExport(r->key);GraphicsReceipt(*r,"EXPORT_FAILED");return GL_NO_ERROR;
    }
    bytes.resize(size_t(copied));auto artifact=Store().Put(r->key,format,std::move(bytes));
    GraphicsReceipt(*r,artifact?"EXPORT_WRITE_SUBMITTED":"CACHE_NOT_STORED",artifact);
  }catch(...){Store().DisableExport(r->key);GraphicsReceipt(*r,"CACHE_EXPORT_EXCEPTION");}
  return GL_NO_ERROR;
}
inline void ForgetGraphics(GLuint program,uint64_t generation,EGLContext context) {
  const auto at=graphicsRecords.find(GraphicsId(program,generation,context));
  if(at!=graphicsRecords.end()&&at->second.owner==std::this_thread::get_id()) {
    GraphicsReceipt(at->second,at->second.prepared?"CONTEXT_RELEASED":"PREPARE_ABORTED");
    graphicsRecords.erase(at);
  }
}
}
