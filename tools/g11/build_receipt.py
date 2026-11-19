"""Inspect a compiled unsigned G11 product and statically compile its GLSL. No execution."""
from pathlib import Path
import hashlib, json, re, shutil, struct, subprocess, sys, zipfile

ROOT = Path(__file__).resolve().parents[2]
ABI_NAMES = ['revision', 'seedReceipt', 'bootstrapHelper', 'phaseWitnessHelper',
             'visualTransport', 'controller', 'coldManager', 'scene', 'r1Control',
             'dmControl', 'programFacts', 'activeProgramIdentity', 'expectedContract',
             'nativeObservation', 'optionalActive', 'optionalExpected',
             'acceptedCompositeProof', 'blockedCompositeProof', 'compositePermit', 'scope']

def sha(data):
    return hashlib.sha256(data).hexdigest()

def elf(data):
    h = struct.unpack_from('<16sHHIQQQIHHHHHH', data)
    assert h[0][:6] == b'\x7fELF\x02\x01' and h[2] == 183
    sections = [struct.unpack_from('<IIQQQQIIQQ', data, h[6]+i*h[11]) for i in range(h[12])]
    exports, abi, owner_abi, build_id = set(), None, None, None
    for s in sections:
        if s[1] not in (2, 11):
            continue
        strings = sections[s[6]]
        names = data[strings[4]:strings[4]+strings[5]]
        for at in range(s[4], s[4]+s[5], s[9]):
            n, info, other, index, value, size = struct.unpack_from('<IBBHQQ', data, at)
            name = names[n:names.index(0, n)].decode('utf8')
            if index and s[1] == 11:
                exports.add(name)
            if name in ('G11NativeAbiV2', 'G11OwnerAbiV2') and index:
                q = sections[index]
                values = list(struct.unpack_from('<%dQ' % (size//8), data, q[4]+value-q[3]))
                if name == 'G11NativeAbiV2':
                    abi = values
                else:
                    owner_abi = dict(zip(['revision', 'ownerApp', 'command', 'commandQueue32',
                                          'replySlots32', 'threadObject', 'loadedIdentityTransient'], values))
    for i in range(h[10]):
        p = struct.unpack_from('<IIQQQQQQ', data, h[5]+i*h[9])
        if p[0] != 4:
            continue
        at, end = p[2], p[2]+p[5]
        while at < end:
            ns, ds, kind = struct.unpack_from('<III', data, at)
            at += 12
            name = data[at:at+ns]
            at += (ns+3)&~3
            note = data[at:at+ds]
            at += (ds+3)&~3
            if name == b'GNU\0' and kind == 3:
                build_id = note.hex()
    assert build_id
    return {'bytes': len(data), 'sha256': sha(data), 'BuildId': build_id,
            'exports': sorted(exports), 'abi': dict(zip(ABI_NAMES, abi)) if abi else None,
            'ownerABI': owner_abi}

def main():
    build, output, validator = [Path(x).resolve() for x in sys.argv[1:4]]
    output.mkdir(parents=True, exist_ok=True)
    native = Path('entry/src/main/cpp/gpu_resident/g11vis')
    files = list((ROOT/native).glob('*'))
    files += list((ROOT/'entry/src/main/cpp/types/libgrs_g11').glob('*'))
    files += [ROOT/p for p in ['test/t566_generate_composite.cjs',
              'entry/src/main/ets/entryability/G11ResearchAbility.ets',
              'entry/src/main/ets/pages/GpuResidentG11.ets',
              'entry/src/main/ets/physics/GpuG11Port.ets',
              'entry/src/main/ets/physics/GpuG11Cold.ets',
              'entry/src/main/ets/physics/G11ColdArchiveCore.ets',
              'entry/src/main/ets/workers/G11ColdWorker.ets']]
    sources = {}
    for file in files:
        if not file.is_file():
            continue
        rel = file.relative_to(ROOT).as_posix()
        data = file.read_bytes()
        assert (build/rel).read_bytes() == data, rel
        sources[rel] = {'rawCompiledSHA256': sha(data), 'normalizedLFSHA256': sha(data.replace(b'\r\n', b'\n'))}
    assets = next(build.glob('entry/.cxx/**/g11_assets/g11vis_inputs.json')).parent
    generated = json.loads((assets/'g11vis_inputs.json').read_text(encoding='utf8'))
    for rel, digest in generated.items():
        assert sha((build/rel).read_bytes()) == digest, rel
    shaders = {}
    shader_inputs = [assets/'g11vis_r1.comp', assets/'g11vis_dm.comp']
    for file in ['scene.cpp', 'scene_shaders.h']:
        text = (ROOT/native/file).read_text(encoding='utf8')
        for name, delimiter, source in re.findall(r'(?:const char\*|const char\s*\*)\s*(\w+)\s*=\s*R"(\w+)\(([\s\S]*?)\)\2"', text):
            stage = 'vert' if 'Vertex' in name else 'frag'
            path = output/(name+'.'+stage)
            path.write_text(source, encoding='utf8', newline='\n')
            shader_inputs.append(path)
    assert len(shader_inputs) == 8
    log = []
    for file in shader_inputs:
        result = subprocess.run([str(validator), '-S', file.suffix[1:], str(file)], capture_output=True)
        log.append(file.name.encode()+b' exit='+str(result.returncode).encode()+b'\n'+result.stdout+result.stderr)
        assert result.returncode == 0, log[-1]
        shaders[file.name] = sha(file.read_bytes())
    (output/'glsl-static-compile.log').write_bytes(b''.join(log))
    hap = build/'entry/build/default/outputs/default/entry-default-unsigned.hap'
    with zipfile.ZipFile(hap) as package:
        binary = package.read('libs/arm64-v8a/libgrs_g11.so')
        module = elf(binary)
        assert module['abi'] and module['abi']['revision'] == 2 and module['abi']['seedReceipt'] == 512
        assert {'RegisterG11', 'G11NativeIdentityAnchor', 'G11NativeAbiV2', 'G11OwnerAbiV2'} <= set(module['exports'])
        assert module['ownerABI'] and module['ownerABI']['revision'] == 2
        assert any('g11vis10Controller4Pump' in name for name in module['exports'])
        for file in shader_inputs[:2]:
            assert file.read_bytes() in binary, 'packaged shader differs: '+file.name
        broker = elf(package.read('libs/arm64-v8a/libgrs_cold_slots.so'))
        abc = package.read('ets/modules.abc')
    record = {'schema': 'T639_G11_PRODUCT_COMPILE_V2', 'status': 'DEVELOPMENT_COMPILE_COMPLETE',
              'testsRun': False, 'productExecuted': False, 'deviceUsed': False, 'signed': False,
              'deployed': False, 'productPASS': False, 'buildDirectory': str(build),
              'hap': {'path': str(hap), 'bytes': hap.stat().st_size, 'sha256': sha(hap.read_bytes())},
              'packagedNative': module, 'processBroker': broker,
              'abc': {'bytes': len(abc), 'sha256': sha(abc)}, 'compiledSources': sources,
              'staticShaders': shaders, 'generatorInputs': generated,
              'normalRoot': {'source': 19, 'r1Quantum': 2543, 'dmQuantum': 2923,
                             'tailReadBytes': 1024, 'fences': 1, 'computeBindings': 4,
                             'deadlineNs': 250000000, 'gapNs': 33333333, 'quotaHz': 120, 'displayFps': 30},
              'remainingEvidence': 'Independent tester/effector owns execution, physical/visual/replay/negative controls, limits and device qualification; no runtime result is inferred from compilation.'}
    for name in ['t639-delivery-v2-build.log', 't639-final-build.log']:
        data = (ROOT/'.codex/local-test'/name).read_bytes()
        assert b'BUILD SUCCESSFUL' in data
        (output/name).write_bytes(data)
        record[name] = {'sha256': sha(data), 'elapsed': re.search(rb'BUILD SUCCESSFUL in ([^\r\n]+)', data).group(1).decode()}
    shutil.copyfile(assets/'g11vis_inputs.json', output/'generated-inputs.json')
    (output/'compile-receipt-v2.json').write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf8', newline='\n')
    print(json.dumps({k: record[k] for k in ['hap', 'abc', 'staticShaders']}, indent=2))
    print('ABI', module['abi'], 'ownerABI', module['ownerABI'], 'BuildId', module['BuildId'], 'nativeSHA', module['sha256'])

if __name__ == '__main__':
    main()
