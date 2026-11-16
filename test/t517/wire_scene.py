from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
D=ROOT/'entry/src/main/cpp/gpu_resident/m3b'
def edit(name,a,b):
 p=D/name;s=p.read_text(encoding='utf-8');assert s.count(a)==1,(name,a,s.count(a));p.write_text(s.replace(a,b),encoding='utf-8')
edit('scene_source.h','virtual ControlCode ReleaseV5(', 'virtual GLuint StateBufferV5(const PublicationLease&)const=0;\n virtual ControlCode ReleaseV5(')
edit('controller.h','ControlCode ReleaseV5(', 'GLuint StateBufferV5(const PublicationLease&)const override;\n ControlCode ReleaseV5(')
p=D/'controller.cpp';s=p.read_text(encoding='utf-8');at=s.rindex('}');s=s[:at]+'''GLuint ControllerV5::StateBufferV5(const PublicationLease& lease)const{
 std::lock_guard<std::mutex> lock(mutex_);if(!lease.leased||lease.bank!=owner_.currentBank||Whole(lease.generation)!=owner_.publicationGeneration)return 0;
 return transport_.StateBuffer(lease.bank);
}
'''+s[at:];p.write_text(s,encoding='utf-8')
edit('scene.cpp','stage="GRS_SCENE_PARTICLES";Geometry(particles,f);',
     'stage="GRS_SCENE_PARTICLES";Geometry(particles,f);const auto stateBuffer=source.StateBufferV5(lease);if(!stateBuffer){r.stage="V5_STATE_LEASE_REJECTED";return r;}glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,stateBuffer);glUniform1ui(uniforms[1].Find("uK"),header[41]+header[43]);')
edit('scene.cpp','|aX:float@0|aY:float@1|aMass:uint@2|aState:uint@3:optional|aRadius:float@4|',
     '|AcceptedState:std430:uint[]@1|uK:uint|aX:float@0|aY:float@1|aMass:uint@2|aState:uint@3:optional|aRadius:float@4|')
p=D/'kernels.cmake';s=p.read_text(encoding='utf-8').replace('m0_scene_particle.vert|','m3b/particle.vert|').replace('m0_scene_particle.frag|','m3b/particle.frag|');p.write_text(s,encoding='utf-8')
edit('transport.cpp','GLint alignment=0,bindings=0,blocks=0,fragment=0,invocations=0,shared=0;',
     'GLint vertexBlocks=0;glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,&vertexBlocks);if(vertexBlocks<1)return TransportResultV5::UNSUPPORTED;\n GLint alignment=0,bindings=0,blocks=0,fragment=0,invocations=0,shared=0;')
