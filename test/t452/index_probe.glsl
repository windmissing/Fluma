void main(){
    m2IndexSort(0u,256u,192u);
    uvec2 pair=m2IndexPair(0u,256u,192u,30u);
    if(gl_LocalInvocationIndex==0u){
        result[0]=pair.x;result[1]=pair.y;
        cases[4u*16u]=0u;
        cases[16u+2u]=96u;
    }
    memoryBarrierBuffer();barrier();
    m2IndexUpdate(0u,256u,256u,192u,1u,4u);
    pair=m2IndexPair(256u,256u,192u,30u);
    if(gl_LocalInvocationIndex==0u){result[2]=pair.x;result[3]=pair.y;}
    if(gl_LocalInvocationIndex==0u){
        uint row[128];for(uint i=0u;i<128u;++i)row[i]=0u;
        row[M2_sourceEvent128_magic]=M2_sourceMagic;
        row[M2_sourceEvent128_revision]=M2_rev_wire;
        row[M2_sourceEvent128_kind]=M2_END;
        row[M2_sourceEvent128_rootKind]=1u;
        row[M2_sourceEvent128_ordinalLo]=1u;
        row[M2_sourceEvent128_stopCode]=M2_NO_PAIR;
        bool sealed=m2SealSourceRow(row,0u,2u);
        bool readable=m2ReadSourceRow(0u,1u,2u,row);
        bool wrongCount=m2ReadSourceRow(0u,3u,2u,row);
        row[M2_sourceEvent128_rowSeal]=0u;
        row[M2_sourceEvent128_ordinalLo]=2u;
        row[97]=1u;
        bool bad=m2SealSourceRow(row,1u,2u);
        result[260]=uint(sealed&&readable&&!wrongCount&&!bad);
        E[100]=1u;
        result[261]=uint(!m2ReadSourceRow(0u,1u,2u,row));
    }
    for(uint r=gl_LocalInvocationIndex;r<256u;r+=64u)result[4u+r]=W[256u+r];
}
