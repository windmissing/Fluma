// V4 SOURCE finalization. Failed trials never publish their private bank.
uniform uint uM3Words,uM3EventArena,uM3BaseBank,uM3CandidateBank,uM3SpareBank,uM3Carry;
const uint M3_TERMINAL=3072u;
bool m3RootReplay=false;
void m3SourceStore(uint at,uint value){
    if(m3RootReplay){if(W[at]!=value)m3SourceFail(31u,at);}
    else W[at]=value;
}
void m3SetPair(uint at,uvec2 value){C[uHeader+at]=value.x;C[uHeader+at+1u]=value.y;}
bool m3AddHeader(uint at,uint amount){uvec2 next;if(!grsAdd64(uvec2(C[uHeader+at],C[uHeader+at+1u]),uvec2(amount,0u),next)){m3SourceFail(13u,at);return false;}m3SetPair(at,next);return true;}
void m3FinalizeSource(){
    if(W[0]!=0u)return;
    bool accepted=m3Get(M3_SOURCE_SUCCESS)!=0u;
    uint admitted=accepted?m3Get(M3_TRIAL_BORN):0u,kind=m3Input(2u),mode=m3Get(M3_SOURCE_MODE),requested=m3Get(M3_SOURCE_REQUEST_SUM);
    if(!accepted)for(uint i=0u;i<uM3Words;++i)C[i]=B[i];
    uint terminal=uM3Scratch+M3_TERMINAL;
    for(uint i=0u;i<32u;++i)W[terminal+i]=0u;
    if(m3Get(M3_REPLAY_TERMINAL)!=0u){
        uint old=m3Get(M3_REPLAY_TERMINAL);for(uint i=0u;i<32u;++i)W[terminal+i]=W[old+i];W[terminal+2u]=1u;
    }else{
        uint stop=m3Get(M3_SOURCE_STOP),encoded=kind==2u?(0x20000u|stop):stop;
        W[terminal]=admitted==0u?3u:admitted==requested?1u:2u;W[terminal+1u]=encoded;W[terminal+3u]=admitted;W[terminal+4u]=requested-admitted;
        W[terminal+5u]=m3Get(M3_SOURCE_COLUMN);W[terminal+6u]=accepted?m3Get(M3_TRIAL_BIRTHS):0u;W[terminal+7u]=admitted>0u&&W[terminal+6u]>0u?B[uHeader+39u+(kind==1u?1u:0u)]:0u;
        W[terminal+8u]=B[uHeader+39u+(kind==1u?1u:0u)];W[terminal+9u]=C[uHeader+39u+(kind==1u?1u:0u)];W[terminal+30u]=mode;W[terminal+31u]=C[uHeader+126u]|(C[uHeader+127u]<<1u)|4u;
    }
    if(accepted){
        if(admitted==0u){m3SourceFail(15u,23u);return;}
        if(!m3AddHeader(14u,1u)||!m3AddHeader(16u,1u)||!m3AddHeader(kind==1u?58u:56u,admitted)||!m3AddHeader(kind==1u?62u:60u,admitted))return;
        if(kind==1u&&!m3AddHeader(mode==1u?112u:114u,admitted))return;
        uint eventIndex=B[uHeader+34u],event=uM3SourceEvents+eventIndex*16u,meta=uM3SourceMeta+eventIndex*32u;
        if(eventIndex>=4096u){m3SourceFail(15u,34u);return;}
        if(!m3RootReplay){for(uint i=0u;i<16u;++i)m3SourceStore(event+i,0u);for(uint i=0u;i<32u;++i)m3SourceStore(meta+i,0u);}
        m3SourceStore(event,m3Input(6u));m3SourceStore(event+1u,m3Input(7u));m3SourceStore(event+2u,C[uHeader+14u]);m3SourceStore(event+3u,C[uHeader+15u]);m3SourceStore(event+4u,kind);m3SourceStore(event+5u,m3Get(M3_SOURCE_COLUMN));m3SourceStore(event+6u,admitted);m3SourceStore(event+7u,W[terminal+7u]);m3SourceStore(event+8u,W[terminal+6u]);m3SourceStore(event+9u,m3Input(14u));m3SourceStore(event+10u,m3Get(M3_SOURCE_X));m3SourceStore(event+11u,m3Get(M3_SOURCE_Y));m3SourceStore(event+12u,W[terminal+8u]);m3SourceStore(event+13u,W[terminal+9u]);m3SourceStore(event+14u,m3Get(M3_TRIAL_MERGES));m3SourceStore(event+15u,m3Get(M3_SOURCE_STOP));
        m3SourceStore(meta,kind);m3SourceStore(meta+1u,kind==2u?m3Get(M3_LAMBDA):0u);m3SourceStore(meta+2u,kind==2u?m3Get(M3_ACCEPTED_TRIAL):0u);m3SourceStore(meta+3u,kind==2u?m3Get(M3_SEED):0u);m3SourceStore(meta+4u,W[terminal+6u]);m3SourceStore(meta+5u,m3Get(M3_TRIAL_MERGES));m3SourceStore(meta+6u,admitted);m3SourceStore(meta+7u,requested);m3SourceStore(meta+8u,C[uHeader+14u]);m3SourceStore(meta+9u,C[uHeader+15u]);m3SourceStore(meta+10u,C[uHeader+16u]);m3SourceStore(meta+11u,C[uHeader+17u]);m3SourceStore(meta+12u,B[uHeader+33u]);m3SourceStore(meta+13u,kind==2u?512u:0u);m3SourceStore(meta+14u,m3Input(14u));m3SourceStore(meta+15u,m3Input(15u));m3SourceStore(meta+16u,W[terminal+7u]);m3SourceStore(meta+17u,W[terminal+8u]);m3SourceStore(meta+18u,W[terminal+9u]);m3SourceStore(meta+19u,kind==2u?m3Get(M3_ETA_HI):0u);m3SourceStore(meta+20u,kind==2u?m3Get(M3_RESIDUAL):0u);m3SourceStore(meta+21u,m3Input(18u));m3SourceStore(meta+22u,m3Input(19u));m3SourceStore(meta+23u,B[uHeader+58u]);m3SourceStore(meta+24u,B[uHeader+59u]);m3SourceStore(meta+25u,W[terminal+1u]);m3SourceStore(meta+26u,m3Input(6u));m3SourceStore(meta+27u,m3Input(7u));
        C[uHeader+34u]=eventIndex+1u;
    }
    uint record[128];m3SourceRow(record,M3_END,0u);record[10]=C[uHeader+14u];record[11]=C[uHeader+15u];record[12]=C[uHeader+16u];record[13]=C[uHeader+17u];record[18]=admitted;record[25]=W[terminal+6u];record[26]=accepted?m3Get(M3_TRIAL_MERGES):0u;record[27]=W[terminal+1u];record[28]=W[terminal+8u];record[29]=W[terminal+9u];record[96]=record[97]=record[98]=record[99]=record[103]=0u;m3SourceEmit(record);if(W[0]!=0u)return;
    if(m3Get(M3_REPLAY_TERMINAL)==0u){
        for(uint i=0u;i<2u;++i){W[terminal+10u+i]=C[uHeader+14u+i];W[terminal+12u+i]=C[uHeader+16u+i];W[terminal+14u+i]=accepted?C[uHeader+14u+i]:0u;W[terminal+16u+i]=B[uHeader+56u+i];W[terminal+18u+i]=C[uHeader+56u+i];W[terminal+20u+i]=B[uHeader+58u+i];W[terminal+22u+i]=C[uHeader+58u+i];W[terminal+24u+i]=C[uHeader+60u+i];W[terminal+26u+i]=C[uHeader+62u+i];}
        W[terminal+28u]=B[uHeader+33u]+m3Get(M3_APPEND_DEDUP);W[terminal+29u]=0u;
        if(m3Get(M3_APPEND_DEDUP)!=0u){uint row=B[uHeader+33u];if(row>=4096u){m3SourceFail(15u,33u);return;}for(uint i=0u;i<32u;++i){m3SourceStore(uM3Dedup+row*64u+i,m3Input(i));m3SourceStore(uM3Dedup+row*64u+32u+i,W[terminal+i]);}for(uint i=0u;i<512u;++i)m3SourceStore(uM3RawDedup+row*512u+i,m3Input(32u+i));C[uHeader+33u]=row+1u;}
    }
    C[uHeader+74u]=C[uHeader+33u];C[uHeader+75u]=0u;
    C[uHeader+137u]=accepted?m3Get(M3_TRIAL_MERGES):0u;C[uHeader+138u]=accepted?m3Get(M3_TRIAL_BIRTHS):0u;C[uHeader+139u]=W[terminal+1u];C[uHeader+140u]=128u;C[uHeader+141u]=m3Get(M3_EVENT_CURSOR);C[uHeader+144u]=uM3EventArena;
    C[uHeader+147u]=kind==2u&&accepted?m3Get(M3_LAMBDA):0u;C[uHeader+148u]=kind==2u?m3Get(M3_ATTEMPTS):0u;C[uHeader+149u]=kind==2u&&accepted?m3Get(M3_SEED):0u;C[uHeader+150u]=0u;C[uHeader+151u]=0u;C[uHeader+152u]=m3Get(M3_BRANCH_LO);C[uHeader+153u]=m3Get(M3_BRANCH_HI);C[uHeader+154u]=kind==2u&&accepted?m3Get(M3_ETA_HI):0u;C[uHeader+155u]=kind==2u&&accepted?m3Get(M3_RESIDUAL):0u;C[uHeader+157u]=m3Get(M3_EVENT_CURSOR);C[uHeader+158u]=0u;C[uHeader+159u]=mode;
    if(!m3AddHeader(72u,1u))return;
    m3SetPair(12u,uNonce);m3SetPair(18u,uControl);m3SetPair(49u,uFrame);
    C[uHeader+47u]=uM3Carry;C[uHeader+38u]=0u;
    C[uHeader+76u]=uM3CandidateBank;C[uHeader+77u]=uM3BaseBank;C[uHeader+78u]=uM3SpareBank;
    C[uHeader+79u]=1u;C[uHeader+80u]=accepted?1u:m3Get(M3_REPLAY_TERMINAL)!=0u?3u:2u;C[uHeader+81u]=4u;
}
