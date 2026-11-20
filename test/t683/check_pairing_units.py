import copy
import json
from check_pairing import inspect, OWNER, SCOPE

def encode(value):
    return json.dumps(value, ensure_ascii=False, separators=(',', ':')).encode()

owner = dict.fromkeys(OWNER, 1)
owner['run'] = 'run-wide-string'
snapshot = {key: owner[key] for key in SCOPE}
evidence = {'revision':'T680_PREPARE_PAIRING_EVIDENCE_V1','owner':owner,'ownerSerial':1,
            'gate':{'prepareId':1,'pageGeneration':1,'observationSerial':0,'diagnosticOverflow':False},
            'snapshot':snapshot,'raster':{'observationSerial':7},
            'lastFrame':{'observationSerial':2,'snapshotSerialMatches':False},
            'truncatedFields':[]}
report = {'owner':owner,'ownerSerial':1,'pairingEvidence':evidence,
          'authority':{'prepareId':1,'pageGeneration':1,'snapshot':dict(snapshot)}}
outer = {'revision':'T659_PREPARE_OWNER_VALUE_V1','readiness':report}
assert inspect(encode(outer))['exactJoinEligible']
for mutate in (
    lambda r: r.pop('authority'),
    lambda r: r.update(detailTruncated=True),
    lambda r: r['pairingEvidence']['truncatedFields'].append('owner.run'),
    lambda r: r['pairingEvidence']['gate'].update(diagnosticOverflow=True),
    lambda r: r['authority']['snapshot'].update(context=2),
    lambda r: r['pairingEvidence'].update(owner=None),
):
    changed = copy.deepcopy(outer)
    mutate(changed['readiness'])
    assert not inspect(encode(changed))['exactJoinEligible']
for target, bound in (('readiness', 14336), ('outer', 16384)):
    changed = copy.deepcopy(outer)
    value = changed['readiness'] if target == 'readiness' else changed
    value['padding'] = ''
    spaces = bound - len(encode(value))
    value['padding'] = '界' * (spaces // 3) + 'x' * (spaces % 3)
    assert len(encode(value)) == bound
    inspect(encode(changed))
    value['padding'] += 'x'
    try:
        inspect(encode(changed))
    except ValueError as error:
        assert 'UTF8_LIMIT' in str(error)
    else:
        raise AssertionError(target)
print(json.dumps({'result':'PASS','cases':11,'device':'NOT_RUN'}))
