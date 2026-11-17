"""Compile-source transform only: preserve each original stage body verbatim.

Remove every non-family entry and every unreachable function definition before
the driver sees the source. No fixture, dynamics or GPU execution occurs here.
"""
import hashlib
import json
from pathlib import Path
import re
import sys

GROUPS = [('COPY',[0,20,22,28]),('PREFLIGHT',[1,24]),('WATER',[2,3,4]),('INDEX_SORT',[6,7,8,9,16,17]),('TRANSPORT',[5,10,11,15,18]),('BED',[12,13,14]),('AUDIT_REPLAY',[19,21,23]),('PUBLICATION',[25,26,27])]

def mask_comments(source):
    return re.sub(r'/\*[\s\S]*?\*/|//[^\n]*',lambda m:re.sub(r'[^\n]',' ',m[0]),source)

def brace_end(source,start):
    depth=0
    for i in range(start,len(source)):
        if source[i]=='{':depth+=1
        elif source[i]=='}':
            depth-=1
            if not depth:return i+1
    raise ValueError('UNBALANCED_GLSL')

def functions(source):
    masked=mask_comments(source);out=[];cursor=0
    pattern=re.compile(r'\b(?:void|bool|uint|int|float|[uib]?vec[234]|[A-Za-z_]\w*)\s+(\w+)\s*\([^;{}]*\)\s*\{')
    while match:=pattern.search(masked,cursor):
        end=brace_end(masked,match.end()-1);out.append((match[1],match.start(),end));cursor=end
    return out

def stage_bodies(source):
    result={}
    for name,a,b in functions(source):
        if name not in ('main','candidateMain'):continue
        text=source[a:b];masked=mask_comments(text)
        for match in re.finditer(r'if\(uPass==(\d+)u\)\s*\{',masked):
            n=int(match[1]);end=brace_end(masked,match.end()-1)
            if n in result:raise ValueError('DUPLICATE_STAGE '+str(n))
            result[n]=text[match.end():end-1]
    if set(result)!=set(range(29)):raise ValueError('STAGE_COVERAGE '+str(sorted(result)))
    return result

def specialize(source,stages):
    bodies=stage_bodies(source)
    # Original exceptional ordering: copy0/rollback20/completion27 occur before
    # the error gate; every other stage is guarded by the original W[0].
    main='void main(){uint i=gl_GlobalInvocationID.x;\n'
    for n in stages:
        gate='' if n in (0,20,27) else 'if(W[0]!=0u)return;'
        main+=f'if(uPass=={n}u){{'+gate+bodies[n]+'}\n'
    main+='}\n'
    definitions=functions(source);names={n for n,_,_ in definitions};needed={'main'}
    while True:
        before=len(needed)
        for n,a,b in definitions:
            if n in needed and n not in ('main','candidateMain'):
                needed.update(set(re.findall(r'\b(\w+)\s*\(',mask_comments(source[a:b])))&names)
        needed.update(set(re.findall(r'\b(\w+)\s*\(',main))&names)
        if len(needed)==before:break
    chunks=[];at=0
    for n,a,b in definitions:
        chunks.append(source[at:a]);at=b
        if n in needed and n not in ('main','candidateMain'):chunks.append(source[a:b])
    chunks.append(source[at:]);chunks.append(main)
    return ''.join(chunks).rstrip()+'\n',sorted(needed-{'main'})

def assemble_m1(base):
    read=lambda p:(base/p).read_text(encoding='utf8').replace('\r\n','\n')
    q=read('m1/quantum_core.comp')
    for marker,file in [('INTEGER_PRIMITIVES','integer_primitives.glsl'),('EVENT_MASKS','m1/quantum_event_masks.glsl'),('T16','m1/quantum_t16.glsl'),('MOTION','m1/quantum_motion.glsl'),('STAGES','m1/quantum_stages.glsl')]:q=q.replace('/* '+marker+' */',read(file))
    q=q.replace('void main(){','void candidateMain(){').replace('W[3]=W[1]+1u;','W[3]=uPass==23u?W[28]:W[1]+1u;')
    for part in ['quantum_replay.glsl','quantum_geometry_audit.glsl','quantum_seal.glsl']:q+='\n'+read('m1/'+part)
    seal=read('m1/source_seal.glsl');pack=seal[seal.index('void packPublication(){'):seal.index('void sealCompletion(){')]
    audit=read('m1/source_audit.glsl');audit=audit[audit.index('bool auditPublication(){'):]
    pack=pack.replace('if(W[S]==4u)','if(W[0]!=0u)');audit=audit.replace('if(W[S]==4u)','if(W[0]!=0u)').replace('W[S+15u]|=512u;return W[S+15u]==1023u;','W[18]|=512u;return W[18]==1023u;')
    return q+'\n'+pack+'\n'+audit+'\n'+read('m1/quantum_commit_main.glsl')

def generate(base,route):
    if route=='m1':source=assemble_m1(base)
    else:
        text=(base/'m2route/full_kernel.cpp').read_text(encoding='utf8')
        pos=text.index('const char* QuantumCommitKernel()');match=re.search(r'R"(\w+)\(([\s\S]*?)\)\1"',text[pos:]);source=match[2]
    out=base/'stage_specialization'/route;out.mkdir(parents=True,exist_ok=True)
    manifest={'revision':'T536_STAGE_FAMILY_V1','route':route,'originalSha256':hashlib.sha256(source.encode()).hexdigest(),'groups':[]}
    header=['#pragma once','#include "../programs.h"','namespace grs::special536 {']
    entries=[]
    for i,(name,stages) in enumerate(GROUPS):
        text,reachable=specialize(source,stages);sha=hashlib.sha256(text.encode()).hexdigest();abi=f'T536|{route}|{name}|fourSSBO|stage29-v1'
        (out/(name.lower()+'.comp')).write_text(text,encoding='utf8',newline='\n')
        required=sorted(set(re.findall(r'\bu[A-Z]\w*\b',mask_comments(text))))
        # Includes declared-but-optimized uniforms; actual active set is reflected
        # by the driver, and every active uniform must have a staging value.
        manifest['groups'].append({'family':i,'name':name,'stages':stages,'sha256':sha,'bytes':len(text.encode()),'functions':reachable,'declaredUniformNames':required})
        header.append(f'static constexpr const char* s{i}=R"FAMILY536({text})FAMILY536";')
        entries.append('{"Q_'+name+'",s'+str(i)+',"'+sha+'","'+abi+'"}')
    header.append('inline constexpr std::array<Source,8> kSources_'+route+'={{'+','.join(entries)+'}};')
    header.append('}');(out/'kernels.h').write_text('\n'.join(header)+'\n',encoding='utf8',newline='\n')
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')

if __name__=='__main__':
    generate(Path(sys.argv[1]),sys.argv[2])
