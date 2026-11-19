// T550 (problem T546 §5): shared generator-side injection for the strict
// typedarray decode contract. Applied by t522/t529/t517 bridge overlays so
// every generated delivery tree is same-source with canonical:
//  - copies napi_words_contract.{h,cpp} into the delivery gpu_resident,
//  - appends the grs_words_contract static library to the delivery CMakeLists,
//  - injects the include + gpuInputDiagnosticsV1 endpoint + registration into
//    the delivery app_bridge.cpp (which is the frozen 0228a7d bridge after
//    t434/frozen.cjs — the overlay chain applies on top of the freeze),
//  - appends the gpuInputDiagnosticsV1 d.ts declaration.
// The injection is idempotent (anchors verified unique; skips when already present).
const fs = require('node:fs'), path = require('node:path');
module.exports = function injectWordsContract(main, projectRoot) {
  const root = projectRoot || path.resolve(__dirname, '../..');
  const base = path.join(main, 'cpp/gpu_resident');
  // 1. same-source copies (canonical -> delivery).
  for (const name of ['napi_words_contract.h', 'napi_words_contract.cpp', 'napi_provider_profile.h', 'napi_provider_profile.cmake'])
    fs.copyFileSync(path.join(root, 'entry/src/main/cpp/gpu_resident', name), path.join(base, name));
  // 2. CMake: fold the contract TU into the delivery tree's grs_foundation
  //    (byte-identical link lines vs pre-T550 — the OHOS toolchain duplicates
  //    linker flags per library entry and the link command sits at the cmd.exe
  //    8191-char boundary, so no new link library entry is allowed).
  const cmake = path.join(base, 'CMakeLists.txt');
  let cmakeText = fs.readFileSync(cmake, 'utf8');
  if (!cmakeText.includes('napi_words_contract.cpp')) {
    const anchor = 'add_library(grs_foundation STATIC ';
    if (cmakeText.split(anchor).length !== 2) throw Error('T550_CMAKE_ANCHOR');
    cmakeText = cmakeText.replace(anchor, anchor + 'napi_words_contract.cpp ');
    fs.writeFileSync(cmake, cmakeText);
  }
  if (!cmakeText.includes('grs_bind_napi_provider_sdk(grs_foundation)')) { fs.appendFileSync(cmake, '\ninclude("${CMAKE_CURRENT_SOURCE_DIR}/napi_provider_profile.cmake")\ngrs_bind_napi_provider_sdk(grs_foundation)\n'); }
  // 3. bridge injection (post-freeze delivery app_bridge.cpp).
  const bridgePath = path.join(base, 'app_bridge.cpp');
  let s = fs.readFileSync(bridgePath, 'utf8').replaceAll('\r\n', '\n');
  if (!s.includes('napi_words_contract.h')) {
    const one = (a, b) => { if (s.split(a).length !== 2) throw Error('T550_BRIDGE_ANCHOR ' + a); s = s.replace(a, b); };
    one('#include "bridge_packet.h"', '#include "napi_words_contract.h"\n#include "bridge_packet.h"');
    one('napi_value init(napi_env e,napi_value out){napi_property_descriptor d[]={',
      'napi_value gpuInputDiagnosticsV1(napi_env e,napi_callback_info info){\n' +
      ' size_t argc=2;napi_value argv[2]={nullptr,nullptr};\n' +
      ' if(napi_get_cb_info(e,info,&argc,argv,nullptr,nullptr)!=napi_ok||argc<1||argc>2)\n' +
      '  return text(e,"{\\"revision\\":\\"GPU_INPUT_DIAGNOSTIC_V1\\",\\"error\\":\\"ARG_COUNT\\"}");\n' +
      ' char route[16]={0};size_t routeLen=0;\n' +
      ' if(napi_get_value_string_utf8(e,argv[0],route,sizeof(route),&routeLen)!=napi_ok)\n' +
      '  return text(e,"{\\"revision\\":\\"GPU_INPUT_DIAGNOSTIC_V1\\",\\"error\\":\\"ROUTE_NOT_STRING\\"}");\n' +
      ' std::string attempt;\n' +
      ' if(argc==2){char buf[24]={0};size_t len=0;\n' +
      '  if(napi_get_value_string_utf8(e,argv[1],buf,sizeof(buf),&len)==napi_ok)attempt.assign(buf,len);}\n' +
      ' std::string out;grs::words::SnapshotJson(route,attempt,out);\n' +
      ' return text(e,out);\n' +
      '}\n' +
      'napi_value init(napi_env e,napi_value out){napi_property_descriptor d[]={');
    one('{"timeline",nullptr,timeline',
      '{"gpuInputDiagnosticsV1",nullptr,gpuInputDiagnosticsV1,nullptr,nullptr,nullptr,napi_default,nullptr},{"timeline",nullptr,timeline');
    fs.writeFileSync(bridgePath, s);
  }
  // 4. d.ts declaration (idempotent append).
  const types = path.join(main, 'cpp/types/libgrsm1/index.d.ts');
  let dts = fs.readFileSync(types, 'utf8');
  if (!dts.includes('gpuInputDiagnosticsV1'))
    fs.appendFileSync(types, '\n// T550 (problem T546 §3): cold input diagnostics endpoint.\nexport const gpuInputDiagnosticsV1:(route:string,attemptId?:string)=>string;\n');
};
