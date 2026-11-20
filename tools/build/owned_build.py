"""T666 explicit baseline/toolchain ownership and target-scoped native linking.
No SDK mutation, shell invocation, product execution, or generic token deduplication.
"""
from pathlib import Path
import ctypes,hashlib,json,os,subprocess,sys,re
from build_invocation import native_call

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(p,s):
 p=Path(p);p.parent.mkdir(parents=True,exist_ok=True)
 if not p.exists() or p.read_bytes()!=s.encode('utf8'):p.write_text(s,encoding='utf8',newline='\n')
def bracket(s):
 for n in range(1,20):
  eq='='*n
  if ']'+eq+']' not in s:return '['+eq+'['+s+']'+eq+']'
 raise ValueError('CMAKE_VALUE_DELIMITER')
def configure(policy_file,build):
 policy_file=Path(policy_file).resolve();p=json.loads(policy_file.read_text(encoding='utf8'));build=Path(build).resolve();build.mkdir(parents=True,exist_ok=True)
 if p['schema']!='GRS_EXPLICIT_LINK_BASELINE_V1' or set(p['baseline'])!={'SHARED','MODULE','EXE'}:raise ValueError('EXPLICIT_BASELINE_REQUIRED')
 for v in p['baseline'].values():
  if not isinstance(v,str) or '\n' in v or '\r' in v:raise ValueError('INVALID_BASELINE')
 if re.search(r'(?:^|[\s=])(?:-ffast-math|-Ofast|-ffp-contract=(?:fast|on))(?:\s|$)',' '.join([*p['baseline'].values(),*p.get('definitions',[])])):raise ValueError('FP_POLICY_MUST_NOT_BE_RELAXED')
 sdk=Path(p['sdkToolchain']).resolve();digest=sha(policy_file);marker=build/'grs-link-policy.json'
 if (build/'CMakeCache.txt').exists() and not marker.exists():raise ValueError('UNKNOWN_CACHE_BASELINE_USE_FRESH_DIRECTORY')
 if marker.exists() and json.loads(marker.read_text(encoding='utf8'))['policySHA256']!=digest:raise ValueError('BASELINE_CHANGED_USE_FRESH_DIRECTORY')
 wrapper=build/'grs_owned_toolchain.cmake'
 text='# T666 generated from explicit immutable user baseline.\n'
 text+='if(DEFINED OHOS_SDK_NATIVE_TOOLCHAIN_DEFINED)\n'
 for kind in p['baseline']:
  text+=f' if(NOT DEFINED GRS_EXPECTED_{kind} OR NOT "${{CMAKE_{kind}_LINKER_FLAGS}}" STREQUAL "${{GRS_EXPECTED_{kind}}}")\n  message(FATAL_ERROR "GRS_CHANGED_FLAGS_AFTER_SDK")\n endif()\n'
 text+=' return()\nendif()\n'
 text+='list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES GRS_EXPECTED_SHARED GRS_EXPECTED_MODULE GRS_EXPECTED_EXE)\n'
 for kind,value in p['baseline'].items():
  name='CMAKE_'+kind+'_LINKER_FLAGS';text+=f'''set(_grs_base_{kind} {bracket(value)})
if(DEFINED {name} AND NOT "${{{name}}}" STREQUAL "${{_grs_base_{kind}}}")
 if(NOT DEFINED GRS_EXPECTED_{kind} OR NOT "${{{name}}}" STREQUAL "${{GRS_EXPECTED_{kind}}}")
  message(FATAL_ERROR "GRS_UNKNOWN_{kind}_FLAG_ORIGIN")
 endif()
endif()
unset({name})
set({name} "${{_grs_base_{kind}}}" CACHE STRING "T666 explicit user baseline" FORCE)
'''
 text+='include('+bracket(sdk.as_posix())+')\n'
 for kind in p['baseline']:
  name='CMAKE_'+kind+'_LINKER_FLAGS';text+=f'''get_property(_grs_result_{kind} CACHE {name} PROPERTY VALUE)
set(GRS_EXPECTED_{kind} "${{_grs_result_{kind}}}" CACHE INTERNAL "T666 one evaluated SDK policy plus baseline" FORCE)
'''
 text+='file(WRITE "${CMAKE_BINARY_DIR}/grs-sdk-flags.cmake" "set(GRS_SDK_COMMON [==[${OHOS_COMMON_LINKER_FLAGS} ${_DEVICEINFO_LIB}]==])\\nset(GRS_SDK_EXE [==[${OHOS_COMMON_LINKER_FLAGS} ${OHOS_EXE_LINKER_FLAGS} ${_DEVICEINFO_LIB}]==])\\n")\n'
 write(wrapper,text)
 record={'schema':p['schema'],'policySHA256':digest,'policy':p,'sdkToolchainSHA256':sha(sdk),'wrapperSHA256':sha(wrapper),'note':'Known original baseline re-established before every SDK evaluation; ordered target/implicit libraries untouched.'}
 write(marker,json.dumps(record,indent=2)+'\n')
 defs=p.get('definitions',[])
 if any(not isinstance(x,str) or not x.startswith('-D') or x.startswith(('-DCMAKE_TOOLCHAIN_FILE=','-DCMAKE_SHARED_LINKER_FLAGS=','-DCMAKE_MODULE_LINKER_FLAGS=','-DCMAKE_EXE_LINKER_FLAGS=')) for x in defs):raise ValueError('UNKNOWN_OR_DUPLICATE_POLICY_DEFINITION')
 argv=[p['cmake'],'-S',str(Path(p['source']).resolve()),'-B',str(build),'-G','Ninja','-DCMAKE_TOOLCHAIN_FILE='+wrapper.as_posix(),'-DCMAKE_MAKE_PROGRAM='+p['ninja'],'-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',*defs]
 result=native_call(argv,p['source'])
 write(build/'grs-configure.stdout',result.stdout.decode('utf8',errors='replace'));write(build/'grs-configure.stderr',result.stderr.decode('utf8',errors='replace'))
 record['configureArgv']=argv;record['cwd']=p['source'];record['configureReturnCode']=result.returncode;write(marker,json.dumps(record,indent=2)+'\n')
 if result.returncode:raise RuntimeError('CMAKE_CONFIGURE_FAILED '+str(build/'grs-configure.stderr'))
 return p

def windows_argv(command):
 if os.name!='nt':raise ValueError('WINDOWS_COMMAND_PROVENANCE_REQUIRED')
 count=ctypes.c_int();parse=ctypes.windll.shell32.CommandLineToArgvW;parse.argtypes=[ctypes.c_wchar_p,ctypes.POINTER(ctypes.c_int)];parse.restype=ctypes.POINTER(ctypes.c_wchar_p)
 data=parse(command,ctypes.byref(count))
 if not data:raise ctypes.WinError()
 try:return [data[i] for i in range(count.value)]
 finally:ctypes.windll.kernel32.LocalFree(ctypes.cast(data,ctypes.c_void_p))

def native_driver(command):
 # Accept only the observed CMake no-op pre/post shell envelope. Never execute it.
 m=re.fullmatch(r'[^\r\n]*?cmd\.exe /C "cd \. && (.+) && cd \."',command,re.I)
 if not m:raise ValueError('UNKNOWN_CMAKE_LINK_ENVELOPE')
 driver=m.group(1)
 if ' && ' in driver or ' || ' in driver:raise ValueError('UNDECLARED_LINK_ACTION')
 args=windows_argv(driver)
 if not args or not Path(args[0]).name.lower().startswith('clang'):raise ValueError('UNEXPECTED_LINK_DRIVER')
 return args

def rsp_quote(arg):
 if '\x00' in arg or '\n' in arg or '\r' in arg:raise ValueError('INVALID_DRIVER_ARGUMENT')
 return '"'+arg.replace('\\','\\\\').replace('"','\\"')+'"'
def ninja_path(p):return str(p).replace('\\','/').replace('$','$$').replace(' ','$ ').replace(':','$:')
def prepare_target(policy,build,target):
 build=Path(build).resolve();p=json.loads(Path(policy).read_text(encoding='utf8'))
 if target!='grs_cold_slots':raise ValueError('TARGET_NOT_AUTHORIZED')
 existing=build/'grs_cold_slots.link.json'
 if 'rule GRS_T666_BROKER_LINK' in (build/'build.ninja').read_text(encoding='utf8'):
  if not existing.exists():raise ValueError('UNKNOWN_PATCHED_TARGET_RECONFIGURE')
  old=json.loads(existing.read_text(encoding='utf8'))
  if old['policySHA256']!=sha(policy) or json.loads((build/'grs_cold_slots.patch-state.json').read_text(encoding='utf8'))['patchedNinjaSHA256']!=sha(build/'build.ninja') or old['rspSHA256']!=sha(old['rsp']) or old['launcherSHA256']!=sha(old['sourceLauncher']):raise ValueError('STALE_LINK_STATE_RECONFIGURE')
  return existing

 # CMake/Ninja itself expands response content in compdb (-x); do not guess objects.
 compdb_result=native_call([p['ninja'],'-t','compdb','-x'],build)
 compdb_result.check_returncode()
 compdb=json.loads(compdb_result.stdout)
 selected=[x for x in compdb if Path(x.get('output','').replace(chr(92),'/')).name=='libgrs_cold_slots.so' and re.search(r'-shared\s',x.get('command',''))]
 if len(selected)!=1:raise ValueError('LINK_PRODUCER_NOT_UNIQUE')
 entry=selected[0];argv=native_driver(entry['command']);original=list(argv)
 # Canonicalize only actual driver/file path operands, never arbitrary user tokens.
 argv[0]=Path(argv[0]).resolve().as_posix()
 for i,arg in enumerate(argv[1:],1):
  if i>1 and argv[i-1]=='-o' or not arg.startswith('-') and arg.endswith(('.o','.obj','.a','.so')):argv[i]=arg.replace('\\','/')
  elif arg.startswith(('--sysroot=','--gcc-toolchain=','-L')):argv[i]=arg.replace('\\','/')
 rsp=build/'grs_cold_slots.full.rsp';write(rsp,'\n'.join(rsp_quote(x) for x in argv[1:])+'\n')
 source=Path(__file__).with_name('link_from_manifest.py').resolve();manifest=build/'grs_cold_slots.link.json'
 objects=[str((build/x).resolve()) for x in argv[1:] if not x.startswith('-') and x.endswith(('.o','.obj'))]
 data={'schema':'GRS_FULL_DRIVER_LINK_V1','target':target,'cwd':str(build),'originalCommand':entry['command'],'originalNativeArgv':original,'driver':argv[0],'driverSHA256':sha(argv[0]),'effectiveArgv':argv[1:],'rsp':str(rsp),'rspSHA256':sha(rsp),'objects':objects,'policySHA256':sha(policy),'sourceLauncher':str(source),'launcherSHA256':sha(source)}
 # Replace only this target's link rule; retain its original edge/dependencies/output.
 ninja=build/'build.ninja';body=ninja.read_text(encoding='utf8');rule=entry.get('file','')
 matches=list(re.finditer(r'^(build [^\n]+): (CXX_SHARED_LIBRARY_LINKER__grs_cold_slots_\S+)([^\n]*)$',body,re.M))
 if len(matches)!=1:raise ValueError('NINJA_TARGET_EDGE_NOT_UNIQUE')
 m=matches[0];deps=m.group(3);extra=' '.join(ninja_path(x) for x in [manifest,rsp,source])
 if ' || ' in deps:
  direct,order=deps.split(' || ',1);direct+=(' ' if ' | ' in direct else ' | ')+extra;deps=direct+' || '+order
 else:deps+=(' ' if ' | ' in deps else ' | ')+extra
 body=body[:m.start()]+m.group(1)+': GRS_T666_BROKER_LINK'+deps+body[m.end():]
 command=subprocess.list2cmdline([sys.executable,str(source),str(manifest)]).replace('$','$$')
 definition='\n# T666 target-limited full driver response, short outer command.\nrule GRS_T666_BROKER_LINK\n  command = '+command+'\n  description = Linking grs_cold_slots with complete driver rsp\n  restat = 1\n'
 anchor='include CMakeFiles/rules.ninja\n'
 if body.count(anchor)!=1:raise ValueError('NINJA_RULES_INCLUDE_UNKNOWN')
 body=body.replace(anchor,anchor+definition)
 write(ninja,body);data['outerCommand']=command;write(manifest,json.dumps(data,indent=2)+'\n');write(build/'grs_cold_slots.patch-state.json',json.dumps({'patchedNinjaSHA256':sha(ninja),'manifestSHA256':sha(manifest)},indent=2)+'\n');return manifest
if __name__=='__main__':
 policy,build=sys.argv[2:4]
 if sys.argv[1]=='configure':configure(policy,build)
 elif sys.argv[1]=='prepare-link':print(prepare_target(policy,build,sys.argv[4]))
 else:raise ValueError('ACTION_REQUIRED')
