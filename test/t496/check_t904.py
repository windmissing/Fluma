"""Validate T904 single-arm archives and preserve strict D1 front gates."""
import importlib.util
import json

from check_pc_d1 import ART, ROOT, D1, need

spec = importlib.util.spec_from_file_location('t905',ROOT/'test/t905/check.py')
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


def main():
    base = ART/'t904'
    receipts = {}
    for label,prefix,mode in (('replay-off','ro','REPLAY_OFF'),('replay-on','ron','REPLAY_ON'),
                              ('cold-reanchor','cr','REPLAY_OFF')):
        path = base/f'a-{label}.jsonl'
        rows = checker.strict_rows(path)
        d1 = checker.T897.d1(rows)
        need(d1['mode']==mode,'T904_MODE')
        result = checker.cold(rows,sorted(base.glob(f'a-{prefix}-*.m1root')))
        wsi, = [r for r in rows if 'm1WsiRetireVkResult' in r]
        need(wsi['m1PublicationReleased'] is True and wsi['m1WsiRetireVkResult']==wsi['m1WsiResult']==0,
             'T904_WSI_RELEASE')
        receipts[label] = dict(result,traceSHA=checker.OLD.sha(path.read_bytes()),run=d1['run'],actionRows=len(d1['actions']),
                               firstActualAction=d1['actions'][0],source=d1['source'])
    names = ('replay-off','replay-on','cold-reanchor')
    first = [receipts[k]['firstActualAction'] for k in names]
    source = [receipts[k]['source'][7] for k in names]
    need(len({x[2] for x in first})>1 and len({x['admittedQ20'] for x in source})>1,'T904_INPUT_DIFF')
    gate = checker.rejects(lambda:D1.compare({'REPLAY_OFF':{},'REPLAY_ON':{}}),'ARM_SET')
    print(json.dumps(dict(arms=receipts,archiveCount=sum(len(r['archives']) for r in receipts.values()),
                          strictState='CLOCK_OR_INPUT_DIFFERENCE',firstAction=first[0][0],
                          firstActionNs=[x[2] for x in first],source7Q20=[x['admittedQ20'] for x in source],
                          source7DeltaNs=[x['deltaNs'] for x in source],t865FourArm=gate),indent=2))


if __name__=='__main__':
    main()
