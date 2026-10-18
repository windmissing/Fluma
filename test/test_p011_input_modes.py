"""Read-only pre-implementation source inventory; not a runtime mode test.

Run: python test/test_p011_input_modes.py --output <new-json-path>
Missing runtime evidence is BLOCKED, never inferred PASS from source strings.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[1]
GIT = r'C:\Users\admin\AppData\Local\GitHubDesktop\app-3.6.6\resources\app\git\mingw64\bin\git.exe'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    paths = ['entry/src/main/ets/pages/SandCanvas.ets',
             'entry/src/main/ets/entryability/EntryAbility.ets',
             'entry/src/main/ets/physics/P011TiltWant.ets']
    sources = {p: (ROOT / p).read_text(encoding='utf-8') for p in paths}
    canvas, _, want = sources.values()
    checks = [
        ('parser-test-only-guard', 'if (!testMode)' in want),
        ('parser-force-invert-guard', 'if (forceInvert || rawForceInvert)' in want),
        ('parser-fresh-sequence-guard', 'sequence <= consumedSequence' in want),
        ('sensor-subscription-present', 'sensor.on(sensor.SensorId.ACCELEROMETER' in canvas),
        ('sensor-route-test-exclusion', 'this.useSensor && !this.testMode' in canvas),
        ('script-target-hold-present', 'this.tiltX = this.p011ScriptTargetTilt' in canvas),
    ]
    result = {
        'dateUtc': datetime.now(timezone.utc).isoformat(),
        'routeId': 'bottom-creep-v2.8-heavy-interface',
        'scope': 'source inventory only; no transpilation, runtime or device execution',
        'commit': subprocess.check_output([GIT, 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        'sources': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in paths},
        'inventory': [{'id': name, 'present': present} for name, present in checks],
        'runtimeCases': [{'id': f'TC-P011-MODE-{i:02}', 'verdict': 'BLOCKED',
                          'reason': 'Requires reviewed dual-mode contract, implementation and runtime evidence'}
                         for i in range(1, 8)],
        'verdict': 'RED_BASELINE_NOT_ACCEPTED',
    }
    destination = Path(args.output)
    with destination.open('x', encoding='utf-8') as stream:
        json.dump(result, stream, indent=2, ensure_ascii=False)
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
