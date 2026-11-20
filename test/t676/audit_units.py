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
