"""Build G13's ABI and immutable shader assets; never executes physics."""
from pathlib import Path
import argparse, hashlib, json, re
from fractions import Fraction
import struct
from split_shader import LIMIT, roles

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / 'entry/src/main/cpp/gpu_resident/g13flow'

def ident(s):
    return re.sub('[^A-Za-z0-9_]', '_', s)

def sha(b):
    return hashlib.sha256(b).hexdigest()

def binary128(q):
    q = Fraction(q)
    sign = q < 0
    q = abs(q)
    if not q:
        return [0]*4
    e = q.numerator.bit_length()-q.denominator.bit_length()
    if q < Fraction(2)**e:
        e -= 1
    v = q / Fraction(2)**(e-112)
    n, r = divmod(v.numerator, v.denominator)
    n += 2*r > v.denominator or (2*r == v.denominator and n % 2)
    if n == 1 << 113:
        n >>= 1
        e += 1
    if not -16382 <= e <= 16383:
        raise ValueError('coefficient exponent')
    bits = (int(sign) << 127) | ((e+16383) << 112) | (n- (1 << 112))
    return [(bits >> (32*i)) & 0xffffffff for i in range(4)]

def generate(out, hooks=False, split=False):
    out.mkdir(parents=True, exist_ok=True)
    tables = {}
    deps = {}
    for name in ('engineering', 'layout', 'dispatch', 'math'):
        p = ROOT / f'docs/architecture-T607-g13-{name}-v1.json'
        b = p.read_bytes()
        deps[str(p.relative_to(ROOT))] = sha(b)
        tables[name] = json.loads(b)
    for item in tables['engineering']['inputs']:
        path=ROOT/item['path'];data=path.read_bytes()
        if len(data)!=item['bytes'] or sha(data)!=item['sha256']:
            raise ValueError('approved input fingerprint: '+item['path'])
        deps[item['path']]=sha(data)
    l, d, m = (tables[k] for k in ('layout', 'dispatch', 'math'))
    domain_path=ROOT/'docs/architecture-T628-math-domain-v1.json'
    domain_raw=domain_path.read_bytes();domain=json.loads(domain_raw)
    if m.get('domainContract')!=domain or m['mathSHA']!=tables['engineering']['hashes']['mathSHA']:
        raise ValueError('T628 domain/math identity mismatch')
    if domain['firstFault']['seal']!=3235779345 or domain['publicError']['code']!=17:
        raise ValueError('unsupported first-fault contract')
    deps[str(domain_path.relative_to(ROOT))]=sha(domain_raw)

    if len(d['quantum']) != 820 or len(d['source']) != 16 or len(set(d['opcodes'].values())) != 64:
        raise ValueError('dispatch contract')
    h = ['#pragma once', '#include <array>', '#include <cstdint>', 'namespace grs::g13 {',
         'inline constexpr uint32_t kSeal=0xc0de1301u;',
         'struct Pass { uint32_t op,scope,J,micro,block; };']
    g = []
    for name, value in l['magic'].items():
        h.append(f'inline constexpr uint32_t kMagic_{name}={value}u;')
    for name, fields in l['schemas'].items():
        if not isinstance(fields,list):
            continue
        repeated={field for field in fields if fields.count(field)>1}
        h.append('namespace '+name+' {')
        for i, field in enumerate(fields):
            if field in repeated:
                field += '_'+str(i)
            h.append(f'inline constexpr uint32_t {ident(field)}={i}u;')
            g.append(f'#define {name}_{ident(field)} {i}u')
        h.append('}')
    for name, value in d['opcodes'].items():
        h.append(f'inline constexpr uint32_t OP_{name}={value}u;')
        g.append(f'#define OP_{name} {value}u')
    for name in ('quantum', 'source'):
        h.append(f'inline constexpr Pass k_{name}[]={{')
        for p in d[name]:
            h.append('{'+','.join(str(p.get(k,0))+'u' for k in ('opcode','scope','J','micro','iterationBlock'))+'},')
        h.append('};')
    h += ['struct Range {uint32_t offset,words;};',
          'struct Profile {uint32_t nx,ny,nc,nf,sw,ww,pw,mgCount,groups; Range state[5],work[20]; uint32_t mg[6][3];};',
          'inline constexpr Profile kProfiles[]={']
    for p in l['profiles']:
        for group in ('state', 'work', 'CTRL', 'traceScope'):
            last = 0
            for r in p[group]:
                if r['wordStart'] != last or r['byteStart'] != last*4 or r['bytes'] != r['words']*4:
                    raise ValueError(f'layout overlap/gap {group}')
                last += r['words']
        values = [p['Nx'], p['Ny'], p['Nc'], p['Nv'], p['stateBytes']//4, p['workBytes']//4,
                  p['renderPackBytes']//4,len(p['MG']),p['reductionGroups']]
        ranges = [','.join('{'+str(r['wordStart'])+'u,'+str(r['words'])+'u}' for r in p[k]) for k in ('state','work')]
        levels = ','.join('{'+','.join(str(x)+'u' for x in (v['Nx'],v['Ny'],v['packedCellStart']))+'}' for v in p['MG'])
        h.append('{'+','.join(str(v)+'u' for v in values)+',{'+ranges[0]+'},{'+ranges[1]+'},{'+levels+'}},')
    h.append('};')
    for key, value in tables['engineering']['hashes'].items():
        if isinstance(value,str):
            h.append(f'inline constexpr char kHash_{ident(key)}[]="{value}";')
    h += ['const char* PipelineSource();','const char* PipelineSha();\nbool PipelineTestHooks();','}']
    (out/'contract_generated.h').write_text('\n'.join(h)+'\n',encoding='utf8')
    (out/'layout_generated.glsl').write_text('\n'.join(g)+'\n',encoding='utf8')
    coefficients = []
    for name, qs in m['coefficientRationals'].items():
        if [binary128(q) for q in qs] != m['coefficientBits'][name]:
            raise ValueError('coefficient bytes differ from contract: '+name)
        coefficients.append(f'const uvec4 SOFT_{name.upper()}[{len(qs)}]=uvec4[]('+','.join('uvec4('+','.join(str(v)+'u' for v in binary128(q))+')' for q in qs)+');')
    for name in ('piBinary128','ln2Binary128'):
        coefficients.append('const uvec4 SOFT_'+name+'=uvec4('+','.join(str(v)+'u' for v in m[name])+');')
    coefficients.append('const uint SOFT_twoOverPi[16]=uint[]('+','.join(str(v)+'u' for v in m['twoOverPiScaled512'])+');')
    (out/'math_constants.glsl').write_text('\n'.join(coefficients)+'\n',encoding='utf8')
    scenes=[]
    graphics=[]
    for filename,symbol in (('scene.vert','kG13Vertex'),('scene.frag','kG13Fragment')):
        p=MODULE/filename
        if p.exists():
            b=p.read_bytes().replace(b'\r\n',b'\n');deps[filename]=sha(b)
            if len(b) > LIMIT:
                raise ValueError(f'G13 graphics stage {filename} exceeds {LIMIT} bytes')
            graphics.append({'entry':filename,'bytes':len(b),'sha256':sha(b)})
            scenes.append('inline constexpr const char '+symbol+'[]=R"G13SCENE('+b.decode()+')G13SCENE";')
    if scenes:
        (out/'scene_sources.h').write_text('#pragma once\n'+'\n'.join(scenes)+'\n',encoding='utf8')
    template = MODULE/'pipeline.comp'
    if template.exists():
        source = template.read_text(encoding='utf8').replace('\r\n','\n')
        def expand(match):
            name = match.group(1)
            p = out/name if (out/name).exists() else MODULE/name
            b = p.read_bytes().replace(b'\r\n',b'\n')
            deps[str(p.name)] = sha(b)
            return b.decode('utf8')
        source = re.sub(r'/\* INCLUDE ([\w.]+) \*/',expand,source)
        if '/* INCLUDE' in source or len(source.encode()) > 4*1024*1024:
            raise ValueError('shader assembly')
        if hooks:
            source=source.replace("#version 310 es", "#version 310 es\n#define G13_TEST_HOOKS 1",1)
        b=source.encode()
        (out/'pipeline_assembled.comp').write_bytes(b)
        deps['pipeline.comp']=sha(template.read_bytes().replace(b'\r\n',b'\n'))
        deps['assembledSha']=sha(b)
    role_manifest = write_pipeline_roles(out, source, d, hooks, split)
    (out/'assets_manifest.json').write_text(json.dumps({'schema':'G13_BUILD_ASSETS_V1','dependencies':deps,'testHooks':hooks,'gpuQualification':'NOT_MEASURED','graphics':graphics,**role_manifest},indent=2)+'\n',encoding='utf8')
    if out.resolve()==MODULE.resolve():
        c=[0]*128
        c[:12]=[l['magic']['config'],1,l['magic']['profile'],128,64,64,1,0]+[struct.unpack('<I',struct.pack('<f',v))[0] for v in (1,1,1/64,1)]
        for at,v in ((12,1.),(14,1000.),(16,15.625)):
            c[at:at+2]=struct.unpack('<II',struct.pack('<d',v))
        c[18]=struct.unpack('<I',struct.pack('<f',1/64))[0];c[19]=1;c[22]=struct.unpack('<I',struct.pack('<f',2.))[0];c[28]=1024;c[29]=struct.unpack('<I',struct.pack('<f',1.))[0];c[30]=1;c[31]=1
        for at,key in ((36,'algorithmSHA'),(44,'layoutSHA'),(52,'gatesSHA')):
            c[at:at+8]=struct.unpack('<8I',bytes.fromhex(tables['engineering']['hashes'][key]))
        seed='{"mode":"REST"}'
        c[60:68]=struct.unpack('<8I',hashlib.sha256(seed.encode()).digest())
        c[76:84]=struct.unpack('<8I',hashlib.sha256(struct.pack('<128I',*c)).digest())
        p=ROOT/'entry/src/main/ets/physics/G13ResearchConfig.ets'
        p.write_text('// Build-generated metadata only; research remains OFF.\nexport class G13ResearchConfig {\n  static words(): Uint32Array { return new Uint32Array('+json.dumps(c)+'); }\n  static seed(): string { return '+json.dumps(seed)+'; }\n}\n',encoding='utf8')

def write_pipeline_roles(out, source, dispatch, hooks, split):
    variant = 'DEPENDENCY_SPLIT' if split else 'SINGLE_PROGRAM'
    parent = source.encode('utf8')
    programs = (list(roles(source, dispatch, hooks)) if split
                else [('FULL', tuple(range(67)), parent, None)])
    if len(programs) > 16:
        raise ValueError('G13 program role table capacity')
    entries = []
    mapping = [-1]*67
    header = ['#pragma once', 'namespace grs::g13 {',
              f'inline constexpr bool kG13TestHooks={str(hooks).lower()};']
    descriptors = []
    for index, (name, opcodes, shader, dependencies) in enumerate(programs):
        filename = f'role_{name}.comp' if split else 'pipeline_assembled.comp'
        if split:
            (out/filename).write_bytes(shader)
        for op in opcodes:
            if mapping[op] != -1:
                raise ValueError(f'G13 duplicate dispatch opcode {op}')
            mapping[op] = index
        identity = sha(shader)
        required = ['uOp','uScope','uIndex']
        if name in ('BOOT','FULL'):
            required.append('uL')
        entries.append({'role':name,'entry':filename,'bytes':len(shader),
                        'sha256':identity,'opcodes':list(opcodes),
                        'qualificationOpcode':66,'dependencies':dependencies,
                        'requiredUniforms':required,
                        'optionalUniforms':sorted(set(('uL','uJ','uMicro','uBlock','uTestHook'))-set(required)),
                        'ssboBindings':{'Base':0,'Candidate':1,'Work':2,'Control':3}})
        symbol = f'kRoleSource{index}'
        header.append(f'inline constexpr char {symbol}[]=R"G13GPU('+shader.decode('utf8')+')G13GPU";')
        descriptors.append(f'{{"{name}",{symbol},"{identity}"}}')
    if -1 in mapping:
        raise ValueError('G13 incomplete opcode mapping')
    manifest = {'variant':variant,'parentShaderSHA':sha(parent),'opcodeToRole':mapping,'roles':entries,
                'splitterSHA':sha(Path(__file__).with_name('split_shader.py').read_bytes().replace(b'\r\n',b'\n'))}
    bundle = sha(json.dumps(manifest,sort_keys=True,separators=(',',':')).encode('utf8')) if split else sha(parent)
    header += ['inline constexpr PipelineRole kG13Roles[]={'+','.join(descriptors)+'};',
               'inline constexpr uint8_t kG13OpcodeRole[]={'+','.join(map(str,mapping))+'};',
               f'inline constexpr char kG13PipelineSha[]="{bundle}";',
               f'inline constexpr char kG13Variant[]="{variant}";','}']
    (out/'pipeline_source.h').write_bytes(('\n'.join(header)+'\n').encode('utf8'))
    return {**manifest,'pipelineSHA':bundle}

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--out',type=Path,default=MODULE)
    parser.add_argument('--test-hooks',action='store_true')
    parser.add_argument('--split',action='store_true')
    args=parser.parse_args();generate(args.out,args.test_hooks,args.split)
