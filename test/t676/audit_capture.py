"""Audit T678 collector exports against native-query and byte budgets."""
import hashlib
import json
from pathlib import Path
import sys
import base64
from classify_cursor import classify, u64

def audit(directory):
    index = directory / 'index.json'
    encoded = index.read_bytes()
    if len(encoded) > 32768 or sum(p.stat().st_size for p in directory.iterdir()) > 1048576:
        raise ValueError('DIRECTORY_OR_INDEX_BUDGET')
    receipt = json.loads(encoded)
    records = receipt['queries']
    if len(records) > 64 or not 0 <= receipt['elapsedMs'] <= 60000:
        raise ValueError('QUERY_OR_TIME_BUDGET')
    scope = receipt['scope']
    files = set()
    payloads = []
    for sequence, record in enumerate(records, 1):
        unbound = record['scope']=='' and record['tag'] in ('M2_PRE_ARM','M2_DISCOVERY','COLD_CAPABILITIES')
        if record['sequence'] != sequence or (record['scope'] != scope and not unbound):
            raise ValueError('QUERY_SCOPE_OR_ORDER')
        if record['tag'] == 'STATUS' and record.get('observeSideEffect') is not True:
            raise ValueError('STATUS_EFFECT_UNDECLARED')
        raw = bytearray()
        for part in record['parts']:
            name = part['file']
            if Path(name).name != name or name in files or part['offset'] != len(raw):
                raise ValueError('FRAGMENT_IDENTITY')
            persisted = (directory / name).read_bytes()
            if len(persisted) > 16384:
                raise ValueError('RECORD_BUDGET')
            data = base64.b64decode(persisted, validate=True) if receipt.get('encoding') == 'BASE64' else persisted
            if not 0 < len(data) <= 8192:
                raise ValueError('FRAGMENT_BUDGET')
            raw.extend(data)
            files.add(name)
        if len(raw) != record['bytes'] or hashlib.sha256(raw).hexdigest() != record['sha256']:
            raise ValueError('RAW_IDENTITY')
        native=raw.decode('utf8', errors='strict')
        payloads.append(native)
        if receipt.get('schema')=='T676_COLLECTOR_V1':
            if not 0 < record['enterMs'] <= record['returnMs']:
                raise ValueError('QUERY_TIME_DOMAIN')
            if sequence>1 and record['enterMs']<records[sequence-2]['returnMs']:
                raise ValueError('QUERY_TIME_ORDER')
    result={'queries':len(records),'rawFiles':len(files),'productPASS':False}
    if receipt.get('schema')=='T676_COLLECTOR_V1' and records:
        result.update(classify_archive(directory,receipt,payloads))
    return result

def classify_archive(directory, receipt, payloads):
    records=receipt['queries']
    tags=[row['tag'] for row in records]
    state=receipt['state']
    if state.startswith('INCOMPLETE'):
        return {'captureState':state}
    witness=json.loads((directory/'page-witness.json').read_bytes())
    if tags[0]=='M2_PRE_ARM':
        assert records[0]['scope']==''
        initial=json.loads(payloads[0]); baseline=u64(initial.get('bootSerial','0'))
        assert u64(witness['baseline'])==baseline
        assert records[0]['returnMs']<=witness['preArmMs'] and witness['mountRequested'] is True
        selected=u64(receipt['scope'] or '0')
        if not selected:
            assert state=='CLOSED_WITHOUT_ATTEMPT'
            return {'captureState':state}
        assert selected>baseline
        rows=[]
        for tag, raw in zip(tags[1:],payloads[1:]):
            if tag not in ('M2_LOCKED','M2_DISCOVERY','M2_AFTER'):
                continue
            value=json.loads(raw)
            if tag=='M2_DISCOVERY' and u64(value.get('bootSerial','0'))==baseline:
                continue
            assert u64(value['bootSerial'])==selected
            rows.append((value,classify(value)))
        assert rows
        last, report=rows[-1]
        identity=('bootSerial','epoch','window','eglContext','ownerTid')
        assert all(all(row[k]==last[k] for k in identity) for row,_ in rows)
        if report['observation']=='NATIVE_TERMINAL':
            assert tags[-4:]==['M2_LOG','M2_LOG','M2_LOG','M2_AFTER']
            for i,raw in enumerate(payloads[-4:-1]):
                item=json.loads(raw)
                assert item['state']=='CREATE_LOG' and u64(item['bootSerial'])==selected and item['object']==i
        return {'captureState':state,'attempt':report,'deviceProcessIdentity':'REQUIRES_EXTERNAL_BINDING'}
    assert tags==['COLD_CAPABILITIES','STATUS','RECOVERY','BOOT','EXEC_DIAG','COLD_CAPABILITIES_AFTER']
    cap,_,_,_,diagnostic,after=map(json.loads,payloads)
    assert cap['scope']==after['scope']==json.loads(receipt['scope'])
    assert records[0]['scope']==''
    if diagnostic.get('state')!='COMPLETE' or diagnostic.get('scopeStale'):
        return {'captureState':'EXEC_DIAG_INCOMPLETE','rawState':diagnostic.get('state')}
    for event in witness:
        assert type(event['returned']) is bool and event['count']>=1
        assert event['pointIndex']<event['count']
    return {'captureState':state,'handlerReturns':witness,'inputStagesPresent':'inputStages' in diagnostic,
            'claimWitnessPresent':'claimWitness' in diagnostic,'inputCompletion':'REQUIRES_SAME_SCOPE_JOIN'}


def legacy_m1(raw):
    rows = []
    findings = []
    for number, line in enumerate(raw.splitlines(), 1):
        try:
            rows.append(json.loads(line))
        except (ValueError, UnicodeError) as error:
            findings.append({'line':number,'sha256':hashlib.sha256(line).hexdigest(),
                             'issue':'INVALID_RECORD_JSON','error':str(error)})
    for row in rows:
        body = row['body'].encode('utf8')
        if len(body) > 8192:
            findings.append({'tag':row['tag'],'rawBytes':len(body),
                             'declaredChunks':row['chunks'],'issue':'BODY_STORED_UNFRAGMENTED'})
        if row['tag'] == 'EXEC_DIAG' and row['body'].startswith('THROW'):
            findings.append({'tag':row['tag'],'issue':row['body']})
    return {'records':len(rows),'findings':findings,'scope':'LEGACY_PROBE','productPASS':False}

if __name__ == '__main__':
    mode, name = sys.argv[1:]
    path = Path(name)
    print(json.dumps(audit(path) if mode == 'audit' else legacy_m1(path.read_bytes()),indent=2))
