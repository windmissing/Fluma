"""T670 actual captured DM/SPF stock and DM event/oracle differential, not a stepper."""
from pathlib import Path
import hashlib,importlib.util,json,mmap,struct,sys
ROOT=Path(__file__).resolve().parents[2]
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
oracle=load('existing_t556',ROOT/'test/t556/independent_oracle.py')
def signed(lo,hi):
 v=lo|(hi<<32);return v-2**64 if hi&0x80000000 else v
def event(row):
 assert row[2]==15 and row[3]<6,'DM_EVENT'
 S,F,p,gn,chi=row[15],row[16],signed(row[17],row[18]),row[20],row[21];J=1<<row[3]
 q,upper,iterations,H0,omega0,branch=oracle.solve_physical(F,p,gn,chi,S+F,S,J)
 H,omega,feasible,_,_,_=oracle.eval_q(q,F,p,gn,chi,S+F,J)
 expected={22:oracle.hg(chi,S+F),25:H0,26:omega0,27:q,30:H,31:omega,32:oracle.rne_div(p,F+q) if F+q else 0,35:iterations,46:upper,47:q,48:q+1,50:int(branch=='CLOSED_FORM')}
 for at,v in expected.items():assert row[at]==v&0xffffffff,('DM_ORACLE',at,row[at],v)
 actual=min(row[27],row[28]);assert row[29]==actual and row[41]==S-actual and row[42]==F+actual and row[43:45]==row[17:19],'DM_LOCAL_CONSERVATION'
 return expected
def check(path):
 path=Path(path);header=path.read_bytes()[:32768];assert len(header)==32768,'COLD_HEADER'
 w=struct.unpack('<8192I',header);dm=w[4]==843924551;spf=w[4]==826691667;assert dm or spf,'PROFILE_FAMILY'
 if dm:integrity=load('existing_dm_reader',ROOT/'tools/g9dm/read_cold.py').read(path)
 else:integrity={'profile':'FROZEN_SPF_T564','productPass':False}
 k=w[7];assert k in (192,512) and w[2]==8192 and (w[8]|w[9]<<32)==path.stat().st_size,'SIZE'
 seal=3235776802 if dm else 3235776769;tail=w[256:512];assert tail[3]==seal and tail[6] in (1,2,3,4),'SEALED_COMPLETION'
 stocks=[];checked=0
 with path.open('rb') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as raw:
  end=8192
  for i in range(w[6]):
   d=w[2048+32*i:2080+32*i];begin=d[8]|d[9]<<32;n=d[10]|d[11]<<32
   assert begin==end and begin+n<=len(raw)//4 and d[31]==seal,'SEGMENT_LAYOUT'
   digest=hashlib.sha256(raw[begin*4:(begin+n)*4]).digest();assert digest==struct.pack('<8I',*d[23:31]),'SEGMENT_SHA';end=begin+n
   if i<2:
    cols=[struct.unpack_from('<32I',raw,(begin+512+c*32)*4) for c in range(128)];packet=280576//4
    stocks.append(sum(c[0]+c[1] for c in cols)+sum(struct.unpack_from('<I',raw,(begin+packet+64*p)*4)[0] for p in range(k)))
    assert all(c[9]==c[0]+c[1] for c in cols),'COLUMN_T_DERIVED'
   elif dm and i in (2,3) and d[3]==2:
    for j in range(d[5]):
     row=struct.unpack_from('<64I',raw,(begin+j*64)*4)
     if row[2]==15:event(row);checked+=1
  assert end==len(raw)//4,'TRAILING_PAYLOAD'
 if tail[5]==2:assert stocks[0]==stocks[1],'MASS_CONSERVATION'
 if dm and tail[5]==2 and tail[6]==1:assert checked>0,'EMPTY_DM_EVIDENCE'
 return {'result':'CAPTURED_STOCK_AND_DM_DIFFERENTIAL_VALID','family':'DM' if dm else 'SPF','massBase':stocks[0],'massFinal':stocks[1],'checkedDmEvents':checked,'productPass':False,'timeGateDeviceVerdict':'SEPARATE_EXECUTION_RECEIPT_REQUIRED','integrity':integrity}
def selftest():
 row=[0]*64;row[2]=15;row[3]=0;S=40*oracle.Q;gn=512*oracle.Q;chi=oracle.Q//8;F=0;p=0
 q,u,it,h,o,b=oracle.solve_physical(F,p,gn,chi,S,S,1);H,om,*_=oracle.eval_q(q,F,p,gn,chi,S,1)
 for at,v in {15:S,16:F,20:gn,21:chi,22:oracle.hg(chi,S),25:h,26:o,27:q,28:q,29:q,30:H,31:om,32:0,35:it,41:S-q,42:q,46:u,47:q,48:q+1,50:1}.items():row[at]=v
 event(tuple(row));row[27]+=1
 try:event(tuple(row))
 except AssertionError:pass
 else:raise AssertionError('CORRUPT_Q_ACCEPTED')
 print(json.dumps({'result':'PASS','checks':['existing T556 event oracle positive','q mismatch rejects'],'device':'NOT_RUN'}))
if __name__=='__main__':
 if sys.argv[1:]==['--selftest']:selftest()
 else:print(json.dumps(check(sys.argv[1]),indent=2))
