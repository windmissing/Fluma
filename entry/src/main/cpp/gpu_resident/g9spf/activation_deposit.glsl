bool g9RneProduct(uvec4 a,uvec4 b,uint denominator,out uvec4 result){uvec4 product;return g9SMul(a,b,product)&&g9SDiv(product,g9U32(denominator),1u,result);}
bool g9Drive(uint c,out uvec4 drive){
 drive=uvec4(0u);uvec4 gx=g9S32(int(C[22u])),gn=g9U32(C[23u]),term;
 uint bedFlow=g9Material(c);
 if(c<127u){uvec4 slope=g9S32(int(g9Material(c+1u))-int(bedFlow));if(!g9RneProduct(gn,slope,1048576u,term)||!g9SSub(gx,term,term))return false;if(g9SCmp(term,drive)>0)drive=term;}
 if(c>0u){uvec4 slope=g9S32(int(g9Material(c-1u))-int(bedFlow)),negativeG;if(!g9SNeg(gx,negativeG)||!g9RneProduct(gn,slope,1048576u,term)||!g9SSub(negativeG,term,term))return false;if(g9SCmp(term,drive)>0)drive=term;}
 uint flow=g9Column(c,1u),shear=W[uConfig+4u],band=W[uConfig+2u];
 if(flow>0u&&shear>0u){uvec4 velocity,squared,numerator,denominator;
  if(!g9SDiv(g9Momentum(c),g9U32(flow),1u,velocity)||!g9SMul(velocity,velocity,squared)||!g9SMul(squared,g9U32(shear),numerator)||!g9UMul(g9U32(65536u),g9U32(max(flow,band)),denominator)||!g9SDiv(numerator,denominator,1u,term))return false;
  if(g9SCmp(term,drive)>0)drive=term;
 }return true;
}
bool g9StartThreshold(out uvec4 threshold){return g9RneProduct(g9U32(W[uConfig]),g9U32(C[23u]),65536u,threshold);}
bool g9ActivationRequest(){
 for(uint c=0u;c<128u;++c){uvec4 drive,threshold;
  if(!g9Drive(c,drive)||!g9StartThreshold(threshold)){g9Fault(29u,c,0u);return false;}
  uint q=0u,bed=g9Column(c,0u);
  if(g9SCmp(drive,threshold)>0){uvec4 numerator,quotient,remainder;if(!g9UMul(g9U32(min(bed,W[uConfig+2u])),g9U32(W[uConfig+3u]),numerator)||!g9UDiv(numerator,g9U32(65536u*120u*uJ),quotient,remainder)||!g9Uint(quotient,q)){g9Fault(29u,c,1u);return false;}q=min(q,bed);}
  g9PutColumn(c,10u,q);g9PutColumn(c,11u,0u);g9PutColumn(c,13u,uMicro==0u?W[uColumnStage+c*64u+21u]:0u);g9WriteWide(uHamilton+c*2048u+256u,drive);
 }
 return true;
}
bool g9Activate(uint c,uint requested,uint trigger){
 if(requested==0u)return true;
 if(!g9RecruitPreflight(c,requested)){g9Reject(11u,c,0u);return false;}
 uint event,path;if(!g9Reserve(1u,0u,event,path))return false;
 uint oldS=g9Column(c,0u),oldF=g9Column(c,1u),oldDepth=g9MapCount(false,c),oldCount=g9MapCount(true,c);uvec4 momentum=g9Momentum(c);
 if(!g9RecruitMap(c,requested)){g9Fault(24u,c,0u);return false;}
 g9PutColumn(c,0u,oldS-requested);g9PutColumn(c,1u,oldF+requested);g9PutColumn(c,5u,0u);g9PutColumn(c,11u,requested);g9PutColumn(c,13u,g9Column(c,13u)|1u);g9Derived(c);
 uint row[64];g9Event(row,G9_EVENT_ACTIVATE,c,c,c);row[14]|=4u;row[15]=requested;row[16]=requested;row[18]=trigger;row[19]=oldS;row[20]=oldS-requested;row[21]=oldF;row[22]=oldF+requested;
 row[23]=row[25]=momentum.x;row[24]=row[26]=momentum.y;row[27]=W[uConfig+2u];row[28]=oldDepth;row[29]=g9MapCount(false,c);row[30]=oldCount;row[31]=g9MapCount(true,c);
 uvec4 drive=g9ReadWide(uHamilton+c*2048u+256u);for(uint i=0u;i<4u;++i)row[32u+i]=drive[i];
 // At fixed B, potential and p are unchanged; adding zero-momentum mass
 // decreases p²/F monotonically. No interval tolerance is introduced.
 return g9WriteEvent(event,row)&&g9Seal();
}
bool g9ActivationApply(){for(uint c=0u;c<128u;++c)if(!g9Activate(c,g9Column(c,10u),1u))return false;return true;}
bool g9Friction(){
 for(uint c=0u;c<128u;++c){uint flow=g9Column(c,1u),mode=g9Column(c,4u),oldRest=g9Column(c,5u),rest=0u;uvec4 oldP=g9Momentum(c),staticBudget,dynamicBudget,product,numerator,denominator=g9U32(65536u*120u*uJ);
  if(!g9SMul(g9U32(C[23u]),g9U32(flow),product)||!g9SMul(product,g9U32(W[uConfig]),numerator)||!g9SDiv(numerator,denominator,0u,staticBudget)||!g9SMul(product,g9U32(W[uConfig+1u]),numerator)||!g9SDiv(numerator,denominator,0u,dynamicBudget)){g9Fault(29u,c,0u);return false;}
  uvec4 magnitude=g9Magnitude(oldP),newP=uvec4(0u);bool supported=mode==0u&&g9UCmp(magnitude,staticBudget)<=0;
  if(!supported&&g9UCmp(magnitude,dynamicBudget)>0){uvec4 difference;if(!g9USub(magnitude,dynamicBudget,difference)||!g9FromMagnitude(difference,g9Negative(oldP),newP)){g9Fault(29u,c,1u);return false;}}
  if(g9SCmp(g9Magnitude(newP),g9Magnitude(oldP))>0){g9Fault(29u,c,2u);return false;}
  uvec4 support;if(!g9SSub(newP,oldP,support)||!g9Accumulate(74u,support)){g9Fault(29u,c,3u);return false;}
  uvec4 drive,threshold;if(!g9Drive(c,drive)||!g9StartThreshold(threshold)){g9Fault(29u,c,4u);return false;}
  bool still=flow>0u&&g9Zero(newP)&&g9Column(c,13u)==0u&&g9SCmp(drive,threshold)<=0;
  if(still){uint increment=32u/uJ,settle=W[uConfig+5u];rest=oldRest>settle||increment>settle-oldRest?settle:oldRest+increment;}
  uint row[64];g9Event(row,G9_EVENT_FRICTION,c,c,c);row[15]=flow;row[16]=mode;row[17]=g9Zero(newP)?0u:1u;row[18]=supported?W[uConfig]:W[uConfig+1u];row[19]=oldRest;row[20]=rest;
  row[21]=oldP.x;row[22]=oldP.y;row[23]=newP.x;row[24]=newP.y;row[25]=staticBudget.x;row[26]=staticBudget.y;row[27]=dynamicBudget.x;row[28]=dynamicBudget.y;row[29]=support.x;row[30]=support.y;
  if(!g9Zero(support)||oldRest!=rest){if(!g9OneEvent(row))return false;}
  if(!g9PutMomentum(c,newP))return false;g9PutColumn(c,5u,rest);
  if(!still||rest<W[uConfig+5u])continue;
  if(!g9DepositPreflight(c)){g9PutColumn(c,12u,12u);continue;}
  uint oldS=g9Column(c,0u),depth=g9MapCount(false,c),count=g9MapCount(true,c);if(!g9DepositMap(c)){g9Fault(24u,c,5u);return false;}
  g9PutColumn(c,0u,oldS+flow);g9PutColumn(c,1u,0u);g9PutColumn(c,5u,0u);g9Derived(c);
  g9Event(row,G9_EVENT_DEPOSIT,c,c,c);row[14]|=4u;row[15]=flow;row[17]=oldS;row[18]=oldS+flow;row[19]=flow;row[21]=depth;row[22]=g9MapCount(false,c);row[23]=count;row[25]=rest;row[26]=W[uConfig+5u];if(!g9OneEvent(row))return false;
 }return true;
}
