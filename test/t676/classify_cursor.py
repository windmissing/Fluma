"""T663 single API cursor, with exact U64 values and same-attempt checks."""
import json
from pathlib import Path
import re
import sys

def u64(value):
    if not isinstance(value,str) or not re.fullmatch('0|[1-9][0-9]{0,19}',value) or int(value) >= 2**64:
        raise ValueError('U64')
    return int(value)

def classify(snapshot):
    if snapshot['revision'] != 'T462_M2_CREATE_DIAGNOSTIC_V1':
        raise ValueError('SNAPSHOT_REVISION')
    attempt = u64(snapshot['bootSerial'])
    if not attempt:
        raise ValueError('NO_ATTEMPT')
    cursor = snapshot.get('apiCursor')
    if cursor is None:
        return {'attempt':str(attempt),'observation':'CURSOR_NOT_EXPORTED'}
    if cursor['revision'] != 1 or cursor['reservedZero'] != [0]*32:
        raise ValueError('CURSOR_ABI')
    for cursor_key, owner_key in [('bootSerial','bootSerial'),('epoch','epoch'),
                                  ('ownerTid','ownerTid'),('window','window'),
                                  ('context','eglContext')]:
        if u64(cursor[cursor_key]) != u64(snapshot[owner_key]):
            raise ValueError('CURSOR_SCOPE')
    begin, end = u64(cursor['beginNs']), u64(cursor['endNs'])
    if snapshot['sealed'] is True and u64(snapshot['nativeEndNs']) > 0:
        state = 'NATIVE_TERMINAL'
    elif cursor['edge'] == 1 and begin > 0 and end == 0 and cursor['returned'] == -1:
        state = 'OPEN_AT_OBSERVATION_END'
    elif cursor['edge'] == 2 and 0 < begin <= end:
        state = 'API_RETURNED'
    else:
        state = 'CURSOR_INCOMPLETE'
    u64(cursor['sequence'])
    return {'attempt':str(attempt),'observation':state,'cursor':cursor,
            'nativeCode':snapshot['code'],'productPASS':False}

if __name__ == '__main__':
    print(json.dumps(classify(json.loads(Path(sys.argv[1]).read_bytes())),indent=2))
