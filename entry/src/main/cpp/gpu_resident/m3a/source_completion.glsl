// Ordinary roots expose only this 256-word tail. Control/events remain resident.
uniform uint uM3PublicationSlot,uM3Uploaded;
const uint M3_COMPLETED_PASSES=64u,M3_AUDIT_FAMILIES=65u;
void m3SealSource(){
    if(W[0]==0u&&(m3Get(M3_COMPLETED_PASSES)!=72u||m3Get(M3_AUDIT_FAMILIES)!=32767u))m3SourceFail(15u,97u);
    bool ok=W[0]==0u;
    for(uint i=0u;i<256u;++i)E[i]=0u;
    E[0]=1381192263u;E[1]=4u;E[2]=256u;E[4]=uNonce.x;E[5]=uNonce.y;
    for(uint i=0u;i<8u;++i)E[6u+i]=B[uHeader+4u+i];
    for(uint i=0u;i<4u;++i){E[14u+i]=B[uHeader+14u+i];E[18u+i]=C[uHeader+14u+i];}
    E[22]=uControl.x;E[23]=uControl.y;E[24]=uFrame.x;E[25]=uFrame.y;
    E[27]=1u;E[28]=ok?C[uHeader+80u]:4u;E[29]=W[0];E[30]=ok?0u:W[2];E[31]=ok?0u:W[3];
    E[32]=uM3BaseBank;E[33]=uM3CandidateBank;E[34]=uM3SpareBank;E[35]=uM3PublicationSlot;
    E[36]=C[uHeader+72u];E[37]=C[uHeader+73u];
    for(uint i=0u;i<13u;++i)E[38u+i]=B[uHeader+20u+i];
    E[51]=uM3EventArena;E[53]=m3Get(M3_EVENT_CURSOR);E[54]=uM3SourceRows;
    E[55]=B[uHeader+34u];E[56]=C[uHeader+34u];E[57]=B[uHeader+33u];E[58]=C[uHeader+33u];
    uint terminal=uM3Scratch+M3_TERMINAL;
    E[59]=m3Input(14u);E[60]=m3Input(15u);E[61]=W[terminal+3u];E[62]=W[terminal+4u];
    E[63]=W[terminal];E[64]=W[terminal+1u];E[65]=W[terminal+2u];E[66]=W[terminal+6u];E[67]=W[terminal+7u];E[68]=W[terminal+9u];
    for(uint i=0u;i<2u;++i){
        E[69u+i]=B[uHeader+56u+i];E[71u+i]=C[uHeader+56u+i];E[73u+i]=B[uHeader+58u+i];E[75u+i]=C[uHeader+58u+i];
        E[77u+i]=C[uHeader+60u+i];E[79u+i]=C[uHeader+62u+i];E[83u+i]=C[uHeader+54u+i];
        E[138u+i]=B[uHeader+112u+i];E[140u+i]=C[uHeader+112u+i];E[142u+i]=B[uHeader+114u+i];E[144u+i]=C[uHeader+114u+i];
        E[146u+i]=B[uHeader+116u+i];E[148u+i]=C[uHeader+116u+i];E[150u+i]=C[uHeader+118u+i];E[152u+i]=C[uHeader+120u+i];
    }
    for(uint c=0u;c<128u;++c)E[87]=max(E[87],col(13u,c));
    E[88]=C[uHeader+41u];E[89]=C[uHeader+43u];E[90]=C[uHeader+42u];E[91]=C[uHeader+44u];E[92]=C[uHeader+39u];E[93]=C[uHeader+40u];
    E[96]=m3Get(M3_EVENT_CURSOR);E[97]=m3Get(M3_COMPLETED_PASSES);E[99]=m3Get(M3_AUDIT_FAMILIES);E[101]=ok?0xffffffffu:W[4];
    E[106]=ok&&E[28]!=1u?1u:0u;E[107]=C[uHeader+45u];E[108]=uM3Carry;
    E[110]=m3Get(M3_COMPLETED_PASSES);E[111]=m3Get(M3_COMPLETED_PASSES);E[112]=uM3Uploaded;
    for(uint i=0u;i<24u;++i)E[114u+i]=B[uHeader+88u+i];
    E[158]=W[terminal+30u];E[159]=W[terminal+7u];
    for(uint i=0u;i<13u;++i)E[160u+i]=C[uHeader+132u+i];
    for(uint i=0u;i<16u;++i)E[173u+i]=C[uHeader+145u+i];
    memoryBarrierBuffer();E[3]=3235823838u;
}
