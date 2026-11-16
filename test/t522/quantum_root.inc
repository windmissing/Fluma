// Offline actual-GL full fixed root; no preview/trial/plan readback in the loop.
if(scenario=="root"||scenario=="root-offset"||scenario=="root-clamped"||scenario=="root-full"){
 glDeleteProgram(program);shader=Read(".codex/local-test/t522-solver/quantum-root.glsl");
 std::array<GLuint,15> programs{};
 for(unsigned op=0;op<15;++op){
   auto specialized=shader;const auto line=specialized.find('\n');assert(line!=std::string::npos);specialized.insert(line+1,grs::m3a::kOpcodeDefines[op]);
   src=specialized.c_str();sh=glCreateShader(GL_COMPUTE_SHADER);glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);assert(ok);
   programs[op]=glCreateProgram();glAttachShader(programs[op],sh);glLinkProgram(programs[op]);glGetProgramiv(programs[op],GL_LINK_STATUS,&ok);assert(ok);glDeleteShader(sh);
 }
 const bool full=scenario=="root-full";std::array<GLuint,20> flows{};if(full){glGenBuffers(20,flows.data());for(auto b:flows){glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferData(GL_SHADER_STORAGE_BUFFER,10622*256,nullptr,GL_DYNAMIC_COPY);}}
 std::cout<<"root specialized programs linked=15\n";
 GLint alignment;glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&alignment);assert(alignment>0&&alignment%4==0);
 const uint32_t combinedWord=uint32_t((25600+alignment-1)/alignment*alignment/4),start=scenario=="root-offset"?7:0;
 std::vector<uint32_t> combined(combinedWord+10622*64),controls(16*128);GLuint extra[3];glGenBuffers(3,extra);
 for(unsigned i=0;i<3;++i){const auto& v=i==2?controls:combined;glBindBuffer(GL_SHADER_STORAGE_BUFFER,extra[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,v.size()*4,v.data(),GL_DYNAMIC_COPY);}

 const std::pair<const char*,uint32_t> quniforms[]={{"uWords",6400},{"uPad",256},{"uSandIndex",20000},{"uWaterIndex",20256},{"uArrivalIndex",20320},{"uEdges",20480},{"uFrozen",23000},{"uRows",23600},{"uReplayFrozen",24000},{"uSpare",24576},{"uAngles",ANGLES},{"uSourceEvents",LEDGER},{"uM3Role",1},{"uM3Ordinal",1},{"uM3BaseSourceLo",1},{"uM3ProposedPhysicsLo",2},{"uQuantumOrdinal",1},{"uGravity",120},{"uSettle",65536},{"uKWet",120},{"uSoft",605393},{"uM3WaterRate",8388608},{"uM3StateWords",6400},{"uM3Header",H},{"uM3Rows",10622},{"uM3CombinedEventWord",combinedWord},{"uM3SelectedBank",3},{"uM3BaseBank",1},{"uM3CandidateBank",0},{"uM3SpareBank",2},{"uM3CompareResult",SCRATCH+80},{"uM3ExpectedPasses",549},{"uM3ActualBarriers",550},{"uM3ActualEventStart",start},{"uM3Uploaded",0}};
 for(auto compiled:programs){program=compiled;glUseProgram(program);for(auto u:uniforms)uniform(u.first,u.second);for(auto u:quniforms)uniform(u.first,u.second);uniform("uM3FullBranchClear",full?1:0);uniform("uM3ScopeRows",10622);uniform("uM3ActualBarriers",full?555:550);}
 if(scenario=="root-clamped")for(auto compiled:programs){program=compiled;glUseProgram(program);uniform("uM3Omega",1966080);glUniform1i(glGetUniformLocation(program,"uM3Target"),7680);}
 using grs::m3a::Opcode;
 uint32_t index=0;
 for(const auto& pass:grs::m3a::kPasses192_64){
   program=programs[size_t(pass.opcode)];glUseProgram(program);
   if(pass.opcode!=Opcode::OCCUPIED_SCOPE_CORE)std::cout<<"root index="<<index<<" op="<<uint32_t(pass.opcode)<<" role="<<pass.role<<"\n";
   GLuint input=buffers[3],output=buffers[1],event=extra[2];
   const bool core=pass.opcode==Opcode::OCCUPIED_SCOPE_CORE;
   const bool scope=core||pass.opcode==Opcode::SCOPE_INIT||pass.opcode==Opcode::WATER_TARGET_40||pass.opcode==Opcode::WATER_MATCH_ORDERED||pass.opcode==Opcode::WATER_PROJECTION_AUDIT;
   if(scope){output=pass.role==4?buffers[1]:extra[1];event=pass.role==4?buffers[5]:extra[1];if(core&&(pass.nativeStage==22||pass.nativeStage==23))output=buffers[2];}
   if(pass.opcode==Opcode::SCOPE_FINALIZE){input=pass.role==4?extra[0]:extra[1];output=pass.role==4?buffers[1]:extra[0];event=pass.role==4?buffers[5]:extra[2];}
   if(pass.opcode==Opcode::SEMANTIC_COMPARE||pass.opcode==Opcode::PRIVATE_PROMOTE){input=extra[0];output=extra[1];event=buffers[1];}
   if(pass.opcode==Opcode::PACK_PUBLICATION||pass.opcode==Opcode::PUBLICATION_AUDIT)event=buffers[6];
   if(pass.opcode==Opcode::SEAL_COMPLETION)event=buffers[7];
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,input);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,output);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers[4]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,event);
   uniform("uM3Opcode",uint32_t(pass.opcode));uniform("uM3RootPass",index++);uniform("uM3Role",pass.role);uniform("uM3WriteBank",pass.role==4?0:4);uniform("uPass",pass.nativeStage);
   uniform("uEventStart",pass.role==4?start:0);uniform("uM3EventStart",pass.role==4?start:0);
   uniform("uEventCapacity",pass.role==4||pass.role==0?42680:10622);uniform("uM3EventCapacity",pass.role==4||pass.role==0?42680:10622);
   uniform("uM3EventWord",scope&&pass.role!=4?combinedWord:0);uniform("uSortK",pass.sortK);uniform("uSortJ",pass.sortJ);uniform("uColor",pass.color);
   glDispatchCompute((pass.count+63)/64,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);assert(glGetError()==GL_NO_ERROR);
   if(full&&pass.opcode==Opcode::SCOPE_FINALIZE){glBindBuffer(GL_COPY_READ_BUFFER,pass.role==4?buffers[5]:extra[1]);glBindBuffer(GL_COPY_WRITE_BUFFER,flows[pass.role-1]);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,(pass.role==4?start*64:combinedWord)*4,0,10622*256);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);assert(glGetError()==GL_NO_ERROR);}
   if(pass.opcode==Opcode::PRIVATE_PROMOTE){
     glBindBuffer(GL_COPY_READ_BUFFER,buffers[4]);glBindBuffer(GL_COPY_WRITE_BUFFER,extra[2]);
     glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,(SCRATCH+6144)*4,0,6*128*4);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);assert(glGetError()==GL_NO_ERROR);
   }
 }
 assert(index==549);
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[7]);auto sealed=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT));assert(sealed);
 std::cout<<"full QUANTUM count="<<sealed[97]<<" outcome="<<sealed[28]<<" reason="<<sealed[29]<<" pass="<<sealed[30]<<" ordinal="<<sealed[31]<<" audits="<<sealed[99]<<" selected="<<sealed[178]<<" status="<<sealed[179]<<" mismatch="<<sealed[101]<<" expected="<<sealed[102]<<" actual="<<sealed[103]<<"\n";
 const bool rootPassed=sealed[0]==1381192263&&sealed[1]==4&&sealed[3]==3235823838u&&sealed[28]==1&&sealed[97]==549&&sealed[99]==32767&&sealed[178]==(scenario=="root-clamped"?2u:1u)&&sealed[179]==(scenario=="root-clamped"?1u:0u)&&sealed[20]==2&&sealed[18]==1&&sealed[52]==start;
 assert(glUnmapBuffer(GL_COPY_READ_BUFFER));if(!rootPassed)return 4;
 if(full){for(unsigned role=0;role<5;++role){glBindBuffer(GL_COPY_READ_BUFFER,flows[role]);auto rows=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,10622*256,GL_MAP_READ_BIT));assert(rows);for(size_t i=(10622-1)*64;i<10622*64;++i)assert(rows[i]==0);if(role==0||role==3||role==4)assert(rows[0]!=0);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));}glDeleteBuffers(20,flows.data());std::cout<<"FULL_BRANCH five scope GPU copies/cleared capacity PASS\n";}
 for(auto compiled:programs)glDeleteProgram(compiled);program=0;glDeleteBuffers(3,extra);std::cout<<"T522 actual GPU full549 five-role QUANTUM/authority-latch/consistency/seal "<<scenario<<" PASS\n";
}
