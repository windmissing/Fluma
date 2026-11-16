#ifdef M3_FIXED_OPCODE
const uint uM3Opcode=M3_FIXED_OPCODE;
#else
uniform uint uM3Opcode;
#endif
uniform uint uM3FullBranchClear,uM3ScopeRows;
// Values are the generated Opcode order; contracts.cpp checks this wire use.
const uint M3_OP_CORE=0u,M3_OP_PACK=1u,M3_OP_PLAN0=2u,M3_OP_PLAN1=3u,M3_OP_PLAN2=4u,M3_OP_PLAN3=5u,M3_OP_PROMOTE=6u,M3_OP_PUB_AUDIT=7u,M3_OP_FINALIZE=8u,M3_OP_INIT=9u,M3_OP_SEAL=10u,M3_OP_COMPARE=11u,M3_OP_WATER_MATCH=12u,M3_OP_WATER_AUDIT=13u,M3_OP_WATER_TARGET=14u;
void m3QCopyBase(){for(uint i=gl_LocalInvocationIndex;i<uWords;i+=64u)C[i]=B[i];memoryBarrierBuffer();barrier();}
bool m3AuditControlMirror(){
    uint selected=m3PlanGet(M3_PLAN_SELECTED);
    for(uint slot=0u;slot<6u;++slot){
        uint at=uM3Scratch+M3_CONTROL_MIRROR+slot*128u,role=slot<3u?slot+1u:slot==3u?selected:slot;
        if(W[at]!=876825680u||W[at+1u]!=4u||W[at+2u]!=(slot==3u?25u:24u)||W[at+3u]!=2u||W[at+4u]!=uNonce.x||W[at+5u]!=uNonce.y||W[at+6u]!=role||W[at+7u]!=role||W[at+31u]!=3235774468u)return false;
        for(uint i=0u;i<4u;++i)if(W[at+12u+i]!=m3QBase(14u+i))return false;
        if(W[at+20u]!=uM3Omega/120u||W[at+29u]!=0u||W[at+30u]!=0u||W[at+34u]>W[at+35u]||W[at+35u]>5u)return false;
        for(uint i=48u;i<128u;++i)if(W[at+i]!=0u)return false;
        if(slot==3u&&(W[at+37u]!=selected||W[at+21u]!=m3PlanGet(M3_PLAN_STATUS)||W[at+34u]!=W[at+35u]||W[at+36u]!=(selected==0u?0u:m3SelectedRows())))return false;
    }
    return true;
}
bool m3BlockedDiagnosticWord(uint i){
    if(i<uHeader)return false;uint h=i-uHeader;
    return h==12u||h==13u||h==18u||h==19u||h==49u||h==50u||(h>=76u&&h<=81u)||(h>=150u&&h<=153u)||h==158u;
}
bool m3AuditBlockedCandidate(){
    if(m3PlanGet(M3_PLAN_SELECTED)!=0u||m3PlanGet(M3_PLAN_STATUS)!=3u||C[uHeader+80u]!=5u||C[uHeader+150u]!=0u||C[uHeader+151u]!=3u)return false;
    for(uint i=0u;i<uWords;++i)if(!m3BlockedDiagnosticWord(i)&&B[i]!=C[i])return false;
    return true;
}
void m3QuantumRoot(){
    uint lane=gl_LocalInvocationIndex,gid=gl_GlobalInvocationID.x;
    if(uM3Opcode==M3_OP_PLAN0){
        m3QCopyBase();
        for(uint i=lane;i<8192u;i+=64u)m3Put(i,0u);
        if(lane==0u)for(uint i=0u;i<32u;++i)W[i]=0u;
        memoryBarrierBuffer();barrier();
        if(lane==0u){
            m3Put(M3_Q_PASS_COUNT,1u);W[uM3CompareResult]=0xffffffffu;
            for(uint i=0u;i<256u;++i)m3Put(4096u+i,B[uHeader+i]);
            if(!m3AuditInventory()){qfail(15u);return;}m3PlanInitialize();
        }return;
    }
    if(gid==0u){if(m3Get(M3_Q_PASS_COUNT)!=uM3RootPass)qfail(28u);m3Put(M3_Q_PASS_COUNT,m3Get(M3_Q_PASS_COUNT)+1u);}
    if(uM3Opcode==M3_OP_INIT){
        m3QCopyBase();
        if(uM3FullBranchClear!=0u)for(uint i=lane;i<uM3ScopeRows*64u;i+=64u)E[uM3EventWord+uEventStart*64u+i]=0u;
        memoryBarrierBuffer();barrier();
        if(lane==0u){m3RoleInitialize(uM3Role);W[m3RoleAddress(uM3Role)+16u]=uM3WriteBank;}return;
    }
    if(uM3Opcode==M3_OP_CORE){
        if(!m3ScopeEnabled()){if(gid==0u&&uPass==24u&&W[m3RoleAddress(uM3Role)]!=0u)m3RoleInterval(uM3Role);return;}
        m3OccupiedDispatch();return;
    }
    if(uM3Opcode==M3_OP_WATER_TARGET||uM3Opcode==M3_OP_WATER_MATCH||uM3Opcode==M3_OP_WATER_AUDIT){
        if(gid!=0u||!m3ScopeEnabled()||W[0]!=0u)return;
        bool valid=uM3Opcode==M3_OP_WATER_TARGET?m3WaterTarget(int(W[5])):uM3Opcode==M3_OP_WATER_MATCH?m3WaterMatch():m3WaterProjectionAudit();
        if(!valid)qfail(30u);return;
    }
    if(uM3Opcode==M3_OP_COMPARE){
        if(m3PlanGet(M3_PLAN_SELECTED)==0u||W[0]!=0u)return;
        if(m3Get(M3_AUTH_MATCH)!=1u||W[m3RoleAddress(5u)+M3_SCOPE_AUDIT]!=1u||m3SelectedRows()!=W[m3RoleAddress(5u)+M3_SCOPE_ROWS]){if(gid==0u)qfail(16u);return;}
        if(!m3OwnScopeHeader(0u,m3PlanGet(M3_PLAN_SELECTED),uM3SelectedBank)||!m3OwnScopeHeader(1u,5u,W[m3RoleAddress(5u)+16u])||!m3OwnScopeHeader(2u,4u,W[m3RoleAddress(4u)+16u])){if(gid==0u)qfail(28u);return;}
        m3CompareCombined();return;
    }
    if(gid!=0u)return;
    if(uM3Opcode==M3_OP_FINALIZE){if(uM3Role==4u)m3AuthorityFinalize();else m3SelectScope();return;}
    if(uM3Opcode>=M3_OP_PLAN1&&uM3Opcode<=M3_OP_PLAN3){
        uint role=uM3Opcode-M3_OP_PLAN1+1u;m3PlanDecision(role);
        uint row[128];m3ControlRecord(row,24u,role);m3StoreControl(role-1u,row,true);return;
    }
    if(uM3Opcode==M3_OP_SEAL){m3SealQuantum();return;}
    if(W[0]!=0u)return;
    if(uM3Opcode==M3_OP_PROMOTE){
        if(m3PlanGet(M3_PLAN_SELECTED)!=0u)m3FinishSemanticCompare();
        else{
            // binding3 is private ACTUAL, never CURRENT or the public frame.
            E[uHeader+12u]=uNonce.x;E[uHeader+13u]=uNonce.y;E[uHeader+18u]=uControl.x;E[uHeader+19u]=uControl.y;E[uHeader+49u]=uFrame.x;E[uHeader+50u]=uFrame.y;
            E[uHeader+76u]=uM3CandidateBank;E[uHeader+77u]=uM3BaseBank;E[uHeader+78u]=uM3SpareBank;E[uHeader+79u]=2u;E[uHeader+80u]=5u;E[uHeader+81u]=4u;
            E[uHeader+150u]=0u;E[uHeader+151u]=3u;E[uHeader+152u]=m3PlanGet(M3_PLAN_EPOCH_LO);E[uHeader+153u]=m3PlanGet(M3_PLAN_EPOCH_HI);E[uHeader+158u]=6u;
        }
        if(W[0]!=0u)return;m3RecordPlannerEnd();if(!m3AuditControlMirror()){qfail(29u);return;}m3Put(M3_Q_AUDITS,32511u);return;
    }
    if(uM3Opcode==M3_OP_PACK){if(m3PlanGet(M3_PLAN_SELECTED)!=0u)m3PackPublication();return;}
    if(uM3Opcode==M3_OP_PUB_AUDIT){
        bool valid=m3PlanGet(M3_PLAN_SELECTED)==0u?m3AuditBlockedCandidate():m3AuditPublication();
        if(!valid)qfail(15u);else m3Put(M3_Q_AUDITS,m3Get(M3_Q_AUDITS)|256u);
    }
}
