"""Adapt lifecycle ownership to T527 descriptor payloads, preserving M2 ABI."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
D=ROOT/'entry/src/main/cpp/gpu_resident/m3b'
s=(D.parent/'m3a/cold_trace.h').read_text(encoding='utf-8').replace('m3a','m3b').replace('V4','V5').replace('GRS_M3_BOUNDED_COLD_V1','T527_V5_BOUNDED_COLD_V1')
s=s.replace('const char* result="FREE";','const char* result="FREE";')
s=s.replace('size_t peakCapacity=0;','size_t peakCapacity=0,detachedBytes=0;')
s=s.replace('size_t Capacity()const{size_t n=0;','size_t Capacity()const{size_t n=detachedBytes;')
a=s.index(' static bool Layout(');b=s.index(' void Finish(',a)
s=s[:a]+''' static bool Layout(const Slot& s,size_t state,size_t events){
  if(!s.block||s.capacity<s.requestWords||state+256!=s.requestWords||events!=0)return false;
  const auto* b=s.block.get();const auto* p=SelectProfile(b[8],b[9]);cold::Layout layout;
  if(!p||!cold::Layout::Make(*p,4,layout,s.fullRows!=0)||layout.words!=s.requestWords)return false;
  return cold::Validate(b,s.capacity,layout);
 }
'''+s[b:]
s=s.replace('const size_t pages=(256+s.stateWords+s.eventWords+PageWords-1)/PageWords;if(page>pages)',
'''const auto* p=SelectProfile(s.block[8],s.block[9]);cold::Layout layout;
   if(!p||!cold::Layout::Make(*p,4,layout,s.fullRows!=0))return "TRACE_FORMAT_REJECTED";
   const size_t pages=layout.Pages();if(page>=pages)''')
s=s.replace('std::unique_ptr<uint32_t[]> Trim(int i){auto& s=slots[i];if(s.state!=FREE||s.reader)return {};s.capacity=0;return std::move(s.block);}',
'''std::unique_ptr<uint32_t[]> Trim(int i){auto& s=slots[i];if(s.state!=FREE||s.reader||s.reserved||!s.block)return {};detachedBytes+=s.capacity*4;s.capacity=0;return std::move(s.block);}
 // Caller must reset/destroy the detached unique_ptr before this credit is
 // returned; moving ownership out of a slot is not a free acknowledgment.
 bool DetachedFreed(size_t bytes){if(bytes>detachedBytes)return false;detachedBytes-=bytes;return true;}''')
a=s.index('inline void TracePage(')
s=s[:a]+'''inline void TracePage(Writer& w,const TraceManager::Read& read,uint32_t page){
 const auto* p=SelectProfile(read.block[8],read.block[9]);cold::Layout layout;
 if(!p||!cold::Layout::Make(*p,4,layout,read.fullRows!=0)||!cold::Page(w,read.block,layout,read.id,read.scope,page)){
  w.Reset();w.Raw("{\\"state\\":\\"INCOMPLETE\\",\\"reason\\":\\"TRACE_FORMAT_REJECTED\\"}");
 }
}
}}}
'''
(D/'cold_trace.h').write_text(s,encoding='utf-8')
