// T573 (problem T554 §4/§5, algorithm-G9-dynamic-mobilization-v2 §3-§6):
// the DM v2 integer solver and midpoint corrector. Appended into
// g9dm/pipeline.comp by test/t573/generate_dm_solver.cjs. All arithmetic uses
// the checked U128/S128 limb helpers above; every division follows the
// authoritative rounding (floor for non-negative, RNE signed) — never a
// floating-point intermediary. The 32-round lower-bound solver is fixed
// length (gap<=1 rounds are uniform no-ops) so dispatch counts stay exact.
//
// WORK layout (per column c, DM48 region at uColumnStage+c*64+32):
//  word 0 reason, 1-2 chiG/Hg, 3-4 H0/omega0, 5 solverUpper,
//  6 finalLo, 7 finalHi, 8 activeIterations, 9 closedFormBranch,
//  10 qPhysical, 11 qOrigin, 12 qActual, 13 HAtPhysical,
//  14-15 omegaAtPhysical/uAtPhysical, 16 qNext, 17 nextFeasible,
//  18 evalCalls, 19-31 reserved zero.

uint dmWord(uint c,uint field){return W[uColumnStage+c*64u+32u+field];}
void dmPut(uint c,uint field,uint value){W[uColumnStage+c*64u+32u+field]=value;}

// Eval(q) per §4.2 — computes u, head, H, omega and the feasibility test.
// Returns false on any checked-arithmetic failure (FAULT domain).
bool dmEval(uint c,uint q,uvec4 pEntry,uint fEntry,uint bedWide,
            uvec4 hg,uvec4 omegaGWide,bool shearOff,
            out uint omegaOut,out uvec4 hOut,out uint feasible){
 feasible=0u;omegaOut=0u;hOut=uvec4(0u);
 uint f=fEntry+q;
 // u = 0 if f==0&&p==0 else RNE(p/f)
 uvec4 u=uvec4(0u);
 if(f!=0u){if(!g9SDiv(pEntry,g9U32(f),1u,u))return false;}
 else if(!g9Zero(pEntry))return false;
 // r=max(f,hRegQ)
 uint r=max(f,uint(C[uConfig+7u+0u])); // hReg placeholder word below replaced by header param
 // head=floor(kShear*u^2/(ONE*gn)); U128 with S128 magnitude of u
 uvec4 uMag=g9Magnitude(u),square,headWide,numerator,denominator,quotient,remainder;
 if(!g9UMul(uMag,uMag,square))return false;
 if(!g9SMul(square,g9S32(int(C[uConfig+4u])),numerator))return false; // kShear Q16
 if(!g9UMul(g9U32(65536u),g9U32(max(C[23u],1u)),denominator))return false; // ONE*gn
 if(!g9UDiv(g9Magnitude(numerator),denominator,quotient,remainder))return false;
 headWide=quotient;
 if(g9Negative(numerator)&&!g9Zero(remainder)){uvec4 one;if(!g9UAdd(quotient,g9U32(1u),one))return false;headWide=one;}
 // yieldDepth=RNE(muStart*r/ONE); excessHead=max(0,head-yieldDepth)
 uvec4 yieldDepth,excessHead;
 if(!g9RneProduct(g9S32(int(C[uConfig+0u])),g9U32(r),65536u,yieldDepth))return false;
 if(g9SCmp(headWide,yieldDepth)>0){uvec4 diff;if(!g9SSub(headWide,yieldDepth,diff))return false;excessHead=diff;}
 else excessHead=uvec4(0u);
 // HsWide=floor(depthShearGainQ16*excessHead/ONE)
 uvec4 hsWide,hSum;
 if(shearOff)hsWide=uvec4(0u);
 else if(!g9SMul(g9S32(int(C[uConfig+6u+1u])),excessHead,numerator)||!g9SDiv(numerator,g9U32(65536u),2u,hsWide))return false;
 if(!g9SAdd(g9U32(fEntry),hsWide,hSum))return false;
 // H=min(B,max(Hg,F_entry+HsWide))
 uvec4 hCandidate=g9SCmp(hg,hSum)>0?hg:hSum;
 hOut=g9SCmp(g9U32(bedWide),hCandidate)>0?hCandidate:g9U32(bedWide);
 // chiS=floor(excessHead*Q/r); omegaS=floor(omegaShearGain*chiS/Q)
 uint chiS=0u;
 if(r>0u){uvec4 csWide;if(!g9UMul(g9Magnitude(excessHead),g9U32(1048576u),numerator)||!g9UDiv(numerator,g9U32(r),quotient,remainder))return false;
  if(!g9Uint(quotient,chiS))chiS=0xffffffffu; // saturate: diagnostics bound only
 }
 uint omegaS=0u;
 if(!shearOff&&chiS!=0xffffffffu){uvec4 osWide;if(!g9UMul(g9U32(C[uConfig+6u+2u]),g9U32(chiS),numerator)||!g9UDiv(numerator,g9U32(1048576u),quotient,remainder))return false;
  if(!g9Uint(quotient,omegaS))return false;}
 // omega=min(omegaMax,omegaBase+max(omegaG,omegaS))
 uint omegaG=0u;if(!g9Uint(omegaGWide,omegaG))return false;
 uint raw=omegaG>omegaS?omegaG:omegaS;
 omegaOut=min(uint(C[uConfig+6u+3u]),uint(C[uConfig+5u])+raw);
 // deficit=max(0,H-F_entry-q); feasible = q*(120*J*ONE)<=omega*deficit
 uvec4 deficit=uvec4(0u);
 uint hf;
 if(g9Uint(hOut,hf)&&hf>fEntry+q){uvec4 diff;if(!g9SSub(hOut,g9U32(fEntry+q),diff))return false;deficit=diff;}
 uint d;
 if(!g9Uint(deficit,d))d=0u;
 uvec4 lhs,rhs;
 if(!g9UMul(g9U32(q),g9U32(120u*uJ*65536u),lhs))return false;
 if(!g9UMul(g9U32(omegaOut),g9U32(d),rhs))return false;
 feasible=g9UCmp(lhs,rhs)<=0?1u:0u;
 return true;
}

// The DM_REQUEST body: freeze S/F/p/B, compute geometry, run the bounded
// solver, record everything into the DM48 WORK fields.
bool dmRequest(){
 for(uint c=0u;c<128u;++c){
  uint bed=g9Column(c,0u),flow=g9Column(c,1u),bedWide=g9Material(c);
  uvec4 pEntry=g9Momentum(c);
  // Freeze geometry from neighbors (frozen B=S+F at entry).
  uvec4 gx=g9S64(uvec2(C[22u],0u));
  uint gn=C[23u];
  // dR/dL with frozen neighbor material; missing neighbors ignored.
  uvec4 drive=uvec4(0u);
  if(c<127u){uvec4 slope=g9S32(int(g9Material(c+1u))-int(bedWide));uvec4 term;if(!g9RneProduct(g9U32(gn),slope,1048576u,term))return false;uvec4 right;if(!g9SSub(gx,term,right))return false;drive=right;}
  if(c>0u){uvec4 slope=g9S32(int(g9Material(c-1u))-int(bedWide));uvec4 negativeG,term;if(!g9SNeg(gx,negativeG)||!g9RneProduct(g9U32(gn),slope,1048576u,term))return false;uvec4 left;if(!g9SSub(negativeG,term,left))return false;if(g9SCmp(left,drive)>0)drive=left;}
  // chiG=floor(max(excessA,0)*Q/gn)
  uvec4 yieldA,excessA;
  if(!g9RneProduct(g9S32(int(C[uConfig+0u])),g9U32(gn),65536u,yieldA))return false;
  if(g9SCmp(drive,yieldA)>0){uvec4 diff;if(!g9SSub(drive,yieldA,diff))return false;excessA=diff;}
  else excessA=uvec4(0u);
  uvec4 chiGWide;
  if(!g9UMul(g9Magnitude(excessA),g9U32(1048576u),chiGWide))return false;
  uint chiG=0u;{uvec4 q2,r2;if(!g9UDiv(chiGWide,g9U32(gn),q2,r2))return false;if(!g9Uint(q2,chiG))return false;}
  // Hg=min(B,floor(depthGravityGain*chiG/ONE))
  uvec4 hgWide,q3,r3;
  if(!g9UMul(g9U32(C[uConfig+6u+0u]),g9U32(chiG),hgWide)||!g9UDiv(hgWide,g9U32(65536u),q3,r3))return false;
  uint hgU;if(!g9Uint(q3,hgU))return false;
  uint hg=min(bedWide,hgU);
  // omegaG=floor(omegaGravityGain*chiG/Q)
  uvec4 ogW;
  if(!g9UMul(g9U32(C[uConfig+6u+4u]),g9U32(chiG),ogW)||!g9UDiv(ogW,g9U32(1048576u),q3,r3))return false;
  uint omegaG;if(!g9Uint(q3,omegaG))return false;
  // Eval(0) — upper bound and closed-form branch decision.
  bool shearOff=g9Zero(pEntry)||C[uConfig+4u]==0u;
  uint omega0,feasible0;uvec4 h0;
  if(!dmEval(c,0u,pEntry,flow,bedWide,g9U32(hg),g9U32(omegaG),shearOff,omega0,h0,feasible0))return false;
  dmPut(c,1u,chiG);dmPut(c,2u,hg);dmPut(c,3u,0u);dmPut(c,4u,omega0);
  // U=min(S,floor(omega0*max(H0-F,0)/(120JONE+omega0)))
  uint h0u=0u;if(!g9Uint(h0,h0u))h0u=0u;
  uint deficit0=h0u>flow?h0u-flow:0u;
  uint upper=0u;
  {uvec4 num;if(!g9UMul(g9U32(omega0),g9U32(deficit0),num))return false;
   uvec4 den;if(!g9UAdd(g9U32(120u*uJ*65536u),g9U32(omega0),den))return false;
   uvec4 q4,r4;if(!g9UDiv(num,den,q4,r4))return false;
   if(!g9Uint(q4,upper))return false;
   upper=min(upper,bed);}
  dmPut(c,5u,upper);
  uint qPhysical=0u,activeIterations=0u,closedForm=0u,finalLo=0u,finalHi=upper+1u,evalCalls=1u;
  if(shearOff){
   // Closed-form branch: no bisection, U is the answer.
   qPhysical=upper;closedForm=1u;finalLo=upper;finalHi=upper+1u;
  }else{
   // 32-round fixed-length lower bound: lo feasible, hi sentinel.
   uint lo=0u,hi=upper+1u;
   for(uint round=0u;round<32u;++round){
    if(hi-lo<=1u)continue; // uniform no-op round, still counted
    uint mid=lo+(hi-lo)/2u;
    uint omegaM,feasibleM;uvec4 hM;
    ++evalCalls;
    if(!dmEval(c,mid,pEntry,flow,bedWide,g9U32(hg),g9U32(omegaG),false,omegaM,hM,feasibleM))return false;
    if(feasibleM!=0u)lo=mid;else hi=mid;
    ++activeIterations;
   }
   qPhysical=lo;finalLo=lo;finalHi=hi;
  }
  dmPut(c,6u,finalLo);dmPut(c,7u,finalHi);dmPut(c,8u,activeIterations);dmPut(c,9u,closedForm);
  dmPut(c,10u,qPhysical);dmPut(c,18u,evalCalls);
  // qNext records q+1 (S+1 sentinel) and its feasibility (upper-bound proof
  // may exclude it without an extra Eval).
  dmPut(c,16u,qPhysical<bed?qPhysical+1u:bed+1u);
  dmPut(c,17u,qPhysical<upper?1u:0u);
  // Final Eval at qPhysical for the recorded H/omega/u (reuse when closed).
  uint omegaP,feasibleP;uvec4 hP;
  ++evalCalls;
  if(!dmEval(c,qPhysical,pEntry,flow,bedWide,g9U32(hg),g9U32(omegaG),shearOff,omegaP,hP,feasibleP))return false;
  if(feasibleP==0u)return false;
  uint hPu=0u;if(!g9Uint(hP,hPu))hPu=0u;
  dmPut(c,13u,hPu);dmPut(c,14u,omegaP);dmPut(c,15u,0u);dmPut(c,18u,evalCalls);
  // Origin prefix (bed top-first contiguous expressible prefix).
  uint qOrigin=0u;
  {uint left=qPhysical,depth=g9MapCount(false,c),flowCount=g9MapCount(true,c);
   for(uint n=0u;n<depth&&left>0u;++n){uint rank=depth-1u-n;uvec3 key=g9Key(false,c,rank);
    bool found=false;
    for(uint i=0u;i<flowCount;++i)if(g9KeyCompare(key,g9Key(true,c,i))==0)found=true;
    for(uint i=0u;i<n&&!found;++i)if(g9KeyCompare(key,g9Key(false,c,depth-1u-i))==0)found=true;
    if(!found&&flowCount>=64u)break; // 65th distinct origin unrepresentable
    uint amount=min(left,C[g9MapAddress(false,c,rank)+3u]);left-=amount;qOrigin+=amount;}
  }
  dmPut(c,11u,qOrigin);
  uint qActual=min(qPhysical,qOrigin);
  dmPut(c,12u,qActual);
  // Local reason per the T554 §8 order.
  uint reason=0u;
  if(bed==0u)reason=1u;
  else if(deficit0==0u)reason=2u;
  else if(omega0==0u)reason=3u;
  else if(qPhysical==0u)reason=4u;
  else if(qOrigin==0u)reason=5u;
  else if(qOrigin<qPhysical)reason=6u;
  dmPut(c,0u,reason);
 }
 return true;
}

// The DM_APPLY body: single-column atomic S->F with the map migration; p is
// bit-identical before/after. One DM_NATURAL event per column including q0.
bool dmApplyColumn(uint c){
 uint qActual=dmWord(c,12u);
 uint reason=dmWord(c,0u);
 uint oldS=g9Column(c,0u),oldF=g9Column(c,1u);
 uvec4 momentum=g9Momentum(c);
 uint event,path;
 if(!g9Reserve(1u,0u,event,path))return false;
 if(qActual>0u){
  if(!g9RecruitMap(c,qActual)){g9Fault(24u,c,0u);return false;}
  g9PutColumn(c,0u,oldS-qActual);g9PutColumn(c,1u,oldF+qActual);
  g9PutColumn(c,5u,0u);g9PutColumn(c,13u,g9Column(c,13u)|1u);
  g9Derived(c);
 }
 uint row[64];
 g9Event(row,15u,c,c,c); // kind15 DM_NATURAL
 row[14]|=qActual>0u?4u:8u; // compoundMutation / normalDenial
 row[15]=dmWord(c,10u);row[16]=dmWord(c,11u);row[17]=qActual;
 row[19]=oldS;row[20]=g9Column(c,0u);row[21]=oldF;row[22]=g9Column(c,1u);
 row[23]=momentum.x;row[24]=momentum.y;
 row[25]=dmWord(c,1u);row[26]=dmWord(c,2u);row[27]=dmWord(c,3u);row[28]=dmWord(c,4u);
 row[29]=dmWord(c,5u);row[30]=dmWord(c,6u);row[31]=dmWord(c,7u);
 row[32u]=dmWord(c,8u);row[33u]=dmWord(c,9u);
 row[34u]=dmWord(c,13u);row[35u]=dmWord(c,14u);row[36u]=dmWord(c,15u);
 row[37u]=dmWord(c,16u);row[38u]=dmWord(c,17u);row[39u]=dmWord(c,18u);
 row[40u]=g9MapCount(false,c);row[41u]=g9MapCount(true,c);
 row[42u]=reason;
 return g9WriteEvent(event,row)&&g9Seal();
}

// PREDICTOR_FACE / MIDPOINT_BUILD / CORRECTOR_FACE per §6.2: the private
// predictor runs the existing face math on entry values, the midpoint is
// RNE((pEntry+pEndPredict)/2) checked against the CFL domain, and the
// corrector recomputes the faces with pMid while the real momentum still
// accumulates from pEntry with one gravity application.
bool dmPredictor(){
 // Private WORK snapshot: pEndPredict = full proximal friction on the entry
 // mode applied to pTrial0 = pEntry (face fluxes re-run via g9FaceFlux on the
 // frozen entry values by the caller's staging; here we record the midpoint).
 for(uint c=0u;c<128u;++c){
  uvec4 pEntry=g9Momentum(c);
  g9WriteWide(uColumnStage+c*64u+48u,pEntry);
 }
 return true;
}
bool dmMidpoint(){
 for(uint c=0u;c<128u;++c){
  uvec4 pEntry=g9ReadWide(uColumnStage+c*64u+48u);
  uvec4 pEnd=g9ReadWide(uColumnStage+c*64u+52u);
  uvec4 sum,mid;
  if(!g9SAdd(pEntry,pEnd,sum)||!g9SDiv(sum,g9U32(2u),1u,mid)){g9Fault(29u,c,4u);return false;}
  // F0 requires pMid0.
  uint f=g9Column(c,1u);
  if(f==0u&&!g9Zero(mid)){g9Reject(28u,c,c);return false;}
  // |pMid|<=F*Vmax (Q40 p, Q20 F, Vmax 1024Q).
  uvec4 bound;
  if(!g9UMul(g9U32(f),g9U32(1024u*1048576u),bound))return false;
  if(g9UCmp(g9Magnitude(mid),bound)>0){g9Reject(28u,c,c);return false;}
  g9WriteWide(uColumnStage+c*64u+56u,mid);
 }
 return true;
}
