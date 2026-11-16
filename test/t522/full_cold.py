"""Versioned FULL_BRANCH extension applied after the normal V4 cold generator."""
def extend(s):
 s=s.replace('expectedState=0,requestWords=0;', 'expectedState=0,requestWords=0;uint32_t fullRows=0;')
 s=s.replace('stateWords=0,eventWords=0;};','stateWords=0,eventWords=0;uint32_t fullRows=0;};')
 s=s.replace('size_t stateWords){','size_t stateWords,uint32_t fullRows=0){')
 s=s.replace('words>MaxTraceBytes/4','words>(fullRows?cold::kMaximumFullTraceBytes:MaxTraceBytes)/4')
 s=s.replace('>TraceBudget)', '>(fullRows?cold::kTwoSlotFullTraceBytes:TraceBudget))')
 s=s.replace('s.expectedState=stateWords;', 's.expectedState=stateWords;s.fullRows=fullRows;')
 s=s.replace('QuotaSnapshotV3 quota;std::memcpy(quota.data(),c+256+state-2848,1024);', '''const size_t extra=s.fullRows?cold::kFullDescriptorWords+20*size_t(s.fullRows)*64:0;
  if(state<2848+extra)return false;
  if(s.fullRows&&s.fullRows!=10622&&s.fullRows!=11646&&s.fullRows!=25982&&s.fullRows!=27006)return false;
  const size_t physical=state-2848-extra;
  if(s.fullRows){const auto* m=c+256+physical+2848;if(m[0]!=0x3442464d||m[1]!=1||m[2]!=20||m[3]!=s.fullRows||(m[6]&~0xfffffu)||(m[7]&~15u))return false;for(size_t i=8;i<32;++i)if(m[i])return false;}
  QuotaSnapshotV3 quota;std::memcpy(quota.data(),c+256+physical,1024);''')
 s=s.replace('s.capacity,s.stateWords,s.eventWords}', 's.capacity,s.stateWords,s.eventWords,s.fullRows}')
 old='w.Raw("{\\"revision\\":\\"GRS_M3_BOUNDED_COLD_V1\\",\\"payloadRevision\\":\\"GRS_M3_TRACE_V4_QUOTA3_CONTROL128\\",\\"state\\":\\"TRACE_READY\\",\\"traceId\\":");'
 new='w.Raw("{\\"revision\\":\\"GRS_M3_BOUNDED_COLD_V1\\",\\"payloadRevision\\":");w.String(read.fullRows?"GRS_M3_FULL_BRANCH_V1":"GRS_M3_TRACE_V4_QUOTA3_CONTROL128",std::strlen(read.fullRows?"GRS_M3_FULL_BRANCH_V1":"GRS_M3_TRACE_V4_QUOTA3_CONTROL128"));w.Raw(",\\"state\\":\\"TRACE_READY\\",\\"traceId\\":");'
 assert old in s
 s=s.replace(old,new)
 s=s.replace('const auto* c=read.block;const auto words=', 'const size_t extra=read.fullRows?cold::kFullDescriptorWords+20*size_t(read.fullRows)*64:0;const size_t physical=read.stateWords-2848-extra;\n const auto* c=read.block;const auto words=')
 s=s.replace('read.stateWords-2848','physical')
 # Undo replacement of the declaration itself.
 s=s.replace('const size_t physical=physical-extra;', 'const size_t physical=read.stateWords-2848-extra;')
 anchor='w.Raw(",\\"eventsWordOffset\\":");'
 s=s.replace(anchor,'''if(read.fullRows){w.Raw(",\\"branchMetaWordOffset\\":");w.Number(physical+3104);w.Raw(",\\"branchControlWordOffset\\":");w.Number(physical+3136);w.Raw(",\\"branchEventsWordOffset\\":");w.Number(physical+11328);w.Raw(",\\"branchScopeRows\\":");w.Number(read.fullRows);}
 '''+anchor)
 return s
