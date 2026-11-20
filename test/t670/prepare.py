from pathlib import Path
import sys,shutil,json,subprocess,hashlib
sys.path.insert(0,str(Path(__file__).parents[1]/'offline_handoff'))
from clone import clone,ROOT
kind=sys.argv[1];assert kind in ('spf','dm');out=Path(sys.argv[2]).resolve()
boot_only=sys.argv[3:]==['--boot-only']
boot_source=sys.argv[3:]==['--boot-source']
if sys.argv[3:] and not ((boot_only or boot_source) and kind=='dm'):
 raise ValueError('BOOT_VARIANT_REQUIRES_DM')
if kind=='spf':
 clone(ROOT/'.codex/local-test/t564-stage-tree',out)
 target=out/'entry/src/main/cpp/t549_harness.cpp';shutil.copyfile(ROOT/'test/t670/harness.cpp',target)
 # Prove the frozen production family matches the actual T564 delivery.
 prefix='entry/src/main/cpp/gpu_resident/g9spf/'
 import io,tarfile
 raw=subprocess.check_output(['git','archive','7a7ee216',prefix],cwd=ROOT)
 with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
  for m in archive:
   if m.isfile():
    f=out/m.name;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(archive.extractfile(m).read())

else:
 subprocess.run(['node','tools/g9dm/prepare_build.cjs',str(out),*sys.argv[3:]],cwd=ROOT,check=True)
 target=out/'entry/src/main/cpp/t670_harness.cpp';shutil.copyfile(ROOT/'test/t670/harness.cpp',target)
 cm=out/'entry/src/main/cpp/CMakeLists.txt'
 with cm.open('a') as f:f.write('''\nadd_executable(g9_dm_differential t670_harness.cpp)
target_include_directories(g9_dm_differential PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(g9_dm_differential PRIVATE T670_DM)
target_compile_features(g9_dm_differential PRIVATE cxx_std_17)
target_compile_options(g9_dm_differential PRIVATE -fno-fast-math -ffp-contract=off)
target_link_libraries(g9_dm_differential PRIVATE grs_foundation libEGL.so libGLESv3.so)
''')
 if boot_only:
  shutil.copyfile(ROOT/'test/t670/dm_boot_receipt.h',target.with_name('dm_boot_receipt.h'))
  with cm.open('a') as f:f.write('''target_compile_definitions(g9_dm_differential PRIVATE GRS_G9_BOOT_ONLY=1)
target_link_libraries(g9_dm_differential PRIVATE libdl.so)
target_sources(grs_foundation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/gpu_resident/g9_dm_boot_capture.cpp")
target_compile_definitions(grs_foundation PRIVATE GRS_G9_DM_BOOT_CAPTURE=1)
''')
 if boot_source:
  for name in ['dm_boot_receipt.h','dm_source_run.h']:
   shutil.copyfile(ROOT/'test/t670'/name,target.with_name(name))
  with cm.open('a') as f:f.write('''target_compile_definitions(g9_dm_differential PRIVATE GRS_G9_BOOT_SOURCE=1)
target_link_libraries(g9_dm_differential PRIVATE libdl.so)
''')
print(out)
