"""Run four isolated T703 arms without overlapping ANGLE owners."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

from compare_actions import load
from compare_replay import correlate, hashes
from verify_t703 import validate

ROOT = Path(__file__).resolve().parents[2]

def run(main, replay, prefix):
    reports = ROOT/'test/reports/t496'
    outcomes = {}
    for role, exe in (('primary',main),('replay',replay)):
        env = dict(os.environ,T693_BINARY_TRACE='1',T703_BUILD_WITNESS='1',
                   T693_REPLAY_EXE=str(exe.resolve()))
        env.pop('T693_REPLAY',None)
        if role=='replay':
            env['T693_REPLAY']='1'
        for arm in ('off','on'):
            attempt=f'{prefix}-{role}-{arm}'
            call=[sys.executable,'test/t496/run_bounded.py',f'D1_{arm.upper()}',attempt]
            result=subprocess.run(call,cwd=ROOT,env=env)
            outcomes[(role,arm)]=(reports/attempt,result.returncode)
            print(f'{attempt} exit={result.returncode}',flush=True)
    if any(code for _,code in outcomes.values()):
        return 1
    main_off,main_on,replay_off,replay_on=[item[0] for item in outcomes.values()]
    assert load(main_off,'D1_OFF')==load(main_on,'D1_ON')
    assert load(replay_off,'D1_OFF')==load(replay_on,'D1_ON')
    arms=[validate(item[0]) for item in outcomes.values()]
    assert hashes(replay_off)==hashes(replay_on), 'REPLAY_BANK_EVENT_OR_COMPLETION'
    matched=[correlate(main_off,replay_off),correlate(main_on,replay_on)]
    print(json.dumps({'arms':arms,'matchedReplayRoots':matched,'productPASS':False},indent=2))
    return 0

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('primary',type=Path)
    parser.add_argument('replay',type=Path)
    parser.add_argument('prefix')
    args=parser.parse_args()
    sys.exit(run(args.primary,args.replay,args.prefix))
