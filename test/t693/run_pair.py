"""Run the two original arms in separate fresh processes, sequentially."""
import os
from pathlib import Path
import subprocess
import sys

base = Path(__file__).resolve().parents[2]
attempt = sys.argv[1]
env = dict(os.environ,T693_BINARY_TRACE='1')
for arm,suffix in [('D1_OFF','off'),('D1_ON','on')]:
    subprocess.run([sys.executable,'test/t496/run_bounded.py',arm,f'{attempt}-{suffix}'],
                   cwd=base,env=env,check=True)
subprocess.run([sys.executable,'test/t693/compare_actions.py',
                f'test/reports/t496/{attempt}-off',f'test/reports/t496/{attempt}-on'],
               cwd=base,check=True)
