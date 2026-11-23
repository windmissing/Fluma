"""Independent raw-word and ordering gate for T854's private OP17 split."""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
BUFFERS = ('BASE', 'CANDIDATE', 'WORK', 'CTRL')
PLAN = json.loads((ROOT / 'docs/architecture-T607-g13-dispatch-v1.json').read_bytes())['quantum']
# The plan labels actual/replay as 1/2. The shader indexes them as 0/1.
SLOTS = {(17, r['scope']-1, r['J'], r['micro'], r['iterationBlock'], r['index'])
         for r in PLAN if r['opcode'] == 17}
FAULT = set(range(513, 518)) | {526, 766} | set(range(733, 751))
MOTION = set(range(624, 642)) | set(range(820, 828))
SEAL = 0xc0de1311
TRACE_SEAL = 0xc0de1301


def need(ok, code):
    if not ok:
        raise ValueError(code)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encode(words):
    return struct.pack(f'<{len(words)}I', *words)


def snapshot(buf):
    return {n: {'words': buf[n], 'sha256': sha(encode(buf[n]))} for n in BUFFERS}


def decode(rows, binding):
    need(set(rows) == set(BUFFERS), 'BUFFERS')
    result = {}
    for n in BUFFERS:
        words = rows[n]['words']
        data = encode(words)
        need(len(data) == binding[n]['bytes'] and sha(data) == rows[n]['sha256'], 'RAW_SHA_OR_RANGE')
        result[n] = words
    return result


def delta(before, after):
    return {n: {i for i, (a, b) in enumerate(zip(before[n], after[n], strict=True)) if a != b}
            for n in BUFFERS}


def f32(word):
    return struct.unpack('<f', struct.pack('<I', word))[0]


def word(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def rounded(x):
    return f32(word(x))


def u64(words, i):
    return words[i] | words[i+1] << 32


def kinetic(buf, layout):
    nx, ny, nf = layout[0], layout[1], layout[3]
    nu = (nx+1)*ny
    work0 = layout[16]
    c, w = buf['CTRL'], buf['WORK']
    need(nx > 0 and ny > 0 and nf == nu + nx*(ny+1) and work0+nf <= len(w), 'FACE_LAYOUT')
    scale = rounded(rounded(.5 * f32(c[10])) * f32(c[10]))
    lanes = [0.] * 64
    for f in range(nf):
        if f < nu:
            x = f % (nx+1)
            active = x < nx and (x > 0 or c[6] in (2, 3))
        else:
            y = (f-nu)//nx
            active = y < ny and (y > 0 or c[6] == 2)
        if active:
            q = f32(w[work0+f])
            lanes[f%64] = rounded(lanes[f%64] + rounded(rounded(scale*q)*q))
    for stride in (32, 16, 8, 4, 2, 1):
        for i in range(stride):
            lanes[i] = rounded(lanes[i] + lanes[i+stride])
    return word(lanes[0])


def first_error(old, new, stage, push, bits):
    need(all(b in (1, 2, 4) for b in bits), 'MATH_BITS')
    mask = old[751]
    for bit in bits:
        mask |= bit
    need(new[751] == mask, 'MATH_ACCUMULATION')
    if old[514] or not bits:
        need(all(old[i] == new[i] for i in FAULT), 'FIRST_ERROR_OVERWRITTEN')
        return
    need(new[513:518] == [17, 17, *push[1:4]] and new[526] == 1 and new[766] == SEAL
         and new[733:735] == old[136:138] and new[750] == bits[0], 'FIRST_ERROR_STAGE')
    observed = new[736]
    need(1 <= new[735] <= 4 and 0 < observed < 16
         and all(observed & flag or not any(new[a:b])
                 for flag, a, b in ((1,737,741), (4,741,745), (8,745,746), (2,746,750))),
         'FIRST_ERROR_DIAGNOSTICS')
    if stage == 'MMS' and new[750] == 4:
        need(new[735] == 4, 'EXP_CONTEXT')


def motion_branch(base, candidate, ctrl, j, micro):
    tick = u64(base, 18)
    start = u64(candidate, 24)
    mode = candidate[30]
    need(start <= tick and mode in (0, 1, 2, 3), 'MOTION_INPUT')
    time = Fraction(tick-start, 120)
    if ctrl[520] < 50:
        time += Fraction(2*micro+1, 240*j)
    return mode in (0, 1) or (mode == 2 and time >= f32(candidate[28]))


def check(row, synthetic=False):
    need(row['synthetic'] is synthetic, 'EVIDENCE_MODE')
    push, ids, binding = tuple(row['push']), row['identity'], row['bindings']
    need(push in SLOTS, 'OP17_SLOT')
    need(all(ids[k] for k in ('pid', 'processStart', 'owner', 'device', 'generation', 'permit'))
         and 0 < int(ids['nonce']) < 2**64, 'ROOT_IDENTITY')
    need(set(binding) == set(BUFFERS) and len({b['object'] for b in binding.values()}) == 4
         and all(b['offset'] >= 0 and b['offset'] % 4 == 0 and 0 < b['bytes'] <= 4*1024*1024
                 and b['bytes'] % 4 == 0 for b in binding.values()), 'BINDINGS')
    events = row['events']
    need(all(e['identity'] == ids and e['bindings'] == binding and tuple(e['push']) == push for e in events),
         'SCOPE_NONCE_OR_BINDINGS')
    need(row['publicationBefore'] == row['publicationAfter'] and row['completionPublished'] is False,
         'PRIVATE_PUBLICATION')
    names = [e['stage'] for e in events]
    order = ['MOTION', 'BARRIER', 'MMS', 'BARRIER', 'LEDGER']
    if row['revoked']:
        need(names in [order[:n]+['REVOKE', 'FENCE'] for n in (1, 2, 3, 4)]
             and events[-1]['vkResult'] == row['submitVkResult'] == 0
             and row['reference'] is None, 'REVOKED_CONSUMER_OR_RETIRE')
    else:
        need(names == order+['FENCE'] and events[-1]['vkResult'] == row['submitVkResult'] == 0, 'STAGE_ORDER')
    for e in events:
        if e['stage'] == 'BARRIER':
            need(e['dependency'] == 'COMPUTE_WRITE_TO_COMPUTE_READ_WRITE', 'BARRIER')

    _, scope, j, micro, _, ordinal = push
    tb = (2048, 10240)[scope]
    trace = tb + 1024*(j+micro)+520
    before = decode(row['before'], binding)
    original = before
    c0 = before['CTRL']
    layout = row['layoutWords']
    private = row['privateWord']
    ledger = layout[35]+256
    private_range = set(range(private, private+32))
    ledger_range = {ledger+i for i in (*range(49), *range(64,84), *range(96,128))}
    need(len(layout) == 80 and len(c0) >= trace+8 and min(len(before[n]) for n in BUFFERS[:2]) >= 256
         and private % 4 == 0 and private >= layout[5] and ledger >= 256
         and ledger+128 <= layout[5] <= private and private+32 <= len(before['WORK'])
         and layout[16]+layout[3] <= layout[5], 'PRIVATE_RANGE')
    need(u64(c0,136) == int(ids['nonce']), 'CTRL_NONCE')
    active = not (c0[514] or c0[518+scope] or c0[768+scope])
    outcome = 'ACTIVE' if active else 'NOOP'
    tick = u64(before['BASE'],18)
    candidate = before['CANDIDATE']
    force = candidate[204] == 3 and u64(candidate,198) <= tick < u64(candidate,200)
    whole = micro == 0 and tick % 120 == 0
    trial_counter = tb+64+64*({1:0,2:1,4:2}[j])+(8 if active else 9)
    trace_range = set(range(trace,trace+8))
    tagged = [e for e in events if e['stage'] in ('MOTION','MMS','LEDGER')]
    for phase, e in enumerate(tagged, 1):
        stage = e['stage']
        after = decode(e['after'], binding)
        diff = delta(before, after)
        old, new = before['CTRL'], after['CTRL']
        w = after['WORK']
        report = w[private:private+32]
        prior = before['WORK'][private:private+32]
        need(report[0] == row['privateTag'] != 0 and report[1] == phase
             and u64(report,2) == int(ids['nonce']) and tuple(report[4:10]) == push
             and report[10] == int(active) and report[11] == report[30] == report[31] == 0,
             'PRIVATE_STAGE_NONCE_OR_PUSH')
        need(not (diff['BASE'] or diff['CANDIDATE']), 'IMMUTABLE_STATE')
        ctrl, work = set(), private_range
        bits = e['mathBits']
        if stage == 'MOTION':
            ctrl = {522 if active else 523, trial_counter, 676, 752}
            need(new[752] == ordinal and new[676] == 0
                 and new[522+int(not active)] == (old[522+int(not active)]+1) & 0xffffffff
                 and new[trial_counter] == (old[trial_counter]+1) & 0xffffffff, 'ENTRY_COUNTS')
            if active:
                ctrl |= MOTION | FAULT | {751}
                if not bits:
                    need(new[629] == int(motion_branch(original['BASE'], candidate, old, j, micro)),
                         'MOTION_BRANCH')
            else:
                ctrl |= trace_range
        elif stage == 'MMS' and active:
            ctrl = FAULT | {751} | ({642,643} if force else set())
        elif stage == 'LEDGER' and active:
            work |= ledger_range
            ctrl = trace_range | ({544,574,576,578} if whole else set())
            k = kinetic(before, layout)
            need(w[ledger] == k and w[ledger+1:ledger+49] == old[544:592]
                 and w[ledger+64:ledger+84] == old[688:708]
                 and w[ledger+96:ledger+128] == candidate[160:192], 'LEDGER_RAW_COPY')
            if whole:
                baseline = rounded(rounded(f32(old[8])*f32(old[9]))*2**-20)
                need([new[i] for i in (544,574,576,578)] == [k,0,0,word(max(f32(k),baseline))],
                     'WHOLE_SECOND_LEDGER')
        need(diff['CTRL'] <= ctrl and diff['WORK'] <= work, stage+'_WRITESET')
        if not active or stage == 'LEDGER':
            need(not bits, 'UNEXPECTED_MATH')
        need((not active and not any(report[12:30])) or active and
             (report[12] == int(bool(report[13])) and report[17] in (0,1)
              and (stage == 'LEDGER' and report[12:30] == prior[12:30]
                   or stage != 'LEDGER' and report[13] == ((prior[13] if stage == 'MMS' else 0) | sum(set(bits))))),
             'PRIVATE_MATH_CARRY')
        first_error(old, new, stage, push, bits)
        if (stage == 'MOTION' and not active) or (stage == 'LEDGER' and active):
            flag = 1 if active else 8 if old[514] else 16 if old[762] == 1 or old[768+scope] else 2
            peak = word(max(f32(new[584]), f32(new[586])))
            need(new[trace:trace+8] == [17, 32|flag, new[514], layout[3] if active else 0,
                                       peak, new[676], new[546], TRACE_SEAL], 'ACTIVE_NOOP_TRACE')
        before = after

    if row['revoked']:
        return 'REVOKED_RETIRED_NO_PUBLICATION'
    ref = row['reference']
    if ref is None:
        return outcome+'_WRITESETS_ONLY'
    need(tuple(ref['push']) == push and ref['layoutWords'] == layout
         and ref['inputSHA'] == {n:row['before'][n]['sha256'] for n in BUFFERS}, 'REFERENCE_INPUT')
    expected = decode(ref['after'], binding)
    for n in BUFFERS:
        size = layout[5] if n == 'WORK' else len(original[n])
        need(expected[n][:size] == before[n][:size], 'MONOLITH_WORD_DIFF')
    return outcome+'_MONOLITH_EQUAL'


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    p.add_argument('--synthetic', action='store_true')
    args = p.parse_args()
    try:
        rows = json.loads(args.capture.read_bytes())
        need(isinstance(rows, list) and rows, 'EMPTY_CAPTURE')
        result = [check(row, args.synthetic) for row in rows]
    except (OSError, KeyError, ValueError, IndexError, struct.error) as error:
        p.exit(1, f'{error}\n')
    print(json.dumps(result, indent=2))
