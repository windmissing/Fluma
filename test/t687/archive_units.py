import copy
import hashlib
import json
from pathlib import Path
import tempfile
from read_archive import archive

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

with tempfile.TemporaryDirectory() as work:
    file = Path(work) / 'fixture.u32le'
    n = 0
    for mode,count in [('ROOT',11),('FULL',17)]:
        data = bytes(32768) + bytes(range(count*4))
        file.write_bytes(data)
        m = {'revision':'G11_VIS_COLD_V2','state':'TRACE_READY','headerWords':8192,
             'bytes':str(len(data)),'traceId':'9007199254740993','dataSHA256':digest(data),
             'segments':[{'id':i+1,'byteOffset':32768+i*4,'bytes':4,'strideWords':1,
                          'sha256':digest(data[32768+i*4:32772+i*4])} for i in range(count)]}
        archive(m,file,mode)
        n += 1
        for mutate in (lambda v: v['segments'].pop(),
                       lambda v: v['segments'][0].update(byteOffset=0),
                       lambda v: v['segments'][0].update(sha256='0'*64),
                       lambda v: v.update(dataSHA256='f'*64),
                       lambda v: v.update(traceId=9007199254740993)):
            changed = copy.deepcopy(m)
            mutate(changed)
            try:
                archive(changed,file,mode)
            except AssertionError:
                n += 1
            else:
                raise AssertionError('BAD_COLD_ACCEPTED')
    seed = bytes(271104)
    file.write_bytes(seed)
    pieces = [{'kind':i+1,'preimageOffset':i*67776,'bytes':67776,
               'sha256':digest(seed[:67776])} for i in range(4)]
    archive({'schema':'G11_VIS_SEED_RAW_V1','state':3,'id':'1','segments':pieces,
             'rawBytes':str(len(seed)),'actualVisualSeedSHA':digest(seed)},file,'BOOTSTRAP')
    witness = bytes(3149824)
    file.write_bytes(witness)
    archive({'revision':'G11_PHASE_WITNESS_V2','state':'READY','id':'1',
             'bytes':len(witness),'sha256':digest(witness)},file,'WITNESS')
print(json.dumps({'result':'PASS','cases':n+2,'fixtureOnly':True,'device':'NOT_RUN'}))
