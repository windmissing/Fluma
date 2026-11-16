from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
DEST=ROOT/'entry/src/main/cpp/gpu_resident/m3b'
def edit(name, fn):
    p=DEST/name;p.write_text(fn(p.read_text(encoding='utf-8')),encoding='utf-8')
def one(text,a,b):
    assert text.count(a)==1,(a,text.count(a));return text.replace(a,b)
def patch():
    edit('source_queue_v5.h',lambda s:s.replace('words[1]!=4','words[1]!=5'))
    edit('shader_variant.h',lambda s:s.replace('char*,15>','char*,19>').replace('"#define M3_FIXED_OPCODE 14u\\n"','"#define M3_FIXED_OPCODE 14u\\n","#define M3_FIXED_OPCODE 15u\\n","#define M3_FIXED_OPCODE 16u\\n","#define M3_FIXED_OPCODE 17u\\n","#define M3_FIXED_OPCODE 18u\\n"'))
    def gate(s):
        for a,b in [('w[1]!=4','w[1]!=5'),('w[46]!=4','w[46]!=5'),('w[47]!=4','w[47]!=5'),('w[48]!=4','w[48]!=5'),('w[49]!=15','w[49]!=31'),('w[99]!=0x7fff','w[99]!=0x3ffff'),('w[174]!=7','w[174]!=15'),('i=189;i<256','i=211;i<256')]:s=s.replace(a,b)
        return one(s,'if(Pair(w,108)!=p.carry)return false;',
                   'if(w[189]!=1||w[190]>2||w[191]>65536||w[192]>65536||w[209]!=64||w[210]>128)return false;\n    if(Pair(w,197)<Pair(w,199)||Pair(w,197)-Pair(w,199)!=Pair(w,205))return false;\n    if(Pair(w,108)!=p.carry)return false;')
    edit('completion_gate.cpp',gate)
    def layout(s):
        return s.replace('const size_t events=size_t(p->scopeRows)*256;','const auto* v5=SelectLineageProfile(k,kw);if(!v5)return false;const size_t events=size_t(p->scopeRows)*256+size_t(v5->ea)*128;').replace('size_t(profile->quantumFrameRows)*256<=maximumBlockBytes','size_t(profile->quantumFrameRows)*256+size_t(SelectLineageProfile(profile->k,profile->kw)->ea)*4*128<=maximumBlockBytes')
    edit('work_layout.h',layout)
    def buffers(s):
        s=one(s,'for(size_t i=0;i<baseline.size();++i)next.bytes[i]=baseline[i];','for(size_t i=0;i<baseline.size();++i)next.bytes[i]=baseline[i];\n        for(size_t i=0;i<3;++i)next.bytes[i]=profile.stateBytes;')
        return s.replace('size_t(profile.quantumFrameRows)*256;','size_t(profile.quantumFrameRows)*256+size_t(SelectLineageProfile(profile.k,profile.kw)->ea)*4*128;')
    edit('buffer_plan.h',buffers)
    def transport_header(s):
        s=s.replace('std::array<Program,16>','std::array<Program,20>')
        s=one(s,'PhysicalConfigV5 config_;','m2::Profile oldStorage_{};uint32_t headerWord_=0;\n PhysicalConfigV5 config_;')
        s=one(s,'#include "source_queue_v5.h"','#include "source_queue_v5.h"\n#include "lift_input.h"')
        s=one(s,'bool omegaProvided=false;','bool omegaProvided=false;\n LiftParameters lift;')
        s=one(s,'return omegaProvided&&','return lift.Valid()&&omegaProvided&&')
        return s
    edit('transport.h',transport_header)
    def transport(s):
        s=one(s,'if(!old_)return TransportResultV5::INVALID;config_=config;',
        '''if(!old_)return TransportResultV5::INVALID;config_=config;
 const auto extension=uint32_t(SelectLineageProfile(p.k,p.kw)->stateBytes-SelectLineageProfile(p.k,p.kw)->oldStateBytes)/4;
 oldStorage_=*old_;headerWord_=old_->stateWords-256;
 auto move=[&](m2::WordRange& r){r.begin+=extension;r.end+=extension;};
 move(oldStorage_.sourceLedger);move(oldStorage_.index0);move(oldStorage_.index1);move(oldStorage_.control);move(oldStorage_.sourceOrigins);move(oldStorage_.config);move(oldStorage_.quantumOrigins);
 oldStorage_.sourceBytes+=extension*4;oldStorage_.quantumBytes+=extension*4;oldStorage_.stateWords+=extension;old_=&oldStorage_;''')
        s=s.replace('n<16','n<20').replace('p<16','p<20').replace('FEATURES15','FEATURES31')
        s=s.replace('const uint32_t h=uint32_t(state.size())-256;','const uint32_t h=headerWord_;')
        s=s.replace('state[h+1]=4','state[h+1]=5').replace('1,1,1,3,4,4,4,15,0','1,1,1,3,5,5,5,31,0').replace('state[h+146]=7','state[h+146]=15')
        s=one(s,'state[h+160]=1;','state[h+160]=1;state[h+161]=1;state[h+162]=uint32_t(config.lift.mode);state[h+181]=64;')
        s=s.replace('uint32_t(p.stateBytes/4)-256','headerWord_').replace('old_->stateWords-256','headerWord_')
        # The original immutable header remains before the appended V5 fields.
        s=s.replace('headerWord_=headerWord_;','headerWord_=old_->stateWords-256;')
        anchor='Pair("uNonce",permit_.nonce);'
        setup='''const auto& v5=*SelectLineageProfile(p.k,p.kw);
 U("uV5Cohort",uint32_t(v5.extension[0].begin/4));U("uV5Depth",uint32_t(v5.extension[1].begin/4));U("uV5Certificate",uint32_t(v5.extension[2].begin/4));U("uV5Aux",uint32_t(v5.extension[3].begin/4));
 const auto origin=uint32_t((source?p.sourceWorkBytes-32768-256:p.quantumWorkBytes-32768-512)/4);
 U("uV5OriginScratch",origin);U("uV5SourceLedger",source?old_->sourceLedger.begin:ledger_);U("uV5SourceMeta",uint32_t((source?p.source[1].begin:p.quantum[0].begin)/4));
 U("uV5Cursor",origin+8192);U("uV5Input",origin+8192+64);U("uV5Mode",uint32_t(config_.lift.mode));U("uV5Rate",config_.lift.liftRateQ16);U("uV5Band",config_.lift.bandQ);U("uV5Ramp",config_.lift.rampQ16);U("uV5Decay",config_.lift.decayQ16);U("uV5Speed",config_.lift.liftSpeedQ);U("uV5Channel",config_.lift.liftCapQ);
 '''
        s=one(s,anchor,setup+anchor)
        s=s.replace('const bool scope=core||','const bool scope=uint32_t(pass.opcode)>=15||core||')
        s=one(s,'U("uM3RootPass",index);',
              'U("uV5AuxWord",scope&&pass.role!=4?uint32_t(layout_.combinedEventOffset/4)+profile_->scopeRows*64:profile_->quantumFrameRows*64);U("uV5AuxCapacity",SelectLineageProfile(profile_->k,profile_->kw)->ea*(pass.role==4?4:1));\n U("uM3RootPass",index);')
        return s
    edit('transport.cpp',transport)
    edit('scene.cpp',lambda s:s.replace('ABI_V5_MEDIUM3','ABI_V5_MEDIUM3'))

if __name__=='__main__':patch()
