from pathlib import Path
import sys,shutil,json
sys.path.insert(0,str(Path(__file__).parents[1]/'offline_handoff'))
from clone import clone,ROOT
out=clone(ROOT/'.codex/local-test/t640-delivery',sys.argv[1]);main=out/'entry/src/main';gpu=main/'cpp/gpu_resident'
shutil.copytree(ROOT/'entry/src/main/cpp/gpu_resident/resource_lab',gpu/'resource_lab',dirs_exist_ok=True)
s=(ROOT/'entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h').read_text(encoding='utf8');s='#include "../resource_lab/bounded_stream.h"\n'+s.replace('std::ostringstream o;','resource538::Stream o;');(gpu/'program_cache/artifact_store.h').write_text(s,encoding='utf8')
shutil.copyfile(ROOT/'entry/src/main/cpp/gpu_resident/cold_slot_broker.cpp',gpu/'cold_slot_broker.cpp')
shutil.copyfile(ROOT/'test/t538/lab_bridge.cpp',main/'cpp/t538_lab_bridge.cpp');shutil.copyfile(ROOT/'test/t538/GpuResourceLab.ets',main/'ets/pages/GpuResourceLab.ets');shutil.copyfile(ROOT/'test/t668/T668LabAbility.ets',main/'ets/entryability/T668LabAbility.ets')
(main/'cpp/CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.16)
project(t668_resource_lab LANGUAGES CXX)
add_library(grs_resource_lab SHARED t538_lab_bridge.cpp gpu_resident/cold_slot_broker.cpp)
target_include_directories(grs_resource_lab PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/gpu_resident")
target_compile_features(grs_resource_lab PRIVATE cxx_std_17)
target_compile_definitions(grs_resource_lab PRIVATE GRS_PROGRAM_CACHE_TEST_FAULTS T531_RESOURCE_LAB_V1)
target_compile_options(grs_resource_lab PRIVATE -fno-fast-math -ffp-contract=off)
target_link_libraries(grs_resource_lab PRIVATE libace_napi.z.so)
''')
p=main/'module.json5';d=json.loads(p.read_text());d['module']['abilities'][0]['srcEntry']='./ets/entryability/T668LabAbility.ets';p.write_text(json.dumps(d,indent=2))
print(out)
