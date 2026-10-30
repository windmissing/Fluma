from pathlib import Path
import sys,os,struct,subprocess
sys.path.insert(0,str(Path(__file__).parent))
import check_t279_quantum as ref
K,Kw=map(int,sys.argv[1:]);cases,water,col,h,n=ref.fixtures(K,Kw)
for case in cases:
 a=case['a'];a[h+124]=a[h+125]=1;a[h+14]=a[h+16]=a[h+33]=a[h+34];a[h+74]=a[h+33]
 si=sum(case['ledger'][i+6] for i in range(0,16*a[h+34],16) if case['ledger'][i+4]==0)
 wi=sum(case['ledger'][i+6] for i in range(0,16*a[h+34],16) if case['ledger'][i+4]==1)
 ref.pair(a,h+60,si);ref.pair(a,h+62,wi);ref.pair(a,h+64,ref.u64(a,h+56)-si);ref.pair(a,h+66,ref.u64(a,h+58)-wi)
out=ref.r/'.codex/local-test/t280';report=ref.r/'test/reports/t280';report.mkdir(exist_ok=True,parents=True)
ip=out/f'input-{K}-{Kw}.bin';op=out/f'output-{K}-{Kw}.bin'
with ip.open('wb') as f:
 f.write(struct.pack('<4I',279,K,Kw,len(cases)))
 for c in cases:
  for v in [c['p'],c['a'],c['ledger']]:f.write(struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
env=os.environ.copy();env['PATH']='C:/Users/admin/AppData/Local/Programs/CokePLUS;'+env['PATH']
p=subprocess.run([str(out/'replay.exe'),str(ip),str(op)],cwd=ref.r,env=env,text=True,capture_output=True,timeout=300)
(report/f'replay-{K}-{Kw}.txt').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr);assert p.returncode==0
raw=op.read_bytes();offset=16
def take(count):
 global offset
 v=list(struct.unpack_from('<'+'I'*count,raw,offset));offset+=count*4;return v
for c in cases:
 status=take(16);info=take(3);bank=take(n);e=take(info[2]);events=[e[i:i+64] for i in range(0,len(e),64)]
 if not c['error']:assert ref.u64(bank,h+16)==ref.u64(c['a'],h+16)+1
 normalized=bank.copy()
 if not c['error']:ref.pair(normalized,h+16,1)
 ref.verify(c,status,normalized,events,info,K,Kw,water,col,h)
print('FULL_REPLAY_CASES',len(cases),'PASS')
