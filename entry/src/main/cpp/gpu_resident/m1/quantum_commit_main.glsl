void main(){uint i=gl_GlobalInvocationID.x;
 if(uPass<=20u){candidateMain();return;}
 if(uPass==27u){if(i==0u)quantumCompletion();return;}
 if(W[0]!=0u)return;
 if(uPass==21u){if(i==0u){W[19]=0xffffffffu;quantumAudit();}return;}
 if(uPass==22u){if(i<uWords)W[uSpare+i]=B[i];if(i<uK+uKw)W[uRows+i]=0u;if(i<127u)W[uEdges+16u*i+15u]=0u;return;}
 if(uPass==23u){if(i==0u){replayQuantum();if(W[0]==0u&&W[17]!=1u)qfail(15u);}return;}
 if(uPass==24u){if(i==0u)quantumMetadata();return;}
 if(uPass==25u){if(i==0u)packPublication();return;}
 if(uPass==26u){if(i==0u&&!auditPublication())qfail(15u);return;}
 if(uPass==28u){if(i<uWords)E[i]=C[i];return;}
}
