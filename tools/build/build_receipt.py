# T666 artifact receipt only: native files/argv/ELF, no product loading.
from pathlib import Path
import hashlib,json,subprocess,re,shutil,sys
root=Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 build=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)
 m=json.loads((build/'grs_cold_slots.link.json').read_text());policy=json.loads((build/'grs-link-policy.json').read_text());receipt=json.loads((build/'grs_cold_slots.link-receipt.json').read_text());p=policy['policy'];sdk=Path(p['sdkToolchain']).parents[2]
 readelf=sdk/'llvm/bin/llvm-readelf.exe';nm=sdk/'llvm/bin/llvm-nm.exe'
 binary=Path(receipt['output']);consumer=build/'libgrsT666Consumer.so'
 exports=subprocess.check_output([str(nm),'--dynamic','--defined-only',str(binary)]).decode();dynamic=subprocess.check_output([str(readelf),'-d','-n',str(binary)]).decode();needed=subprocess.check_output([str(readelf),'-d','-n',str(consumer)]).decode()
 expected=['GrsColdReserve','GrsColdBuilderDone','GrsColdReadBegin','GrsColdReadEnd','GrsColdFreed','GrsFullGpuReserve','GrsFullGpuFreed','GrsFullGpuContextRetired'];assert all(re.search(r'\b'+x+r'$',exports,re.M) for x in expected);assert '[libgrs_cold_slots.so]' in dynamic and '[libgrs_cold_slots.so]' in needed
 assert receipt['returnCode']==0 and len(receipt['argv'])==2 and receipt['argv'][1].startswith('@')
 cache=(build/'CMakeCache.txt').read_text();flags=re.search(r'^CMAKE_SHARED_LINKER_FLAGS:STRING=(.*)$',cache,re.M).group(1);assert flags.count('--rtlib=compiler-rt')==1
 assert policy['sdkToolchainSHA256']==sha(p['sdkToolchain'])
 commands=json.loads((build/'compile_commands.json').read_text());sources={x['file']:sha(x['file']) for x in commands};assert '-fno-fast-math' in json.dumps(commands) and '-ffp-contract=off' in json.dumps(commands)
 result={'schema':'T666_OWNED_NATIVE_BUILD_DELIVERY_V1','developmentCompile':'PASS','productPASS':False,'testsRun':False,'deviceUsed':False,'productExecuted':False,'HAP':'NOT_APPLICABLE build-only wrapper/native broker ABI scope; no App behavior changed','SDKUnmodified':True,'flags':{'explicitUserBaseline':p['baseline'],'SDKSharedBlockCount':1,'effectiveSharedCache':flags,'orderedArgvPreserved':True,'globalTokenDeduplication':False,'FPGuardsPreserved':True,'reconfigure':'Same known baseline, SDK count1; final ninja no work to do'},'toolSHA':{str(x):sha(x) for x in [Path(m['driver']),sdk/'llvm/bin/ld.lld.exe',readelf,nm,Path(p['cmake']),Path(p['ninja'])]},'sourceSHA':sources,'ownedTools':{str(x.relative_to(root)):sha(x) for x in (root/'tools/build').rglob('*') if x.is_file() and '__pycache__' not in str(x)},'outerCommand':m['outerCommand'],'outerCharacters':len(m['outerCommand']),'nativeArgv':receipt['argv'],'rsp':{'sha256':sha(m['rsp']),'bytes':Path(m['rsp']).stat().st_size,'allEffectiveArgv':m['effectiveArgv'],'encoding':'UTF8 no BOM LF; LLVM driver rsp quoting, separate from Windows native argv'},'ELF':{'path':str(binary),'sha256':sha(binary),'buildId':re.search(r'Build ID: ([a-f0-9]+)',dynamic).group(1),'requiredEightSymbols':expected,'soname':'libgrs_cold_slots.so','needed':re.findall(r'\(NEEDED\).*?\[(.*?)\]',dynamic)},'consumer':{'sha256':sha(consumer),'needed':re.findall(r'\(NEEDED\).*?\[(.*?)\]',needed)},'objects':receipt['objects'],'manifestSHA256':sha(build/'grs_cold_slots.link.json'),'SDKToolchainSHA256':policy['sdkToolchainSHA256'],'scope':'actual repo broker through existing cold_slot_broker.cmake; only its Ninja edge/rule changed, original deps plus manifest/rsp/launcher. Incremental rebuild requires changed inputs.'}
 for name in ['grs-link-policy.json','grs_owned_toolchain.cmake','grs-sdk-flags.cmake','CMakeCache.txt','compile_commands.json','grs_cold_slots.link.json','grs_cold_slots.patch-state.json','grs_cold_slots.full.rsp','grs_cold_slots.link-receipt.json','grs_cold_slots.driver-diagnostic.log','build.ninja']:
  shutil.copyfile(build/name,out/name)
 shutil.copyfile(root/'.codex/local-test/t666-delivery.log',out/'native-build.log');shutil.copyfile(root/'.codex/local-test/t666-final-incremental.log',out/'incremental.log')
 (out/'exports.txt').write_text(exports);(out/'dynamic.txt').write_text(dynamic+'\nCONSUMER:\n'+needed)
 (out/'compile-receipt.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8');print(json.dumps({'ELF':result['ELF'],'flags':result['flags'],'outerCharacters':result['outerCharacters']},indent=2))
if __name__=='__main__':main()
