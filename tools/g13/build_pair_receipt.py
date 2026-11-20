"""Bind the two compiled G13 products to their common inputs and exact ELF GLSL."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from zipfile import ZipFile

from split_shader import LIMIT


def digest(data):
    return hashlib.sha256(data).hexdigest()


def write_json(path, item):
    path.write_text(json.dumps(item, indent=2) + '\n', encoding='utf8', newline='\n')


def build_id(tool, binary):
    text = subprocess.check_output([str(tool), '-n', str(binary)], text=True)
    return re.search(r'Build ID: ([a-f0-9]+)', text)[1]


def read_build(project, arm, validator, readelf, out):
    cache = project / 'entry/.cxx/default/default/debug/arm64-v8a'
    assets = cache / 'gpu_resident/g13flow/g13_assets'
    manifest = json.loads((assets / 'assets_manifest.json').read_bytes())
    assert manifest['variant'] == ('SINGLE_PROGRAM' if arm == 'MONOLITH' else 'DEPENDENCY_SPLIT')
    assert not manifest['testHooks']
    hap = project / 'entry/build/default/outputs/default/entry-default-unsigned.hap'
    lib = project / 'entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libgrs_g13.so'
    log = project.with_suffix('.log')
    assert 'BUILD SUCCESSFUL' in log.read_text(encoding='utf-8-sig', errors='replace')
    linked = lib.read_bytes()
    with ZipFile(hap) as pack:
        packaged = pack.read('libs/arm64-v8a/libgrs_g13.so')
        abc = pack.read('ets/modules.abc')
    packed_elf = out / (arm + '.so')
    packed_elf.write_bytes(packaged)
    identity = build_id(readelf, packed_elf)
    assert identity == build_id(readelf, lib)
    entries = {}
    for role in manifest['roles'] + manifest['graphics']:
        compute = 'opcodes' in role
        name = role['role'] if compute else role['entry']
        path = (assets if compute else project / 'entry/src/main/cpp/gpu_resident/g13flow') / role['entry']
        disk = path.read_bytes()
        source = disk.replace(b'\r\n', b'\n')
        assert len(source) == role['bytes'] and digest(source) == role['sha256']
        assert source + b'\0' in linked and source + b'\0' in packaged
        reworked = compute and arm == 'SPLIT'
        if reworked or not compute:
            assert len(source) <= LIMIT
        stage = 'comp' if compute else path.suffix[1:]
        command = [str(validator), '-S', stage, '-l', '-q', str(path)]
        reflected = subprocess.check_output(command, text=True)
        uniforms = reflected.split('Uniform reflection:\n')[1].split('\nUniform block')[0]
        for required in role.get('requiredUniforms', []):
            assert re.search(r'^' + required + r':', uniforms, re.M), (name, required)
        entries[name] = {
            'path': path.relative_to(project).as_posix(),
            'kind': 'compute' if compute else 'graphics',
            'sha256': role['sha256'], 'bytes': role['bytes'],
            'opcodes': role.get('opcodes', []), 'reworked': reworked,
            'dependencies': role.get('dependencies'),
            'ssboBindings': role.get('ssboBindings'),
            'requiredUniforms': role.get('requiredUniforms', []),
            'optionalUniforms': role.get('optionalUniforms', []),
            'staticUniforms': [line.split(':',1)[0] for line in uniforms.splitlines() if ':' in line],
        }
        if source != disk:
            entries[name].update(sourceNewlines='CPP_TRANSLATION_LF', sourceSHA256=digest(disk))
    parent_shader = (assets / 'pipeline_assembled.comp').read_bytes()
    if arm == 'SPLIT':
        assert parent_shader + b'\0' not in packaged
    return manifest, {
        'variant': arm, 'project': project.as_posix(),
        'unsigned': hap.relative_to(project).as_posix(),
        'unsignedSHA256': digest(hap.read_bytes()), 'nativeSHA256': digest(packaged),
        'linkedSHA256': digest(linked), 'buildId': identity, 'roles': entries,
        'pipelineSHA': manifest['pipelineSHA'], 'opcodeToRole': manifest['opcodeToRole'],
        'hvigorLogSHA256': digest(log.read_bytes()), 'abcSHA256': digest(abc),
        'compileCommandsSHA256': digest((cache / 'compile_commands.json').read_bytes()),
        'staticCompilerSHA256': digest(validator.read_bytes()),
        'deviceUsed': False, 'signed': False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('monolith', type=Path)
    parser.add_argument('split', type=Path)
    parser.add_argument('out', type=Path)
    parser.add_argument('--parent', required=True)
    parser.add_argument('--readelf', type=Path, required=True)
    parser.add_argument('--validator', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    evidence = args.out.resolve()
    tmp = args.monolith.parent / 't728-receipt-artifacts'
    tmp.mkdir(exist_ok=True)
    ma, mono = read_build(args.monolith.resolve(), 'MONOLITH', args.validator.resolve(),
                          args.readelf.resolve(), tmp)
    mb, split = read_build(args.split.resolve(), 'SPLIT', args.validator.resolve(),
                           args.readelf.resolve(), tmp)
    assert ma['dependencies'] == mb['dependencies']
    assert ma['graphics'] == mb['graphics']
    assert ma['parentShaderSHA'] == mb['parentShaderSHA']
    assert ma['splitterSHA'] == mb['splitterSHA']
    assert mono['buildId'] != split['buildId'] and mono['unsignedSHA256'] != split['unsignedSHA256']
    shared = {}
    for relative in ('entry/src/main/cpp/gpu_resident/g13flow',
                     'entry/src/main/ets/pages/GpuResidentG13.ets',
                     'entry/src/main/cpp/gpu_resident/program_cache',
                     'tools/g13'):
        left = args.monolith / relative
        files = sorted(left.rglob('*')) if left.is_dir() else [left]
        for item in files:
            if not item.is_file() or '__pycache__' in item.parts:
                continue
            key = item.relative_to(args.monolith)
            twin = args.split / key
            assert item.read_bytes() == twin.read_bytes(), key
            shared[key.as_posix()] = digest(item.read_bytes())
    parent = evidence / 'parent.json'
    write_json(parent, {'schema': 'T728_COMMON_PARENT_V1', 'revision': args.parent,
                        'shaderSHA256': ma['parentShaderSHA'], 'commonInputs': shared})
    for arm in (mono, split):
        arm['parentReceipt'] = parent.as_posix()
        arm['parentSHA256'] = digest(parent.read_bytes())
        write_json(evidence / (arm['variant'] + '.json'), arm)


if __name__ == '__main__':
    main()
