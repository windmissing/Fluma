uniform uint uM3EmitQ;
// Ordinary kind0/1 preserve M2 prefix and DIRECT/FLIGHT behavior in V4.
void m3OrdinaryInitialize(){
    uint column=m3Get(M3_SOURCE_COLUMN),kind=m3Input(2u);
    for(uint c=0u;c<128u;++c){m3Put(M3_ADMITTED+c,c==column?m3Get(M3_WANT):0u);m3Put(M3_TARGET_WATER+c,col(6u,c));}
    m3Put(M3_TRIAL_REASON,0u);m3Put(M3_TRIAL_BORN,0u);m3Put(M3_TRIAL_BIRTHS,0u);m3Put(M3_TRIAL_MERGES,0u);m3Put(M3_SEED,0u);
}
void m3OrdinaryWater(){
    uint column=m3Get(M3_SOURCE_COLUMN),amount=m3Get(M3_WANT),mode=m3Get(M3_SOURCE_MODE);
    if(amount==0u){m3Put(M3_SOURCE_STOP,1u);return;}
    if(mode==1u){
        if(!m3SourceCredit(2u))return;
        uint before=col(6u,column),row[128];m3SourceRow(row,M3_DIRECT,column);row[14]=1u;row[18]=amount;row[21]=amount;row[23]=before;row[24]=before+amount;row[28]=row[29]=C[uHeader+40u];m3SourceEmit(row);
        if(W[0]!=0u)return;C[uColumn+6u*128u+column]=before+amount;m3SourceDerived(column);m3Put(M3_TRIAL_BORN,amount);return;
    }
    uint slot=uKw;
    for(uint i=0u;i<uKw;++i)if(C[uWater+i]==0u){slot=i;break;}
    if(slot==uKw){m3Put(M3_SOURCE_STOP,5u);return;}
    uint old[16],born[16],row[128];for(uint f=0u;f<16u;++f){old[f]=C[uWater+f*uKw+slot];born[f]=0u;}
    if(old[11]==0xffffffffu){m3SourceFail(21u,slot);return;}
    uint id=C[uHeader+40u];if(id==0u||C[uHeader+127u]!=0u){m3Put(M3_SOURCE_STOP,3u);return;}
    if(m3Get(M3_SOURCE_Y)<=m3Eta(column)||!m3SourceCredit(2u)){m3SourceFail(15u,slot);return;}
    born[0]=amount;born[1]=m3Get(M3_SOURCE_X);born[2]=m3Get(M3_SOURCE_Y);born[4]=0u-uM3EmitQ;born[5]=id;born[6]=m3Get(M3_PENDING_ORIGIN_LO);born[7]=m3Get(M3_PENDING_ORIGIN_HI);uvec2 commit;
    if(!grsAdd64(uvec2(m3SourceBase(16u),m3SourceBase(17u)),uvec2(1u,0u),commit)){m3SourceFail(13u,16u);return;}
    born[8]=commit.x;born[9]=commit.y;born[10]=1u;born[11]=old[11]+1u;born[12]=m3Input(14u);born[13]=m3SourceBase(118u);born[14]=m3SourceBase(119u);
    m3SourceRow(row,M3_BIRTH,column);row[14]=1u;row[15]=slot;row[18]=amount;row[21]=amount;row[23]=col(7u,column);row[24]=row[23]+amount;row[25]=1u;row[28]=id;row[29]=id==0xffffffffu?id:id+1u;if(id==0xffffffffu)row[30]|=2u;
    for(uint i=0u;i<16u;++i){row[32u+i]=old[i];row[64u+i]=born[i];}
    m3SourceEmit(row);if(W[0]!=0u||!m3SourceMutation())return;
    for(uint f=0u;f<16u;++f)C[uWater+f*uKw+slot]=born[f];
    C[uHeader+40u]=id==0xffffffffu?id:id+1u;if(id==0xffffffffu)C[uHeader+127u]=1u;
    C[uHeader+42u]++;C[uHeader+44u]--;C[uColumn+7u*128u+column]+=amount;m3SourceDerived(column);m3Put(M3_TRIAL_BIRTHS,1u);m3Put(M3_TRIAL_BORN,amount);
}
void m3OrdinaryEnd(){
    if(W[0]!=0u)return;
    uint admitted=m3Get(M3_TRIAL_BORN),reason=m3Get(M3_TRIAL_REASON);
    if(m3Input(2u)==0u&&reason!=0u){uint stop=reason==4u?4u:reason==5u?2u:reason==6u?3u:1u;m3Put(M3_SOURCE_STOP,stop);}
    else if(admitted<m3Get(M3_SOURCE_REQUEST_SUM)&&m3Get(M3_SOURCE_STOP)==0u)m3Put(M3_SOURCE_STOP,1u);
    if(admitted>0u)m3Put(M3_SOURCE_SUCCESS,1u);
    m3Put(M3_ROOT_STATE,2u);
}
