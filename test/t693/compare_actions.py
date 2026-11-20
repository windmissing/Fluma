"""Check a pair of independent T693 main-arm receipts."""
import json
from pathlib import Path
import re
import sys
from verify import read

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 't496'))
from check_actions import check

def load(directory, arm):
    root = Path(directory)
    launch = json.loads((root / 'launch.json').read_bytes())
    result = json.loads((root / 'result.json').read_bytes())
    assert launch['arm'] == arm, 'ARM'
    assert result['exitCodeUnsigned'] == 0 and result['binaryTraceSealed'], 'INCOMPLETE'
    attempt = launch['attempt']
    stem = root / f'gltrace-{attempt}.ndjson.t693'
    assert sum(1 for _ in read(str(stem))) > 0, 'EMPTY_TRACE'
    timeline = [json.loads(line) for line in
                (root / f't496_log_{attempt}.ndjson').read_bytes().splitlines()]
    check(timeline)
    actions = []
    for event in timeline:
        if not str(event.get('actionId','')).isdigit():
            continue
        number = int(event['actionId'])
        detail = event['detail']
        if number in (7,29):
            detail = re.search(r'payloadSHA=[a-f0-9]{64} words=[0-9,]+',detail)[0]
        elif number in (19,43):
            detail = 'CAPTURE_CONTROL'
        actions.append((number,event['declaredScriptNs'],detail))
    source = json.loads((root / 'source-closure.json').read_bytes())
    modules = sorted((m['path'], m['sha256']) for m in result['modules'])
    assert modules, 'MISSING_MODULE_IDENTITIES'
    return launch['PE']['sha256'], source['cacheInputs'], modules, actions

if __name__ == '__main__':
    assert load(sys.argv[1],'D1_OFF') == load(sys.argv[2],'D1_ON'), 'PE_OR_INPUT_DIVERGENCE'
    print(json.dumps({'actions':'MATCH','bankEventReplay':'NOT_MEASURED','productPASS':False}))
