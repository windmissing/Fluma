bool t485NoFormats=false;
int main(int argc,char** argv) {
 assert(argc==2);Context context;assert(Store().Configure(argv[1]));
 const auto descriptors=m1::ScenePrograms();const auto environment=Environment();
 unsigned passed=0;
 auto cleanup=[&](GLuint program,uint64_t generation){ForgetGraphics(program,generation,eglGetCurrentContext());glDeleteProgram(program);assert(graphicsRecords.empty());};
 {
  auto d=descriptors[0];d.stages[1].bytes={};assert(!ValidGraphics(d));
  d=descriptors[0];d.stages[1].stage=GL_COMPUTE_SHADER;assert(!ValidGraphics(d));
  d=descriptors[0];d.stages[0].bytes=std::string_view("x",kEntryLimit+1);assert(!ValidGraphics(d));++passed;
 }
 for(size_t stage=0;stage<2;++stage) {
  auto d=descriptors[0];std::string broken(d.stages[stage].bytes);broken+="\ninvalid shader syntax\n";d.stages[stage].bytes=broken;
  GLuint program=0;GLenum error=0;assert(!BuildGraphics(d,program,error,1)&&program==0);++passed;
 }
 {
  auto d=descriptors[0];
  d.stages[0].bytes="#version 310 es\nprecision highp float;out vec3 varyingValue;void main(){varyingValue=vec3(1.);gl_Position=vec4(0.);}";
  d.stages[1].bytes="#version 310 es\nprecision highp float;in vec2 varyingValue;out vec4 color;void main(){color=vec4(varyingValue,0.,1.);}";
  GLuint program=0;GLenum error=0;assert(!BuildGraphics(d,program,error,1)&&!program);++passed;
 }
 {
  auto d=descriptors[1];std::string source(d.stages[0].bytes);
  const auto at=source.find("location=0");assert(at!=std::string::npos);source.replace(at,10,"location=5");d.stages[0].bytes=source;
  GLuint program=0;GLenum error=0;GraphicsUniformTable uniforms;
  assert(BuildGraphics(d,program,error,2));assert(!ReflectGraphics(program,uniforms,d,error,2));
  cleanup(program,2);++passed;
 }
 for(bool unknownFormat:{true,false}) {
  auto d=descriptors[0];std::string abi(d.bindingAbi);abi+=unknownFormat?"|UNKNOWN_FORMAT":"|DRIVER_REJECT";d.bindingAbi=abi;
  const auto key=GraphicsKey(d,environment);
  GLint count=0;glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,&count);assert(count>0);
  std::vector<GLint> formats(size_t(count),0);glGetIntegerv(GL_PROGRAM_BINARY_FORMATS,formats.data());
  assert(Store().Put(key,unknownFormat?0xffffffffu:uint32_t(formats[0]),{1,2,3,4}));
  GLuint program=0;GLenum error=0;assert(BuildGraphics(d,program,error,3));
  const auto* record=GraphicsCurrent(program,3);assert(record&&record->compileCalls==2&&record->route=="REJECT_THEN_SOURCE_ONCE");
  cleanup(program,3);++passed;
 }
 {
  GLuint program=0;GLenum error=0;glEnable(0xffffffffu);
  assert(!BuildGraphics(descriptors[0],program,error,4)&&error==GL_INVALID_ENUM&&program==0);++passed;
 }
 {
  auto d=descriptors[0];std::string abi(d.bindingAbi);abi+="|IO_FAILURE";d.bindingAbi=abi;
  Store().SetWriteFault(1);GLuint program=0;GLenum error=0;GraphicsUniformTable uniforms;
  assert(BuildGraphics(d,program,error,5)&&ReflectGraphics(program,uniforms,d,error,5));
  GLuint vao=0;glGenVertexArrays(1,&vao);glBindVertexArray(vao);
  assert(GraphicsPrepared(program,5)==GL_NO_ERROR);Store().Drain();assert(glIsProgram(program));
  auto artifact=Store().Get(GraphicsKey(d,environment));assert(artifact&&artifact->state==WRITE_FAILED);
  Store().SetWriteFault(0);glDeleteVertexArrays(1,&vao);cleanup(program,5);++passed;
 }
 {
  GLuint program=0;GLenum error=0;assert(BuildGraphics(descriptors[0],program,error,6));
  auto old=*GraphicsCurrent(program,6);old.generation=5;
  graphicsRecords.emplace(GraphicsId(program,5,eglGetCurrentContext()),old);
  ForgetGraphics(program,5,eglGetCurrentContext());assert(GraphicsCurrent(program,6)&&glIsProgram(program));
  std::thread stale([&]{ForgetGraphics(program,6,context.context);});stale.join();
  assert(GraphicsCurrent(program,6)&&glIsProgram(program));cleanup(program,6);++passed;
 }
 {
  m1::Scene old;assert(old.PreparePrograms(7));
  EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE};
  auto next=eglCreateContext(context.display,context.config,EGL_NO_CONTEXT,ca);assert(next!=EGL_NO_CONTEXT);
  assert(eglMakeCurrent(context.display,context.surface,context.surface,next));
  m1::Scene current;assert(current.PreparePrograms(8));
  GLuint program=0;
  for(const auto& row:graphicsRecords)if(row.second.context==next)program=std::get<2>(row.first);
  assert(program&&glIsProgram(program));old.Reset(false);
  assert(graphicsRecords.size()==3&&glIsProgram(program));
  current.Reset(true);assert(graphicsRecords.empty());
  assert(eglMakeCurrent(context.display,context.surface,context.surface,context.context));
  assert(eglDestroyContext(context.display,next));++passed;
 }
 {
  auto d=descriptors[0];std::string abi(d.bindingAbi);abi+="|NO_FORMATS";d.bindingAbi=abi;
  t485NoFormats=true;GLuint program=0;GLenum error=0;GraphicsUniformTable uniforms;
  assert(BuildGraphics(d,program,error,10)&&ReflectGraphics(program,uniforms,d,error,10));
  assert(GraphicsPrepared(program,10)==GL_NO_ERROR&&!Store().Get(GraphicsKey(d,environment)));
  assert(GraphicsCurrent(program,10)->formats.empty()&&GraphicsCurrent(program,10)->compileCalls==2);
  cleanup(program,10);t485NoFormats=false;++passed;
 }
 {
  m1::Scene scene;const auto invalid=[](void*){return false;};
  assert(!scene.PreparePrograms(9,invalid)&&scene.LastGoodToken().empty()&&graphicsRecords.empty());++passed;
 }
 Store().Drain();
 std::cout<<"{\"schema\":\"T485_GRAPHICS_FACTORY_FAULTS_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}"<<std::endl;
}
