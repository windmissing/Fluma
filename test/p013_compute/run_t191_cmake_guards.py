"""Offline negative configure tests; separate from the actual ARM build proof."""
from pathlib import Path
import json, subprocess, tempfile

root = Path(__file__).resolve().parents[2]
scratch = Path(tempfile.mkdtemp(prefix='t191-cmake-negative-', dir=root/'.codex/local-test'))
module = (root/'test/p013_compute/strict_variant_v1.cmake').as_posix()
(scratch/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.18)\nproject(T191Guard NONE)\nadd_library(p011_gpu_pair_napi INTERFACE)\nset(T191_BOUND_VARIANT STRICT_O2)\ninclude("'+module+'")\n', encoding='utf-8')
bin_dir = Path('C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/build-tools/cmake/bin')
cases = [('identity', ['-DP011_GPU_STRICT_VARIANT=STRICT_O0'], 'T191_VARIANT_IDENTITY_CONFLICT'),
         ('release', ['-DCMAKE_BUILD_TYPE=Release'], 'T191_REQUIRES_UNCHANGED_DEBUG_CONFIGURATION'),
         ('lto', ['-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON'], 'T191_LTO_FORBIDDEN'),
         ('ndebug', ['-DCMAKE_CXX_FLAGS=-DNDEBUG'], 'T191_FORBIDDEN_FLAGS'),
         ('fastmath', ['-DCMAKE_CXX_FLAGS=-ffast-math'], 'T191_FORBIDDEN_FLAGS'),
         ('contraction', ['-DCMAKE_CXX_FLAGS=-ffp-contract=fast'], 'T191_FORBIDDEN_FLAGS')]
rows = []
for name, args, expected in cases:
    command = [str(bin_dir/'cmake.exe'), '-G', 'Ninja', '-S', str(scratch), '-B', str(scratch/name),
               '-DCMAKE_MAKE_PROGRAM='+str(bin_dir/'ninja.exe'), '-DCMAKE_BUILD_TYPE=Debug']+args
    result = subprocess.run(command, capture_output=True, text=True)
    output = result.stdout+result.stderr
    rows.append({'case': name, 'exitCode': result.returncode, 'expected': expected, 'output': output,
                 'status': 'PASS' if result.returncode != 0 and expected in output else 'FAIL'})
report = {'scope': 'REAL_CMAKE_CONFIGURE_REJECTION_ONLY_NO_COMPILER_IN_NEGATIVE_HARNESS',
          'passed': sum(r['status'] == 'PASS' for r in rows), 'failed': sum(r['status'] == 'FAIL' for r in rows), 'cases': rows}
print(json.dumps(report, indent=2))
raise SystemExit(1 if report['failed'] else 0)
