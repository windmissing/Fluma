uniform uint uM3ExpectedPasses,uM3ActualBarriers,uM3Uploaded,uM3PublicationSlot,uM3EventArena;
const uint M3_Q_AUDITS=83u;
void m3SealQuantum(){
    if(W[0]==0u&&(m3Get(M3_Q_PASS_COUNT)!=uM3ExpectedPasses||m3Get(M3_Q_AUDITS)!=32767u))qfail(15u);
    bool ok=W[0]==0u;uint selected=m3PlanGet(M3_PLAN_SELECTED);
    for(uint i=0u;i<256u;++i)E[i]=0u;
    E[0]=1381192263u;E[1]=4u;E[2]=256u;E[4]=uNonce.x;E[5]=uNonce.y;
    for(uint i=0u;i<8u;++i)E[6u+i]=B[uHeader+4u+i];
    for(uint i=0u;i<4u;++i){E[14u+i]=B[uHeader+14u+i];E[18u+i]=C[uHeader+14u+i];}
    E[22]=uControl.x;E[23]=uControl.y;E[24]=uFrame.x;E[25]=uFrame.y;E[26]=uQuantumOrdinal;E[27]=2u;
    E[28]=ok?(selected==0u?5u:1u):4u;E[29]=W[0];E[30]=ok?0u:W[2];E[31]=ok?0u:W[3];
    E[32]=uM3BaseBank;E[33]=uM3CandidateBank;E[34]=uM3SpareBank;E[35]=uM3PublicationSlot;E[36]=C[uHeader+72u];E[37]=C[uHeader+73u];
    for(uint i=0u;i<13u;++i)E[38u+i]=B[uHeader+20u+i];
    E[51]=uM3EventArena;E[52]=uM3ActualEventStart;E[53]=uM3ActualEventStart+(selected==0u?0u:m3SelectedRows());E[54]=uEventCapacity;
    E[55]=B[uHeader+34u];E[56]=C[uHeader+34u];E[57]=B[uHeader+33u];E[58]=C[uHeader+33u];
    for(uint i=0u;i<2u;++i){
        E[69u+i]=B[uHeader+56u+i];E[71u+i]=C[uHeader+56u+i];E[73u+i]=B[uHeader+58u+i];E[75u+i]=C[uHeader+58u+i];
        E[77u+i]=C[uHeader+60u+i];E[79u+i]=C[uHeader+62u+i];E[83u+i]=C[uHeader+54u+i];
        E[138u+i]=B[uHeader+112u+i];E[140u+i]=C[uHeader+112u+i];E[142u+i]=B[uHeader+114u+i];E[144u+i]=C[uHeader+114u+i];
        E[146u+i]=B[uHeader+116u+i];E[148u+i]=C[uHeader+116u+i];E[150u+i]=C[uHeader+118u+i];E[152u+i]=C[uHeader+120u+i];
    }
    E[81]=C[uHeader+52u]-C[uHeader+54u];
    for(uint c=0u;c<128u;++c)E[87]=max(E[87],col(13u,c));
    E[88]=C[uHeader+41u];E[89]=C[uHeader+43u];E[90]=C[uHeader+42u];E[91]=C[uHeader+44u];E[92]=C[uHeader+39u];E[93]=C[uHeader+40u];
    E[97]=m3Get(M3_Q_PASS_COUNT);E[99]=m3Get(M3_Q_AUDITS);E[101]=ok?0xffffffffu:W[19];E[102]=W[26];E[103]=W[27];
    E[107]=C[uHeader+45u];E[108]=uM3Carry;E[110]=m3Get(M3_Q_PASS_COUNT);E[111]=uM3ActualBarriers;E[112]=uM3Uploaded;
    for(uint i=0u;i<24u;++i)E[114u+i]=B[uHeader+88u+i];
    for(uint i=0u;i<4u;++i)E[154u+i]=C[uHeader+128u+i];
    for(uint i=0u;i<13u;++i)E[160u+i]=C[uHeader+132u+i];
    for(uint i=0u;i<16u;++i)E[173u+i]=C[uHeader+145u+i];
    memoryBarrierBuffer();E[3]=3235823838u;
}
