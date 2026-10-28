"""Read-only artifact provenance for a generated T212 strict variant.

Run after assembleHap (unsigned is sufficient). Captures real Ninja commands,
compiler macro dumps and ELF/ZIP bytes; never substitutes an object SO digest
for a packaged SO digest. No device or signing operation.
"""
import argparse, ctypes, hashlib, json, os, re, shlex, struct, subprocess, zipfile
from pathlib import Path


def digest(data):
    return hashlib.sha256(data).hexdigest()


def file_info(p):
    b = p.read_bytes()
    return {"path": str(p), "bytes": len(b), "sha256": digest(b)}


def command_args(command):
    if os.name != 'nt':
        return shlex.split(command)
    count = ctypes.c_int()
    split = ctypes.windll.shell32.CommandLineToArgvW
    split.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
    split.restype = ctypes.POINTER(ctypes.c_wchar_p)
    value = split(command, ctypes.byref(count))
    if not value:
        raise RuntimeError('COMMAND_SPLIT_FAILED')
    try:
        return [value[i] for i in range(count.value)]
    finally:
        ctypes.windll.kernel32.LocalFree(ctypes.cast(value, ctypes.c_void_p))


def elf_info(data):
    if data[:6] != b'\x7fELF\x02\x01':
        raise RuntimeError('NOT_ELF64_LE')
    machine = struct.unpack_from('<H', data, 18)[0]
    if machine != 183:
        raise RuntimeError('NOT_AARCH64')
    off = struct.unpack_from('<Q', data, 40)[0]
    size, count, names_idx = struct.unpack_from('<HHH', data, 58)
    headers = [struct.unpack_from('<IIQQQQIIQQ', data, off+i*size) for i in range(count)]
    names_header = headers[names_idx]
    names = data[names_header[4]:names_header[4]+names_header[5]]
    selected = {}
    build_id = None
    for h in headers:
        name = names[h[0]:].split(b'\0', 1)[0].decode('ascii')
        if name in ['.text', '.rodata', '.data', '.dynsym', '.note.gnu.build-id']:
            b = data[h[4]:h[4]+h[5]]
            selected[name] = {'bytes': len(b), 'sha256': digest(b)}
            if name == '.note.gnu.build-id':
                n, d, kind = struct.unpack_from('<III', b)
                if kind != 3 or b[12:12+n].rstrip(b'\0') != b'GNU':
                    raise RuntimeError('BUILD_ID_NOTE_INVALID')
                start = 12+((n+3)//4)*4
                build_id = b[start:start+d].hex()
    if not build_id:
        raise RuntimeError('BUILD_ID_MISSING')
    return {'machine': 'AARCH64', 'buildId': build_id, 'sections': selected}


def archive(project, destination):
    project, destination = project.resolve(), destination.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    source_path = project/'t212-source-manifest.json'
    source = json.loads(source_path.read_text(encoding='utf-8'))
    variant = source['variant']
    expected = '-O2' if variant == 'STRICT_O2' else '-O0'
    commands_files = list((project/'entry/.cxx').glob('**/compile_commands.json'))
    if len(commands_files) != 1:
        raise RuntimeError('AMBIGUOUS_COMPILE_COMMANDS')
    commands_path = commands_files[0]
    build = commands_path.parent
    commands = json.loads(commands_path.read_text(encoding='utf-8'))
    cache = (build/'CMakeCache.txt').read_text(encoding='utf-8')
    if not re.search(r'^CMAKE_BUILD_TYPE:STRING=Debug$', cache, re.M):
        raise RuntimeError('NON_DEBUG_BUILD')
    ninja = re.search(r'^CMAKE_MAKE_PROGRAM:[^=]+=(.+)$', cache, re.M)[1]
    native_commands = subprocess.check_output([ninja, '-t', 'commands', 'p011_gpu_pair_napi'], cwd=build, text=True)
    (destination/'ninja-commands.txt').write_text(native_commands, encoding='utf-8')
    records = []
    compiler = None
    for command in commands:
        if not re.search(r'[/\\]p013_compute[/\\]pair_(binding|prepare|executor|owner)_v1.cpp$', command['file']):
            continue
        args = command_args(command['command'])
        compiler = Path(args[0])
        opts = [a for a in args if re.fullmatch(r'-O(?:[0-3szg]|fast)', a)]
        if not opts or opts[-1] != expected or '-fno-fast-math' not in args or '-ffp-contract=off' not in args:
            raise RuntimeError('EFFECTIVE_STRICT_FLAGS_INVALID')
        forbidden = ['-Ofast', '-ffast-math', '-funsafe-math-optimizations', '-fassociative-math', '-ffinite-math-only', '-fno-signed-zeros', '-freciprocal-math', '-fno-trapping-math', '-ffp-contract=fast', '-ffp-contract=on']
        if any(a in forbidden or a.startswith('-flto') or a.startswith('-DNDEBUG') for a in args):
            raise RuntimeError('FORBIDDEN_OPTIMIZATION')
        if [a for a in args if a.startswith('-ffp-contract=')][-1:] != ['-ffp-contract=off']:
            raise RuntimeError('CONTRACTION_NOT_OFF')
        obj_index = args.index('-o')
        obj = Path(command['directory'])/args[obj_index+1]
        if not obj.exists():
            raise RuntimeError('OBJECT_MISSING')
        preprocessing = args[:obj_index]+args[obj_index+2:]
        preprocessing[preprocessing.index('-c')] = '-E'
        preprocessing.append('-dM')
        macros = subprocess.check_output(preprocessing, cwd=command['directory'])
        if re.search(rb'^#define NDEBUG(?:\s|$)', macros, re.M):
            raise RuntimeError('NDEBUG_CHANGED')
        macro_path = destination/(Path(command['file']).stem+'.macros.txt')
        macro_path.write_bytes(macros)
        (destination/(Path(command['file']).name+'.o')).write_bytes(obj.read_bytes())
        records.append({'source': command['file'], 'sourceFile': file_info(Path(command['file'])), 'command': command['command'], 'arguments': args,
                        'effectiveOptimization': opts[-1], 'object': file_info(obj), 'macros': file_info(macro_path)})
    if len(records) != 4:
        raise RuntimeError('FOUR_TARGET_TRANSLATION_UNITS_REQUIRED')
    so = project/'entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libp011_gpu_pair_prototype.so'
    hap = project/'entry/build/default/outputs/default/entry-default-unsigned.hap'
    # All required artifacts must exist; no "pass" with placeholders.
    obj_so = file_info(so)
    unsigned = file_info(hap)
    obj_elf = elf_info(so.read_bytes())
    with zipfile.ZipFile(hap) as z:
        so_entries = [n for n in z.namelist() if n.endswith('/libp011_gpu_pair_prototype.so')]
        if len(so_entries) != 1:
            raise RuntimeError('PACKAGED_SO_AMBIGUOUS')
        packaged = z.read(so_entries[0])
        packaged_elf = elf_info(packaged)
        raw = {n: {'bytes': len(z.read(n)), 'sha256': digest(z.read(n))} for n in z.namelist()
               if '/rawfile/t141/glsl/' in n or '/rawfile/t171/' in n}
    if obj_elf != packaged_elf:
        raise RuntimeError('OBJECT_PACKAGE_CODE_OR_BUILD_ID_MISMATCH')
    (destination/'packaged-libp011_gpu_pair_prototype.so').write_bytes(packaged)
    (destination/'object-libp011_gpu_pair_prototype.so').write_bytes(so.read_bytes())
    (destination/'entry-default-unsigned.hap').write_bytes(hap.read_bytes())
    signed = list(hap.parent.glob('*signed.hap'))
    signed = [p for p in signed if not p.name.endswith('unsigned.hap')]
    version = subprocess.check_output([str(compiler), '--version'], text=True)
    toolchain_file = Path(re.search(r'^CMAKE_TOOLCHAIN_FILE:[^=]+=(.+)$', cache, re.M)[1])
    cmake_exe = Path(ninja).with_name('cmake.exe' if os.name == 'nt' else 'cmake')
    toolchain = {'ohosToolchain': file_info(toolchain_file), 'cmakeCache': file_info(build/'CMakeCache.txt'),
                 'ninja': {'path': ninja, 'version': subprocess.check_output([ninja, '--version'], text=True).strip()},
                 'cmake': {'path': str(cmake_exe), 'version': subprocess.check_output([str(cmake_exe), '--version'], text=True).strip()}}
    generated_sources = {}
    expected_inputs = source['v3SourceManifest']['v2SourceManifest']['inputs']
    for p in sorted((project/'entry/src/main/cpp/p013_compute').glob('pair_*')):
        if p.suffix not in ['.h', '.cpp']:
            continue
        row = file_info(p)
        if row['sha256'] != expected_inputs['test/p013_compute/'+p.name]:
            raise RuntimeError('NATIVE_SOURCE_CHANGED_AFTER_GENERATION:'+p.name)
        generated_sources[p.name] = row
    manifest = {'revision': 'T212_POST_BUILD_V1', 'variant': variant, 'generatedRoot': str(project),
                'sourceManifestSha256': source['sourceManifestSha256'], 'sourceManifestFile': file_info(source_path),
                'configurationIdentity': 'TX_GPU_PAIR_EXPERIMENT_V1|variant:'+variant,
                'compiler': {'path': str(compiler), 'version': version, 'file': file_info(compiler)},
                'toolchain': toolchain,
                'abi': 'arm64-v8a', 'buildType': 'Debug', 'compileCommands': records,
                'nativeSourceFiles': generated_sources,
                'ninjaCommands': file_info(destination/'ninja-commands.txt'), 'actualLinkCommand': native_commands.splitlines()[-1],
                'objectSo': obj_so, 'objectElf': obj_elf,
                'packagedSo': {'entry': so_entries[0], 'bytes': len(packaged), 'sha256': digest(packaged), 'elf': packaged_elf},
                'unsignedHap': unsigned, 'signedHap': [file_info(p) for p in signed] if signed else 'NOT_BUILT',
                'rawResources': raw, 'device': 'NOT_RUN', 'performanceBenefit': 'NOT_MEASURED',
                'instrumentation': source['instrumentation'], 'productionTouched': False}
    text = json.dumps(manifest, indent=2, ensure_ascii=False)+'\n'
    (destination/'post-build-manifest.json').write_text(text, encoding='utf-8')
    (project/'t212-post-build-manifest.json').write_text(text, encoding='utf-8')
    (destination/'source-manifest.json').write_bytes(source_path.read_bytes())
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('project', type=Path)
    parser.add_argument('destination', type=Path)
    options = parser.parse_args()
    result = archive(options.project, options.destination)
    print(json.dumps({'variant': result['variant'], 'unsignedSha256': result['unsignedHap']['sha256'],
                      'packagedSoSha256': result['packagedSo']['sha256'], 'buildId': result['objectElf']['buildId']}))
