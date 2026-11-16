"""Reuse the existing T499 oracle for GPU solver conformance inputs."""
from pathlib import Path
import importlib.util
import json
import struct
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('oracle499',ROOT/'test/t499/m3_source_oracle.py')
o=importlib.util.module_from_spec(spec);spec.loader.exec_module(o)
out=ROOT/'.codex/local-test/t522-solver';out.mkdir(parents=True,exist_ok=True)
summary=[]
for name,water,requested,lam in [('uniform',[100]*128,[1]+[0]*127,65536),('floor',[100]*128,[3,5]+[0]*126,32768),('zero',[100]*128,[1]+[0]*127,7),('bound',[o.CQ-3]*128,[20]+[0]*127,65536),('large_u64',[o.CQ//2]*128,[123333]*128,65536)]:
    material=[0]*128;cap=[o.CQ]*128
    status,admitted,result,detail=o.solve_lambda(requested,cap,water,material,water,lam)
    code={'OK':0,'WANT0':1,'CAP_BOUND_RETRY':2,'ETA_CAPACITY_RETRY':3,'ETA_TRUNCATION_RETRY':4}[status]
    words=[lam,code,detail.get('want',sum(admitted)),detail.get('eHi',0),detail.get('d',0),detail.get('Lnext',lam)]+material+water+requested+(result or [0]*128)+admitted
    (out/(name+'.bin')).write_bytes(struct.pack('<'+'I'*len(words),*words));summary.append({'name':name,'status':status,'detail':detail})
# Formal T497 §4.5 applies even when F(CQ) admits another distribution.
# Existing T499 solve_lambda currently omits that conservative truncation gate.
water=[5,5]+[0]*126;request=[0,4]+[0]*126;cap=[5]+[o.CQ]*127
status,admitted,_,detail=o.solve_lambda(request,cap,water,[0]*128,water,65536)
assert status=='OK'
words=[65536,4,4,0,0,65536]+[0]*128+water+request+[0]*128+admitted
(out/'truncation_contract.bin').write_bytes(struct.pack('<'+'I'*len(words),*words))
summary.append({'name':'truncation_contract','formalStatus':'ETA_TRUNCATION_RETRY','existingOracleStatus':status,'eHi':detail['eHi'],'cap0':5,'eta0':5})
for name,water,tangent,rate in [('level_odd',[1]+[0]*127,0,8388608),('level_pair',[1000]+[0]*127,0,8388608),('level_tilt',[500000]*128,211,8388608),('level_reverse',[500000]*128,-211,8388608),('level_zero_rate',[1000]+[0]*127,0,0)]:
    material=[0]*128;cap=[o.CQ]*128;alpha=min(o.Q,rate//120)
    transfers,final,detail=o.water_level(tangent,material,water,cap,alpha)
    words=[tangent&0xffffffff,rate,len(transfers)]+material+water+final
    for transfer in transfers:
        words+=[transfer["donor"],transfer["receiver"],transfer["q"]]
    (out/(name+'.bin')).write_bytes(struct.pack('<'+'I'*len(words),*words));summary.append({'name':name,'transfers':len(transfers)})
(out/'expected.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
