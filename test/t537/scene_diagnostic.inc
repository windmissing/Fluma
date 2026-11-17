bool Scene::DrawDiagnostic(const ResidentSceneFrame& frame,uint64_t epoch,CandidateCheck check,ResidentScene::Swap swap,void* user,input537::Pixel& out){
 out={};out.epoch=epoch;out.width=frame.width;out.height=frame.height;
 const auto current=[&]{return check&&check(user);};
 if(!current()||!PreparePrograms(epoch,check,user)||!current()||targets.Ensure(frame.width,frame.height,epoch)!=p013::Status::OK)return false;
 if(!diagnosticPublication){glGenBuffers(1,&diagnosticPublication);glBindBuffer(GL_SHADER_STORAGE_BUFFER,diagnosticPublication);std::array<uint32_t,256> empty{};glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(empty),empty.data(),GL_STATIC_DRAW);}
 const auto error=[&]{const auto value=glGetError();if(value&&!out.gl)out.gl=value;return out.gl==0&&current();};
 if(!error())return false;glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,diagnosticPublication);
 glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,frame.width,frame.height);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindVertexArray(vao);Geometry(full,frame);glUniform1i(uniforms[0].Find("uLayer"),7);glDrawArrays(GL_TRIANGLES,0,3);if(!error())return false;
 out.fbo=targets.fbos[targets.candidate];out.texture=targets.textures[targets.candidate];out.program=full;
 glGetIntegerv(GL_VIEWPORT,out.viewport.data());glGetIntegerv(GL_SCISSOR_BOX,out.scissor.data());out.blend=glIsEnabled(GL_BLEND);out.scissorEnabled=glIsEnabled(GL_SCISSOR_TEST);
 glReadBuffer(GL_COLOR_ATTACHMENT0);for(uint32_t i=0;i<3;++i){out.roi[2*i]=int32_t((2*i+1)*frame.width/6);out.roi[2*i+1]=int32_t(frame.height/2);glReadPixels(out.roi[2*i],out.roi[2*i+1],1,1,GL_RGBA,GL_UNSIGNED_BYTE,out.fboPixels.data()+4*i);}out.fboRead=error();if(!out.fboRead)return false;
 // Identical final present shader/texture/sampler path to the physical scene.
 Geometry(full,frame);glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,frame.width,frame.height);glDisable(GL_BLEND);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,targets.textures[targets.candidate]);glUniform1i(uniforms[0].Find("uFrame"),1);glUniform1i(uniforms[0].Find("uLayer"),6);glDrawArrays(GL_TRIANGLES,0,3);if(!error())return false;
 glReadBuffer(GL_BACK);for(uint32_t i=0;i<3;++i)glReadPixels(out.roi[2*i],out.roi[2*i+1],1,1,GL_RGBA,GL_UNSIGNED_BYTE,out.defaultPixels.data()+4*i);out.defaultRead=error();if(!out.defaultRead)return false;
 out.swapBegin=programcache::Stamp();if(!current())return false;out.swap=swap&&swap(user);out.swapEnd=programcache::Stamp();if(!out.swap){const auto e=eglGetError();if(e!=EGL_SUCCESS)out.egl=e;}if(!out.swap||!current()||!error())return false;
 out.created=out.swapEnd;out.until=out.created+2000000000ull;targets.Commit();lastGood="T537_DIAGNOSTIC_NOT_PHYSICS|"+std::to_string(epoch)+"|"+std::to_string(out.created);return true;
}
