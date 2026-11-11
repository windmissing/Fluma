import json
import subprocess
from pathlib import Path

checker = 'test/t450/install_input.py'
old = json.loads(Path('test/reports/t257/T450-t410.json').read_text())
new = json.loads(Path('test/reports/t257/T467-t410-final.json').read_text())
other = json.loads(Path('test/reports/t257/T467-t437-final.json').read_text())
base = ['python', '-X', 'utf8', checker, 't410', new['signed']['path'],
        '--receipt', 'test/reports/t257/T467-t410-final.json']
result = subprocess.run(base, capture_output=True, text=True)
assert result.returncode == 0, result.stderr
value = json.loads(result.stdout)
assert value['sha256'] == new['signed']['sha256']
assert value['launch'][-1] == new['signed']['sha256']
assert value['install'][-1] == str(Path(new['signed']['path']).resolve())
result = subprocess.run(base + ['--declared-sha', old['signed']['sha256']], capture_output=True, text=True)
assert result.returncode != 0 and 'COLD_WANT_SHA_MISMATCH' in result.stderr
result = subprocess.run(base + ['--declared-sha', new['signed']['sha256']], capture_output=True, text=True)
assert result.returncode == 0, result.stderr
result = subprocess.run(['python', '-X', 'utf8', checker, 't410', other['signed']['path'],
                         '--receipt', 'test/reports/t257/T467-t437-final.json'],
                        capture_output=True, text=True)
assert result.returncode != 0 and 'SIGNING_RECEIPT_TARGET_MISMATCH' in result.stderr
result = subprocess.run(['python', '-X', 'utf8', checker, 't410', old['signed']['path']],
                        capture_output=True, text=True)
assert result.returncode == 0, result.stderr
result = subprocess.run(['python', '-X', 'utf8', checker, 't437', other['signed']['path'],
                         '--receipt', 'test/reports/t257/T467-t437-final.json'],
                        capture_output=True, text=True)
assert result.returncode == 0, result.stderr
assert json.loads(result.stdout)['launch'][-1] == other['signed']['sha256']
print(json.dumps({'schema': 'T483_INSTALL_INPUT_CHECKS_V1', 'passed': 6, 'failed': 0,
                  'device': 'NOT_RUN'}))
