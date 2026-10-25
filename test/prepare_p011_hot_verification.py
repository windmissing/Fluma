"""Create a disposable verification project without touching shared edited sources.

The accepted S0 baseline supplies unrelated app files; current M1 files and tests
are overlaid exactly. This verifies D1 in isolation, not the concurrent M2 tree.
"""
from pathlib import Path
import io,subprocess,tarfile,shutil,json
git=r'C:\Program Files\WorkBuddy\resources\vendor\PortableGit\cmd\git.exe'
root=Path(__file__).resolve().parents[1]
destination=root/'.codex/local-test/p011-hot-verification'
destination.mkdir(parents=True,exist_ok=True)
members=['AppScope','entry','hvigor','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5','test','docs/endpoint-protocol-P011-terminal-nondeposited-v1.md']
payload=subprocess.check_output([git,'archive','1511fbf',*members],cwd=root)
with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
    for member in archive.getmembers():
        target=(destination/member.name).resolve()
        if not target.is_relative_to(destination.resolve()):raise RuntimeError('archive path outside verification project')
    archive.extractall(destination,filter='data')
for name in ['P011ParticleField.ets','P011ParticlePrototype.ets','P011ParticleRefundAdapter.ets',
             'P011ParticleHot.ets','P011ParticleHotBudget.ets','P011ParticleHotPrototype.ets','P011ParticleHotEvidence.ets']:
    source=root/'entry/src/main/ets/physics'/name
    shutil.copyfile(source,destination/source.relative_to(root))
for name in ['P011NaturalObservation.ets']:
    # The committed D6 declaration is included, without unpublished peak/M2 edits.
    relative='entry/src/main/ets/physics/'+name
    (destination/relative).write_bytes(subprocess.check_output([git,'show','HEAD:'+relative],cwd=root))
for source in (root/'test').glob('*'):
    if source.is_file() and ('particle_hot' in source.name or source.name in
        ('p011_endpoint_check.py','test_p011_endpoint_check.py','p011_endpoint_protocol_v1_execution.py','run_p011_endpoint_units.cjs')):
        shutil.copyfile(source,destination/source.relative_to(root))
shutil.copytree(root/'test/fixtures',destination/'test/fixtures',dirs_exist_ok=True)
shutil.copyfile(root/'local.properties',destination/'local.properties')
for relative in ['oh_modules','entry/oh_modules']:
    source=root/relative;target=destination/relative
    if source.exists() and not target.exists():
        subprocess.run(['powershell','-NoProfile','-Command',
            "New-Item -ItemType Junction -Path '"+str(target)+"' -Target '"+str(source)+"' | Out-Null"],check=True)
# Reuse the bundled compiler directly; do not install another wrapper runtime.
for package in ['hvigor','hvigor-ohos-plugin']:
    target=destination/'node_modules/@ohos'/package
    target.parent.mkdir(parents=True,exist_ok=True)
    if not target.exists():
        source=Path(r'C:\Program Files\Huawei\DevEco Studio\tools\hvigor')/package
        subprocess.run(['powershell','-NoProfile','-Command',
            "New-Item -ItemType Junction -Path '"+str(target)+"' -Target '"+str(source)+"' | Out-Null"],check=True)
config=destination/'hvigor/hvigor-config.json5'
value=json.loads(config.read_text(encoding='utf-8'));value.pop('ohos',None);value.pop('hvigorVersion',None)
config.write_text(json.dumps(value),encoding='utf-8')
# Register the otherwise deliberately disconnected D1 modules in ArkTS compilation.
probe=destination/'entry/src/main/ets/physics/P011ParticleHotCompileProbe.ets'
probe.write_text("""import { P011ParticleHotConfig, P011ParticleHotWorkspace, configureHot } from './P011ParticleHot';
import { P011ParticleHotPrototypeSink } from './P011ParticleHotPrototype';
import { registerParticleHotRun, exportParticleEvidence, P011ParticleHotEvidence } from './P011ParticleHotEvidence';
export function compileHot(config: P011ParticleHotConfig): P011ParticleHotWorkspace {
  const work = configureHot(config); const sink = new P011ParticleHotPrototypeSink(work);
  registerParticleHotRun(work, 1, 'compile');
  if (sink.work !== work) throw new Error('COMPILE_PROBE');
  return work;
}
export function compileExport(work: P011ParticleHotWorkspace, token: number): P011ParticleHotEvidence {
  return exportParticleEvidence(work, token);
}
""",encoding='utf-8')
profile=destination/'entry/build-profile.json5'
text=profile.read_text(encoding='utf-8')
profile.write_text(text.replace('"workers": [','"workers": ["./src/main/ets/physics/P011ParticleHotCompileProbe.ets",',1),encoding='utf-8')
print(destination)
