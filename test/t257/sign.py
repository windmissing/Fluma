"""Authorized local signing. Read credentials in memory; never print argv/material/logs.
No device operations. Config and keystore are not modified. Output package is new.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess,zipfile,ssl,re
ROOT=Path(__file__).resolve().parents[2]
JAVA=Path('C:/Program Files/Huawei/DevEco Studio/jbr/bin/java.exe')
JAR=Path('C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/lib/hap-sign-tool.jar')
sha=lambda b:hashlib.sha256(b).hexdigest()
parser=argparse.ArgumentParser();parser.add_argument('task');parser.add_argument('project');args=parser.parse_args()
project=Path(args.project).resolve();assert project.is_relative_to(ROOT/'.codex/local-test')
unsigned=list((project/'entry/build/default/outputs').rglob('*unsigned.hap'));assert len(unsigned)==1;unsigned=unsigned[0]
work=ROOT/'.codex/local-test/t257-sign'/args.task;work.mkdir(parents=True,exist_ok=True)
report=ROOT/'test/reports/t257';report.mkdir(exist_ok=True)
config=ROOT/'build-profile.json5';config_before=config.read_bytes();m=json.loads(config_before)['app']['signingConfigs'][0]['material']
for key in ['storeFile','certpath','profile']:assert Path(m[key]).is_file()
def invoke(operation,options):
 result=subprocess.run([str(JAVA),'-jar',str(JAR),operation,*options],capture_output=True)
 # Tool output is intentionally kept in memory only: external signer could echo material.
 row={'operation':operation,'exitCode':result.returncode,'stdoutBytes':len(result.stdout),'stderrBytes':len(result.stderr),'outputSha256':sha(result.stdout+result.stderr)}
 if result.returncode:
  message=(result.stdout+result.stderr).decode('utf-8',errors='replace')
  for value in m.values():
   if isinstance(value,str) and value:message=message.replace(value,'[MATERIAL_REDACTED]')
  row['diagnostic']=message[-2500:]
  (report/(args.task+'-failure.json')).write_text(json.dumps(row,indent=2)+'\n',encoding='utf-8')
  raise RuntimeError(json.dumps(row))
 return row
checks=[];checks.append(invoke('verify-profile',['-inFile',m['profile'],'-outFile',str(work/'profile.json')]))
profile=json.loads((work/'profile.json').read_text(encoding='utf-8'));assert profile['verifiedPassed'] is True
with zipfile.ZipFile(unsigned) as z:
 module=json.loads(z.read('module.json'));bundle=module['app']['bundleName'];api=module['app']['minAPIVersion'];assert bundle==profile['content']['bundle-info']['bundle-name']
 def entries(package):
  return {n:sha(package.read(n)) for n in package.namelist() if not n.endswith('/')}
 original=entries(z)
def certificate_der(pem):
 block=re.search(r'-----BEGIN CERTIFICATE-----[\s\S]+?-----END CERTIFICATE-----',pem).group(0)
 return ssl.PEM_cert_to_DER_cert(block)
cert=Path(m['certpath']).read_bytes()
if b'BEGIN CERTIFICATE' in cert:cert=certificate_der(cert.decode('ascii'))
profile_cert=certificate_der(profile['content']['bundle-info']['development-certificate']);(work/'profile-cert.der').write_bytes(profile_cert)
exported=work/'store-cert.der'
r=subprocess.run([str(JAVA.parent/'keytool.exe'),'-exportcert','-alias',m['keyAlias'],'-keystore',m['storeFile'],'-storepass',m['storePassword'],'-file',str(exported)],capture_output=True)
assert r.returncode==0 and sha(exported.read_bytes())==sha(cert)
keys=subprocess.run([str(JAVA),str(ROOT/'test/t257/CertPublic.java'),str(exported),str(work/'profile-cert.der')],capture_output=True)
assert keys.returncode==0
public_keys=keys.stdout.decode('ascii').split();assert len(public_keys)==2 and public_keys[0]==public_keys[1]
# The existing profile has a valid renewed leaf for the SAME authorized key.
# Assemble only public certificate bytes in a temporary copy, never repair/mutate materials.
blocks=re.findall(r'-----BEGIN CERTIFICATE-----[\s\S]+?-----END CERTIFICATE-----',Path(m['certpath']).read_text(encoding='ascii'))
assert len(blocks)>=2
sign_chain=work/'profile-matched-chain.cer';sign_chain.write_text(profile['content']['bundle-info']['development-certificate']+'\n'+'\n'.join(blocks[1:])+'\n',encoding='ascii')
chain_check=subprocess.run([str(JAVA),str(ROOT/'test/t257/VerifyChain.java'),str(sign_chain)],capture_output=True)
assert chain_check.returncode==0 and b'VALID_CHAIN' in chain_check.stdout
input_sha=sha(unsigned.read_bytes());signed=work/(args.task+'-signed.hap');assert not signed.exists(),'NO_OVERWRITE_SIGNED_ARTIFACT'
checks.append(invoke('sign-app',['-mode','localSign','-keyAlias',m['keyAlias'],'-appCertFile',str(sign_chain),'-profileFile',m['profile'],'-inFile',str(unsigned),'-signAlg',m['signAlg'],'-keystoreFile',m['storeFile'],'-keystorePwd',m['storePassword'],'-keyPwd',m['keyPassword'],'-outFile',str(signed),'-compatibleVersion',str(api)]))
checks.append(invoke('verify-app',['-inFile',str(signed),'-outCertChain',str(work/'verified-chain.cer'),'-outProfile',str(work/'verified-profile.p7b')]))
assert (work/'verified-chain.cer').is_file() and (work/'verified-profile.p7b').is_file()
checks.append(invoke('verify-profile',['-inFile',str(work/'verified-profile.p7b'),'-outFile',str(work/'verified-profile.json')]))
verified=json.loads((work/'verified-profile.json').read_text(encoding='utf-8'));assert verified['verifiedPassed'] is True and verified['content']['bundle-info']['bundle-name']==bundle
assert sha(certificate_der(verified['content']['bundle-info']['development-certificate']))==sha(profile_cert)
with zipfile.ZipFile(signed) as z:
 after=entries(z);assert all(after.get(n)==v for n,v in original.items()),'SIGNED_ZIP_PAYLOAD_CHANGED'
assert config.read_bytes()==config_before and sha(unsigned.read_bytes())==input_sha
result={'revision':'T257_LOCAL_SIGNED_HANDOFF_V1','task':args.task,'bundleName':bundle,'compatibleVersion':api,'input':{'path':str(unsigned),'sha256':input_sha},'signed':{'path':str(signed),'sha256':sha(signed.read_bytes()),'bytes':signed.stat().st_size},'toolSha256':sha(JAR.read_bytes()),'certificateSha256':sha(cert),'profileCertificateSha256':sha(profile_cert),'certificateSPKI':public_keys[0],'profileVerified':True,'profileMatchedPublicChainSha256':sha(sign_chain.read_bytes()),'originalLeafSignatureMismatchHandledWithExistingProfileCertificate':True,'keystoreCertificateMatchesProfile':True,'verifyApp':'PASS','payloadEntriesUnchanged':len(original),'sourceConfigUnchanged':True,'credentialsLogged':False,'device':'NOT_RUN','operations':checks}
(report/(args.task+'.json')).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps({'task':args.task,'sign':'PASS','verify':'PASS','inputSha':input_sha,'signedSha':result['signed']['sha256']}))
