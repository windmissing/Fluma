"""Attach the independent input64 to the V5 controller/owner thread."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
D=ROOT/'entry/src/main/cpp/gpu_resident/m3b'
def edit(name,fn):
 p=D/name;p.write_text(fn(p.read_text(encoding='utf-8')),encoding='utf-8')
def one(s,a,b):
 assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def generate():
 edit('protocol_generated.h',lambda s:one(s,'inline constexpr uint32_t kColumnCapacity=', 'inline constexpr uint32_t kQ=1048576,kChannelBitmap=15;\ninline constexpr uint32_t kColumnCapacity='))
 edit('progress_clock_v3.cpp',lambda s:s.replace('packet.words[1]!=4','packet.words[1]!=5'))
 edit('publication_lease.h',lambda s:s.replace('revision=4,features=15','revision=5,features=31').replace('token.revision!=4','token.revision!=5').replace('token.features!=15','token.features!=31'))
 edit('publication_reader.h',lambda s:s.replace('words[1]!=4','words[1]!=5').replace('i=65;i<256','i=71;i<256').replace('1,1,1,3,4,4,4,15,0','1,1,1,3,5,5,5,31,0'))
 edit('scene.cpp',lambda s:s.replace('header[1]!=4','header[1]!=5').replace('header[31]!=15','header[31]!=31'))
 def header(s):
  s=one(s,'Words256 tail_{};', 'LiftInput64 liftInput_{};uint64_t auxFrame_=0;uint32_t auxEnd_=0,auxStart_=0;\n Words256 tail_{};')
  s=one(s,'bool Busy()const', 'void SetLiftInput(const LiftInput64& words){liftInput_=words;}\n GLuint StateBuffer(uint32_t bank)const{return bank<2?buffers_[bank]:0;}\n bool Busy()const')
  return s
 edit('transport.h',header)
 def transport(s):
  s=one(s,'evidence_={};tail_={};','auxStart_=permit.frame==auxFrame_?auxEnd_:0;\n evidence_={};tail_={};')
  s=one(s,'U("uV5Cursor",origin+8192);','U("uV5EventArena",permit_.eventArena);U("uV5ActualAuxStart",auxStart_);U("uV5Cursor",origin+8192);')
  s=one(s,'const auto previous=uploadCounter_,program=activeProgram_;','const auto previous=uploadCounter_,program=activeProgram_;')
  s=s.replace('uploadCounter_=kind==1?2176:0','uploadCounter_=kind==1?2176:256').replace('uploadCounter_=permit.kind==1?2176:0','uploadCounter_=permit.kind==1?2176:256')
  s=one(s,'}else{\n  Copy(buffers_[15],',
       '}else{\n  const auto inputAt=profile_->quantumWorkBytes-256;glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[16]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(inputAt),256,liftInput_.data());\n  Copy(buffers_[15],')
  s=one(s,'currentBank_=permit.candidateBank;','if(permit.kind==2){auxFrame_=permit.frame;auxEnd_=tail[207];}\n currentBank_=permit.candidateBank;')
  # FULL cannot be enabled while the prefix's self-referential SHA contract is
  # unresolved. It is explicitly rejected, never exported as a complete file.
  s=one(s,'if(busy_||ready_||!config.Valid()', 'if(config.fullBranchTrace)return TransportResultV5::UNSUPPORTED;\n if(busy_||ready_||!config.Valid()')
  return s
 edit('transport.cpp',transport)
 def controller_header(s):
  s=one(s,'ProgressClockV3 clock_;','ProgressClockV3 clock_;LiftInputGate liftInput_;LiftParameters liftParameters_;')
  s=one(s,'ControlResult Pulse(', 'InputResult OfferLiftInput(const LiftInput64&,uint64_t receipt);\n ControlResult ResumeLiftSession(const ControlIdentityV5&);\n ControlResult Pulse(')
  return s
 edit('controller.h',controller_header)
 def controller(s):
  s=one(s,'clock_.ready=clock_.uiFresh=clock_.gravityFresh=true;configured_=true;',
        'liftParameters_=config.lift;const auto inputResult=liftInput_.Configure({id.inputEpoch,id.pageSession,id.owner,id.run,id.context},config.lift,config.lift.mode==LiftMode::SYNTHETIC_TEST);if(inputResult!=InputResult::OK)return ControlResult::INVALID_ARGUMENT;\n clock_.uiFresh=true;clock_.ready=clock_.gravityFresh=config.lift.mode==LiftMode::OFF;configured_=true;')
  s=one(s,'SourcePacketV5 packet;bool source=false;', 'LiftInput64 frozen{};SourcePacketV5 packet;bool source=false;')
  s=one(s,'source=run_.sourcePhase;',
        'const LiftInput64* input=nullptr;if(liftInput_.Freeze(clock_.grant.id,now,input)!=InputResult::OK){clock_.gravityFresh=false;owner_.RevokeLocked({ControlResult::STALE_REJECTED});return ControlResult::STALE_REJECTED;}frozen=*input;\n  source=run_.sourcePhase;')
  s=one(s,'const auto started=transport_.Begin(', 'transport_.SetLiftInput(frozen);\n const auto started=transport_.Begin(')
  s=s.replace('sceneHeader_[1]=4','sceneHeader_[1]=5').replace('sceneHeader_[31]=15','sceneHeader_[31]=31')
  append='''
InputResult ControllerV5::OfferLiftInput(const LiftInput64& raw,uint64_t receipt){
 std::lock_guard<std::mutex> lock(mutex_);if(!configured_||!clock_.open)return InputResult::STALE_INPUT;
 const auto result=liftInput_.Offer(raw,receipt);
 if(result!=InputResult::OK){clock_.gravityFresh=false;owner_.RevokeLocked({ControlResult::STALE_REJECTED});if(coldEpoch_!=UINT64_MAX)++coldEpoch_;return result;}
 if(!clock_.ready){clock_.ready=true;clock_.baseline=false;}clock_.gravityFresh=true;return result;
}
ControlResult ControllerV5::ResumeLiftSession(const ControlIdentityV5& id){
 std::lock_guard<std::mutex> lock(mutex_);const auto old=clock_.identity;
 if(!configured_||run_.active||run_.jobLive||!run_.workspaceRetired||owner_.HasRootLocked())return ControlResult::WOULD_BLOCK;
 if(id.run!=old.run||id.owner!=old.owner||id.context!=old.context||id.surface!=old.surface||id.window!=old.window||id.config!=old.config||id.permission!=old.permission||id.inputEpoch<=old.inputEpoch||id.pageSession<=old.pageSession)return ControlResult::STALE_REJECTED;
 if(clock_.sourceControlGeneration==UINT64_MAX)return ControlResult::QUOTA_OVERFLOW;
 const auto result=clock_.BeginSessionLocked({id});if(result!=ControlResult::OK)return result;
 ++clock_.sourceControlGeneration;queue_=SourceQueueV5{};
 if(liftInput_.Configure({id.inputEpoch,id.pageSession,id.owner,id.run,id.context},liftParameters_,liftParameters_.mode==LiftMode::SYNTHETIC_TEST)!=InputResult::OK)return ControlResult::INVALID_ARGUMENT;
 clock_.ready=clock_.gravityFresh=liftParameters_.mode==LiftMode::OFF;clock_.uiFresh=true;clock_.baseline=false;return ControlResult::OK;
}
'''
  s=s[:s.rindex('}')]+append+s[s.rindex('}'):]
  return s
 edit('controller.cpp',controller)
generate()
