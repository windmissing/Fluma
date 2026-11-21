"""Audit frozen DM/RX artifacts without executing ARM64 binaries."""
from pathlib import Path
import json
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
import zipfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'test/offline_handoff'))
from package import build_id, READELF
from hashlib import sha256
GLSL=ROOT/'.codex/local-test/g13-glslang/bin/glslang.exe'

def sha(raw):
    return sha256(raw).hexdigest()

def load(path):
    return json.loads((ROOT/path).read_bytes())

def raw(info,base=ROOT):
    p=base/info['path']
    data=p.read_bytes()
    assert sha(data)==info['sha256'],str(p)
    return data

def hap(info,name,digest,bid=None,base=ROOT):
    raw(info,base)
    with zipfile.ZipFile(base/info['path']) as z:
        data=z.read('libs/arm64-v8a/'+name)
        assert sha(data)==digest and build_id(data) and (bid is None or build_id(data)==bid)
    return data

def elf(info,base=ROOT):
    data=raw(info,base)
    assert build_id(data)==info['buildId']
    args=[str(READELF),'-h','-d','-n',str(base/info['path'])]
    r=subprocess.run(args,check=True,capture_output=True,text=True)
    deps=re.findall(r'NEEDED.*?\[([^\]]+)\]',r.stdout)
    assert 'AArch64' in r.stdout and deps==info['needed']
    return {'argv':args,'rawReadelf':r.stdout,'DT_NEEDED':deps,'sha256':sha(data),'BuildId':build_id(data)}

def compile_shader(path,expected=True):
    args=[str(GLSL),'-S','comp',str(path)]
    r=subprocess.run(args,capture_output=True,text=True)
    if expected is not None:
        assert (r.returncode==0)==expected,(str(path),r.stdout,r.stderr)
    return {'argv':args,'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr,'sha256':sha(path.read_bytes())}

def linked_shader(path,data,digest):
    shader=path.read_bytes()
    assert sha(shader)==digest and shader+b'\0' in data
    return compile_shader(path)

def archived_shader(data,digest,directory,name):
    exact=[data[m.start():].split(b'\0',1)[0] for m in re.finditer(rb'#version 310 es',data)]
    matches=[source for source in exact if sha(source)==digest]
    assert len(matches)==1,name
    path=directory/(name+'.comp')
    path.write_bytes(matches[0])
    return path

def audit(directory):
    dm=load(Path('test/reports/t718/compile.json'))
    rx=load(Path('test/reports/t720/compile.json'))['builds']['rx']
    dm_current=load(Path('test/reports/t748/compile.json'))
    rx_current=load(Path('test/reports/t763/compile.json'))
    dso='libgrs_dm_t647.so'
    rso='libgrs_g9rx.so'
    legacy_dm=hap(dm['unsignedHap'],dso,dm['unsignedHap']['packagedElfSHA256'],dm['sharedLibrary']['buildId'])
    h=rx['unsignedHap']
    historical_rx=load(Path('test/reports/t257/T720-rx.json'))
    assert historical_rx['verifyApp']=='PASS' and historical_rx['input']['sha256']==h['sha256']
    observed_unsigned_rx_sha=sha((ROOT/h['path']).read_bytes())
    legacy_rx=hap(historical_rx['signed'],rso,h['elf']['sha256'],h['elf']['buildId'])
    historical={'T720_RX_unsignedAtHistoricalPath':{'expected':h['sha256'],
                'observed':observed_unsigned_rx_sha,'matches':h['sha256']==observed_unsigned_rx_sha,
                'authoritativeOriginal':'T720-rx signed HAP'}}
    for task,hap_original,expected_elf in (('T718-dm',dm['unsignedHap'],legacy_dm),('T720-rx',h,legacy_rx)):
        signing=load(Path(f'test/reports/t257/{task}.json'))
        assert signing['verifyApp']=='PASS' and signing['input']['sha256']==hap_original['sha256']
        raw(signing['signed'])
        name=dso if task.endswith('dm') else rso
        with zipfile.ZipFile(ROOT/signing['signed']['path']) as signed:
            assert signed.read('libs/arm64-v8a/'+name)==expected_elf
        historical[task]=signing['signed']
    shaders={}
    dm_path=archived_shader(legacy_dm,dm['shaderSHA'],directory,'T718_DM')
    rx_path=archived_shader(legacy_rx,h['shader']['sha256'],directory,'T720_RX')
    shaders['T718_DM']=linked_shader(dm_path,legacy_dm,dm['shaderSHA'])
    shaders['T720_RX']=linked_shader(rx_path,legacy_rx,h['shader']['sha256'])
    entries={'T718_DM_DRIVER':elf(dm['differential']),'T748_DM_DRIVER':elf(dm_current['driver'])}
    old=load(Path('test/reports/t670/dm-native.json'))
    old_copy='.codex/local-test/t670-dm/entry/build/default/intermediates/cmake/default/obj/arm64-v8a/g9_dm_differential'
    entries['T670_DM_DRIVER']=elf(dict(old['executable'],path=old_copy,buildId=old['buildId'],
                                     needed=re.findall(r'NEEDED.*?\[([^\]]+)\]',old['dynamic'])))
    entries['T670_DM_DRIVER']['receiptPathSHAObserved']=sha(Path(old['executable']['path']).read_bytes())
    assert dso in entries['T748_DM_DRIVER']['DT_NEEDED'] and not any(
        'grsm' in n for n in entries['T748_DM_DRIVER']['DT_NEEDED'])
    current={}
    for label,receipt,name in [('T748_DM',dm_current,dso),('T763_RX',rx_current,rso)]:
        package=receipt.get('packagedElf',receipt.get('pairedDsoFromHap'))
        lib=hap(receipt['unsignedHap'],name,package['sha256'],package['buildId'])
        assert lib==raw(package)
        raw(receipt.get('roleManifest',receipt.get('sourceManifest')))
        for role,record in receipt['roles'].items():
            if 'path' in record:
                shaders[label+'_'+role]=linked_shader(ROOT/record['path'],lib,record['sha256'])
        current[label]={'BuildId':package['buildId'],'elfSHA':sha(lib),'hapSHA':receipt['unsignedHap']['sha256']}
    dm_tree=ROOT.with_name('HelloWorld-T771')
    t771_path=dm_tree/'docs/receipt-T771-compile-v1.json'
    t771=json.loads(t771_path.read_bytes())
    entries['T771_DM_DRIVER']=elf(t771['driver'],dm_tree)
    assert dso in entries['T771_DM_DRIVER']['DT_NEEDED']
    package=t771['pairedDsoFromHap']
    dmso=hap(t771['unsignedHap'],dso,package['sha256'],package['buildId'],dm_tree)
    assert dmso==raw(package,dm_tree)
    raw(t771['sourceManifest'],dm_tree)
    for role,r in t771['roles'].items():
        shaders['T771_DM_'+role]=linked_shader(dm_tree/r['path'],dmso,r['sha256'])
    current['T771_DM']={'BuildId':package['buildId'],'elfSHA':sha(dmso),'hapSHA':t771['unsignedHap']['sha256'],
                        'receiptSHA':sha(t771_path.read_bytes())}
    product=t771['product']
    dm_default=hap({'path':product['hap'],'sha256':product['sha256']},dso,product['elfSHA256'],base=dm_tree)
    rx_default=rx_current['defaultRx']
    with zipfile.ZipFile(ROOT/rx_default['hap']['path']) as z:
        rx_payload=z.read('libs/arm64-v8a/'+rso)
    raw(rx_default['hap'])
    assert build_id(rx_payload)==rx_default['elf']['buildId']
    for name,lib,info in [('T771_DM_default',dm_default,product),('T763_RX_default',rx_payload,rx_default['hap'])]:
        current[name]={'BuildId':build_id(lib),'elfSHA':sha(lib),'hapSHA':info['sha256']}
        large={source for source in re.findall(rb'#version 310 es[^\0]*',lib) if len(source)>=50000}
        assert len(large)==1,name
        source=large.pop()
        out=directory/f'{name}_{sha(source)[:12]}.comp'
        out.write_bytes(source)
        shaders[name]=compile_shader(out)
    mapping_path=Path('test/reports/t491/t766/v6/g9boot-t766.jsonl')
    mapping=load(mapping_path)
    assert mapping and all(r['mappedBuildId']==current['T763_RX']['BuildId'] and
                           r['sourceSHA']==rx_current['roles']['boot']['sha256'] for r in mapping)
    historical['T766_RX_mapping']={'path':str(mapping_path),'sha256':sha((ROOT/mapping_path).read_bytes()),
                                  'pids':sorted({r['pid'] for r in mapping}),'rows':len(mapping),
                                  'BuildId':mapping[0]['mappedBuildId']}
    return shaders,entries,current,historical,dm_path,rx_path,dm

def controls(dm_path,rx_path,dm):
    sys.path.insert(0,str(ROOT/'tools/glsl'))
    sys.path.insert(0,str(ROOT/'tools/g9rx'))
    from qualified_values import direct_value_arguments
    from coherent_values import direct_reads
    rx_id=load(Path('.codex/local-test/t763-final/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/g9rx/rx_assets/pipeline_identity.json'))
    assert sha(rx_path.read_bytes())==rx_id['shaderSHA']
    out={}
    for name,path,groups in (
        ('DM',dm_path,[dm['readonlyTransform']['arguments']]),
        ('RX',rx_path,[rx_id['coherentArgumentTransforms'],rx_id['readonlyArgumentTransforms']])):
        lines=path.read_text(encoding='utf8').splitlines()
        out[name]=[]
        for group in groups:
            assert group
            for item in group:
                line=item['canonicalLine']
                assert item['after'] in lines[line-1],(name,line,item['after'])
                out[name].append({'canonicalLine':line,'actual':lines[line-1],'before':item['before']})
        text='\n'.join(lines)+'\n'
        assert not list(direct_value_arguments(text,'B')),name
        readonly=groups[-1][0]
        revised=list(lines)
        at=readonly['canonicalLine']-1
        revised[at]=revised[at].replace(readonly['after'],readonly['before'],1)
        assert list(direct_value_arguments('\n'.join(revised),'B'))
        out[name+'ReadonlyRollback']='DIRECT_VALUE_ARGUMENT_DETECTED'
        if name=='RX':
            assert not any(r[-1] for r in direct_reads(text)),name
            coherent=groups[0][0]
            revised=list(lines)
            at=coherent['canonicalLine']-1
            revised[at]=revised[at].replace(coherent['after'],coherent['before'],1)
            assert any(r[-1] for r in direct_reads('\n'.join(revised)))
            out['RXCoherentRollback']='DIRECT_VALUE_ARGUMENT_DETECTED'
        capture=text.split('bool g9Capture(){',1)[1].split('\n}',1)[0]
        assert all(re.search(r'\buint '+v+r'\b',capture) for v in ('clearSlot','scanSlot','selectedSlot'))
    with TemporaryDirectory(prefix='t780-glsl-') as d:
        temp=Path(d)
        preamble='#version 310 es\nprecision highp int;\nlayout(local_size_x=1) in;\n'
        negative=temp/'duplicate.comp'
        negative.write_text(preamble+'void main(){uint slot=0u;uint slot=1u;}\n')
        out['redeclaredSlot']=compile_shader(negative,False)
        out['redeclaredSlot']['source']=negative.read_text()
        positive=temp/'scoped.comp'
        positive.write_text(preamble+'void main(){{uint slot=0u;}{uint slot=1u;}}\n')
        out['legalScopes']=compile_shader(positive)
        for spec,block in [('coherent','coherent'),('readonly','readonly')]:
            for direct in (True,False):
                case=temp/(spec+str(direct)+'.comp')
                expression='B[0]' if direct else 'copyValue(0u)'
                case.write_text(preamble+f'layout(std430,binding=0) {block} buffer InputState{{uint B[];}};\n'
                    f'uint copyValue(uint i){{return B[i];}}\nvoid consume(uint x){{}}\nvoid main(){{consume({expression});}}\n')
                result=compile_shader(case,None if direct else True)
                result['source']=case.read_text()
                out[spec+('DirectObserved' if direct else 'ValueCopy')]=result
    return out

if __name__=='__main__':
    with TemporaryDirectory(prefix='t780-archived-shaders-') as temp:
        shaders,entries,current,historical,dm_path,rx_path,dm=audit(Path(temp))
        print(json.dumps({'shaders':shaders,'independentELF':entries,'current':current,
                          'historicalSigned':historical,'controls':controls(dm_path,rx_path,dm),
                          'historicalDiagnostics':{
                              'DMReadonly':dm['priorAttribution'],
                              'RXCurrentDeviceOriginal':'test/reports/t491/t766/v6/g9boot-t766.jsonl',
                              'RXCurrentMapping':'T766 historical only, separate from this offline run'},
                          'loadedRuntimeIdentity':'NOT_OBSERVED_OFFLINE','device':'NOT_RUN'},indent=2))
