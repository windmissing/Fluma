"""Validate the exact file selected for installation and print tester commands only."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

p = argparse.ArgumentParser()
p.add_argument('target', choices=['t410', 't437'])
p.add_argument('hap', type=Path)
p.add_argument('--declared-sha', help='validate an existing cold Want declaration against the file')
p.add_argument('--receipt', type=Path, help='exact signing receipt for this generation; defaults to the T450 receipt')
a = p.parse_args()
receipt = a.receipt or Path(f'test/reports/t257/T450-{a.target}.json')
report = json.loads(receipt.read_text(encoding='utf-8'))
if a.receipt is not None:
    if a.target not in report.get('task', '').split('-'):
        p.error('SIGNING_RECEIPT_TARGET_MISMATCH')
    if report.get('verifyApp') != 'PASS':
        p.error('SIGNING_RECEIPT_NOT_VERIFIED')
actual = hashlib.sha256(a.hap.read_bytes()).hexdigest()
if actual != report['signed']['sha256'] or a.hap.stat().st_size != report['signed']['bytes']:
    p.error('INSTALL_HAP_IDENTITY_MISMATCH')
if a.declared_sha is not None and a.declared_sha.lower() != actual:
    p.error('COLD_WANT_SHA_MISMATCH')
with zipfile.ZipFile(a.hap) as z:
    lib = z.read('libs/arm64-v8a/libgrsm1.so')
    app = json.loads(z.read('module.json'))['app']
    if b'invalidField' not in lib or b'PROGRESS_V2_NEXT_RUN' not in lib:
        p.error('CAPTURE_PROFILE_MISSING')
    if (b'T434_COPY_ACCOUNTING_V1' in lib) != (a.target == 't437'):
        p.error('WRONG_T434_VARIANT')
    if app['bundleName'] != report['bundleName']:
        p.error('BUNDLE_IDENTITY_MISMATCH')
print(json.dumps({'schema':'T450_SIGNED_INSTALL_INPUT_V1','target':a.target,'sha256':actual,
    'signingReceipt':str(receipt.resolve()),
    'file':str(a.hap.resolve()),'bytes':a.hap.stat().st_size,'bundle':app['bundleName'],
    'versionCode':app.get('versionCode'),'versionName':app.get('versionName'),
    'install':['hdc','-t','<device>','install','-r',str(a.hap.resolve())],
    'stop':['hdc','-t','<device>','shell','aa','force-stop',app['bundleName']],
    'launch':['hdc','-t','<device>','shell','aa','start','-b',app['bundleName'],'-a','EntryAbility',
              '--pb','grsm1','true','--ps','gpusha',actual],
    'device':'NOT_RUN'},indent=2))
