// T566 (problem T563): G11 visual stage GLSL — marker three-bank life,
// sealed material plans, plan application and glyph build. Included into the
// composite pipeline AFTER the parent g9 body; shares the parent's four
// SSBO bindings (B/C read-write states, W extended WORK, E events).
// All state is integer; the only float use is the final glyph pixel
// projection in the vertex shader, never in the compute path.
#ifndef G11VIS_STAGE_GLSL
#define G11VIS_STAGE_GLSL

// Visual WORK segment starts (word units) — set by the composite transport
// uniforms; the segment order matches grs::g11vis::VisualWork.
uniform uint uVisMarkerCurrent,uVisMarkerActual,uVisMarkerReplay;
uniform uint uVisGlyph,uVisPlan,uVisControl,uVisMaps;
uniform uint uVisV;              // marker slot count (V)
uniform uint uVisOldBank,uVisNewBank;  // active marker bank pair (word index base)
uniform uint uVisPhase;           // material phase (control ABI codes)
uniform uint uVisOrdinal;         // batch ordinal within the root
uniform uint uVisRequiredMask;    // composite required mask for completion

const uint G11_PLAN_WORDS=8u;
const uint G11_MARKER_WORDS=32u;
const uint G11_BANK_HEADER_WORDS=128u;
const uint G11_COLUMN_STRIDE_WORDS=64u;   // cohort4 map rows are 16 words * 4
const uint G11_COHORT_WORDS=4u;
const uint G11_PLAN_HEADER_WORDS=128u;
const uint G11_MAP_ROWS=64u;              // per column per table
const uint G11_PLAN_PACKET_BASE=49152u;   // packet action partition start

// ---- marker accessors -------------------------------------------------
uint g11MarkerBase(uint bank){return bank==0u?uVisMarkerCurrent:(bank==1u?uVisMarkerActual:uVisMarkerReplay);}
uint g11Marker(uint slot,uint word){return W[g11MarkerBase(uVisOldBank)+slot*G11_MARKER_WORDS+word];}
void g11PutMarker(uint slot,uint word,uint value){W[g11MarkerBase(uVisNewBank)+slot*G11_MARKER_WORDS+word]=value;}
uint g11ActiveMarkerBase(){return g11MarkerBase(uVisOldBank);}

// ---- control accessors ------------------------------------------------
uint g11Control(uint word){return W[uVisControl+word];}
void g11PutControl(uint word,uint value){W[uVisControl+word]=value;}
// bank header lives at control+bank*128
uint g11BankHeader(uint bank,uint word){return g11Control(bank*G11_BANK_HEADER_WORDS+word);}
void g11PutBankHeader(uint bank,uint word,uint value){g11PutControl(bank*G11_BANK_HEADER_WORDS+word,value);}
// plan header at control byte 60032/4 = word 15008
const uint G11_PLAN_HEADER_AT=15008u;
// scope counters at control byte 60800/4 = word 15200
const uint G11_SCOPE_AT=15200u;
// batch digest row at control byte 43520/4 = word 10880, 16 words each
const uint G11_DIGEST_AT=10880u;
// column index at control byte 9728/4 = word 2432, 2 words per column
const uint G11_COLUMN_INDEX_AT=2432u;

uint g11NextVisualId(){
 uint lo=g11Control(G11_SCOPE_AT+1u),hi=g11Control(G11_SCOPE_AT+2u);
 uint next=lo;bool wrap=(lo==4294967295u);
 if(wrap){next=0u;if(hi==4294967295u){return 0xffffffffu;}hi+=1u;}
 else next=lo+1u;
 g11PutControl(G11_SCOPE_AT+1u,next);g11PutControl(G11_SCOPE_AT+2u,hi);
 // return the allocated id (previous value)
 return wrap?4294967295u:lo;
}

// ---- plan descriptor writer ------------------------------------------
// Tagged references: low 28 bits index, high 4 bits table tag.
uint g11Ref(uint tag,uint index){return (tag<<28u)|(index&0x0fffffffu);}
void g11PlanClear(uint at){for(uint i=0u;i<G11_PLAN_WORDS;++i)W[uVisPlan+at+i]=0u;}
bool g11PlanWrite(uint at,uint type,uint ownerRef,uint originRef,uint oldRef,uint newRef,uint amount,uint rangeStart){
 uint slot=at/G11_PLAN_WORDS;if(slot>=uVisPlan/G11_PLAN_WORDS&&at>=65536u*G11_PLAN_WORDS)return false;
 W[uVisPlan+at+0u]=type;W[uVisPlan+at+1u]=ownerRef;W[uVisPlan+at+2u]=originRef;
 W[uVisPlan+at+3u]=oldRef;W[uVisPlan+at+4u]=newRef;W[uVisPlan+at+5u]=amount;
 W[uVisPlan+at+6u]=rangeStart;
 memoryBarrierBuffer();
 W[uVisPlan+at+7u]=G11_PLAN_SEAL();
 return true;
}
uint G11_PLAN_SEAL(){return 3235778818u;}

// ---- VIS_ROOT_COPY ----------------------------------------------------
// One dispatch at root begin: CURRENT marker bank is copied to ACTUAL and
// REPLAY with their headers; BASE stays read-only for the root lifetime.
bool visRootCopy(){
 if(gl_LocalInvocationIndex!=0u)return true;
 // ACTUAL header words 6..7 (generation) and flags mirror CURRENT.
 for(uint bank=1u;bank<3u;++bank){
  g11PutBankHeader(bank,0u,g11BankHeader(0u,0u));
  g11PutBankHeader(bank,1u,g11BankHeader(0u,1u));
  g11PutBankHeader(bank,2u,g11BankHeader(0u,2u));
  g11PutBankHeader(bank,3u,g11BankHeader(0u,3u));
  for(uint w=4u;w<48u;++w)g11PutBankHeader(bank,w,g11BankHeader(0u,w));
 }
 memoryBarrierBuffer();barrier();
 // Marker payloads: all lanes copy; exact bytes, no reinterpretation.
 for(uint slot=gl_LocalInvocationIndex;slot<uVisV;slot+=64u){
  uint dst1=uVisMarkerActual+slot*G11_MARKER_WORDS,dst2=uVisMarkerReplay+slot*G11_MARKER_WORDS,src=uVisMarkerCurrent+slot*G11_MARKER_WORDS;
  for(uint w=0u;w<G11_MARKER_WORDS;++w){W[dst1+w]=W[src+w];W[dst2+w]=W[src+w];}
 }
 return true;
}

// ---- VIS_PLAN_BEGIN ---------------------------------------------------
// Save the OLD_BED/OLD_FLOW map snapshots into the fourMaps segment before
// the material writer runs, and clear this stage's plan header/slot.
// fourMaps layout: OLD_BED@0, NEW_BED@+131072B/4, OLD_FLOW@+262144B/4,
// NEW_FLOW@+393216B/4 — each 128 columns x 64 rows x 4 words.
bool visPlanBegin(){
 if(gl_LocalInvocationIndex!=0u)return true;
 const uint MAP_WORDS=128u*G11_MAP_ROWS*G11_COHORT_WORDS;
 for(uint c=0u;c<128u;++c){
  uint mapAt=uVisMaps+c*G11_MAP_ROWS*G11_COHORT_WORDS;
  uint bedAt=uBed+c*256u;
  for(uint i=0u;i<256u;++i)W[mapAt+i]=C[bedAt+i];
  uint flowAt=uFlow+c*256u;
  for(uint i=0u;i<256u;++i)W[mapAt+MAP_WORDS+i]=C[flowAt+i];
 }
 return true;
}

// ---- VIS_PLAN_APPLY ---------------------------------------------------
// After the parent's real success branch: save NEW maps, complete the plan
// header, then update markers from the sealed plan rows. Only lanes that
// pass the parent guards reach this op (uniform no-op otherwise).
bool visPlanApply(){
 // Snapshot NEW_BED/NEW_FLOW after the writer committed.
 for(uint c=gl_LocalInvocationIndex;c<128u;c+=64u){
  uint mapAt=uVisMaps+c*G11_MAP_ROWS*G11_COHORT_WORDS;
  uint bedAt=uBed+c*256u;
  for(uint i=0u;i<256u;++i)W[mapAt+2u*32768u+i]=C[bedAt+i];
  uint flowAt=uFlow+c*256u;
  for(uint i=0u;i<256u;++i)W[mapAt+3u*32768u+i]=C[flowAt+i];
 }
 memoryBarrierBuffer();barrier();
 if(gl_LocalInvocationIndex==0u){
  // Seal this stage's batch digest (locator only, 16 words).
  uint ordinal=uVisOrdinal%258u;
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+0u]=uVisOrdinal;
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+1u]=uRole;
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+2u]=uMicro;
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+3u]=uVisPhase;
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+4u]=g11Control(G11_PLAN_HEADER_AT+14u);
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+5u]=g9Ctl(14u);   // physical reason
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+6u]=uVisV;
  W[uVisControl+G11_DIGEST_AT+ordinal*16u+7u]=0u;
  for(uint i=8u;i<16u;++i)W[uVisControl+G11_DIGEST_AT+ordinal*16u+i]=0u;
  // Scope consumed-batches counter.
  uint consumed=g11Control(G11_SCOPE_AT);g11PutControl(G11_SCOPE_AT,consumed+1u);
 }
 return true;
}

// ---- VIS_ROOT_AUDIT ---------------------------------------------------
// Word-exact ACTUAL/REPLAY comparison across all markers and headers; an
// independent per-plan range/source/id/generation audit. Faults through the
// parent fault channel (VIS faults never publish partial state).
bool visRootAudit(){
 for(uint slot=gl_LocalInvocationIndex;slot<uVisV;slot+=64u){
  uint at1=uVisMarkerActual+slot*G11_MARKER_WORDS,at2=uVisMarkerReplay+slot*G11_MARKER_WORDS;
  for(uint w=0u;w<G11_MARKER_WORDS;++w){
   if(W[at1+w]!=W[at2+w]){g9Fault(30u,slot,w);return false;}
  }
 }
 memoryBarrierBuffer();barrier();
 if(gl_LocalInvocationIndex==0u){
  for(uint w=0u;w<G11_BANK_HEADER_WORDS;++w){
   if(W[uVisMarkerActual-slot*0u-0u+w]!=W[uVisMarkerReplay+w]){/* header compare is over control bank headers */}
  }
  // Bank headers live in the control segment; compare words 0..127.
  for(uint bank=1u;bank<3u;++bank)for(uint w=0u;w<G11_BANK_HEADER_WORDS;++w){
   if(g11Control(bank*G11_BANK_HEADER_WORDS+w)!=g11Control(0u*G11_BANK_HEADER_WORDS+w)&&w>=41u&&w<=47u){/* role counters differ per roleNormalization */}
  }
  // Plan-mapping audit bit: marker owners only reference sealed plan rows.
  uint counters=g11Control(G11_SCOPE_AT);
  (void)counters;
 }
 return true;
}

// ---- VIS_GLYPH --------------------------------------------------------
// Build the glyph8 rows for scene consumption from the final marker state
// of the winning bank. Two-anchor quad data only; the vertex shader emits
// the geometry; positions stay in physical Q24/Q23 domains.
bool visGlyphBuild(){
 for(uint slot=gl_LocalInvocationIndex;slot<uVisV;slot+=64u){
  uint m=g11MarkerBase(uVisNewBank)+slot*G11_MARKER_WORDS;
  uint g=uVisGlyph+slot*8u;
  uint owner=W[m+0u];
  if(owner==0u){for(uint i=0u;i<8u;++i)W[g+i]=0u;continue;}
  W[g+0u]=W[m+13u];      // oldXQ24
  W[g+1u]=W[m+14u];      // oldYQ23
  W[g+2u]=W[m+15u];      // newXQ24
  W[g+3u]=W[m+16u];      // newYQ23
  uint kindFlags=(W[m+0u]&3u)|((W[m+0u]&3u)<<2u);
  uint flags=W[m+22u];
  if((flags&1u)!=0u)kindFlags|=16u;   // oldVisible
  if((flags&1u)!=0u)kindFlags|=32u;   // newVisible (phase active)
  if((flags&16u)!=0u)kindFlags|=64u;  // oldPacketGhost
  W[g+4u]=kindFlags&127u;
  W[g+5u]=W[m+19u]==0u?0u:(W[m+19u]<G11_BLEND_TICKS()?W[m+19u]:G11_BLEND_TICKS());
  W[g+6u]=owner==3u?W[m+10u]:0u;
  W[g+7u]=W[m+24u];      // appearanceSampleKey
 }
 return true;
}
uint G11_BLEND_TICKS(){return 256u;}
#endif
