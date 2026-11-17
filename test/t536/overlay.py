from pathlib import Path
import hashlib,json,re,sys
from specialize import generate

ROOT=Path(__file__).resolve().parents[2]
def one(text,anchor,replacement):
    if text.count(anchor)!=1:raise ValueError('T536_ANCHOR '+anchor[:90])
    return text.replace(anchor,replacement)

def overlay(out,active):
    base=out/'entry/src/main/cpp/gpu_resident'
    shared=base/'stage_specialization';shared.mkdir(exist_ok=True)
    (shared/'programs.h').write_bytes((ROOT/'entry/src/main/cpp/gpu_resident/stage_specialization/programs.h').read_bytes())
    routes=['m1']+(['m2route'] if active=='m2' else [])
    for route in routes:
        generate(base,route)
        d=base/route
        h=(d/'quantum_candidate.h').read_text(encoding='utf8')
        h=one(h,'#pragma once','#pragma once\n#include "../stage_specialization/programs.h"')
        h=one(h,' UniformLocations uniformLocations;',' UniformLocations uniformLocations;\n special536::Programs families;bool familyProtocolFault=false;')
        (d/'quantum_candidate.h').write_text(h,encoding='utf8')
        s=(d/'quantum_candidate.cpp').read_text(encoding='utf8')
        s='#include "../stage_specialization/'+route+'/kernels.h"\n'+s
        s=one(s,'glUniform1ui(uniformLocations.Find(name),v);','if(families.Live())families.Put(name,v);else glUniform1ui(uniformLocations.Find(name),v);')
        s=one(s,'Set("uPass",stage);','Set("uPass",stage);if(families.Live()&&!families.Bind(stage)){familyProtocolFault=true;return;}')
        begin=s.index('if(!programcache::Build(1,');end=s.index('\n SubmitScope resources',begin)
        s=s[:begin]+f'if(!families.Create(special536::kSources_{route},submitObserver,firstGlError)){{Release();return TransportCode::SHADER_FAILED;}}program=families.Id(0);'+s[end:]
        s=s.replace('glUniform1i(uniformLocations.Find("uRequestedAngle"),p.requestedAngle);','families.Put("uRequestedAngle",uint32_t(p.requestedAngle),0,1);')
        for name,field in [('uNonce','nonce'),('uFrame','frame'),('uControl','control')]:s=one(s,f'glUniform2ui(uniformLocations.Find("{name}"),r.{field}.lo,r.{field}.hi);',f'families.Put("{name}",r.{field}.lo,r.{field}.hi,2);')
        s=one(s,'submittedUniformBytes=0;ready=false;','submittedUniformBytes=0;families.ResetValues();familyProtocolFault=false;ready=false;')
        s=s.replace('if(program){programcache::Forget(program);glDeleteProgram(program);}','if(families.Live())families.Release(true);else if(program){programcache::Forget(program);glDeleteProgram(program);}')
        s=one(s,'void QuantumCandidate::ContextLost(){','void QuantumCandidate::ContextLost(){families.Release(false);')
        s=s.replace('BuildPassTable();if(resource501::current)', 'BuildPassTable();if(resource501::current)resource501::current->HostStorage("T536_STAGE_PROGRAMS",reinterpret_cast<uintptr_t>(&families),sizeof(families),sizeof(families));if(resource501::current)')
        # Preserve wire's logical staging count. Physical GL uniform bytes and
        # program switches are independent counters in Programs/cold receipt.
        (d/'quantum_candidate.cpp').write_text(s,encoding='utf8')
        p=d/'full_transport.h';s=p.read_text(encoding='utf8')
        anchor=' TransportCode PrepareColdPrefix('
        pos=s.index(anchor);s=s[:pos]+' TransportCode PrepareAllStages(const QuantumParameters&,const ColdPrepareOptions&,uint64_t,uint32_t&);\n'+s[pos:];p.write_text(s,encoding='utf8')
        p=d/'full_transport.cpp';s=p.read_text(encoding='utf8')
        begin=s.index(' glDeleteProgram(quantum.program);');end=s.index('\n ',begin+2)
        s=s[:begin]+' // T536: QuantumCandidate already owns all eight final stage families.\n'+s[end:]
        begin=s.index(' if(preparation.twoBatches)prepared=');end=s.index(' warmupWallNs=',begin)
        s=s[:begin]+' prepared=PrepareAllStages(cold,preparation,start,error);\n'+s[end:]
        inc=(ROOT/'entry/src/main/cpp/gpu_resident/stage_specialization/cold_prepare.inc').read_text(encoding='utf8')
        inc=inc.replace('command[1]=2;','command[1]='+('2' if route=='m1' else '3')+';')
        inc=inc.replace('T536_SOURCE_UPLOADED','232' if route=='m1' else '240').replace('T536_SOURCE_PASSES','8' if route=='m1' else '13').replace('T536_SOURCE_PUB','5' if route=='m1' else '10')
        at=s.index('TransportCode FullM1Transport::PrepareColdPrefix(');s=s[:at]+inc+'\n'+s[at:]
        s=one(s,'TransportCode FullM1Transport::ObserveBatch(){','TransportCode FullM1Transport::ObserveBatch(){if(quantum.familyProtocolFault)return TransportCode::PROTOCOL_REJECTED;')
        p.write_text(s,encoding='utf8')
    for route in routes:
        kernel=base/route/'full_kernel.cpp'
        original=kernel.read_text(encoding='utf8')
        family_manifest=json.loads((shared/route/'manifest.json').read_text(encoding='utf8'))
        bundle=hashlib.sha256((original+json.dumps(family_manifest,sort_keys=True)).encode()).hexdigest()
        updated,count=re.subn(r'const char\* FullM1ShaderSha\(\)\{return [^;]+;\}', 'const char* FullM1ShaderSha(){return "'+bundle+'";}', original)
        if count!=1:raise ValueError('T536_BUNDLE_IDENTITY '+route)
        kernel.write_text(updated,encoding='utf8')
        family_manifest['bundleSha256']=bundle
        (shared/route/'manifest.json').write_text(json.dumps(family_manifest,indent=2)+'\n',encoding='utf8')
    # Cold compile assembly is the actual frozen T507 M1/M2 input; no mutable
    # parent worktree paths are referenced by the copied native project.
    manifest={'revision':'T536_STAGE_FAMILIES_DELIVERY_V1','route':active,'tests':'NOT_RUN_BY_USER_DIRECTION','device':'NOT_RUN','groups':{}}
    for route in routes:manifest['groups'][route]=json.loads((shared/route/'manifest.json').read_text(encoding='utf8'))
    manifest['generatedNativeSha256']={str(f.relative_to(base)):hashlib.sha256(f.read_bytes()).hexdigest() for route in routes for name in ['full_transport.cpp','full_transport.h','quantum_candidate.cpp','quantum_candidate.h','full_kernel.cpp'] if (f:=base/route/name).is_file()}
    (out/'t536-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
    profile=out/'build-profile.json5';doc=json.loads(profile.read_text(encoding='utf8'));doc['app'].pop('signingConfigs',None)
    for product in doc['app']['products']:product.pop('signingConfig',None)
    profile.write_text(json.dumps(doc,indent=2)+'\n',encoding='utf8')

if __name__=='__main__':overlay(Path(sys.argv[1]),sys.argv[2])
