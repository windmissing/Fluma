// Replay binds the candidate at binding0 for a full-word comparison, while
// the immutable CURRENT header remains in this root-owned scratch segment.
uniform uint uM3ReplayPass;
const uint M3_BASE_HEADER=4096u;
uint m3SourceBase(uint field){return uM3ReplayPass!=0u?m3Get(M3_BASE_HEADER+field):B[uHeader+field];}
