// Independent per-trial index/state replay is sequenced by the source driver.
// Scratch fields 48..63 retain one candidate descriptor, not another state copy.
const uint M3_TRIAL_START_ROW=48u,M3_TRIAL_END_ROW=49u,M3_TRIAL_ERROR=50u,M3_TRIAL_END_BIRTHS=51u,M3_TRIAL_END_MERGES=52u,M3_TRIAL_END_BORN=53u;
const uint M3_TRIAL_EXPECTED_LAMBDA=54u,M3_SOURCE_SUCCESS=55u,M3_REPLAY_DONE=56u,M3_ATTEMPTS=57u;
void m3TrialOpen(){
    if(W[0]!=0u||m3Get(M3_SOURCE_SUCCESS)!=0u||m3Get(M3_ROOT_STATE)!=1u)return;
    uint offered=0u;
    for(uint c=0u;c<128u;++c){uint hi,lo;umulExtended(m3Get(M3_REQUESTED+c),m3Get(M3_LAMBDA),hi,lo);offered+=(hi<<16u)|(lo>>16u);}
    // A zero quantized offer terminates before an actual trial begins. It must
    // not leave an unpaired BEGIN or invent a non-contract trial reason.
    if(offered==0u){m3Put(M3_SOURCE_STOP,4u);m3Put(M3_ROOT_STATE,2u);return;}
    uvec2 epoch=uvec2(m3Get(M3_BRANCH_LO),m3Get(M3_BRANCH_HI)),next;
    if(!grsAdd64(epoch,uvec2(1u,0u),next)){m3SourceFail(13u,152u);return;}
    m3Put(M3_BRANCH_LO,next.x);m3Put(M3_BRANCH_HI,next.y);
    m3Put(M3_ATTEMPTS,uM3Trial+1u);
    m3Put(M3_TRIAL_START_ROW,m3Get(M3_EVENT_CURSOR));m3Put(M3_SCOPE_ORDINAL,0u);m3Put(M3_TRIAL_REASON,0u);
    m3Put(M3_TRIAL_BIRTHS,0u);m3Put(M3_TRIAL_MERGES,0u);m3Put(M3_TRIAL_BORN,0u);m3Put(M3_SEED,0u);m3Put(M3_REPLAY_DONE,0u);
    uint row[128];m3SourceRow(row,M3_TRIAL_BEGIN,0u);row[43]=m3Get(M3_LAMBDA);m3SourceEmit(row);
}
void m3TrialSolve(){
    if(W[0]!=0u||m3Get(M3_SOURCE_SUCCESS)!=0u||m3Get(M3_ROOT_STATE)!=1u)return;
    if(!m3CaptureColumns()){m3SourceFail(15u,0u);return;}
    uint lambda=m3Get(M3_LAMBDA),result=m3TrialEta(lambda);
    // Translate the solver's internal finite status into T497 trialReasons.
    if(result!=0u){
        if(result==1u){m3Put(M3_SOURCE_STOP,4u);m3Put(M3_ROOT_STATE,2u);return;}
        m3Put(M3_TRIAL_REASON,result-1u);m3Put(M3_NEXT_LAMBDA,result==2u?m3Get(M3_LAMBDA):lambda/2u);m3Put(M3_LAMBDA,lambda);return;
    }
    for(uint c=0u;c<128u;++c){
        uint row[128];m3SourceRow(row,M3_ETA_COLUMN,c);
        row[32]=col(6u,c);row[33]=m3Get(M3_TARGET_WATER+c);row[34]=m3Get(M3_BASE_MATERIAL+c);row[35]=row[34]+m3Get(M3_ADMITTED+c);row[36]=m3Get(M3_BASE_ETA+c);row[37]=row[35]+row[33];row[38]=m3Get(M3_CAP+c);row[39]=m3Get(M3_ETA_HI);row[40]=0u;row[41]=m3Get(M3_REQUESTED+c);row[42]=m3Get(M3_ADMITTED+c);
        // Compute the recorded ascending residual grant from the two endpoints.
        if(row[39]>0u){uint lo=clamp(row[36]+row[39]-1u,row[35],row[38])-row[35];row[40]=row[33]-lo;}
        m3SourceEmit(row);if(W[0]!=0u)return;
        C[uColumn+6u*128u+c]=row[33];m3SourceDerived(c);
    }
}
void m3TrialAudit(){
    if(W[0]!=0u||m3Get(M3_SOURCE_SUCCESS)!=0u||m3Get(M3_ROOT_STATE)!=1u)return;
    uint reason=m3Get(M3_TRIAL_REASON);
    if(!m3AuditTrialState()){m3SourceFail(15u,0u);return;}
    if(reason==0u&&m3Get(M3_TRIAL_BORN)!=m3Get(M3_WANT)){m3SourceFail(15u,23u);return;}
    m3Put(M3_TRIAL_END_ROW,m3Get(M3_EVENT_CURSOR));m3Put(M3_TRIAL_ERROR,reason);m3Put(M3_TRIAL_END_BIRTHS,m3Get(M3_TRIAL_BIRTHS));m3Put(M3_TRIAL_END_MERGES,m3Get(M3_TRIAL_MERGES));m3Put(M3_TRIAL_END_BORN,m3Get(M3_TRIAL_BORN));m3Put(M3_TRIAL_EXPECTED_LAMBDA,m3Get(M3_LAMBDA));
}
void m3TrialReplayBegin(){
    m3Replay=true;m3Put(M3_EVENT_CURSOR,m3Get(M3_TRIAL_START_ROW));m3Put(M3_SCOPE_ORDINAL,0u);m3Put(M3_TRIAL_REASON,0u);m3Put(M3_TRIAL_BIRTHS,0u);m3Put(M3_TRIAL_MERGES,0u);m3Put(M3_TRIAL_BORN,0u);m3Put(M3_SEED,0u);m3Put(M3_LAMBDA,m3Get(M3_TRIAL_EXPECTED_LAMBDA));
    uint row[128];m3SourceRow(row,M3_TRIAL_BEGIN,0u);row[43]=m3Get(M3_LAMBDA);m3SourceEmit(row);
}
void m3TrialReplayEnd(){
    if(W[0]!=0u)return;
    if(m3Get(M3_EVENT_CURSOR)!=m3Get(M3_TRIAL_END_ROW)||m3Get(M3_TRIAL_REASON)!=m3Get(M3_TRIAL_ERROR)||m3Get(M3_TRIAL_BIRTHS)!=m3Get(M3_TRIAL_END_BIRTHS)||m3Get(M3_TRIAL_MERGES)!=m3Get(M3_TRIAL_END_MERGES)||m3Get(M3_TRIAL_BORN)!=m3Get(M3_TRIAL_END_BORN)){m3SourceFail(31u,0u);return;}
    m3Put(M3_REPLAY_DONE,1u);m3Replay=false;
}
void m3TrialClose(){
    if(W[0]!=0u||m3Get(M3_SOURCE_SUCCESS)!=0u||m3Get(M3_ROOT_STATE)!=1u)return;
    if(m3Get(M3_REPLAY_DONE)!=1u){m3SourceFail(31u,0u);return;}
    uint row[128];m3SourceRow(row,M3_TRIAL_END,0u);row[18]=m3Get(M3_TRIAL_BORN);row[27]=m3Get(M3_TRIAL_REASON);row[43]=m3Get(M3_LAMBDA);
    uint next=m3Get(M3_TRIAL_REASON)==1u?m3Get(M3_NEXT_LAMBDA):m3Get(M3_LAMBDA)/2u;row[44]=m3Get(M3_TRIAL_REASON)==0u?m3Get(M3_LAMBDA):next;row[45]=m3Get(M3_SEED);row[46]=m3Get(M3_TRIAL_BIRTHS);row[47]=m3Get(M3_TRIAL_MERGES);m3SourceEmit(row);
    if(W[0]!=0u)return;
    if(m3Get(M3_TRIAL_REASON)==0u){m3Put(M3_SOURCE_SUCCESS,1u);m3Put(M3_ACCEPTED_TRIAL,uM3Trial+1u);m3Put(M3_SOURCE_STOP,m3Get(M3_LAMBDA)==65536u?0u:1u);}
    else if(uM3Trial==7u||next<7u){m3Put(M3_ROOT_STATE,2u);m3Put(M3_SOURCE_STOP,2u);}
    else m3Put(M3_LAMBDA,next);
}
