// Six fixed control rows fit the 16-row arena. The final PLANNER_END is built
// after both independent reruns; transport copies its fixed mirror to CONTROL.
uniform uint uM3CombinedEventWord,uM3SelectedBank,uM3StateWords,uM3ActualEventStart;
uint m3QBase(uint field){return m3Get(4096u+field);}
uint m3ControlSlot(uint role){return role<=3u?role-1u:role;}
void m3ControlRecord(out uint row[128],uint kind,uint role){
    for(uint i=0u;i<128u;++i)row[i]=0u;
    uint at=m3RoleAddress(role),selected=m3PlanGet(M3_PLAN_SELECTED);
    row[0]=876825680u;row[1]=4u;row[2]=kind;row[3]=2u;row[4]=uNonce.x;row[5]=uNonce.y;
    row[6]=role;row[7]=role;row[10]=W[at+5u];row[11]=W[at+6u];row[12]=m3QBase(14u);row[13]=m3QBase(15u);row[14]=m3QBase(16u);row[15]=m3QBase(17u);
    row[16]=W[at+1u];row[17]=selected!=0u?W[m3RoleAddress(selected)+1u]:m3PlanGet(M3_PLAN_J0);
    row[18]=W[at+8u];row[19]=W[at+9u];row[20]=uM3Omega/120u;
    row[21]=kind==25u?m3PlanGet(M3_PLAN_STATUS):W[at]==0u?4u:W[at+2u]==0u?3u:role<=3u?role-1u:m3PlanGet(M3_PLAN_STATUS);
    row[22]=W[at+10u];row[23]=W[at+11u];row[24]=m3QBase(147u);row[27]=W[at+M3_SCOPE_ROWS];row[28]=W[at+16u];row[32]=W[at+12u];row[33]=W[at+13u];
    for(uint r=1u;r<=5u;++r){uint d=m3RoleAddress(r);row[34]+=W[d+M3_SCOPE_AUDIT];row[35]+=W[d]!=0u&&W[d+7u]!=0u?1u:0u;}
    row[36]=selected==0u?0u:m3SelectedRows();row[37]=selected;row[38]=m3PlanGet(M3_PLAN_J0);row[39]=uint(uM3Target);
    row[44]=W[at]==0u&&kind==24u?8u:0u;row[45]=W[2];row[46]=W[3];row[47]=W[0];
}
void m3StoreControl(uint slot,uint row[128],bool writeArena){
    uint mirror=uM3Scratch+M3_CONTROL_MIRROR+slot*128u;
    for(uint i=0u;i<128u;++i)if(i!=31u){W[mirror+i]=row[i];if(writeArena)E[slot*128u+i]=row[i];}
    memoryBarrierBuffer();W[mirror+31u]=3235774468u;if(writeArena)E[slot*128u+31u]=3235774468u;
}
void m3RecordScope(bool writeArena){uint row[128];m3ControlRecord(row,24u,uM3Role);m3StoreControl(m3ControlSlot(uM3Role),row,writeArena);}
void m3RecordPlannerEnd(){uint row[128];m3ControlRecord(row,25u,m3PlanGet(M3_PLAN_SELECTED));m3StoreControl(3u,row,false);}
bool m3RepresentationState(uint word){return word==uHeader+76u||word==uHeader+77u||word==uHeader+78u||word==uHeader+152u||word==uHeader+153u;}
uint m3ScopeHeader(uint bank,uint field){uint at=uHeader+field;return bank==0u?B[at]:bank==1u?C[at]:E[at];}
bool m3OwnScopeHeader(uint bank,uint role,uint physicalBank){
    uint descriptor=m3RoleAddress(role);
    if(W[descriptor+M3_SCOPE_AUDIT]!=1u)return false;
    for(uint i=4u;i<12u;++i)if(m3ScopeHeader(bank,i)!=m3QBase(i))return false;
    if(m3ScopeHeader(bank,12u)!=uNonce.x||m3ScopeHeader(bank,13u)!=uNonce.y||m3ScopeHeader(bank,18u)!=uControl.x||m3ScopeHeader(bank,19u)!=uControl.y||m3ScopeHeader(bank,49u)!=uFrame.x||m3ScopeHeader(bank,50u)!=uFrame.y)return false;
    if(m3ScopeHeader(bank,76u)!=physicalBank||m3ScopeHeader(bank,77u)!=uM3BaseBank||m3ScopeHeader(bank,78u)!=uM3SpareBank||m3ScopeHeader(bank,152u)!=W[descriptor+5u]||m3ScopeHeader(bank,153u)!=W[descriptor+6u])return false;
    uint selected=role<=3u?role:m3PlanGet(M3_PLAN_SELECTED);
    if(m3ScopeHeader(bank,150u)!=selected||m3ScopeHeader(bank,151u)!=selected-1u||m3ScopeHeader(bank,45u)!=W[descriptor+1u]||m3ScopeHeader(bank,158u)!=6u)return false;
    uint start=role==4u?uM3ActualEventStart:0u;
    if(m3ScopeHeader(bank,38u)!=start+W[descriptor+M3_SCOPE_ROWS])return false;
    for(uint i=88u;i<112u;++i)if(m3ScopeHeader(bank,i)!=m3QBase(i))return false;
    return true;
}
void m3SelectScope(){
    uint at=m3RoleAddress(uM3Role);
    if(W[0]==0u&&m3ScopeEnabled()&&!m3OwnScopeHeader(0u,uM3Role,W[at+16u])){qfail(28u);return;}
    if(W[0]==0u&&uM3Role<=3u&&W[at+2u]!=0u){
        if(W[at+M3_SCOPE_AUDIT]!=1u){qfail(15u);return;}
        for(uint i=0u;i<uM3StateWords;++i)C[i]=B[i];
        C[uHeader+76u]=uM3SelectedBank;
        uint words=W[at+M3_SCOPE_ROWS]*64u;
        for(uint i=0u;i<words;++i)C[uM3CombinedEventWord+i]=B[uM3CombinedEventWord+i];
    }
    m3RecordScope(true);
}
void m3AuthorityFinalize(){
    uint selected=m3PlanGet(M3_PLAN_SELECTED);
    if(W[0]!=0u||selected==0u){m3RecordScope(false);return;}
    uint rows=m3SelectedRows(),at=m3RoleAddress(4u);
    if(!m3OwnScopeHeader(0u,selected,uM3SelectedBank)||!m3OwnScopeHeader(1u,4u,W[at+16u])){qfail(28u);return;}
    if(W[at+M3_SCOPE_AUDIT]!=1u||rows!=W[at+M3_SCOPE_ROWS]){qfail(16u);return;}
    for(uint i=0u;i<uM3StateWords;++i){
        if(m3RepresentationState(i))continue;
        uint actual=C[i];if(i==uHeader+38u){if(actual<uM3ActualEventStart){qfail(16u);return;}actual-=uM3ActualEventStart;}
        if(B[i]!=actual){qfail(16u);W[19]=i;W[26]=B[i];W[27]=actual;return;}
    }
    // Latch full authority-event equality before consistency reuses WORK.
    for(uint i=0u;i<rows*64u;++i){
        uint mask=i%64u==52u?~(7u<<22u):0xffffffffu;
        uint expected=B[uM3CombinedEventWord+i]&mask,actual=E[(uM3ActualEventStart*64u)+i]&mask;
        if(i%64u==7u){if(actual<uM3ActualEventStart){qfail(16u);return;}actual-=uM3ActualEventStart;}
        if(expected!=actual){qfail(16u);W[19]=uM3StateWords+i;W[26]=expected;W[27]=actual;return;}
    }
    m3Put(M3_AUTH_MATCH,1u);m3RecordScope(false);
}
