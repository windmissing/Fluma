# T666 direct native argv; rsp is one argument, shell=False. No product execution.
from pathlib import Path
import hashlib,json,subprocess,sys,struct

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 path=Path(sys.argv[1]);m=json.loads(path.read_text(encoding='utf8'));cwd=Path(m['cwd'])
 if m['schema']!='GRS_FULL_DRIVER_LINK_V1' or m['target']!='grs_cold_slots' or sha(m['driver'])!=m['driverSHA256'] or sha(m['rsp'])!=m['rspSHA256']:raise ValueError('LINK_MANIFEST_IDENTITY_CHANGED')
 argv=[m['driver'],'@'+Path(m['rsp']).resolve().as_posix()]
 # Driver diagnostic argv only, followed by real native compilation/linking.
 probe=subprocess.run([m['driver'],'-###',argv[1]],cwd=cwd,capture_output=True,shell=False)
 (cwd/'grs_cold_slots.driver-diagnostic.log').write_bytes(probe.stdout+probe.stderr)
 if probe.returncode:raise RuntimeError('DRIVER_RESPONSE_REJECTED')
 result=subprocess.run(argv,cwd=cwd,capture_output=True,shell=False)
 (cwd/'grs_cold_slots.link.stdout').write_bytes(result.stdout);(cwd/'grs_cold_slots.link.stderr').write_bytes(result.stderr)
 output=m['effectiveArgv'][m['effectiveArgv'].index('-o')+1];elf=(cwd/output).resolve()
 receipt={'schema':'T666_NATIVE_LINK_RECEIPT_V1','argv':argv,'cwd':str(cwd),'driverSHA256':sha(m['driver']),'rspSHA256':sha(m['rsp']),'originalArgv':m['originalNativeArgv'],'effectiveArgv':m['effectiveArgv'],'returnCode':result.returncode,'objects':{p:sha(p) for p in m['objects']},'diagnosticSHA256':sha(cwd/'grs_cold_slots.driver-diagnostic.log'),'manifestSHA256':sha(path),'output':str(elf),'ELFSHA256':sha(elf) if result.returncode==0 else None}
 (cwd/'grs_cold_slots.link-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf8')
 if result.returncode:sys.stderr.buffer.write(result.stderr);raise SystemExit(result.returncode)
if __name__=='__main__':main()
