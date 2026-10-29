"""Offline handoff manifest. Hash actual ELF/HAP bytes; never infer signed/install success."""
from pathlib import Path
import hashlib,json,zipfile,shutil
root=Path(__file__).resolve().parents[2]
sha=lambda b:hashlib.sha256(b).hexdigest()
projects={246:('t246-evidence','t246-evidence.txt'),248:('t248-delivery','t248-delivery.txt'),250:('t250-handoff','t250-handoff.txt')}
for task,(folder,log) in projects.items():
    project=root/'.codex/local-test'/folder
    report=root/f'test/reports/t{task}';report.mkdir(parents=True,exist_ok=True)
    build=(root/'.codex/local-test'/log).read_bytes();sign=(root/f'.codex/local-test/t{task}-sign.txt').read_bytes()
    assert b'BUILD SUCCESSFUL' in build and b'BUILD FAILED' in sign
    (report/'build.txt').write_bytes(build);(report/'sign.txt').write_bytes(sign)
    manifest=json.loads((project/f't{task}-manifest.json').read_text(encoding='utf-8'))
    (report/'source-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    artifacts=[]
    for file in sorted((project/'entry/build').rglob('*')):
        if not file.is_file() or file.suffix not in ('.hap','.so'):continue
        data=file.read_bytes();row={'path':str(file),'bytes':len(data),'sha256':sha(data)}
        if file.suffix=='.so':
            assert data[:4]==b'\x7fELF';row['elfMachine']=int.from_bytes(data[18:20],'little');assert row['elfMachine']==183
        if file.suffix=='.hap':
            with zipfile.ZipFile(file) as z:
                row['packagedSo']=[{'name':n,'sha256':sha(z.read(n)),'bytes':len(z.read(n))} for n in z.namelist() if n.endswith('.so')]
        artifacts.append(row)
    assert any(a['path'].endswith('.hap') for a in artifacts)
    inputs={str(p.relative_to(root)):sha(p.read_bytes()) for p in (root/f'test/t{task}').rglob('*') if p.is_file()}
    payload={'revision':f'T{task}_DEVELOPER_HANDOFF_V1','date':'2026-10-29','project':str(project),'compile':'PASS_ARKTS_ARM64_NATIVE','package':'PASS_UNSIGNED','sign':'FAIL_00303116_PASSWORD_FIELD_LENGTH','install':'NOT_RUN','device':'NOT_RUN','recipeFiles':inputs,'artifacts':artifacts,'buildLogSha256':sha(build),'signLogSha256':sha(sign),'signingConfigUnchanged':sha((root/'build-profile.json5').read_bytes())==sha((project/'build-profile.json5').read_bytes())}
    assert payload['signingConfigUnchanged']
    (report/'delivery.json').write_text(json.dumps(payload,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    if task==250:shutil.copyfile(project/'t250-dictionary.json',report/'dictionary.json')
    print(task,payload['compile'],payload['package'],payload['sign'],len(artifacts))
