"""Generate, compile, package and archive two isolated unsigned H1 variants.

No production edits, device access, signing, algorithm or instrumentation edits.
Use a fresh --name-prefix for each run; stale projects are never overwritten.
"""
import argparse, json, os, re, subprocess, sys
from pathlib import Path
from archive_t191_post_build import archive

root = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument('--name-prefix', default='t191-strict')
parser.add_argument('--reports', type=Path, default=root/'test/reports/t191')
options = parser.parse_args()
if not re.fullmatch(r'[a-zA-Z0-9_-]{1,64}', options.name_prefix):
    raise SystemExit('INVALID_NAME_PREFIX')
reports = options.reports.resolve()
reports.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env['JAVA_HOME'] = r'C:\Program Files\Huawei\DevEco Studio\jbr'
env['PATH'] = env['JAVA_HOME']+r'\bin;'+env['PATH']
hvigor = 'C:/Program Files/Huawei/DevEco Studio/tools/hvigor/bin/hvigorw.bat'
summary = []
for suffix, variant in [('o0', 'STRICT_O0'), ('o2', 'STRICT_O2')]:
    project = root/'.codex/local-test'/(options.name_prefix+'-'+suffix)
    if project.exists():
        raise SystemExit('OUTPUT_ALREADY_EXISTS:'+str(project))
    generated = subprocess.check_output(['node', str(Path(__file__).with_name('generate_transaction_project_v4.cjs')), str(project), variant], cwd=root)
    (reports/('generate-'+suffix+'.json')).write_bytes(generated)
    # PackageHap is the explicitly unsigned task. Do not alter signing material
    # or claim an assemble/SignHap success from this zero exit code.
    command = "& '"+hvigor+"' --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon"
    with (reports/('package-'+suffix+'.txt')).open('wb') as log:
        code = subprocess.run(['powershell', '-NoProfile', '-Command', command], cwd=project, env=env, stdout=log, stderr=subprocess.STDOUT).returncode
    if code:
        raise SystemExit('PACKAGE_FAILED:'+variant+':'+str(code))
    result = archive(project, reports/('strict-'+suffix))
    summary.append({'variant': variant, 'compilePackageExitCode': code, 'unsignedHapSha256': result['unsignedHap']['sha256'],
                    'sourceManifestSha256': result['sourceManifestSha256'], 'signed': 'NOT_BUILT', 'device': 'NOT_RUN'})
(reports/'build-summary.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8')
print(json.dumps(summary))
