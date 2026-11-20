"""T727 G13/GLES offline measurement and four-arm comparisons."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('t721_rules',ROOT/'test/t721/check.py')
t721=importlib.util.module_from_spec(spec)
spec.loader.exec_module(t721)
numerical_gates,decide,analyze,u64=(t721.numerical_gates,t721.decide,t721.analyze,t721.u64)

SCHEMA=json.loads((ROOT/'docs/architecture-T727-measurement-v1.json').read_bytes())
TYPES={'object':dict,'array':list,'string':str,'integer':int,'boolean':bool,'null':type(None)}
POINTS={'configure':'CONFIG_ADMITTED','create':'CREATE','ready':'COMPUTE_READY',
        'firstAcceptedRoot':'ROOT_ACCEPTED','firstValidSwap':'PRESENT'}

def need(ok,code):
    if not ok:
        raise ValueError(code)

def shape(value, rule, path='measurement'):
    def check(ok):
        need(ok,'SCHEMA '+path)
    if '$ref' in rule:
        key=rule['$ref'].split('/')[-1]
        shape(value,SCHEMA['$defs'][key],path)
        if key=='u64':
            u64(value)
    if 'anyOf' in rule:
        for option in rule['anyOf']:
            try:
                shape(value,option,path)
                break
            except ValueError:
                pass
        else:
            check(False)
    if 'type' in rule:
        names=rule['type'] if isinstance(rule['type'],list) else [rule['type']]
        check(type(value) in [TYPES[n] for n in names])
    if 'const' in rule:
        check(value==rule['const'])
    if 'enum' in rule:
        check(value in rule['enum'])
    if 'minimum' in rule:
        check(value>=rule['minimum'])
    if 'minLength' in rule:
        check(len(value)>=rule['minLength'])
    if 'pattern' in rule:
        check(re.fullmatch(rule['pattern'],value) is not None)
    if 'minItems' in rule:
        check(len(value)>=rule['minItems'])
    if 'items' in rule:
        for n,item in enumerate(value):
            shape(item,rule['items'],f'{path}[{n}]')
    if 'properties' in rule:
        keys=rule['properties']
        check(set(rule.get('required',()))<=value.keys())
        if not rule.get('additionalProperties',True):
            check(value.keys()<=keys.keys())
        for key,child in keys.items():
            if key in value:
                shape(value[key],child,path+'.'+key)

def load(path):
    return json.loads(Path(path).read_bytes())

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def success(event):
    return (event['edge'] in ('RETURNED','EXIT','MARK') and
            event['rawReturn'] in (True,1,'1','OK','SUCCESS') and event['rawError'] in (0,'0'))

def check_archive(raw,receipt):
    spec=importlib.util.spec_from_file_location('g13_archive',ROOT/'tools/g13/read_archive.py')
    reader=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    return reader.read(raw,receipt)

def inspect(file):
    file=Path(file).resolve()
    data=load(file)
    shape(data,SCHEMA)
    ident,exp,out=(data[k] for k in ('identity','experiment','outcomes'))
    need(ident['route']=='G13' and ident['backend']=='GLES','G13_GLES_SCOPE')
    originals={}
    for item in data['originals']:
        path=(file.parent/item['path']).resolve()
        need(path.is_relative_to(file.parent) and item['id'] not in originals,'ORIGINAL_SCOPE_OR_ID')
        raw=path.read_bytes()
        need(len(raw)==item['bytes'] and sha(raw)==item['sha256'],'ORIGINAL_BYTES')
        originals[item['id']]=path
    need(exp['appCacheInventoryOriginalId'] in originals,'CACHE_INVENTORY_MISSING')
    inventory=load(originals[exp['appCacheInventoryOriginalId']])
    need(type(inventory['entries']) is list,'CACHE_INVENTORY_FORMAT')
    if exp['condition']=='APP_COLD':
        need(not inventory['entries'],'COLD_CACHE_NOT_EMPTY')
    mapped=False
    if 'loaded-module' in originals:
        module=load(originals['loaded-module'])
        fields=('pid','processStartId','loadedModuleBuildId','loadedModuleSha')
        need(all(ident[k] and ident[k]==module[k] for k in fields),'MAPPED_MODULE_MISMATCH')
        mapped=True
    prog={p['id']:p for p in data['programs']}
    need(len(prog)==len(data['programs']),'DUPLICATE_PROGRAM')
    need(len({p['role'] for p in prog.values()})==len(prog),'DUPLICATE_ROLE')
    begin,end=map(u64,(exp['observeStartNs'],exp['observeEndNs']))
    events={}
    pairs={}
    previous=(0,begin)
    for event in data['events']:
        sequence,ns=map(u64,(event['sequence'],event['ns']))
        need(sequence>previous[0] and previous[1]<=ns<=end,'EVENT_ORDER_OR_WINDOW')
        previous=sequence,ns
        events[event['sequence']]=event
        need(event['originalId'] in originals,'EVENT_ORIGINAL')
        program=event['programId']
        if program is not None:
            need(program in prog and event['scope']==prog[program]['scope'],'PROGRAM_SCOPE')
        if event['granularity']!='EXACT_NATIVE_CALL':
            continue
        call=event['invocationId']
        need(call and event['edge']!='MARK','NATIVE_CALL_ID')
        edges=pairs.setdefault(call,{})
        edge=event['edge']
        need(edge not in edges and (edge=='ENTER' or 'ENTER' in edges) and 'EXIT' not in edges,
             'NATIVE_EDGE_ORDER')
        if edges:
            first=edges['ENTER']
            need(all(first[k]==event[k] for k in ('operation','programId','scope','tid')),
                 'NATIVE_EDGE_SCOPE')
        edges[edge]=event
    durations={}
    for call,edges in pairs.items():
        enter=edges['ENTER']
        returned=edges.get('RETURNED') or edges.get('EXIT')
        durations[call]={'operation':enter['operation'],'programId':enter['programId'],
                         'state':'MEASURED' if returned else 'OPEN_LOWER_BOUND',
                         'ns':str((u64(returned['ns']) if returned else end)-u64(enter['ns']))}
    for p in prog.values():
        if p['binaryLoad']=='ACCEPTED_AFTER_REFLECTION':
            seen=[e for e in events.values() if e['programId']==p['id'] and success(e)]
            loads=[e for e in seen if e['operation']=='GL_PROGRAM_BINARY']
            reflects=[e for e in seen if e['operation']=='REFLECT']
            need(p['cacheLookup']=='FOUND' and p['cacheKey'] and p['binarySha'] and p['binaryBytes']
                 and loads and reflects and u64(loads[0]['ns'])<=u64(reflects[0]['ns']),'UNPROVEN_BINARY_LOAD')
        if p['persistence']=='PERSISTED':
            need(p['cacheKey'] and p['binarySha'] and p['binaryBytes'] and any(
                e['operation']=='CACHE_PERSIST' and e['programId']==p['id'] and success(e)
                for e in events.values()),'UNPROVEN_PERSIST')
        count=sum(x['ENTER']['operation']=='GL_COMPILE_SHADER' and x['ENTER']['programId']==p['id']
                  for x in pairs.values())
        if p['sourceCompileCount'] is not None:
            need(p['sourceCompileCount']==count,'COMPILE_COUNT_MISMATCH')
        if p['sourceFallbackCount']==1:
            need(p['cacheLookup'] in ('KEY_MISMATCH','CORRUPT') or p['binaryLoad']=='REJECTED','FALLBACK_WITHOUT_REJECTION')
            need(count==1,'FALLBACK_COMPILE_COUNT')
    observed={}
    for name,op in POINTS.items():
        endpoint=out[name]
        state=endpoint['state']
        if state not in ('OBSERVED_SUCCESS','OBSERVED_FAILURE'):
            need(not any(endpoint[k] is not None for k in ('ns','eventSequence','rootToken')),
                 'UNOBSERVED_ENDPOINT_FILLED')
            continue
        row=events.get(endpoint['eventSequence'])
        need(row and row['operation']==op and (row['ns'],row['rootToken'])==
             (endpoint['ns'],endpoint['rootToken']),'ENDPOINT_EVIDENCE')
        if state=='OBSERVED_SUCCESS':
            need(success(row),'ENDPOINT_NOT_SUCCESSFUL')
            observed[name]=row
        else:
            need(not success(row) and row['edge']!='ENTER' and
                 (row['rawReturn'] is not None or row['rawError'] not in (None,0,'0')),
                 'ENDPOINT_FAILURE_NOT_OBSERVED')
    if out['terminalReason']=='FIRST_NATIVE_FAILURE':
        need(out['firstFailureOriginalId'] in originals,'FAILURE_ORIGINAL')
    elif out['terminalReason']=='DEADLINE_CENSORED':
        need(any(e['operation']=='OBSERVATION_END' and u64(e['ns'])==end for e in events.values()),
             'CENSOR_WITHOUT_END')
    first=None
    received=[e for e in events.values() if e['operation']=='CONFIG_RECEIVED']
    need(len(received)<=1,'MULTIPLE_CONFIGURATIONS')
    if 'firstValidSwap' in observed:
        swap,root=observed['firstValidSwap'],observed.get('firstAcceptedRoot')
        need(root and received and ident['presentationRoute']=='XCOMPONENT' and
             swap['granularity']=='EXACT_NATIVE_CALL' and swap['scope']==root['scope'] and
             swap['tid']==swap['scope']['ownerTid'] and
             all(v and u64(v)>0 for v in swap['scope'].values()) and swap['rootToken'] and
             root['rootToken']==swap['rootToken'] and u64(root['ns'])<=u64(swap['ns']),
             'FIRST_SWAP_SCOPE_TOKEN_OR_ORDER')
        need(not any(e['operation']=='PRESENT' and e['scope']==root['scope'] and
                     e['rootToken']==root['rootToken'] and e['granularity']=='EXACT_NATIVE_CALL'
                     and success(e) and u64(root['sequence'])<=u64(e['sequence'])<u64(swap['sequence'])
                     for e in events.values()),'EARLIER_VALID_SWAP')
        need(not any(e['operation'] in ('REVOKE','RETIRE') and e['scope']==root['scope']
                     and u64(root['sequence'])<u64(e['sequence'])<u64(swap['sequence'])
                     for e in events.values()),'FIRST_SWAP_REVOKED')
        first=u64(swap['ns'])-u64(received[0]['ns'])
        need(first>=0,'FIRST_SWAP_ORDER')
    numerical='NOT_MEASURED'
    if 'oracle' in originals:
        evidence=load(originals['oracle'])
        root=observed.get('firstAcceptedRoot')
        need(root and evidence['rootToken']==root['rootToken'] and evidence['scope']==root['scope'] and
             all(evidence[k]==ident[k] for k in ('configSha','inputSha','seedSha')) and
             evidence['baselineSHA256']==sha((ROOT/'test/t606/independent-baseline.json').read_bytes()),
             'ORACLE_IDENTITY')
        numerical=numerical_gates(evidence['J'],evidence['residuals'])
    if out['numericalResult']!='NOT_MEASURED':
        need(numerical in ('THRESHOLDS_SATISFIED','THRESHOLDS_EXCEEDED'),'ORACLE_NOT_OBSERVED')
        # Residual evidence can disprove a numerical pass. A field comparison
        # still requires the complete T631 fixture-specific evaluation.
        if out['numericalResult']=='PASS_EXISTING_ORACLE':
            need(numerical=='THRESHOLDS_SATISFIED' and 'field-oracle' in originals,
                 'FIELD_ORACLE_REQUIRED')
            field=load(originals['field-oracle'])
            need(field['result']=='PASS' and field['case'] in
                 {c['id'] for c in load(ROOT/'test/t606/independent-baseline.json')['cases']}
                 and all(field[k]==evidence[k] for k in ('rootToken','scope','configSha','inputSha','seedSha','baselineSHA256'))
                 and field['rawArchiveOriginalId'] in originals
                 and field['archiveReceiptOriginalId'] in originals,'FIELD_ORACLE_IDENTITY')
            archive=check_archive(originals[field['rawArchiveOriginalId']],originals[field['archiveReceiptOriginalId']])
            need(archive['state']=='STRUCTURALLY_COMPLETE','FIELD_ARCHIVE_INCOMPLETE')
        else:
            need(numerical=='THRESHOLDS_EXCEEDED' or 'field-oracle' in originals,'ORACLE_RESULT_MISMATCH')
    steady=data['steady']
    fps=None
    if steady is not None:
        start,stop=map(u64,(steady['sampleBeginNs'],steady['sampleEndNs']))
        need(first is not None and u64(observed['firstValidSwap']['ns'])<=start<stop<=end and
             all(steady[k] in originals for k in ('frameIntervalsOriginalId','rootCostsOriginalId')),
             'STEADY_EVIDENCE')
        fps=steady['validNewFrameCount']*10**9/(stop-start)
    return {'data':data,'calls':durations,'firstSwapNs':first,'numericalThresholds':numerical,
            'reportedFreshFps':fps,
            'loadedIdentityVerified':mapped,'originals':sorted(originals)}

def compare(paths, deployment):
    need(deployment['schema']=='T729_DEPLOYMENT_V1' and deployment['signed'],'SIGNED_DEPLOYMENT_REQUIRED')
    a,b=(deployment['arms'][v] for v in ('MONOLITH','SPLIT'))
    need(a['signedSHA256']!=b['signedSHA256'] and a['buildId']!=b['buildId'],'DUPLICATE_DEPLOYMENT_ARMS')
    runs=[inspect(p) for p in paths]
    keys=('device','osBuild','driver','profile','configSha','inputSha','seedSha','presentationRoute')
    signatures=set()
    cached={}
    process=set()
    used=set()
    for run in runs:
        data=run['data']
        i,e=data['identity'],data['experiment']
        signatures.add(tuple(i[k] for k in keys)+(e['groupId'],e['cachePolicy'],
                       e['driverCacheCondition'],e['thermalCondition'],e['observationPolicy'],e['captureLimits']))
        need((i['pid'],i['processStartId']) not in process,'REUSED_PROCESS')
        process.add((i['pid'],i['processStartId']))
        need(e['foregroundCondition']=='FOREGROUND','FOREGROUND_MISMATCH')
        need(e['cachePolicy']=='ON','WARM_COMPARISON_REQUIRES_CACHE')
        key=e['variant'],e['condition']
        need(key not in used and key[0] in ('MONOLITH','SPLIT') and key[1] in
             ('APP_COLD','WARM_NEW_PROCESS'),'ARM_ID')
        artifact=deployment['arms'][key[0]]
        need(run['loadedIdentityVerified'] and (i['packageOrExecutableSha'],i['loadedModuleBuildId'],i['loadedModuleSha'])==
             (artifact['signedSHA256'],artifact['buildId'],artifact['nativeSHA256']),'LOADED_ARM_ID')
        code={r['role']:(r['sourceOrSpirvSha'],r['payloadBytes']) for r in data['programs']}
        need(code=={role:(r['sha256'],r['bytes']) for role,r in artifact['roles'].items()},'ARM_ROLE_ID')
        used.add(key)
        cached[key]=run
    need(len(signatures)==1 and len(used)==4,'PAIR_CONDITIONS')
    pair_results={}
    for variant in ('MONOLITH','SPLIT'):
        cold=cached[variant,'APP_COLD']
        warm=cached[variant,'WARM_NEW_PROCESS']
        before=cold['data']; after=warm['data']
        need(before['experiment']['orderInGroup']<after['experiment']['orderInGroup'],'WARM_RUN_ORDER')
        c={p['role']:p for p in before['programs']}
        for w in after['programs']:
            p=c[w['role']]
            if w['binaryLoad']=='ACCEPTED_AFTER_REFLECTION':
                fields=('cacheKey','binarySha','binaryFormat','binaryBytes','sourceOrSpirvSha',
                        'layoutSha','bindingOrLinkConfigSha','environmentSha')
                need(p['persistence']=='PERSISTED' and all(p[k]==w[k] for k in fields),
                     'WARM_WITHOUT_MATCHED_PERSISTED')
        pair_results[variant]={'coldNs':cold['firstSwapNs'],'warmNs':warm['firstSwapNs'],
                               'warmSavingsNs':cold['firstSwapNs']-warm['firstSwapNs']
                               if all(r['firstSwapNs'] is not None for r in (cold,warm)) else None,
                               'warmLoadedRoles':[w['role'] for w in after['programs']
                                                  if w['binaryLoad']=='ACCEPTED_AFTER_REFLECTION']}
    a,b=[cached[v,'APP_COLD'] for v in ('MONOLITH','SPLIT')]
    cold_delta=None
    lower_bound=None
    if a['firstSwapNs'] is not None and b['firstSwapNs'] is not None:
        cold_delta=a['firstSwapNs']-b['firstSwapNs']
    elif (b['firstSwapNs'] is not None and
          a['data']['outcomes']['terminalReason']=='DEADLINE_CENSORED'):
        receiving=[e for e in a['data']['events'] if e['operation']=='CONFIG_RECEIVED']
        if receiving:
            lower_bound=(u64(a['data']['experiment']['observeEndNs'])-
                         u64(receiving[0]['ns'])-b['firstSwapNs'])
    return {'attempts':len(runs),'firstSwapSuccesses':sum(r['firstSwapNs'] is not None for r in runs),
            'states':[r['data']['outcomes']['terminalReason'] for r in runs],
            'pairings':pair_results,'coldSavingsNs':cold_delta,'coldSavingsLowerBoundNs':lower_bound,
            'productPASS':False}

def main():
    p=argparse.ArgumentParser()
    commands=p.add_subparsers(dest='command',required=True)
    commands.add_parser('run').add_argument('measurement',type=Path)
    pair=commands.add_parser('pair')
    pair.add_argument('deployment',type=Path)
    pair.add_argument('measurements',type=Path,nargs=4)
    pages=commands.add_parser('pages')
    pages.add_argument('directory',type=Path)
    pages.add_argument('round',type=int)
    args=p.parse_args()
    if args.command=='pages':
        raw=analyze(args.directory,args.round)
        raw['decision']=decide(raw,load(args.directory/f'r{args.round}-status.json'))
        output=raw
    elif args.command=='pair':
        output=compare(args.measurements,load(args.deployment))
    else:
        output=inspect(args.measurement)
        output.pop('data')
    print(json.dumps(output,indent=2))

if __name__=='__main__':
    main()
