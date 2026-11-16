"""Independent SPARE replay for mediumRev3; expected state is an old WORK
snapshot, not a copy used as replay operands. Native slot22 restores CURRENT.
"""
from generate_occupied_core import ROOT,GPU,read,once
import re
def generate():
    text=read('m1/quantum_replay.glsl')
    text=once(text,'uint rr(uint a){return W[uSpare+a];}void rw(uint a,uint v){W[uSpare+a]=v;}',
                   'uint rr(uint a){return C[a];}void rw(uint a,uint v){C[a]=v;}')
    text=once(text,'return E[row*64u+field];','return E[uM3EventWord+row*64u+field];')
    text=once(text,'void rd(uint c){wc(10u,c,rc(2u,c));wc(12u,c,rc(10u,c)+rc(6u,c));wc(13u,c,rc(12u,c)+rc(7u,c)+rc(8u,c));}',
        'void rd(uint c){wc(10u,c,rc(2u,c));wc(11u,c,rc(10u,c)+rc(8u,c));wc(12u,c,rc(11u,c)+rc(6u,c));wc(13u,c,rc(12u,c)+rc(7u,c));}')
    start=text.index('bool quantumAudit(){');end=text.index('bool replayField(',start)
    audit='''bool quantumAudit(){
 if(!requireQ(m3AuditInventory(),15u))return false;
 for(uint c=0u;c<128u;++c){
  if(!requireQ(col(14u,c)<=244667u&&col(15u,c)==244667u-col(14u,c),15u))return false;
  if(c<127u&&!requireQ(abs(int(col(10u,c+1u))-int(col(10u,c))-int(W[5]))<=605394,14u))return false;
 }
 if(!requireQ(C[uHeader+54u]<=2621440u&&C[uHeader+55u]==0u,15u))return false;
 W[16]=1u;W[18]=127u;return true;
}
uint replayJRoom(uint c){
 uint cap=CQ-rc(7u,c);
 for(uint i=0u;i<uKw;++i)if(rr(uWater+i)!=0u&&(rr(uWater+uKw+i)>>20u)==c){uint y=rr(uWater+2u*uKw+i);cap=min(cap,y==0u?0u:y-1u);}
 return cap>rc(12u,c)?cap-rc(12u,c):0u;
}
'''
    prefix=text[:start]+audit
    body=text[end:]
    # Only candidate comparisons read expected WORK. rr/rw remain SPARE.
    body=re.sub(r'C\[([^\]]+)\]',r'W[uSpare+\1]',body)
    body=once(body,'(st>>22u)==0u','((st>>22u)&7u)==uM3Role&&(st>>25u)==0u')
    body=body.replace('st==(2u<<18u)','st==((2u<<18u)|(uM3Role<<22u))')
    body=once(body,'W[ledger+4u]==ns','(W[ledger+4u]==1u?1u:0u)==ns')
    body=once(body,'kind>=3u&&kind<=14u','((kind>=3u&&kind<=14u)||kind==19u)')
    body=once(body,'ev(row,1u)==2u','ev(row,1u)==4u')
    body=once(body,'kind==9u?3u:','kind==9u||kind==19u?3u:')
    body=once(body,'uvec2 mask=m1Masks[kind];','if(kind==19u){if(!requireQ(row<m3LevelCursor&&!frozen&&phase==0u,16u))return;continue;}\n  uvec2 mask=m1Masks[kind];')
    body=once(body,'W[ledger+4u]==ns','(W[ledger+4u]==1u?1u:0u)==ns') if 'W[ledger+4u]==ns' in body else body
    body=once(body,'m<=CQ-rc(13u,to)&&ev(row,23u)==m','m<=CQ-rc(13u,to)&&(ns!=0u||m<=replayJRoom(to))&&ev(row,23u)==m')
    body=once(body,'min(CQ-rc(13u,to),min(rc(15u,from),rc(15u,to)))','min(min(CQ-rc(13u,to),replayJRoom(to)),min(rc(15u,from),rc(15u,to)))')
    body=once(body,'bool frozen=false;uint phase=0u;', '''m3LevelReplay=true;m3LevelCursor=uEventStart;
 if(!m3WaterTarget(int(W[5]))||!m3WaterMatch()||!m3WaterProjectionAudit()){qfail(30u);return;}
 m3LevelReplay=false;bool frozen=false;uint phase=0u;''')
    geometry=read('m1/quantum_geometry_audit.glsl')
    geometry=once(geometry,'capacity=old[0]>CQ-rc(13u,to)','capacity=old[0]>CQ-rc(13u,to)||(ns==0u&&old[0]>replayJRoom(to))')
    (GPU/'m3a/occupied_replay.glsl').write_text((prefix+body+'\n'+geometry).replace('uRequestedAngle','m3ScopeAngle'),encoding='utf-8')
if __name__=='__main__':generate()
