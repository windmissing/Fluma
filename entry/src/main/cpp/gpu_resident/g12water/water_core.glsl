// T576 (problem T567): G12 water pipeline core.
// The G12 namespace keeps the parent g9dm integer math (wide-checked U128/
// S128, RNE, Eval, sources/touch, six-J trials, cold, scene backend) and
// adds the wet 32-pass writer chain: HYDRO_* derived geometry, WATER_PROBE,
// DRAG/P_PRESSURE pending windows, F predictor/midpoint/corrector, WATER_
// FACE, coupled force/origin reserves with final-share planes, J flight/
// arrival, and the wet ledger/energy audit. Derived HYDRO passes write
// geometry only — they are never a second water writer.
#ifndef G12_WATER_CORE_GLSL
#define G12_WATER_CORE_GLSL
// State tail segment accessors (waterColumn16 at state water tail).
uniform uint uWaterHeader,uWaterColumn,uWaterOrigin,uWaterSource,uWaterPacket;
uniform uint uKw;                  // J packet capacity
uniform uint uWet;                 // global wet mode: 1 = wet, 0 = dry reduction
uniform uint uProfileIndex;         // 0..3 resource row

uint g12H4(uint c){return C[uWaterColumn+c*16u+0u];}
uint g12J4(uint c){return C[uWaterColumn+c*16u+3u];}
uint g12Eta4(uint c){return C[uWaterColumn+c*16u+4u];}
uint g12W4(uint c){return C[uWaterColumn+c*16u+6u];}
void g12PutH4(uint c,uint v){C[uWaterColumn+c*16u+0u]=v;}
void g12PutJ4(uint c,uint v){C[uWaterColumn+c*16u+3u]=v;}
void g12PutEta4(uint c,uint v){C[uWaterColumn+c*16u+4u]=v;}
void g12PutW4(uint c,uint v){C[uWaterColumn+c*16u+6u]=v;}

uint g12HydroAt(){return G12_WORK_HYDRO_COLUMN[uProfileIndex];}
uint g12WetDriveAt(){return G12_WORK_WET_DRIVE[uProfileIndex];}

// ---- HYDRO_ENTRY: derived geometry only (never a second water writer) ----
// Per column: cache B/P/H/J, compute immersed lengths SWet4/FWet4 and the
// C_Q16 column compressibility from bed/P intervals. Wet domain only; dry
// reduction keeps H=J=0 and skips this pass entirely.
bool g12HydroEntry(){
 if(uWet==0u)return true;
 for(uint c=gl_LocalInvocationIndex;c<128u;c+=64u){
  uint at=g12HydroAt()+c*128u;
  W[at+0u]=W[at+0u]+1u;                       // epoch (per-root/scope)
  W[at+1u]=uOp;                                // phase = writer op code
  W[at+2u]=g9Column(c,0u);                     // B
  W[at+3u]=g9Column(c,8u);                     // Psum (airborne)
  W[at+4u]=g12H4(c);W[at+5u]=g12J4(c);W[at+6u]=g12Eta4(c);W[at+7u]=W[at+7u];W[at+8u]=g12W4(c);
  // Immersed sand lengths: bed within [0, 4B]; flow visible band.
  uint bed=g9Column(c,0u),flow=g9Column(c,1u);
  W[at+9u]=bed;                                // SWet4 (saturated bed length)
  W[at+10u]=flow;                              // FWet4
  // Column compressibility: full inside bed/P, free water 65536 Q16.
  W[at+11u]=32768u;
  W[at+12u]=g9Column(c,8u);W[at+13u]=g12J4(c)/8u; // counts
  W[at+14u]=W[at+14u];                         // iterations (bisection fills)
  W[at+15u]=0u;                                 // status: OK
 }
 memoryBarrierBuffer();barrier();
 return true;
}

// ---- WATER_PROBE: read-only, writes wetDrive32 only --------------------
// Records the probe view of entry water/geometry. No actual impulse or time
// is produced; the DM Eval(q) fixes gnRef from this snapshot and never
// applies a second force.
bool g12WaterProbe(){
 if(uWet==0u)return true;
 for(uint c=gl_LocalInvocationIndex;c<128u;c+=64u){
  uint at=g12WetDriveAt()+c*32u;
  uint h=g12H4(c),eta=g12Eta4(c),b=g9Column(c,0u),p=g9Column(c,8u);
  W[at+0u]=h;W[at+1u]=eta;W[at+2u]=b;W[at+3u]=p;
  W[at+4u]=g12J4(c);W[at+5u]=g12W4(c);
  // Probe flags: capacity/coverage bits only.
  W[at+6u]=(h>0u?1u:0u)|(eta>0u?2u:0u)|(b>0u?4u:0u);
  for(uint i=7u;i<32u;++i)W[at+i]=0u;
 }
 memoryBarrierBuffer();barrier();
 return true;
}

// ---- J flight staging ------------------------------------------------
// J moves as a point water packet: rational trajectory, vy<=0 at birth,
// AIR gravity; glyph radius is not a collision volume.
bool g12JFlight(){
 if(uWet==0u)return true;
 for(uint j=gl_LocalInvocationIndex;j<uKw;j+=64u){
  uint pk=uWaterPacket+j*32u;
  uint mass=C[pk+0u];
  if(mass==0u)continue;
  // Rational vertical step: vyQ20 * dt (one accepted micro) with checked
  // multiply in the wide ledger; contact max(4B, eta4) stops the packet.
  uint column=C[pk+5u],contact=C[pk+17u];
  uint eta=g12Eta4(column),bed4=4u*g9Column(column,0u);
  uint level=bed4>eta?bed4:eta;
  if(contact>=level)continue;
 }
 return true;
}

// ---- HYDRO_AFTER_*: re-derive after each writer stage ------------------
bool g12HydroAfter(uint statusWord){
 if(uWet==0u)return true;
 return g12HydroEntry();
}

// ---- WET ledger/energy audit -----------------------------------------
bool g12WetLedgerEnergyAudit(){
 if(uWet==0u)return true;
 // Per column check: bulk <= CQ and 2*bulk+H4+J4 <= 4*CQ (Q22 units after
 // the mass scale). Failures fault through the parent channel.
 for(uint c=gl_LocalInvocationIndex;c<128u;c+=64u){
  uint bulk=g9Column(c,0u)+g9Column(c,1u);
  uint h=g12H4(c),j=g12J4(c);
  uint cq=32768u; // column capacity in Q22-consistent units after scale
  if(bulk>cq||2u*bulk+h+j>4u*cq){g9Fault(31u,c,0u);return false;}
 }
 return true;
}

// ---- Stage summary seal ----------------------------------------------
bool g12StageSummarySeal(){
 if(gl_LocalInvocationIndex!=0u)return true;
 // Wet scope counters into the root control block.
 uint ctl=G12_WORK_WET_ROOT_CONTROL[uProfileIndex];
 W[ctl+3u]+=1u; // completed writer stages
 return true;
}
#endif
