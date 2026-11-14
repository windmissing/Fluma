 {
  auto d=descriptors[0];std::string abi(d.bindingAbi);abi+="|T502_REMOVE_FAILURE";d.bindingAbi=abi;
  GLuint program=0;GLenum error=0;GraphicsUniformTable uniforms;
  Store().SetWriteFault(3);Store().SetRemoveFault(true);
  assert(BuildGraphics(d,program,error,11)&&ReflectGraphics(program,uniforms,d,error,11));
  assert(GraphicsPrepared(program,11)==GL_NO_ERROR);Store().Drain();
  auto artifact=Store().Get(GraphicsKey(d,environment));assert(artifact&&artifact->state==WRITE_FAILED);
  glUseProgram(program);assert(glIsProgram(program)&&glGetError()==GL_NO_ERROR);
  assert(Store().Diagnostics502().find("\"tempBytes\":0")==std::string::npos);
  Store().SetWriteFault(0);Store().SetRemoveFault(false);Store().Receipt(artifact->key,"{}",artifact);Store().Drain();
  assert(Store().Diagnostics502().find("\"tempBytes\":0")!=std::string::npos);
  glUseProgram(program);assert(glGetError()==GL_NO_ERROR);cleanup(program,11);++passed;
 }
 {
  auto d=descriptors[0];std::string abi(d.bindingAbi);abi+="|T502_QUEUE_FULL";d.bindingAbi=abi;
  GLuint program=0;GLenum error=0;GraphicsUniformTable uniforms;assert(BuildGraphics(d,program,error,12)&&ReflectGraphics(program,uniforms,d,error,12));Store().Drain();
  std::promise<void> entered,release,cleared;auto wait=release.get_future().share();Store().Enqueue([&]{entered.set_value();wait.wait();});entered.get_future().wait();
  for(unsigned i=0;i<128;++i)Store().Enqueue([&,i]{if(i==127)cleared.set_value();});
  assert(GraphicsPrepared(program,12)==GL_NO_ERROR);glUseProgram(program);assert(glIsProgram(program)&&glGetError()==GL_NO_ERROR);
  release.set_value();cleared.get_future().wait();Store().Drain();cleanup(program,12);++passed;
 }
 {
  std::vector<std::shared_ptr<Artifact>> pins;
  for(unsigned i=0;i<4;++i){pins.push_back(Store().Put(Hash("T502_PIN_"+std::to_string(i)),7,std::vector<uint8_t>(kEntryLimit,1)));assert(pins.back());Store().Drain();}
  auto d=descriptors[0];std::string abi(d.bindingAbi);abi+="|T502_PIN_FULL";d.bindingAbi=abi;
  GLuint program=0;GLenum error=0;GraphicsUniformTable uniforms;assert(BuildGraphics(d,program,error,13)&&ReflectGraphics(program,uniforms,d,error,13));assert(GraphicsPrepared(program,13)==GL_NO_ERROR);
  glUseProgram(program);assert(glIsProgram(program)&&glGetError()==GL_NO_ERROR);assert(!Store().Get(GraphicsKey(d,environment)));cleanup(program,13);pins.clear();Store().Drain();++passed;
 }
