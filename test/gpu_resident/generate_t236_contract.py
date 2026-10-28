"""T236 reproducible manifest bindings and independent Decimal angle table.
Does not import designer code or product numeric helpers. No device access.
"""
import argparse,hashlib,json,struct,math
from decimal import Decimal,localcontext,ROUND_FLOOR,ROUND_HALF_EVEN
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
DEST=ROOT/'entry/src/main/cpp/gpu_resident'
MAN=ROOT/'docs/analysis-T233-gpu-resident-M0-engineering-manifest-v1.json'
def atan(x):
 total=term=x;n=1
 while True:
  term=-term*x*x;n+=2;next_value=total+term/n
  if total==next_value:return total
  total=next_value

def sincos(x):
 s=st=x;c=ct=Decimal(1);n=1
 while True:
  st=-st*x*x/(2*n*(2*n+1));ct=-ct*x*x/((2*n-1)*2*n)
  ns,nc=s+st,c+ct
  if ns==s and nc==c:return s,c
  s,c=ns,nc;n+=1

def table(precision):
 with localcontext() as ctx:
  ctx.prec=precision
  pi=16*atan(Decimal(1)/5)-4*atan(Decimal(1)/239);q=Decimal(1<<20)
  words=[];max_error=Decimal(0);max_libm_error=Decimal(0)
  for j in range(-7680,7681):
   s,c=sincos(Decimal(j)*pi/(256*180));t=s/c
   tq=int((abs(t)*q).to_integral_value(rounding=ROUND_FLOOR))*(-1 if t<0 else 1)
   row=(tq,int((s*q).to_integral_value(rounding=ROUND_HALF_EVEN)),int((c*q).to_integral_value(rounding=ROUND_HALF_EVEN)),0)
   words.extend(row);max_error=max(max_error,abs(Decimal(tq)/q-t));max_libm_error=max(max_libm_error,abs(Decimal(tq)/q-Decimal.from_float(math.tan(j/256*math.pi/180))))
  return struct.pack('<'+'i'*len(words),*words),{'precision':precision,'pi':str(pi),'maxTanError':str(max_error),'maxLibmTanError':str(max_libm_error),'strictErrorBound':bool(max_error<1/q),'libmErrorBound':bool(max_libm_error<1/q),'Hq':605394,'Hfloor':605395,'strictTrueTan30Bound':bool(Decimal(605395)/q<=sincos(pi/6)[0]/sincos(pi/6)[1])}

def artifacts():
 raw=MAN.read_bytes();m=json.loads(raw.decode('utf-8-sig'));sha=hashlib.sha256(raw).hexdigest()
 lines=['// Generated from frozen T233 manifest. Do not hand edit.','#pragma once','#include <cstddef>','#include <cstdint>','namespace grs {','inline constexpr char kLayoutManifestSha256[]="'+sha+'";']
 for name,layout in m['layouts'].items():
  lines+=['namespace '+name+' {','inline constexpr size_t Words='+str(layout['wordCount'])+';']
  lines+=['inline constexpr size_t '+n+'='+str(i)+';' for i,n in enumerate(layout['words'])]
  lines+=['}']
 lines+=['inline constexpr int64_t '+name+'='+str(value)+';' for name,value in m['constants'].items()];lines+=['} // namespace grs','']
 a,proof=table(80);b,proof2=table(110);assert a==b and proof['strictErrorBound'] and proof2['strictTrueTan30Bound'] and proof2['libmErrorBound']
 angle_sha=hashlib.sha256(a).hexdigest();proof.update({'revision':'T236_DECIMAL_ANGLE_CERTIFICATE_V1','rows':15361,'bytes':len(a),'sha256':angle_sha,'secondPrecision':110,'precisionAgreement':a==b,'policy':'GRS_Q20_V1','status':'OFFLINE_GENERATED_NOT_DEVICE_VALIDATED','layoutSha256':sha})
 header=['// T236 cold angle bytes, generated independently at 80/110 decimal digits.','#pragma once','#include <cstdint>','namespace grs {','inline constexpr char kAngleSha256[]="'+angle_sha+'";','inline constexpr uint32_t kAngleWords[61444]={']
 values=struct.unpack('<61444I',a)
 for i in range(0,len(values),16):header.append(','.join('0x%08xu'%v for v in values[i:i+16])+',')
 header+=['};','}']
 return {'layout_generated.h':'\n'.join(lines).encode(),'angles_q20.bin':a,'angles_generated.h':('\n'.join(header)+'\n').encode(),'angles_certificate.json':(json.dumps(proof,indent=2)+'\n').encode()}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--check',action='store_true');args=ap.parse_args()
 for name,raw in artifacts().items():
  p=DEST/name
  if args.check:assert p.read_bytes()==raw,name+' generated content differs'
  else:p.write_bytes(raw)
 print('T236 contract/angle generation '+('CHECK PASS' if args.check else 'WRITE PASS'))
