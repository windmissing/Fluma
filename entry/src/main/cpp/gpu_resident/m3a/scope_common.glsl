// Scope descriptors are GPU-owned; no branch/angle plan is read by the host.
uniform uint uM3WriteBank,uM3BaseBank,uM3SpareBank,uM3Carry;
const uint M3_SCOPE_ROWS=14u,M3_SCOPE_AUDIT=15u;
const uint M3_CONTROL_MIRROR=6144u,M3_AUTH_MATCH=81u,M3_Q_PASS_COUNT=82u;
void m3ScopeMetadata(){
    uint descriptor=m3RoleAddress(uM3Role);
    m3RoleInterval(uM3Role);
    if(W[descriptor]==0u||W[descriptor+7u]==0u)return;
    if(W[0]!=0u||W[16]!=1u||W[17]!=1u){qfail(15u);return;}
    plusHeader(72u,1u);if(W[0]!=0u)return;
    C[uHeader+76u]=uM3WriteBank;C[uHeader+77u]=uM3BaseBank;C[uHeader+78u]=uM3SpareBank;
    C[uHeader+79u]=2u;C[uHeader+80u]=1u;C[uHeader+81u]=4u;C[uHeader+47u]=uM3Carry;
    uint selected=uM3Role<=3u?uM3Role:m3PlanGet(M3_PLAN_SELECTED);
    C[uHeader+150u]=selected;C[uHeader+151u]=selected-1u;
    C[uHeader+152u]=W[descriptor+5u];C[uHeader+153u]=W[descriptor+6u];C[uHeader+158u]=6u;
    if(W[1]<uEventStart){qfail(15u);return;}
    W[descriptor+M3_SCOPE_ROWS]=W[1]-uEventStart;W[descriptor+M3_SCOPE_AUDIT]=1u;
}
uint m3SelectedRows(){return W[m3RoleAddress(m3PlanGet(M3_PLAN_SELECTED))+M3_SCOPE_ROWS];}
bool m3ScopeEnabled(){uint at=m3RoleAddress(uM3Role);return W[at]!=0u&&W[at+7u]!=0u;}
