// V4 event record helpers. Scope/root identity is frozen by the root driver.
uniform uint uM3Role,uM3EventCapacity,uM3EventStart;
uniform uvec2 uM3Nonce,uM3Frame;
uniform uint uM3Ordinal,uM3BaseSourceLo,uM3BaseSourceHi,uM3ProposedPhysicsLo,uM3ProposedPhysicsHi;
bool m3LevelReplay=false;
uint m3LevelCursor=0u;
void m3EmitLevel(uint donor,uint receiver,uint donorBefore,uint receiverBefore,uint amount,uvec2 budgetBefore,uvec2 budgetAfter){
    uint cursor=m3LevelReplay?m3LevelCursor:W[1];
    if(cursor>=uM3EventCapacity){if(W[0]==0u)W[0]=12u;return;}
    uint at=cursor*64u;uint expected[64];for(uint i=0u;i<64u;++i)expected[i]=0u;
    expected[0u]=1163088455u;expected[1u]=4u;expected[2u]=uM3Nonce.x;expected[3u]=uM3Nonce.y;
    expected[4u]=2u;expected[5u]=3u;expected[6u]=19u;expected[7u]=cursor+1u;
    expected[9u]=uM3Frame.x;expected[10u]=uM3Frame.y;expected[11u]=uM3Ordinal;
    expected[12u]=uM3BaseSourceLo;expected[13u]=uM3BaseSourceHi;expected[14u]=uM3ProposedPhysicsLo;expected[15u]=uM3ProposedPhysicsHi;
    expected[20u]=donor;expected[21u]=receiver;expected[32u]=donorBefore;expected[33u]=donorBefore-amount;expected[34u]=receiverBefore;expected[35u]=receiverBefore+amount;
    expected[36u]=budgetBefore.x;expected[37u]=budgetBefore.y;expected[38u]=budgetAfter.x;expected[39u]=budgetAfter.y;
    expected[52u]=(2u<<18u)|(uM3Role<<22u);expected[63u]=amount;
    if(m3LevelReplay){
        for(uint i=0u;i<64u;++i)if(E[at+i]!=expected[i]){if(W[0]==0u){W[0]=16u;W[3]=cursor+1u;W[4]=i;}return;}
        m3LevelCursor=cursor+1u;
    }else{
        for(uint i=0u;i<64u;++i)E[at+i]=expected[i];
        memoryBarrierBuffer();W[1]=cursor+1u;
    }
}
