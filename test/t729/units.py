"""T729 fixtures exercise the real reader, never a device."""
from copy import deepcopy
import base64
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import struct
from measure import inspect, compare, sha, analyze, decide, t721

D='a'*64
W=9007199254740993

def fixture(dir,variant,warm,index,censored=False):
    dir.mkdir()
    cache='cache.json'
    module='mapped.json'
    (dir/cache).write_text(json.dumps({'entries':['key'] if warm else []}))
    identity=dict(route='G13',backend='GLES',device='fixture',osBuild='os',driver='driver',
                  bundleOrExecutable='example',packageOrExecutableSha=D if variant=='MONOLITH' else 'b'*64,
                  loadedModuleBuildId='0123' if variant=='MONOLITH' else '4567',
                  loadedModuleSha=D,pid=str(index),processStartId=str(index+10),nativeBoot=str(W),
                  nativeTicket=str(W+1),attemptId=str(index),profile='F07',configSha=D,inputSha=D,
                  seedSha=D,presentationRoute='XCOMPONENT')
    (dir/module).write_text(json.dumps({k:identity[k] for k in
                         ('pid','processStartId','loadedModuleBuildId','loadedModuleSha')}))
    (dir/'events.json').write_text('{"fixture":true}')
    def orig(id,name):
        raw=(dir/name).read_bytes()
        return dict(id=id,path=name,bytes=len(raw),sha256=sha(raw))
    originals=[orig('cache',cache),orig('loaded-module',module),orig('trace','events.json')]
    scope=dict(window='1',contextOrDeviceGeneration='2',ownerTid='3',surface='4',run='5',
               epoch='6',session='7',permissionRevision='8')
    role=dict(role='BOOT',id='boot',roleGeneration='1',scope=scope,sourceOrSpirvSha=D,payloadBytes=10,
              layoutSha=D,bindingOrLinkConfigSha=D,environmentSha=D,cacheKey='key',
              cacheLookup='FOUND' if warm else 'MISS',binaryLoad='ACCEPTED_AFTER_REFLECTION' if warm else 'NOT_ATTEMPTED',
              vkPipelineCacheFeedback='NOT_APPLICABLE',binaryFormat='1',binarySha=D,binaryBytes=20,
              persistence='NOT_REACHED' if warm or censored else 'PERSISTED',
              sourceCompileCount=0 if warm else 1,sourceFallbackCount=0)
    events=[]
    def event(op,delta,edge='MARK',call=None,program=None,token=None):
        result=dict(sequence=str(len(events)+1),ns=str(W+delta),tid='3',scope=scope,operation=op,
                    edge=edge,granularity='EXACT_NATIVE_CALL' if call else 'NATIVE_MARK',
                    invocationId=call,programId=program,rootToken=token,
                    rawReturn=None if edge=='ENTER' else 1,rawError=None if edge=='ENTER' else 0,originalId='trace')
        events.append(result)
        return result
    def endpoint(row=None,state='NOT_REACHED'):
        return dict(state='OBSERVED_SUCCESS' if row else state,eventSequence=row['sequence'] if row else None,
                    ns=row['ns'] if row else None,rootToken=row['rootToken'] if row else None)
    event('CONFIG_RECEIVED',1)
    admitted=event('CONFIG_ADMITTED',2)
    op='GL_PROGRAM_BINARY' if warm else 'GL_COMPILE_SHADER'
    event(op,3,'ENTER','program','boot')
    if censored:
        event('OBSERVATION_END',120)
        created=ready=root=swap=None
    else:
        event(op,15,'RETURNED','program','boot')
        event(op,16,'EXIT','program','boot')
        event('REFLECT',17,program='boot')
        if not warm:
            event('CACHE_PERSIST',18,program='boot')
        created=event('CREATE',19)
        ready=event('COMPUTE_READY',20)
        event('ACTIVATION',21)
        root=event('ROOT_ACCEPTED',30,token='accepted')
        event('PRESENT',35,'ENTER','swap',token='accepted')
        swap=event('PRESENT',40,'RETURNED','swap',token='accepted')
        event('OBSERVATION_END',120)
    exp=dict(groupId='F07',variantId=variant,variant=variant,cachePolicy='ON',
             condition='WARM_NEW_PROCESS' if warm else 'APP_COLD',appCacheInventoryOriginalId='cache',
             driverCacheCondition='UNKNOWN',thermalCondition='COMPARABLE',foregroundCondition='FOREGROUND',
             orderInGroup=index,observationPolicy='fixture',observeStartNs=str(W),observeEndNs=str(W+120),
             captureLimits='fixture')
    data=dict(revision='T727_SHADER_MEASUREMENT_V1',identity=identity,experiment=exp,programs=[role],
              events=events,outcomes=dict(configure=endpoint(admitted),create=endpoint(created),ready=endpoint(ready),
              firstAcceptedRoot=endpoint(root),firstValidSwap=endpoint(swap),
              terminalReason='DEADLINE_CENSORED' if censored else 'COMPLETE',firstFailureOriginalId=None,
              numericalResult='NOT_MEASURED',originalReference='FROZEN_ORACLE_ONLY'),
              steady=None,resource=None,originals=originals)
    p=dir/'measurement.json'
    write(p,data)
    return p,data

def write(path,data):
    path.write_text(json.dumps(data),encoding='utf8')

def reject(action,code):
    try:
        action()
    except ValueError as error:
        assert code in str(error),(code,str(error))
    else:
        raise AssertionError('ACCEPTED '+code)

def pages(root):
    root.mkdir()
    ticket=str(W+1)
    write(root/'configure.json',{'attempt':ticket})
    raw=dict(t721.MINIMUM,stage='CAP_COMPUTE',precisionThreshold=23,
             precisionSource='VERTEX_HIGH_FLOAT_DECLARATION',precisionRawGL=0,resourceRawGL=0,
             rawGL=0,precisionStatus='PREFILTER_PASS',floatPrecision=23,precisionRange=[127,127],
             CAP_FP32='QUALIFIED',CAP_MATH='NOT_MEASURED',CAP_SCENE='NOT_MEASURED',
             fp32Source='SAME_PROGRAM_COLD_COMPUTE',fp32Words=t721.EXPECTED*2,fp32RawGL=0)
    status={'lastCommand':{'api':'g13Configure','boot':str(W),'attempt':ticket},
            'initialization':{'firstFailureAttempt':'0'},'native':{'capability':raw}}
    write(root/'r1-status.json',status)
    payload=bytearray(32768)
    struct.pack_into('<QQ',payload,28672+56,W,W+1)
    for n,phase in enumerate((b'CAPABILITY',b'INIT_FP32',b'INIT_FP32_RESULT')):
        at=4096+n*256
        struct.pack_into('<9Q',payload,at,n+1,W+20+n,W,W+1,W+1,1,2,3,4)
        payload[at+128:at+128+len(phase)]=phase
        payload[at+160:at+164]=b'EXIT'
    values={}
    for n in range(4):
        values[n]={'schema':'G13InitDiagnosticV1','state':'SNAPSHOT','page':n,'offsetBytes':n*8192,
                   'rawBytes':8192,'sampleSeq':'3','base64':base64.b64encode(payload[n*8192:(n+1)*8192]).decode()}
        write(root/f'r1-page{n}.json',values[n])
    for key in ('before','after'):
        write(root/f'r1-anchor-{key}.json',values[3])
    assert decide(analyze(root),status)=='SYNCHRONIZED_FP32_EVIDENCE'
    wrong=bytearray(base64.b64decode(values[3]['base64']))
    wrong[0]^=1
    write(root/'r1-anchor-after.json',dict(values[3],base64=base64.b64encode(wrong).decode()))
    assert decide(analyze(root),status)=='ANCHOR_MISMATCH'

def main():
    with TemporaryDirectory(prefix='T729-') as d:
        root=Path(d)
        pages(root/'pages')
        arms=[fixture(root/str(i),v,w,i) for i,(v,w) in
              enumerate([('MONOLITH',False),('SPLIT',False),('MONOLITH',True),('SPLIT',True)],1)]
        target,original=arms[0]
        control=inspect(target)
        assert control['firstSwapNs']==39 and control['calls']['program']['ns']=='12'
        role={'signedSHA256':D,'nativeSHA256':D,'buildId':'0123','roles':{'BOOT':{'sha256':D,'bytes':10}}}
        deploy={'schema':'T729_DEPLOYMENT_V1','signed':True,
                'arms':{'MONOLITH':role,'SPLIT':dict(role,signedSHA256='b'*64,buildId='4567')}}
        inputs=[p for p,_ in arms]
        assert compare(inputs,deploy)['firstSwapSuccesses']==4
        path,_=fixture(root/'open','MONOLITH',False,5,censored=True)
        open_run=inspect(path)
        assert open_run['firstSwapNs'] is None and open_run['calls']['program']['ns']=='117'
        assert open_run['calls']['program']['state']=='OPEN_LOWER_BOUND'
        cases=[
          ('FIRST_SWAP_SCOPE_TOKEN_OR_ORDER',lambda d:d['events'][-2].update(rootToken='stale')),
          ('FIRST_SWAP_SCOPE_TOKEN_OR_ORDER',lambda d:d['events'][-2].update(scope=dict(d['events'][-2]['scope'],epoch='9'))),
          ('NATIVE_EDGE_SCOPE',lambda d:d['events'][3].update(tid='8')),
          ('ENDPOINT_NOT_SUCCESSFUL',lambda d:d['events'][-2].update(rawReturn=False)),
          ('ORIGINAL_BYTES',lambda d:d['originals'][0].update(sha256='b'*64)),
          ('EVENT_ORDER_OR_WINDOW',lambda d:d['events'][1].update(sequence='1')),
          ('UNPROVEN_PERSIST',lambda d:d['events'][6].update(operation='CACHE_EXPORT')),
          ('ORACLE_NOT_OBSERVED',lambda d:d['outcomes'].update(numericalResult='PASS_EXISTING_ORACLE')),
          ('SCHEMA',lambda d:d['events'][0].update(ns=W)),
          ('SCHEMA',lambda d:d['events'][0].update(operation='GPU_READY'))]
        # Update the matching endpoint and ENTER in scope/token negatives so the semantic gate is reached.
        for n,(code,change) in enumerate(cases):
            data=deepcopy(original)
            change(data)
            if n==0:
                data['outcomes']['firstValidSwap']['rootToken']='stale'
            if n==1:
                data['events'][-3]['scope']=dict(data['events'][-3]['scope'],epoch='9')
            write(target,data)
            reject(lambda:inspect(target),code)
        write(target,original)
        warm,data=arms[2]
        changed=deepcopy(data)
        changed['programs'][0]['binarySha']='c'*64
        write(warm,changed)
        reject(lambda:compare(inputs,deploy),'WARM_WITHOUT_MATCHED_PERSISTED')
        write(warm,data)
        print(json.dumps({'measurementNegatives':len(cases),'coldWarm':'MATCHED_TEST_FIXTURE',
                          'wrongWarmBinary':'REJECTED','firstSwapNs':39,'censoredCallLowerBoundNs':117,
                          'fourPageWideTicket':'MATCHED','anchorMismatch':'REJECTED',
                          'device':'NOT_RUN','productPASS':False},indent=2))

if __name__=='__main__':
    main()
