from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
D=ROOT/'entry/src/main/cpp/gpu_resident/m3b'
def function(s,name,body):
 at=s.index(name);begin=s.index('{',at);level=1;i=begin+1
 while level:
  if s[i]=='{':level+=1
  elif s[i]=='}':level-=1
  i+=1
 return s[:begin+1]+body+s[i-1:]
p=D/'controller.cpp';s=p.read_text(encoding='utf-8')
for name in ['ControllerV5::ColdBegin(','ControllerV5::ColdState(','ControllerV5::ColdPage(','ControllerV5::ColdEnd(']:
 s=function(s,name,'return "{\\\"revision\\\":\\\"T527_V5_BOUNDED_COLD_V1\\\",\\\"state\\\":\\\"PREFIX_DIGEST_POLICY_REQUIRED\\\"}";'.replace('\\\"','\\"'))
s=function(s,'ControllerV5::BuildCold(','// No allocation, GL capture or invented digest while the canonical input is unresolved.\n (void)now;')
p.write_text(s,encoding='utf-8')
p=D/'transport.cpp';s=p.read_text(encoding='utf-8');s=function(s,'TransportV5::CaptureColdV5(','(void)out;(void)capacity;(void)quota;stateWords=eventWords=0;return false; // PREFIX_DIGEST_POLICY_REQUIRED\n');p.write_text(s,encoding='utf-8')
for path in [D/'kernels.cmake']:
 s=path.read_text(encoding='utf-8').replace('SPECIALIZATION_0_14','SPECIALIZATION_0_18')
 path.write_text(s,encoding='utf-8')
