"""Validate T690 capability evidence without inferring device qualification."""
import json
from pathlib import Path
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'t672'))
from analyze import analyze

EXPECTED = [0x3F800001, 0x3F800000, 1, 0x00400000, 7, 0, 0, 0x46503332]
MINIMUM = {'blocks':4,'bindings':4,'invocations':64,'laneX':64,'shared':4096,'maxBlock':2331392}

def capability(raw):
    if raw is None:
        return {'declaration':'NOT_MEASURED','compute':'NOT_MEASURED'}
    assert len(json.dumps(raw,separators=(',',':'),ensure_ascii=False).encode()) <= 1984
    assert raw['stage']=='CAP_COMPUTE' and raw['precisionThreshold']==23
    assert raw['precisionSource']=='VERTEX_HIGH_FLOAT_DECLARATION'
    resource, precision = raw['resourceRawGL'], raw['precisionRawGL']
    assert all(type(v) is int and v>=0 for v in (resource,precision))
    assert raw['rawGL']==(resource or precision)
    if precision:
        assert raw['floatPrecision'] is None and raw['precisionRange'] is None
        declaration='QUERY_ERROR'
    else:
        value=raw['floatPrecision']
        assert type(value) is int and value>=0
        assert len(raw['precisionRange'])==2 and all(type(x) is int for x in raw['precisionRange'])
        declaration='LOW_PRECISION' if value<23 else 'PREFILTER_PASS'
    assert raw['precisionStatus']==declaration
    resources=not resource and all(type(raw[k]) is int and raw[k]>=v for k,v in MINIMUM.items())
    compute=raw['CAP_FP32']
    assert compute in ('NOT_MEASURED','FAILED','QUALIFIED')
    if 'fp32Words' in raw:
        words=raw['fp32Words']
        assert len(words)==16 and all(type(w) is int and 0<=w<2**32 for w in words)
        assert raw['fp32Source']=='SAME_PROGRAM_COLD_COMPUTE'
        assert type(raw['fp32RawGL']) is int and raw['fp32RawGL']>=0
        observed=words[7]==words[15]==EXPECTED[-1]
        expected='QUALIFIED' if observed and words==EXPECTED*2 and raw['fp32RawGL']==0 else 'FAILED' if observed else 'NOT_MEASURED'
        assert compute==expected, 'FP32_INCONSISTENT_RESULT'
    else:
        assert compute=='NOT_MEASURED', 'FP32_MISSING_RAW_WORDS'
    if compute=='QUALIFIED':
        assert resources and declaration=='PREFILTER_PASS'
    return {'resourcePrefilter':resources,'declaration':declaration,
            'compute':compute,'math':raw['CAP_MATH'],'scene':raw['CAP_SCENE']}

def inspect(root, round):
    receipt=analyze(root,round)
    cap=capability(receipt['capability'])
    stages=receipt['stages']
    if cap['compute']=='QUALIFIED':
        assert receipt['result']=='ANCHORS_EQUAL_NOT_ATOMIC'
        actual=[s for s in stages if s['attempt']==receipt['selectedTicket']
                and s['boot']==receipt['controlBoot'] and s['context']!='0'
                and s['phase'] in ('CAPABILITY','INIT_FP32','INIT_FP32_RESULT')]
        qualifying=[s for s in actual if s['edge']=='EXIT' and s['rawGL']==0]
        assert [s['phase'] for s in qualifying]==['CAPABILITY','INIT_FP32','INIT_FP32_RESULT']
        assert len({(s['context'],s['window'],s['tid']) for s in qualifying})==1
    receipt['evidenceClassification']=cap
    return receipt

if __name__=='__main__':
    print(json.dumps(inspect(sys.argv[1],int(sys.argv[2]) if len(sys.argv)>2 else 1),indent=2))
