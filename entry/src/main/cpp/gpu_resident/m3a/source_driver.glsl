// One workgroup per SOURCE slot. Native bindings are fixed by slot, never by a
// mapped trial result: CURRENT/candidate except slot 8 (candidate/SPARE).
uniform uint uM3ExpectedState;
const uint M3_ROOT_PRE_END=58u;
bool m3SourceTrialActive(){return W[0]==0u&&m3Get(M3_ROOT_STATE)==1u&&m3Get(M3_SOURCE_SUCCESS)==0u&&(m3Input(2u)==2u||uM3Trial==0u);}
void m3SourceCopy(){for(uint i=gl_LocalInvocationIndex;i<uM3Words;i+=64u)C[i]=B[i];memoryBarrierBuffer();barrier();}
void m3OrdinarySaveAudit(){
    if(!m3AuditTrialState()){m3SourceFail(15u,0u);return;}
    m3Put(M3_TRIAL_END_ROW,m3Get(M3_EVENT_CURSOR));m3Put(M3_TRIAL_ERROR,m3Get(M3_TRIAL_REASON));m3Put(M3_TRIAL_END_BIRTHS,m3Get(M3_TRIAL_BIRTHS));m3Put(M3_TRIAL_END_MERGES,m3Get(M3_TRIAL_MERGES));m3Put(M3_TRIAL_END_BORN,m3Get(M3_TRIAL_BORN));
}
void m3SourceSlot(){
    uint lane=gl_LocalInvocationIndex;
    if(uPass!=0u){
        if(lane==0u){if(m3Get(M3_COMPLETED_PASSES)!=uPass)m3SourceFail(28u,97u);m3Put(M3_COMPLETED_PASSES,m3Get(M3_COMPLETED_PASSES)+1u);}
        memoryBarrierBuffer();barrier();
    }
    if(uPass==0u){
        m3SourceCopy();
        for(uint i=lane;i<8192u;i+=64u)m3Put(i,0u);
        if(lane==0u)for(uint i=0u;i<16u;++i)W[i]=0u;
        memoryBarrierBuffer();barrier();if(lane==0u)m3Put(M3_COMPLETED_PASSES,1u);return;
    }
    if(uPass==1u){
        if(lane==0u){
            for(uint i=0u;i<256u;++i)m3Put(M3_BASE_HEADER+i,B[uHeader+i]);
            m3Put(M3_LAMBDA,65536u);
            if(!m3AuditInventory()){m3SourceFail(15u,0u);return;}
            if(m3DecodeSource())m3Put(M3_ROOT_STATE,1u);
        }return;
    }
    if(uPass>=2u&&uPass<66u){
        uint slot=(uPass-2u)%8u;
        bool enabled=m3SourceTrialActive(); // Uniform across this single group.
        if(!enabled)return;
        bool bulk=m3Input(2u)==2u;
        if(slot==0u){m3SourceCopy();if(lane==0u){if(bulk)m3TrialOpen();else m3OrdinaryInitialize();}return;}
        if(slot==1u){if(lane==0u&&bulk)m3TrialSolve();return;}
        if(slot==2u){if(bulk||m3Input(2u)==0u)m3BuildSeedIndex();return;}
        if(slot==3u){if(bulk||m3Input(2u)==0u)m3ExecuteSeeds(false);else if(lane==0u)m3OrdinaryWater();return;}
        if(slot==4u){if(lane==0u){if(bulk)m3TrialAudit();else m3OrdinarySaveAudit();}return;}
        if(slot==5u){m3SourceCopy();return;}
        if(slot==6u){
            if(lane==0u){
                if(bulk){m3TrialReplayBegin();m3TrialSolve();}
                else{m3Replay=true;m3Put(M3_EVENT_CURSOR,0u);m3Put(M3_SCOPE_ORDINAL,0u);m3OrdinaryInitialize();}
            }
            memoryBarrierBuffer();barrier();
            if(bulk||m3Input(2u)==0u)m3ExecuteSeeds();else if(lane==0u)m3OrdinaryWater();
            memoryBarrierBuffer();barrier();
            if(lane==0u){
                m3TrialReplayEnd();
                if(W[0]==0u&&!m3AuditTrialState())m3SourceFail(31u,0u);
                for(uint i=0u;i<uM3Words&&W[0]==0u;++i)if(B[i]!=C[i])m3SourceFail(31u,i);
            }return;
        }
        if(slot==7u&&lane==0u){if(bulk)m3TrialClose();else m3OrdinaryEnd();}return;
    }
    if(uPass==68u){if(W[0]==0u)m3SourceCopy();return;}
    if(lane!=0u)return;
    if(uPass==66u){
        m3Put(M3_ROOT_PRE_END,m3Get(M3_EVENT_CURSOR));m3FinalizeSource();
        if(W[0]==0u)for(uint i=0u;i<uM3Words;++i)W[uM3ExpectedState+i]=C[i];
    }
    else if(uPass==67u&&W[0]==0u){
        // Native binds CURRENT/SPARE here. Reconstruct root metadata and both
        // persistent ledgers independently, verifying rather than rewriting.
        m3RootReplay=true;m3Replay=true;m3Put(M3_EVENT_CURSOR,m3Get(M3_ROOT_PRE_END));
        m3FinalizeSource();m3Replay=false;m3RootReplay=false;
        if(W[0]==0u&&(!m3AuditInventory()||!m3AuditSourceEvents()||!m3AuditRawCommand()||!m3AuditSourceTransaction()))m3SourceFail(15u,0u);
        for(uint i=0u;i<uM3Words&&W[0]==0u;++i)if(C[i]!=W[uM3ExpectedState+i])m3SourceFail(31u,i);
        // All applicable root families have now been independently checked;
        // publication qualification remains unset until its separate audit.
        if(W[0]==0u)m3Put(M3_AUDIT_FAMILIES,32511u);
    }
    // Slot68 is a private native copy to ACTUAL, with no public bank swap.
    else if(uPass==69u)m3PackPublication();
    else if(uPass==70u&&W[0]==0u){if(!m3AuditPublication())m3SourceFail(15u,0u);else m3Put(M3_AUDIT_FAMILIES,m3Get(M3_AUDIT_FAMILIES)|256u);}
    else if(uPass==71u)m3SealSource();
}
