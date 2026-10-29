"""Developer primitive differential, Python int/Fraction raw bits; no designer imports.
Tests C++ foundation only. GLSL/device and full M0 are NOT_EXECUTED.
"""
import random,struct,subprocess,json,hashlib,math
from fractions import Fraction
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
import argparse
parser=argparse.ArgumentParser();parser.add_argument('--output-dir',default='test/reports/t236');args=parser.parse_args()
OUT=ROOT/args.output_dir;OUT.mkdir(parents=True,exist_ok=True);LOCAL=ROOT/'.codex/local-test/t236-host'
rng=random.Random(236);MASK=2**32-1;M64=2**64-1;M128=2**128-1
rows=[];expected=[];groups={}
def words(v,n=2):return [(v>>(32*i))&MASK for i in range(n)]
def case(op,args,result):
 rows.append(struct.pack('<12I',op,*args,*([0]*(11-len(args)))))
 expected.append(struct.pack('<8I',*result,*([0]*(8-len(result)))))
 groups[op]=groups.get(op,0)+1
for _ in range(4000):
 a=rng.getrandbits(64);b=rng.getrandbits(64);p=a+b
 case(1,words(a)+words(b),[int(p<=M64)]+(words(p) if p<=M64 else [0x12345678,0x87654321]))
 case(2,words(a)+words(b),[int(a>=b)]+(words(a-b) if a>=b else [0x12345678,0x87654321]))
 aa=a&MASK;bb=b&MASK;case(3,[aa,bb],[1]+words(aa*bb))
 case(4,words(a)+words(b),[1]+words(a*b,4))
 n=rng.getrandbits(128);d=rng.getrandbits(32);q,r=divmod(n,d or 1)
 case(5,words(n,4)+[d],[1]+words(q,4)+[r] if d else [0,0,0,0,0,0])
for a in [-2**31,-2**31+1,-1001,-3,-1,0,1,3,1001,2**31-1]+[rng.randrange(-2**31,2**31) for _ in range(3000)]:
 b=rng.randrange(-2**31,2**31)
 for d in [1,2,3,120,2**20,2**32-1]:
  n=a*b;mag,r=divmod(abs(n),d);mag+=r*2>d or r*2==d and mag%2
  q=mag*(-1 if n<0 else 1);valid=-2**31<=q<2**31
  case(6,[a&MASK,b&MASK,d],[int(valid),q&MASK if valid else 0x12345678])
for a,b,d in [(1,1,2),(3,1,2),(-1,1,2),(-3,1,2),(2**31-1,2**31-1,0)]:
 if d:
  n=a*b;v,r=divmod(abs(n),d);v+=r*2>d or r*2==d and v%2;v*= -1 if n<0 else 1
  case(6,[a&MASK,b&MASK,d],[1,v&MASK])
 else:case(6,[a&MASK,b&MASK,d],[0,0x12345678])
values=[0.,-0.,math.inf,-math.inf,math.nan,2**-1074,2**-1022,2**-20,2**-21,-2**-1074,256.,math.nextafter(256.,math.inf),4294967295/2**20]+[struct.unpack('<d',rng.getrandbits(64).to_bytes(8,'little'))[0] for _ in range(8000)]
for x in values:
 raw=list(struct.unpack('<2I',struct.pack('<d',x)))
 for cap in [268435456,4294967295]:
  valid=math.isfinite(x) and x>=0
  v=math.floor(Fraction.from_float(x)*2**20) if valid else 0
  valid=valid and v<=cap
  case(7,raw+[cap],[int(valid),v if valid else 0x12345678])
for u in [0,1,2,2**20-1,2**20,2**24-1,2**24,2**24+1,2**28-1,2**28,2**32-1]+[rng.getrandbits(32) for _ in range(8000)]:
 f=struct.unpack('<I',struct.pack('<f',u/2**20))[0];case(8,[u],[1,f])
for u in [0,1,2**20-1,64*2**20-1,128*2**20-1]+[rng.randrange(128*2**20) for _ in range(8000)]:
 f=struct.unpack('<I',struct.pack('<f',u/2**20))[0];value=struct.unpack('<f',struct.pack('<I',f))[0]
 if value>=(u//2**20)+1:f-=1
 case(9,[u],[1,f])
for a,b in [(a,b) for a in [-2**31,-7,-1,0,1,7,2**31-1] for b in [-2**31,-3,-1,0,1,3,2**31-1]]+[(rng.randrange(-2**31,2**31),rng.randrange(-2**31,2**31)) for _ in range(4000)]:
 for op in [10,11]:
  value=(a//b if op==10 else -((-a)//b)) if b else 0
  valid=b!=0 and -2**31<=value<2**31
  case(op,[a&MASK,b&MASK],[int(valid),value&MASK if valid else 0x12345678])
for _ in range(4000):
 a,b=rng.getrandbits(128),rng.getrandbits(128);case(12,words(a,4)+words(b,4),[1,(-1 if a<b else 1 if a>b else 0)&MASK])
LOCAL.mkdir(parents=True,exist_ok=True);raw=b''.join(rows);expect=b''.join(expected);input_file=LOCAL/'numeric-input.bin';input_file.write_bytes(raw)
results=[]
for opt in ['Od','O2']:
 output=LOCAL/('numeric-'+opt+'.bin');subprocess.run([str(LOCAL/('foundation-'+opt+'.exe')),str(input_file),str(output)],check=True,cwd=ROOT)
 got=output.read_bytes();assert len(got)==len(expect)
 mismatches=[]
 for i in range(len(rows)):
  if got[i*32:(i+1)*32]!=expect[i*32:(i+1)*32]:
   mismatches.append({'row':i,'input':list(struct.unpack('<12I',rows[i])),'expected':list(struct.unpack('<8I',expect[i*32:(i+1)*32])),'actual':list(struct.unpack('<8I',got[i*32:(i+1)*32]))})
   if len(mismatches)>=10:break
 results.append({'optimization':opt,'cases':len(rows),'mismatches':mismatches,'resultSha256':hashlib.sha256(got).hexdigest()})
report={'revision':'T236_INTEGER_DIFFERENTIAL_V1','scope':'FOUNDATION_CPP_ONLY_NOT_M0_NOT_GPU','casesPerOptimization':len(rows),'groups':groups,'inputSha256':hashlib.sha256(raw).hexdigest(),'expectedSha256':hashlib.sha256(expect).hexdigest(),'results':results,'device':False,'GLSL':'NOT_EXECUTED'}
(OUT/'integer-differential.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2));assert all(not r['mismatches'] for r in results)
