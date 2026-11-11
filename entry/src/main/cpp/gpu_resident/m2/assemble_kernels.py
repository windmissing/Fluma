from pathlib import Path
import argparse
import hashlib
import re

M2 = Path(__file__).resolve().parent
M1 = M2.parent / 'm1'


def read(path):
    return path.read_text(encoding='utf-8').strip() + '\n'


def source_shader():
    seal = read(M1 / 'source_seal.glsl')
    pack = seal[seal.index('void packPublication()'):seal.index('void sealCompletion()')]
    pub = read(M1 / 'source_audit.glsl').split('bool auditPublication(){', 1)[1]
    pack = pack.replace('E[h+1u]=2u;', 'E[h+1u]=3u;')
    pub = 'bool auditPublication(){' + pub.replace('E[h+1u]!=2u', 'E[h+1u]!=3u')
    # These M1 publication helpers also serve M2's coherent Candidate block.
    # Keep the coherent load in the getter and pass only its value to arithmetic.
    def publication_values(text):
        return re.sub(r'(grsColumnPreservingFloatBits|grsQ20FloatBits)\(C\[([^\[\]]+)\]\)',
                      r'\1(m2CandidateValue(\2))', text)
    pack = publication_values(pack)
    pub = publication_values(pub)
    completion = seal[seal.index('void sealCompletion()'):].replace('void sealCompletion()', 'void sealCompletionV3()')
    completion = completion.replace('W[S+14u]!=127u', 'W[S+14u]!=4095u')
    completion = completion.replace('E[1]=2u;', 'E[1]=3u;').replace('W[S]==4u?9u:0u', 'W[S]==4u?W[ctrl()+3u]:0u')
    completion = completion.replace('E[51]=uEventArena;E[53]=W[S+9u];E[54]=uK+1u;', '')
    completion = completion.replace('W[S]==4u?W[S+14u]:255u', 'W[S]==4u?W[S+14u]:8191u')
    completion = completion.replace('E[96]=W[S+9u];', 'E[96]=0u;')
    completion = completion.replace('E[110]=8u;E[111]=9u;', 'E[110]=13u;E[111]=14u;')
    completion = completion.replace('E[154]=W[2]==1u?W[S+9u]:0u;', 'E[154]=W[2]==1u&&W[S]==1u?W[T+6u]:0u;')
    completion = completion.replace(' E[3]=3235823838u;', ''' for(uint i=0u;i<13u;++i)E[160u+i]=C[uHeader+132u+i];
 if(W[S]==4u){E[102]=W[S+16u];E[103]=W[S+17u];E[31]=W[ctrl()+4u];E[169]=W[S+9u];E[172]=2u;}
 memoryBarrierBuffer();E[3]=3235823838u;''')
    text = read(M2 / 'source_v3.comp')
    for marker, body in {
        'INTEGER_AND_LAYOUT': read(M2.parent / 'integer_primitives.glsl') + read(M2 / 'layout_generated.glsl'),
        'OPERATIONS': ''.join(read(M2 / f) for f in ['merge_primitives.glsl','pair_index.glsl','source_row.glsl','source_execute.glsl']),
        'AUDIT_AND_DRIVER': read(M2 / 'source_audit.glsl'),
        'PUBLICATION': pack + pub + completion,
    }.items():
        token = '/* ' + marker + ' */'
        if text.count(token) != 1:
            raise ValueError('wrong GLSL assembly marker ' + token)
        text = text.replace(token, body)
    return text


def replace_one(text, old, new):
    if text.count(old) != 1:
        raise ValueError('V3 assembly base drift: ' + old)
    return text.replace(old, new)


def quantum_shader():
    core = read(M1 / 'quantum_core.comp')
    stage = read(M1 / 'quantum_stages.glsl')
    stage = replace_one(stage, 'C[uHeader+1u]!=2u', 'C[uHeader+1u]!=3u')
    stage = replace_one(stage, 'C[uHeader+31u]!=3u', 'C[uHeader+31u]!=7u')
    stage = read(M2 / 'quantum_origins.glsl') + replace_one(stage, ' uvec2 es,ew;', ' if(!quantumOriginsV3()){qfail(15u);return false;}\n uvec2 es,ew;')
    for marker, value in {'INTEGER_PRIMITIVES':read(M2.parent/'integer_primitives.glsl'),
                          'EVENT_MASKS':read(M1/'quantum_event_masks.glsl'),'T16':read(M1/'quantum_t16.glsl'),
                          'MOTION':read(M1/'quantum_motion.glsl'),'STAGES':stage}.items():
        core = replace_one(core, '/* '+marker+' */', value)
    core = replace_one(core, 'e[1]=2u;', 'e[1]=3u;')
    core = replace_one(core, 'W[3]=W[1]+1u;', 'W[3]=uPass==23u?W[28]:W[1]+1u;')
    core = replace_one(core, 'void main(){', 'void candidateMain(){')
    replay = read(M1/'quantum_replay.glsl')
    replay = replace_one(replay, 'ev(row,1u)==2u', 'ev(row,1u)==3u')
    replay = replace_one(replay, 'W[a+14u]==0u&&W[a+15u]==0u', 'W[a+14u]<=uK&&W[a+15u]<=5u')
    replay = replace_one(replay, 'for(uint i=132u;i<256u;i++)',
                         'for(uint i=132u;i<145u;i++)if(!requireQ(C[uHeader+i]==B[uHeader+i],15u))return false;\n for(uint i=145u;i<256u;i++)')
    seal = read(M1/'quantum_seal.glsl')
    seal = replace_one(seal,'C[uHeader+81u]=2u;', 'C[uHeader+81u]=3u;')
    seal = replace_one(seal,'E[1]=2u;', 'E[1]=3u;')
    seal = replace_one(seal, 'E[3]=3235823838u;', 'for(uint i=0u;i<13u;++i)E[160u+i]=C[uHeader+132u+i];memoryBarrierBuffer();E[3]=3235823838u;')
    pubSource = read(M1/'source_seal.glsl')
    pack = pubSource[pubSource.index('void packPublication()'):pubSource.index('void sealCompletion()')]
    pack = replace_one(pack, 'E[h+1u]=2u;', 'E[h+1u]=3u;').replace('if(W[S]==4u)', 'if(W[0]!=0u)')
    audit = 'bool auditPublication(){'+read(M1/'source_audit.glsl').split('bool auditPublication(){',1)[1]
    audit = replace_one(audit, 'E[h+1u]!=2u', 'E[h+1u]!=3u').replace('if(W[S]==4u)', 'if(W[0]!=0u)').replace('W[S+15u]', 'W[18]')
    return core+replay+read(M1/'quantum_geometry_audit.glsl')+seal+pack+audit+read(M1/'quantum_commit_main.glsl')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('out', type=Path)
    p.add_argument('--quantum', action='store_true')
    args = p.parse_args()
    data = quantum_shader() if args.quantum else source_shader()
    args.out.write_text(data, encoding='utf-8', newline='\n')
    print(hashlib.sha256(data.encode()).hexdigest())
