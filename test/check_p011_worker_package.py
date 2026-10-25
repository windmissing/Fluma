"""T43 offline package check. No install, device access, or runtime PASS claim."""
import argparse, hashlib, json, re, subprocess, zipfile
from pathlib import Path
parser = argparse.ArgumentParser()
parser.add_argument('--hap', default='entry/build/default/outputs/default/entry-default-unsigned.hap')
parser.add_argument('--disassembler', default='C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/toolchains/ark_disasm.exe')
parser.add_argument('--output', default='test/reports/t43/worker-package.json')
args = parser.parse_args()
hap = Path(args.hap)
store = Path('entry/src/main/ets/physics/P011NaturalObservationStore.ets').read_text(encoding='utf-8')
profile = Path('entry/build-profile.json5').read_text(encoding='utf-8')
assert "entry/ets/workers/P011NaturalObservationWorker.ets" in store
assert "./src/main/ets/workers/P011NaturalObservationWorker.ets" in profile
out = Path('.codex/local-test/t43-package'); out.mkdir(exist_ok=True, parents=True)
with zipfile.ZipFile(hap) as z:
    entries = [n for n in z.namelist() if n.endswith('.abc')]
    abc = z.read('ets/modules.abc')
(out / 'modules.abc').write_bytes(abc)
subprocess.run([args.disassembler, str(out / 'modules.abc'), str(out / 'modules.pa')], check=True, capture_output=True)
text = (out / 'modules.pa').read_text(encoding='utf-8')
record = re.search(r'^\.record [^ \n]*\.ets\.workers\.P011NaturalObservationWorker \{', text, re.M)
assert record, 'packaged worker record missing'
assert re.search(r'^\.function .*\.ets\.workers\.P011NaturalObservationWorker\.func_main_0\(', text, re.M), 'worker entry body missing'
assert not re.search(r'^\.record [^ \n]*\.ets\.physics\.P011NaturalObservationWorker \{', text, re.M), 'stale worker record'
result = {'status': 'PASS', 'scope': 'OFFLINE_PACKAGE_ONLY_NOT_DEVICE_RUNTIME',
          'hap': str(hap), 'hapSha256': hashlib.sha256(hap.read_bytes()).hexdigest(),
          'abcEntries': entries, 'workerPackaging': 'ES_MODULE_RECORD_IN_MODULES_ABC',
          'workerRecord': record.group(0)[:-2], 'threadWorkerPath': 'entry/ets/workers/P011NaturalObservationWorker.ets',
          'modulesAbcSha256': hashlib.sha256(abc).hexdigest(),
          'workerSourceSha256': hashlib.sha256(Path('entry/src/main/ets/workers/P011NaturalObservationWorker.ets').read_bytes()).hexdigest(),
          'standaloneWorkerAbc': False}
Path(args.output).parent.mkdir(exist_ok=True, parents=True)
Path(args.output).write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps(result, ensure_ascii=False, indent=2))
