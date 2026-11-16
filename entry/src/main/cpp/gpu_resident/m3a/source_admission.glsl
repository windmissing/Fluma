// T497 SOURCE entry. Original command32 and payload512 are preserved verbatim.
uniform uint uM3Dedup,uM3RawDedup,uM3SourceMeta,uM3SourceEvents;
const uint M3_SOURCE_STOP=27u,M3_SOURCE_REQUEST=28u,M3_SOURCE_MODE=29u,M3_SOURCE_X=30u,M3_SOURCE_Y=31u;
const uint M3_REPLAY_TERMINAL=32u,M3_APPEND_DEDUP=33u,M3_ACCEPTED_TRIAL=34u,M3_NEXT_LAMBDA=35u;
const uint M3_ROOT_STATE=36u,M3_SOURCE_REQUEST_SUM=37u,M3_TRIAL_CURSOR_BEGIN=38u,M3_TRIAL_SCOPE_BEGIN=39u;
uint m3Input(uint i){return W[uM3Input+i];}
bool m3CommandKey(uint at){return W[at+4u]==m3Input(4u)&&W[at+5u]==m3Input(5u)&&W[at+6u]==m3Input(6u)&&W[at+7u]==m3Input(7u);}
bool m3DecodeSource(){
    uint prefix=B[uHeader+33u];
    m3Put(M3_REPLAY_TERMINAL,0u);m3Put(M3_APPEND_DEDUP,0u);m3Put(M3_SOURCE_STOP,0u);m3Put(M3_ROOT_STATE,0u);
    if(prefix>4096u){m3SourceFail(15u,33u);return false;}
    if(m3Input(0u)!=1129534023u||m3Input(1u)!=4u||m3Input(4u)!=B[uHeader+4u]||m3Input(5u)!=B[uHeader+5u]){m3Put(M3_SOURCE_STOP,5u);return false;}
    for(uint i=0u;i<prefix;++i){
        uint row=uM3Dedup+64u*i;
        if(!m3CommandKey(row))continue;
        for(uint j=0u;j<32u;++j)if(W[row+j]!=m3Input(j)){m3Put(M3_SOURCE_STOP,5u);return false;}
        for(uint j=0u;j<512u;++j)if(W[uM3RawDedup+512u*i+j]!=m3Input(32u+j)){m3Put(M3_SOURCE_STOP,5u);return false;}
        m3Put(M3_REPLAY_TERMINAL,row+32u);m3Put(M3_SOURCE_STOP,6u);return false;
    }
    if(prefix==4096u){m3Put(M3_SOURCE_STOP,5u);return false;}
    m3Put(M3_APPEND_DEDUP,1u);
    if(m3Input(2u)>2u||m3Input(3u)!=0u||m3Input(30u)!=0u||m3Input(31u)!=0u||m3Input(27u)>1u){m3Put(M3_SOURCE_STOP,5u);return false;}
    if(m3Input(8u)!=B[uHeader+84u]||m3Input(9u)!=B[uHeader+85u]||m3Input(12u)!=B[uHeader+20u]||m3Input(13u)!=B[uHeader+21u]||m3Input(28u)!=B[uHeader+22u]||m3Input(29u)!=B[uHeader+23u]||m3Input(14u)==0u||m3Input(15u)==0u||(m3Input(6u)|m3Input(7u))==0u||(m3Input(10u)|m3Input(11u))==0u||(m3Input(24u)|m3Input(25u))==0u){m3Put(M3_SOURCE_STOP,5u);return false;}
    uint x,y,quantity;
    if(!grsFloorBinary64Q20(uvec2(m3Input(16u),m3Input(17u)),134217727u,x)||!grsFloorBinary64Q20(uvec2(m3Input(18u),m3Input(19u)),CQ,y)||!grsFloorBinary64Q20(uvec2(m3Input(20u),m3Input(21u)),CQ,quantity)){m3Put(M3_SOURCE_STOP,5u);return false;}
    m3Put(M3_SOURCE_X,x);m3Put(M3_SOURCE_Y,y);m3Put(M3_SOURCE_REQUEST,quantity);
    if((m3Input(19u)&0x7fffffffu)>0x40700000u||((m3Input(19u)&0x7fffffffu)==0x40700000u&&m3Input(18u)!=0u)||(m3Input(21u)&0x7fffffffu)>0x40700000u||((m3Input(21u)&0x7fffffffu)==0x40700000u&&m3Input(20u)!=0u)){m3Put(M3_SOURCE_STOP,5u);return false;}
    if(!m3CaptureColumns()){m3SourceFail(15u,0u);return false;}
    if(m3Input(2u)==2u){
        if(!m3DecodeBulk()){m3Put(M3_SOURCE_STOP,5u);return false;}
        uint total=0u;for(uint c=0u;c<128u;++c)total+=m3Get(M3_REQUESTED+c);
        m3Put(M3_SOURCE_REQUEST_SUM,total);if(total==0u){m3Put(M3_SOURCE_STOP,4u);return false;}
        for(uint c=0u;c<128u;++c)if(m3Get(M3_REQUESTED+c)>0u&&col(6u,c)==0u){m3Put(M3_SOURCE_STOP,3u);return false;}
    }else{
        for(uint i=0u;i<512u;++i)if(m3Input(32u+i)!=0u){m3Put(M3_SOURCE_STOP,5u);return false;}
        if(quantity==0u){m3Put(M3_SOURCE_STOP,4u);return false;}
        uint column=x>>20u;if(y<col(10u,column)){m3Put(M3_SOURCE_STOP,5u);return false;}
        uint mode=m3Input(2u)==0u?0u:y<=m3Eta(column)?1u:2u;
        m3Put(M3_SOURCE_MODE,mode);m3Put(M3_SOURCE_COLUMN,column);
        uint allowed=mode==2u?CQ-col(13u,column):m3Cap(column)-m3Eta(column);
        m3Put(M3_SOURCE_REQUEST_SUM,quantity);m3Put(M3_WANT,min(quantity,allowed));
    }
    uvec2 origin;if(!grsAdd64(uvec2(B[uHeader+14u],B[uHeader+15u]),uvec2(1u,0u),origin)){m3SourceFail(13u,14u);return false;}
    m3Put(M3_PENDING_ORIGIN_LO,origin.x);m3Put(M3_PENDING_ORIGIN_HI,origin.y);return true;
}
