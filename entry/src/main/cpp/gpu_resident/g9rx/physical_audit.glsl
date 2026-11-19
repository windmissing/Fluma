// T647: bounded certificates for the actual enabled 14 scopes, no new dispatch.
uint dmScopeId(){return uRole==1u?uint(findMSB(uJ)):uRole==2u?6u:uRole==3u?7u+uint(findMSB(uJ)):13u;}
bool dmMicroOp(out uint bit){
 bit=0u;
 if(uOp==G9_OP_DM_REQUEST)bit=0u;else if(uOp==G9_OP_DM_APPLY)bit=1u;else if(uOp==G9_OP_FLIGHT_PATH)bit=2u;else if(uOp==G9_OP_PARTICLE_FOLD)bit=3u;else if(uOp==G9_OP_PREDICTOR_FACE)bit=4u;else if(uOp==G9_OP_MIDPOINT_BUILD)bit=5u;else if(uOp==G9_OP_CORRECTOR_FACE)bit=6u;else if(uOp==G9_OP_FLOW_ORIGIN_RESERVE)bit=7u;else if(uOp==G9_OP_FLOW_PAIR_APPLY)bit=8u;else if(uOp==G9_OP_FRICTION_REST_DEPOSIT)bit=9u;else if(uOp==G9_OP_ORDERED_CAPTURE)bit=10u;else if(uOp==G9_OP_MICRO_AUDIT_SEAL)bit=11u;else return false;return true;
}
void dmBeginScope(){
 uint id=dmScopeId();if(id>=14u||g9Ctl(127u)!=1u||(g9Ctl(121u)&(1u<<id))!=0u){g9Fault(25u,id,120u);return;}
 g9Set(121u,g9Ctl(121u)|(1u<<id));g9Set(124u,id);g9Set(125u,4095u);g9Set(126u,0u);
 for(uint c=0u;c<128u;++c)for(uint f=0u;f<48u;++f)dmPut(c,f,0u);
}
bool dmStageEnter(){
 uint bit;if(!dmMicroOp(bit))return true;uint visited=g9Ctl(126u);
 if(bit==0u){uint completed=dmWord(0u,46u)>>16u;if(uMicro!=completed||(uMicro!=0u&&visited!=4095u)){g9Fault(25u,0u,uMicro);return false;}visited=0u;}
 if(visited!=((1u<<bit)-1u)){g9Fault(25u,visited,bit);return false;}g9Set(126u,visited|(1u<<bit));return true;
}
bool dmMicroCertificate(){
 if(g9Ctl(126u)!=4095u||uJ==0u||32u%uJ!=0u)return false;
 for(uint c=0u;c<128u;++c){uint flags=dmWord(c,46u);if((flags&15u)!=15u||(flags>>16u)!=uMicro||dmWord(c,44u)!=1u||dmWord(c,47u)!=0u)return false;dmPut(c,46u,(flags&65535u)|((uMicro+1u)<<16u));}
 return true;
}
void dmSealScope(){
 if(g9Ctl(0u)!=0u)return;uint reason=g9Ctl(14u),id=g9Ctl(124u);bool valid=id==dmScopeId()&&id<14u;
 valid=valid&&g9Ctl(6u)==g9Ctl(7u)&&g9Ctl(8u)==g9Ctl(9u)&&g9Ctl(10u)==0u;
 if(reason==0u){valid=valid&&g9Ctl(126u)==4095u;for(uint c=0u;c<128u;++c)valid=valid&&(dmWord(c,46u)>>16u)==uJ;}
 else valid=valid&&reason<=18u&&g9Ctl(2u)!=0u; // exact first rejected legal prefix, not fictitious later stages
 if(!valid){g9Fault(25u,id,126u);return;}
 g9Set(122u,g9Ctl(122u)|(1u<<id));g9Set(123u,g9Ctl(123u)|(1u<<id));
}
uint dmExpectedScopes(){uint selected=g9Ctl(11u);if(selected==0u)return 63u|(63u<<7u);uint k=uint(findMSB(selected));return ((1u<<(k+1u))-1u)|((1u<<k)-1u)<<7u|64u|8192u;}
uint dmRootAudit(){
 if(g9Ctl(0u)!=0u)return 0u;uint mask=0u;
 bool identity=g9ConfigValid()&&g9ValidateInput(uRootKind);if(identity)mask|=1u;
 bool mass=g9AuditMassGeometry();if(mass)mask|=62u; // actual mass, origin/map, slot certificate and geometry predicates
 if(g9AuditMomentum())mask|=64u;
 bool prefix=g9Ctl(6u)==g9Ctl(7u)&&g9Ctl(8u)==g9Ctl(9u)&&g9Ctl(10u)==0u&&g9AuditStream();if(prefix)mask|=16384u;
 bool source=uRootKind==1u,chosen=g9Ctl(11u)!=0u;
 bool certified=source;
 if(!source){uint expected=dmExpectedScopes();g9Set(120u,expected);certified=g9Ctl(127u)==1u&&g9Ctl(121u)==expected&&g9Ctl(122u)==expected&&g9Ctl(123u)==expected;}
 // Energy/work/CFL/rest were checked by the visited producer chain, including legitimate rejected prefixes.
 if(certified&&g9Ctl(0u)==0u)mask|=128u|256u|512u|1024u|2048u;
 bool choice=source;if(!source){choice=true;bool found=false;for(uint i=0u;i<6u;++i){uint at=uSummary+i*64u;if(W[at]==0u){if(!found)choice=false;continue;}if(W[at]!=(1u<<i)||(W[at+1u]!=1u&&W[at+1u]!=4u))choice=false;if(W[at+1u]==1u){if(found||(1u<<i)!=g9Ctl(11u))choice=false;found=true;}else if(W[at+2u]==0u||W[at+2u]>18u)choice=false;}choice=choice&&(found==chosen);}
 if(choice)mask|=4096u;
 bool replay=source?g9Ctl(0u)==0u:certified&&(!chosen||(g9Ctl(24u)==1u&&g9Ctl(25u)==1u));if(replay)mask|=8192u;
 bool candidate=identity&&mass&&prefix&&choice&&replay&&g9Ctl(0u)==0u;if(candidate)mask|=32768u;
 if(!source&&certified&&candidate)mask|=196608u;
 return mask;
}

uint dmScopeAudit(){
 if(g9Ctl(0u)!=0u||g9Ctl(14u)!=0u)return 0u;
 uint mask=g9ConfigValid()&&g9ValidateInput(uRootKind)?1u:0u;
 bool mass=g9AuditMassGeometry(),momentum=g9AuditMomentum(),prefix=g9AuditStream()&&g9Ctl(6u)==g9Ctl(7u)&&g9Ctl(8u)==g9Ctl(9u)&&g9Ctl(10u)==0u;
 if(mass)mask|=62u;if(momentum)mask|=64u;if(prefix)mask|=16384u;
 bool chain=g9Ctl(126u)==4095u;
 for(uint c=0u;c<128u;++c)chain=chain&&(dmWord(c,46u)&15u)==15u&&(dmWord(c,46u)>>16u)==uJ&&dmWord(c,44u)==1u;
 if(chain&&mass&&momentum&&prefix)mask|=128u|256u|512u|1024u|2048u|196608u;
 return mask;
}
