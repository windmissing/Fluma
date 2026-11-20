from pathlib import Path
import base64
import hashlib
import json
import tempfile
from audit_capture import audit

with tempfile.TemporaryDirectory() as name:
    directory = Path(name)
    data = ('界'*4000).encode()
    records = []
    for offset in range(0,len(data),8192):
        file = f'{offset}.raw'
        (directory/file).write_bytes(base64.b64encode(data[offset:offset+8192]))
        records.append({'file':file,'offset':offset})
    row = {'tag':'STATUS','scope':'wide-9007199254740993','sequence':1,'parts':records,
           'observeSideEffect':True,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    index = {'scope':row['scope'],'elapsedMs':60000,'encoding':'BASE64','queries':[row]}
    (directory/'index.json').write_text(json.dumps(index))
    assert audit(directory)['queries']==1
    for key,value in [('observeSideEffect',False),('sha256','0'*64),('scope','foreign'),('sequence',2)]:
        changed = dict(row)
        changed[key]=value
        (directory/'index.json').write_text(json.dumps(dict(index, queries=[changed])))
        try:
            audit(directory)
        except ValueError:
            pass
        else:
            raise AssertionError(key)
print(json.dumps({'result':'PASS','cases':5,'device':'NOT_RUN'}))

# Construct a bounded collector envelope from a retained cursor sample.
# The pre-arm metadata is synthetic and only exercises the offline reader.
sample = Path('test/reports/t491/t590c/t468-create-1-1795092162916-native.json').read_bytes()
with tempfile.TemporaryDirectory() as name:
    directory = Path(name)
    pre = b'{"state":"NO_CREATE_ATTEMPT"}'
    queries=[]
    for sequence,tag,data,scope in ((1,'M2_PRE_ARM',pre,''),(2,'M2_DISCOVERY',sample,'')):
        row={'tag':tag,'scope':scope,'sequence':sequence,'parts':[],
             'enterMs':sequence+1,'returnMs':sequence+1,
             'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        for offset in range(0,len(data),8192):
            part=f'q{sequence}-{offset}.raw'
            (directory/part).write_bytes(data[offset:offset+8192])
            row['parts'].append({'file':part,'offset':offset})
        queries.append(row)
    index={'schema':'T676_COLLECTOR_V1','state':'WINDOW_RECORDED',
           'scope':'1','elapsedMs':4,'queries':queries}
    (directory/'index.json').write_text(json.dumps(index))
    witness=directory/'page-witness.json'
    witness.write_text('{"baseline":"0","preArmMs":2,"mountRequested":true}')
    assert audit(directory)['attempt']['observation']=='OPEN_AT_OBSERVATION_END'
    witness.write_text('{"baseline":"1","preArmMs":2,"mountRequested":true}')
    try:
        audit(directory)
    except AssertionError:
        pass
    else:
        raise AssertionError('BASELINE_SERIAL_REUSED')
print(json.dumps({'result':'PASS','addedCases':2,'fixtureOnly':True,'device':'NOT_RUN'}))
