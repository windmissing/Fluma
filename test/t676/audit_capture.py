"""Audit T678 collector exports against native-query and byte budgets."""
import hashlib
import json
from pathlib import Path
import sys
import base64

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
    for sequence, record in enumerate(records, 1):
        if record['sequence'] != sequence or record['scope'] != scope:
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
        raw.decode('utf8', errors='strict')
    return {'queries':len(records),'rawFiles':len(files),'productPASS':False}

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
