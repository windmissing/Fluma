"""Generate V5 physical-cut lineage writers and independent SPARE replay.
This builds production source only; it does not execute a test or the GPU.
"""
from pathlib import Path
import re
from assemble_source import ROOT,GPU,once,assemble as source

def assemble():
    fragments={};source(fragments)
    core=(GPU/'m3a/occupied_core.glsl').read_text(encoding='utf-8')
    prototypes='''
void v5RefreshAcceptedMedium();uint v5Cursor(uint);void v5SetCursor(uint,uint);
void v5ClearCertificate(uint);bool v5CertificateValid(uint,uvec2);
uint v5LandAllowed(uint,uint,uint);bool v5LandPatch(uint,uint,uint,uint,uint);bool v5SealCompound();
uint v5BedPrefix(uint,uint,uint);uint v5FragmentCount(uint,uint);bool v5ColorCredit(uint,uint);bool v5Credit(uint,uint);
'''
    core=once(core,'void state(uint ns,',prototypes+'\nvoid state(uint ns,')
    core=core.replace('e[1]=4u','e[1]=5u').replace('C[uHeader+1u]!=4u','C[uHeader+1u]!=5u').replace('C[uHeader+31u]!=15u','C[uHeader+31u]!=31u')
    core=once(core,'||col(4u,c)!=0u','')
    core=once(core,'if(ns==1u)W[8]++;',
              'if(ns==0u)v5RefreshAcceptedMedium();if(v5Cursor(4u)==0u){v5SetCursor(0u,W[1]);v5SetCursor(2u,W[1]);}\n if(ns==1u)W[8]++;')
    core=once(core,'void retire(uint ns,uint slot){','void retire(uint ns,uint slot){if(ns==0u)v5ClearCertificate(slot);')
    start=core.index('void land(');end=core.index('\nuint m3JClearance',start)
    core=core[:start]+'''void land(uint slot,uint c){
 uint q=min(C[slot],min(C[uHeader+54u],col(15u,c)));q=guard(q,c,c,true);q=v5LandAllowed(slot,c,q);if(W[0]!=0u)return;
 if(q==0u){if(v5LandPatch(slot,c,0u,W[1],7u))v5SealCompound();return;}
 uint old[16];before(0u,slot,old);uint a=col(2u,c),b=col(8u,c),t=C[uHeader+54u],l=col(15u,c);
 if(!v5LandPatch(slot,c,q,W[1]+1u,8u))return;
 charge(q,c,c,false);pc(2u,c,a+q);pc(10u,c,a+q);pc(8u,c,b-q);
 if(q==old[0])retire(0u,slot);else{C[slot]-=q;C[2u*uK+slot]=col(10u,c);C[4u*uK+slot]=0u;state(0u,slot,2u,1u,0u);}
 derived(c);emit(8u,0u,slot,c,c,old,a,a+q,b,b-q,t,l,0u,0u,0u,TICKS,TICKS,0u,256u+c,0u);v5SealCompound();
}
'''+core[end:]
    core=once(core,'||C[at(ns,5u,i)]<W[se+7u]||C[at(ns,5u,i)]-W[se+7u]>=W[se+8u]',
              '||(ns==0u?!v5CertificateValid(i,uvec2(B[uHeader+16u]+uint(B[uHeader+16u]!=0xffffffffu),B[uHeader+17u]+uint(B[uHeader+16u]==0xffffffffu))):(C[at(ns,5u,i)]<W[se+7u]||C[at(ns,5u,i)]-W[se+7u]>=W[se+8u]))')
    # Use checked addition for the permitted same-root proposed certificate.
    core=core.replace('uvec2(B[uHeader+16u]+uint(B[uHeader+16u]!=0xffffffffu),B[uHeader+17u]+uint(B[uHeader+16u]==0xffffffffu))','v5ProposedCommit()')
    core=once(core,'bool inventory(){','uvec2 v5ProposedCommit(){uvec2 n;if(!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(1u,0u),n))qfail(13u);return n;}\nbool inventory(){')
    core=once(core,'void bedPrefix(){','void bedPrefix(){if(!v5Credit(0u,0u))return;')
    core=once(core,'q=guard(min(q,C[uHeader+54u]),d,r,false);','q=v5BedPrefix(d,r,q);q=guard(min(q,C[uHeader+54u]),d,r,false);')
    core=once(core,'W[a+3u]=q;W[a+6u]',
              'uint fragments=v5FragmentCount(d,q);if(fragments>64u)return;W[a+9u]=v5Cursor(1u);W[a+12u]=fragments;if(!v5ColorCredit(1u,fragments))return;W[a+3u]=q;W[a+6u]')
    core=once(core,'void finishCandidate(){','void finishCandidate(){C[uHeader+35u]=uFrame.x;C[uHeader+36u]=uFrame.y;C[uHeader+37u]=C[uHeader+180u];')
    replay=(GPU/'m3a/occupied_replay.glsl').read_text(encoding='utf-8')
    replay=replay.replace('ev(row,1u)==4u','ev(row,1u)==5u')
    replay=once(replay,'request=replayGuard(min(request,rr(uHeader+54u)),from,to,false);',
                'request=v5BedPrefix(from,to,request);request=replayGuard(min(request,rr(uHeader+54u)),from,to,false);')
    replay=once(replay,'wc(f,from,a-q);','if(kind==9u&&!v5ReplayBed(from,to,q,row+1u))return false;wc(f,from,a-q);')
    replay=once(replay,'W[ledger+7u]!=0u&&old[5]>=W[ledger+7u]&&old[5]-W[ledger+7u]<W[ledger+8u]',
                '(ns==0u?v5CertificateValid(slot,v5ProposedCommit()):(W[ledger+7u]!=0u&&old[5]>=W[ledger+7u]&&old[5]-W[ledger+7u]<W[ledger+8u]))')
    replay=once(replay,'if(W[16]!=1u)return;',
                'if(W[16]!=1u)return;v5Replay=true;uint v5ReplayEnd=W[1],v5LiftCut=v5Scratch(V5_LIFT_START);bool v5LiftDone=false;if(v5LiftCut<uEventStart||v5LiftCut>v5ReplayEnd||!v5InputEnergy())return;')
    hook='''
  if(!v5LiftDone&&row==v5LiftCut){W[1]=v5LiftCut;if(!v5LiftLoop())return;uint afterLift=W[1];W[1]=v5ReplayEnd;v5LiftDone=true;row=afterLift-1u;continue;}
'''
    replay=once(replay,'for(uint row=uEventStart;row<W[1];row++){','for(uint row=uEventStart;row<W[1];row++){'+hook)
    replay=once(replay,'row<m3LevelCursor&&!frozen&&phase==0u,16u))return;continue;',
                'row<m3LevelCursor&&!frozen&&phase==0u,16u))return;v5SetCursor(0u,row+1u);v5SetCursor(2u,row+1u);continue;')
    replay=once(replay,'phase=2u;W[25]++;continue;',
                'phase=2u;W[25]++;v5SetCursor(0u,row+1u);v5SetCursor(2u,row+1u);continue;')
    replay=once(replay,'from,from,true);if(!requireQ(ns==0u',
                'from,from,true);allowed=v5LandAllowed(slot,from,allowed);if(!requireQ(ns==0u')
    replay=once(replay,'wc(2u,from,rc(2u,from)+q);',
                'if(!v5LandPatch(slot,from,q,row+1u,8u))return;wc(2u,from,rc(2u,from)+q);')
    replay=once(replay,'W[24]++;if(ns==1u)',
                'if(kind==8u&&!v5SealCompound())return;if(ns==0u)v5RefreshAcceptedMedium();v5SetCursor(0u,row+1u);v5SetCursor(2u,row+1u);W[24]++;if(ns==1u)')
    replay=once(replay,'if(!frozen)replayFreeze();',
                'if(!v5LiftDone){W[1]=v5LiftCut;if(!v5LiftLoop())return;W[1]=v5ReplayEnd;}\n if(!frozen)replayFreeze();')
    replay=once(replay,'W[28]=0u;for(uint ns=',
                'rw(uHeader+35u,uFrame.x);rw(uHeader+36u,uFrame.y);rw(uHeader+37u,rr(uHeader+180u));v5Replay=false;W[28]=0u;for(uint ns=')
    dispatch=(GPU/'m3a/occupied_dispatch.glsl').read_text(encoding='utf-8')
    dispatch=once(dispatch,'if(i<127u)edgeApply(i,true);','v5ApplyBedColor();')
    # A q0 LAND still has one auxiliary record bound to its actual CONTACT.
    # Independent replay consumes it immediately at that cut, not at the end.
    replay=once(replay,'if(kind==8u&&!v5SealCompound())return;',
    '''if(kind==7u&&ns==0u&&((st>>14u)&15u)==1u){uint allowed=v5LandAllowed(slot,from,replayGuard(min(C[slot],min(C[uHeader+54u],rc(15u,from))),from,from,true));if(allowed==0u){v5SetCursor(0u,row+1u);v5SetCursor(2u,row+1u);if(!v5LandPatch(slot,from,0u,row+1u,7u)||!v5SealCompound())return;}}
  if(kind==8u&&!v5SealCompound())return;''')
    event=(GPU/'m3a/water_event.glsl').read_text(encoding='utf-8').replace('expected[1u]=4u','expected[1u]=5u')
    event=once(event,'memoryBarrierBuffer();W[1]=cursor+1u;',
               'memoryBarrierBuffer();W[1]=cursor+1u;v5SetCursor(0u,W[1]);v5SetCursor(2u,W[1]);')
    completion=(GPU/'m3a/quantum_completion.glsl').read_text(encoding='utf-8').replace('E[1]=4u','E[1]=5u').replace('!=32767u','!=262143u')
    completion=once(completion,'memoryBarrierBuffer();E[3]=',
                    'for(uint i=0u;i<22u;++i)E[189u+i]=C[uHeader+161u+i];memoryBarrierBuffer();E[3]=')
    driver=(GPU/'m3a/quantum_driver.glsl').read_text(encoding='utf-8').replace('W[at+1u]!=4u','W[at+1u]!=5u').replace('M3_Q_AUDITS,32511u','M3_Q_AUDITS,261887u')
    driver=once(driver,'if(!m3AuditInventory()){qfail(15u);return;}m3PlanInitialize();',
                'if(!m3AuditInventory()){qfail(15u);return;}if(!v5EnvelopeValid()){qfail(37u);return;}m3PlanInitialize();')
    driver=once(driver,'if(uM3Opcode==M3_OP_CORE){',
    '''if(uM3Opcode>=15u){
        if(gid!=0u||!m3ScopeEnabled()||W[0]!=0u)return;
        if(uM3Opcode==15u)v5InputEnergy();else if(uM3Opcode==16u)v5LiftLoop();
        else if(!v5AuditState(true)||v5Cursor(4u)!=0u||v5Cursor(0u)!=v5Cursor(2u)||v5Cursor(1u)!=v5Cursor(3u))qfail(38u);
        return;
    }
    if(uM3Opcode==M3_OP_CORE){''')
    driver=once(driver,'m3RoleInitialize(uM3Role);','v5ClearScopeEntry();m3RoleInitialize(uM3Role);')
    # Control revision5 keeps the original semantic control operands.
    receipts=(GPU/'m3a/plan_receipts.glsl').read_text(encoding='utf-8').replace('row[1]=4u','row[1]=5u')
    receipts=once(receipts,'if(B[i]!=actual){qfail(16u);',
                  'if(i==uHeader+179u){if(actual<uV5ActualAuxStart){qfail(38u);return;}actual-=uV5ActualAuxStart;}if(B[i]!=actual){qfail(16u);')
    receipts=once(receipts,'m3Put(M3_AUTH_MATCH,1u);', '''
    uint auxRows=W[m3RoleAddress(selected)+17u];
    if(auxRows!=W[at+17u]){qfail(38u);return;}
    for(uint i=0u;i<auxRows*32u;++i){
        uint expected=B[uV5CombinedAuxWord+i],actual=E[uV5FrameAuxWord+uV5ActualAuxStart*32u+i];
        if(i%32u==3u){expected=actual=0u;}
        if(i%32u==6u){if(actual<uV5ActualAuxStart){qfail(38u);return;}actual-=uV5ActualAuxStart;}
        if(i%32u==7u&&actual!=0u){if(actual<uM3ActualEventStart){qfail(38u);return;}actual-=uM3ActualEventStart;}
        if(expected!=actual){qfail(38u);W[19]=i;return;}
    }
    m3Put(M3_AUTH_MATCH,1u);''')
    receipts=once(receipts,'for(uint i=0u;i<words;++i)C[uM3CombinedEventWord+i]=B[uM3CombinedEventWord+i];',
                  'for(uint i=0u;i<words;++i)C[uM3CombinedEventWord+i]=B[uM3CombinedEventWord+i];\n        for(uint i=0u;i<W[at+17u]*32u;++i)C[uV5CombinedAuxWord+i]=B[uV5CombinedAuxWord+i];')
    scope=(GPU/'m3a/scope_common.glsl').read_text(encoding='utf-8')
    scope=once(scope,'W[descriptor+M3_SCOPE_ROWS]=W[1]-uEventStart;',
               'W[descriptor+17u]=C[uHeader+179u]-(uM3Role==4u?uV5ActualAuxStart:0u);W[descriptor+18u]=uM3Role==4u?uV5ActualAuxStart:0u;W[descriptor+19u]=v5Scratch(V5_LIFT_START)-uEventStart;\n    W[descriptor+M3_SCOPE_ROWS]=W[1]-uEventStart;')
    semantic=(GPU/'m3a/semantic_compare.glsl').read_text(encoding='utf-8')
    semantic=once(semantic,'uint eventWords=m3SelectedRows()*64u;',
                  'uint eventWords=m3SelectedRows()*64u,auxWords=W[m3RoleAddress(m3PlanGet(M3_PLAN_SELECTED))+17u]*32u;')
    semantic=semantic.replace('max(uM3StateWords,eventWords)','max(uM3StateWords,max(eventWords,auxWords))')
    semantic=once(semantic,'if(word<uM3StateWords&&!m3RepresentationWord(word)&&B[word]!=C[word])',
                  'if(word<auxWords&&word%32u!=3u&&B[uV5CombinedAuxWord+word]!=C[uV5CombinedAuxWord+word])atomicMin(W[uM3CompareResult],2u*uM3StateWords+eventWords+word);\n    if(word<uM3StateWords&&!m3RepresentationWord(word)&&B[word]!=C[word])')
    semantic=once(semantic,'if(B[word]!=actual)atomicMin(',
                  'if(word==uM3Header+179u)actual=actual<uV5ActualAuxStart?0xffffffffu:actual-uV5ActualAuxStart;\n        if(B[word]!=actual)atomicMin(')
    semantic=once(semantic,'else{uint word=mismatch-uM3StateWords-m3SelectedRows()*64u;',
                  'else if(mismatch>=2u*uM3StateWords+m3SelectedRows()*64u){uint word=mismatch-2u*uM3StateWords-m3SelectedRows()*64u;m3SemanticFailure(mismatch,B[uV5CombinedAuxWord+word],C[uV5CombinedAuxWord+word]);}\n    else{uint word=mismatch-uM3StateWords-m3SelectedRows()*64u;')
    semantic=once(semantic,'m3SemanticFailure(mismatch,B[word],actual);}',
                  'if(word==uM3Header+179u)actual=actual<uV5ActualAuxStart?0xffffffffu:actual-uV5ActualAuxStart;m3SemanticFailure(mismatch,B[word],actual);}')
    shared=['lineage.glsl','lineage_audit.glsl','lift_numeric.glsl','lineage_events.glsl','lineage_material.glsl','lift_writer.glsl']
    # Material and lift helpers depend on WORK scratch declared by solvers.
    libraries='\n'.join((GPU/'m3b'/name).read_text(encoding='utf-8') for name in shared)
    parts=[core,'uniform uint uM3SourceEvents,uM3SourceMeta,uM3CandidateBank,uV5CombinedAuxWord,uV5FrameAuxWord;\n',fragments['m3a/occupied_solvers.glsl'],libraries,event,
           (GPU/'m3a/water_writer.glsl').read_text(encoding='utf-8'),fragments['m3a/inventory_audit.glsl'],
           replay,(GPU/'m3a/planner.glsl').read_text(encoding='utf-8'),
           scope,receipts,semantic,fragments['m3a/publication.glsl'],dispatch,completion,driver,
           'void main(){m3QuantumRoot();}\n']
    text='\n'.join(parts)
    (GPU/'m3b/quantum_v5.comp').write_text(text,encoding='utf-8')
    (GPU/'m3b/source_v5.comp').write_text(source().read_text(encoding='utf-8'),encoding='utf-8')
    return text

if __name__=='__main__':assemble()
