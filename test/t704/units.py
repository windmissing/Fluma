import copy
import json
from check import capability, EXPECTED, MINIMUM

base=dict(MINIMUM,stage='CAP_COMPUTE',precisionThreshold=23,
          precisionSource='VERTEX_HIGH_FLOAT_DECLARATION',precisionRawGL=0,
          resourceRawGL=0,rawGL=0,precisionStatus='PREFILTER_PASS',
          floatPrecision=23,precisionRange=[127,127],CAP_FP32='NOT_MEASURED',
          CAP_MATH='NOT_MEASURED',CAP_SCENE='NOT_MEASURED')
assert capability(None)['compute']=='NOT_MEASURED'
assert capability(base)['compute']=='NOT_MEASURED'
fault=dict(base,precisionRawGL=1280,rawGL=1280,floatPrecision=None,
           precisionRange=None,precisionStatus='QUERY_ERROR')
assert capability(fault)['declaration']=='QUERY_ERROR'
qualified=dict(base,fp32Source='SAME_PROGRAM_COLD_COMPUTE',fp32Words=EXPECTED*2,
               fp32RawGL=0,CAP_FP32='QUALIFIED')
assert capability(qualified)['compute']=='QUALIFIED'
failed=copy.deepcopy(qualified)
failed['fp32Words'][9]^=1
failed['CAP_FP32']='FAILED'
assert capability(failed)['compute']=='FAILED'
for wrong in (dict(base,CAP_FP32='QUALIFIED'),dict(fault,floatPrecision=23),
              dict(failed,CAP_FP32='QUALIFIED'),dict(qualified,blocks=3),
              dict(qualified,fp32RawGL=1280),dict(qualified,precisionSource='COMPUTE_ENUM')):
    try:
        capability(wrong)
    except AssertionError:
        pass
    else:
        raise AssertionError('INVALID_EVIDENCE_ACCEPTED')
print(json.dumps({'result':'PASS','cases':11,'fixtureOnly':True,'device':'NOT_RUN'}))
